// as_object.cpp:  ActionScript "DisplayObject" class, for Gnash.
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

#include "display/DisplayObject_as.h"
#include "log.h"
#include "fn_call.h"
#include "Global_as.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function
#include "GnashException.h" // for ActionException

namespace gnash {

// Forward declarations
namespace {
    as_value displayobject_getRect(const fn_call& fn);
    as_value displayobject_globalToLocal(const fn_call& fn);
    as_value displayobject_hitTestObject(const fn_call& fn);
    as_value displayobject_hitTestPoint(const fn_call& fn);
    as_value displayobject_localToGlobal(const fn_call& fn);
    as_value displayobject_added(const fn_call& fn);
    as_value displayobject_addedToStage(const fn_call& fn);
    as_value displayobject_enterFrame(const fn_call& fn);
    as_value displayobject_removed(const fn_call& fn);
    as_value displayobject_removedFromStage(const fn_call& fn);
    as_value displayobject_render(const fn_call& fn);
    as_value displayobject_ctor(const fn_call& fn);
    void attachDisplayObjectInterface(as_object& o);
    void attachDisplayObjectStaticInterface(as_object& o);
}

// extern (used by Global.cpp)
void
displayobject_class_init(as_object& where, const ObjectURI& uri)
{
    Global_as& gl = getGlobal(where);
    as_object* proto = gl.createObject();
    attachDisplayObjectInterface(*proto);
    as_object* cl = gl.createClass(&displayobject_ctor, proto);
    attachDisplayObjectStaticInterface(*cl);

    // Register _global.DisplayObject
    where.init_member(uri, cl, as_object::DefaultFlags);
}

namespace {

void
attachDisplayObjectInterface(as_object& o)
{
    Global_as& gl = getGlobal(o);

    o.init_member("getRect", gl.createFunction(displayobject_getRect));
    o.init_member("globalToLocal", gl.createFunction(displayobject_globalToLocal));
    o.init_member("hitTestObject", gl.createFunction(displayobject_hitTestObject));
    o.init_member("hitTestPoint", gl.createFunction(displayobject_hitTestPoint));
    o.init_member("localToGlobal", gl.createFunction(displayobject_localToGlobal));
    o.init_member("added", gl.createFunction(displayobject_added));
    o.init_member("addedToStage", gl.createFunction(displayobject_addedToStage));
    o.init_member("enterFrame", gl.createFunction(displayobject_enterFrame));
    o.init_member("removed", gl.createFunction(displayobject_removed));
    o.init_member("removedFromStage", gl.createFunction(displayobject_removedFromStage));
    o.init_member("render", gl.createFunction(displayobject_render));
}

void
attachDisplayObjectStaticInterface(as_object& /*o*/)
{
}

as_value
displayobject_getRect(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_globalToLocal(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_hitTestObject(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_hitTestPoint(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_localToGlobal(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_added(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_addedToStage(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_enterFrame(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_removed(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_removedFromStage(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_render(const fn_call& fn)
{
    as_object* ptr = ensure<ValidThis>(fn);
    UNUSED(ptr);
    log_unimpl (__FUNCTION__);
    return as_value();
}

as_value
displayobject_ctor(const fn_call& /*fn*/)
{
    return as_value(); 
}

} // anonymous namespace 
} // gnash namespace

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

