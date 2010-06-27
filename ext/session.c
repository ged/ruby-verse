/* 
 * Verse::Session -- Verse session class
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

VALUE rbverse_cVerseSession;
VALUE rbverse_mVerseSessionObserver;

static VALUE rbverse_session_mutex;
static st_table *session_table;

/* Structs for passing callback data back into Ruby */
struct rbverse_node_create_event {
	VNodeID node_id;
	VNodeType type;
	VNodeOwner owner;
};

struct rbverse_connect_accept_event {
	VNodeID		    avatar;
	const char	    *address;
	uint8		    *hostid;
};



/* --------------------------------------------------
 *	Memory-management functions
 * -------------------------------------------------- */

/*
 * Allocation function
 */
static struct rbverse_session *
rbverse_session_alloc( void ) {
	struct rbverse_session *ptr = ALLOC( struct rbverse_session );

	ptr->id                = NULL;
	ptr->address           = Qnil;
	ptr->create_callbacks  = Qnil;
	ptr->destroy_callbacks = Qnil;

	DEBUGMSG( "allocated a rbverse_SESSION <%p>", ptr );
	return ptr;
}


/*
 * GC Mark function
 */
static void
rbverse_session_gc_mark( struct rbverse_session *ptr ) {
	if ( ptr ) {
		rb_gc_mark( ptr->address );
		rb_gc_mark( ptr->create_callbacks );
		rb_gc_mark( ptr->destroy_callbacks );
	}
}



/*
 * GC Free function
 */
static void
rbverse_session_gc_free( struct rbverse_session *ptr ) {
	if ( ptr ) {
		/* TODO: terminate the session and remove it from the session table if it's connected */
		if ( ptr->id ) {
			verse_session_destroy( ptr->id );
		}

		ptr->id                = NULL;
		ptr->address           = Qnil;
		ptr->create_callbacks  = Qnil;
		ptr->destroy_callbacks = Qnil;

		xfree( ptr );
		ptr = NULL;
	}
}


/*
 * Object validity checker. Returns the data pointer.
 */
static struct rbverse_session *
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
static struct rbverse_session *
rbverse_get_session( VALUE self ) {
	struct rbverse_session *session = check_session( self );

	if ( !session )
		rb_fatal( "Use of uninitialized Session." );

	return session;
}


/*
 * Get the Verse::Session object that corresponds to the current Verse session, or
 * Qnil if there isn't one.
 */
VALUE
rbverse_get_current_session() {
	VSession id = verse_session_get();
	VALUE session;

	if ( st_lookup(session_table, (st_data_t)id, (st_data_t *)&session) ) {
		rbverse_log( "debug", "Got session object %s for session ID %p",
			RSTRING_PTR(rb_inspect( session )), id );
		return session;
	} else {
		rbverse_log( "warn", "No session object for session ID %p", id );
		return Qnil;
	}
}


/* 
 * Call the specified +func+ with the given +arg+ after setting the current 
 * verse session to +id+.
 */
VALUE
rbverse_with_session_lock( VALUE sessionobj, VALUE (*func)(ANYARGS), VALUE arg ) {
	struct rbverse_session *session = rbverse_get_session( sessionobj );
	VALUE rval = Qnil;

	rbverse_log( "debug", "About to acquire the session mutex for session %d", session->id );
	rb_mutex_lock( rbverse_session_mutex );
	verse_session_set( session->id );
    rval = rb_ensure( func, arg, rb_mutex_unlock, rbverse_session_mutex );
	rbverse_log( "debug", "  done with the session mutex for session %d.", session->id );

	return rval;
}



/*
 * Create a Verse::Session from a VSession.
 */
VALUE
rbverse_verse_session_from_vsession( VSession id, VALUE address ) {
	VALUE session_obj = rb_class_new_instance( 1, &address, rbverse_cVerseSession );
	struct rbverse_session *session = check_session( session_obj );

	st_insert( session_table, (st_data_t)id, (st_data_t)session_obj );
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


/* Body of rbverse_verse_callback_update after GVL is given up. */
static VALUE
rbverse_verse_session_s_update_body( void *ptr ) {
	uint32 *microseconds = (uint32 *)ptr;
	DEBUGMSG( "  calling verse_callback_update( %d ).", *microseconds );
	verse_callback_update( *microseconds );
	return Qtrue;
}


/* 
 * Iterator for rbverse_verse_session_update 
 */
static int
rbverse_verse_session_s_update_i( VSession id, VALUE session, st_data_t timeout ) {
	rbverse_log( "debug", "Callback update for session %p (timeout=%u µs).", id, timeout );

	verse_session_set( id );
	rb_thread_blocking_region( rbverse_verse_session_s_update_body, (uint32 *)&timeout,
		RUBY_UBF_IO, NULL );

	rbverse_log( "debug", "  after the blocking region, there are %d client sessions",
	             session_table->num_entries );

	return ST_CONTINUE;
}


/*
 * call-seq:
 *     Verse::Session.update( timeout=0.1 )
 * 
 * Reads any incoming packets from the network, parses them (splits
 * them into commands) and issues calls to any callbacks that are
 * registered for the found commands. It will block for at most
 * +timeout+ microseconds and wait for something to arrive.
 * 
 * An application must call this function periodically in order to
 * service the connection with the other end of the Verse link; failure
 * to do so will cause the other end's packet buffer to grow monotonically,
 * which in turn might cause the connection to be terminated.
 * 
 * Any registered callbacks can be called as a result of calling this
 * function if the corresponding Verse event has happened. Execution
 * of this function is the only time during which callbacks can be
 * called. This means that a client program can rely on the fact that
 * calling some other Verse API function, such as any command-sending
 * function, is guaranteed to not cause a callback to be invoked.
 * 
 * @param [Float] timeout  the maximum amount of time (in decimal seconds) to
 *                         block waiting for updates
 */
static VALUE
rbverse_verse_session_s_update( int argc, VALUE *argv, VALUE module ) {
	VALUE seconds = Qnil;
	uint32 microseconds, slice;

	if ( rb_scan_args(argc, argv, "01", &seconds) == 1 )
		microseconds = floor( NUM2DBL(seconds) * 1000000 );
	else
		microseconds = DEFAULT_UPDATE_TIMEOUT;

	slice = microseconds / ( session_table->num_entries + 1 );
	DEBUGMSG( "Update timeslice is %d µs", slice );

	DEBUGMSG( "  updating the global session" );
	verse_session_set( 0 );
	rb_thread_blocking_region( rbverse_verse_session_s_update_body, (uint32 *)&slice,
		RUBY_UBF_IO, NULL );

	if ( session_table->num_entries ) {
		DEBUGMSG( "  updating %d client sessions", session_table->num_entries );
		st_foreach( session_table, rbverse_verse_session_s_update_i, (st_data_t)slice );
	} else {
		DEBUGMSG( "  no client sessions to update" );
	}

	return Qtrue;
}


/*
 * Iterator body for rbverse_verse_session_s_all_connected
 */
static int
rbverse_verse_session_s_all_connected_i( VSession id, VALUE session, st_data_t data ) {
	VALUE *session_ary = (void *)data;
	rb_ary_push( *session_ary, session );
	return ST_CONTINUE;
}


/*
 * call-seq:
 *    Verse::Session.all_connected   -> array
 *
 * Return an Array of all Verse::Session instances that are connected (have a session ID).
 *
 * @return [Array<Verse::Session>]  all connected sessions
 */
static VALUE
rbverse_verse_session_s_all_connected( VALUE klass ) {
	VALUE sessions = rb_ary_new();

	st_foreach( session_table, rbverse_verse_session_s_all_connected_i, (st_data_t)&sessions );
	return sessions;
}


/* --------------------------------------------------------------
 * Instance methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::Session.new( address )                  -> session
 *
 * Create a new Verse::Session object that will use the given +address+. 
 * 
 * @param [String] address  the address to bind/connect to
 */
static VALUE
rbverse_verse_session_initialize( int argc, VALUE *argv, VALUE self ) {
	if ( !check_session(self) ) {
		struct rbverse_session *session;
		VALUE address = Qnil;

		DATA_PTR( self ) = session = rbverse_session_alloc();

		if ( rb_scan_args(argc, argv, "01", &address) ) {
			SafeStringValue( address );
			session->address = address;
		}

		/* Creation callbacks have a queue per node class, so create them now
		 * to avoid the race condition in on-demand creation. */
		session->create_callbacks = rb_hash_new();
		rb_hash_aset( session->create_callbacks, rbverse_cVerseAudioNode,    rb_ary_new() );
		rb_hash_aset( session->create_callbacks, rbverse_cVerseBitmapNode,   rb_ary_new() );
		rb_hash_aset( session->create_callbacks, rbverse_cVerseCurveNode,    rb_ary_new() );
		rb_hash_aset( session->create_callbacks, rbverse_cVerseGeometryNode, rb_ary_new() );
		rb_hash_aset( session->create_callbacks, rbverse_cVerseMaterialNode, rb_ary_new() );
		rb_hash_aset( session->create_callbacks, rbverse_cVerseObjectNode,   rb_ary_new() );
		rb_hash_aset( session->create_callbacks, rbverse_cVerseTextNode,     rb_ary_new() );

		/* The destruction callbacks are keyed by the node requesting destruction,
		 * so it can be just a simple hash */
		session->destroy_callbacks = rb_hash_new();

		rb_call_super( 0, NULL );
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
	struct rbverse_session *session = rbverse_get_session( self );
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
	struct rbverse_session *session = rbverse_get_session( self );

	SafeStringValue( address );
	session->address = rb_obj_clone( address );

	return Qtrue;
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
	struct rbverse_session *session = rbverse_get_session( self );
	return ( session->id ? Qtrue : Qfalse );
}


/*
 * Do the connection once the session mutex is acquired.
 */
static VALUE
rbverse_verse_session_connect_body( VALUE args ) {
	VALUE self, name, pass, expected_host_id;
	struct rbverse_session *session;
	VSession session_id;
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
	session_id = verse_send_connect( name_str, pass_str, addr_str, hostid );
	rbverse_log_with_context( self, "debug", "  session: %p", session_id );

	if ( !session_id )
		rb_raise( rbverse_eVerseConnectError, "Couldn't create connection to '%s'.", addr_str );

	/* Add the instance to the session table, keyed by its VSession */
	st_insert( session_table, (st_data_t)session_id, (st_data_t)self );

	session->id = session_id;

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
	struct rbverse_session *session = rbverse_get_session( self );
	VALUE args, name, pass, expected_host_id = Qnil;
	VALUE rval = Qnil;

	if ( !RTEST(session->address) )
		rb_raise( rbverse_eVerseSessionError, "No address set." );
	if ( session->id )
		rb_raise( rbverse_eVerseSessionError, "Session already established." );

	rb_scan_args( argc, argv, "21", &name, &pass, &expected_host_id );

	rbverse_log_with_context( self, "debug", "Acquiring session lock for new session." );
	args = rb_ary_new3( 4, self, name, pass, expected_host_id );
	rval = rbverse_with_session_lock( self, rbverse_verse_session_connect_body, args );
	rbverse_log_with_context( self, "debug", "  releasing session lock." );

	return rval;
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
	struct rbverse_session *session = rbverse_get_session( self );
	const char *msg;

	SafeStringValue( message );
	msg = RSTRING_PTR( message );

	verse_send_connect_terminate( RSTRING_PTR(session->address), msg );

	rbverse_log( "debug", "Removing terminated session %p from session table (%p, %d entries)",
	             session->id, session_table, session_table->num_entries );
	st_delete_safe( session_table, (st_data_t*)&session->id, 0, Qundef );
	st_cleanup_safe( session_table, Qundef );

	return Qtrue;
}


/* 
 * The part of Verse::Session#node_index_subscribe called inside the
 * session lock.
 */
static VALUE
rbverse_verse_session_subscribe_to_node_index_l( VALUE typemask ) {
	verse_send_node_index_subscribe( (uint32)typemask );
	return Qtrue;
}


/*
 * call-seq:
 *    session.subscribe_to_node_index( *node_classes )
 *
 * Subscribe to creation and destruction events for the specified +node_classes+. Node 
 * creation will be sent to the Verse::SessionObservers via their #on_node_create method,
 * and when nodes are deleted, #on_node_destroy is called.
 *
 * @param [Array<Class>] node_classes  one or more subclasses of Verse::Node that indicate
 *                                     which types of nodes to subscribe to. If you include
 *                                     Verse::Node itself, all node types will be subscribed
 *                                     to. Calling this method with no classes unsubscribes
 *                                     from all node types.
 */
static VALUE
rbverse_verse_session_subscribe_to_node_index( int argc, VALUE *argv, VALUE self ) {
	struct rbverse_session *session = rbverse_get_session( self );
	VALUE node_classes = Qnil;
	uint32 typemask = ~0;
	int i;

	if ( !session->id )
		rb_raise( rbverse_eVerseSessionError, "can't subscribe an unconnected session" );

	if ( rb_scan_args(argc, argv, "0*", &node_classes) ) {
		for ( i = 0; i < RARRAY_LEN(node_classes); i++ ) {
			VALUE typenum = rb_const_get( RARRAY_PTR(node_classes)[i], rb_intern("TYPE_NUMBER") );
			typemask |= (1 << FIX2INT( typenum ));
		}
	}

	rbverse_with_session_lock( self, rbverse_verse_session_subscribe_to_node_index_l,
	                           (VALUE)typemask );

	return INT2FIX( typemask );
}


/*
 * Synchronized portion of rbverse_verse_session_create_node() 
 */
static VALUE
rbverse_verse_session_create_node_l( VALUE nodeclass ) {
	const VNodeType nodetype = FIX2UINT( rb_const_get(nodeclass, rb_intern( "TYPE_NUMBER" )) );
	verse_send_node_create( ~0, nodetype, 0 );
	return Qtrue;
}

/*
 * call-seq:
 *    session.create_node( nodeclass ) {|node| ... }
 *
 * Ask the server to create a new node of the specified +nodeclass+. If the node is successfully 
 * created, the provided block will be called with the new node object. Note that this happens
 * asynchronously, so you shouldn't count on the callback being run when this method returns.
 * 
 * @yield [node]  called when the node is created with the node object
 * @example Creating a new ObjectNode
 *     objectnode = nil
 *     session.create_node( Verse::ObjectNode ) {|node| objectnode = node }
 *     Verse::Session.update until objectnode
 *   
 */
static VALUE
rbverse_verse_session_create_node( int argc, VALUE *argv, VALUE self ) {
	struct rbverse_session *session = rbverse_get_session( self );
	VALUE nodeclass, callback, callback_queue;

	if ( !session->id )
		rb_raise( rbverse_eVerseSessionError, "can't create a node via an unconnected session" );

	rb_scan_args( argc, argv, "1&", &nodeclass, &callback );
	callback_queue = rb_hash_aref( session->create_callbacks, nodeclass );

	Check_Type( nodeclass, T_CLASS );

	if ( !RTEST(callback) )
		callback = rb_block_proc();
	if ( !RTEST(callback_queue) )
		rb_raise( rbverse_eVerseSessionError,
		          "don't know how to create %s objects (no callback queue)", 
		          rb_class2name(nodeclass) );

	/* Add the callback to the queue. This isn't done inside the lock as
	 * we don't really care if the nodes are created strictly in the order
	 * in which they're created. */
	rb_ary_push( callback_queue, callback );
	return rbverse_with_session_lock( self, rbverse_verse_session_create_node_l, nodeclass );
}



/* Synchronized portion of rbverse_verse_session_destroy_node() */
static VALUE
rbverse_verse_session_destroy_node_l( VALUE node ) {
	VNodeID node_id = (VNodeID)node;
	verse_send_node_destroy( node_id );
	return Qtrue;
}


/*
 * call-seq:
 *    session.destroy_node( node ) {|node| ... }
 *
 * Ask the server to destroy the specified +node+. If the node is successfully destroyed,
 * the provided block is called with the now-destroyed node and the session's observers 
 * are notified via Verse::SessionObserver#on_node_destroy.
 * 
 * @yield [node]  called when the node is destroyed.
 * 
 * @example Remove the node from the list of active objects once it's destroyed
 *     active_nodes = [ objectnode ]
 *     session.destroy_node( objectnode ) {|node| active_nodes.delete(node) }
 *     Verse::Session.update while active_nodes.include?( objectnode )
 */
static VALUE
rbverse_verse_session_destroy_node( int argc, VALUE *argv, VALUE self ) {
	struct rbverse_session *session = rbverse_get_session( self );
	VALUE nodeobj, callback;
	struct rbverse_node *node;

	if ( !session->id )
		rb_raise( rbverse_eVerseSessionError,
		          "can't destroy a node via an unconnected session" );

	/* Unwrap the arguments and check the node object for sanity */
	rb_scan_args( argc, argv, "1&", &nodeobj, &callback );
	node = rbverse_get_node( nodeobj );
	rbverse_ensure_node_is_alive( node );

	/* Use a no-op destruction callback if none was specified. */
	if ( !RTEST(callback) ) callback = rb_block_proc();

	/* Add the destruction callback for the node */
	rb_hash_aset( session->destroy_callbacks, nodeobj, callback );

	/* Cast: VNodeID -> VALUE */
	return rbverse_with_session_lock( self, rbverse_verse_session_destroy_node_l, (VALUE)node->id );
}


/* --------------------------------------------------------------
 * Callbacks
 * -------------------------------------------------------------- */

/*
 * Iterator body for connect_accept observers.
 */
static VALUE
rbverse_cb_session_connect_accept_i( VALUE observer, VALUE cb_args ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseSessionObserver) )
		return Qnil;

	rbverse_log( "debug", "Connect_accept callback: notifying observer: %s.",
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_connect_accept"), 
	                    RARRAY_LEN(cb_args), RARRAY_PTR(cb_args) );
}


/*
 * Ruby handler for the 'connect_accept' message; called after re-establishing the GVL
 * from rbverse_cb_session_connect_accept().
 */
static void *
rbverse_cb_session_connect_accept_body( void *ptr ) {
	struct rbverse_connect_accept_event *event = (struct rbverse_connect_accept_event *)ptr;
	const VALUE cb_args = rb_ary_new2( 3 );
	VALUE session = rbverse_get_current_session();
	VALUE observers = rb_funcall( session, rb_intern("observers"), 0 );

	RARRAY_PTR(cb_args)[0] = INT2FIX( event->avatar );
	RARRAY_PTR(cb_args)[1] = rb_str_new2( event->address );
	RARRAY_PTR(cb_args)[2] = rbverse_host_id2str( event->hostid );

	rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_session_connect_accept_i, cb_args );

	return NULL;
}


/*
 * Verse callback for the 'connect_accept' message
 */
static void
rbverse_cb_session_connect_accept( void *unused, VNodeID avatar, const char *address, uint8 *host_id ) {
	struct rbverse_connect_accept_event event;

	event.avatar  = avatar;
	event.address = address;
	event.hostid  = host_id;

	rb_thread_call_with_gvl( rbverse_cb_session_connect_accept_body, (void *)&event );
}


/*
 * Iterator body for connect_terminate observers.
 */
static VALUE
rbverse_cb_session_connect_terminate_i( VALUE observer, VALUE cb_args ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseSessionObserver) )
		return Qnil;

	rbverse_log( "debug", "Connect_terminate callback: notifying observer: %s.",
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_connect_terminate"),
	                    RARRAY_LEN(cb_args), RARRAY_PTR(cb_args) );
}


/*
 * Call the connect_terminate handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_session_connect_terminate_body( void *ptr ) {
	const char **args = (const char **)ptr;
	const VALUE cb_args = rb_ary_new2( 2 );
	VALUE session, observers;

	session = rbverse_get_current_session();
	observers = rb_funcall( session, rb_intern("observers"), 0 );

	RARRAY_PTR(cb_args)[0] = rb_str_new2( args[0] );
	RARRAY_PTR(cb_args)[1] = rb_str_new2( args[1] );

	rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_session_connect_terminate_i, cb_args );

	return NULL;
}


/*
 * Callback for the 'connect_terminate' command.
 */
static void
rbverse_cb_session_connect_terminate( void *unused, const char *address, const char *msg ) {
	const char *(args[2]) = { address, msg };
	DEBUGMSG( " Acquiring GVL for 'connect_terminate' event.\n" );
	fflush( stdout );
	rb_thread_call_with_gvl( rbverse_cb_session_connect_terminate_body, args );
}


/*
 * Iterator body for node_create observers.
 */
static VALUE
rbverse_cb_session_node_create_i( VALUE observer, VALUE node ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseSessionObserver) )
		return Qnil;

	rbverse_log( "debug", "Node_create callback: notifying observer: %s.",
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_node_create"), 1, &node );
}


/*
 * Call the node_create handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_session_node_create_body( void *ptr ) {
	struct rbverse_node_create_event *event = (struct rbverse_node_create_event *)ptr;
	VALUE self, node, cb_queue, callback, observers;
	struct rbverse_session *session = NULL;

	node      = rbverse_wrap_verse_node( event->node_id, event->type, event->owner );
	self      = rbverse_get_current_session();
	session   = rbverse_get_session( self );
	cb_queue  = rb_hash_aref( session->create_callbacks, rb_class_of(node) );
	observers = rb_funcall( self, rb_intern("observers"), 0 );

	/* Set the node's session */
	rb_funcall3( node, rb_intern("session="), 1, &self );

	/* If this session was the node's creator, and there's a creation
	 * callback queue for the class of node that was created, and there's a callback in
	 * the queue, shift it off and call it with the node object. */
	if ( event->owner == VN_OWNER_MINE &&
	     RTEST(cb_queue) &&
	     RTEST(callback = rb_ary_shift(cb_queue)) )
	{
		rbverse_log_with_context( self, "debug", "calling create callback %p for node %p",
		                          RSTRING_PTR(rb_inspect( callback )),
		                          RSTRING_PTR(rb_inspect( node )) );
		rb_funcall3( callback, rb_intern("call"), 1, &node );
	} else {
		rbverse_log_with_context( self, "debug", "no creation callback for node %p",
		                          RSTRING_PTR(rb_inspect( node )) );
	}

	/* Now notify the session's observers that a node was created */
	rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_session_node_create_i, node );

	return NULL;
}

/*
 * Callback for the 'node_create' command.
 */
static void
rbverse_cb_session_node_create( void *unused, VNodeID node_id, VNodeType type, VNodeOwner owner ) {
	struct rbverse_node_create_event event;

	event.node_id = node_id;
	event.type    = type;
	event.owner   = owner;

	DEBUGMSG( " Acquiring GVL for 'node_create' event.\n" );
	fflush( stdout );

	rb_thread_call_with_gvl( rbverse_cb_session_node_create_body, (void *)&event );
}



/*
 * Iterator body for node_destroy observers.
 */
static VALUE
rbverse_cb_session_node_destroy_i( VALUE observer, VALUE node ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseSessionObserver) )
		return Qnil;

	rbverse_log( "debug", "Node_destroy callback: notifying observer: %s.",
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_node_destroy"), 1, &node );
}


/*
 * Call the node_destroy handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_session_node_destroy_body( void *ptr ) {
	VNodeID *node_id = (VNodeID *)ptr;
	VALUE node;

	if ( RTEST(node = rbverse_lookup_verse_node( *node_id )) ) {
		VALUE session = rb_funcall( node, rb_intern("session"), 0 );
		VALUE observers = rb_funcall( session, rb_intern("observers"), 0 );
		rbverse_mark_node_destroyed( node );
		rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_session_node_destroy_i, node );
	} else {
		rbverse_log( "info", "destroy event received for unwrapped node %x", node_id );
	}

	return NULL;
}

/*
 * Callback for the 'node_destroy' command.
 */
static void
rbverse_cb_session_node_destroy( void *unused, VNodeID node_id ) {
	DEBUGMSG( " Acquiring GVL for 'node_destroy' event.\n" );
	fflush( stdout );

	rb_thread_call_with_gvl( rbverse_cb_session_node_destroy_body, (void *)&node_id );
}



/*
 * Verse::Session class
 */
void
rbverse_init_verse_session( void ) {
	rbverse_log( "debug", "Initializing Verse::Session" );

	session_table = st_init_numtable();
	rbverse_session_mutex = rb_mutex_new();

#ifdef FOR_RDOC
	rbverse_mVerse = rb_define_module( "Verse" );
#endif

	/* Related modules */
	rbverse_mVerseSessionObserver = rb_define_module_under( rbverse_mVerse, "SessionObserver" );

	/* Class methods */
	rbverse_cVerseSession = rb_define_class_under( rbverse_mVerse, "Session", rb_cObject );
	rb_include_module( rbverse_cVerseSession, rbverse_mVerseLoggable );
	rb_include_module( rbverse_cVerseSession, rbverse_mVerseObservable );

	rb_define_alloc_func( rbverse_cVerseSession, rbverse_verse_session_s_allocate );

	rb_define_singleton_method( rbverse_cVerseSession, "update", rbverse_verse_session_s_update,
	                            -1 );
	rb_define_alias( rb_singleton_class(rbverse_cVerseSession), "callback_update", "update" );

	rb_attr( rb_singleton_class(rbverse_cVerseSession), rb_intern("mutex"), Qtrue, Qfalse, Qtrue );
	rb_iv_set( rbverse_cVerseSession, "@mutex", rbverse_session_mutex );
	rb_define_singleton_method( rbverse_cVerseSession, "all_connected",
	                            rbverse_verse_session_s_all_connected, 0 );

	/* Initializer */
	rb_define_method( rbverse_cVerseSession, "initialize", rbverse_verse_session_initialize, -1 );

	/* Public instance methods */
	rb_define_method( rbverse_cVerseSession, "address", rbverse_verse_session_address, 0 );
	rb_define_method( rbverse_cVerseSession, "address=", rbverse_verse_session_address_eq, 1 );

	rb_define_method( rbverse_cVerseSession, "connected?", rbverse_verse_session_connected_p, 0 );

	rb_define_method( rbverse_cVerseSession, "connect", rbverse_verse_session_connect, -1 );
	rb_define_method( rbverse_cVerseSession, "terminate", rbverse_verse_session_terminate, 1 );

	rb_define_method( rbverse_cVerseSession, "subscribe_to_node_index",
	                  rbverse_verse_session_subscribe_to_node_index, -1 );
	rb_define_method( rbverse_cVerseSession, "create_node", rbverse_verse_session_create_node, -1 );
	rb_define_method( rbverse_cVerseSession, "destroy_node", rbverse_verse_session_destroy_node,
	                  -1 );

	// tag_group_create(VNodeID node_id, uint16 group_id, const char *name);
	// tag_group_destroy(VNodeID node_id, uint16 group_id);
	// tag_group_subscribe(VNodeID node_id, uint16 group_id);
	// tag_group_unsubscribe(VNodeID node_id, uint16 group_id);
	// tag_create(VNodeID node_id, uint16 group_id, uint16 tag_id, const char *name, VNTagType type, 
	//     const VNTag *tag);
	// tag_destroy(VNodeID node_id, uint16 group_id, uint16 tag_id);

	// node_name_set(VNodeID node_id, const char *name);

	verse_callback_set( verse_send_connect_accept, rbverse_cb_session_connect_accept, NULL );
	verse_callback_set( verse_send_connect_terminate, rbverse_cb_session_connect_terminate, NULL );
	verse_callback_set( verse_send_node_create, rbverse_cb_session_node_create, NULL );
	verse_callback_set( verse_send_node_destroy, rbverse_cb_session_node_destroy, NULL );
}

