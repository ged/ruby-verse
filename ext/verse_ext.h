/*
 * Ruby Verse extension header
 * $Id$
 */

#ifndef __VERSE_EXT_H__
#define __VERSE_EXT_H__

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "verse.h"

#include "ruby.h"
#ifndef RUBY_VM
#	error Ruby-Verse requires at least Ruby 1.9.1
#else
#	include "ruby/encoding.h"
#	include "ruby/st.h"
#endif /* !RUBY_VM */

#ifdef DEBUG
#	define DEBUGMSG(format, args...) fprintf( stderr, "\033[37mDEBUG: "format"\033[0m\n", ##args );
#else
#	define DEBUGMSG(format, args...)
#endif

/* Missing declarations of "experimental" thread functions from ruby/thread.c */
void * rb_thread_call_with_gvl(void *(*)(void *), void *);


/* --------------------------------------------------------------
 * Globals
 * -------------------------------------------------------------- */

extern VALUE rbverse_mVerse;
extern VALUE rbverse_mVerseConstants;

extern VALUE rbverse_mVerseLoggable;
extern VALUE rbverse_mVerseVersionUtilities;

extern VALUE rbverse_mVerseVersioned;
extern VALUE rbverse_mVerseObserver;
extern VALUE rbverse_mVerseObservable;
extern VALUE rbverse_mVersePingObserver;
extern VALUE rbverse_mVerseSessionObserver;
extern VALUE rbverse_mVerseNodeObserver;

extern VALUE rbverse_cVerseServer;
extern VALUE rbverse_cVerseSession;
extern VALUE rbverse_cVerseNode;

extern VALUE rbverse_cVerseObjectNode;
extern VALUE rbverse_cVerseGeometryNode;
extern VALUE rbverse_cVerseMaterialNode;
extern VALUE rbverse_cVerseBitmapNode;
extern VALUE rbverse_cVerseTextNode;
extern VALUE rbverse_cVerseCurveNode;
extern VALUE rbverse_cVerseAudioNode;

extern VALUE rbverse_eVerseError;
extern VALUE rbverse_eVerseServerError;
extern VALUE rbverse_eVerseConnectError;
extern VALUE rbverse_eVerseSessionError;
extern VALUE rbverse_eVerseNodeError;

extern st_table *session_table;


/* --------------------------------------------------------------
 * Typedefs
 * -------------------------------------------------------------- */

/* Class structures */
struct rbverse_session {
	VSession id;
	VALUE    address;
	VALUE    create_callbacks;
	VALUE    destroy_callbacks;
};

struct rbverse_node {
	VNodeID		id;
	VNodeType	type;
	VNodeOwner	owner;
	VALUE		name;
	VALUE		tag_groups;
	VALUE		session;
	boolean     destroyed;
	union {
		struct {
			VALUE links;
			VALUE transform;
			VALUE light;
			VALUE method_groups;
			VALUE animations;
			boolean hidden;
		} object;
		struct {
			VALUE buffers;
			VALUE streams;
		} audio;
	};
};


/* Verse::Node globals. These are used to hook up child classes into
 * Verse::Node's memory-management and node-creation functions. */
extern VALUE rbverse_nodetype_to_nodeclass[];
extern void ( *node_mark_funcs[] )(struct rbverse_node *);
extern void ( *node_free_funcs[] )(struct rbverse_node *);


/* --------------------------------------------------------------
 * Macros
 * -------------------------------------------------------------- */
#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_UPDATE_TIMEOUT 100000


/* --------------------------------------------------------------
 * Inline functions
 * -------------------------------------------------------------- */

/* 
 * Check the "alive"-ness of the given +node+, raising an exception if it's either not part
 * of a session or has been destroyed.
 */
static inline void
rbverse_ensure_node_is_alive( struct rbverse_node *node ) {
	if ( !node )
		rb_fatal( "node pointer was NULL!" );
	if ( !RTEST(node->session) )
		rb_raise( rbverse_eVerseNodeError, "node is not associated with an active session" );
	if ( node->destroyed )
		rb_raise( rbverse_eVerseNodeError, "node is destroyed" );
}

/* Type-check functions */
static inline boolean IsSession( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseSession ) ? TRUE : FALSE;
}
static inline boolean IsServer( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseServer ) ? TRUE : FALSE;
}
static inline boolean IsNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseNode ) ? TRUE : FALSE;
}
static inline boolean IsAudioNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseAudioNode ) ? TRUE : FALSE;
}
static inline boolean IsBitmapNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseBitmapNode ) ? TRUE : FALSE;
}
static inline boolean IsCurveNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseCurveNode ) ? TRUE : FALSE;
}
static inline boolean IsGeometryNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseGeometryNode ) ? TRUE : FALSE;
}
static inline boolean IsMaterialNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseMaterialNode ) ? TRUE : FALSE;
}
static inline boolean IsObjectNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseObjectNode ) ? TRUE : FALSE;
}
static inline boolean IsTextNode( VALUE obj ) {
	return rb_obj_is_kind_of( obj, rbverse_cVerseTextNode ) ? TRUE : FALSE;
}


/* --------------------------------------------------------------
 * Declarations
 * -------------------------------------------------------------- */

/* verse_ext.c */
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

extern inline uint8 * rbverse_str2host_id			_(( VALUE ));
extern inline VALUE rbverse_host_id2str				_(( const uint8 * ));

/* session.c */
extern VALUE rbverse_get_current_session			_(( void ));
extern VALUE rbverse_with_session_lock				_(( VALUE, VALUE (*)(ANYARGS), VALUE ));
extern VALUE rbverse_verse_session_from_vsession	_(( VSession, VALUE ));
extern VALUE rbverse_verse_session_s_all_connected  _(( VALUE ));

/* node.c */
extern VALUE rbverse_node_class_from_node_type		_(( VNodeType  ));
extern VALUE rbverse_wrap_verse_node				_(( VNodeID, VNodeType, VNodeOwner ));
extern VALUE rbverse_lookup_verse_node				_(( VNodeID ));
extern void rbverse_mark_node_destroyed				_(( VALUE ));
extern struct rbverse_node * rbverse_get_node				_(( VALUE ));


/* --------------------------------------------------------------
 * Initializers
 * -------------------------------------------------------------- */

void Init_verse_ext( void );

extern void rbverse_init_verse_server       _(( void ));
extern void rbverse_init_verse_session      _(( void ));
extern void rbverse_init_verse_mixins       _(( void ));

extern void rbverse_init_verse_node         _(( void ));
extern void rbverse_init_verse_audionode    _(( void ));
extern void rbverse_init_verse_bitmapnode   _(( void ));
extern void rbverse_init_verse_curvenode    _(( void ));
extern void rbverse_init_verse_geometrynode _(( void ));
extern void rbverse_init_verse_materialnode _(( void ));
extern void rbverse_init_verse_objectnode   _(( void ));
extern void rbverse_init_verse_textnode     _(( void ));


#endif /* __VERSE_EXT_H__ */

