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

	def on_create_node( session, node )
		node.add_observer( self )
	end

	def on_set_name( node, name )
		$stdout.puts "Node created: %p (%s)" % [ node, name ]
	end

end


if $0 == __FILE__
	host = if ARGV.empty?
		'localhost'
	else
		ARGV.shift
	end

	session = Verse::Session.new( host, "spoo", "fleem" )

	lister = NodeLister.new
	session.add_observer( lister )

	Verse.update until session.terminated?
end

