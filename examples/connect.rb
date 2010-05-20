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
running = false

v = Verse::Session.new( 'localhost' )
v.connect( 'user', 'pass' )
v.on_connect_accept do |*args|
	$stderr.puts "on_connect_accept: %p" % [ args ]
end
v.on_connect_terminate do |*args|
	$stderr.puts "on_connect_terminate: %p" % [ args ]
	running = false
end

Signal.trap( :INT ) { running = false }

running = true
while running
	Verse.callback_update( 0.1 )
end


