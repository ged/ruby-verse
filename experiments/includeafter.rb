#!/usr/bin/env ruby19

# This is an experiment to see what happens if you define methods in
# a module after previously including it in a class. Does the class
# see the new methods?
# 
# I need to figure this out to see whether or not I need to ensure
# that all the Observer/Observable functionality is defined before
# declaring the concrete classes that use them.

module Observable
	def initialize
		$stderr.puts "Object %p is Observable" % [ self ]
		@observers = []
	end
end

class Node
	include Observable

	def initialize
		$stderr.puts "Created: %p" % [ self ]
		super
	end

end

module Observable
	def add_observer( obj )
		$stderr.puts "Adding observer to %p: %p" % [ self, obj ]
		@observers << obj
	end
end


obj = Node.new
obj.add_observer( Object.new )


# Apparently I don't need to worry about it:
# 
# $ ruby1.9 experiments/includeafter.rb 
# Created: #<Node:0x000001010331c0>
# Object #<Node:0x000001010331c0> is Observable
# Adding observer to #<Node:0x000001010331c0 @observers=[]>: #<Object:0x00000101032f20>
# 

