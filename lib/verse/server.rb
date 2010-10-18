#!/usr/bin/env ruby

require 'verse'
require 'verse/mixins'

# The base server class.
class Verse::Server
	include Verse::Loggable

	### Callback for connection events. In this server, it's a no-op.
	def on_connect( username, password, address, expected_host_id )
		self.log.info "Connect from %s: (user = %s)" % [ address, username ]
	end

	### Callback for node index subscription events. In this server, it's a no-op.
	def on_node_index_subscribe( session, *classes )
		self.log.info "Index subscription from %s: %p" % [ session, classes ]
	end

end # class Verse::Server

