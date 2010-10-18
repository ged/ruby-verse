#!/usr/bin/env ruby

require 'mkmf'
require 'fileutils'

if $DEBUG
	$CFLAGS << ' -Wall'
	$CFLAGS << ' -ggdb' << ' -DDEBUG'
end

def fail( *messages )
	$stderr.puts( *messages )
	exit( 1 )
end


find_library( 'verse', 'verse_callback_set' ) or
	fail( "Could not find Verse library (http://verse.blender.org/download/)." )
find_header( 'verse.h' )  or fail( "missing verse.h" )

have_header( 'stdio.h' )    or fail( "missing stdio.h" )
have_header( 'string.h' )   or fail( "missing string.h" )
have_header( 'inttypes.h' ) or fail( "missing inttypes.h" )

# find_library( 'efence', 'malloc', *ADDITIONAL_INCLUDE_DIRS )

create_makefile( 'verse_ext' )
