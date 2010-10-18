#!/usr/bin/ruby
# coding: utf-8

BEGIN {
	require 'rbconfig'
	require 'pathname'
	basedir = Pathname.new( __FILE__ ).dirname.parent

	libdir = basedir + "lib"
	extdir = libdir + Config::CONFIG['sitearch']

	$LOAD_PATH.unshift( basedir ) unless $LOAD_PATH.include?( basedir )
	$LOAD_PATH.unshift( libdir.to_s ) unless $LOAD_PATH.include?( libdir.to_s )
	$LOAD_PATH.unshift( extdir.to_s ) unless $LOAD_PATH.include?( extdir.to_s )
}

require 'pp'
require 'yaml'
require 'verse'

require 'spec/lib/constants'


### RSpec helper functions.
module Verse::SpecHelpers
	include Verse::TestConstants

	class ArrayLogger
		### Create a new ArrayLogger that will append content to +array+.
		def initialize( array )
			@array = array
		end

		### Write the specified +message+ to the array.
		def write( message )
			@array << message
		end

		### No-op -- this is here just so Logger doesn't complain
		def close; end

	end # class ArrayLogger


	unless defined?( LEVEL )
		LEVEL = {
			:debug => Logger::DEBUG,
			:info  => Logger::INFO,
			:warn  => Logger::WARN,
			:error => Logger::ERROR,
			:fatal => Logger::FATAL,
		  }
	end

	###############
	module_function
	###############

	### Reset the logging subsystem to its default state.
	def reset_logging
		Verse.reset_logger
	end


	### Alter the output of the default log formatter to be pretty in SpecMate output
	def setup_logging( level=Logger::FATAL )

		# Turn symbol-style level config into Logger's expected Fixnum level
		if Verse::Loggable::LEVEL.key?( level )
			level = Verse::Loggable::LEVEL[ level ]
		end

		logger = Logger.new( $stderr )
		Verse.logger = logger
		Verse.logger.level = level

		# Only do this when executing from a spec in TextMate
		if ENV['HTML_LOGGING'] || (ENV['TM_FILENAME'] && ENV['TM_FILENAME'] =~ /_spec\.rb/)
			Thread.current['logger-output'] = []
			logdevice = ArrayLogger.new( Thread.current['logger-output'] )
			Verse.logger = Logger.new( logdevice )
			# Verse.logger.level = level
			Verse.logger.formatter = Verse::HtmlLogFormatter.new( Verse.logger )
		elsif $stderr.tty?
			Verse.logger.formatter = Verse::ColorLogFormatter.new( Verse.logger )
		end
	end

end

# vim: set nosta noet ts=4 sw=4:

