#!/usr/bin/ruby

require 'rbconfig'
require 'erb'
require 'etc'
require 'logger'

require 'verse'


# A collection of mixins shared between Verse classes. Stolen mostly
# from ThingFish.

module Verse

	### Add logging to a Verse class. Including classes get #log and #log_debug methods.
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


	### A collection of functions for manipulating and comparing versions.
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


	### A mixin for adding data and structure versioning.
	module Versioned

		### Set up versioning data
		def initialize( *args )
			@data_version      = 0
			@structure_version = 0
		end


		######
		public
		######

	 	# The current version of the internal state. This value is updated for any 
		# kind of change.
		attr_reader :data_version

		# The current version of the internal structure. This value is only updated 
		# when the internal structure changes.
		attr_reader :structure_version


		### Update the version for the internal state.
		def update_data_version
			@data_version += 1
		end


		### Update both the version for both the internal state and the structure.
		def update_structure_version
			self.update_data_version
			@structure_version += 1
		end


		#########
		protected
		#########

		### Return a fragment of an object's #inspect output for inclusion in objects 
		### that include this mixin.
		def inspect_fragment
			return "version: %d.%d" % [
				self.structure_version,
				self.data_version
			]
		end

	end # module Versioned


	### A mixin for adding the ability to observe 
	module Observable
		
		
	end

end # module Verse

# vim: set nosta noet ts=4 sw=4:

