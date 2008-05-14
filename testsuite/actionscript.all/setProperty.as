// 
//   Copyright (C) 2008 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// Test case for setProperty/getProperty action opcodes.
// compile this test case with Ming makeswf, and then
// execute it like this gnash -1 -r 0 -v out.swf

rcsid="$Id: setProperty.as,v 1.7 2008/05/14 08:14:25 zoulunkai Exp $";
#include "check.as"

#ifdef MING_SUPPORTS_ASM

//
// Test getProperty and setProperty
//
createSprite = ASnative(901, 0); // MovieClip.prototype.createEmptyMovieClip 
createSprite("mc1", 10);

mc1.func = function () {

    pass_check = _root.pass_check;
    xpass_check = _root.xpass_check;
    fail_check = _root.fail_check;
    xfail_check = _root.xfail_check;

    check_equals(this, _root.mc1);
    check_equals(_root.mc1._xscale, 100);
    check_equals(_root._xscale, 100);
    
    // setProperty("", _xscale, 30);
    // for SWF6 and above, it's _root._xscale = 30.
    // for SWF5, it's mc1._xscale = 30;
    asm{
        push "", 2, 30
        setproperty
    };  
#if OUTPUT_VERSION == 5
    // failed by looking into the wrong context.
    xcheck_equals(_root.mc1._xscale, 30);
    xcheck_equals(_root._xscale, 100);
#else
    // Gnash might be failed due to accuracy problem.
    // should we be more tolerant here?
    check_equals(_root.mc1._xscale, 100);
    check_equals(Math.round(_root._xscale), 30); 
#endif
    _root._xscale = 60;
    var testvar = 0;
    // testvar = getProperty("", _xscale);
    // for SWF6 and above, it's testvar = _root._xscale;
    // for SWF5, it's testvar = mc1._xscale;
    asm{
        push "testvar"
        push "", 2
        getproperty
        setvariable
    };
#if OUTPUT_VERSION == 5
    xcheck_equals(testvar, 30);
#else
    // Gnash might be failed due to accuracy problem.
    // should we be more tolerant here?
    check_equals(Math.round(testvar), 60);
#endif
};
mc1.func();
// restore the _xscale of _root after test.
_root._xscale = 100;


//
//  test removeMovieClip(""), opcode 37
//
createSprite("mc2", 20);
mc2.remove = function () {
    // in SWF6 and obove, it's no effect(probably results _root.removeMovieClip()).
    // in SWF5, mc2.removeMovieClip().
    removeMovieClip("");
};
mc2.remove();
#if OUTPUT_VERSION == 5
    xcheck_equals(typeof(mc2), 'undefined');
#else
    check_equals(typeof(mc2), 'movieclip');
#endif


//
//  test setTarget(""), opcode 139 
//
createSprite("mc3", 30);
_root.checkpoint = 0;
_root.mc3thisPtr = 0;

mc3.set_target = function() {
    // for SWF6 and above, it's setTarget("_root");
    // for SWF5, it's setTarget("mc3");
    setTarget("");
    _root.mc3thisPtr = this;
    asm{
        push '_root.checkpoint', '', 11         
        getproperty  //_target         
        setvariable             
    };
};
mc3.set_target();

check_equals(mc3thisPtr, mc3);

#if OUTPUT_VERSION == 5
    xcheck_equals(_root.checkpoint, "/mc3");
#else
    check_equals(_root.checkpoint, "/");
#endif

//
// Test scope chain in swf5.
//
// The scope chain in swf5 and below works differently, which  
// might explain why AS in swf5(and below) was so unstable. All 
// tests above can also be explained by the scope chain difference
// in swf5.
//
_root.checkpoint = "var_in_root";
createSprite("mc4", 40);
mc4.checkpoint = "var_in_mc4";
mc4.func = function () {
	check_equals(this, _root.mc4);
#if OUTPUT_VERSION == 5
	// for swf5, mc4 is the top of the scope chain.
	xcheck_equals(checkpoint, var_in_mc4);
	xcheck_equals(_target, "/mc4");
#else
	// for swf6 and above, current target is the top of the scope chain.
	check_equals(checkpoint, "var_in_root");
	check_equals(_target, "/");
#endif
};
mc4.func();

obj.checkpoint = "var_in_obj";
obj = new Object();
obj.func = function () {
	check_equals(this, obj);
	// "this" object is not part of the scope chain for all swf versions.
	// But if "this" is a sprite(tests right above), the situation is different...
	check_equals(checkpoint, "var_in_root");
};
obj.func();

check_totals(14); 

#endif //MING_SUPPORTS_ASM
