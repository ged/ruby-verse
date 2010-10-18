/* 
 * Verse -- Ruby binding for the Verse network protocol
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

VALUE rbverse_mVerse;
VALUE rbverse_mVerseConstants;

VALUE rbverse_mVerseLoggable;
VALUE rbverse_mVerseVersionUtilities;

VALUE rbverse_mVerseVersioned;
VALUE rbverse_mVerseObservable;
VALUE rbverse_mVerseObserver;
VALUE rbverse_mVersePingObserver;

VALUE rbverse_eVerseError;
VALUE rbverse_eVerseServerError;
VALUE rbverse_eVerseConnectError;
VALUE rbverse_eVerseSessionError;
VALUE rbverse_eVerseNodeError;


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


/* Body of rbverse_verse_update after GVL is given up. */
static VALUE
rbverse_verse_update_body( void *ptr ) {
	uint32 *microseconds = (uint32 *)ptr;
	DEBUGMSG( "  calling verse_callback_update( %d ).", *microseconds );
	verse_callback_update( *microseconds );
	return Qtrue;
}


/* 
 * Iterator for rbverse_verse_session_update 
 */
static int
rbverse_verse_update_i( VSession id, VALUE session, st_data_t timeout ) {
	DEBUGMSG( "Callback update for session %p (timeout=%u µs).", id, (uint32)timeout );

	verse_session_set( id );
	rb_thread_blocking_region( rbverse_verse_update_body, (uint32 *)&timeout,
		RUBY_UBF_IO, NULL );

	return ST_CONTINUE;
}


/*
 * call-seq:
 *     Verse.update( timeout=0.1 )
 * 
 * Reads any incoming packets from the network, parses them (splits
 * them into commands) and issues calls to any callbacks that are
 * registered for the found commands. It will block for at most
 * +timeout+ microseconds while waiting for something to arrive.
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
rbverse_verse_update( int argc, VALUE *argv, VALUE module ) {
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
	rb_thread_blocking_region( rbverse_verse_update_body, (uint32 *)&slice,
		RUBY_UBF_IO, NULL );

	if ( session_table->num_entries ) {
		DEBUGMSG( "  updating %lu client sessions", (long unsigned int)session_table->num_entries );
		st_foreach( session_table, rbverse_verse_update_i, (st_data_t)slice );
	} else {
		DEBUGMSG( "  no client sessions to update" );
	}

	return Qtrue;
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
	return rb_funcall2( observer, rb_intern("on_ping"), RARRAY_LEN(cb_args), RARRAY_PTR(cb_args) );
}


/*
 * Call the ping handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_ping_body( void *ptr ) {
	const char **args = (const char **)ptr;
	const VALUE cb_args = rb_ary_new();
	const VALUE observers = rb_iv_get( rbverse_mVerse, "@observers" );

	rb_ary_push( cb_args, rb_str_new2(args[0]) );
	rb_ary_push( cb_args, rb_str_new2(args[1]) );

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
 * Verse namespace.
 * 
 */
void
Init_verse_ext( void ) {
	rb_require( "verse" );

	rbverse_mVerse = rb_define_module( "Verse" );

	rbverse_mVerseLoggable = rb_define_module_under( rbverse_mVerse, "Loggable" );
	rbverse_mVerseVersionUtilities = rb_define_module_under( rbverse_mVerse, "VersionUtilities" );

	rbverse_mVerseVersioned = rb_define_module_under( rbverse_mVerse, "Versioned" );
	rbverse_mVerseObserver = rb_define_module_under( rbverse_mVerse, "Observer" );
	rbverse_mVerseObservable = rb_define_module_under( rbverse_mVerse, "Observable" );

	rbverse_mVersePingObserver = rb_define_module_under( rbverse_mVerse, "PingObserver" );

	rbverse_eVerseError =
		rb_define_class_under( rbverse_mVerse, "Error", rb_eRuntimeError );
	rbverse_eVerseServerError =
		rb_define_class_under( rbverse_mVerse, "ServerError", rbverse_eVerseError );
	rbverse_eVerseConnectError =
		rb_define_class_under( rbverse_mVerse, "ConnectError", rbverse_eVerseError );
	rbverse_eVerseSessionError =
		rb_define_class_under( rbverse_mVerse, "SessionError", rbverse_eVerseError );
	rbverse_eVerseNodeError =
		rb_define_class_under( rbverse_mVerse, "NodeError", rbverse_eVerseError );

	/* Verse constants */
	rb_define_const( rbverse_mVerse, "RELEASE_NUMBER", INT2FIX(V_RELEASE_NUMBER) );
	rb_define_const( rbverse_mVerse, "RELEASE_PATCH",  INT2FIX(V_RELEASE_PATCH) );
	rb_define_const( rbverse_mVerse, "RELEASE_LABEL",  rb_str_new2(V_RELEASE_LABEL) );

	/* Module methods */
	rb_define_singleton_method( rbverse_mVerse, "port=", rbverse_verse_port_eq, 1 );
	rb_define_alias( CLASS_OF(rbverse_mVerse),  "connect_port=", "port=" );
	rb_define_singleton_method( rbverse_mVerse, "create_host_id", rbverse_verse_create_host_id, 0 );
	rb_define_alias( CLASS_OF(rbverse_mVerse),  "make_host_id", "create_host_id" );
	rb_define_singleton_method( rbverse_mVerse, "host_id=", rbverse_verse_host_id_eq, 1 );
	rb_define_singleton_method( rbverse_mVerse, "ping", rbverse_verse_ping, 2 );
	rb_define_singleton_method( rbverse_mVerse, "update", rbverse_verse_update, -1 );
	rb_define_alias( CLASS_OF(rbverse_mVerse),  "callback_update", "update" );

	/*
	 * Constants
	 */
	rbverse_mVerseConstants = rb_define_module_under( rbverse_mVerse, "Constants" );

	rb_define_const( rbverse_mVerseConstants, "HOST_ID_SIZE", INT2FIX(V_HOST_ID_SIZE) );

	rb_define_const( rbverse_mVerseConstants, "V_NT_OBJECT", rb_uint2inum(V_NT_OBJECT) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_GEOMETRY", rb_uint2inum(V_NT_GEOMETRY) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_MATERIAL", rb_uint2inum(V_NT_MATERIAL) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_BITMAP", rb_uint2inum(V_NT_BITMAP) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_TEXT", rb_uint2inum(V_NT_TEXT) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_CURVE", rb_uint2inum(V_NT_CURVE) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_AUDIO", rb_uint2inum(V_NT_AUDIO) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_NUM_TYPES", rb_uint2inum(V_NT_NUM_TYPES) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_SYSTEM", rb_uint2inum(V_NT_SYSTEM) );
	rb_define_const( rbverse_mVerseConstants, "V_NT_NUM_TYPES_NETPACK",
	                 rb_uint2inum(V_NT_NUM_TYPES_NETPACK) );

	/* Init the subordinate classes */
	rbverse_init_verse_session();
	rbverse_init_verse_server();
	rbverse_init_verse_node();
	rbverse_init_verse_mixins();

	/* Set up calbacks */
	verse_callback_set( verse_send_ping, rbverse_cb_ping, NULL );

	rbverse_log( "debug", "Initialized the extension." );
}

