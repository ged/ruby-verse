/* 
 * Verse::TextNode -- Verse Text Node class
 * $Id$
 * 
 * @author Michael Granger <ged@FaerieMUD.org>
 * 
 * Copyright (c) 2010 The FaerieMUD Consortium
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

VALUE rbverse_cVerseTextNode;


/*
 * Mark the text part of a node.
 */
static void
rbverse_textnode_gc_mark( rbverse_NODE *ptr ) {
	if ( ptr ) {
		/* TODO: Mark child-specific data */
	}
}


/*
 * Free the text part of a node.
 */
static void
rbverse_textnode_gc_free( rbverse_NODE *ptr ) {
	if ( ptr ) {
		/* TODO: Free child-specific data */
	}
}


/* --------------------------------------------------------------
 * Instance Methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::TextNode.initialize
 *
 * Set up a new Verse::TextNode.
 */
static VALUE
rbverse_verse_textnode_initialize( VALUE self ) {
	rbverse_NODE *ptr;

	rb_call_super( 0, NULL );

	ptr = rbverse_get_node( self );

	/* TODO: Initialize text-specific instance data. */

	return self;
}


/*
 * Verse::TextNode class
 */
void
rbverse_init_verse_textnode( void ) {
	rbverse_log( "debug", "Initializing Verse::TextNode" );

	/* Class methods */
	rbverse_cVerseTextNode = rb_define_class_under( rbverse_mVerse, "TextNode", rbverse_cVerseNode );

	/* Initializer */
	rb_define_method( rbverse_cVerseTextNode, "initialize", rbverse_verse_textnode_initialize, 0 );

	/* Tell Verse::Node about this subclass */
	rbverse_nodetype_to_nodeclass[ V_NT_TEXT ] = rbverse_cVerseTextNode;
	node_mark_funcs[ V_NT_TEXT ] = &rbverse_textnode_gc_mark;
	node_free_funcs[ V_NT_TEXT ] = &rbverse_textnode_gc_free;
}

