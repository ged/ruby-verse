/* 
 * Verse::Session -- Verse session class
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

VALUE rbverse_cVerseSession;

static VALUE rbverse_session_mutex;

static VALUE sym_connect_accept;

	// sym_node_create, sym_node_destroy, sym_node_subscribe, sym_node_unsubscribe,
	// sym_tag_group_create, sym_tag_group_destroy, sym_tag_group_subscribe,
	// sym_tag_group_unsubscribe, sym_tag_create, sym_tag_destroy, sym_node_name_set,
	// sym_o_transform_pos_real32, sym_o_transform_rot_real32, sym_o_transform_scale_real32,
	// sym_o_transform_pos_real64, sym_o_transform_rot_real64, sym_o_transform_scale_real64,
	// sym_o_transform_subscribe, sym_o_transform_unsubscribe, sym_o_light_set, sym_o_link_set,
	// sym_o_link_destroy, sym_o_method_group_create, sym_o_method_group_destroy,
	// sym_o_method_group_subscribe, sym_o_method_group_unsubscribe, sym_o_method_create,
	// sym_o_method_destroy, sym_o_method_call, sym_o_anim_run, sym_o_hide,
	// sym_g_layer_create, sym_g_layer_destroy, sym_g_layer_subscribe, sym_g_layer_unsubscribe,
	// sym_g_vertex_set_xyz_real32, sym_g_vertex_delete_real32, sym_g_vertex_set_xyz_real64,
	// sym_g_vertex_delete_real64, sym_g_vertex_set_uint32, sym_g_vertex_set_real64,
	// sym_g_vertex_set_real32, sym_g_polygon_set_corner_uint32, sym_g_polygon_delete,
	// sym_g_polygon_set_corner_real64, sym_g_polygon_set_corner_real32, sym_g_polygon_set_face_uint8,
	// sym_g_polygon_set_face_uint32, sym_g_polygon_set_face_real64, sym_g_polygon_set_face_real32,
	// sym_g_crease_set_vertex, sym_g_crease_set_edge, sym_g_bone_create, sym_g_bone_destroy,
	// sym_m_fragment_create, sym_m_fragment_destroy, sym_b_dimensions_set, sym_b_layer_create,
	// sym_b_layer_destroy, sym_b_layer_subscribe, sym_b_layer_unsubscribe, sym_b_tile_set,
	// sym_t_language_set, sym_t_buffer_create, sym_t_buffer_destroy, sym_t_buffer_subscribe,
	// sym_t_buffer_unsubscribe, sym_t_text_set, sym_c_curve_create, sym_c_curve_destroy,
	// sym_c_curve_subscribe, sym_c_curve_unsubscribe, sym_c_key_set, sym_c_key_destroy,
	// sym_a_buffer_create, sym_a_buffer_destroy, sym_a_buffer_subscribe, sym_a_buffer_unsubscribe,
	// sym_a_block_set, sym_a_block_clear, sym_a_stream_create, sym_a_stream_destroy,
	// sym_a_stream_subscribe, sym_a_stream_unsubscribe, sym_a_stream

/* --------------------------------------------------
 *	Memory-management functions
 * -------------------------------------------------- */

/*
 * Allocation function
 */
static rbverse_SESSION *
rbverse_session_alloc( void ) {
	rbverse_SESSION *ptr = ALLOC( rbverse_SESSION );

	ptr->id        = NULL;
	ptr->address   = Qnil;
	ptr->callbacks = Qnil;
	ptr->self      = Qnil;

	rbverse_log( "debug", "initialized a rbverse_SESSION <%p>", ptr );
	return ptr;
}


/*
 * GC Mark function
 */
static void
rbverse_session_gc_mark( rbverse_SESSION *ptr ) {
	if ( ptr ) {
		rb_gc_mark( ptr->address );
		rb_gc_mark( ptr->callbacks );
		/* Don't need to mark ->self */
	}
}



/*
 * GC Free function
 */
static void
rbverse_session_gc_free( rbverse_SESSION *ptr ) {
	if ( ptr ) {
		/* TODO: terminate the session and remove it from the session table if it's connected */
		if ( ptr->id ) {
			verse_session_destroy( ptr->id );
		}

		ptr->id         = NULL;
		ptr->address    = Qnil;
		ptr->callbacks  = Qnil;
		ptr->self       = Qnil;

		xfree( ptr );
		ptr = NULL;
	}
}


/*
 * Object validity checker. Returns the data pointer.
 */
static rbverse_SESSION *
check_session( VALUE self ) {
	Check_Type( self, T_DATA );

    if ( !IsSession(self) ) {
		rb_raise( rb_eTypeError, "wrong argument type %s (expected Verse::Session)",
				  rb_obj_classname(self) );
    }

	return DATA_PTR( self );
}


/*
 * Fetch the data pointer and check it for sanity.
 */
rbverse_SESSION *
rbverse_get_session( VALUE self ) {
	rbverse_SESSION *session = check_session( self );

	if ( !session )
		rb_fatal( "Use of uninitialized Session." );

	return session;
}


/*
 * Create a Verse::Session from a VSession.
 */
VALUE
rbverse_verse_session_from_vsession( VSession id, VALUE address ) {
	VALUE session_obj = rb_class_new_instance( 1, &address, rbverse_cVerseSession );
	rbverse_SESSION *session = check_session( session_obj );

	session->id = id;

	return session_obj;
}



/* --------------------------------------------------------------
 * Class methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::Session.allocate   -> session
 * 
 * Allocate a new Verse::Session object.
 * 
 */
static VALUE
rbverse_verse_session_s_allocate( VALUE klass ) {
	return Data_Wrap_Struct( klass, rbverse_session_gc_mark, rbverse_session_gc_free, 0 );
}


/* Call the synchronized block after acquiring the mutex */
static VALUE
rbverse_verse_session_s_synchronize_body( VALUE block ) {
	return rb_funcall( block, rb_intern("call"), 0 );
}


/*
 * call-seq:
 *    Verse::Session.synchronize { ... }
 * 
 * Call the block after acquiring the global session lock.
 * 
 */
static VALUE
rbverse_verse_session_s_synchronize( int argc, VALUE *argv, VALUE klass ) {
	VALUE block;

	if ( !rb_block_given_p() )
		rb_raise( rb_eLocalJumpError, "no block given" );

	block = rb_block_proc();

	return rb_mutex_synchronize( rbverse_session_mutex, rbverse_verse_session_s_synchronize_body, 
	                             block );
}



/* --------------------------------------------------------------
 * Instance methods
 * -------------------------------------------------------------- */

/*
 *  call-seq:
 *     Verse::Session.new( address )                  -> session
 *
 *  Create a new Verse::Session object that will use the given +address+. 
 *  
 *  @param [String] address  the address to bind/connect to
 */
static VALUE
rbverse_verse_session_initialize( int argc, VALUE *argv, VALUE self ) {
	if ( !check_session(self) ) {
		rbverse_SESSION *session;
		VALUE address = Qnil;

		rbverse_log( "debug", "Initializing: %s", RSTRING_PTR(rb_inspect(self)) );
		DATA_PTR( self ) = session = rbverse_session_alloc();

		if ( rb_scan_args(argc, argv, "01", &address) ) {
			SafeStringValue( address );
			session->address = address;
		}

		session->callbacks = rb_hash_new();
		session->self = self;

	} else {
		rb_raise( rb_eRuntimeError,
				  "Cannot re-initialize a session once it's been created." );
	}

	return self;
}


/*
 *  call-seq:
 *     session.address   -> string
 *
 *  Return the address of the Verse server the session is associated with.
 *
 */
static VALUE
rbverse_verse_session_address( VALUE self ) {
	rbverse_SESSION *session = rbverse_get_session( self );
	return rb_obj_clone( session->address );
}


/*
 *  call-seq:
 *     session.address = string
 *
 *  Set the address of the Verse server the session is associated with.
 *
 */
static VALUE
rbverse_verse_session_address_eq( VALUE self, VALUE address ) {
	rbverse_SESSION *session = rbverse_get_session( self );

	SafeStringValue( address );
	session->address = rb_obj_clone( address );

	return Qtrue;
}


/*
 *  call-seq:
 *     session.callbacks   -> hash
 *
 *  The hash of callbacks registered for this session.
 *
 */
static VALUE
rbverse_verse_session_callbacks( VALUE self ) {
	rbverse_SESSION *session = rbverse_get_session( self );
	return session->callbacks;
}


/*
 *  call-seq:
 *     session.connected?   -> true or false
 *
 *  Returns +true+ if the session has connected with the Verse server
 *
 */
static VALUE
rbverse_verse_session_connected_p( VALUE self ) {
	rbverse_SESSION *session = rbverse_get_session( self );
	return ( session->id ? Qtrue : Qfalse );
}


/*
 * Do the connection once the session mutex is acquired.
 */
static VALUE
rbverse_verse_session_connect_body( VALUE args ) {
	VALUE self, name, pass, expected_host_id;
	rbverse_SESSION *session;
	const char *name_str, *pass_str, *addr_str;
	const uint8 *hostid = NULL;

	self             = rb_ary_entry( args, 0 );
	name             = rb_ary_entry( args, 1 );
	pass             = rb_ary_entry( args, 2 );
	expected_host_id = rb_ary_entry( args, 3 );

	session = rbverse_get_session( self );
	name_str = RSTRING_PTR( name );
	pass_str = RSTRING_PTR( pass );
	addr_str = RSTRING_PTR( session->address );

	if ( RTEST(expected_host_id) )
		hostid = (uint8 *)(StringValuePtr( expected_host_id ));

	rbverse_log_with_context( self, "debug", "Sending 'connect' to %s", addr_str );
	session->id = verse_send_connect( name_str, pass_str, addr_str, hostid );
	rbverse_log_with_context( self, "debug", "  session: %p", session->id );

	if ( !session->id )
		rb_raise( rbverse_eVerseConnectError, "Couldn't create connection to '%s'.", addr_str );

	return Qtrue;
}


/*
 *  call-seq:
 *     session.connect( name, pass, expected_host_id=nil )
 *
 *  Send the connect event to the Verse server.
 *
 */
static VALUE
rbverse_verse_session_connect( int argc, VALUE *argv, VALUE self ) {
	rbverse_SESSION *session = rbverse_get_session( self );
	VALUE args, name, pass, expected_host_id = Qnil;
	VALUE rval = Qnil;

	if ( !RTEST(session->address) )
		rb_raise( rb_eRuntimeError, "No address set." );
	if ( session->id )
		rb_raise( rb_eRuntimeError, "Session already established." );

	rb_scan_args( argc, argv, "21", &name, &pass, &expected_host_id );

	rbverse_log_with_context( self, "debug", "Acquiring session lock for new session." );
	args = rb_ary_new3( 4, self, name, pass, expected_host_id );
	rval = rb_mutex_synchronize( rbverse_session_mutex, rbverse_verse_session_connect_body, args );
	rbverse_log_with_context( self, "debug", "  releasing session lock." );

	return rval;
}


/*
 * Ruby handler for the 'connect_accept' message; called after re-establishing the GVL
 * from rbverse_handle_connect_accept().
 */
static void *
rbverse_handle_connect_accept_body( void *ptr ) {
	rbverse_CONNECT_ACCEPT_EVENT *event = (rbverse_CONNECT_ACCEPT_EVENT *)ptr;
	rbverse_SESSION *session = event->session;
	VALUE block = Qnil;
	VALUE avatarid, addrstring, hostid_string;

	if ( !session )
		rb_fatal( "Pointer to session was NULL!" );

	avatarid = INT2FIX( event->avatar );
	addrstring = rb_str_new2( event->address );
	hostid_string = rbverse_host_id2str( event->hostid );
	block = rb_hash_aref( session->callbacks, sym_connect_accept );

	if ( RTEST(block) ) {
		rb_funcall( block, rb_intern("call"), 4, session->self, avatarid, addrstring, hostid_string );
	}

	return NULL;
}


/*
 * Verse callback for the 'connect_accept' message
 */
static void
rbverse_cb_connect_accept( void *userdata, VNodeID avatar, const char *address, uint8 *host_id ) {
	rbverse_CONNECT_ACCEPT_EVENT *event = malloc( sizeof(rbverse_CONNECT_ACCEPT_EVENT) );

	printf( ">>> connect_accept callback: userdata is: %p\n", userdata );
	fflush( stdout );

	/* If we can't malloc, acquire the gvl to rb_sys_fail() */
	if ( event ) {
		event->session = (rbverse_SESSION *)userdata;
		event->avatar  = avatar;
		event->address = address;
		event->hostid  = host_id;

		rb_thread_call_with_gvl( rbverse_handle_connect_accept_body, (void *)event );

		/* Handle exception case? */
		free( event );
	} else {
		free( event );
		const char *msg = "while handling connect_accept";
		rb_thread_call_with_gvl( rbverse_sysfail, (void *)msg );
	}

}


/*
 * Synchronized callback setter for #on_connect_accept
 */
static VALUE
rbverse_set_connect_accept( VALUE self ) {
	rbverse_SESSION *session = rbverse_get_session( self );

	if ( !session->id )
		rb_raise( rbverse_eVerseSessionError, "not connected." );

	rbverse_log_with_context( self, "debug",
	                          "Setting connect_accept callback with user data = %p", session );
	verse_callback_set( verse_send_connect_accept, rbverse_cb_connect_accept, (void *)session );

	return Qtrue;
}


/*
 * call-seq:
 *    session.on_connect_accept {|avatar| block }
 *
 * Callback for the 'connect_accept' event. 
 *
 * @yield [avatar, hostid]  Called when the event is received.
 * @yieldparam [Verse::Node]    avatar  the object node that acts as the client's 
 *                                      representation on the Verse host
 * @yieldparam [String]         hostid  the server's host ID key
 */
static VALUE
rbverse_verse_session_on_connect_accept( VALUE self ) {
	rbverse_SESSION *session = rbverse_get_session( self );
	VALUE block = Qnil;

	if ( rb_block_given_p() ) {
		rbverse_log_with_context( self, "debug",
			"Setting connect_accept callback to: %s", RSTRING_PTR(rb_inspect( block )) );

		rbverse_log_with_context( self, "debug", "Acquiring session lock for session %p", session->id );
		rb_hash_aset( session->callbacks, sym_connect_accept, block );
		rb_mutex_synchronize( rbverse_session_mutex, rbverse_set_connect_accept, self );
		rbverse_log_with_context( self, "debug", "  done with session lock for session %p", session->id );
	} else {
		block = rb_hash_aref( session->callbacks, sym_connect_accept );
	}

	return block;
}


/*
 * call-seq:
 *    session.on_connect_accept = handler
 * 
 * Set the callback for the 'connect_accept' event.
 * 
 * @param [#call] handler  A callable object that will handle the event. See #on_connect_accept for
 *                         details.
 */
static VALUE
rbverse_verse_session_on_connect_accept_eq( VALUE self, VALUE handler ) {
	rbverse_SESSION *session = rbverse_get_session( self );

	if ( !rb_respond_to(handler, rb_intern( "call" )) )
		rb_raise( rb_eTypeError, "expected %s to respond to #call",
		          RSTRING_PTR(rb_inspect( handler )) );

	rbverse_log_with_context( self, "debug",
	                          "Setting connect_accept callback to: %s",
	                          RSTRING_PTR(rb_inspect( handler )) );

	rbverse_log_with_context( self, "debug", "Acquiring session lock for session %p", session->id );
	rb_hash_aset( session->callbacks, sym_connect_accept, handler );
	rb_mutex_synchronize( rbverse_session_mutex, rbverse_set_connect_accept, self );
	rbverse_log_with_context( self, "debug", "  done with session lock for session %p", session->id );

	return handler;
}


/* 
 * call-seq: 
 *    session.terminate( address, message )
 * 
 * Sent by a client that wishes to disconnect from a host, before
 * actually going ahead and severing the connection. This gives the
 * host a chance to gracefully free any resources dedicated to this
 * client, rather than having to find out after the fact that it has
 * disconnected.
 * 
 * This command needs to be sent even when there is no established
 * connection, so it also requires the address of the machine to send
 * it to. This information is implementation/API-level though, it does
 * not appear in the command's network representation.
 * 
 */
static VALUE
rbverse_verse_session_terminate( VALUE self, VALUE message ) {
	rbverse_SESSION *session = rbverse_get_session( self );
	const char *msg;

	SafeStringValue( message );
	msg = RSTRING_PTR( message );

	verse_send_connect_terminate( RSTRING_PTR(session->address), msg );

	return Qtrue;
}




/*
 * Verse::Session class
 */
void
rbverse_init_verse_session( void ) {
	rbverse_log( "debug", "Initializing Verse::Session" );

	rb_global_variable( &rbverse_session_mutex );
	rbverse_session_mutex = rb_mutex_new();

#ifdef FOR_RDOC
	rbverse_mVerse = rb_define_module( "Verse" );
#endif

	/* Class methods */
	rbverse_cVerseSession = rb_define_class_under( rbverse_mVerse, "Session", rb_cObject );
	rb_define_alloc_func( rbverse_cVerseSession, rbverse_verse_session_s_allocate );

	rb_define_singleton_method( rbverse_cVerseSession, "synchronize",
	                            rbverse_verse_session_s_synchronize, -1 );

	/* Initializer */
	rb_define_method( rbverse_cVerseSession, "initialize", rbverse_verse_session_initialize, -1 );

	/* Public instance methods */
	rb_define_method( rbverse_cVerseSession, "address", rbverse_verse_session_address, 0 );
	rb_define_method( rbverse_cVerseSession, "address=", rbverse_verse_session_address_eq, 1 );

	rb_define_method( rbverse_cVerseSession, "callbacks", rbverse_verse_session_callbacks, 0 );
	rb_define_method( rbverse_cVerseSession, "connected?", rbverse_verse_session_connected_p, 0 );

	rb_define_method( rbverse_cVerseSession, "connect", rbverse_verse_session_connect, -1 );

	rb_define_method( rbverse_cVerseSession, "on_connect_accept",
	                  rbverse_verse_session_on_connect_accept, 0 );
	rb_define_method( rbverse_cVerseSession, "on_connect_accept=",
	                  rbverse_verse_session_on_connect_accept_eq, 1 );

	rb_define_method( rbverse_cVerseSession, "terminate", rbverse_verse_session_terminate, 1 );

	// node_index_subscribe(uint32 mask);
	// node_create(VNodeID node_id, VNodeType type, VNodeOwner owner);
	// node_destroy(VNodeID node_id);
	// node_subscribe(VNodeID node_id);
	// node_unsubscribe(VNodeID node_id);

	// tag_group_create(VNodeID node_id, uint16 group_id, const char *name);
	// tag_group_destroy(VNodeID node_id, uint16 group_id);
	// tag_group_subscribe(VNodeID node_id, uint16 group_id);
	// tag_group_unsubscribe(VNodeID node_id, uint16 group_id);
	// tag_create(VNodeID node_id, uint16 group_id, uint16 tag_id, const char *name, VNTagType type, 
	//     const VNTag *tag);
	// tag_destroy(VNodeID node_id, uint16 group_id, uint16 tag_id);

	// node_name_set(VNodeID node_id, const char *name);

	// sym_connect_accept              = ID2SYM( rb_intern("connect_accept") );

	// sym_node_create                 = ID2SYM( rb_intern("node_create") );
	// sym_node_destroy                = ID2SYM( rb_intern("node_destroy") );
	// sym_node_subscribe              = ID2SYM( rb_intern("node_subscribe") );
	// sym_node_unsubscribe            = ID2SYM( rb_intern("node_unsubscribe") );
	// sym_tag_group_create            = ID2SYM( rb_intern("tag_group_create") );
	// sym_tag_group_destroy           = ID2SYM( rb_intern("tag_group_destroy") );
	// sym_tag_group_subscribe         = ID2SYM( rb_intern("tag_group_subscribe") );
	// sym_tag_group_unsubscribe       = ID2SYM( rb_intern("tag_group_unsubscribe") );
	// sym_tag_create                  = ID2SYM( rb_intern("tag_create") );
	// sym_tag_destroy                 = ID2SYM( rb_intern("tag_destroy") );
	// sym_node_name_set               = ID2SYM( rb_intern("node_name_set") );
	// sym_o_transform_pos_real32      = ID2SYM( rb_intern("o_transform_pos_real32") );
	// sym_o_transform_rot_real32      = ID2SYM( rb_intern("o_transform_rot_real32") );
	// sym_o_transform_scale_real32    = ID2SYM( rb_intern("o_transform_scale_real32") );
	// sym_o_transform_pos_real64      = ID2SYM( rb_intern("o_transform_pos_real64") );
	// sym_o_transform_rot_real64      = ID2SYM( rb_intern("o_transform_rot_real64") );
	// sym_o_transform_scale_real64    = ID2SYM( rb_intern("o_transform_scale_real64") );
	// sym_o_transform_subscribe       = ID2SYM( rb_intern("o_transform_subscribe") );
	// sym_o_transform_unsubscribe     = ID2SYM( rb_intern("o_transform_unsubscribe") );
	// sym_o_light_set                 = ID2SYM( rb_intern("o_light_set") );
	// sym_o_link_set                  = ID2SYM( rb_intern("o_link_set") );
	// sym_o_link_destroy              = ID2SYM( rb_intern("o_link_destroy") );
	// sym_o_method_group_create       = ID2SYM( rb_intern("o_method_group_create") );
	// sym_o_method_group_destroy      = ID2SYM( rb_intern("o_method_group_destroy") );
	// sym_o_method_group_subscribe    = ID2SYM( rb_intern("o_method_group_subscribe") );
	// sym_o_method_group_unsubscribe  = ID2SYM( rb_intern("o_method_group_unsubscribe") );
	// sym_o_method_create             = ID2SYM( rb_intern("o_method_create") );
	// sym_o_method_destroy            = ID2SYM( rb_intern("o_method_destroy") );
	// sym_o_method_call               = ID2SYM( rb_intern("o_method_call") );
	// sym_o_anim_run                  = ID2SYM( rb_intern("o_anim_run") );
	// sym_o_hide                      = ID2SYM( rb_intern("o_hide") );
	// sym_g_layer_create              = ID2SYM( rb_intern("g_layer_create") );
	// sym_g_layer_destroy             = ID2SYM( rb_intern("g_layer_destroy") );
	// sym_g_layer_subscribe           = ID2SYM( rb_intern("g_layer_subscribe") );
	// sym_g_layer_unsubscribe         = ID2SYM( rb_intern("g_layer_unsubscribe") );
	// sym_g_vertex_set_xyz_real32     = ID2SYM( rb_intern("g_vertex_set_xyz_real32") );
	// sym_g_vertex_delete_real32      = ID2SYM( rb_intern("g_vertex_delete_real32") );
	// sym_g_vertex_set_xyz_real64     = ID2SYM( rb_intern("g_vertex_set_xyz_real64") );
	// sym_g_vertex_delete_real64      = ID2SYM( rb_intern("g_vertex_delete_real64") );
	// sym_g_vertex_set_uint32         = ID2SYM( rb_intern("g_vertex_set_uint32") );
	// sym_g_vertex_set_real64         = ID2SYM( rb_intern("g_vertex_set_real64") );
	// sym_g_vertex_set_real32         = ID2SYM( rb_intern("g_vertex_set_real32") );
	// sym_g_polygon_set_corner_uint32 = ID2SYM( rb_intern("g_polygon_set_corner_uint32") );
	// sym_g_polygon_delete            = ID2SYM( rb_intern("g_polygon_delete") );
	// sym_g_polygon_set_corner_real64 = ID2SYM( rb_intern("g_polygon_set_corner_real64") );
	// sym_g_polygon_set_corner_real32 = ID2SYM( rb_intern("g_polygon_set_corner_real32") );
	// sym_g_polygon_set_face_uint8    = ID2SYM( rb_intern("g_polygon_set_face_uint8") );
	// sym_g_polygon_set_face_uint32   = ID2SYM( rb_intern("g_polygon_set_face_uint32") );
	// sym_g_polygon_set_face_real64   = ID2SYM( rb_intern("g_polygon_set_face_real64") );
	// sym_g_polygon_set_face_real32   = ID2SYM( rb_intern("g_polygon_set_face_real32") );
	// sym_g_crease_set_vertex         = ID2SYM( rb_intern("g_crease_set_vertex") );
	// sym_g_crease_set_edge           = ID2SYM( rb_intern("g_crease_set_edge") );
	// sym_g_bone_create               = ID2SYM( rb_intern("g_bone_create") );
	// sym_g_bone_destroy              = ID2SYM( rb_intern("g_bone_destroy") );
	// sym_m_fragment_create           = ID2SYM( rb_intern("m_fragment_create") );
	// sym_m_fragment_destroy          = ID2SYM( rb_intern("m_fragment_destroy") );
	// sym_b_dimensions_set            = ID2SYM( rb_intern("b_dimensions_set") );
	// sym_b_layer_create              = ID2SYM( rb_intern("b_layer_create") );
	// sym_b_layer_destroy             = ID2SYM( rb_intern("b_layer_destroy") );
	// sym_b_layer_subscribe           = ID2SYM( rb_intern("b_layer_subscribe") );
	// sym_b_layer_unsubscribe         = ID2SYM( rb_intern("b_layer_unsubscribe") );
	// sym_b_tile_set                  = ID2SYM( rb_intern("b_tile_set") );
	// sym_t_language_set              = ID2SYM( rb_intern("t_language_set") );
	// sym_t_buffer_create             = ID2SYM( rb_intern("t_buffer_create") );
	// sym_t_buffer_destroy            = ID2SYM( rb_intern("t_buffer_destroy") );
	// sym_t_buffer_subscribe          = ID2SYM( rb_intern("t_buffer_subscribe") );
	// sym_t_buffer_unsubscribe        = ID2SYM( rb_intern("t_buffer_unsubscribe") );
	// sym_t_text_set                  = ID2SYM( rb_intern("t_text_set") );
	// sym_c_curve_create              = ID2SYM( rb_intern("c_curve_create") );
	// sym_c_curve_destroy             = ID2SYM( rb_intern("c_curve_destroy") );
	// sym_c_curve_subscribe           = ID2SYM( rb_intern("c_curve_subscribe") );
	// sym_c_curve_unsubscribe         = ID2SYM( rb_intern("c_curve_unsubscribe") );
	// sym_c_key_set                   = ID2SYM( rb_intern("c_key_set") );
	// sym_c_key_destroy               = ID2SYM( rb_intern("c_key_destroy") );
	// sym_a_buffer_create             = ID2SYM( rb_intern("a_buffer_create") );
	// sym_a_buffer_destroy            = ID2SYM( rb_intern("a_buffer_destroy") );
	// sym_a_buffer_subscribe          = ID2SYM( rb_intern("a_buffer_subscribe") );
	// sym_a_buffer_unsubscribe        = ID2SYM( rb_intern("a_buffer_unsubscribe") );
	// sym_a_block_set                 = ID2SYM( rb_intern("a_block_set") );
	// sym_a_block_clear               = ID2SYM( rb_intern("a_block_clear") );
	// sym_a_stream_create             = ID2SYM( rb_intern("a_stream_create") );
	// sym_a_stream_destroy            = ID2SYM( rb_intern("a_stream_destroy") );
	// sym_a_stream_subscribe          = ID2SYM( rb_intern("a_stream_subscribe") );
	// sym_a_stream_unsubscribe        = ID2SYM( rb_intern("a_stream_unsubscribe") );
	// sym_a_stream                    = ID2SYM( rb_intern("a_stream") );
}

