/* 
 * Verse::AudioNode -- Verse Audio Node class
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

VALUE rbverse_cVerseAudioNode;


/*
 * Mark the audio part of a node.
 */
static void
rbverse_audionode_gc_mark( rbverse_NODE *ptr ) {
	if ( ptr ) {
		rb_gc_mark( ptr->audio.buffers );
		rb_gc_mark( ptr->audio.streams );
	}
}


/*
 * Free the audio part of a node.
 */
static void
rbverse_audionode_gc_free( rbverse_NODE *ptr ) {
	if ( ptr ) {
		ptr->audio.buffers = Qnil;
		ptr->audio.streams = Qnil;
	}
}


/* --------------------------------------------------------------
 * Instance Methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::AudioNode.initialize
 *
 * Set up a new Verse::AudioNode.
 */
static VALUE
rbverse_verse_audionode_initialize( VALUE self ) {
	rbverse_NODE *ptr;

	rb_call_super( 0, NULL );

	ptr = rbverse_get_node( self );

	ptr->audio.buffers = rb_ary_new();
	ptr->audio.streams = rb_ary_new();

	return self;
}



/*
 * Verse::AudioNode class
 */
void
rbverse_init_verse_audionode( void ) {
	rbverse_log( "debug", "Initializing Verse::AudioNode" );

	/* Class methods */
	rbverse_cVerseAudioNode = rb_define_class_under( rbverse_mVerse, "AudioNode", rbverse_cVerseNode );

    /* Constants */
	rb_define_const( rbverse_cVerseAudioNode, "TYPE_NUMBER", INT2FIX(V_NT_AUDIO) );

	/* Initializer */
	rb_define_method( rbverse_cVerseAudioNode, "initialize", rbverse_verse_audionode_initialize, 0 );

	/* Tell Verse::Node about this subclass */
	rbverse_nodetype_to_nodeclass[ V_NT_AUDIO ] = rbverse_cVerseAudioNode;
	node_mark_funcs[ V_NT_AUDIO ] = &rbverse_audionode_gc_mark;
	node_free_funcs[ V_NT_AUDIO ] = &rbverse_audionode_gc_free;

	// verse_callback_set( verse_send_a_buffer_create, rbverse_a_buffer_create_callback, NULL );
	// verse_callback_set( verse_send_a_buffer_destroy, rbverse_a_buffer_destroy_callback, NULL );
	// verse_callback_set( verse_send_a_buffer_subscribe, rbverse_a_buffer_subscribe_callback, NULL );
	// verse_callback_set( verse_send_a_buffer_unsubscribe, rbverse_a_buffer_unsubscribe_callback, NULL );
	// verse_callback_set( verse_send_a_block_set, rbverse_a_block_set_callback, NULL );
	// verse_callback_set( verse_send_a_block_clear, rbverse_a_block_clear_callback, NULL );
	// verse_callback_set( verse_send_a_stream_create, rbverse_a_stream_create_callback, NULL );
	// verse_callback_set( verse_send_a_stream_destroy, rbverse_a_stream_destroy_callback, NULL );
	// verse_callback_set( verse_send_a_stream_subscribe, rbverse_a_stream_subscribe_callback, NULL );
	// verse_callback_set( verse_send_a_stream_unsubscribe, rbverse_a_stream_unsubscribe_callback, NULL );
	// verse_callback_set( verse_send_a_stream, rbverse_a_stream_callback, NULL );

}

