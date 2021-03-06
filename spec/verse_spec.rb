#!/usr/bin/env ruby
# encoding: utf-8

BEGIN {
	require 'rbconfig'
	require 'pathname'
	basedir = Pathname.new( __FILE__ ).dirname.parent

	libdir = basedir + "lib"
	extdir = libdir + Config::CONFIG['sitearch']

	$LOAD_PATH.unshift( basedir ) unless $LOAD_PATH.include?( basedir )
	$LOAD_PATH.unshift( libdir ) unless $LOAD_PATH.include?( libdir )
	$LOAD_PATH.unshift( extdir ) unless $LOAD_PATH.include?( extdir )
}

require 'rspec'

require 'socket'
require 'spec/lib/constants'
require 'spec/lib/helpers'

require 'verse'


include Verse::TestConstants
include Verse::Constants

#####################################################################
###	C O N T E X T S
#####################################################################

describe Verse do
	include Verse::SpecHelpers

	before( :all ) do
		setup_logging( :fatal )
		@port = 45196
		Verse.port = @port
	end

	after( :all ) do
		reset_logging()
	end

	it "should know if its default logger is replaced" do
		Verse.reset_logger
		Verse.should be_using_default_logger
		Verse.logger = Logger.new( $stderr )
		Verse.should_not be_using_default_logger
	end


	it "returns a version string if asked" do
		Verse.version_string.should =~ /\w+ [\d.]+/
	end


	it "returns a version string with a build number if asked" do
		Verse.version_string(true).should =~ /\w+ [\d.]+ \(build \w+\)/
	end


	it "knows what version of the verse library it's using" do
		Verse.library_version.should =~ /^r(\d+)p(\d+).*/i
	end


	describe Verse::PingObserver do

		before( :each ) do
			setup_logging( :fatal )
		end

		after( :each ) do
			Verse.remove_observers
		end

		it "receives pings" do
			observer_class = Class.new do
				include Verse::PingObserver

				def initialize
					@ping_received = false
					@address       = address
					@data          = data
				end

				def on_ping( address, data )
					@ping_received = true
					@address       = address
					@data          = data
				end

				attr_reader :address, :data

				def received_ping?; @ping_received ? true : false ; end
			end

			observer = observer_class.new
			addr     = "127.0.0.1:#@port"
			data     = 'expected message'

			Verse.add_observer( observer )
			Verse.ping( addr, data )

			10.times do
				Verse.update( 0.1 )
				break if observer.received_ping?
			end

			observer.address.should == addr
			observer.data.should == data
		end

	end

	describe "host ID functions" do

		before( :each ) do
			setup_logging( :debug )
		end

		it "can generate a new host ID" do
			hostid = Verse.create_host_id
			hostid.length.should == HOST_ID_SIZE
			hostid.encoding.should == Encoding::ASCII_8BIT
		end

		it "provides a method for setting the host id" do
			hostid = Verse.create_host_id
			Verse.host_id = hostid
		end

		it "prevents a host id from being too short" do
			shortid = 'too short'.encode( 'ASCII-8BIT' )
			expect {
				Verse.host_id = shortid
			}.to raise_exception( ArgumentError, /too short/i )
		end

		it "prevents a host id from being set to anything other than ASCII 8-bit data" do
			utf8_id = "ü" * HOST_ID_SIZE
			utf8_id.encoding.should == Encoding::UTF_8
			expect {
				Verse.host_id = utf8_id
			}.to raise_exception( ArgumentError, /ASCII-8bit/i )
		end

		it "prevents a host id from being too long" do
			longid = ( 'x' * (HOST_ID_SIZE+10) ).encode( Encoding::ASCII_8BIT )
			expect {
				Verse.host_id = longid
			}.to raise_exception( ArgumentError, /too long/i )
		end

	end

	describe "logging subsystem" do
		before(:each) do
			Verse.reset_logger
		end

		after(:each) do
			Verse.reset_logger
		end


		it "has the default logger instance after being reset" do
			Verse.logger.should equal( Verse.default_logger )
		end

		it "has the default log formatter instance after being reset" do
			Verse.logger.formatter.should equal( Verse.default_log_formatter )
		end

		describe "with new defaults" do
			before( :all ) do
				@original_logger = Verse.default_logger
				@original_log_formatter = Verse.default_log_formatter
			end

			after( :all ) do
				Verse.default_logger = @original_logger
				Verse.default_log_formatter = @original_log_formatter
			end


			it "uses the new defaults when the logging subsystem is reset" do
				logger = mock( "dummy logger" ).as_null_object
				formatter = mock( "dummy log formatter" )

				Verse.default_logger = logger
				Verse.default_log_formatter = formatter

				logger.should_receive( :formatter= ).with( formatter ).twice

				Verse.reset_logger
				Verse.logger.should equal( logger )
			end

		end

	end

end

# vim: set nosta noet ts=4 sw=4:
