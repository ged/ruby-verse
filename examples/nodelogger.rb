#!/usr/bin/env ruby

require 'verse'

class NodeLister
	include Verse::SessionObserver,
	        Verse::NodeObserver

	def on_create_node( session, node )
		node.add_observer( self )
	end


	def on_set_name( node, name )
		$stdout.puts "node: %s (%p)" % [ name, node ]
	end

end

session = Verse::Session.new( 'localhost', 'spoo', 'fleem' )

nodelister = NodeLister.new
session.add_observer( nodelister )
session.update( 1.0 ) while session.connected?

