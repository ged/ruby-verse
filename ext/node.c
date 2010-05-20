/* 
 * Verse::Node -- Verse node class
 * $Id$
 * 
 * @author Michael Granger <ged@FaerieMUD.org>
 * 
 * Copyright (c) 2010 Michael Granger
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *  
 *  * Redistributions in binary form must reproduce the above copyright notice, this
 *    list of conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *  
 *  * Neither the name of the authors, nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior
 *    written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * 
 */

#include "verse_ext.h"

VALUE rbverse_cVerseNode;



/* --------------------------------------------------
 *	Memory-management functions
 * -------------------------------------------------- */

/*
 * Allocation function
 */
static rbverse_NODE *
rbverse_node_alloc( void ) {
	rbverse_NODE *ptr = ALLOC( rbverse_NODE );

	rbverse_log( "debug", "initialized a rbverse_NODE <%p>", ptr );
	return ptr;
}


/*
 * GC Mark function
 */
static void
rbverse_node_gc_mark( rbverse_NODE *ptr ) {
	if ( ptr ) {
		// Mark
	}
}



/*
 * GC Free function
 */
static void
rbverse_node_gc_free( rbverse_NODE *ptr ) {
	if ( ptr ) {
		// Free attributes

		xfree( ptr );
		ptr = NULL;
	}
}


/*
 * Object validity checker. Returns the data pointer.
 */
static rbverse_NODE *
check_node( VALUE self ) {
	Check_Type( self, T_DATA );

    if ( !IsNode(self) ) {
		rb_raise( rb_eTypeError, "wrong argument type %s (expected Verse::Node)",
				  rb_obj_classname(self) );
    }

	return DATA_PTR( self );
}


/*
 * Fetch the data pointer and check it for sanity.
 */
rbverse_NODE *
rbverse_get_node( VALUE self ) {
	rbverse_NODE *node = check_node( self );

	if ( !node )
		rb_fatal( "Use of uninitialized Node." );

	return node;
}


/*
 * Create a Verse::Node from a VNode.
 */
VALUE
rbverse_verse_node_from_vnode( VNodeID node_id, VALUE name ) {
	VALUE node_obj = rb_class_new_instance( 1, &name, rbverse_cVerseNode );
	rbverse_NODE *node = check_node( node_obj );

	node->id = node_id;

	return node_obj;
}



/* --------------------------------------------------------------
 * Class methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::Node.allocate   -> node
 * 
 * Allocate a new Verse::Node object.
 * 
 */
static VALUE
rbverse_verse_node_s_allocate( VALUE klass ) {
	return Data_Wrap_Struct( klass, rbverse_node_gc_mark, rbverse_node_gc_free, 0 );
}



/* --------------------------------------------------------------
 * Instance methods
 * -------------------------------------------------------------- */

/*
 *  call-seq:
 *     Verse::Node.new( name )                  -> node
 *
 *  Create a new Verse::Node object with the given +name+.
 *  
 *  @param [String] address  the address to bind/connect to
 */
static VALUE
rbverse_verse_node_initialize( int argc, VALUE *argv, VALUE self ) {
	if ( !check_node(self) ) {

	} else {
		rb_raise( rb_eRuntimeError,
				  "Cannot re-initialize a node once it's been created." );
	}

	return self;
}



/*
 * Verse::Node class
 */
void
rbverse_init_verse_node( void ) {
	rbverse_log( "debug", "Initializing Verse::Node" );

#ifdef FOR_RDOC
	rbverse_mVerse = rb_define_module( "Verse" );
#endif

	/* Class methods */
	rbverse_cVerseNode = rb_define_class_under( rbverse_mVerse, "Node", rb_cObject );
	rb_define_alloc_func( rbverse_cVerseNode, rbverse_verse_node_s_allocate );


	/* Initializer */
	rb_define_method( rbverse_cVerseNode, "initialize", rbverse_verse_node_initialize, -1 );

}

