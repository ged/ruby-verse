#!/usr/bin/env ruby

require 'verse'

# A collection of constants shared throughout Verse classes.
module Verse::Constants

	# The default verse protocol port
	DEFAULT_PORT = 4950

	# VNodeType
	V_NT_OBJECT            = 0
	V_NT_GEOMETRY          = 1
	V_NT_MATERIAL          = 2
	V_NT_BITMAP            = 3
	V_NT_TEXT              = 4
	V_NT_CURVE             = 5
	V_NT_AUDIO             = 6
	V_NT_NUM_TYPES         = 7
	V_NT_SYSTEM            = V_NT_NUM_TYPES
	V_NT_NUM_TYPES_NETPACK = 9


end # module Verse::Constants

