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


	### A mixin for making Verse objects observable.
	module Observable

		### Set up observable data structures.
		def initialize( *args )
			@observers = []
			super
		end


		######
		public
		######

		### The object's observers
		### @return [Array<Observer>]  the Array of observer.
		attr_reader :observers


		### Add an observer.
		### 
		### @param [Observable] observer  the object that is interested in events from
		###                               the receiver.
		def add_observer( observer )
			@observers << observer
		end


		### Remove an observer.
		### 
		### @param [Observable] observer  the object that is no longer interested in events
		###                               from the receiver.
		def remove_observer( observer )
			@observers.delete( observer )
		end


		### Remove all current observers.
		### @returns [Array<Verse::Observer>]  the observers that were removed
		def remove_observers
			@observers.collect {|observer| self.remove_observer(observer) }
		end

	end # module Observable


	### A mixin for making an object capable of observing one or more 
	### Verse::Observable objects.
	module Observer

		### Start observing the specified +objects+.
		### 
		### @param [Array<Observable>] objects  The objects to observe.
		def observe( *objects )
			objects.each {|obj| obj.add_observer(self) }
		end


		### Remove the receiver from the list of interested observers from
		### the given +objects+.
		### 
		### @params [Array<Observable>] objects the objects to no longer observe.
		def stop_observing( *objects )
			objects.each {|obj| obj.remove_observer(self) }
		end

	end # module Observer


	### A mixin for objects which wish to handle 'ping' events.
	module PingObserver
		include Verse::Loggable,
		        Verse::Observer

		### Called when a ping is received from +address+.
		###
		### @param [String]  address  the address of the sender, in `<host>:<port>` form.
		### @param [String]  message  the 'payload' of the ping message.
		def on_ping( address, message )
			self.log.debug "unhandled on_ping: got '%s' from '%s'" % [ message, address ]
		end

	end # PingObserver


	### A mixin for objects which wish to handle 'connect' and 'connect_terminate' events.
	module ConnectionObserver
		include Verse::Loggable,
		        Verse::Observer

		### Called when a Verse client connects.
		### 
		### @param [String]  user     the username provided by the connecting client
		### @param [String]  pass     the password provided by the connecting client
		### @param [String]  address  the address of the connecting client
		### @param [String]  hostid   the expected hostid provided by the connecting client
		def on_connect( user, pass, address, hostid )
			self.log.debug "unhandled on_connect: '%s' connected from '%s'" % [ user, address ]
		end


		### Called when a Verse client notifies the server that it wishes to terminate
		### communication.
		### 
		### @param [String]  address  the address of the terminating client
		### @param [String]  message  the termination message
		def on_connect_terminate( address, message )
			self.log.debug "unhandled on_connect_terminate: '%s' terminated communication: %s" % 
				[ address, message ]
		end

	end


end # module Verse

# vim: set nosta noet ts=4 sw=4:

