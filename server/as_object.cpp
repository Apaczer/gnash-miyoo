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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "log.h"

#include "as_object.h"
#include "as_function.h"
#include "as_environment.h" // for enumerateProperties
#include "Property.h" // for findGetterSetter
#include "VM.h"
#include "GnashException.h"
#include "fn_call.h" // for generic methods
#include "Object.h" // for getObjectInterface

#include <set>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>
#include <utility> // for std::pair


// Anonymous namespace used for module-static defs
namespace {

using namespace gnash;

// A PropertyList visitor copying properties to an object
class PropsCopier {

	as_object& _tgt;

public:

	/// \brief
	/// Initialize a PropsCopier instance associating it
	/// with a target object (an object whose members has to be set)
	///
	PropsCopier(as_object& tgt)
		:
		_tgt(tgt)
	{}

	/// \brief
	/// Use the set_member function to properly set *inherited* properties
	/// of the given target object
	///
	void operator() (const std::string& name, const as_value& val)
	{
		//log_msg("Setting member '%s' to value '%s'", name.c_str(), val.to_string());
		_tgt.set_member(name, val);
	}
};

} // end of anonymous namespace


namespace gnash {

bool
as_object::add_property(const std::string& key, as_function& getter,
		as_function& setter)
{
	if ( _vm.getSWFVersion() < 7 )
	{
		std::string name = key;
		boost::to_lower(name, _vm.getLocale());
		return _members.addGetterSetter(name, getter, setter);
	}
	else
	{
		return _members.addGetterSetter(key, getter, setter);
	}
}

/*protected*/
bool
as_object::get_member_default(const std::string& name, as_value* val)
{
	assert(val);

	//log_msg("Getting member %s (SWF version:%d)", name.c_str(), vm.getSWFVersion());

	// TODO: inspect wheter it is possible to make __proto__ a
	//       getter/setter property instead, to take this into account
	//
	if (name == "__proto__")
	{
		as_object* p = get_prototype();
		assert(p);
		val->set_as_object(get_prototype());
		return true;
	}

	Property* prop = findProperty(name);
	if ( ! prop ) return false;

	try 
	{
		*val = prop->getValue(*this);
		return true;
	}
	catch (ActionException& exc)
	{
		log_warning("%s", exc.what());
		return false;
	}
	
}

/*private*/
Property*
as_object::findProperty(const std::string& key)
{
	// this set will keep track of visited objects,
	// to avoid infinite loops
	std::set<const as_object*> visited;

	as_object* obj = this;
	while ( obj && visited.insert(obj).second )
	{
		Property* prop = obj->_members.getProperty(key);
		if ( prop ) return prop;
		else obj = obj->get_prototype();
	}

	// No Property found
	return NULL;

}

/*private*/
Property*
as_object::findGetterSetter(const std::string& key)
{
	// this set will keep track of visited objects,
	// to avoid infinite loops
	std::set<const as_object*> visited;

	as_object* obj = this;
	while ( obj && visited.insert(obj).second )
	{
		Property* prop = obj->_members.getProperty(key);
		if ( prop && prop->isGetterSetter() )
		{
			// what if a property is found which is
			// NOT a getter/setter ?
			return prop;
		}
		obj = obj->get_prototype();
	}

	// No Getter/Setter property found
	return NULL;

}

/*protected*/
void
as_object::set_prototype(boost::intrusive_ptr<as_object> proto)
{
	m_prototype = proto;
}

void
as_object::set_member_default(const std::string& key, const as_value& val )
{

	//log_msg("set_member_default( %s ) ", key.c_str() );

	// TODO: make __proto__ a getter/setter ?
	if (key == "__proto__") 
	{
		set_prototype(val.to_object());
		return;
	}

	// found a getter/setter property in the inheritance chain
	// so set that and return
	Property* prop = findGetterSetter(key);
	if ( prop )
	{
		try 
		{
			//log_msg("Found a getter/setter property for key %s", key.c_str());
			if (prop->isReadOnly())
			{
				IF_VERBOSE_ASCODING_ERRORS(
                			log_aserror("Attempt to set read-only property '%s'",
						    key.c_str());
	                	);
			} else
			{
				prop->setValue(*this, val);
			}
			return;
		}
		catch (ActionException& exc)
		{
			log_warning("%s: %s. Will create a new member.", key.c_str(), exc.what());
		}
	}

	//log_msg("Found NO getter/setter property for key %s", key.c_str());

	// No getter/setter property found, so set (or create) a
	// SimpleProperty (if possible)
	if ( ! _members.setValue(key, val, *this) )
	{
		log_warning("Attempt to set Read-Only property ``%s''"
			" on object ``%p''",
			key.c_str(), (void*)this);
	}

}

void
as_object::init_member(const std::string& key, const as_value& val, int flags)
{

	//log_msg("Setting member %s (SWF version:%d)", key.c_str(), vm.getSWFVersion());
	//log_msg("Found NO getter/setter property for key %s", key.c_str());

	VM& vm = _vm;

	if ( vm.getSWFVersion() < 7 )
	{
		std::string keylower = key;
		boost::to_lower(keylower, vm.getLocale());

		// Set (or create) a SimpleProperty 
		if ( ! _members.setValue(keylower, val, *this) )
		{
			log_error("Attempt to initialize Read-Only property ``%s''"
				" (%s) on object ``%p''",
				keylower.c_str(), key.c_str(), (void*)this);
			// We shouldn't attempt to initialize a member twice, should we ?
			assert(0);
		}
		// TODO: optimize this, don't scan again !
		_members.setFlags(keylower, flags, 0);
	}
	else
	{
		// Set (or create) a SimpleProperty 
		if ( ! _members.setValue(key, val, *this) )
		{
			log_error("Attempt to initialize Read-Only property ``%s''"
				" on object ``%p''",
				key.c_str(), (void*)this);
			// We shouldn't attempt to initialize a member twice, should we ?
			assert(0);
		}
		// TODO: optimize this, don't scan again !
		_members.setFlags(key, flags, 0);
	}




}

void
as_object::init_property(const std::string& key, as_function& getter,
		as_function& setter)
{
	bool success;
	if ( _vm.getSWFVersion() < 7 )
	{
		std::string name = key;
		boost::to_lower(name, _vm.getLocale());
		success = _members.addGetterSetter(name, getter, setter);
		//log_msg("Initialized property '%s'", name.c_str());
	}
	else
	{
		success = _members.addGetterSetter(key, getter, setter);
		//log_msg("Initialized property '%s'", key.c_str());
	}

	// We shouldn't attempt to initialize a property twice, should we ?
	assert(success);
}

void
as_object::init_property(const std::string& key, as_function& getter)
{
	init_property(key, getter, getter);

	as_prop_flags& flags = getOwnProperty(key)->getFlags();

	// ActionScript must not change the flags of this builtin property.
	flags.set_is_protected(true);

	// Make the property read-only; that is, the default no-op handler will
	// be triggered when ActionScript tries to set it.
	flags.set_read_only();
}

std::string
as_object::asPropName(std::string name)
{
	std::string orig = name;
	if ( _vm.getSWFVersion() < 7 )
	{
		boost::to_lower(orig, _vm.getLocale());
	}

	return orig;
}


bool
as_object::set_member_flags(const std::string& name,
		int setTrue, int setFalse)
{
	// TODO: accept a std::string directly
	return _members.setFlags(name, setTrue, setFalse);
}

void
as_object::clear()
{
	_members.clear();
	m_prototype = NULL;
}

bool
as_object::instanceOf(as_function* ctor)
{
	const as_object* obj = this;

	std::set<const as_object*> visited;

	while (obj && visited.insert(obj).second )
	{
		if ( obj->get_prototype() == ctor->getPrototype() ) return true;
		obj = obj->get_prototype(); 
	}

	// See actionscript.all/Inheritance.as for a way to trigger this
	IF_VERBOSE_ASCODING_ERRORS(
	if ( obj ) log_aserror("Circular inheritance chain detected during instanceOf call");
	);

	return false;
}

bool
as_object::prototypeOf(as_object& instance)
{
	const as_object* obj = &instance;

	std::set<const as_object*> visited;

	while (obj && visited.insert(obj).second )
	{
		if ( obj->get_prototype() == this ) return true;
		obj = obj->get_prototype(); 
	}

	// See actionscript.all/Inheritance.as for a way to trigger this
	IF_VERBOSE_ASCODING_ERRORS(
	if ( obj ) log_aserror("Circular inheritance chain detected during isPrototypeOf call");
	);

	return false;
}

void
as_object::dump_members() 
{
	log_msg("%d Members of object %p follow",
		_members.size(), (const void*)this);
	_members.dump(*this);
}

void
as_object::setPropFlags(as_value& props_val, int set_false, int set_true)
{
	if ( props_val.is_string() )
	{
		std::string propstr = props_val.to_string();
		for(;;)
		{
			std::string prop;
			size_t next_comma=propstr.find(",");
			if ( next_comma == std::string::npos )
			{
				prop=propstr;
			} 
			else
			{
				prop=propstr.substr(0,next_comma);
				propstr=propstr.substr(next_comma);
			}

			// set_member_flags will take care of case conversion
			if ( ! set_member_flags(prop.c_str(), set_true, set_false) )
			{
				log_warning("Can't set propflags on object "
					"property %s "
					"(either not found or protected)",
					prop.c_str());
			}

			if ( next_comma == std::string::npos )
			{
				break;
			}
		}
		return;
	}

	boost::intrusive_ptr<as_object> props = props_val.to_object();

	// Evan: it seems that if set_true == 0 and set_false == 0,
	// this function acts as if the parameters were (object, null, 0x1, 0)
	if (set_false == 0 && set_true == 0)
	{
	    props = NULL;
	    set_false = 0;
	    set_true = 0x1;
	}

	if (props == NULL)
	{
		// TODO: this might be a comma-separated list
		//       of props as a string !!
		//

		// Take all the members of the object
		//std::pair<size_t, size_t> result = 
		_members.setFlagsAll(set_true, set_false);

		// Are we sure we need to descend to __proto__ ?
		// should we recurse then ?

		if (m_prototype)
		{
			m_prototype->_members.setFlagsAll(set_true, set_false);
		}
	}
	else
	{
		//std::pair<size_t, size_t> result = 
		_members.setFlagsAll(props->_members, set_true, set_false);
	}
}


void
as_object::copyProperties(const as_object& o)
{
	PropsCopier copier(*this);
	o._members.visitValues(copier,
			// Need const_cast due to getValue getting non-const ...
			const_cast<as_object&>(o));
}

void
as_object::enumerateProperties(as_environment& env) const
{
	assert( env.top(0).is_null() );


	// this set will keep track of visited objects,
	// to avoid infinite loops
	std::set<const as_object*> visited;

	const as_object* obj = this;
	while ( obj && visited.insert(obj).second )
	{
		obj->_members.enumerateKeys(env);
		obj = obj->get_prototype();
	}

	// This happens always since top object in hierarchy
	// is always Object, which in turn derives from itself
	//if ( obj ) log_warning("prototype loop during Enumeration");
}

void
as_object::enumerateProperties(std::map<std::string, std::string>& to)
{

	// this set will keep track of visited objects,
	// to avoid infinite loops
	std::set<const as_object*> visited;

	as_object* obj = this;
	while ( obj && visited.insert(obj).second )
	{
		obj->_members.enumerateKeyValue(*this, to);
		obj = obj->get_prototype();
	}

}

as_object::as_object()
	:
	_members(),
	_vm(VM::get()),
	m_prototype(NULL)
{
}

as_object::as_object(as_object* proto)
	:
	_members(),
	_vm(VM::get()),
	m_prototype(proto)
{
}

as_object::as_object(boost::intrusive_ptr<as_object> proto)
	:
	_members(),
	_vm(VM::get()),
	m_prototype(proto)
{
}

as_object::as_object(const as_object& other)
	:
	ref_counted(),
	_members(other._members),
	_vm(VM::get()),
	m_prototype(other.m_prototype)
{
}

std::pair<bool,bool>
as_object::delProperty(const std::string& name)
{
	if ( _vm.getSWFVersion() < 7 )
	{
        	std::string key = name;
		boost::to_lower(key, _vm.getLocale());
		return _members.delProperty(key);
	}
	else
	{
		return _members.delProperty(name);
	}
}

Property*
as_object::getOwnProperty(const std::string& name)
{
	if ( _vm.getSWFVersion() < 7 )
	{
        	std::string key = name;
		boost::to_lower(key, _vm.getLocale());
		return _members.getProperty(key);
	}
	else
	{
		return _members.getProperty(name);
	}
}

as_value
as_object::tostring_method(const fn_call& fn)
{
	boost::intrusive_ptr<as_object> obj = fn.this_ptr;

	const char* text_val = obj->get_text_value();
	if ( text_val )
	{
		return as_value(text_val);
	}
	else
	{
		return as_value("[object Object]");
	}
}

as_value
as_object::valueof_method(const fn_call& fn)
{
	boost::intrusive_ptr<as_object> obj = fn.this_ptr;

	return obj->get_primitive_value();
}

as_object*
as_object::get_prototype()
{
	if ( m_prototype ) return m_prototype.get();
	//log_msg("as_object::get_prototype(): Hit top of inheritance chain");
	return getObjectInterface();
}

} // end of gnash namespace

