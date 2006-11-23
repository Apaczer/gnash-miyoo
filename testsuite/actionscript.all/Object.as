// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
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

// Test case for Object ActionScript class
// compile this test case with Ming makeswf, and then
// execute it like this gnash -1 -r 0 -v out.swf

rcsid="$Id: Object.as,v 1.16 2006/11/23 15:45:42 strk Exp $";

#include "check.as"

// Test Object creation using 'new'
var obj = new Object; // uses SWFACTION_NEWOBJECT
check (obj != undefined);
check (typeof(obj) == "object");
check (obj.__proto__.constructor == Object);

// Test instantiated Object members
obj.member = 1;
check (obj.member == 1)

// Test Object creation using literal initialization
var obj2 = { member:1 }; // uses SWFACTION_INITOBJECT
check (obj2 != undefined );
check (typeof(obj2) == "object");
check (obj2.__proto__.constructor == Object);

// Test initialized object members
check ( obj2.member == 1 )

// Test Object creation using initializing constructor
var obj3 = new Object({ member:1 });
check (obj3 != undefined);
check (typeof(obj3) == "object");
check (obj3.__proto__.constructor == Object);

// Test initialized object members
check ( obj3.member != undefined );
check ( obj3.member == 1 );

// Test after-initialization members set/get
obj3.member2 = 3;
check ( obj3.member2 != undefined );
check ( obj3.member2 == 3 );

//----------------------
// Test copy ctors
//----------------------

var copy = new Object(obj3);
check_equals( copy.member2, 3 );
copy.test = 4;
check_equals( copy.test, 4 );
check_equals( obj3.test, 4 );
check (copy.__proto__.constructor == Object);


//----------------------
// Test addProperty
//----------------------

// the 'getter' function
function getLen() {
	return this._len;
}

// the 'setter' function
function setLen(l) {
	this._len = l;
}

// add the "len" property
var ret = obj3.addProperty("len", getLen, setLen);
check_equals(ret, true);

check_equals (obj3.len, undefined);
obj3._len = 3;
check_equals (obj3.len, 3);
obj3.len = 5;
check_equals (obj3._len, 5);
check_equals (obj3.len, 5);

// TODO: try using the name of an existing property

// Try property inheritance

var proto = new Object();
check(proto.addProperty("len", getLen, setLen));
var inh1 = new Object();
inh1.__proto__ = proto;
var inh2 = new Object();
inh2.__proto__ = proto;
check_equals (inh1._len, undefined);
check_equals (inh2._len, undefined);
inh1.len = 4; 
inh2.len = 9;
check_equals (inh1._len, 4);
check_equals (inh2._len, 9);
check_equals (proto._len, undefined);
inh1._len = 5;
inh2._len = 7;
check_equals (inh1.len, 5);
check_equals (inh2.len, 7);
check_equals (proto.len, undefined);


//----------------------
// Test enumeration
//----------------------

function enumerate(obj, enum)
{
	var enumlen = 0;
	for (var i in obj) {
		enum[i] = obj[i];
		++enumlen;
	}
	return enumlen;
}

var l0 = new Object({a:1, b:2});
var l1 = new Object({c:3, d:4});
l1.__proto__ = l0;
var l2 = new Object({e:5, f:6});
l2.__proto__ = l1;

// check properties
var enum = new Object;
var enumlen = enumerate(l2, enum);
check_equals( enumlen, 6);
check_equals( enum["a"], 1);
check_equals( enum["b"], 2);
check_equals( enum["c"], 3);
check_equals( enum["d"], 4);
check_equals( enum["e"], 5);
check_equals( enum["f"], 6);

// Hide a property of a base object
var ret = ASSetPropFlags(l0, "a", 1);

var enum = new Object;
var enumlen = enumerate(l2, enum);
check_equals( enumlen, 5);
check_equals( enum["a"], undefined);

var obj5 = new Object();
obj5['a'] = 1;
check_equals(obj5['a'], 1);
#if OUTPUT_VERSION < 7
check_equals(obj5['A'], 1);
#else
check_equals(obj5['A'], undefined);
#endif
