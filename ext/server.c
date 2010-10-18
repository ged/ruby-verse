/* 
 * Verse::Server -- Verse server class
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

VALUE rbverse_cVerseServer;
static VALUE rbverse_verse_running_server;

static void rbverse_server_cb_connect( void *, const char *, const char *, const char *, const uint8 * );
static void rbverse_server_cb_index_subscribe( void *, uint32 );


/* --------------------------------------------------------------
 * Class methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::Server.running_instance   -> server
 *
 * Return the instance of Verse::Server that is currently running.
 * 
 * @return [Verse::Server, nil]  the currently-running server, if any.
 * 
 */
static VALUE
rbverse_verse_server_s_running_instance( VALUE class ) {
	return rbverse_verse_running_server;
}


/* --------------------------------------------------------------
 * Instance methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::Server#run
 *
 * Set up the server instance to handle connections and index subscriptions.
 *
 */
static VALUE
rbverse_verse_server_run( VALUE self ) {
	if ( RTEST(rbverse_verse_running_server) )
		rb_raise( rbverse_eVerseServerError, "another server is already running" );

	rbverse_verse_running_server = self;

	rbverse_log_with_context( self, "info", "Starting up." );
	verse_callback_set( verse_send_connect, rbverse_server_cb_connect, NULL );
	verse_callback_set( verse_send_node_index_subscribe, rbverse_server_cb_index_subscribe,
	                    NULL );

	return Qtrue;
}


/*
 * call-seq:
 *    Verse::Server#running?		-> true or false
 *
 * @return [boolean]  true if the receiving server is the running one.
 *
 */
static VALUE
rbverse_verse_server_running_p( VALUE self ) {
	return ( rbverse_verse_running_server == self ? Qtrue : Qfalse );
}


/*
 * call-seq:
 *    Verse::Server#shutdown
 *
 * Tear down the server instance and stop handling connections and index subscriptions.
 *
 */
static VALUE
rbverse_verse_server_shutdown( VALUE self ) {
	if ( rbverse_verse_running_server != self )
		rb_raise( rbverse_eVerseServerError, "server isn't running" );

	rbverse_log_with_context( self, "info", "Shutting down." );
	verse_callback_set( verse_send_connect, NULL, NULL );
	verse_callback_set( verse_send_node_index_subscribe, NULL, NULL );

	rbverse_verse_running_server = Qnil;

	return Qtrue;
}


/*
 * call-seq:
 *    server.accept_connection( avatar, address, host_id )
 *
 * Indicate acceptance of a client's connection request from +address+, assigning it the 
 * given +avatar+ and +host_id+.
 * 
 * @param [Verse::ObjectNode] avatar   the node that will serve as the client's representation 
 *                                     in the host's world.
 * @param [String] address
 * @param [String] host_id             the server's host ID, such as that generated by
 *                                     Verse.create_host_id.
 * 
 * @return [Verse::Session]  the session that contains the new connection
 */
static VALUE
rbverse_verse_accept_connection( VALUE self, VALUE avatar, VALUE address, VALUE host_id ) {
	uint8 *id = rbverse_str2host_id( host_id );
	const char *addr = NULL;
	VNodeID avatar_node_id;
	VSession session_id;
	VALUE session = Qnil;

	if ( rbverse_verse_running_server != self )
		rb_raise( rbverse_eVerseServerError, "server isn't running" );

	SafeStringValue( address );
	addr = RSTRING_PTR( address );

	if ( !IsObjectNode(avatar) )
		rb_raise( rb_eTypeError, "can't convert %s into Verse::ObjectNode", rb_obj_classname(avatar) );
	avatar_node_id = (VNodeID)FIX2ULONG( rb_funcall(avatar, rb_intern("id"), 0) );

	rbverse_log( "debug", "Accepting connection from %s with avatar %lu", addr, avatar_node_id );
	session_id = verse_send_connect_accept( avatar_node_id, addr, id );
	session = rbverse_verse_session_from_vsession( session_id, address );

	return session;
}



/* --------------------------------------------------------------
 * Callbacks
 * -------------------------------------------------------------- */

/*
 * Call the connect handler after acquiring the GVL.
 */
static void *
rbverse_server_cb_connect_body( void *ptr ) {
	const char **args = (const char **)ptr;
	const VALUE cb_args = rb_ary_new2( 4 );

	if ( RTEST(rbverse_verse_running_server) ) {
		rb_ary_store( cb_args, 0, rb_str_new2(args[0]) );
		rb_ary_store( cb_args, 1, rb_str_new2(args[1]) );
		rb_ary_store( cb_args, 2, rb_str_new2(args[2]) );
		rb_ary_store( cb_args, 3, rbverse_host_id2str((const uint8 *)args[3]) );

		rb_funcall2( rbverse_verse_running_server, rb_intern("on_connect"),
		             RARRAY_LEN(cb_args), RARRAY_PTR(cb_args) );
	}

	else {
		rbverse_log( "warn", "on_connect event called with no server instance to handle it!" );
	}

	return NULL;
}

/*
 * Callback for the 'connect' command.
 */
static void
rbverse_server_cb_connect( void *unused, const char *name, const char *pass, const char *address,
                    const uint8 *expected_host_id )
{
	const char *(args[4]) = { name, pass, address, (const char *)expected_host_id };
	DEBUGMSG( "*** Acquiring GVL for 'connect' event. ***" );
	rb_thread_call_with_gvl( rbverse_server_cb_connect_body, args );
}


/*
 * Call the 'node_index_subscribe' handler after acquiring the GVL.
 */
static void *
rbverse_server_cb_index_subscribe_body( void *ptr ) {
	const uint32 mask = *((uint32 *)ptr);
	const VALUE session = rbverse_get_current_session();
	const VALUE cb_args = rb_ary_new3( 1, session );
	VALUE node_class = Qnil;
	VNodeType node_type;

	if ( RTEST(rbverse_verse_running_server) ) {
		rbverse_log( "debug", "Building the list of subscribed classes from mask: %u.", mask );
		for ( node_type = V_NT_OBJECT; node_type < V_NT_NUM_TYPES; node_type++ ) {
			if ( mask & (1 << node_type) ) {
				node_class = rbverse_node_class_from_node_type( node_type );
				rbverse_log( "debug", "  adding %s", rb_class2name(node_class) );
				rb_ary_push( cb_args, node_class );
			}
		}

		rbverse_log_with_context( rbverse_verse_running_server, "debug",
		                          "Calling on_node_index_subscribe with %d node classes.",
		                          RARRAY_LEN(cb_args) );
		rb_funcall( rbverse_verse_running_server, rb_intern("on_node_index_subscribe"),
		            RARRAY_LEN(cb_args), RARRAY_PTR(cb_args) );
	}

	else {
		rbverse_log( "warn", "on_node_index_subscribe event called with no server instance to handle it!" );
	}

	return NULL;
}


/*
 * Callback for the 'node_index_subscribe' command.
 */
static void
rbverse_server_cb_index_subscribe( void *unused, uint32 mask ) {
	rb_thread_call_with_gvl( rbverse_server_cb_index_subscribe_body, (void *)&mask );
}



/*
 * Verse::Server class
 */
void
rbverse_init_verse_server( void ) {
	rbverse_log( "debug", "Initializing Verse::Server" );
	rbverse_verse_running_server = Qnil;

#ifdef FOR_RDOC
	rbverse_mVerse = rb_define_module( "Verse" );
#endif

	rbverse_cVerseServer = rb_define_class_under( rbverse_mVerse, "Server", rb_cObject );

	/* Class methods */
	rb_define_singleton_method( rbverse_cVerseServer, "running_instance",
	                            rbverse_verse_server_s_running_instance, 0 );

	/* Public instance methods */
	rb_define_method( rbverse_cVerseServer, "run", rbverse_verse_server_run, 0 );
	rb_define_method( rbverse_cVerseServer, "running?", rbverse_verse_server_running_p, 0 );
	rb_define_method( rbverse_cVerseServer, "shutdown", rbverse_verse_server_shutdown, 0 );

	/* Protected instance methods */
	rb_define_protected_method( rbverse_cVerseServer, "accept_connection",
	                            rbverse_verse_accept_connection, 3 );

	rb_require( "verse/server.rb" );
}

