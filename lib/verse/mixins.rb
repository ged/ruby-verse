#!/usr/bin/ruby

require 'rbconfig'
require 'erb'
require 'etc'
require 'logger'

require 'verse'


# A collection of mixins shared between Verse classes. Some parts of this source
# code were stolen from ThingFish under the BSD license.
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

		### Extension callback -- add the versioning instance variables to the
		### extending object.
		def self::extended( object )
			object.instance_variable_set( :@data_version, 0 )
			object.instance_variable_set( :@structure_version, 0 )
			super
		end

		#
		# Instance methods
		#

		### Set up versioning data
		def initialize( *args )
			@data_version      = 0
			@structure_version = 0
			super
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


		### Return a fragment of an object's #inspect output for inclusion in objects 
		### that include this mixin.
		def inspect
			rval = super
			return rval unless self.structure_version
			vstring = "%d.%d" % [ self.structure_version, self.data_version ]
			return rval.sub( />$/, "; version: #{vstring}>" )
		end

	end # module Versioned


	### A mixin for making Verse objects observable.
	module Observable
		include Verse::Versioned,
		        Verse::Loggable

		### Set up observable data structures.
		def initialize( *args )
			@observers = []
			super
		end


		######
		public
		######

		### The object's observers
		### @return [Array<Verse::Observer>]  the objects that would like to be notified about 
		###                                   changes in the receiver.
		attr_reader :observers


		### Add an observer.
		### 
		### @param [Observer] observer  the object that would like to be notified about 
		###                             changes in the receiver.
		def add_observer( observer )
			self.log.debug "%p: adding observer %p" % [ self, observer ]
			@observers << observer unless @observers.include?( observer )
		end


		### Remove an observer.
		### 
		### @param [Observer] observer   the object that is no longer interested in being
		###                              notified about changes in the receiver.
		def remove_observer( observer )
			self.log.debug "%p: removing observer %p" % [ self, observer ]
			@observers.delete( observer )
		end


		### Remove all current observers.
		### @return [Array<Verse::Observer>]  the observers that were removed
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
		### @param [Array<Observable>] objects the objects to no longer observe.
		def stop_observing( *objects )
			objects.each {|obj| obj.remove_observer(self) }
		end

	end # module Observer


	### A mixin for objects which wish to observe ping events.
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


	### A mixin for objects which wish to observe Verse connection events.
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


		### Called when a Verse client requests subscriptions to create/destroy events
		### for the specified node +classes+.
		### 
		### @param [Array<Class>] classes  the Verse::Node classes to subscribe to.
		def on_node_index_subscribe( *classes )
			classes.flatten!
			if classes.empty?
				self.log.debug "unhandled node_index_subscribe: clear subscriptions"
			else
				self.log.debug "unhandled node_index_subscribe: classes: %s" %
					[ classes.collect {|kl| kl.name }.join(', ') ]
			end
		end

	end # module ConnectionObserver


	### A mixin for objects which wish to observe events on a Verse::Session.
	module SessionObserver
		include Verse::Loggable,
		        Verse::Observer

		### Called when a Verse server accepts a connection from a Verse::Session.
		### 
		### @param [Verse::Node]  avatar   the node in the object graph assigned by the server
		###                                to the session
		### @param [String]       address  the address (in "<ipaddr>:<port>" form) of the
		###                                accepting server
		### @param [String]       hostid   the server's key, which can be used to verify its
		###                                identity when reconnecting later
		def on_connect_accept( avatar, address, hostid )
			self.log.debug "unhandled on_connect_accept: connection accepted from '%s' (%s)" %
				[ address, hostid ]
		end


		### Called when a Verse server notifies a client that it's terminating its connection.
		### 
		### @param [String]  address  the address of the terminating client
		### @param [String]  message  the termination message
		def on_connect_terminate( address, message )
			self.log.debug "unhandled on_connect_terminate: '%s' terminated communication: %s" %
				[ address, message ]
		end


		### Called when a new node is created (or indexed) on the server.
		def on_node_create( node )
			self.log.debug "unhandled on_node_create for %p" % [ node ]
		end


		### Called when a node is destroyed on the server.
		def on_node_destroy( node )
			self.log.debug "unhandled on_node_destroy for %p" % [ node ]
		end

	end # module SessionObserver


	### A mixin for objects which wish to observe events on a Verse::Node.
	module NodeObserver
		include Verse::Loggable,
		        Verse::Observer

		### Called when the verse server sets the node's name.
		### 
		### @param [Verse::Node] node  the node that has just had its name set/changed.
		### @param [String] name       the node's new name.
		def on_node_name_set( node, name )
			self.log.debug "unhandled on_node_name_set: %p is now named %p" % [ node, name ]
		end

		### Called when a new +tag_group+ is created for the +node+.
		### 
		### @param [Verse::Node] node                 the node the tag group belongs to
		### @param [Verse::Node::TagGroup] tag_group  the new tag group
		def on_tag_group_create( node, tag_group )
			self.log.debug "unhandled on_tag_group_create: node %p now has tag group %p" %
			 	[ node, tag_group ]
		end

		### Called when a +tag_group+ is destroyed.
		### 
		### @param [Verse::Node] node                 the node the tag group used to belong to.
		### @param [Verse::Node::TagGroup] tag_group  the tag group that has been destroyed.
		def on_tag_group_destroy( node, tag_group )
			self.log.debug "unhandled on_tag_group_destroy: node %p no longer has tag group %p" %
			 	[ node, tag_group ]
		end

		### Called when the verse server sets the node's name.
		### 
		### @param [Verse::Node] node  the node that has just had its name set/changed.
		### @param [String] name       the node's new name.
		def on_tag_group_subscribe( node )
			self.log.debug "unhandled on_node_destroy for %p" % [ node ]
		end

		### Called when the verse server sets the node's name.
		### 
		### @param [Verse::Node] node  the node that has just had its name set/changed.
		### @param [String] name       the node's new name.
		def on_tag_group_unsubscribe( node )
			self.log.debug "unhandled on_node_destroy for %p" % [ node ]
		end

		### Called when the verse server sets the node's name.
		### 
		### @param [Verse::Node] node  the node that has just had its name set/changed.
		### @param [String] name       the node's new name.
		def on_tag_create( node )
			self.log.debug "unhandled on_node_destroy for %p" % [ node ]
		end

		### Called when the verse server sets the node's name.
		### 
		### @param [Verse::Node] node  the node that has just had its name set/changed.
		### @param [String] name       the node's new name.
		def on_tag_destroy( node )
			self.log.debug "unhandled on_node_destroy for %p" % [ node ]
		end

	end


	### A collection of ANSI color utility functions
	module ANSIColorUtilities

		# Set some ANSI escape code constants (Shamelessly stolen from Perl's
		# Term::ANSIColor by Russ Allbery <rra@stanford.edu> and Zenin <zenin@best.com>
		ANSI_ATTRIBUTES = {
			'clear'      => 0,
			'reset'      => 0,
			'bold'       => 1,
			'dark'       => 2,
			'underline'  => 4,
			'underscore' => 4,
			'blink'      => 5,
			'reverse'    => 7,
			'concealed'  => 8,

			'black'      => 30,   'on_black'   => 40,
			'red'        => 31,   'on_red'     => 41,
			'green'      => 32,   'on_green'   => 42,
			'yellow'     => 33,   'on_yellow'  => 43,
			'blue'       => 34,   'on_blue'    => 44,
			'magenta'    => 35,   'on_magenta' => 45,
			'cyan'       => 36,   'on_cyan'    => 46,
			'white'      => 37,   'on_white'   => 47
		}

		###############
		module_function
		###############

		### Create a string that contains the ANSI codes specified and return it
		def ansi_code( *attributes )
			attributes.flatten!
			attributes.collect! {|at| at.to_s }
			return '' unless /(?:vt10[03]|xterm(?:-color)?|linux|screen)/i =~ ENV['TERM']
			attributes = ANSI_ATTRIBUTES.values_at( *attributes ).compact.join(';')

			if attributes.empty?
				return ''
			else
				return "\e[%sm" % attributes
			end
		end


		### Colorize the given +string+ with the specified +attributes+ and return it, handling 
		### line-endings, color reset, etc.
		def colorize( *args )
			string = ''

			if block_given?
				string = yield
			else
				string = args.shift
			end

			ending = string[/(\s)$/] || ''
			string = string.rstrip

			return ansi_code( args.flatten ) + string + ansi_code( 'reset' ) + ending
		end

	end # module ANSIColorUtilities


end # module Verse

# vim: set nosta noet ts=4 sw=4:

