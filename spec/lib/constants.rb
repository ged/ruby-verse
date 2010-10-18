#!/usr/bin/env ruby

require 'pathname'

require 'verse'
require 'verse/constants'


module Verse::TestConstants
	include Verse::Constants

	class SimpleTestServer < Verse::Server
		include Verse::SessionObserver

		### Create a new SimpleServer.
		def initialize
			@connections   = []
			@subscriptions = []
		end

		attr_reader :connections, :subscriptions

		### Accept any connection.
		def on_connect( user, pass, addr, hostid )
			self.connections << [ user, pass, addr, hostid ]
		end

		### Handle a client telling the server it's disconnecting
		def on_node_index_subscribe( addr, message )
			self.subscriptions << [ addr, message ]
		end

	end # class SimpleTestServer

end

