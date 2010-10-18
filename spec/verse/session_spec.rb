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
require 'verse/server'


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
		@address = "localhost:#@port"
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
			Verse.update
		}

		Verse::Session.all_connected.should include( session1 )
		Verse::Session.all_connected.should_not include( session2 )
	end

	it "doesn't block other threads when calling .update" do
		updater = Thread.new do
			Thread.current.abort_on_exception = true
			Verse.update( 0.5 )
		end

		count = 0
		count += 1 while updater.alive?

		updater.join
		count.should > 1
	end


	describe "instances that have not yet connected" do
		before( :each ) do
			@session = Verse::Session.new
		end

		it "raise an exception if their address hasn't been set when connecting" do
			expect {
				@session.connect( 'user', 'pass' )
			}.to raise_exception( Verse::SessionError, /address/i )
		end

	end


	describe "instances that are connected" do
		before( :all ) do
			unless @server_pid = Process.fork
				begin
					config = OpenStruct.new
					config.port = 45196
					Verse::Server.new( config ).run
				rescue RuntimeError, ScriptError => err
					$stderr.puts "\e[31;43m %p in the server setup: %s\e[0m" %
					 	[ err.class, err.message ]
					err.backtrace.each do |frame|
						$stderr.puts "   #{frame}"
					end
				ensure
					exit!
				end
			end
		end

		before( :each ) do
			@session = Verse::Session.new( @address )
			@session.connect( 'test', 'test' )
			@update_thread = Thread.new do
				Thread.current.abort_on_exception = true
				Thread.current[:running] = true
				Verse.update until Thread.current[:halt]
			end
		end

		after( :each ) do
			@update_thread[:halt] = true
			@update_thread.join
		end

		after( :all ) do
			Process.kill( :TERM, @server_pid )
			Process.wait
		end

		it "raise an exception if asked to destroy a node that doesn't belong to a session" do
			node = Verse::ObjectNode.new
			expect {
				@session.destroy_node( node )
			}.to raise_exception( Verse::NodeError, /active session/i )
		end

		it "raise an exception if asked to destroy a node that belongs to another session"
		# 	other_session = Verse::Session.new( @address )
		# 	other_session.connect( 'test2', 'test2' )
		# 
		# 	finished = false
		# 	other_session.create_node( Verse::ObjectNode ) do |node|
		# 		expect {
		# 			@session.destroy_node( node )
		# 		}.to raise_exception( Verse::SessionError, /alive/ )
		# 		finished = true
		# 	end
		# 
		# 	sleep 1 until finished
		# end

	end

end

# vim: set nosta noet ts=4 sw=4:
