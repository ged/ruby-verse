#!/usr/bin/env ruby


# A Ruby binding for the Verse network protocol library.
module Verse

	# The library version
	VERSION = '1.0.0'

	# The library revision
	REVISION = %q$Revision$

	# Load the logformatters and some other stuff first
	require 'verse/utils'
	require 'verse/mixins'
	require 'verse/constants'

	extend Verse::VersionUtilities

 	if vvec( RUBY_VERSION ) < vvec( '1.9.1' )
		warn ">>> This library was written for Ruby 1.9.1. It may or may not work " +
		     "with earlier versions. <<<"
	end


	### Logging 
	@default_logger = Logger.new( $stderr )
	@default_logger.level = $DEBUG ? Logger::DEBUG : Logger::WARN

	@default_log_formatter = Verse::LogFormatter.new( @default_logger )
	@default_logger.formatter = @default_log_formatter

	@logger = @default_logger


	class << self
		# The log formatter that will be used when the logging subsystem is reset
		attr_accessor :default_log_formatter

		# The logger that will be used when the logging subsystem is reset
		attr_accessor :default_logger

		# The logger that's currently in effect
		attr_accessor :logger
		alias_method :log, :logger
		alias_method :log=, :logger=
	end


	### Reset the global logger object to the default
	def self::reset_logger
		self.logger = self.default_logger
		self.logger.level = Logger::WARN
		self.logger.formatter = self.default_log_formatter
	end


	### Returns +true+ if the global logger has not been set to something other than
	### the default one.
	def self::using_default_logger?
		return self.logger == self.default_logger
	end


	### Return the library's version string
	def self::version_string( include_buildnum=false )
		vstring = "%s %s" % [ self.name, VERSION ]
		vstring << " (build %s)" % [ REVISION[/.*: ([[:xdigit:]]+)/, 1] || '0' ] if include_buildnum
		return vstring
	end


	### Return the version string from the Verse library.
	def self::library_version
		return "r%dp%d%s" %
			[ Verse::RELEASE_NUMBER, Verse::RELEASE_PATCH, Verse::RELEASE_LABEL ]
	end


	require 'verse/session'


	# Load the extension, prepending the version directory for Windows
	if RUBY_PLATFORM =~/(mswin|mingw)/i
		major_minor = RUBY_VERSION[ /^(\d+\.\d+)/ ] or
			raise "Oops, can't extract the major/minor version from #{RUBY_VERSION.dump}"
		require "#{major_minor}/verse_ext"
	else
		require 'verse_ext'
	end

end # module Verse

