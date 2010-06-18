#!/usr/bin/env ruby1.9

BEGIN {
	require 'pathname'

	basedir = Pathname( __FILE__ ).dirname.parent
	libdir = basedir + 'lib'

	$LOAD_PATH.unshift( libdir.to_s )
}

require 'verse'
require 'logger'

Verse.logger.level = Logger::DEBUG

class Client
	include Verse::SessionObserver

	PORT = 4950

	def initialize
		@session = Verse::Session.new( "localhost:#{PORT}" )
		@session.add_observer( self )
		@my_avatar = nil
		@running = false
	end

	def start
		puts "Connecting..."
		@session.connect( 'user', 'pass' )
		@running = true
		Verse::Session.update while @running
		puts "Disconnected."
	end

	def shutdown( *args )
		puts "Shutting down..."
		@running = false
	end

	def on_connect_accept( avatar, address, host_id )
		puts "Got connect_accept: %p" % [[ avatar, address, host_id ]]
		@avatar = avatar
	end

	def on_connect_terminate( address, msg )
		puts "Got connect_terminate: %p" % [[ address, msg ]]
	end

end # class Client

c = Client.new
Signal.trap( 'INT' ) { c.shutdown }
c.start

