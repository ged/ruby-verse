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

require 'rspec'

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
		expect { Verse::Node.new }.to raise_exception( TypeError, /can't instantiate/i )
	end


	describe "concrete subclasses" do

		before( :each ) do
			@nodeclass = Class.new( Verse::Node )
			@node = @nodeclass.new
		end

		it "raises an exception if its ID is set twice" do
			@node.id = 13
			expect {
				@node.id = 131
			}.to raise_exception( Verse::NodeError, /already set/ )
		end

		it "can have a name"

		it "can be subscribed to"
		it "can be unsubscribed from"

		it "can create tag groups"
		it "can destroy tag groups"

	end
end

# vim: set nosta noet ts=4 sw=4:
