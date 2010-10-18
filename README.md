# Ruby-Verse

This is a Ruby binding for Verse. Verse is a network protocol that lets multiple applications act
together as one large application by sharing data over a network.

You can check out the current development source with Mercurial like so:

    hg clone http://bitbucket.org/ged/ruby-verse

or if you prefer Git:

    git clone git://github.com/ged/ruby-verse.git

You'll probably also need the rake tasks in a directory called 'rake' in the working directory. If you're using Mercurial, it should pull them down for you as a subrepo, but if you unpack the source or check it out using Git, you may need to do:

    git clone git://github.com/ged/geds-rake-tasklibs.git rake

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

* Server
* Session (Observable)
* Node (Observable)
	- ObjectNode
		* ObjectLink
		* ObjectTransform
		* ObjectMethod
		* ObjectMethodGroup
		* ObjectAnimation
	- GeometryNode
		* GeometryBone
		* GeometryLayer
		* GeometryCrease
	- MaterialNode
		* MaterialFragment
	- TextNode
		* TextBuffer
	- BitmapNode
		* BitmapLayer
	- CurveNode
	  * CurveKey
	- AudioNode
	  * AudioBuffer
	  * AudioStream


### Observer Mixins

* PingObserver
* SessionObserver

* NodeObserver
	- NodeTagObserver
	- NodeTagGroupObserver
* ObjectNodeObserver
	- ObjectLinkObserver
	- ObjectTransformObserver
	- ObjectMethodObserver
	- ObjectMethodGroupObserver
* GeometryNodeObserver
	- GeometryBoneObserver
	- GeometryLayerObserver
	- GeometryCreaseObserver
* MaterialNodeObserver
	- FragmentObserver
* TextNodeObserver
	- TextBufferObserver
* BitmapNodeObserver
	- BitmapLayerObserver
* CurveNodeObserver
* AudioNodeObserver
  - AudioBufferObserver
  - AudioStreamObserver

## License

Portions of this source code were derived from the Verse source, and are used under the 
following licensing terms:

> Copyright (c) 2005-2008, The Uni-Verse Consortium.
> All rights reserved.
> 
> Redistribution and use in source and binary forms, with or without
> modification, are permitted provided that the following conditions are
> met:
> 
>  * Redistributions of source code must retain the above copyright
>    notice, this list of conditions and the following disclaimer.
>  * Redistributions in binary form must reproduce the above copyright
>    notice, this list of conditions and the following disclaimer in the
>    documentation and/or other materials provided with the distribution.
> 
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
> IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
> TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
> PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
> EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
> PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
> PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
> LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
> NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
> SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  
See the included [LICENSE](LICENSE.html) file for licensing details for this library.

