#!/usr/bin/env ruby1.9
#coding: utf-8

BEGIN {
	require 'pathname'

	basedir = Pathname( __FILE__ ).dirname.parent
	libdir = basedir + 'lib'

	$LOAD_PATH.unshift( libdir.to_s )
}

require 'pp'
require 'verse'
require 'logger'
require 'pathname'

HOST_ID_FILE = Pathname( "hostkey.rsa" )
PORT = 4555

Verse.logger.level = Logger::DEBUG

sessions = {}
my_hostid = nil

Verse.port = PORT
if HOST_ID_FILE.exist?
	puts "Using existing hostid..."
	my_hostid = HOST_ID_FILE.read( encoding: "ascii-8bit" )
else
	puts "Creating a new hostid..."
	my_hostid = Verse.create_host_id
	HOST_ID_FILE.open( File::WRONLY|File::CREAT|File::EXCL, encoding: "ascii-8bit" ) do |fh|
		fh.print( my_hostid )
	end
end
Verse.host_id = my_hostid


class Server
	require 'digest/sha1'

	include Verse::ConnectionObserver

	USERS = { 'bargle' => 'a76282512cb523ec1f82c79b4f51e34cdd5f11ea' }

	def initialize
		@connections = {}
	end

	def on_connect( name, pass, address, hostid )
	    Verse.connect_terminate( address, "wrong hostid" ) unless hostid == my_hostid
		Verse.connect_terminate( address, "auth failed" ) unless self.authenticate( user, pass )
	    avatar = Verse::ObjectNode.new
	    session = Verse.connect_accept( avatar, address, my_hostid )
	    @connections[ address ] = { :avatar => avatar, :session => session }
	end

	def authenticate( user, pass )
		return USERS[ user ] == Digest::SHA1.hexdigest( pass )
	end

	def on_connect_terminate( address, message )
		puts "#{address} terminated connection: #{message}"
		conn = @connections.delete( address )
		conn[:avatar].destroy
	end

end # class Server

signal_handler = lambda {|signal| $running = false; Signal.trap(signal, 'IGN') }
Signal.trap( 'INT', &signal_handler )

puts "Listening on port #{PORT}"
$running = true
Verse.update while $running
puts "Shutting down."

