// Global.cpp:  Global ActionScript class setup, for Gnash.
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

#include "as_object.h"
#include "as_prop_flags.h"
#include "as_value.h"
#include "as_function.h" // for function_class_init
#include "array.h"
#include "AsBroadcaster.h"
#include "Boolean.h"
#include "Camera.h"
#include "Color.h"
#include "ContextMenu.h"
#include "CustomActions.h"
#include "Date.h"
#include "Error.h"
#include "Global.h"
#include "gstring.h"
#include "Key.h"
#include "LoadVars.h"
#include "LocalConnection.h"
#include "Microphone.h"
#include "Number.h"
#include "Object.h"
#include "GMath.h"
#include "Mouse.h"
#include "MovieClipLoader.h"
#include "movie_definition.h"
#include "NetConnection.h"
#include "NetStream.h"
#include "Selection.h"
#include "SharedObject.h"
#include "Sound.h"
#include "Stage.h"
#include "System.h"
#include "TextFormat.h"
#include "TextSnapshot.h"
#include "video_stream_instance.h"
#include "extension.h"
#include "VM.h"
#include "timers.h"
#include "URL.h" // for URL::encode and URL::decode (escape/unescape)
#include "builtin_function.h"
#include "edit_text_character.h"
#include "rc.h"
#include "ClassHierarchy.h"
#include "namedStrings.h"

#include "fn_call.h"
#include "sprite_instance.h"

#include "xml.h"
#include "xmlsocket.h"

#include <limits> // for numeric_limits<double>::quiet_NaN
#include <sstream>

// Common code to warn and return if a required single arg is not present
// and to warn if there are extra args.
#define ASSERT_FN_ARGS_IS_1						\
    if (fn.nargs < 1) {							\
	IF_VERBOSE_ASCODING_ERRORS(					\
            log_aserror(_("%s needs one argument"), __FUNCTION__);		\
            )								\
         return as_value();							\
    }									\
    IF_VERBOSE_ASCODING_ERRORS(						\
	if (fn.nargs > 1)						\
            log_aserror(_("%s has more than one argument"), __FUNCTION__);	\
    )

using namespace std;

namespace gnash {

static as_value
as_global_trace(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    // Log our argument.
    //
    // @@ what if we get extra args?
    //
    // @@ Nothing needs special treatment,
    //    as_value::to_string() will take care of everything
    const char* arg0 = fn.arg(0).to_string().c_str();
    log_trace("%s", arg0);
    return as_value();
}


static as_value
as_global_isnan(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    return as_value( static_cast<bool>(isnan(fn.arg(0).to_number()) ));
}

static as_value
as_global_isfinite(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    return as_value( (bool)isfinite(fn.arg(0).to_number()) );
}

/// \brief Encode a string to URL-encoded format
/// converting all dodgy characters to %AB hex sequences
//
/// Characters that need escaping are:
/// - ASCII control characters: 0-31 and 127
/// - Non-ASCII chars: 128-255
/// - URL syntax characters: $ & + , / : ; = ? @
/// - Unsafe characters: SPACE " < > # % { } | \ ^ ~ [ ] `
/// Encoding is a % followed by two hexadecimal characters, case insensitive.
/// See RFC1738 http://www.rfc-editor.org/rfc/rfc1738.txt,
/// Section 2.2 "URL Character Encoding Issues"

static as_value
as_global_escape(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    string input = fn.arg(0).to_string();
    URL::encode(input);
    return as_value(input.c_str());
}

/// \brief Decode a string from URL-encoded format
/// converting all hexadecimal sequences to ASCII characters.
//
/// A sequence to convert is % followed by two case-independent hexadecimal
/// digits, which is replaced by the equivalent ASCII character.
/// See RFC1738 http://www.rfc-editor.org/rfc/rfc1738.txt,
/// Section 2.2 "URL Character Encoding Issues"

static as_value
as_global_unescape(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    string input = fn.arg(0).to_string();
    URL::decode(input);
    return as_value(input.c_str());
}

// parseFloat (string)
static as_value
as_global_parsefloat(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    as_value rv;
    double result;
    
    std::istringstream s(fn.arg(0).to_string());
    
    if ( ! (s >> result)  )
    {
        rv.set_nan();
        return rv;   
    }    

    rv = result;
    return rv;
}

// parseInt(string[, base])
// The second argument, if supplied, is the base.
// If none is supplied, we have to work out the 
// base from the string. Decimal, octal and hexadecimal are
// possible, according to the following rules:
// 1. If the string starts with 0x or 0X, the number is hex.
// 2. The 0x or 0X may be *followed* by '-' or '+' to indicate sign. A number
//    with no sign is positive.
// 3. If the string starts with 0, -0 or +0 and contains only the characters
//    0-7.
// 4. If the string starts with *any* other sequence of characters, including
//    whitespace, it is decimal.
static as_value
as_global_parseint(const fn_call& fn)
{
    // assert(fn.nargs == 2 || fn.nargs == 1);
    if (fn.nargs < 1) {
	IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("%s needs at least one argument"), __FUNCTION__);
            )
         return as_value();
    }
    IF_VERBOSE_ASCODING_ERRORS(
	if (fn.nargs > 2)
            log_aserror(_("%s has more than two arguments"), __FUNCTION__);
    )

    const std::string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    const std::string& expr = fn.arg(0).to_string();

    bool negative = false;
    int base = 0;

    std::string::const_iterator it = expr.begin();

    // Try hexadecimal first
    if (expr.substr(0, 2) == "0x" || expr.substr(0, 2) == "0X")
    {
        base = 16;
        it += 2;
        
        if (*it == '-')
        {
            negative = true;
            it++;
        }
        else if (*it == '+') 
        {
            it++;
        }
    }
    // Either octal or decimal.
    else if (*it == '0' || *it == '-' || *it == '+')
    {

        base = 8;

        // Check for negative and move to the next digit
        if (*it == '-')
        {
            negative = true;
            it++;
        }
        else if (*it == '+') it++;
        
        if (*it != '0') base = 10;
        
        // Check for expectional case "-0x" or "+0x", which
        // return NaN
        else if (std::toupper(*(it + 1)) == 'X')
        {
            as_value rv;
            rv.set_nan();
            return rv;
        }
        
        // Check from the current position for non-octal characters;
        // it's decimal in that case.
        else if (expr.find_first_not_of("01234567", it - expr.begin()) !=
        	std::string::npos)
        {
            base = 10;
        }
    }
    // Everything else is decimal.
    else
    {
        base = 10;
        
        // Skip leading whitespace
        while(*it == ' ' || *it == '\n' || *it == '\t' || *it == '\r')
        {
            ++it;
        }
        if (*it == '-')
        {
            negative = true;
            it++;
        }
        else if (*it == '+') it++;
    }    

	// After all that, a second argument specifies the base.
	// Parsing still starts after any positive/negative 
	// sign or hex identifier (parseInt("0x123", 8) gives
	// 83, not 0; parseInt(" 0x123", 8) is 0), which is
	// why we do this here.
    if (fn.nargs > 1)
    {
	    base = (fn.arg(1).to_int());
	
	    // Bases from 2 to 36 are valid, otherwise return NaN
            if (base < 2 || base > 36)
            {
	            as_value rv;
	            rv.set_nan();
	            return rv;
            }
          
    }
    
    // Now we have the base, parse the digits. The iterator should
    // be pointing at the first digit.
    
    // Check to see if the first digit is valid, otherwise 
    // return NaN.
    int digit = digits.find(toupper(*it));

    if (digit >= base || digit < 0)
    {
	    as_value rv;
	    rv.set_nan();
	    return rv;
    }

    // The first digit was valid, so continue from the present position
    // until we reach the end of the string or an invalid character,
    // adding valid characters to our result.
    // Which characters are invalid depends on the base. 
    double result = digit;
    ++it;
    
    while (it != expr.end() && (digit = digits.find(toupper(*it))) < base
    		&& digit >= 0)
    {
	    result = result * base + digit;
	    ++it;
    }

    if (negative)
	result = -result;
    
    // Now return the parsed string as an integer.
    return as_value(result);
}

// ASSetPropFlags function
static as_value
as_global_assetpropflags(const fn_call& fn)
{
    //int version = VM::get().getSWFVersion();

    //log_debug(_("ASSetPropFlags called with %d args"), fn.nargs);

    // Check the arguments
    // assert(fn.nargs == 3 || fn.nargs == 4);
    // assert((version == 5) ? (fn.nargs == 3) : true);

    if (fn.nargs < 3) {
	IF_VERBOSE_ASCODING_ERRORS(	
            log_aserror(_("%s needs at least three arguments"), __FUNCTION__);
            )
         return as_value();
    }
    IF_VERBOSE_ASCODING_ERRORS(
	if (fn.nargs > 4)
            log_aserror(_("%s has more than four arguments"), __FUNCTION__);
#if 0 // it is perfectly legal to have 4 args in SWF5 it seems..
	if (version == 5 && fn.nargs == 4)
            log_aserror(_("%s has four arguments in a SWF version 5 movie"), __FUNCTION__);
#endif
    )
		
    // ASSetPropFlags(obj, props, n, allowFalse=false)

    // object
    boost::intrusive_ptr<as_object> obj = fn.arg(0).to_object();
    if ( ! obj )
    {
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("Invalid call to ASSetPropFlags: "
			"first argument is not an object: %s"),
			fn.arg(0).to_debug_string().c_str());
		);
		return as_value();
    }

    // list of child names

    as_value& props = fn.arg(1);

    // a number which represents three bitwise flags which
    // are used to determine whether the list of child names should be hidden,
    // un-hidden, protected from over-write, un-protected from over-write,
    // protected from deletion and un-protected from deletion
    int set_true = int(fn.arg(2).to_number()) & as_prop_flags::as_prop_flags_mask;

    // Is another integer bitmask that works like set_true,
    // except it sets the attributes to false. The
    // set_false bitmask is applied before set_true is applied

    // ASSetPropFlags was exposed in Flash 5, however the fourth argument 'set_false'
    // was not required as it always defaulted to the value '~0'. 
    int set_false = (fn.nargs < 4 ? 0 : int(fn.arg(3).to_number()))
	& as_prop_flags::as_prop_flags_mask;

    obj->setPropFlags(props, set_false, set_true);

    return as_value();
}

// ASNative function
// See: http://osflash.org/flashcoders/undocumented/asnative?s=asnative
static as_value
as_global_asnative(const fn_call& fn)
{
	as_value ret;

	if (fn.nargs < 2)
	{
		IF_VERBOSE_ASCODING_ERRORS(	
		log_aserror(_("ASNative(%s): needs at least two arguments"), fn.dump_args().c_str());
		)
		return ret;
	}

	const int sx = fn.arg(0).to_int();
	const int sy = fn.arg(1).to_int();

	if ( sx < 0 )
	{
		IF_VERBOSE_ASCODING_ERRORS(	
		log_aserror(_("ASNative(%s): first arg must be >= 0"), fn.dump_args().c_str());
		)
		return ret;
	}
	if ( sy < 0 )
	{
		IF_VERBOSE_ASCODING_ERRORS(	
		log_aserror(_("ASNative(%s): second arg must be >= 0"), fn.dump_args().c_str());
		)
		return ret;
	}

	const unsigned int x = static_cast<unsigned int>(sx);
	const unsigned int y = static_cast<unsigned int>(sy);

	VM& vm = VM::get();
	as_function* fun = vm.getNative(x, y);
	if ( ! fun ) {
	    log_debug(_("No ASnative(%d, %d) registered with the VM"), x, y);
	    return ret;
	}
	ret.set_as_function(fun);
	return ret;
		
}

// ASSetNative function
// TODO: find dox 
static as_value
as_global_assetnative(const fn_call& /*fn*/)
{
	log_unimpl("ASSetNative");
	return as_value();
}

// ASSetNativeAccessor function
// TODO: find dox 
static as_value
as_global_assetnativeaccessor(const fn_call& /*fn*/)
{
	log_unimpl("ASSetNativeAccessor");
	return as_value();
}

// ASconstructor function
// TODO: find dox 
static as_value
as_global_asconstructor(const fn_call& /*fn*/)
{
	log_unimpl("ASconstructor");
	return as_value();
}

// updateAfterEvent function
static as_value
as_global_updateAfterEvent(const fn_call& /*fn*/)
{
	static bool warned=false;
	if ( ! warned )
	{
		log_unimpl("updateAfterEvent()");
		warned=true;
	}
	return as_value();
}

Global::Global(VM& vm, ClassHierarchy *ch)
	:
	as_object()
{

	//-------------------------------------------------
	// Unclassified - TODO: move to appropriate section
	//
	// WARNING: this approach seems to be bogus, in 
	//          that the proprietary player seems to 
	//          always provide all the core classes it
	//          supports, reguardless of target SWF version.
	//          The only difference seems to be in actual
	//          usability of them. For example some will
	//          be available [ typeof(Name) == 'function' ]
	//          but not instanciatable.
	//-------------------------------------------------

	// No idea why, but it seems there's a NULL _global.o 
	// defined at player startup...
	// Probably due to the AS-based initialization 
	// Not enumerable but overridable and deletable.
	//
	as_value nullVal; nullVal.set_null();
	init_member("o", nullVal, as_prop_flags::dontEnum);

	// ASSetPropFlags
	vm.registerNative(as_global_assetpropflags, 1, 0);
	init_member("ASSetPropFlags", vm.getNative(1, 0));

	// ASnative
	init_member("ASnative", new builtin_function(as_global_asnative));

	// ASSetNative
	init_member("ASSetNative", new builtin_function(as_global_assetnative));

	// ASSetNativeAccessor
	init_member("ASSetNativeAccessor", new builtin_function(as_global_assetnativeaccessor));

	// ASconstructor
	init_member("ASconstructor", new builtin_function(as_global_asconstructor));

	// updateAfterEvent
	init_member("updateAfterEvent", new builtin_function(as_global_updateAfterEvent));

	// Defined in timers.h
	init_member("setInterval", new builtin_function(timer_setinterval));
	init_member("clearInterval", new builtin_function(timer_clearinterval));
	init_member("setTimeout", new builtin_function(timer_settimeout));
	init_member("clearTimeout", new builtin_function(timer_clearinterval));

	ch->setGlobal(this);

// If extensions aren't used, then no extensions will be loaded.
#ifdef USE_EXTENSIONS
	ch->setExtension(&et);
#endif

	ch->massDeclare(vm.getSWFVersion());

	if (vm.getSWFVersion() >= 5)
	{
		object_class_init(*this);
		ch->getGlobalNs()->stubPrototype(NSV::CLASS_OBJECT);
		ch->getGlobalNs()->getClass(NSV::CLASS_OBJECT)->setDeclared();
		array_class_init(*this);
		ch->getGlobalNs()->stubPrototype(NSV::CLASS_ARRAY);
		ch->getGlobalNs()->getClass(NSV::CLASS_ARRAY)->setDeclared();
		string_class_init(*this);
		ch->getGlobalNs()->stubPrototype(NSV::CLASS_STRING);
		ch->getGlobalNs()->getClass(NSV::CLASS_STRING)->setDeclared();
	}
	if (vm.getSWFVersion() >= 6)
	{
		function_class_init(*this);
		ch->getGlobalNs()->stubPrototype(NSV::CLASS_FUNCTION);
		ch->getGlobalNs()->getClass(NSV::CLASS_FUNCTION)->setDeclared();
	}
	
	if ( vm.getSWFVersion() < 3 ) goto extscan;
	//-----------------------
	// SWF3
	//-----------------------

	if ( vm.getSWFVersion() < 4 ) goto extscan;
	//-----------------------
	// SWF4
	//-----------------------

    // The Math class was available from SWF4. It
    // is initialized on demand, and the native
    // functions *must* be registered before this.
	registerMathNative(*this);
	
	// TODO: When should these be registered?
	registerSystemNative(*this);

    vm.registerNative(as_global_trace, 100, 4);
	init_member("trace", vm.getNative(100, 4));

	if ( vm.getSWFVersion() < 5 ) goto extscan;
	//-----------------------
	// SWF5
	//-----------------------

    vm.registerNative(as_global_escape, 100, 0);
	init_member("escape", vm.getNative(100, 0));
	vm.registerNative(as_global_unescape, 100, 1);
	init_member("unescape", vm.getNative(100, 1));
    vm.registerNative(as_global_parseint, 100, 2);
	init_member("parseInt", vm.getNative(100, 2));
    vm.registerNative(as_global_parsefloat, 100, 3);
	init_member("parseFloat", vm.getNative(100, 3));
	
	vm.registerNative(as_global_isnan, 200, 18);
	init_member("isNaN", vm.getNative(200, 18));
	vm.registerNative(as_global_isfinite, 200, 19);
	init_member("isFinite", vm.getNative(200, 19));

	// NaN and Infinity should only be in _global since SWF6,
	// but this is just because SWF5 or lower did not have a "_global"
	// reference at all, most likely.
	init_member("NaN", as_value(NAN));
	init_member("Infinity", as_value(INFINITY));

    // The following initializations are necessary
    // to register ASnative functions
	color_class_init(*this);
	textformat_class_init(*this);

    registerDateNative(*this);
    registerMouseNative(*this);

	if ( vm.getSWFVersion() < 6 ) goto extscan;
	//-----------------------
	// SWF6
	//-----------------------
	init_member("LocalConnection", new builtin_function(localconnection_new));

	if ( vm.getSWFVersion() < 7 ) goto extscan;
	//-----------------------
	// SWF7
	//-----------------------

	if ( vm.getSWFVersion() < 8 ) goto extscan;
	//-----------------------
	// SWF8
	//-----------------------

extscan: 

	//-----------------------
	// Extensions
	//-----------------------
        // Scan the plugin directories for all plugins, and load them now.
        // FIXME: this should actually be done dynamically, and only
        // if a plugin defines a class that a movie actually wants to
        // use.
#ifdef USE_EXTENSIONS

	if ( RcInitFile::getDefaultInstance().enableExtensions() )
	{
		log_security(_("Extensions enabled, scanning plugin dir for load"));
		//et.scanDir("/usr/local/lib/gnash/plugins");
		et.scanAndLoad(*this);
	}
	else
	{
		log_security(_("Extensions disabled"));
	}
#else
	return;
#endif
}

} // namespace gnash
