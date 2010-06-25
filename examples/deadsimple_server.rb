#!/usr/bin/env ruby1.9
#coding: utf-8

# Add dead-simple authentication to the base server. 

BEGIN {
	require 'pathname'

	basedir = Pathname( __FILE__ ).dirname.parent
	libdir = basedir + 'lib'

	$LOAD_PATH.unshift( libdir.to_s )
}

require 'pp'
require 'verse'
require 'logger'
require 'pathname'

class MyServer < Verse::Server
	require 'digest/sha1'

	include Verse::ConnectionObserver

	USERS = {
		'bargle' => 'a76282512cb523ec1f82c79b4f51e34cdd5f11ea'
	}

	def on_connect( name, pass, address, hostid )
		return unless self.authenticate( user, pass )
		super
	end

	def authenticate( user, pass )
		return USERS[ user ] == Digest::SHA1.hexdigest( pass )
	end

end # class Server

MyServer.new( :port => 5618 ).run

