// Rectangle_as.cpp:  ActionScript "Rectangle" class, for Gnash.
//
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "Rectangle_as.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function
#include "GnashException.h" // for ActionException
#include "Object.h" // for AS inheritance
#include "VM.h" // for addStatics
#include "as_value.h"
#include "namedStrings.h"

#include <sstream>

namespace gnash {

static as_value Rectangle_clone(const fn_call& fn);
static as_value Rectangle_contains(const fn_call& fn);
static as_value Rectangle_containsPoint(const fn_call& fn);
static as_value Rectangle_containsRectangle(const fn_call& fn);
static as_value Rectangle_equals(const fn_call& fn);
static as_value Rectangle_inflate(const fn_call& fn);
static as_value Rectangle_inflatePoint(const fn_call& fn);
static as_value Rectangle_intersection(const fn_call& fn);
static as_value Rectangle_intersects(const fn_call& fn);
static as_value Rectangle_isEmpty(const fn_call& fn);
static as_value Rectangle_offset(const fn_call& fn);
static as_value Rectangle_offsetPoint(const fn_call& fn);
static as_value Rectangle_setEmpty(const fn_call& fn);
static as_value Rectangle_toString(const fn_call& fn);
static as_value Rectangle_union(const fn_call& fn);
static as_value Rectangle_bottom_getset(const fn_call& fn);
static as_value Rectangle_bottomRight_getset(const fn_call& fn);
static as_value Rectangle_left_getset(const fn_call& fn);
static as_value Rectangle_right_getset(const fn_call& fn);
static as_value Rectangle_size_getset(const fn_call& fn);
static as_value Rectangle_top_getset(const fn_call& fn);
static as_value Rectangle_topLeft_getset(const fn_call& fn);


as_value Rectangle_ctor(const fn_call& fn);

static void
attachRectangleInterface(as_object& o)
{
    o.init_member("clone", new builtin_function(Rectangle_clone), 0);
    o.init_member("contains", new builtin_function(Rectangle_contains), 0);
    o.init_member("containsPoint", new builtin_function(Rectangle_containsPoint), 0);
    o.init_member("containsRectangle", new builtin_function(Rectangle_containsRectangle), 0);
    o.init_member("equals", new builtin_function(Rectangle_equals), 0);
    o.init_member("inflate", new builtin_function(Rectangle_inflate), 0);
    o.init_member("inflatePoint", new builtin_function(Rectangle_inflatePoint), 0);
    o.init_member("intersection", new builtin_function(Rectangle_intersection), 0);
    o.init_member("intersects", new builtin_function(Rectangle_intersects), 0);
    o.init_member("isEmpty", new builtin_function(Rectangle_isEmpty), 0);
    o.init_member("offset", new builtin_function(Rectangle_offset), 0);
    o.init_member("offsetPoint", new builtin_function(Rectangle_offsetPoint), 0);
    o.init_member("setEmpty", new builtin_function(Rectangle_setEmpty), 0);
    o.init_member("toString", new builtin_function(Rectangle_toString), 0);
    o.init_member("union", new builtin_function(Rectangle_union), 0);
    o.init_property("bottom", Rectangle_bottom_getset, Rectangle_bottom_getset, 0);
    o.init_property("bottomRight", Rectangle_bottomRight_getset, Rectangle_bottomRight_getset, 0);
    o.init_property("left", Rectangle_left_getset, Rectangle_left_getset, 0);
    o.init_property("right", Rectangle_right_getset, Rectangle_right_getset, 0);
    o.init_property("size", Rectangle_size_getset, Rectangle_size_getset, 0);
    o.init_property("top", Rectangle_top_getset, Rectangle_top_getset, 0);
    o.init_property("topLeft", Rectangle_topLeft_getset, Rectangle_topLeft_getset, 0);
}

static void
attachRectangleStaticProperties(as_object& /*o*/)
{
}

static as_object*
getRectangleInterface()
{
	static boost::intrusive_ptr<as_object> o;

	if ( ! o )
	{
		// TODO: check if this class should inherit from Object
		//       or from a different class
		o = new as_object(getObjectInterface());
		VM::get().addStatic(o.get());

		attachRectangleInterface(*o);

	}

	return o.get();
}

class Rectangle_as: public as_object
{

public:

	Rectangle_as()
		:
		as_object(getRectangleInterface())
	{}

	// override from as_object ?
	//std::string get_text_value() const { return "Rectangle"; }

	// override from as_object ?
	//double get_numeric_value() const { return 0; }
};


static as_value
Rectangle_clone(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_contains(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_containsPoint(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_containsRectangle(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_equals(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_inflate(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_inflatePoint(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_intersection(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_intersects(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_isEmpty(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);

	as_value w;
	ptr->get_member(NSV::PROP_WIDTH, &w);
	if ( w.is_undefined() || w.is_null() ) return as_value(true);

	as_value h;
	ptr->get_member(NSV::PROP_HEIGHT, &h);
	if ( h.is_undefined() || h.is_null() ) return as_value(true);

	double wn = w.to_number();
	if ( ! isfinite(wn) || wn == 0 ) return as_value(true);

	double hn = h.to_number();
	if ( ! isfinite(hn) || hn == 0 ) return as_value(true);

	log_debug("Width: %g, Height: %g", wn, hn);

	return as_value(false);
}

static as_value
Rectangle_offset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_offsetPoint(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_setEmpty(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_toString(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);

	as_value x, y, w, h;

	ptr->get_member(NSV::PROP_X, &x);
	ptr->get_member(NSV::PROP_Y, &y);
	ptr->get_member(NSV::PROP_WIDTH, &w);
	ptr->get_member(NSV::PROP_HEIGHT, &h);

	std::stringstream ss;
	ss << "(x=" << x.to_string()
		<< ", y=" << y.to_string()
		<< ", w=" << w.to_string()
		<< ", h=" << h.to_string()
		 << ")";

	return as_value(ss.str());
}

static as_value
Rectangle_union(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_bottom_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);

	as_value ret;

	if ( ! fn.nargs ) // getter
	{
		as_value height;
		ptr->get_member(NSV::PROP_Y, &ret);
		ptr->get_member(NSV::PROP_HEIGHT, &height);
		ret.newAdd(height);
	}
	else // setter
	{
		as_value y;
		ptr->get_member(NSV::PROP_Y, &y);

		as_value bottom = fn.arg(0);
		as_value newh = bottom.subtract(y);
		ptr->set_member(NSV::PROP_HEIGHT, newh);
	}

	return ret;
}

static as_value
Rectangle_bottomRight_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_left_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);

	as_value ret;

	if ( ! fn.nargs ) // getter
	{
		ptr->get_member(NSV::PROP_X, &ret);
	}
	else // setter
	{
		as_value oldx;
		ptr->get_member(NSV::PROP_X, &oldx);

		as_value newx = fn.arg(0);
		ptr->set_member(NSV::PROP_X, newx);

		as_value w;
		ptr->get_member(NSV::PROP_WIDTH, &w);

		w.newAdd(oldx.subtract(newx));
		ptr->set_member(NSV::PROP_WIDTH, w);
	}

	return ret;
}

static as_value
Rectangle_right_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);

	as_value ret;

	if ( ! fn.nargs ) // getter
	{
		as_value width;
		ptr->get_member(NSV::PROP_X, &ret);
		ptr->get_member(NSV::PROP_WIDTH, &width);
		ret.newAdd(width);
	}
	else // setter
	{
		as_value x;
		ptr->get_member(NSV::PROP_X, &x);

		as_value right = fn.arg(0);
		as_value neww = right.subtract(x);
		ptr->set_member(NSV::PROP_WIDTH, neww);
	}

	return ret;
}

static as_value
Rectangle_size_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}

static as_value
Rectangle_top_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);

	as_value ret;

	if ( ! fn.nargs ) // getter
	{
		ptr->get_member(NSV::PROP_Y, &ret);
	}
	else // setter
	{
		as_value oldy;
		ptr->get_member(NSV::PROP_Y, &oldy);

		as_value newy = fn.arg(0);
		ptr->set_member(NSV::PROP_Y, newy);

		as_value h;
		ptr->get_member(NSV::PROP_HEIGHT, &h);

		h.newAdd(oldy.subtract(newy));
		ptr->set_member(NSV::PROP_HEIGHT, h);
	}

	return ret;
}

static as_value
Rectangle_topLeft_getset(const fn_call& fn)
{
	boost::intrusive_ptr<Rectangle_as> ptr = ensureType<Rectangle_as>(fn.this_ptr);
	UNUSED(ptr);
	LOG_ONCE( log_unimpl (__FUNCTION__) );
	return as_value();
}


as_value
Rectangle_ctor(const fn_call& fn)
{
	boost::intrusive_ptr<as_object> obj = new Rectangle_as;

	as_value x;
	as_value y;
	as_value w;
	as_value h;

	if ( ! fn.nargs )
	{
		x.set_double(0);
		y.set_double(0);
		w.set_double(0);
		h.set_double(0);
	}
	else
	{
		do {
			x = fn.arg(0);
			if ( fn.nargs < 2 ) break;
			y = fn.arg(1);
			if ( fn.nargs < 3 ) break;
			w = fn.arg(2);
			if ( fn.nargs < 4 ) break;
			h = fn.arg(3);
			if ( fn.nargs < 5 ) break;
			IF_VERBOSE_ASCODING_ERRORS(
				std::stringstream ss;
				fn.dump_args(ss);
				log_aserror("flash.geom.Rectangle(%s): %s", ss.str(), _("arguments after the first four discarded"));
			);
		} while(0);
	}

	// TODO: use named strings (we have PROP_X and PROP_Y, lack PROP_WIDTH and PROP_HEIGHT)
	obj->set_member(NSV::PROP_X, x);
	obj->set_member(NSV::PROP_Y, y);
	obj->set_member(NSV::PROP_WIDTH, w);
	obj->set_member(NSV::PROP_HEIGHT, h);

	return as_value(obj.get()); // will keep alive
}

// extern 
void Rectangle_class_init(as_object& where)
{
	// This is going to be the Rectangle "class"/"function"
	// in the 'where' package
	boost::intrusive_ptr<builtin_function> cl;
	cl=new builtin_function(&Rectangle_ctor, getRectangleInterface());
	attachRectangleStaticProperties(*cl);

	// Register _global.Rectangle
	where.init_member("Rectangle", cl.get());
}

} // end of gnash namespace
