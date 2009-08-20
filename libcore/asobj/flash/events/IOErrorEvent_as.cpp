// IOErrorEvent_as.cpp:  ActionScript "IOErrorEvent" class, for Gnash.
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

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "events/IOErrorEvent_as.h"
#include "log.h"
#include "fn_call.h"
#include "Global_as.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function
#include "GnashException.h" // for ActionException

namespace gnash {

// Forward declarations
namespace {
    as_value ioerrorevent_toString(const fn_call& fn);
    as_value ioerrorevent_IO_ERROR(const fn_call& fn);
    as_value ioerrorevent_ctor(const fn_call& fn);
    void attachIOErrorEventInterface(as_object& o);
    void attachIOErrorEventStaticInterface(as_object& o);
    as_object* getIOErrorEventInterface();

}

// extern (used by Global.cpp)
void
ioerrorevent_class_init(as_object& where, const ObjectURI& uri)
{
    registerBuiltinClass(where, ioerrorevent_ctor, attachIOErrorEventInterface, 
        attachIOErrorEventStaticInterface, uri);
}

namespace {

void
attachIOErrorEventInterface(as_object& o)
{
    Global_as* gl = getGlobal(o);
    o.init_member("toString", gl->createFunction(ioerrorevent_toString));
    o.init_member("IO_ERROR", gl->createFunction(ioerrorevent_IO_ERROR));
}

void
attachIOErrorEventStaticInterface(as_object& /*o*/)
{
}

as_object*
getIOErrorEventInterface()
{
    static boost::intrusive_ptr<as_object> o;
    if ( ! o ) {
        o = new as_object();
        attachIOErrorEventInterface(*o);
    }
    return o.get();
}

as_value
ioerrorevent_toString(const fn_call& fn)
{
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
ioerrorevent_IO_ERROR(const fn_call& fn)
{
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
ioerrorevent_ctor(const fn_call& /*fn*/)
{

    return as_value(); // will keep alive
}

} // anonymous namespace 
} // gnash namespace

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

