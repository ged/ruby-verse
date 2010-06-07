# Ruby-Verse

This is a Ruby binding for the Verse network protocol. Verse is a network 
protocol that lets multiple applications act together as one large application by 
sharing data over a network. 

You can check out the current development source with Mercurial like so:

    hg clone http://bitbucket.org/ged/ruby-verse

You can submit bug reports, suggestions, and read more about future plans at the
[project page](http://bitbucket.org/ged/ruby-verse).

## Basic Design

The API was inspired by [Ample](http://www.elmindreda.org/verse/ample/), a C++ wrapper for Verse by
Camilla Berglund <elmindreda@elmindreda.org>.

Ruby-Verse consists of several core module functions for interacting with a Verse server, a
collection of Node classes, and mixin modules for adding Verse-awareness to your classes.

### Namespace

All classes and modules in Ruby-Verse are declared inside the _Verse_ module.

### Classes

* Session

* Node
** ObjectNode
*** ObjectLink
*** ObjectTransform
*** ObjectMethod
*** ObjectMethodGroup
** GeometryNode
*** GeometryBone
*** GeometryLayer
*** GeometryCrease
** MaterialNode
*** MaterialFragment
** TextNode
*** TextBuffer
** BitmapNode
*** BitmapLayer
** CurveNode
** AudioNode


### Observer Mixins

* ConnectionObserver
* SessionObserver

* NodeObserver
** NodeTagObserver
** NodeTagGroupObserver
* ObjectNodeObserver
** ObjectLinkObserver
** ObjectTransformObserver
** ObjectMethodObserver
** ObjectMethodGroupObserver
* GeometryNodeObserver
** GeometryBoneObserver
** GeometryLayerObserver
** GeometryCreaseObserver
* MaterialNodeObserver
** FragmentObserver
* TextNodeObserver
** TextBufferObserver
* BitmapNodeObserver
** BitmapLayerObserver
* CurveNodeObserver
* AudioNodeObserver




## License

See the included LICENSE file for licensing details.
