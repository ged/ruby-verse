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

require 'verse/mixins'


include Verse::TestConstants
include Verse::Constants

#####################################################################
###	C O N T E X T S
#####################################################################

describe Verse, "mixins: " do
	include Verse::SpecHelpers

	before( :all ) do
		setup_logging( :debug )
	end


	describe Verse::Loggable, "objects" do
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


		it "are able to output to the log via the #log method" do
			@obj.log_test_message( :debug, "debugging message" )
			@logfile.rewind
			@logfile.read.should =~ /debugging message/
		end

		it "are able to output to the log via the #log_debug method" do
			@obj.logdebug_test_message( "sexydrownwatch" )
			@logfile.rewind
			@logfile.read.should =~ /sexydrownwatch/
		end
	end


	describe Verse::VersionUtilities do

		it "doesn't affect an including object's public interface" do
			including_class = Class.new do
				include Verse::VersionUtilities
				def use_vvec; vvec( "2.8.8" ); end
			end
			instance = including_class.new
			instance.should_not respond_to( :vvec )
			instance.use_vvec.should == [ 2, 8, 8 ].pack( 'N*' )
		end

		it "can vectorize x.y.z version strings" do
			Verse::VersionUtilities.vvec( "1.12.8" ).should == [ 1, 12, 8 ].pack( 'N*' )
		end

		it "vectorizes version strings so they are stringwise-comparable" do
			lesser = Verse::VersionUtilities.vvec( "1.8.12" )
			greater = Verse::VersionUtilities.vvec( "1.12.8" )

			lesser.should < greater
		end

	end


	describe Verse::Versioned, "objects" do

		before( :each ) do
			@versioned_class = Class.new do
				include Verse::Versioned
			end
			@versioned_object = @versioned_class.new
		end


		it "have a data version" do
			@versioned_object.data_version.should == 0
		end

		it "have a structure version" do
			@versioned_object.structure_version.should == 0
		end

		it "have a incrementer for its data version" do
			@versioned_object.update_data_version
			@versioned_object.data_version.should == 1
			@versioned_object.structure_version.should == 0
		end

		it "have a incrementer for its structure version that also updates its data version" do
			@versioned_object.update_structure_version
			@versioned_object.data_version.should == 1
			@versioned_object.structure_version.should == 1
		end

		it "return their version in inspect output" do
			@versioned_object.update_structure_version
			@versioned_object.update_data_version
			@versioned_object.inspect.should =~ /version: 1.2/
		end

	end


	describe Verse::Observable, "objects" do

		before( :each ) do
			@observable_class = Class.new do
				include Verse::Observable
			end
			@observable = @observable_class.new
			@object = Object.new
		end

		it "are also versioned (via Verse::Versioned)" do
			@observable.should be_a_kind_of( Verse::Versioned )
			@observable.data_version.should == 0
		end

		it "can register an observer" do
			@observable.add_observer( @object )
			@observable.observers.should include( @object )
		end

		it "only registers an observer once even if it's added more than once" do
			@observable.add_observer( @object )
			@observable.add_observer( @object )
			@observable.observers.count( @object ).should == 1
		end

		it "can remove an observer" do
			@observable.add_observer( @object )
			@observable.remove_observer( @object )
			@observable.observers.should_not include( @object )
		end

		it "removing an observer that isn't registered doesn't error" do
			expect {
				@observable.remove_observer( @object )
			}.to_not raise_exception()
		end

		it "can remove all observers" do
			@observable.add_observer( @object )
			@observable.observers.should_not be_empty()
			@observable.remove_observers
			@observable.observers.should be_empty()
		end


	end


	describe Verse::Observer, "objects" do

		before( :each ) do
			@observer_class = Class.new do
				include Verse::Observer
			end
			@observer = @observer_class.new
			@observable = mock( "observable object" )
		end

		it "can add themselves as an observer to Observable objects" do
			@observable.should_receive( :add_observer ).with( @observer )
			@observer.observe( @observable )
		end

		it "can stop observing an Observable object" do
			@observable.should_receive( :remove_observer ).with( @observer )
			@observer.stop_observing( @observable )
		end

	end


	describe Verse::PingObserver, "objects" do
		before( :each ) do
			@observer_class = Class.new do
				include Verse::PingObserver
			end
			@observer = @observer_class.new
		end

		it "can receive ping events" do
			@observer.should respond_to( :on_ping )
		end
	end


	describe Verse::SessionObserver, "objects" do
		before( :each ) do
			@observer_class = Class.new do
				include Verse::SessionObserver
			end
			@observer = @observer_class.new
		end


		it "can receive connect_accept events" do
			@observer.should respond_to( :on_connect_accept )
		end

		it "can receive connect_terminate events" do
			@observer.should respond_to( :on_connect_terminate )
		end

	end

end

# vim: set nosta noet ts=4 sw=4:
