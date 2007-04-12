// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License

// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

//

// Test case for Function ActionScript class
// compile this test case with Ming makeswf, and then
// execute it like this gnash -1 -r 0 -v out.swf

rcsid="$Id: MovieClip.as,v 1.54 2007/04/12 09:14:36 strk Exp $";

#include "check.as"

#if OUTPUT_VERSION < 6
note("WARNING: it has been reported that adobe flash player version 9 fails a few tests here.");
note("         We belive those are actually adobe player bugs since older versions ");
note("         of the player are reported to pass all tests. If you have another idea ");
note("         we'd be glad to hear from you, just check the testcase source code.");
note();
#endif

// Get a reference to a MovieClip
var mc = _root;
check(typeof(mc)=="movieclip");

// Check some references
check_equals(typeof(this), 'movieclip');
check_equals(typeof(_parent), 'undefined');
check_equals(_root, this);

// Check inheritance
check(MovieClip);
check_equals(mc.__proto__, MovieClip.prototype);
check_equals(typeof(MovieClip.prototype._width), "undefined");
check_equals(typeof(MovieClip.prototype.attachMovie), "function");
check_equals(typeof(mc._width), "number");

// Check methods existance

// SWF5 or higher
check(mc.attachMovie);
check(mc.getBytesLoaded);
check(mc.getBytesTotal);
check(mc.getBounds);
check(mc.globalToLocal);
check_equals(typeOf(mc.unloadMovie), 'function');

// This seems unavailable
// when targetting SWF > 6
#if OUTPUT_VERSION > 6
check_equals(mc.duplicateMovieClip, undefined);
#else
check(mc.duplicateMovieClip);
#endif

#if OUTPUT_VERSION >= 6
	check_equals(typeof(mc.beginFill), 'function');
	check_equals(typeof(mc.beginGradientFill), 'function');
        check_equals(typeof(mc.clear), 'function');
	check_equals(typeof(mc.createEmptyMovieClip), 'function');
	check_equals(typeof(mc.createTextField), 'function');
	check_equals(typeof(mc.curveTo), 'function');
	check_equals(typeof(mc.lineStyle), 'function');
	check_equals(typeof(mc.lineTo), 'function');
	check_equals(typeof(mc.moveTo), 'function');
	check_equals(typeof(mc.attachAudio), 'function');
	check_equals(typeof(mc.endFill), 'function');
	check_equals(typeof(mc.getDepth), 'function');
	check_equals(typeof(mc.getURL), 'function');
	check_equals(typeof(mc.gotoAndPlay), 'function');
	check_equals(typeof(mc.gotoAndStop), 'function');
	check_equals(typeof(mc.hitTest), 'function');
	check_equals(typeof(mc.nextFrame), 'function');
	check_equals(typeof(mc.play), 'function');
	check_equals(typeof(mc.prevFrame), 'function');
	check_equals(typeof(mc.stop), 'function');
	check_equals(typeof(mc.swapDepths), 'function');

	// These two seem unavailable
	// when targetting SWF > 6
#if OUTPUT_VERSION > 6
	check_equals(typeof(mc.loadMovie), 'undefined');
	check_equals(typeof(mc.removeMovieClip), 'undefined');
#else
	check_equals(typeof(mc.loadMovie), 'function');
	check_equals(typeof(mc.removeMovieClip), 'function');
#endif

#endif // OUTPUT_VERSION >= 6

#if OUTPUT_VERSION >= 7
    xcheck(mc.getInstanceAtDepth != undefined);
    xcheck(mc.getSWFVersion != undefined);
    xcheck(mc.getTextSnapshot != undefined);

    // can't confirm this works !
    // maybe we should just NOT use the _root for this ?
    //check(mc.loadVariables != undefined);

    xcheck(mc.localToGlobal);
    xcheck(mc.setMask);
    check(mc.startDrag);
    check(mc.stopDrag);
    xcheck(mc.enabled);

    // maybe this is the start condition...
    check_equals(mc.focusEnabled, undefined);
    check_equals(mc.hitArea, undefined);
    check_equals(mc.menu, undefined);

    check_equals(mc.getNextHighestDepth(), 0);
#else
    check_equals(mc.getNextHighestDepth(), undefined);
#endif

// Even handlers are initially undefined, user can
// assign them a function to be called on that event...
check_equals(mc.onData, undefined);
check_equals(mc.onDragOut, undefined);
check_equals(mc.onDragOver, undefined);
check_equals(mc.onEnterFrame, undefined);
check_equals(mc.onKeyDown, undefined);
check_equals(mc.onKeyUp, undefined);
check_equals(mc.onKillFocus, undefined);
check_equals(mc.onLoad, undefined);
check_equals(mc.onMouseDown, undefined);
check_equals(mc.onMouseMove, undefined);
check_equals(mc.onMouseUp, undefined);
check_equals(mc.onPress, undefined);
check_equals(mc.onRelease, undefined);
check_equals(mc.onReleaseOutside, undefined);
check_equals(mc.onRollOut, undefined);
check_equals(mc.onRollOver, undefined);
check_equals(mc.onSetFocus, undefined);
check_equals(mc.onUnload, undefined);
#if OUTPUT_VERSION >=6
check(! mc.hasOwnProperty('onData'));
check(! mc.hasOwnProperty('onDragOut'));
check(! mc.hasOwnProperty('onDragOver'));
check(! mc.hasOwnProperty('onEnterFrame')); 
check(! mc.hasOwnProperty('onKeyDown'));
check(! mc.hasOwnProperty('onKeyUp'));
check(! mc.hasOwnProperty('onKillFocus'));
check(! mc.hasOwnProperty('onLoad'));
check(! mc.hasOwnProperty('onMouseDown'));
check(! mc.hasOwnProperty('onMouseMove'));
check(! mc.hasOwnProperty('onMouseUp'));
check(! mc.hasOwnProperty('onPress'));
check(! mc.hasOwnProperty('onRelease'));
check(! mc.hasOwnProperty('onReleaseOutside'));
check(! mc.hasOwnProperty('onRollOut'));
check(! mc.hasOwnProperty('onRollOver'));
check(! mc.hasOwnProperty('onSetFocus'));
check(! mc.hasOwnProperty('onUnload'));
#endif

// Check property existance

// These are undefined by default
check_equals(mc.tabChildren, undefined);
mc.tabChildren = false;
check_equals(mc.tabChildren, false);
mc.tabChildren = true;
check_equals(mc.tabChildren, true);
check_equals(mc.tabEnabled, undefined);
check_equals(mc.tabIndex, undefined);
check_equals(mc.trackAsMenu, undefined);
xcheck_equals(mc.useHandCursor, true);
mc.useHandCursor = false;
check_equals(mc.useHandCursor, false);
check_equals(mc._alpha, 100);
check(mc._currentframe != undefined);

#if OUTPUT_VERSION > 5
check_equals(mc._droptarget, "");
check_equals(typeof(mc._droptarget), "string");
#else
check_equals(mc._droptarget, undefined);
#endif

check(mc._focusrect != undefined);
check(mc._framesloaded != undefined);
check(mc._height != undefined);
check(mc._highquality != undefined);
check(mc._y != undefined);
check(mc._ymouse != undefined);
check(mc._yscale != undefined);
xcheck(mc._lockroot != undefined);

#if OUTPUT_VERSION > 5
check_equals(mc._name, "");
check_equals(typeof(mc._name), "string");
mc._name = "changed";
check_equals(typeof(mc._name), "string");
check_equals(typeof(mc), "movieclip");
#else
check_equals(mc._name, undefined);
#endif

check(mc._parent == undefined);
check(mc._rotation != undefined);
check(mc._soundbuftime != undefined);
check(mc._target != undefined);
check(mc._totalframes != undefined);
check(mc._url != undefined);
check(mc._visible != undefined);
check(mc._width != undefined);
check(mc._x != undefined);
check(mc._xmouse != undefined);
check(mc._xscale != undefined);

//----------------------------------------------
// Test createEmptyMovieClip
//----------------------------------------------

#if OUTPUT_VERSION >= 6
// Test movieclip creation
var mc2 = createEmptyMovieClip("mc2_mc", 50, 0, 0, 0);
check(mc2 != undefined);
check_equals(mc2_mc.getBytesLoaded(), 0);
check_equals(mc2_mc.getBytesTotal(), 0);
check_equals(mc2.getBytesLoaded(), 0);
check_equals(mc2.getBytesTotal(), 0);

var mc3 = createEmptyMovieClip("mc3_mc", 50);
check(mc3 != undefined);
check_equals(mc3.getDepth(), 50);

// By default useHandCursor is false in SWF5 and true in later versions
#if OUTPUT_VERSION < 6
check_equals(mc3.useHandCursor, false);
#else
xcheck_equals(mc3.useHandCursor, true);
#endif
// We add a mouse event handler, and expect this
// to make useHandCursor true
mc3.onMouseOver = function() { trace("over"); };
xcheck_equals(mc3.useHandCursor, true);
mc3.useHandCursor = false;
check_equals(mc3.useHandCursor, false);

check_equals(mc3_mc.getBytesLoaded(), 0);
check_equals(mc3_mc.getBytesTotal(), 0);
check_equals(mc3.getBytesLoaded(), 0);
check_equals(mc3.getBytesTotal(), 0);
check_equals(mc3_mc, _level0.mc3_mc);
check_equals(String(mc3_mc), "_level0.mc3_mc");
#endif


// Test the _target property
check_equals(_root._target, "/");

#if OUTPUT_VERSION >= 6
// unfortunately we can't use createEmptyMovieClip with
// lower SWF targets...
var mc4 = _root.createEmptyMovieClip("mc4_mc", 60);
check_equals(mc4._parent, _root);
check_equals(mc4._target, "/mc4_mc");
var mc5 = mc4.createEmptyMovieClip("mc5_mc", 60);
check_equals(mc5._target, "/mc4_mc/mc5_mc");
check_equals(mc5._parent, mc4);
#endif

//--------------------------------------------------------------------------
// Test "soft" references
// See http://thread.gmane.org/gmane.comp.web.flashcoders.devel/84030
//--------------------------------------------------------------------------

// There's no such think as a _global.removeMovieClip
// What is referred to the "global" function does actually
// resolve to an ActionRemoveClip tag (0.25)
check_equals(typeof(_global.removeMovieClip), 'undefined');

#if OUTPUT_VERSION >= 6

softref = _root.createEmptyMovieClip("hardref", 60);
check_equals(typeof(hardref), 'movieclip');
check_equals(typeof(softref), 'movieclip');
softref.member = 1;
check_equals(typeof(softref.member), 'number');
check_equals(softref.member, 1);
#if OUTPUT_VERSION > 6
removeMovieClip(hardref); // using ActionRemoveClip (0x25)
#else
// just to test another way, ActionRemoveClip in SWF6 will work as well
hardref.removeMovieClip(); // using the sprite's removeMovieClip 
//softref.removeMovieClip(); // use the softref's removeMovieClip 
#endif
check_equals(typeof(hardref), 'undefined');
check_equals(typeof(softref), 'movieclip');
check_equals(typeof(softref.member), 'undefined');
_root.createEmptyMovieClip("hardref", 61);
hardref.member = 2;
check_equals(typeof(softref.member), 'number');
check_equals(softref.member, 2);

#endif // OUTPUT_VERSION >= 6

//----------------------------------------------
// Test duplicateMovieClip
//----------------------------------------------
#if OUTPUT_VERSION >= 6
_root.createEmptyMovieClip("original", 61);
_root.original.createEmptyMovieClip("child1", 1);
_root.original._x = 100;
_root.original.onEnterFrame = function() { };
_root.original.onRollOver = function() { };
check_equals(typeof(_root.original), 'movieclip');
check_equals(typeof(_root.original.child1), 'movieclip');
check_equals(_root.original._x, 100);
check_equals(typeof(_root.original.onEnterFrame), 'function');
check_equals(typeof(_root.original.onRollOver), 'function');

duplicateMovieClip(_root.original, "copy1", 63);
check_equals(typeof(_root.copy1), 'movieclip');
check_equals(typeof(_root.copy1.child1), 'undefined');
check_equals(typeof(_root.copy1.onEnterFrame), 'undefined');
check_equals(typeof(_root.copy1.onRollOver), 'undefined');
check_equals(_root.copy1.getDepth(), 63);
check_equals(_root.copy1._x, 100);

#if OUTPUT_VERSION == 6
// SWF7 and higher removed duplicateMovieClip method of MovieClip class
_root.original.duplicateMovieClip("copy2", 64);
check_equals(typeof(_root.copy2), 'movieclip');
check_equals(typeof(_root.copy2.child1), 'undefined');
check_equals(typeof(_root.copy2.onEnterFrame), 'undefined');
check_equals(typeof(_root.copy2.onRollOver), 'undefined');
check_equals(_root.copy2.getDepth(), 64);
check_equals(_root.copy2._x, 100);
#endif // OUTPUT_VERSION = 6
#endif // OUTPUT_VERSION >= 6

//----------------------------------------------
// Test timeline variables
//----------------------------------------------

var c = 1;
b = 1;
check_equals(c, 1);
check_equals(_root.c, 1);
check_equals(b, 1);
check_equals(_root.b, 1);

//----------------------------------------------
// Test new MovieClip
//----------------------------------------------

var cl = new MovieClip();
check_equals(cl.__proto__.constructor, MovieClip);
check(cl instanceOf MovieClip);
check(cl instanceOf Object);
check_equals(typeof(cl), "object");
check_equals(typeof(cl.attachMovie), "function");
check_equals(typeof(cl._width), "undefined");
check_equals(typeof(cl._parent), "undefined");

//------------------------------------------------
// Test onLoad to be allowed to be set to anything
//------------------------------------------------

_root.onLoad = 3;
check_equals(typeof(_root.onLoad), 'number');
_root.onLoad = "test";
check_equals(typeof(_root.onLoad), 'string');

//-----------------------------------------------------------
// Test $version
//-----------------------------------------------------------

#if OUTPUT_VERSION >= 6
check(this.hasOwnProperty("$version"));
#endif
check_equals(typeof(this.$version), 'string');

function enumerate(obj, enum)
{
	var enumlen = 0;
	for (var i in obj) {
		enum[i] = obj[i];
		++enumlen;
	}
	return enumlen;
}

// Check that $version is enumerable
enum = new Object; enumlen = enumerate(this, enum);
check_equals(typeof(enum['$version']), 'string');

// Check that $version is overridable and deletable
this.$version = "fake version";
check_equals(this.$version, 'fake version');
check(delete $version);
check_equals(typeof(this.$version), 'undefined');

//------------------------------------------------
// Test getProperty 
//------------------------------------------------

#ifdef MING_SUPPORTS_ASM_GETPROPERTY

asm {
	push "a"
	push "" // this doesn't resolve to top of with stack
	push 13 // _name
	getproperty
	setvariable
};
#if OUTPUT_VERSION > 5
check_equals(a, "changed");
#else
check_equals(a, undefined);
#endif

asm {
	push "b"
	push "" // this doesn't resolve to top of with stack
	push 11 // _target
	getproperty
	setvariable
};
check_equals(b, "/");

asm {
	push "a"
	push "_root"
	push 13 // _name
	getproperty
	setvariable
};
#if OUTPUT_VERSION > 5
check_equals(a, "changed");
#else
check_equals(a, undefined);
#endif

asm {
	push "b"
	push "_root"
	push 11 // _target
	getproperty
	setvariable
};
check_equals(b, "/");

#endif // MING_SUPPORT_ASM_GETPROPERTY

//------------------------------------------------
// Test createTextField
//------------------------------------------------

t = createTextField("textfieldTest", 3, 0, 100, 100, 100);
check_equals(typeof(t), 'undefined');
#if OUTPUT_VERSION > 5
check_equals(typeof(textfieldTest), 'object');
check(textfieldTest instanceof TextField);
#else
xcheck_equals(typeof(textfieldTest), 'movieclip');
xcheck(textfieldTest instanceof MovieClip);
#endif

//----------------------------------------------
// Test getDepth
//----------------------------------------------

// A getDepth call against a script-created clip
// is already tested in the createEmptyMovieClip test
// section. Here we try to test it against a statically
// defined movie. We hope that the 'dejagnu' clip
// is statically defined, if it's not we'll raise a 
// warning about it.

// getDepth was not available as of SWF5
#if OUTPUT_VERSION > 5

static_clip_name = "__shared_assets";
static_clip = eval(static_clip_name);
if ( typeof(static_clip) == 'movieclip' )
{
	check_equals(static_clip.getDepth(), -16383);
}
else
{
	note("There is not '"+static_clip_name+"' clip statically-defined, so we could not test getDepth() against it");

}

#endif // OUTPUT_VERSION > 5


//----------------------------------------------
// Test _width, _height and getBounds
//----------------------------------------------

#if OUTPUT_VERSION >= 6

createEmptyMovieClip("container", 5);
container.createEmptyMovieClip("draw", 5);
draw = container.draw;

check_equals(draw._width, 0);
check_equals(draw._height, 0);
b = draw.getBounds();
check_equals(typeof(b), 'object');
check_equals(typeof(b.xMin), 'number');
check_equals(typeof(b.xMax), 'number');
check_equals(typeof(b.yMin), 'number');
check_equals(typeof(b.yMax), 'number');
// Returned number is (2^28/2)-1 twips : any ringing bell ?
check_equals(b.xMin, 6710886.35);
check_equals(b.xMax, 6710886.35);
check_equals(b.yMin, 6710886.35);
check_equals(b.yMax, 6710886.35);

with (draw)
{
	lineStyle(0, 0x000000);
	moveTo(10, 10);
	lineTo(10, 30);
	lineTo(20, 30);
	lineTo(20, 10);
	lineTo(10, 10);
}
check_equals(draw._width, 10);
check_equals(draw._height, 20);
b = draw.getBounds();
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
b = draw.getBounds(container);
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);

draw._x += 20;
b = draw.getBounds();
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
b = draw.getBounds(container);
check_equals(b.xMin, 30);
check_equals(b.xMax, 40);
container._x -= 20;
b = draw.getBounds();
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
b = draw.getBounds(container);
check_equals(b.xMin, 30);
check_equals(b.xMax, 40);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
check_equals(draw._width, 10); 
check_equals(draw._height, 20);
b = draw.getBounds(_root);
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
draw._x -= 20;
container._x += 20;

draw._rotation = 90;
check_equals(draw._width, 20); 
xcheck_equals(draw._height, 10); 
b = draw.getBounds(); // these are local, untransformed
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
b = draw.getBounds(container); // these are transformed by container draw matrix
check_equals(b.xMin, -30);
xcheck_equals(b.xMax, -10);
xcheck_equals(b.yMin, 10);
xcheck_equals(b.yMax, 20);

draw._visible = false;
check_equals(draw._width, 20);
xcheck_equals(draw._height, 10);
b = draw.getBounds(); // these are local, untransformed
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
b = draw.getBounds(container); // these are transformed by container draw matrix
check_equals(b.xMin, -30);
xcheck_equals(b.xMax, -10);
xcheck_equals(b.yMin, 10);
xcheck_equals(b.yMax, 20);

draw._xscale = 200;
check_equals(draw._width, 20);
xcheck_equals(draw._height, 20);

draw._rotation = 0;
xcheck_equals(draw._width, 20);
xcheck_equals(draw._height, 20);

draw._visible = true;
draw._xscale = 100;
check_equals(draw._width, 10);
xcheck_equals(draw._height, 20);

draw._yscale = 50;
check_equals(draw._width, 10);
check_equals(draw._height, 10);
check_equals(container._width, 10);
check_equals(container._height, 10);

container._xscale = 800;
check_equals(draw._width, 10);
check_equals(draw._height, 10);
check_equals(container._width, 80);
check_equals(container._height, 10);
b = draw.getBounds(); // these are local, untransformed
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
b = draw.getBounds(container); // these are transformed by container draw matrix
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 5);
check_equals(b.yMax, 15);

container._xscale = 100;
container._yscale = 100;
draw._yscale = 100;
draw._xscale = 100;
b = draw.getBounds(container); // these are transformed by container draw matrix
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
container.createEmptyMovieClip("draw2", 6);
draw = container.draw2;
with (draw)
{
	lineStyle(0, 0x000000);
	moveTo(60, 20);
	lineTo(60, 40);
	lineTo(80, 40);
	lineTo(80, 20);
	lineTo(60, 20);
}

b = container.draw.getBounds(container); 
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
b = container.draw.getBounds(); 
check_equals(b.xMin, 10);
check_equals(b.xMax, 20);
check_equals(b.yMin, 10);
check_equals(b.yMax, 30);
b = container.draw2.getBounds(container); 
check_equals(b.xMin, 60);
check_equals(b.xMax, 80);
check_equals(b.yMin, 20);
check_equals(b.yMax, 40);
b = container.draw2.getBounds(container.draw);
check_equals(b.xMin, 60);
check_equals(b.xMax, 80);
check_equals(b.yMin, 20);
check_equals(b.yMax, 40);
b = container.draw2.getBounds(invalid);
check_equals(typeof(b), 'undefined');
b = container.draw2.getBounds(__shared_assets);
check_equals(b.xMin, 60);
check_equals(b.xMax, 80);
check_equals(b.yMin, 20);
check_equals(b.yMax, 40);

b = container.draw2.getBounds(); 
check_equals(b.xMin, 60);
check_equals(b.xMax, 80);
check_equals(b.yMin, 20);
check_equals(b.yMax, 40);
b = container.getBounds(); 
check_equals(b.xMin, 10);
check_equals(b.xMax, 80);
check_equals(b.yMin, 10);
check_equals(b.yMax, 40);

container.draw2._x += 20;
b = container.getBounds(); 
check_equals(b.xMin, 10);
check_equals(b.xMax, 100);
check_equals(b.yMin, 10);
check_equals(b.yMax, 40);
check_equals(container._width, 90);
check_equals(container._height, 30);

container.draw2._xscale = 200;
b = container.getBounds(); 
check_equals(b.xMin, 10);
check_equals(b.xMax, 180);
check_equals(b.yMin, 10);
check_equals(b.yMax, 40);

#endif // OUTPUT_VERSION >= 6
