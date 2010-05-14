#!/usr/bin/env ruby

require 'verse'

# A port of the minimal example from http://verse.blender.org/coding-intro/
class NodeLogger
	include Verse::Constants

	### Create a new NodeLogger that will log node creations on the specified +host+ 
	### and +port+.
	def initialize( host, name, password, port=nil, hostid=nil )
		@log      = Logger.new( $stderr )
		@host     = host || 'localhost'
		@name     = name
		@password = password

		# These might be nil
		@port     = port
		@hostid   = hostid

		@session  = nil
	end


	######
	public
	######

	# The Verse host
	attr_reader :host

	# The port on the Verse host; if this is nil, the default port will be used.
	attr_reader :port

	# The username to use when connecting
	attr_reader :name

	# The password to use when connecting
	attr_reader :password


	### Register callbacks and start logging creation events.
	def run
		address = [ self.host, self.port ].compact.join( ':' )
		@session = Verse.connect( address, self.user, self.password, self.hostid )

		# Register the callback for node-creation
		@session.set_callback( :node_create, self.method(:handle_node_create) )

		@session.connect( self.name, self.password ) do ||
		    @log.info "Connected as avatar %p to %s (hostid: %s)\n" % [
				@session.avatar,
				@session.address,
				@session.host_id
			  ]
			@session.node_index_subscribe( Verse::V_NT_ALL )
		end

		@session.callback_update while @running
	end


	### Callback for send_node_create: log the created node.
	def handle_node_create( node_id, type, owner )
		@log.info " Node #%d has type %d, owner %s\n" % [
			node_id,
			type,
			owner == VN_OWNER_MINE ? "me" : "someone else"
		]
	end

end # class NodeLogger


if __FILE__ == $0
	nl = NodeLogger.new( *ARGV )
	nl.run
end


