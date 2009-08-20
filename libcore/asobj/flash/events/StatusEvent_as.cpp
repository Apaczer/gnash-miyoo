// StatusEvent_as.cpp:  ActionScript "StatusEvent" class, for Gnash.
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

#include "events/StatusEvent_as.h"
#include "log.h"
#include "fn_call.h"
#include "Global_as.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function
#include "GnashException.h" // for ActionException

namespace gnash {

// Forward declarations
namespace {
    as_value statusevent_toString(const fn_call& fn);
    as_value statusevent_STATUS(const fn_call& fn);
    as_value statusevent_ctor(const fn_call& fn);
    void attachStatusEventInterface(as_object& o);
    void attachStatusEventStaticInterface(as_object& o);
    as_object* getStatusEventInterface();

}

class StatusEvent_as : public as_object
{

public:

    StatusEvent_as()
        :
        as_object(getStatusEventInterface())
    {}
};

// extern (used by Global.cpp)
void
statusevent_class_init(as_object& where, const ObjectURI& uri)
{
    registerBuiltinClass(where, statusevent_ctor, attachStatusEventInterface, 
        attachStatusEventStaticInterface, uri);
}

namespace {

void
attachStatusEventInterface(as_object& o)
{
    Global_as* gl = getGlobal(o);
    o.init_member("toString", gl->createFunction(statusevent_toString));
    o.init_member("STATUS", gl->createFunction(statusevent_STATUS));
}

void
attachStatusEventStaticInterface(as_object& /*o*/)
{
}

as_object*
getStatusEventInterface()
{
    static boost::intrusive_ptr<as_object> o;
    if ( ! o ) {
        o = new as_object();
        attachStatusEventInterface(*o);
    }
    return o.get();
}

as_value
statusevent_toString(const fn_call& fn)
{
    boost::intrusive_ptr<StatusEvent_as> ptr =
        ensureType<StatusEvent_as>(fn.this_ptr);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
statusevent_STATUS(const fn_call& fn)
{
    boost::intrusive_ptr<StatusEvent_as> ptr =
        ensureType<StatusEvent_as>(fn.this_ptr);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
statusevent_ctor(const fn_call& /*fn*/)
{
    boost::intrusive_ptr<as_object> obj = new StatusEvent_as;

    return as_value(obj.get()); // will keep alive
}

} // anonymous namespace 
} // gnash namespace

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

