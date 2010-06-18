#!/usr/bin/env ruby

BEGIN {
	require 'rbconfig'
	require 'pathname'
	basedir = Pathname.new( __FILE__ ).dirname.parent.parent

	libdir = basedir + "lib"
	extdir = libdir + Config::CONFIG['sitearch']

	$LOAD_PATH.unshift( libdir ) unless $LOAD_PATH.include?( libdir )
	$LOAD_PATH.unshift( extdir ) unless $LOAD_PATH.include?( extdir )
}

require 'spec'
require 'spec/lib/constants'
require 'spec/lib/helpers'

require 'verse'


include Verse::TestConstants
include Verse::Constants

#####################################################################
###	C O N T E X T S
#####################################################################

describe Verse::Session do
	include Verse::SpecHelpers

	TEST_ADDRESS = 'localhost:25688'

	before( :all ) do
		setup_logging( :debug )
	end

	before( :each ) do
		Verse.remove_observers
		@port = 45196
		Verse.port = @port
	end


	it "can be created with an address" do
		Verse::Session.new( TEST_ADDRESS ).address.should == TEST_ADDRESS
	end

	it "can set its address after creation" do
		session = Verse::Session.new
		session.address = TEST_ADDRESS
		session.address.should == TEST_ADDRESS
	end

	it "knows about all of its connected instances" do
		session1 = Verse::Session.new( "localhost:#@port" )
		session1.connect( 'user', 'pass' )
		session2 = Verse::Session.new( "localhost:#@port" )

		2.times {
			Verse::Session.all_connected
			Verse::Session.update
		}

		Verse::Session.all_connected.should include( session1 )
		Verse::Session.all_connected.should_not include( session2 )
	end

	it "can synchronize via a global session mutex to avoid race conditions with multiple sessions" do
		Verse::Session.mutex.should_not be_locked()
		Verse::Session.synchronize do
			Verse::Session.mutex.should be_locked()
		end
		Verse::Session.mutex.should_not be_locked()
	end

	it "doesn't block other threads when calling .update" do
		updater = Thread.new do
			Thread.current.abort_on_exception = true
			Verse::Session.update( 0.5 )
		end

		count = 0
		count += 1 while updater.alive?

		updater.join
		count.should > 1
	end


	describe "instances" do
		before( :each ) do
			@address = "localhost:#@port"
			@hostid = Verse.create_host_id
			@session = Verse::Session.new( @address )
		end

		it "calls #on_connect_accept on its SessionObservers when its connection is accepted"
		it "can connect to a Verse host"
		it "raises an exception if the address hasn't been set when connecting"

	end

end

# vim: set nosta noet ts=4 sw=4:
