// string.cpp:  ActionScript "String" class, for Gnash.
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


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "smart_ptr.h"
#include "fn_call.h"
#include "as_object.h"
#include "builtin_function.h" // need builtin_function
#include "log.h"
#include "array.h"
#include "as_value.h"
#include "GnashException.h"
#include "VM.h" // for registering static GcResources (constructor and prototype)
#include "Object.h" // for getObjectInterface
#include "namedStrings.h"
#include "utf8.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <algorithm>

#define ENSURE_FN_ARGS(min, max, rv)                                    \
    if (fn.nargs < min) {                                               \
        IF_VERBOSE_ASCODING_ERRORS(                                     \
            log_aserror(_("%s needs one argument"), __FUNCTION__);         \
            )                                                           \
         return as_value(rv);                                           \
    }                                                                   \
    IF_VERBOSE_ASCODING_ERRORS(                                         \
        if (fn.nargs > max)                                             \
            log_aserror(_("%s has more than one argument"), __FUNCTION__); \
    )



namespace gnash
{

// Forward declarations

static as_value string_get_length(const fn_call& fn);
static as_value string_concat(const fn_call& fn);
static as_value string_slice(const fn_call& fn);
static as_value string_split(const fn_call& fn);
static as_value string_last_index_of(const fn_call& fn);
static as_value string_sub_str(const fn_call& fn);
static as_value string_sub_string(const fn_call& fn);
static as_value string_index_of(const fn_call& fn);
static as_value string_from_char_code(const fn_call& fn);
static as_value string_char_code_at(const fn_call& fn);
static as_value string_char_at(const fn_call& fn);
static as_value string_to_upper_case(const fn_call& fn);
static as_value string_to_lower_case(const fn_call& fn);
static as_value string_to_string(const fn_call& fn);
static as_value string_ctor(const fn_call& fn);

static void
attachStringInterface(as_object& o)
{
	VM& vm = o.getVM();

	// TODO fill in the rest

	// ASnative(251, 1) - [String.prototype] valueOf
	vm.registerNative(as_object::tostring_method, 251, 1);
	o.init_member("valueOf", vm.getNative(251, 1));

	// ASnative(251, 2) - [String.prototype] toString
	vm.registerNative(string_to_string, 251, 2);
	o.init_member("toString", vm.getNative(251, 2));

	// ASnative(251, 3) - [String.prototype] toUpperCase
	// TODO: register as ASnative(102, 0) for SWF5 ?
	vm.registerNative(string_to_upper_case, 251, 3);
	o.init_member("toUpperCase", vm.getNative(251, 3));

	// ASnative(251, 4) - [String.prototype] toLowerCase
	// TODO: register as ASnative(102, 1) for SWF5 ?
	vm.registerNative(string_to_lower_case, 251, 4);
	o.init_member("toLowerCase", vm.getNative(251, 4));

	// ASnative(251, 5) - [String.prototype] charAt
	vm.registerNative(string_char_at, 251, 5);
	o.init_member("charAt", vm.getNative(251, 5));

	// ASnative(251, 6) - [String.prototype] charCodeAt
	vm.registerNative(string_char_code_at, 251, 6);
	o.init_member("charCodeAt", vm.getNative(251, 6));

	// ASnative(251, 7) - [String.prototype] concat
	vm.registerNative(string_concat, 251, 7);
	o.init_member("concat", vm.getNative(251, 7));

	// ASnative(251, 8) - [String.prototype] indexOf
	vm.registerNative(string_index_of, 251, 8);
	o.init_member("indexOf", vm.getNative(251, 8));

	// ASnative(251, 9) - [String.prototype] lastIndexOf
	vm.registerNative(string_last_index_of, 251, 9);
	o.init_member("lastIndexOf", vm.getNative(251, 9));

	// ASnative(251, 10) - [String.prototype] slice
	vm.registerNative(string_slice, 251, 10);
	o.init_member("slice", vm.getNative(251, 10));

	// ASnative(251, 11) - [String.prototype] substring
	vm.registerNative(string_sub_string, 251, 11);
	o.init_member("substring", vm.getNative(251, 11));

	// ASnative(251, 12) - [String.prototype] split
	vm.registerNative(string_split, 251, 12);
	o.init_member("split", vm.getNative(251, 12));

	// ASnative(251, 13) - [String.prototype] substr
	vm.registerNative(string_sub_str, 251, 13);
	o.init_member("substr", vm.getNative(251, 13));

	// This isn't advertised as native, so might be a proper property ?
	o.init_readonly_property("length", string_get_length);

}

static as_object*
getStringInterface()
{
    static boost::intrusive_ptr<as_object> o;

    if ( o == NULL )
    {
        o = new as_object(getObjectInterface());
	VM::get().addStatic(o.get());

        attachStringInterface(*o);
    }

    return o.get();
}

class string_as_object : public as_object
{

public:
    string_as_object()
            :
            as_object(getStringInterface())
    {}

    bool useCustomToString() const { return false; }

    std::string get_text_value() const
    {
        return _string;
    }

    as_value get_primitive_value() const

    {
        return as_value(_string);
    }

    std::string& str()

    {
        return _string;
    }

private:
    std::string _string;
};

static as_value
string_get_length(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    std::wstring wstr = utf8::decodeCanonicalString(obj->str(), version);

    return as_value(wstr.size());
}

// all the arguments will be converted to string and concatenated
static as_value
string_concat(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    // Make a copy of our string.
    std::string str = obj->str();

    for (unsigned int i = 0; i < fn.nargs; i++) {
        str += fn.arg(i).to_string();
    }

    return as_value(str);
}


static size_t
validIndex(const std::wstring& subject, int index)
{

    if (index < 0) {
        index = subject.size() + index;
    }

    index = iclamp(index, 0, subject.size());

    return index;
}

// 1st param: start_index, 2nd param: end_index
static as_value
string_slice(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    // Make a copy.
    std::wstring wstr = utf8::decodeCanonicalString(obj->str(), version);

    ENSURE_FN_ARGS(1, 2, as_value());

    size_t start = validIndex(wstr, fn.arg(0).to_int());

    size_t end = wstr.length();

    if (fn.nargs >= 2)
    {
    	end = validIndex(wstr, fn.arg(1).to_int());

    } 

    if (end < start) // move out of if ?
    {
            return as_value("");
    }

    size_t retlen = end - start;

    log_debug("start: "SIZET_FMT", end: "SIZET_FMT", retlen: "SIZET_FMT, start, end, retlen);

    return as_value(utf8::encodeCanonicalString(wstr.substr(start, retlen), version));
}

static as_value
string_split(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj =
        ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    std::wstring wstr = utf8::decodeCanonicalString(obj->str(), version);

    as_value val;

    boost::intrusive_ptr<as_array_object> array(new as_array_object());

    int SWFVersion = VM::get().getSWFVersion();

    if (fn.nargs == 0)
    {
        val.set_std_string(obj->str());
        array->push(val);

        return as_value(array.get());
    }

    const std::wstring& delim = utf8::decodeCanonicalString(fn.arg(0).to_string(), version);

    // SWF5 didn't support multichar or empty delimiter
    if ( SWFVersion < 6 )
    {
	    if ( delim.size() != 1 )
	    {
		    val.set_std_string(obj->str());
		    array->push(val);
		    return as_value(array.get());
	    }
    }

    size_t max = wstr.size();

    if (fn.nargs >= 2)
    {
	int max_in = fn.arg(1).to_int();
	if ( SWFVersion < 6 && max_in < 1 )
	{
		return as_value(array.get());
	}
        max = iclamp((size_t)max_in, 0, wstr.size());
    }

    if ( wstr.empty() )
    {
        val.set_std_string(obj->str());
        array->push(val);

        return as_value(array.get());
    }

    if ( delim.empty() ) {
        for (unsigned i=0; i <max; i++) {
            val.set_std_string(utf8::encodeCanonicalString(wstr.substr(i, 1), version));
            array->push(val);
        }

        return as_value(array.get());
    }

    size_t pos = 0, prevpos = 0;
    size_t num = 0;

    while (num < max) {
        pos = wstr.find(delim, pos);

        if (pos != std::wstring::npos) {
            val.set_std_string(utf8::encodeCanonicalString(
            		wstr.substr(prevpos, pos - prevpos),
            		version));
            array->push(val);
            num++;
            prevpos = pos + delim.size();
            pos++;
        } else {
            val.set_std_string(utf8::encodeCanonicalString(
            		wstr.substr(prevpos),
            		version));
            array->push(val);
            break;
        }
    }

    return as_value(array.get());
}

static as_value
string_last_index_of(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    const std::string& str = obj->str();

    ENSURE_FN_ARGS(1, 2, -1);

    const std::string& toFind = fn.arg(0).to_string();

    int start = str.size();

    if (fn.nargs >= 2) {
        start = fn.arg(1).to_int();
    }
    
    if (start < 0) {
        return as_value(-1);
    }

    size_t found = str.find_last_of(toFind, start);

    if (found == std::string::npos) {
        return as_value(-1);
    }

    return as_value(found - toFind.size() + 1);
}

// String.substr(start[, length]).
// If the second value is absent or undefined, the remainder of the string from
// <start> is returned.
// If start is more than string length or length is 0, empty string is returned.
// If length is negative, the substring is taken from the *end* of the string.
static as_value
string_sub_str(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    std::wstring wstr = utf8::decodeCanonicalString(obj->str(), version);

    ENSURE_FN_ARGS(1, 2, obj->str());

    int start = validIndex(wstr, fn.arg(0).to_int());

    int num = wstr.length();

    if (fn.nargs >= 2 && !fn.arg(1).is_undefined())
    {
        num = fn.arg(1).to_int();
	    if ( num < 0 )
	    {
		    if ( -num <= start ) num = 0;
		    else
		    {
			    num = wstr.length() + num;
			    if ( num < 0 ) return as_value("");
		    }
	    }
    }

    return as_value(utf8::encodeCanonicalString(wstr.substr(start, num), version));
}

// string.substring(start[, end])
// If *either* value is less than 0, 0 is used.
// The values are *then* swapped if end is before start.
// Valid values for the start position are up to string 
// length - 1.
static as_value
string_sub_string(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    const std::wstring& wstr = utf8::decodeCanonicalString(obj->str(), version);

    ENSURE_FN_ARGS(1, 2, obj->str());

    int start = fn.arg(0).to_int();
    int end = wstr.size();

    if (start < 0) {
        start = 0;
    }

    if (static_cast<unsigned>(start) >= wstr.size()) {
        return as_value("");
    }

    if (fn.nargs >= 2) {
        int num = fn.arg(1).to_int();

        if (num < 0) {
            num = 0;
        }

        end = num;
        
        if (end < start) {
            IF_VERBOSE_ASCODING_ERRORS(
                log_aserror(_("string.slice() called with end < start"));
            )
            std::swap (end, start);
        }
    }
    
    if (static_cast<unsigned>(end) > wstr.size()) {
        end = wstr.size();
    }
    
    end -= start;
    //log_debug("Start: %d, End: %d", start, end);

    return as_value(utf8::encodeCanonicalString(wstr.substr(start, end), version));
}

static as_value
string_index_of(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    const std::wstring& wstr = utf8::decodeCanonicalString(obj->str(), version);

    ENSURE_FN_ARGS(1, 2, -1);

    const as_value& tfarg = fn.arg(0); // to find arg
    const std::wstring& toFind = utf8::decodeCanonicalString(tfarg.to_string(), version);

    size_t start = 0;

    if (fn.nargs >= 2)
    {
        const as_value& saval = fn.arg(1); // start arg val
        int start_arg = saval.to_int();
        if ( start_arg > 0 ) start = (size_t) start_arg;
	else
	{
		IF_VERBOSE_ASCODING_ERRORS(
		if ( start_arg < 0 )
		{
			log_aserror("String.indexOf(%s, %s): second argument casts to invalid offset (%d)",
				tfarg.to_debug_string().c_str(),
				saval.to_debug_string().c_str(), start_arg);
		}
		);
	}
    }

    size_t pos = wstr.find(toFind, start);

    if (pos == std::wstring::npos) {
        return as_value(-1);
    }

    return as_value(pos);
}

// String.fromCharCode(code1[, code2[, code3[, code4[, ...]]]])
// Makes a string out of any number of char codes.
// The string is always UTF8, so SWF5 mangles it.
static as_value
string_from_char_code(const fn_call& fn)
{

    int version = VM::get().getSWFVersion();

    if (version == 5)
    {
        std::string str;
        for (unsigned int i = 0; i < fn.nargs; i++)
        {
            // Maximum 65535, as with all character codes.
            boost::uint16_t c = static_cast<boost::uint16_t>(fn.arg(i).to_int());
            
            // If more than 255, push 'overflow' byte.
            if (c > 255)
            {
                str.push_back(static_cast<unsigned char>(c >> 8));
            }

            // 0 terminates the string, but mustn't be pushed or it
            // will break concatenation.
            if (static_cast<unsigned char>(c) == 0) break;
            str.push_back(static_cast<unsigned char>(c));
        }    
        return as_value(str);
    }

    std::wstring wstr;

    for (unsigned int i = 0; i < fn.nargs; i++)
    {
        boost::uint16_t c = static_cast<boost::uint16_t>(fn.arg(i).to_int());
        if (c == 0) break;
        wstr.push_back(c);
    }
    
    return as_value(utf8::encodeCanonicalString(wstr, version));

}

static as_value
string_char_code_at(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    const std::wstring& wstr = utf8::decodeCanonicalString(obj->str(), version);

    if (fn.nargs == 0) {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("string.charCodeAt needs one argument"));
        )
        as_value rv;
        rv.set_nan();
        return rv;	// Same as for out-of-range arg
    }

    IF_VERBOSE_ASCODING_ERRORS(
        if (fn.nargs > 1) {
            log_aserror(_("string.charCodeAt has more than one argument"));
        }
    )

    size_t index = static_cast<size_t>(fn.arg(0).to_number());

    if (index >= wstr.length()) {
        as_value rv;
        rv.set_nan();
        return rv;
    }

    return as_value(wstr.at(index));
}

static as_value
string_char_at(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);

    int version = VM::get().getSWFVersion();

    const std::wstring& wstr = utf8::decodeCanonicalString(obj->str(), version);

    ENSURE_FN_ARGS(1, 1, "");

    size_t index = static_cast<size_t>(fn.arg(0).to_number());

    if (index >= wstr.length()) {
        as_value rv;
        rv.set_nan();
        return rv;
    }

    std::string rv;

    rv.append(utf8::encodeCanonicalString(wstr.substr(index, 1), version));

    return as_value(rv);
}

static as_value
string_to_upper_case(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);
    std::string subject = obj->str();

    VM& vm = VM::get();

    boost::to_upper(subject, vm.getLocale());

    return as_value(subject);
}

static as_value
string_to_lower_case(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);
    std::string subject = obj->str();

    VM& vm = VM::get();

    boost::to_lower(subject, vm.getLocale());

    return as_value(subject);
}

static as_value
string_to_string(const fn_call& fn)
{
    boost::intrusive_ptr<string_as_object> obj = ensureType<string_as_object>(fn.this_ptr);
    return as_value(obj->str());
}


static as_value
string_ctor(const fn_call& fn)
{
	std::string str;
	
	if (fn.nargs )
	{
		str = fn.arg(0).to_string();
	}

	if ( ! fn.isInstantiation() )
	{
		return as_value(str);
	}
	
	boost::intrusive_ptr<string_as_object> obj = new string_as_object;

	obj->str() = str;

	return as_value(obj.get());
}

static boost::intrusive_ptr<builtin_function>
getStringConstructor()
{
    // This is going to be the global String "class"/"function"

    static boost::intrusive_ptr<builtin_function> cl;

    if ( cl == NULL )
    {
	VM& vm = VM::get();

        cl=new builtin_function(&string_ctor, getStringInterface());
	vm.addStatic(cl.get());

	// ASnative(251, 14) - [String] fromCharCode 
	vm.registerNative(string_from_char_code, 251, 14);
	cl->init_member("fromCharCode", vm.getNative(251, 14)); 

    }

    return cl;
}

// extern (used by Global.cpp)
void string_class_init(as_object& global)
{
    // This is going to be the global String "class"/"function"
    boost::intrusive_ptr<builtin_function> cl = getStringConstructor();

    // Register _global.String
    // TODO: register as ASnative(251, 0)
    // TODO: register as ASnative(3, 0) for SWF5 ?
    global.init_member("String", cl.get());
}

boost::intrusive_ptr<as_object>
init_string_instance(const char* val)
{
	// TODO: get the environment passed in !!
	as_environment env;

	// TODO: get VM from the environment ?
	VM& vm = VM::get();
	int swfVersion = vm.getSWFVersion();

	boost::intrusive_ptr<as_function> cl;

	if ( swfVersion < 6 )
	{
		cl = getStringConstructor();
	}
	else
	{
		as_object* global = vm.getGlobal();
		as_value clval;
		if ( ! global->get_member(NSV::CLASS_STRING, &clval) )
		{
			log_debug("UNTESTED: String instantiation requested but _global doesn't contain a 'String' symbol. Returning the NULL object.");
			return cl;
			//cl = getStringConstructor();
		}
		else if ( ! clval.is_function() )
		{
			log_debug("UNTESTED: String instantiation requested but _global.String is not a function (%s). Returning the NULL object.",
				clval.to_debug_string().c_str());
			return cl;
			//cl = getStringConstructor();
		}
		else
		{
			cl = clval.to_as_function();
			assert(cl);
		}
	}

#ifndef NDEBUG
	size_t prevStackSize = env.stack_size();
#endif
	env.push(val);
	boost::intrusive_ptr<as_object> ret = cl->constructInstance(env, 1, 0);
	env.drop(1);
#ifndef NDEBUG
	assert( prevStackSize == env.stack_size());
#endif

	return ret;
}

} // namespace gnash
