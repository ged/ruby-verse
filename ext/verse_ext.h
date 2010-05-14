#ifndef __VERSE_EXT_H__
#define __VERSE_EXT_H__

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <verse.h>

#include "ruby.h"
#include "ruby/intern.h"
#include "ruby/encoding.h"

/* Missing declarations of "experimental" thread functions from ruby/thread.c */
void * rb_thread_call_with_gvl(void *(*)(void *), void *);


/* --------------------------------------------------------------
 * Globals
 * -------------------------------------------------------------- */

extern VALUE rbverse_mVerse;
extern VALUE rbverse_mVerseConstants;

extern VALUE rbverse_cVerseSession;

extern VALUE rbverse_eVerseConnectError;



/* --------------------------------------------------------------
 * Typedefs
 * -------------------------------------------------------------- */

typedef struct rbverse_session {
	VSession id;
	VALUE    self;
	VALUE    address;
	VALUE    callbacks;
} rbverse_SESSION;

typedef struct rbverse_connect_accept_event {
	void		*session;
	VNodeID		avatar;
	const char	*address;
	uint8		*hostid;
} rbverse_CONNECT_ACCEPT_EVENT;

/* --------------------------------------------------------------
 * Macros
 * -------------------------------------------------------------- */
#define IsSession( obj ) rb_obj_is_kind_of( (obj), rbverse_cVerseSession )

#define DEFAULT_ADDRESS rb_str_new2( "127.0.0.1" );

/* --------------------------------------------------------------
 * Declarations
 * -------------------------------------------------------------- */

#ifdef HAVE_STDARG_PROTOTYPES
#include <stdarg.h>
#define va_init_list(a,b) va_start(a,b)
void rbverse_log_with_context( VALUE, const char *, const char *, ... );
void rbverse_log( const char *, const char *, ... );
#else
#include <varargs.h>
#define va_init_list(a,b) va_start(a)
void rbverse_log_with_context( VALUE, const char *, const char *, va_dcl );
void rbverse_log( const char *, const char *, va_dcl );
#endif

extern inline uint8 * rbverse_str2host_id( VALUE );
extern inline VALUE rbverse_host_id2str( const uint8 * );
extern void * rbverse_sysfail( void * );


/* --------------------------------------------------------------
 * Initializers
 * -------------------------------------------------------------- */

void Init_verse_ext( void );

extern void rbverse_init_verse_session _(( void ));

#endif /* __VERSE_EXT_H__ */

