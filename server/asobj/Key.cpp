// Key.cpp:  ActionScript "Key" class (keyboards), for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "smart_ptr.h" // GNASH_USE_GC
#include "log.h"
#include "Key.h"
#include "fn_call.h"
#include "movie_root.h"
#include "action.h" // for call_method
#include "VM.h" // for registerNative
#include "builtin_function.h"
#include "Object.h" // for getObjectInterface()
#include "AsBroadcaster.h" // for initializing self as a broadcaster
#include "namedStrings.h"

namespace gnash {

/************************************************************************
*
* This has been moved from action.cpp, when things are clean
* everything should have been moved up
*
************************************************************************/

key_as_object::key_as_object()
    :
    as_object(getObjectInterface()),
    _unreleasedKeys(0),
    _lastKeyEvent(0)
{
    // Key is a broadcaster only in SWF6 and up (correct?)
    int swfversion = _vm.getSWFVersion();
    if ( swfversion > 5 )
    {
        AsBroadcaster::initialize(*this);
    }
}

bool
key_as_object::is_key_down(int keycode)
{
    if (keycode < 0 || keycode >= key::KEYCOUNT) return false;
    if (_unreleasedKeys.test(keycode)) return true;
    return false;
}

void
key_as_object::set_key_down(key::code code)
{
    if (code >= key::KEYCOUNT) return;

    // This is used for getAscii() of the last key event, so we store
    // the unique gnash::key::code.
    _lastKeyEvent = code;

    // Key.isDown() only cares about flash keycode, not character, so
    // we lookup keycode to add to _unreleasedKeys.   
    size_t keycode = key::codeMap[code][key::KEY];

    assert(keycode < _unreleasedKeys.size());

    _unreleasedKeys.set(keycode, 1);
}

void
key_as_object::set_key_up(key::code code)
{
    if (code >= key::KEYCOUNT) return;

    // This is used for getAscii() of the last key event, so we store
    // the unique gnash::key::code.    
    _lastKeyEvent = code;

    // Key.isDown() only cares about flash keycode, not character, so
    // we lookup keycode to add to _unreleasedKeys.
    size_t keycode = key::codeMap[code][key::KEY];

    assert(keycode < _unreleasedKeys.size());

    _unreleasedKeys.set(keycode, 0);
}


void 
key_as_object::notify_listeners(const event_id& key_event)
{  
    // There is no user defined "onKeyPress" event handler
    if( (key_event.m_id != event_id::KEY_DOWN) && (key_event.m_id != event_id::KEY_UP) ) return;

    as_value ev(key_event.get_function_name());

    //log_debug("notify_listeners calling broadcastMessage with arg %s", ev.to_debug_string().c_str());
    callMethod(NSV::PROP_BROADCAST_MESSAGE, ev);
}

int
key_as_object::get_last_key() const
{
    return _lastKeyEvent;
}


/// Return the ascii number of the last key pressed.
static as_value   
key_get_ascii(const fn_call& fn)
{
    boost::intrusive_ptr<key_as_object> ko = ensureType<key_as_object>(fn.this_ptr);

    int code = ko->get_last_key();

    return as_value(gnash::key::codeMap[code][key::ASCII]);
}

/// Returns the keycode of the last key pressed.
static as_value   
key_get_code(const fn_call& fn)
{
    boost::intrusive_ptr<key_as_object> ko = ensureType<key_as_object>(fn.this_ptr);

    int code = ko->get_last_key();

    return as_value(key::codeMap[code][key::KEY]);
}

/// Return true if the specified (first arg keycode) key is pressed.
static as_value   
key_is_down(const fn_call& fn)
{
    boost::intrusive_ptr<key_as_object> ko = ensureType<key_as_object>(fn.this_ptr);

    if (fn.nargs < 1)
    {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("Key.isDown needs one argument (the key code)"));
        );
        return as_value();
    }

    int keycode = fn.arg(0).to_number<int>();

    return as_value(ko->is_key_down(keycode));
}

/// \brief
/// Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
/// the associated state is on.
///
static as_value   
key_is_toggled(const fn_call& /* fn */)
{
    log_unimpl("Key.isToggled");
    // @@ TODO
    return as_value(false);
}

void key_class_init(as_object& global)
{

    //  GNASH_REPORT_FUNCTION;
    //

    // Create built-in key object.
    // NOTE: _global.Key *is* an object, not a constructor
    as_object*  key_obj = new key_as_object;

    // constants
#define KEY_CONST(k) key_obj->init_member(#k, key::codeMap[key::k][key::KEY])
    KEY_CONST(BACKSPACE);
    KEY_CONST(CAPSLOCK);
    KEY_CONST(CONTROL);
    KEY_CONST(DELETEKEY);
    KEY_CONST(DOWN);
    KEY_CONST(END);
    KEY_CONST(ENTER);
    KEY_CONST(ESCAPE);
    KEY_CONST(HOME);
    KEY_CONST(INSERT);
    KEY_CONST(LEFT);
    KEY_CONST(PGDN);
    KEY_CONST(PGUP);
    KEY_CONST(RIGHT);
    KEY_CONST(SHIFT);
    KEY_CONST(SPACE);
    KEY_CONST(TAB);
    KEY_CONST(UP);

    // methods

    VM& vm = global.getVM();

    vm.registerNative(key_get_ascii, 800, 0);
    key_obj->init_member("getAscii", vm.getNative(800, 0));

    vm.registerNative(key_get_code, 800, 1);
    key_obj->init_member("getCode", vm.getNative(800, 1));

    vm.registerNative(key_is_down, 800, 2);
    key_obj->init_member("isDown", vm.getNative(800, 2));

    vm.registerNative(key_is_toggled, 800, 3);
    key_obj->init_member("isToggled", vm.getNative(800, 3));

    global.init_member("Key", key_obj);
}

#ifdef GNASH_USE_GC
void
key_as_object::markReachableResources() const
{
    markAsObjectReachable();
    for (Listeners::const_iterator i=_listeners.begin(), e=_listeners.end();
                i != e; ++i)
    {
        (*i)->setReachable();
    }
}
#endif // def GNASH_USE_GC

} // end of gnash namespace

