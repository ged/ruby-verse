#!/usr/bin/env ruby

require 'pathname'

require 'verse'
require 'verse/mixins'
require 'verse/utils'

class Verse::SimpleServer < Verse::Server
	include Verse::Loggable,
	        Verse::PingObserver,
	        Verse::SessionObserver

	# The path to the server's saved hostid
	HOSTID_FILE = Pathname( 'hostid.rsa' )

	# Logging level names and corresponding constants
	LOG_LEVELS = {
		'debug' => Logger::DEBUG,
		'info'  => Logger::INFO,
		'warn'  => Logger::WARN,
		'fatal' => Logger::FATAL,
	}

	# The hostid that's sent if the client hasn't specified one
	WILDCARD_HOSTID = "\x0" * Verse::HOST_ID_SIZE

	# Connection struct -- stores connection details in the @connections hash
	Connection = Struct.new( "VerseServerConnection", :address, :user, :session, :avatar,
	                         :index_subscriptions )


	#################################################################
	###	I N S T A N C E   M E T H O D S
	#################################################################

	### Create the Server instance.
	def initialize
		@host_id     = nil
		@connections = {}
		@nodes       = {}
		@state       = :stopped
	end


	######
	public
	######

	# The server's host id
	attr_reader :host_id

	# The hash of nodes registered with the server
	attr_reader :nodes

	# A Verse::Server::Connection per session, keyed by address.
	attr_reader :connections

	# The Verse::Nodes that are being managed by the server, keyed by id.
	attr_reader :nodes


	### Set the server up using the specified +options+ and start listening for events.
	def run( options )
		Verse.logger.level = LOG_LEVELS[ options.loglevel ]
		Verse.port = options.port

		@host_id = self.load_host_id

		self.set_signal_handlers

		self.log.info "Starting run loop"
		@state = :running
		Verse.update until @state != :running
	end


	### Shut the server down using the specified +reason+.
	def shutdown( reason="No reason given." )
		self.reset_signal_handlers
		self.log.warn "Server shutdown: #{reason}"

		@connections.keys.each do |addr|
			conn = @connections.delete( addr )
			self.stop_observing( conn.session )
			Verse.terminate_connection( addr, reason )
		end

		@state = :shutdown
	end


	#########
	protected
	#########

	### Set up signal handlers to restart/shutdown the server.
	def set_signal_handlers
		Signal.trap( :INT ) { self.shutdown("Caught interrupt.") }
		Signal.trap( :TERM ) { self.shutdown("Terminated.") }
		Signal.trap( :HUP ) { self.shutdown("Hangup.") }
	end


	### Set up signal handlers to restart/shutdown the server.
	def reset_signal_handlers
		Signal.trap( :INT, 'DEFAULT' )
		Signal.trap( :TERM, 'DEFAULT' )
		Signal.trap( :HUP, 'DEFAULT' )
	end


	### Load the hostid from the saved file if it exists. If it doesn't, generate a new one
	### and save it before returning it.
	def load_host_id
		if HOSTID_FILE.exist?
			return HOSTID_FILE.read( encoding: 'ascii-8bit' )
		else
			hostid = Verse.create_host_id
			mode = File::WRONLY|File::CREAT|File::EXCL
			HOSTID_FILE.open( mode, 0600, encoding: "ascii-8bit" ) do |ofh|
				ofh.write( hostid )
			end
			return hostid
		end
	end


	### Add a node to the server, assigning it an ID and storing it.
	def add_node( node )
		node.id = node.object_id
		@nodes[ node.object_id ] = node
	end


	### Remove a node from the server.
	def remove_node( node )
		@nodes.delete( node.object_id )
	end


	#
	# Verse::Server Overrides
	#

	### Receive a connect event from a client. 
	def on_connect( user, pass, address, expected_host_id=WILDCARD_HOSTID )
		self.log.info "Connect: %s@%s" % [ user, address ]
		unless expected_host_id == WILDCARD_HOSTID || expected_host_id == self.host_id
			self.log.warn "  connection expected a different hostid (%p). Ignoring connection." %
				[ expected_host_id ]
			return
		end

		# Create the session's avatar
		self.log.debug "  creating %s's avatar" % [ user ]
		avatar = Verse::ObjectNode.new
		self.log.debug "  adding avatar: %p" % [ avatar ]
		self.add_node( avatar )

		self.log.debug "  sending connect_accept back to %p" % [ address ]
		session = Verse.connect_accept( avatar, address, self.host_id )
		avatar.session = session
		self.observe( session )
		connection = Verse::Server::Connection.new( address, user, session, avatar, [] )

		self.log.debug "  adding the connection (total: %d)" % [ self.connections.length + 1 ]
		self.connections[ address ] = connection
	end


	### Subscribe the specified +session+ to creation/destruction events for the given
	### +classes+ of node.
	def on_node_index_subscribe( session, *classes )
		conn = self.connections[ session.address ] or return nil

		if classes.empty?
			self.log.info "%p: unsubscribe from index events" % [ session ]
			conn.index_subscriptions.clear
		else
			self.log.info "%p: subscribe to index events for: %p" % [ session, classes ]
			newclasses = classes - conn.index_subscriptions
			@nodes.find {|id,node| newclasses.include?(node.class) }.each do |node|
				conn.session.node_created( )
			end
			conn.index_subscriptions += newclasses
		end
	end



	#
	# PingObserver API
	#

	### Receive a ping event.
	def on_ping( address, message )
		self.log.debug "Got a ping from %s: %s" % [ address, message ]
	end


	# 
	# SessionObserver API
	# 

	### Node-creation callback. This implementation creates any node requested 
	### without limitation.
	def on_create_node( nodeclass )
		
	end


end # class Verse::Server

