#!/usr/bin/env ruby

require 'pathname'
require 'singleton'

require 'verse'
require 'verse/mixins'
require 'verse/utils'

# Basic Verse server object class
class Verse::Server
	include Verse::Loggable,
	        Verse::PingObserver,
	        Verse::ConnectionObserver,
	        Verse::SessionObserver

	# The path to the server's saved hostid
	HOSTID_FILE = Pathname( 'hostid.rsa' )

	# Connection struct -- stores connection details in the @connections hash
	Connection = Struct.new( "VerseServerConnection", :address, :user, :session, :avatar )


	#################################################################
	###	I N S T A N C E   M E T H O D S
	#################################################################

	### Create a new Server configured with the specified +options+.
	def initialize( options )
		Verse.port   = options.port

		@host_id      = self.load_host_id
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


	### Start listening for events.
	def run
		self.observe( Verse )
		self.log.info "Starting run loop"

		@state = :running
		Verse::Session.update until @state != :running
	end


	### Shut the server down using the specified +reason+.
	def shutdown( reason="No reason given." )
		self.reset_signal_handlers
		self.log.warn "Server shutdown: #{reason}"

		self.stop_observing( Verse )
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
		Signal.trap( :HUP ) { self.reload }
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
			return HOSTID_FILE.read
		else
			hostid = Verse.create_host_id
			HOSTID_FILE.open( File::WRONLY|File::CREAT|File::EXCL, 0600 ) do |ofh|
				ofh.write( hostid )
			end
			return hostid
		end
	end


	### Add a node to the server, assigning it an ID and storing it.
	def add_node( node )
		
	end


	### Remove a node from the server.
	def remove_node( node )
		
	end


	#
	# ConnectionObserver API
	#

	### Receive a connect event from a client. 
	def on_connect( user, pass, address, expected_host_id=nil )
		self.log.info "Connect: %s@%s" % [ user, address ]
		unless expected_host_id.nil? || expected_host_id == self.host_id
			self.log.warn "  connection expected a different hostid. Ignoring connection."
			return
		end

		# Create the session's avatar
		self.log.debug "  creating %s's avatar" % [ user ]
		avatar = Verse::ObjectNode.new
		self.log.debug "  adding avatar: %p" % [ avatar ]
		self.add_node( avatar )

		self.log.debug "  sending connect_accept back to %p" % [ address ]
		session = Verse.connect_accept( avatar, address, self.host_id )
		self.observe( session )
		connection = Verse::Server::Connection.new( address, user, session, avatar )

		self.log.debug "  adding the connection (total: %d)" % [ self.connections.length + 1 ]
		self.connections[ address ] = connection
	end


	#
	# PingObserver API
	#

	### Receive a ping event.
	def on_ping( address, message )
		self.log.debug "Got a ping from %s: %s" % [ address, message ]
	end



end # class Verse::Server

