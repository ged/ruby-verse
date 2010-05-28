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

require 'verse/mixins'


include Verse::TestConstants
include Verse::Constants

#####################################################################
###	C O N T E X T S
#####################################################################

describe Verse, "mixins" do
	include Verse::SpecHelpers

	before( :all ) do
		setup_logging( :debug )
	end


	describe Verse::Loggable, "mixed into a class" do
		before(:each) do
			@logfile = StringIO.new('')
			Verse.logger = Logger.new( @logfile )

			@test_class = Class.new do
				include Verse::Loggable

				def log_test_message( level, msg )
					self.log.send( level, msg )
				end

				def logdebug_test_message( msg )
					self.log_debug.debug( msg )
				end
			end
			@obj = @test_class.new
		end


		it "is able to output to the log via its #log method" do
			@obj.log_test_message( :debug, "debugging message" )
			@logfile.rewind
			@logfile.read.should =~ /debugging message/
		end

		it "is able to output to the log via its #log_debug method" do
			@obj.logdebug_test_message( "sexydrownwatch" )
			@logfile.rewind
			@logfile.read.should =~ /sexydrownwatch/
		end
	end


	describe Verse::ConnectionObserver, "mixed into a class" do

		it "" do
			handler = lambda {|*args| }
			Verse.on_connect( &handler )
			Verse.on_connect.should == handler
		end

	end


end

# vim: set nosta noet ts=4 sw=4:
