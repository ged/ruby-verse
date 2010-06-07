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

	PORT = 4555

	def initialize
		@session = Verse::Session.new( "localhost:#{PORT}" )
		@my_avatar = nil
		@running = false
	end

	def start
		puts "Connecting..."
		@session.connect( 'user', 'pass' )
		@running = true
		Verse.update while @running
		puts "Disconnected."
	end

	def shutdown( *args )
		puts "Shutting down..."
		@running = false
	end

	def on_connect_accept( avatar )
		puts "connected."
		@my_avatar = avatar
	end

end # class Client

Client.new.start

