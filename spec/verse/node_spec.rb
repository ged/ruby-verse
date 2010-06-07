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

describe Verse::Node do
	include Verse::SpecHelpers

	before( :all ) do
		setup_logging( :debug )
	end


	it "is an abstract class" do
		pending "conversion to the Observer-based API"
		expect { Verse::Node.new }.to raise_exception( NoMethodError, /undefined method/ )
	end


	describe "concrete subclasses" do

		before( :each ) do
			@session = mock( 'verse session' )
			@nodeclass = Class.new( Verse::Node )
		end

		it "can be created" do
			pending "conversion to the Observer-based API"
			@session.should_receive( :create_node ).with( @nodeclass ).and_yield( :the_new_node )
			@node = @nodeclass.new( @session )
		end

		it "can be created with a callback that is called when creation succeeds" do
			pending "conversion to the Observer-based API"
			@session.should_receive( :create_node ).with( @nodeclass ).and_yield( :the_new_node )
			@nodeclass.new( @session ) do |node|
				result = node
			end
			result.should == :the_new_node
		end

		it "can be destroyed"

		it "can have a name"

		it "can be subscribed to"
		it "can be unsubscribed from"

		it "can create tag groups"
		it "can destroy tag groups"
		it "can subscribe to a tag group"
		it "can unsubscribe from a tag group"

	end
end

# vim: set nosta noet ts=4 sw=4:
