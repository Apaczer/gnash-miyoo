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

#include "TextSnapshot.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function

namespace gnash {

void textsnapshot_findtext(const fn_call& fn);
void textsnapshot_getcount(const fn_call& fn);
void textsnapshot_getselected(const fn_call& fn);
void textsnapshot_getselectedtext(const fn_call& fn);
void textsnapshot_gettext(const fn_call& fn);
void textsnapshot_hittesttextnearpos(const fn_call& fn);
void textsnapshot_setselectcolor(const fn_call& fn);
void textsnapshot_setselected(const fn_call& fn);
void textsnapshot_ctor(const fn_call& fn);

static void
attachTextSnapshotInterface(as_object& o)
{
	// FIXME: check name case of all methods, and only initialize
	//        the ones expected to be found based on SWF version

	o.init_member("findText", new builtin_function(textsnapshot_findtext));
	o.init_member("getCount", new builtin_function(textsnapshot_getcount));
	o.init_member("getSelected", new builtin_function(textsnapshot_getselected));
	o.init_member("getSelectedText", new builtin_function(textsnapshot_getselectedtext));
	o.init_member("getText", new builtin_function(textsnapshot_gettext));
	o.init_member("hitTestTextNearPos", new builtin_function(textsnapshot_hittesttextnearpos));
	o.init_member("setSelectColor", new builtin_function(textsnapshot_setselectcolor));
	o.init_member("setSelected", new builtin_function(textsnapshot_setselected));
}

static as_object*
getTextSnapshotInterface()
{
	static boost::intrusive_ptr<as_object> o;
	if ( ! o )
	{
		o = new as_object();
		attachTextSnapshotInterface(*o);
	}
	return o.get();
}

class textsnapshot_as_object: public as_object
{

public:

	textsnapshot_as_object()
		:
		as_object(getTextSnapshotInterface())
	{}

	// override from as_object ?
	//const char* get_text_value() const { return "TextSnapshot"; }

	// override from as_object ?
	//double get_numeric_value() const { return 0; }
};

void textsnapshot_findtext(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_getcount(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_getselected(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_getselectedtext(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_gettext(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_hittesttextnearpos(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_setselectcolor(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}
void textsnapshot_setselected(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
}

void
textsnapshot_ctor(const fn_call& fn)
{
	boost::intrusive_ptr<as_object> obj = new textsnapshot_as_object;
	
	fn.result->set_as_object(obj.get()); // will keep alive
}

// extern (used by Global.cpp)
void textsnapshot_class_init(as_object& global)
{
	// This is going to be the global TextSnapshot "class"/"function"
	static boost::intrusive_ptr<builtin_function> cl;

	if ( cl == NULL )
	{
		cl=new builtin_function(&textsnapshot_ctor, getTextSnapshotInterface());
		// replicate all interface to class, to be able to access
		// all methods as static functions
		attachTextSnapshotInterface(*cl);
		     
	}

	// Register _global.TextSnapshot
	global.init_member("TextSnapshot", cl.get());

}


} // end of gnash namespace

