#!/usr/bin/env ruby

#
# Rawk server, a Ruby port of the Tawk basic IRC-like chat for 
# Verse. *** UNFINISHED ***
#
VERSION = "0.1"

require 'verse'

class RawkServer
	include Verse::SessionObserver,
	        Verse::NodeObserver,
	        Verse::TextNodeObserver,
	        Verse::ObjectNodeObserver

	# The server version
	VERSION = '0.0.1'

	# The version-control revision
	REVISION = %q$Revision$

	DEFAULT_BANNER = '%s v%s (%s)' % [ self.name, VERSION, REVISION ]

	### Create a new server that will communicate via the Verse server
	### at +address+, logging in with the specified +user+ and +password+.
	def initialize( address, user, password, banner=DEFAULT_BANNER )
		@session = Verse::Session.new( address )
		@user    = user
		@pass    = password
		@banner  = banner
		@avatar  = nil
	end


	### Start the server.
	def start
		@session.connect( @user, @pass )
	end


	#
	# SessionObserver API
	#

	### Callback for acceptance of the connection to the Verse server.
	def on_connect_accept( avatar )
		@avatar = avatar
	end


end # class RawkServer

