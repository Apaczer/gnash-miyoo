// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
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
#include "config.h"
#endif

#include "SharedObject.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function

namespace gnash {

void sharedobject_clear(const fn_call& fn);
void sharedobject_flush(const fn_call& fn);
void sharedobject_getlocal(const fn_call& fn);
void sharedobject_getsize(const fn_call& fn);
void sharedobject_ctor(const fn_call& fn);

static void
attachSharedObjectInterface(as_object& o)
{
	// TODO: clear, flush and getSize not in SWF<6 , it seems
	o.init_member("clear", new builtin_function(sharedobject_clear));
	o.init_member("flush", new builtin_function(sharedobject_flush));
	o.init_member("getLocal", new builtin_function(sharedobject_getlocal));
	o.init_member("getSize", new builtin_function(sharedobject_getsize));
}

static as_object*
getSharedObjectInterface()
{
	static boost::intrusive_ptr<as_object> o;
	if ( ! o )
	{
		o = new as_object();
		attachSharedObjectInterface(*o);
	}
	return o.get();
}

class sharedobject_as_object: public as_object
{

public:

	sharedobject_as_object()
		:
		as_object(getSharedObjectInterface())
	{}

	// override from as_object ?
	//const char* get_text_value() const { return "SharedObject"; }

	// override from as_object ?
	//double get_numeric_value() const { return 0; }
};

void sharedobject_clear(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void sharedobject_flush(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void sharedobject_getlocal(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void sharedobject_getsize(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}

void
sharedobject_ctor(const fn_call& fn)
{
	boost::intrusive_ptr<as_object> obj = new sharedobject_as_object;
	
	fn.result->set_as_object(obj.get()); // will keep alive
}

// extern (used by Global.cpp)
void sharedobject_class_init(as_object& global)
{
	// This is going to be the global SharedObject "class"/"function"
	static boost::intrusive_ptr<builtin_function> cl;

	if ( cl == NULL )
	{
		cl=new builtin_function(&sharedobject_ctor, getSharedObjectInterface());
		// replicate all interface to class, to be able to access
		// all methods as static functions
		attachSharedObjectInterface(*cl);
		     
	}

	// Register _global.SharedObject
	global.init_member("SharedObject", cl.get());

}


} // end of gnash namespace

