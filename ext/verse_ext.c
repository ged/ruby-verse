/* 
 * Verse -- Ruby binding for the Verse network protocol
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

VALUE rbverse_mVerse;
VALUE rbverse_mVerseConstants;

VALUE rbverse_mVerseVersioned;
VALUE rbverse_mVerseObserver;
VALUE rbverse_mVersePingObserver;
VALUE rbverse_mVerseConnectionObserver;
VALUE rbverse_mVerseObservable;

VALUE rbverse_eVerseConnectError;
VALUE rbverse_eVerseSessionError;



/*
 * Log a message to the given +context+ object's logger.
 */
void
#ifdef HAVE_STDARG_PROTOTYPES
rbverse_log_with_context( VALUE context, const char *level, const char *fmt, ... ) 
#else
rbverse_log_with_context( VALUE context, const char *level, const char *fmt, va_dcl ) 
#endif
{
	char buf[BUFSIZ];
	va_list	args;
	VALUE logger = Qnil;
	VALUE message = Qnil;

	va_start( args, fmt );
	vsnprintf( buf, BUFSIZ, fmt, args );
	message = rb_str_new2( buf );

	logger = rb_funcall( context, rb_intern("log"), 0, 0 );
	rb_funcall( logger, rb_intern(level), 1, message );

	va_end( args );
}


/* 
 * Log a message to the global logger.
 */
void
#ifdef HAVE_STDARG_PROTOTYPES
rbverse_log( const char *level, const char *fmt, ... ) 
#else
rbverse_log( const char *level, const char *fmt, va_dcl ) 
#endif
{
	char buf[BUFSIZ];
	va_list	args;
	VALUE logger = Qnil;
	VALUE message = Qnil;

	va_init_list( args, fmt );
	vsnprintf( buf, BUFSIZ, fmt, args );
	message = rb_str_new2( buf );

	logger = rb_funcall( rbverse_mVerse, rb_intern("logger"), 0, 0 );
	rb_funcall( logger, rb_intern(level), 1, message );

	va_end( args );
}


/*
 * Validate and convert the contents of a String to a Verse host id
 */
inline uint8 *
rbverse_str2host_id( VALUE idstring ) {
	if ( !ENCODING_IS_ASCII8BIT(idstring) )
		rb_raise( rb_eArgError, "invalid encoding: expected ASCII-8BIT data" );

	if ( RSTRING_LEN(idstring) > V_HOST_ID_SIZE ) {
		rb_raise( rb_eArgError,
		          "hostid is too long: should be %d bytes, got %ld", 
		          V_HOST_ID_SIZE, RSTRING_LEN(idstring) );
	} else if ( RSTRING_LEN(idstring) < V_HOST_ID_SIZE ) {
		rb_raise( rb_eArgError,
		          "hostid is too short: should be %d bytes, got %ld", 
		          V_HOST_ID_SIZE, RSTRING_LEN(idstring) );
	}

	return (uint8 *)RSTRING_PTR(idstring);
}


/*
 * Convert a Verse host id to a String with ASCII8BIT encoding.
 */
inline VALUE
rbverse_host_id2str( const uint8 *hostid ) {
	VALUE idstring = rb_str_new( (const char *)hostid, V_HOST_ID_SIZE );

	rb_enc_associate( idstring, rb_ascii8bit_encoding() );
	return idstring;
}


/*
 * Call rb_sys_fail() with the message in +ptr+ after acquiring the GVL.
 */
void *
rbverse_sysfail( void *ptr ) {
	rb_sys_fail( ptr );
}



/* -----------------------------------
 * Class methods
 * ----------------------------------- */

/*
 * call-seq:
 *    Verse.port = fixnum
 *    Verse.connect_port = fixnum
 *
 * Set the port Verse will bind to for incoming events. Defaults to 
 * Verse::Constants::DEFAULT_PORT.
 *
 */
static VALUE
rbverse_verse_port_eq( VALUE self, VALUE newport ) {
	const unsigned int port = NUM2UINT( newport );

	rbverse_log( "info", "Setting Verse port to %u", port );
	verse_set_port( port );

	return Qtrue;
}


/*
 * call-seq:
 *     Verse.ping( address, message )
 *
 * General-purpose unconnected "ping" command.
 *  
 * @param [String] address  The address of the server to send the ping to.
 * @param [String] message  The content of the ping, limited to a maximum of
 *                          1399 bytes.
 *
 * @example
 *    Verse.ping( 'verse.acme.com', 'MS:ANNOUNCE' )
 *
 */
static VALUE
rbverse_verse_ping( VALUE module, VALUE address, VALUE message ) {
	const char *addr = RSTRING_PTR( address );
	const char *msg  = RSTRING_PTR( message );

	rbverse_log_with_context( module, "debug", "Pinging '%s' with message '%s'", addr, msg );
	verse_send_ping( addr, msg );

	return Qtrue;
}


/*
 * call-seq:
 *    Verse.connect_accept( avatar, address, host_id )
 *
 * Indicate acceptance of a client's connection request from +address+, assigning it the 
 * given +avatar+ and +host_id+.
 *
 * @return [Verse::Session]  the session that contains the new connection
 * @todo Convert the 'avatar' parameter to a Verse::Node once that is defined.
 */
static VALUE
rbverse_verse_connect_accept( VALUE module, VALUE avatar, VALUE address, VALUE host_id ) {
	VNodeID av       = NUM2ULONG( avatar );
	const char *addr = NULL;
	uint8 *id        = rbverse_str2host_id( host_id );
	VSession session_id;
	VALUE session = Qnil;

	SafeStringValue( address );
	addr = RSTRING_PTR( address );

	session_id = verse_send_connect_accept( av, addr, id );
	session = rbverse_verse_session_from_vsession( session_id, address );

	return session;
}


/* Body of rbverse_verse_callback_update after GVL is given up. */
static VALUE
rbverse_verse_update_body( void *ptr ) {
	uint32 *microseconds = (uint32 *)ptr;

	verse_callback_update( *microseconds );

	return Qtrue;
}


/*
 * call-seq:
 *     Verse.update( timeout=0.1 )
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
rbverse_verse_update( VALUE module, VALUE timeout ) {
	double seconds = NUM2DBL( timeout );
	uint32 microseconds = floor( seconds * 1000000 );

	rbverse_log( "debug", "Callback update for session %p (timeout=%u µs).",
	             verse_session_get(), microseconds );
	rb_thread_blocking_region( rbverse_verse_update_body, &microseconds,
		RUBY_UBF_IO, NULL );

	return Qtrue;
}


/*
 *  call-seq:
 *     Verse.create_host_id   -> string
 *
 *  Create an ID for the verse host that clients can later use to verify
 *  which server they're connecting to.
 *
 */
static VALUE
rbverse_verse_create_host_id( VALUE module ) {
	uint8 id[V_HOST_ID_SIZE];
	verse_host_id_create( id );
	return rb_str_new( (const char *)id, V_HOST_ID_SIZE );
}


/*
 *  call-seq:
 *     Verse.host_id = string
 *
 *  Set the server's host id.
 *
 */
static VALUE
rbverse_verse_host_id_eq( VALUE module, VALUE id ) {
	VALUE idstring = rb_str_to_str( id );
	verse_host_id_set( rbverse_str2host_id(idstring) );
	return idstring;
}


/* --------------------------------------------------------------
 * Observable Support
 * -------------------------------------------------------------- */

/*
 * Iterator for ping callback.
 */
static VALUE
rbverse_cb_ping_i( VALUE observer, VALUE cb_args ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVersePingObserver) )
		return Qnil;

	rbverse_log( "debug", "Ping callback: notifying observer: %s.", 
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_ping"), 2, RARRAY_PTR(cb_args) );
}


/*
 * Call the ping handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_ping_body( void *ptr ) {
	const char **args = (const char **)ptr;
	const VALUE cb_args = rb_ary_new2( 2 );
	const VALUE observers = rb_iv_get( rbverse_mVerse, "@observers" );

	RARRAY_PTR(cb_args)[0] = rb_str_new2( args[0] );
	RARRAY_PTR(cb_args)[1] = rb_str_new2( args[1] );

	rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_ping_i, cb_args );

	return NULL;
}


/*
 * Callback for the 'ping' command.
 */
static void
rbverse_cb_ping( void *unused, const char *addr, const char *msg ) {
	const char *(args[2]) = { addr, msg };
	rb_thread_call_with_gvl( rbverse_cb_ping_body, args );
}


/*
 * Iterator for connect callback.
 */
static VALUE
rbverse_cb_connect_i( VALUE observer, VALUE cb_args ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseConnectionObserver) )
		return Qnil;

	rbverse_log( "debug", "Connect callback: notifying observer: %s.", 
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_connect"), 4, RARRAY_PTR(cb_args) );
}


/*
 * Call the connect handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_connect_body( void *ptr ) {
	const char **args = (const char **)ptr;
	const VALUE cb_args = rb_ary_new2( 4 );
	const VALUE observers = rb_iv_get( rbverse_mVerse, "@observers" );

	RARRAY_PTR(cb_args)[0] = rb_str_new2( args[0] );
	RARRAY_PTR(cb_args)[1] = rb_str_new2( args[1] );
	RARRAY_PTR(cb_args)[2] = rb_str_new2( args[2] );
	RARRAY_PTR(cb_args)[3] = rbverse_host_id2str( (const uint8 *)args[3] );

	rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_connect_i, cb_args );

	return NULL;
}

/*
 * Callback for the 'connect' command.
 */
static void
rbverse_cb_connect( void *unused, const char *name, const char *pass, const char *address,
                        const uint8 *expected_host_id )
{
	const char *(args[4]) = { name, pass, address, (const char *)expected_host_id };
	printf( " Acquiring GVL for 'connect' event.\n" );
	fflush( stdout );
	rb_thread_call_with_gvl( rbverse_cb_connect_body, args );
}


/*
 * Iterator for connect_terminate callback.
 */
static VALUE
rbverse_cb_connect_terminate_i( VALUE observer, VALUE cb_args ) {
	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseConnectionObserver) )
		return Qnil;

	rbverse_log( "debug", "Connect_terminate callback: notifying observer: %s.", 
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_connect_terminate"), 4, RARRAY_PTR(cb_args) );
}


/*
 * Call the connect_terminate handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_connect_terminate_body( void *ptr ) {
	const char **args = (const char **)ptr;
	const VALUE cb_args = rb_ary_new2( 2 );
	const VALUE observers = rb_iv_get( rbverse_mVerse, "@observers" );

	RARRAY_PTR(cb_args)[0] = rb_str_new2( args[0] );
	RARRAY_PTR(cb_args)[1] = rb_str_new2( args[1] );

	rb_block_call( observers, rb_intern("each"), 0, 0, rbverse_cb_connect_terminate_i, cb_args );

	return NULL;
}

/*
 * Callback for the 'connect_terminate' command.
 */
static void
rbverse_cb_connect_terminate( void *unused, const char *address, const char *msg ) {
	const char *(args[2]) = { address, msg };
	printf( " Acquiring GVL for 'connect_terminate' event.\n" );
	fflush( stdout );
	rb_thread_call_with_gvl( rbverse_cb_connect_terminate_body, args );
}


/*
 * call-seq:
 *    Verse.add_observer( observer )
 *
 * @see Verse::Observable#add_observer
 *
 */
static VALUE
rbverse_verse_add_observer( VALUE module, VALUE observer ) {
	rbverse_log( "debug", "Adding observer %s.", RSTRING_PTR(rb_inspect( observer )) );

	/* Register callbacks when an observer is added; I don't think re-registering
	   them on observers after the first has any negative consequences...
	 */
	verse_callback_set( verse_send_ping, rbverse_cb_ping, NULL );
	verse_callback_set( verse_send_connect, rbverse_cb_connect, NULL );

	return rb_call_super( 1, &observer );
}


/*
 * call-seq:
 *    Verse.remove_observer( observer )
 *
 * @see Verse::Observable#remove_observer
 *
 */
static VALUE
rbverse_verse_remove_observer( VALUE module, VALUE observer ) {
	VALUE rval = Qnil;

	rbverse_log( "debug", "Removing observer %s.", RSTRING_PTR(rb_inspect( observer )) );
	rval = rb_call_super( 1, &observer );

	/* Unregister callbacks if the last observer has been removed */
	if ( !RARRAY_LEN(rb_iv_get( rbverse_mVerse, "@observers" )) ) {
		verse_callback_set( verse_send_ping, NULL, NULL );
		verse_callback_set( verse_send_connect, NULL, NULL );
	}

	return rval;
}



/*
 * Verse namespace.
 * 
 */
void
Init_verse_ext( void ) {
	rb_require( "verse" );

	rbverse_mVerse = rb_define_module( "Verse" );
	rbverse_mVerseVersioned = rb_define_module_under( rbverse_mVerse, "Versioned" );
	rbverse_mVerseObserver = rb_define_module_under( rbverse_mVerse, "Observer" );
	rbverse_mVersePingObserver = rb_define_module_under( rbverse_mVerse, "PingObserver" );
	rbverse_mVerseConnectionObserver = rb_define_module_under( rbverse_mVerse, "ConnectionObserver" );
	rbverse_mVerseObservable = rb_define_module_under( rbverse_mVerse, "Observable" );

	rbverse_eVerseConnectError =
		rb_define_class_under( rbverse_mVerse, "ConnectError", rb_eRuntimeError );
	rbverse_eVerseSessionError =
		rb_define_class_under( rbverse_mVerse, "SessionError", rb_eRuntimeError );

	/* version constants */
	rb_define_const( rbverse_mVerse, "RELEASE_NUMBER", INT2FIX(V_RELEASE_NUMBER) );
	rb_define_const( rbverse_mVerse, "RELEASE_PATCH",  INT2FIX(V_RELEASE_PATCH) );
	rb_define_const( rbverse_mVerse, "RELEASE_LABEL",  rb_str_new2(V_RELEASE_LABEL) );

	/* Module methods */
	rb_define_singleton_method( rbverse_mVerse, "port=", rbverse_verse_port_eq, 1 );
	rb_define_alias( rb_singleton_class(rbverse_mVerse), "connect_port=", "port=" );
	rb_define_singleton_method( rbverse_mVerse, "create_host_id", rbverse_verse_create_host_id, 0 );
	rb_define_singleton_method( rbverse_mVerse, "host_id=", rbverse_verse_host_id_eq, 1 );
	rb_define_singleton_method( rbverse_mVerse, "connect_accept", rbverse_verse_connect_accept, 3 );
	rb_define_singleton_method( rbverse_mVerse, "ping", rbverse_verse_ping, 2 );
	rb_define_singleton_method( rbverse_mVerse, "update", rbverse_verse_update, 1 );
	rb_define_alias( rb_singleton_class(rbverse_mVerse), "callback_update", "update" );
	rb_define_singleton_method( rbverse_mVerse, "add_observer", rbverse_verse_add_observer, 1 );
	rb_define_singleton_method( rbverse_mVerse, "remove_observer", rbverse_verse_remove_observer, 1 );

	/*
	 * Constants
	 */
	rbverse_mVerseConstants = rb_define_module_under( rbverse_mVerse, "Constants" );

	rb_define_const( rbverse_mVerseConstants, "HOST_ID_SIZE", INT2FIX(V_HOST_ID_SIZE) );


	/* Init the subordinate classes */
	rbverse_init_verse_session();
	rbverse_init_verse_node();

	rbverse_init_verse_mixins();

	rbverse_log( "debug", "Initialized the extension." );
}

