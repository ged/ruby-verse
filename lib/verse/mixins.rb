#!/usr/bin/ruby

require 'rbconfig'
require 'erb'
require 'etc'
require 'logger'

require 'verse'


# A collection of mixins shared between Verse classes. Stolen mostly
# from ThingFish.

module Verse

	# Add logging to a Verse class. Including classes get #log and #log_debug methods.
	module Loggable

		LEVEL = {
			:debug => Logger::DEBUG,
			:info  => Logger::INFO,
			:warn  => Logger::WARN,
			:error => Logger::ERROR,
			:fatal => Logger::FATAL,
		  }

		### A logging proxy class that wraps calls to the logger into calls that include
		### the name of the calling class.
		class ClassNameProxy # :nodoc:

			### Create a new proxy for the given +klass+.
			def initialize( klass, force_debug=false )
				@classname   = klass.name
				@force_debug = force_debug
			end

			### Delegate calls the global logger with the class name as the 'progname' 
			### argument.
			def method_missing( sym, msg=nil, &block )
				return super unless LEVEL.key?( sym )
				sym = :debug if @force_debug
				Verse.logger.add( LEVEL[sym], msg, @classname, &block )
			end
		end # ClassNameProxy

		#########
		protected
		#########

		### Return the proxied logger.
		def log
			@log_proxy ||= ClassNameProxy.new( self.class )
		end

		### Return a proxied "debug" logger that ignores other level specification.
		def log_debug
			@log_debug_proxy ||= ClassNameProxy.new( self.class, true )
		end
	end # module Loggable


	# A collection of functions for manipulating and comparing versions.
	module VersionUtilities

		###############
		module_function
		###############

		### Convert a version string to a Comparable binary vector.
		def vvec( version_string )
			return version_string.split( '.' ).
				collect {|num| num.to_i }.pack( 'N*' )
		end

	end # module VersionUtilities

end # module Verse

# vim: set nosta noet ts=4 sw=4:

