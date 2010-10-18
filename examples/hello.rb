#!/usr/bin/env ruby19 -w

BEGIN {
	require 'pathname'

	basedir = Pathname( __FILE__ ).dirname.parent
	libdir = basedir + 'lib'

	$LOAD_PATH.unshift( libdir.to_s )
}

require 'verse'


class NodeLister
	include Verse::SessionObserver,
	        Verse::NodeObserver

	def initialize( session )
		self.observe( session )
		@session = session
	end

	def on_connect_accept( avatar, addr, hostid )
		$stderr.puts "Connection accepted; my avatar is: %p" % [ avatar ]
		@session.subscribe_to_node_index( Verse::ObjectNode )
	end

	def on_node_created( node )
		self.observe( node )
	end

	def on_node_destroy( node )
		self.stop_observing( node )
	end

	def on_node_name_set( node, name )
		$stdout.puts "Node %p is now named %p" % [ node, name ]
	end

end


if $0 == __FILE__
	host = if ARGV.empty?
		'localhost'
	else
		ARGV.shift
	end

	Verse.logger.level = Logger::DEBUG
	Verse.logger.formatter = Verse::ColorLogFormatter.new( Verse.logger )

	session = Verse::Session.new( host )
	lister = NodeLister.new( session )
	$stderr.puts "Connecting to %p" % [ host ]

	session.connect( "spoo", "fleem" )

	Verse.update( 2 ) while session.connected?
end

