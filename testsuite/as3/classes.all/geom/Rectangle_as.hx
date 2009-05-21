// Rectangle_as.hx:  ActionScript 3 "Rectangle" class, for Gnash.
//
// Generated by gen-as3.sh on: 20090515 by "rob". Remove this
// after any hand editing loosing changes.
//
//   Copyright (C) 2009 Free Software Foundation, Inc.
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
//

// This test case must be processed by CPP before compiling to include the
//  DejaGnu.hx header file for the testing framework support.

#if flash9
import flash.geom.Rectangle;
import flash.geom.Point;
import flash.display.MovieClip;
#else
import flash.Rectangle;
import flash.MovieClip;
#end
import flash.Lib;
import Type;

// import our testing API
import DejaGnu;

// Class must be named with the _as suffix, as that's the same name as the file.
class Rectangle_as {
    static function main() {
        var x1:Rectangle = new Rectangle();
        var p1:Point = new Point();

        // Make sure we actually get a valid class        
        if (x1 != null) {
            DejaGnu.pass("Rectangle class exists");
        } else {
            DejaGnu.fail("Rectangle class doesn't exist");
        }
// Tests to see if all the properties exist. All these do is test for
// existance of a property, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (x1.bottom == 0) {
	    DejaGnu.pass("Rectangle.bottom property exists");
	} else {
	    DejaGnu.fail("Rectangle.bottom property doesn't exist");
	}
// 	if (x1.bottomRight == Point) {
// 	    DejaGnu.pass("Rectangle.bottomRight property exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle.bottomRight property doesn't exist");
// 	}
	if (x1.height == 0) {
	    DejaGnu.pass("Rectangle.height property exists");
	} else {
	    DejaGnu.fail("Rectangle.height property doesn't exist");
	}
	if (x1.left == 0) {
	    DejaGnu.pass("Rectangle.left property exists");
	} else {
	    DejaGnu.fail("Rectangle.left property doesn't exist");
	}
	if (x1.right == 0) {
	    DejaGnu.pass("Rectangle.right property exists");
	} else {
	    DejaGnu.fail("Rectangle.right property doesn't exist");
	}
// 	if (x1.size == Point) {
// 	    DejaGnu.pass("Rectangle.size property exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle.size property doesn't exist");
// 	}
	if (x1.top == 0) {
	    DejaGnu.pass("Rectangle.top property exists");
	} else {
	    DejaGnu.fail("Rectangle.top property doesn't exist");
	}
// 	if (x1.topLeft == Point) {
// 	    DejaGnu.pass("Rectangle.topLeft property exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle.topLeft property doesn't exist");
// 	}
	if (x1.width == 0) {
	    DejaGnu.pass("Rectangle.width property exists");
	} else {
	    DejaGnu.fail("Rectangle.width property doesn't exist");
	}
	if (x1.x == 0) {
	    DejaGnu.pass("Rectangle.x property exists");
	} else {
	    DejaGnu.fail("Rectangle.x property doesn't exist");
	}
	if (x1.y == 0) {
	    DejaGnu.pass("Rectangle.y property exists");
	} else {
	    DejaGnu.fail("Rectangle.y property doesn't exist");
	}

// Tests to see if all the methods exist. All these do is test for
// existance of a method, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
// 	if (x1.Rectangle == 0) {
// 	    DejaGnu.pass("Rectangle::Rectangle() method exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle::Rectangle() method doesn't exist");
// 	}
// 	if (x1.clone == Rectangle) {
// 	    DejaGnu.pass("Rectangle::clone() method exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle::clone() method doesn't exist");
// 	}
	if (x1.contains(1,0) == false) {
	    DejaGnu.pass("Rectangle::contains() method exists");
	} else {
	    DejaGnu.fail("Rectangle::contains() method doesn't exist");
	}
	
	if (x1.containsPoint(p1) == false) {
	    DejaGnu.pass("Rectangle::containsPoint() method exists");
	} else {
	    DejaGnu.fail("Rectangle::containsPoint() method doesn't exist");
	}
	if (x1.containsRect(x1) == false) {
	    DejaGnu.pass("Rectangle::containsRect() method exists");
	} else {
	    DejaGnu.fail("Rectangle::containsRect() method doesn't exist");
	}
// 	if (x1.equals == false) {
// 	    DejaGnu.pass("Rectangle::equals() method exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle::equals() method doesn't exist");
// 	}
	if (x1.inflate == null) {
	    DejaGnu.pass("Rectangle::inflate() method exists");
	} else {
	    DejaGnu.fail("Rectangle::inflate() method doesn't exist");
	}
	if (x1.inflatePoint == null) {
	    DejaGnu.pass("Rectangle::inflatePoint() method exists");
	} else {
	    DejaGnu.fail("Rectangle::inflatePoint() method doesn't exist");
	}
// 	if (x1.intersection == Rectangle) {
// 	    DejaGnu.pass("Rectangle::intersection() method exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle::intersection() method doesn't exist");
// 	}
	if (x1.intersects(x1) == false) {
	    DejaGnu.pass("Rectangle::intersects() method exists");
	} else {
	    DejaGnu.fail("Rectangle::intersects() method doesn't exist");
	}
	if (x1.isEmpty() == false) {
	    DejaGnu.pass("Rectangle::isEmpty() method exists");
	} else {
	    DejaGnu.fail("Rectangle::isEmpty() method doesn't exist");
	}
	if (x1.offset == null) {
	    DejaGnu.pass("Rectangle::offset() method exists");
	} else {
	    DejaGnu.fail("Rectangle::offset() method doesn't exist");
	}
	if (x1.offsetPoint == null) {
	    DejaGnu.pass("Rectangle::offsetPoint() method exists");
	} else {
	    DejaGnu.fail("Rectangle::offsetPoint() method doesn't exist");
	}
	if (x1.setEmpty == null) {
	    DejaGnu.pass("Rectangle::setEmpty() method exists");
	} else {
	    DejaGnu.fail("Rectangle::setEmpty() method doesn't exist");
	}
	if (x1.toString == null) {
	    DejaGnu.pass("Rectangle::toString() method exists");
	} else {
	    DejaGnu.fail("Rectangle::toString() method doesn't exist");
	}
// 	if (x1.union == Rectangle) {
// 	    DejaGnu.pass("Rectangle::union() method exists");
// 	} else {
// 	    DejaGnu.fail("Rectangle::union() method doesn't exist");
// 	}

        // Call this after finishing all tests. It prints out the totals.
        DejaGnu.done();
    }
}

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

