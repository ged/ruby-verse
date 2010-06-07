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

	it "can be created with an address" do
		Verse::Session.new( TEST_ADDRESS ).address.should == TEST_ADDRESS
	end

	it "can set its address after creation" do
		session = Verse::Session.new
		session.address = TEST_ADDRESS
		session.address.should == TEST_ADDRESS
	end

	it "can acquire the session mutex for the null session" do
		pending "conversion to the Observer-based API"
		current_session = nil
		Verse::Session.synchronize { current_session = Verse::Session.current }
		current_session.should == nil
	end

	describe "acting in the client role" do
		before( :each ) do
			@port = 4950
			Verse.port = @port
			@address = "localhost:#@port"
			@hostid = Verse.create_host_id
			@session = Verse::Session.new( @address )
		end

		it "can register a callback for `connect_accept' events" do
			pending "conversion to the Observer-based API"
			session = nil

			Verse.on_connect do |name, pass, address, exp_hostid|
				Verse.log.notice "Got connect: %p" % [[ name, pass, address, exp_hostid ]]
				session = Verse.connect_accept( 1, address, exp_hostid )
			end

			client_thread = Thread.new do
				Thread.current.abort_on_exception = true
				callback_values = nil

				@session.connect( "testuser", "testpass" )
				@session.on_connect_accept do |avatar, address, host_id|
					Verse.log.notice "Got connect_accept: %p" % [[ avatar, address, host_id ]]
					callback_values = [avatar, address, host_id]
				end

				while callback_values.nil?
					Verse.callback_update( 0.25 )
				end

				callback_values
			end

			while client_thread.alive?
				Verse.callback_update( 0.25 )
			end

			rval = client_thread.value
			rval.should == [ 1, @address, @hostid ]

			session.should be_a( Verse::Session )
		end

		it "can connect to a Verse host"
		it "raises an exception if the address hasn't been set when connecting"

	end

	describe "acting in the server role" do

		before( :each ) do
			@port = 4950
			Verse.port = @port
			@hostid = Verse.create_host_id
			@session = Verse::Session.new
		end

		it "can register a handler for `connect' events"

	end

end

# vim: set nosta noet ts=4 sw=4:
