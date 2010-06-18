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
		expect { Verse::Node.new }.to raise_exception( NoMethodError, /undefined method/ )
	end


	describe "concrete subclasses" do

		it "can be created on the server"

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
