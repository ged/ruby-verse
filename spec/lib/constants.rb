#!/usr/bin/env ruby

require 'pathname'

require 'verse'
require 'verse/constants'


module Verse::TestConstants
	include Verse::Constants

	class SimpleConnectionObserver
		include Verse::ConnectionObserver

		### Create a new SimpleConnectionObserver.
		def initialize
			@clients = {}
		end

		### Accept any connection.
		def on_connect( user, pass, addr, hostid )
			self.log.debug "on_connect: %s from %s, looking for host %p" % [ user, addr, hostid ]
			avatar = self.make_avatar( user, pass, addr )
			session = Verse.connect_accept( avatar, addr, hostid )

			@clients[ addr ] = { :session => session, :avatar => avatar }

			return session
		end

		### Handle a client telling the server it's disconnecting
		def on_connect_terminate( addr, message )
			self.log.debug "on_connect_terminate: from %s: %s" % [ addr, message ]
			client = @clients.delete( addr )
			client[:avatar].destroy if client[:avatar].respond_to?( :destroy )
		end

		### Terminate the session from the specified +addr+.
		def terminate( addr, message )
			self.log.debug "Terminating connection from %s: %s" % [ addr, message ]
			Verse.connect_terminate( addr, message )
			self.on_connect_terminate( addr, message )
		end

		### Create an avatar node for the given +user+ and return it.
		def make_avatar( user, addr )
			self.log.debug "emulating an avatar node"
			return 1
		end

	end # class SimpleConnectionObserver

end

