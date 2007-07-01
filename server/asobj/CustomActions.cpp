// CustomActions.cpp:  ActionScript CustomActions class, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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
#include "config.h"
#endif

#include "CustomActions.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function

namespace gnash {

as_value customactions_get(const fn_call& fn);
as_value customactions_install(const fn_call& fn);
as_value customactions_list(const fn_call& fn);
as_value customactions_uninstall(const fn_call& fn);
as_value customactions_ctor(const fn_call& fn);

static void
attachCustomActionsInterface(as_object& o)
{
	o.init_member("get", new builtin_function(customactions_get));
	o.init_member("install", new builtin_function(customactions_install));
	o.init_member("list", new builtin_function(customactions_list));
	o.init_member("uninstall", new builtin_function(customactions_uninstall));
}

static as_object*
getCustomActionsInterface()
{
	static boost::intrusive_ptr<as_object> o;
	if ( ! o )
	{
		o = new as_object();
		attachCustomActionsInterface(*o);
	}
	return o.get();
}

class customactions_as_object: public as_object
{

public:

	customactions_as_object()
		:
		as_object(getCustomActionsInterface())
	{}

	// override from as_object ?
	//std::string get_text_value() const { return "CustomActions"; }

	// override from as_object ?
	//double get_numeric_value() const { return 0; }
};

as_value customactions_get(const fn_call& /*fn*/) {
    log_unimpl (__FUNCTION__);
    return as_value();
}
as_value customactions_install(const fn_call& /*fn*/) {
    log_unimpl (__FUNCTION__);
    return as_value();
}
as_value customactions_list(const fn_call& /*fn*/) {
    log_unimpl (__FUNCTION__);
    return as_value();
}
as_value customactions_uninstall(const fn_call& /*fn*/) {
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
customactions_ctor(const fn_call& /* fn */)
{
	boost::intrusive_ptr<as_object> obj = new customactions_as_object;
	
	return as_value(obj.get()); // will keep alive
}

// extern (used by Global.cpp)
void customactions_class_init(as_object& global)
{
	// This is going to be the global CustomActions "class"/"function"
	static boost::intrusive_ptr<builtin_function> cl;

	if ( cl == NULL )
	{
		cl=new builtin_function(&customactions_ctor, getCustomActionsInterface());
		// replicate all interface to class, to be able to access
		// all methods as static functions
		attachCustomActionsInterface(*cl);
		     
	}

	// Register _global.CustomActions
	global.init_member("CustomActions", cl.get());

}


} // end of gnash namespace

