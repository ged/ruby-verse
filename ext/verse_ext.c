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

VALUE rbverse_eVerseConnectError;
VALUE rbverse_eVerseSessionError;

/* The global 'ping' and 'connect' callback Procs */
static VALUE rbverse_ping_callback_proc;
static VALUE rbverse_connect_callback_proc;
static VALUE rbverse_connect_terminate_callback_proc;


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

	rbverse_log( "debug", "Pinging '%s' with message '%s'", addr, msg );
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


/* 
 * call-seq: 
 *    Verse.connect_terminate( address, message )
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
rbverse_verse_connect_terminate( VALUE module, VALUE address, VALUE message ) {
	const char *addr, *msg;

	SafeStringValue( address );
	addr = RSTRING_PTR( address );
	SafeStringValue( message );
	msg = RSTRING_PTR( message );

	verse_send_connect_terminate( addr, msg );

	return Qtrue;
}



/* ----------------------------------- *
 *  Callback handlers
 * ----------------------------------- */

/*
 * Call the ping handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_ping_body( void *ptr ) {
	const char **args = (const char **)ptr;
	VALUE address, msg;

	address = rb_str_new2( args[0] );
	msg = rb_str_new2( args[1] );

	if ( rbverse_ping_callback_proc && RTEST(rbverse_ping_callback_proc) ) {
		rbverse_log( "debug", "Handling ping with callback: %s",
			RSTRING_PTR(rb_inspect( rbverse_ping_callback_proc )) );
		rb_funcall( rbverse_ping_callback_proc, rb_intern("call"), 2, address, msg );
	} else {
		rbverse_log( "info", "ping event received with no handler set!" );
	}

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
 * call-seq:
 *     Verse.on_ping( &block )
 *
 * Register a callback for 'ping' events.
 *
 */
static VALUE
rbverse_verse_on_ping( int argc, VALUE *argv, VALUE module ) {
	if ( rb_block_given_p() ) {
		VALUE block = rb_block_proc();

		rbverse_log( "debug", "Setting ping callback to: %s", RSTRING_PTR(rb_inspect( block )) );
		rbverse_ping_callback_proc = block;

		verse_callback_set( verse_send_ping, rbverse_cb_ping, NULL );
		return Qnil;
	} else {
		return rbverse_ping_callback_proc;
	}
}


/*
 *  call-seq:
 *     Verse.on_ping = proc
 *
 * Set the callback for 'ping' events.
 */
static VALUE
rbverse_verse_on_ping_eq( VALUE module, VALUE block ) {
	rbverse_log( "debug", "Setting ping callback to: %s", RSTRING_PTR(rb_inspect( block )) );
	rbverse_ping_callback_proc = block;

	verse_callback_set( verse_send_ping, rbverse_cb_ping, NULL );

	return block;
}


/*
 * Call the connect handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_connect_body( void *ptr ) {
	const char **args = (const char **)ptr;
	VALUE name, pass, address, hostid;

	name    = rb_str_new2( args[0] );
	pass    = rb_str_new2( args[1] );
	address = rb_str_new2( args[2] );
	hostid  = rbverse_host_id2str( (const uint8 *)args[3] );

	if ( rbverse_connect_callback_proc && RTEST(rbverse_connect_callback_proc) ) {
		rbverse_log( "debug", "Handling connect with callback: %s",
			RSTRING_PTR(rb_inspect( rbverse_connect_callback_proc )) );
		rb_funcall( rbverse_connect_callback_proc, rb_intern("call"), 4, name, pass, address, hostid );
	} else {
		rbverse_log( "info", "connect event received with no handler set!" );
	}

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
 * call-seq:
 *    Verse.on_connect {|name, pass, address, expected_host_id| ... }   -> nil
 *    Verse.on_connect                                                  -> proc
 * 
 * Register a callback to handle 'connect' events, or return the current 
 * callback if no block is given.
 * 
 * @example
 *    sessions = {}
 *    Verse.on_connect do |name, pass, address, hostid|
 *        Verse.connect_terminate( address, "wrong hostid" ) unless
 *            hostid == my_hostid
 *        Verse.connect_terminate( address, "auth failed" ) unless 
 *            authenticate( user, pass )
 *        avatar = Verse::ObjectNode.new
 *        session = Verse.connect_accept( avatar, address, my_hostid )
 *        sessions[ address ] = { :avatar => avatar, :session => session }
 *    end
 *    
 *    Verse.on_connect
 *    # => 
 * 
 * @see Verse.on_connect=
 */
static VALUE
rbverse_verse_on_connect( int argc, VALUE *argv, VALUE module ) {
	if ( rb_block_given_p() ) {
		VALUE block = rb_block_proc();
		rbverse_log( "debug", "Setting 'connect' callback to: %s", RSTRING_PTR(rb_inspect( block )) );
		rbverse_connect_callback_proc = block;

		verse_callback_set( verse_send_connect, rbverse_cb_connect, NULL );
		return Qnil;
	} else {
		return rbverse_connect_callback_proc;
	}
}


/*
 * call-seq:
 *    Verse.on_connect = proc
 * 
 * Register a callback to handle 'connect' events.
 * 
 * @see Verse.on_connect
 */
static VALUE
rbverse_verse_on_connect_eq( VALUE module, VALUE block ) {
	rbverse_log( "debug", "Setting 'connect' callback to: %s", RSTRING_PTR(rb_inspect( block )) );
	rbverse_connect_callback_proc = block;
	return block;
}


/*
 * Call the connect_terminate handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_connect_terminate_body( void *ptr ) {
	const char **args = (const char **)ptr;
	VALUE address, message;

	address = rb_str_new2( args[0] );
	message = rb_str_new2( args[1] );

	if ( rbverse_connect_terminate_callback_proc && RTEST(rbverse_connect_terminate_callback_proc) ) {
		rbverse_log( "debug", "Handling connect_terminate with callback: %s",
			rb_inspect(rbverse_connect_terminate_callback_proc) );
		rb_funcall( rbverse_connect_terminate_callback_proc, rb_intern("call"), 2, address, message );
	} else {
		rbverse_log( "info", "connect_terminate event received with no handler set!" );
	}

	return NULL;
}

/*
 * Callback for the 'connect_terminate' command.
 */
static void
rbverse_cb_connect_terminate( void *unused, const char *address, const char *message ) {
	const char *(args[2]) = { address, message };
	printf( " Acquiring GVL for 'connect_terminate' event." );
	rb_thread_call_with_gvl( rbverse_cb_connect_terminate_body, args );
}


/*
 * call-seq:
 *     Verse.on_connect_terminate {|address, message| ... }
 *
 * Register a callback to handle `connect_terminate' events.
 * 
 *    Verse.on_connect_terminate do |address, message|
 *        conn = sessions.delete( address )
 *        conn[:avatar].destroy
 *    end
 */
static VALUE
rbverse_verse_on_connect_terminate( int argc, VALUE *argv, VALUE module ) {
	if ( rb_block_given_p() ) {
		VALUE block = rb_block_proc();

		rbverse_log( "debug", "Setting 'connect_terminate' callback to: %s", 
		             RSTRING_PTR(rb_inspect( block )) );
		rbverse_connect_terminate_callback_proc = block;

		verse_callback_set( verse_send_connect_terminate, rbverse_cb_connect_terminate, NULL );

		return Qnil;
	} else {
		return rbverse_connect_terminate_callback_proc;
	}
}


/*
 * call-seq:
 *    Verse.on_connect_terminate = proc
 * 
 * Set the callback that handles connect_terminate messages to +proc+.
 * 
 */
static VALUE
rbverse_verse_on_connect_terminate_eq( VALUE module, VALUE block ) {
	rbverse_log( "debug", "Setting 'connect_terminate' callback to: %s",
	             RSTRING_PTR(rb_inspect( block )) );
	rbverse_connect_terminate_callback_proc = block;

	verse_callback_set( verse_send_connect_terminate, rbverse_cb_connect_terminate, NULL );

	return Qnil;
}


/* Body of rbverse_verse_callback_update after GVL is given up. */
static VALUE
rbverse_verse_callback_update_body( void *ptr ) {
	uint32 *microseconds = (uint32 *)ptr;

	verse_callback_update( *microseconds );

	return Qtrue;
}


/*
 * call-seq:
 *     Verse.callback_update( timeout )
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
rbverse_verse_callback_update( VALUE module, VALUE timeout ) {
	double seconds = NUM2DBL( timeout );
	uint32 microseconds = floor( seconds * 1000000 );

	rbverse_log( "debug", "Callback update for session %p (timeout=%u µs).",
	             verse_session_get(), microseconds );
	rb_thread_blocking_region( rbverse_verse_callback_update_body, &microseconds,
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


/*
 * Verse namespace.
 * 
 */
void
Init_verse_ext( void ) {
	rbverse_mVerse = rb_define_module( "Verse" );

	rbverse_eVerseConnectError =
		rb_define_class_under( rbverse_mVerse, "ConnectError", rb_eRuntimeError );
	rbverse_eVerseSessionError =
		rb_define_class_under( rbverse_mVerse, "SessionError", rb_eRuntimeError );

	rb_global_variable( &rbverse_ping_callback_proc );
	rbverse_ping_callback_proc = Qnil;
	rb_global_variable( &rbverse_connect_callback_proc );
	rbverse_connect_callback_proc = Qnil;
	rb_global_variable( &rbverse_connect_terminate_callback_proc );
	rbverse_connect_terminate_callback_proc = Qnil;

	/* version constants */
	rb_define_const( rbverse_mVerse, "RELEASE_NUMBER", INT2FIX(V_RELEASE_NUMBER) );
	rb_define_const( rbverse_mVerse, "RELEASE_PATCH",  INT2FIX(V_RELEASE_PATCH) );
	rb_define_const( rbverse_mVerse, "RELEASE_LABEL",  rb_str_new2(V_RELEASE_LABEL) );

	/* Module methods */
	rb_define_singleton_method( rbverse_mVerse, "port=", rbverse_verse_port_eq, 1 );
	rb_define_alias( rb_singleton_class(rbverse_mVerse), "connect_port=", "port=" );

	rb_define_singleton_method( rbverse_mVerse, "ping", rbverse_verse_ping, 2 );
	rb_define_singleton_method( rbverse_mVerse, "connect_accept",
		rbverse_verse_connect_accept, 3 );
	rb_define_singleton_method( rbverse_mVerse, "connect_terminate",
		rbverse_verse_connect_terminate, 2 );

	rb_define_singleton_method( rbverse_mVerse, "on_ping", rbverse_verse_on_ping, -1 );
	rb_define_singleton_method( rbverse_mVerse, "on_ping=", rbverse_verse_on_ping_eq, 1 );
	rb_define_singleton_method( rbverse_mVerse, "on_connect", rbverse_verse_on_connect, -1 );
	rb_define_singleton_method( rbverse_mVerse, "on_connect=", rbverse_verse_on_connect_eq, 1 );
	rb_define_singleton_method( rbverse_mVerse, "on_connect_terminate",
		rbverse_verse_on_connect_terminate, -1 );

	rb_define_singleton_method( rbverse_mVerse, "callback_update",
		rbverse_verse_callback_update, 1 );

	rb_define_singleton_method( rbverse_mVerse, "create_host_id", rbverse_verse_create_host_id, 0 );
	rb_define_singleton_method( rbverse_mVerse, "host_id=", rbverse_verse_host_id_eq, 1 );

	/*
	 * Constants
	 */
	rbverse_mVerseConstants = rb_define_module_under( rbverse_mVerse, "Constants" );

	rb_define_const( rbverse_mVerseConstants, "HOST_ID_SIZE", INT2FIX(V_HOST_ID_SIZE) );


	/* Init the subordinate classes */
	rbverse_init_verse_session();

	rbverse_log( "debug", "Initialized the extension." );
	rb_require( "verse" );
}

