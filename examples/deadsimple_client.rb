#!/usr/bin/env ruby1.9

BEGIN {
	require 'pathname'

	basedir = Pathname( __FILE__ ).dirname.parent
	libdir = basedir + 'lib'

	$LOAD_PATH.unshift( libdir.to_s )
}

require 'verse'
require 'logger'
require 'pathname'

HOST_ID_FILE = Pathname( "hostkey.rsa" )
PORT = 4555

Verse.logger.level = Logger::DEBUG

session = Verse::Session.new( "localhost:#{PORT}" )
my_avatar = nil
running = false

print "Connecting..."
session.connect( 'user', 'pass' )
session.on_connect_accept do |avatar|
	puts "connected."
	my_avatar = avatar
end
session.on_connect_terminate do |address, message|
	puts "#{address} terminated connection: #{message}"
	running = false
end

signal_handler = lambda { running = false }
Signal.trap( 'INT', &signal_handler )
Signal.trap( 'TERM', &signal_handler )

running = true
while running
	session.callback_update( 1 )
end
puts "Disconnected."

