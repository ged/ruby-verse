#!/usr/bin/env ruby

require 'pathname'
require 'singleton'

require 'verse'
require 'verse/mixins'
require 'verse/utils'

# Basic Verse server object class
class Verse::Server
	include Singleton,
	        Verse::Loggable,
	        Verse::PingObserver,
	        Verse::ConnectionObserver

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
		Signal.trap( :INT, 'IGN' )
		Signal.trap( :TERM, 'IGN' )
		Signal.trap( :HUP, 'IGN' )
	end


	### Load the hostid from the saved file if it exists. If it doesn't, generate a new one
	### and save it before returning it.
	def load_host_id
		if HOSTID_FILE.exist?
			return HOSTID_FILE.read
		else
			hostid = Verse.make_host_id
			HOSTID_FILE.open( File::WRONLY|File::CREAT|File::EXCL, 0600 ) do |ofh|
				ofh.write( hostid )
			end
			return hostid
		end
	end


	### Add a node to the server, assigning it an ID and storing it.
	def add_node( node )
		
	end


	#
	# ConnectionObserver API
	#

	### Receive a connect event from a client. 
	def on_connect( user, pass, address, expected_host_id )
		return unless expected_host_id.nil? || expected_host_id == self.host_id

		# Create the session's avatar
		avatar = Verse::ObjectNode.new
		self.add_node( avatar )

		session = Verse.connect_accept( avatar, address, self.host_id )
		connection = Verse::Server::Connection.new( address, user, session, avatar )

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

