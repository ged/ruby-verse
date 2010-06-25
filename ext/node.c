/* 
 * Verse::Node -- Verse node class
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

VALUE rbverse_cVerseNode;
VALUE rbverse_mVerseNodeObserver;

/* Mapping of V_NT_* enum to the equivalent Verse::Node subclass */
VALUE rbverse_nodetype_to_nodeclass[ V_NT_NUM_TYPES ];

/* Mapping of VNodeID -> Verse::Node VALUE */
static st_table *node_map;

/* Vtables for mark and free functions for child types. These are
 * populated by the initializers for each of the child types. */
void ( *node_mark_funcs[V_NT_NUM_TYPES] )(struct rbverse_node *) = {};
void ( *node_free_funcs[V_NT_NUM_TYPES] )(struct rbverse_node *) = {};

/* Structs for passing callback data back into Ruby */
struct rbverse_node_name_set_event {
	VNodeID node_id;
	const char *name;
};


/* --------------------------------------------------
 *	Memory-management functions
 * -------------------------------------------------- */

/*
 * Allocation function
 */
static struct rbverse_node *
rbverse_node_alloc( void ) {
	struct rbverse_node *ptr = ALLOC( struct rbverse_node );

	ptr->id         = ~0;
	ptr->type       = V_NT_SYSTEM;
	ptr->owner      = VN_OWNER_OTHER;
	ptr->name       = Qnil;
	ptr->tag_groups = rb_ary_new();
	ptr->session    = Qnil;

	rbverse_log( "debug", "allocated a rbverse_SESSION <%p>", ptr );
	return ptr;
}


/*
 * GC Mark function
 */
static void
rbverse_node_gc_mark( struct rbverse_node *ptr ) {
	if ( ptr ) {
		rb_gc_mark( ptr->name );
		rb_gc_mark( ptr->tag_groups );
		rb_gc_mark( ptr->session );

		/* Call the node-specific mark function if there is one */
		if ( node_mark_funcs[ptr->type] ) {
			DEBUGMSG( "  mark function 0x%p for node type %d",
			        node_mark_funcs[ptr->type], ptr->type );
			node_mark_funcs[ptr->type]( ptr );
		}
	}
}



/*
 * GC Free function
 */
static void
rbverse_node_gc_free( struct rbverse_node *ptr ) {
	if ( ptr ) {
		DEBUGMSG( "Freeing node 0x%p\n", ptr );

		if ( ptr->id != ~0 ) {
			DEBUGMSG( "  removing node ID %d from node map\n", ptr->id );
			st_delete_safe( node_map, (st_data_t*)&ptr->id, 0, Qundef );
			/* TODO: Do I need to do st_cleanup or anything after deletion? */
		}

		/* Call the node-specific free function if there is one */
		if ( ptr->type < V_NT_NUM_TYPES ) {
			DEBUGMSG( "  free function %p for node type %d\n",
			        node_free_funcs[ptr->type], ptr->type );
			node_free_funcs[ptr->type]( ptr );
		}

		DEBUGMSG( "  clearing struct.\n" );
		ptr->id         = ~0;
		ptr->type       = V_NT_SYSTEM;
		ptr->owner      = VN_OWNER_OTHER;
		ptr->name       = Qnil;
		ptr->tag_groups = Qnil;
		ptr->session    = Qnil;

		xfree( ptr );
		ptr = NULL;

		DEBUGMSG( "  done.\n" );
	}
}


/*
 * Object validity checker. Returns the data pointer.
 */
static struct rbverse_node *
rbverse_check_node( VALUE self ) {
	Check_Type( self, T_DATA );

    if ( !IsNode(self) ) {
		rb_raise( rb_eTypeError, "wrong argument type %s (expected Verse::Node)",
				  rb_obj_classname(self) );
    }

	return DATA_PTR( self );
}


/* 
 * Fetch the data pointer and check it for sanity. Not static because
 * Verse::Node's children use it as well.
 */
struct rbverse_node *
rbverse_get_node( VALUE self ) {
	struct rbverse_node *node = rbverse_check_node( self );

	if ( !node )
		rb_fatal( "Use of uninitialized Node." );

	return node;
}


/*
 * Return the Verse::Node subclass that corresponds to the specified
 * +node_type+.
 */
VALUE
rbverse_node_class_from_node_type( VNodeType node_type ) {
	if ( node_type >= V_NT_NUM_TYPES ) return Qnil;
	return rbverse_nodetype_to_nodeclass[ node_type ];
}


/*
 * Create a Verse::Node subclass from a VNodeID and a VNodeType.
 */
VALUE
rbverse_wrap_verse_node( VNodeID node_id, VNodeType node_type, VNodeOwner owner ) {
	VALUE node_class = rbverse_nodetype_to_nodeclass[ node_type ];
	VALUE node = Qnil;
	struct rbverse_node *ptr = NULL;

	rbverse_log( "debug", "Wrapping a %s object around node %x",
	             rb_class2name(node_class), node_id );
	node = rb_class_new_instance( 0, NULL, node_class );
	ptr = rbverse_get_node( node );

	if ( ptr->type != node_type )
		rb_fatal( "Ack! Expected a type %d node, but got a %d node instead!",
		          node_type, ptr->type );

	ptr->id    = node_id;
	ptr->owner = owner;

	/* TODO: This should maybe be factored up into Verse::Node#id= or something. */
	st_insert( node_map, (st_data_t)node_id, (st_data_t)node );

	return node;
}


/*
 * Look up an already-wrapped Verse::Node object by its VNodeID.
 */
VALUE
rbverse_lookup_verse_node( VNodeID node_id ) {
	VALUE node = Qnil;

	if ( !st_lookup(node_map, (st_data_t)node_id, (st_data_t *)&node) )
		return Qnil;

	return node;
}


/*
 * Mark a node object as destroyed, disassociating it from its session.
 */
void
rbverse_mark_node_destroyed( VALUE nodeobj ) {
	struct rbverse_node *node = rbverse_get_node( nodeobj );

	rbverse_log_with_context( nodeobj, "debug", "Marking node %x destroyed.", node->id );
	node->destroyed = TRUE;
}


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




/* --------------------------------------------------------------
 * Class Methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    Verse::Node.allocate   -> node
 * 
 * Allocate a new Verse::Node object.
 */
static VALUE
rbverse_verse_node_s_allocate( VALUE klass ) {
	if ( klass == rbverse_cVerseNode )
		rb_raise( rb_eTypeError, "can't instantiate %s directly", rb_class2name(klass) );
	return Data_Wrap_Struct( klass, rbverse_node_gc_mark, rbverse_node_gc_free, 0 );
}



/* --------------------------------------------------------------
 * Public Instance Methods
 * -------------------------------------------------------------- */

/*
 * call-seq:
 *    node.initialize
 * 
 * Initialize the generic parts of a new Verse::Node object.
 * 
 */
static VALUE
rbverse_verse_node_initialize( VALUE self ) {
	if ( !rbverse_check_node(self) ) {
		struct rbverse_node *node;

		DATA_PTR( self ) = node = rbverse_node_alloc();

		rb_call_super( 0, NULL );
	} else {
		rb_raise( rb_eRuntimeError,
				  "Cannot re-initialize a session once it's been created." );
	}

	return self;
}


/*
 * call-seq:
 *    node.add_observer( observer )
 *
 * @see Verse::Observable#add_observer
 *
 */
static VALUE
rbverse_verse_node_add_observer( VALUE self, VALUE observer ) {
	struct rbverse_node *node = rbverse_get_node( self );

	verse_send_node_subscribe( node->id );

	return rb_call_super( 1, &observer );
}


/*
 * Session-locked section of rbverse_verse_node_destroy().
 */
static VALUE
rbverse_verse_node_destroy_l( VALUE nodeid ) {
	verse_send_node_destroy( (VNodeID)nodeid );
	return Qtrue;
}


/*
 * call-seq:
 *    node.destroy
 *
 * Tell the Verse server that the receiving node should be destroyed.
 *
 * @example
 *   
 *   class DestroyLogger
 *       include Verse::SessionObserver
 *       
 *       def on_node_destroy( node )
 *           log.message "Node %d has been destroyed!" % [ node.id ]
 *       end
 *   end
 *   
 *   node.destroy
 *   
 */
static VALUE
rbverse_verse_node_destroy( VALUE self ) {
	struct rbverse_node *node = rbverse_get_node( self );

	rbverse_ensure_node_is_alive( node );
	rbverse_with_session_lock( node->session, rbverse_verse_node_destroy_l, (VALUE)node->id );

	return Qtrue;
}


/*
 * call-seq:
 *    node.destroyed?   -> true or false
 *
 * Returns +true+ if the receiving node has been destroyed on the Verse server.
 *
 */
static VALUE
rbverse_verse_node_destroyed_p( VALUE self ) {
	struct rbverse_node *node = rbverse_get_node( self );
	return node->destroyed ? Qtrue : Qfalse;
}


/*
 * call-seq:
 *    node.session   -> session
 *
 * Returns the Verse::Session this node belongs to if it's been registered with one
 * and hasn't been destroyed.
 */
static VALUE
rbverse_verse_node_session( VALUE self ) {
	struct rbverse_node *node = rbverse_get_node( self );
	return node->session;
}


/*
 * call-seq:
 *    node.id   -> Fixnum
 *
 * Return the node's numeric ID.
 *
 */
static VALUE
rbverse_verse_node_id( VALUE self ) {
	struct rbverse_node *node = rbverse_get_node( self );
	return INT2FIX( node->id );
}


/*
 * call-seq:
 *    node.id = fixnum
 *
 * Set the node's numeric ID.
 * 
 * @raise [Verse::NodeError]  if the node's ID has already been set.
 */
static VALUE
rbverse_verse_node_id_eq( VALUE self, VALUE newid ) {
	struct rbverse_node *node = rbverse_get_node( self );

	if ( node->id != ~0 )
		rb_raise( rbverse_eVerseNodeError, "node's ID is already set" );

	node->id = NUM2UINT( newid );

	return Qtrue;
}


/*
 * call-seq:
 *    node.session = session
 *
 * Set the Verse::Session associated with the node. This is usually done for you; setting the
 * session yourself might have unusual consequences.
 *
 */
static VALUE
rbverse_verse_node_session_eq( VALUE self, VALUE session ) {
	struct rbverse_node *node = rbverse_get_node( self );

	if ( RTEST(node->session) )
		rb_raise( rbverse_eVerseSessionError, "%s already belongs to a session", 
		          RSTRING_PTR(rb_inspect( self )) );

	if ( !IsSession(session) )
		rb_raise( rb_eTypeError, "can't convert %s to %s",
		          rb_obj_classname(session), rb_class2name(rbverse_cVerseSession) );

	node->session = session;

	return session;
}



/* --------------------------------------------------------------
 * Callbacks
 * -------------------------------------------------------------- */

/*
 * Iterator body for node_name_set observers.
 */
static VALUE
rbverse_cb_node_name_set_i( VALUE observer, VALUE node, int argc, VALUE *argv ) {
	const VALUE name = argv[0];
	VALUE cb_args = rb_ary_new2( 2 );

	if ( !rb_obj_is_kind_of(observer, rbverse_mVerseNodeObserver) )
		return Qnil;

	RARRAY_PTR( cb_args )[0] = node;
	RARRAY_PTR( cb_args )[1] = name;

	rbverse_log( "debug", "Node_name_set callback: notifying observer: %s.",
	             RSTRING_PTR(rb_inspect( observer )) );
	return rb_funcall2( observer, rb_intern("on_node_name_set"), RARRAY_LEN(cb_args),
	                    RARRAY_PTR(cb_args) );
}


/*
 * Call the node_name_set handler after aqcuiring the GVL.
 */
static void *
rbverse_cb_node_name_set_body( void *ptr ) {
	struct rbverse_node_name_set_event *event = (struct rbverse_node_name_set_event *)ptr;
	const VALUE node = rbverse_lookup_verse_node( event->node_id );
	VALUE name = rb_str_new2( event->name );
	VALUE observers;

	if ( RTEST(node) ) {
		rbverse_log( "info", "Got node name '%s' for %p.", event->name,
		             RSTRING_PTR(rb_inspect(node)) );

		observers = rb_funcall( node, rb_intern("observers"), 0 );
		rb_block_call( observers, rb_intern("each"), 1, &name, rbverse_cb_node_name_set_i, node );

	} else {
		rbverse_log( "info", "Got node name for a node we haven't loaded (%d)", event->node_id );
	}

	return NULL;
}


/* 
 * Callback for the 'node_name_set' command
 */
static void
rbverse_cb_node_name_set( void *unused, VNodeID node_id, const char *name ) {
	struct rbverse_node_name_set_event event;

	event.node_id = node_id;
	event.name = name;

	DEBUGMSG( " Acquiring GVL for 'node_name_set' event.\n" );
	fflush( stdout );

	rb_thread_call_with_gvl( rbverse_cb_node_name_set_body, (void *)&event );
}


// static void
// rbverse_cb_node_tag_group_create( void *unused ) {}
// 
// static void
// rbverse_cb_node_tag_group_destroy( void *unused ) {}
// 
// static void
// rbverse_cb_node_tag_group_subscribe( void *unused ) {}
// 
// static void
// rbverse_cb_node_tag_group_unsubscribe( void *unused ) {}
// 
// static void
// rbverse_cb_node_tag_create( void *unused ) {}
// 
// static void
// rbverse_cb_node_tag_destroy( void *unused ) {}



/*
 * Verse::Node class
 */
void
rbverse_init_verse_node( void ) {
	rbverse_log( "debug", "Initializing Verse::Node" );

#ifdef FOR_RDOC
	rbverse_mVerse = rb_define_module( "Verse" );
#endif

	/* Related modules */
	rbverse_mVerseNodeObserver = rb_define_module_under( rbverse_mVerse, "NodeObserver" );

	/* VNodeID -> Ruby object lookup table */
	node_map = st_init_numtable();

	rbverse_cVerseNode = rb_define_class_under( rbverse_mVerse, "Node", rb_cObject );
	rb_include_module( rbverse_cVerseNode, rbverse_mVerseLoggable );
	rb_include_module( rbverse_cVerseNode, rbverse_mVerseObservable );

	rb_define_const( rbverse_cVerseNode, "TYPE_NUMBER", INT2FIX(V_NT_SYSTEM) );

	/* Class methods */
	rb_define_alloc_func( rbverse_cVerseNode, rbverse_verse_node_s_allocate );

	/* Initializer */
	rb_define_method( rbverse_cVerseNode, "initialize", rbverse_verse_node_initialize, 0 );

	/* Public instance methods */
	rb_define_method( rbverse_cVerseNode, "add_observer", rbverse_verse_node_add_observer, 1 );

	rb_define_method( rbverse_cVerseNode, "destroy", rbverse_verse_node_destroy, 0 );
	rb_define_method( rbverse_cVerseNode, "destroyed?", rbverse_verse_node_destroyed_p, 0 );

	rb_define_method( rbverse_cVerseNode, "session", rbverse_verse_node_session, 0 );

	rb_define_method( rbverse_cVerseNode, "id", rbverse_verse_node_id, 0 );
	rb_define_method( rbverse_cVerseNode, "id=", rbverse_verse_node_id_eq, 1 );

	/* Protected instance methods */
	rb_define_protected_method( rbverse_cVerseNode, "session=", rbverse_verse_node_session_eq, 1 );


	/* Init child classes */
	rbverse_init_verse_objectnode();
	rbverse_init_verse_geometrynode();
	rbverse_init_verse_materialnode();
	rbverse_init_verse_textnode();
	rbverse_init_verse_bitmapnode();
	rbverse_init_verse_curvenode();
	rbverse_init_verse_audionode();

	// DEBUGMSG( "Free function for ObjectNode (%d) is: %p\n", V_NT_OBJECT, node_free_funcs[V_NT_OBJECT] );
	// DEBUGMSG( "Free function for GeometryNode (%d) is: %p\n", V_NT_GEOMETRY, node_free_funcs[V_NT_GEOMETRY] );
	// DEBUGMSG( "Free function for MaterialNode (%d) is: %p\n", V_NT_MATERIAL, node_free_funcs[V_NT_MATERIAL] );
	// DEBUGMSG( "Free function for TextNode (%d) is: %p\n", V_NT_TEXT, node_free_funcs[V_NT_TEXT] );
	// DEBUGMSG( "Free function for BitmapNode (%d) is: %p\n", V_NT_BITMAP, node_free_funcs[V_NT_BITMAP] );
	// DEBUGMSG( "Free function for CurveNode (%d) is: %p\n", V_NT_CURVE, node_free_funcs[V_NT_CURVE] );
	// DEBUGMSG( "Free function for AudioNode (%d) is: %p\n", V_NT_AUDIO, node_free_funcs[V_NT_AUDIO] );

	verse_callback_set( verse_send_node_name_set, rbverse_cb_node_name_set, NULL );
	// verse_callback_set( verse_send_tag_group_create, rbverse_cb_node_tag_group_create, NULL );
	// verse_callback_set( verse_send_tag_group_destroy, rbverse_cb_node_tag_group_destroy, NULL );
	// verse_callback_set( verse_send_tag_group_subscribe, rbverse_cb_node_tag_group_subscribe, NULL );
	// verse_callback_set( verse_send_tag_group_unsubscribe, rbverse_cb_node_tag_group_unsubscribe, NULL );
	// verse_callback_set( verse_send_tag_create, rbverse_cb_node_tag_create, NULL );
	// verse_callback_set( verse_send_tag_destroy, rbverse_cb_node_tag_destroy, NULL );
}

