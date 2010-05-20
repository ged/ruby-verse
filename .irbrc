#!/usr/bin/ruby -*- ruby -*-

BEGIN {
	require 'rbconfig'
	require 'pathname'
	basedir = Pathname.new( __FILE__ ).dirname.expand_path
	libdir = basedir + "lib"

	puts ">>> Adding #{libdir} to load path..."
	$LOAD_PATH.unshift( libdir.to_s )
}


begin
	$stderr.puts "Loading Verse..."
	require 'logger'
	require 'verse'

	Verse.log.level = Logger::DEBUG if $DEBUG
rescue => e
	$stderr.puts "Ack! Verse library failed to load: #{e.message}\n\t" +
		e.backtrace.join( "\n\t" )
end

