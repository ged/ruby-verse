#!/usr/bin/env ruby

BEGIN {
	require 'rbconfig'
	require 'pathname'
	basedir = Pathname.new( __FILE__ ).dirname.parent.parent

	libdir = basedir + "lib"
	extdir = libdir + Config::CONFIG['sitearch']

	$LOAD_PATH.unshift( basedir ) unless $LOAD_PATH.include?( basedir )
	$LOAD_PATH.unshift( libdir ) unless $LOAD_PATH.include?( libdir )
	$LOAD_PATH.unshift( extdir ) unless $LOAD_PATH.include?( extdir )
}

require 'ostruct'
require 'rspec'

require 'spec/lib/constants'
require 'spec/lib/helpers'

require 'verse'


include Verse::TestConstants
include Verse::Constants

#####################################################################
###	C O N T E X T S
#####################################################################

describe Verse::Server do
	include Verse::SpecHelpers

	TEST_ADDRESS = 'localhost:25688'

	before( :all ) do
		setup_logging( :debug )
		@port = 45196
		@address = "localhost:#@port"
	end

	after( :each ) do
		Verse.remove_observers
		if server = Verse::Server.running_instance
			server.shutdown
		end
	end


	it "knows which instance is running" do
		server = Verse::Server.new
		Verse::Server.running_instance.should be_nil()
		server.run
		Verse::Server.running_instance.should == server
		server.shutdown
		Verse::Server.running_instance.should be_nil()
	end


	it "only allows one running instance at a time" do
		server = Verse::Server.new
		server.run

		server2 = Verse::Server.new
		expect {
			server2.run
		}.to raise_exception( Verse::ServerError, /another server is already running/i )

	end

	it "can accept incoming connections" do
		serverclass = Class.new( SimpleTestServer ) do
			def initialize
				super
				@hostid = Verse.make_host_id
				@sessions = []
			end
			attr_reader :sessions
			def on_connect( user, pass, address, hostid )
				self.log.debug "Connect from #{address}"
				super
				avatar = Verse::ObjectNode.new
				avatar.id = avatar.object_id
				self.sessions << self.accept_connection( avatar, address, @hostid )
			end
		end
		server = serverclass.new
		Verse.connect_port = @port
		server.run

		session = Verse::Session.new( @address )
		session.should_receive( :on_connect_accept ).
			with( an_instance_of(Verse::ObjectNode), @address, an_instance_of(String) ).
			and_return { server.shutdown }
		session.connect( 'user', 'pass' )

		count = 0
		while server.running? && count < 15
			Verse.update
			count += 1
		end

		server.sessions.should have( 1 ).member
		server.connections.should == [ 'user', 'pass', 'address', "\0" * Verse::HOST_ID_SIZE ]
	end


end

# vim: set nosta noet ts=4 sw=4:
