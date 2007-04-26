// as_environment.cpp:  Variable, Sprite, and Movie locators, for Gnash.
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

/* $Id: as_environment.cpp,v 1.74 2007/04/26 17:06:10 strk Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smart_ptr.h"
#include "as_environment.h"
#include "sprite_instance.h"
#include "shape_character_def.h"
#include "as_value.h"
#include "with_stack_entry.h"
#include "VM.h"
#include "log.h"
#include "Property.h"
#include "as_object.h"

#include <string>
#include <utility> // for std::pair

// Define this to have find_target() calls trigger debugging output
//#define DEBUG_TARGET_FINDING 1

namespace gnash {

// Return the value of the given var, if it's defined.
as_value
as_environment::get_variable(const std::string& varname,
		const ScopeStack& with_stack, as_object** retTarget) const
{
    // Path lookup rigamarole.
    std::string	path;
    std::string	var;
    //log_msg(_("get_variable(%s)"), varname.c_str());
    bool is_slash_based;
    if (parse_path(varname, path, var, &is_slash_based)) {
	//as_value target_val = get_variable_raw(path, with_stack);
        //as_object* target = target_val.to_object();
	// TODO: let find_target return generic as_objects, or use 'with' stack,
	//       see player2.swf or bug #18758 (strip.swf)
	// @@ TODO: should we use with_stack here too ?
        as_object* target = is_slash_based ? find_object_slashsyntax(path) : find_object_dotsyntax(path); 

	if (target) {
	    as_value	val;
	    target->get_member(var.c_str(), &val);
	    if ( retTarget ) *retTarget = target;
	    return val;
	} else {

	    IF_VERBOSE_ASCODING_ERRORS(
	    log_aserror(_("find_object%s(\"%s\") [ varname = '%s' - current target = '%s' ] failed"),
			    is_slash_based ? "_slashsyntax" : "_dotsyntax",
			    path.c_str(),
			    varname.c_str(),
			    m_target->get_text_value().c_str()
			    );
	    );

	    as_value tmp = get_variable_raw(path, with_stack, retTarget);
	    if ( ! tmp.is_undefined() )
	    {
	    IF_VERBOSE_ASCODING_ERRORS(
		    log_aserror(_("...but get_variable_raw(%s, <with_stack>) succeeded!"), path.c_str());
	    )
	    }
	    return as_value();
	}
    } else {
	return get_variable_raw(varname, with_stack, retTarget);
    }
}

as_value
as_environment::get_variable(const std::string& varname) const
{
	static ScopeStack empty_with_stack;
	return get_variable(varname, empty_with_stack);
}

as_value
as_environment::get_variable_raw(
    const std::string& varname,
    const ScopeStack& with_stack, as_object** retTarget) const
    // varname must be a plain variable name; no path parsing.
{
    assert(strchr(varname.c_str(), ':') == NULL);

    as_value	val;

    // Check the with-stack.
    for (size_t i = with_stack.size(); i > 0; --i) {
        // const_cast needed due to non-const as_object::get_member 
	as_object* obj = const_cast<as_object*>(with_stack[i-1].object());
	if (obj && obj->get_member(varname.c_str(), &val)) {
	    // Found the var in this context.
	    if ( retTarget ) *retTarget = obj;
	    return val;
	}
    }

    // Check locals for getting them
    //as_environment::frame_slot slot;
    if ( findLocal(varname, val, true, retTarget) ) // do we really want to descend here ??
    {
	return val;
    }


    // Check target members.
    if (m_target->get_member(varname.c_str(), &val)) {
	if ( retTarget ) *retTarget = m_target;
	return val;
    }

    // Looking for "this"?
    if (varname == "this") {
	val.set_as_object(m_target);
	if ( retTarget ) *retTarget = NULL; // correct ??
	return val;
    }

    // Check built-in constants.
    if (varname == "_root" || varname == "_level0") {
	if ( retTarget ) *retTarget = NULL; // correct ??
	return as_value(m_target->get_root_movie());
    }

    VM& vm = VM::get();
    as_object* global = vm.getGlobal();

    if ( vm.getSWFVersion() > 5 && varname == "_global" )
    {
	// The "_global" ref was added in SWF6
	if ( retTarget ) *retTarget = NULL; // correct ??
	return as_value(global);
    }

    if (global->get_member(varname.c_str(), &val))
    {
	if ( retTarget ) *retTarget = global;
	return val;
    }
    
    // Fallback.
    // FIXME, should this be log_error?  or log_swferror?
	IF_VERBOSE_ACTION (
    log_action(_("get_variable_raw(\"%s\") failed, returning UNDEFINED"),
	       varname.c_str());
	);

    return as_value();
}

bool
as_environment::del_variable_raw(
    const std::string& varname,
    const ScopeStack& with_stack) 
    // varname must be a plain variable name; no path parsing.
{
	assert(strchr(varname.c_str(), ':') == NULL);
	assert(strchr(varname.c_str(), '/') == NULL);
	assert(strchr(varname.c_str(), '.') == NULL);

	as_value	val;

	// Check the with-stack.
	for (size_t i = with_stack.size(); i > 0; --i)
	{
		// const_cast needed due to non-const as_object::get_member 
		as_object* obj = const_cast<as_object*>(with_stack[i-1].object());
		if (obj)
		{
			std::pair<bool,bool> ret = obj->delProperty(varname);
			if (ret.first)
			{
			    return ret.second;
			}
		}
	}

	// Check locals for deletion.
	if ( delLocal(varname) )
	{
		return true;
	}


	// Try target
	std::pair<bool,bool> ret = m_target->delProperty(varname);
	if ( ret.first )
	{
		return ret.second;
	}


	// Try _global
	return VM::get().getGlobal()->delProperty(varname).second;
}

// varname must be a plain variable name; no path parsing.
as_value
as_environment::get_variable_raw(const std::string& varname) const
{
	static ScopeStack empty_with_stack;
	return get_variable_raw(varname, empty_with_stack);
}

// Given a path to variable, set its value.
void
as_environment::set_variable(
    const std::string& varname,
    const as_value& val,
    const ScopeStack& with_stack)
{
	IF_VERBOSE_ACTION (
    log_action("-------------- %s = %s",
	       varname.c_str(), val.to_debug_string().c_str());
	);

    // Path lookup rigamarole.
    as_object* target = m_target;
    std::string	path;
    std::string	var;
    //log_msg(_("set_variable(%s, %s)"), varname.c_str(), val.to_debug_string().c_str());
    bool is_slash_based;
    if (parse_path(varname, path, var, &is_slash_based)) {
    	//log_msg(_("Variable '%s' parsed into path='%s', var='%s'"), varname.c_str(), path.c_str(), var.c_str());
	//target = find_target(path);
        target = is_slash_based ? find_object_slashsyntax(path) : find_object_dotsyntax(path); 
	if (target)
	{
	    target->set_member(var.c_str(), val);
	}
	else
	{
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("Path target '%s' not found while setting %s=%s"),
			path.c_str(), varname.c_str(), val.to_debug_string().c_str());
		);
	}
    } else {
	set_variable_raw(varname, val, with_stack);
    }
}

void
as_environment::set_variable(
		const std::string& varname,
		const as_value& val)
{
	static ScopeStack empty_with_stack;
	set_variable(varname, val, empty_with_stack);
}

// No path rigamarole.
void
as_environment::set_variable_raw(
    const std::string& varname,
    const as_value& val,
    const ScopeStack& with_stack)
{

	// Check locals for setting them
	if ( setLocal(varname, val, true) )
	{
		return;
	}

	// Check the with-stack.
	for (size_t i = with_stack.size(); i > 0; --i)
	{
		// const_cast needed due to non-const as_object::get_member 
		as_object* obj = const_cast<as_object*>(with_stack[i-1].object());
		as_value	dummy;
		if (obj && obj->get_member(varname.c_str(), &dummy)) {
		    // This object has the member; so set it here.
		    obj->set_member(varname.c_str(), val);
		    return;
		}
	}
    
	assert(m_target);
	m_target->set_member(varname.c_str(), val);
}

void
as_environment::set_variable_raw(
		const std::string& varname,
		const as_value& val)
{
	static ScopeStack empty_with_stack;
	set_variable_raw(varname, val, empty_with_stack);
}

// Set/initialize the value of the local variable.
void
as_environment::set_local(const std::string& varname, const as_value& val)
{
	// why would you want to set a local if there's no call frame on the
	// stack ?
	assert(_localFrames.size());

	// Is it in the current frame already?
	// TODO: should we descend to upper frames ?
	//       (probably not as we want to update it)
	//as_environment::frame_slot slot;
	if ( setLocal(varname, val, false) )
	{
		return;
	}
	else
	{
		// Not in frame; create a new local var.
		assert(_localFrames.size());
		assert(varname.length() > 0);	// null varnames are invalid!
		LocalVars& locals = _localFrames.back().locals;
		//locals.push_back(as_environment::frame_slot(varname, val));
		locals->set_member(varname, val);
	}
}
	
// Create the specified local var if it doesn't exist already.
void
as_environment::declare_local(const std::string& varname)
{
	// TODO: should we descend to upper frames ?
	//       (probably not as we want to declare it)
	//as_environment::frame_slot slot;

	as_value tmp;
	if ( ! findLocal(varname, tmp, false) )
	{
		// Not in frame; create a new local var.
		assert(_localFrames.size());
		assert(varname.length() > 0);	// null varnames are invalid!
		LocalVars& locals = _localFrames.back().locals;
		//locals.push_back(as_environment::frame_slot(varname, as_value()));
		locals->set_member(varname, as_value());
	}
}
	
bool
as_environment::get_member(const std::string& varname, as_value* val) const
{
    Variables::const_iterator it = _variables.find(varname);
    if ( it == _variables.end() ) return false;
    
    *val = it->second;
    return true;
}


void
as_environment::set_member(const std::string& varname, const as_value& val)
{
    _variables[varname] = val;
}

/* public static */
bool
as_environment::parse_path(const std::string& var_path,
		std::string& path, std::string& var, bool* is_slash_based) 
{
//log_msg(_("parse_path(%s)"), var_path.c_str());
    // Search for colon.
    int	colon_index = 0;
    int	var_path_length = var_path.length();
    for ( ; colon_index < var_path_length; colon_index++) {
	if (var_path[colon_index] == ':') {
            if ( is_slash_based ) *is_slash_based = true;
	    // Found it.
	    break;
	}
    }
    
    if (colon_index >= var_path_length)	{
	// No colon.  Is there a '.'?  Find the last
	// one, if any.
	for (colon_index = var_path_length - 1; colon_index >= 0; colon_index--) {
	    if (var_path[colon_index] == '.') {
		// Found it.
                if ( is_slash_based ) *is_slash_based = false;
		break;
	    }
	}
	if (colon_index < 0) {
//log_msg(_(" no colon index"));
	    return false;
	}
    }
    
    // Make the subparts.
    
    // Var.
    var = &var_path[colon_index + 1];
    
    // @@ could be better. 
    path = var_path;
    path.resize(colon_index);
    
//log_msg(_(" path=%s var=%s"), path.c_str(), var.c_str());

    return true;
}

bool
as_environment::parse_path(const std::string& var_path,
		as_object** target, as_value& val)
{
	string path;
	string var;
	bool is_slash_based;
	if( ! parse_path(var_path, path, var, &is_slash_based) ) return false;
        as_object* target_ptr = is_slash_based ? find_object_slashsyntax(path) : find_object_dotsyntax(path); 
	if ( ! target_ptr ) return false;

	target_ptr->get_member(var, &val);
	*target = target_ptr;
	return true;
}

character*
as_environment::find_target(const as_value& val) const
{
	// TODO: should we reduce this whole function to
	//       find_target(val.to_std_string()) ?
	//       a quick test shows it would work, I'm just not sure about
	//       edit_text_chars, that might return the text value rather
	//       then their target ...

	if ( val.is_object() )
	{
		boost::intrusive_ptr<as_object> obj = val.to_object();
		assert (obj);
		character* s=dynamic_cast<character*>(obj.get());
		assert(s->get_ref_count() > 1); // or the intrusive_ptr above going out-of-scope will kill it
		//log_msg(_("find_target is a character, returning it"));
		return s; // might be NULL
	}
	else if ( val.is_string() )
	{
		return find_target(val.to_string());
	}
	else
	{
		// TODO: should we *force* string conversion above instead ?
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("as_environment::find_target: '%s': "
			"invalid path; neither string nor object"),
			val.to_debug_string().c_str() );
		);
		return NULL;
	}
}

// Search for next '.' or '/' character in this word.  Return
// a pointer to it, or to NULL if it wasn't found.
static const char*
next_slash_or_dot(const char* word)
{
    for (const char* p = word; *p; p++)	{
	if (*p == '.' && p[1] == '.') {
	    p++;
	} else if (*p == '.' || *p == '/') {
	    return p;
	}
    }
    
    return NULL;
}

static const char*
find_next_slash(const char* word)
{
    for (const char* p = word; *p; p++)	{
	if (*p == '/') {
	    return p;
	}
    }
    
    return NULL;
}

static const char*
find_next_dot(const char* word)
{
    for (const char* p = word; *p; p++)	{
	if (*p == '.' ) {
	    return p;
	}
    }
    
    return NULL;
}

// Find the sprite/movie referenced by the given path.
//
// Supports both /slash/syntax and dot.syntax
//
character*
as_environment::find_target(const std::string& path) const
{
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("find_target(%s) called"), path.c_str());
#endif

    if (path.length() <= 0) {
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Returning m_target (empty path)"));
#endif
	return m_target;
    }
    
    // we'd have returned m_target in this case
    //assert(path.length() > 0);
    
    character* env = m_target;
    assert(env);
    
    const char*	p = path.c_str();

    if (*p == '/') {
	// Absolute path.  Start at the root.
	env = env->get_root_movie();
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Absolute path, start at the root (%p)"), (void*)env);
#endif
	p++;
    }
    
    if (*p == '\0') {
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Null path, returning m_target"));
#endif
	return env;
    }

    std::string	subpart;
    while (env) {
	const char*	next_slash = next_slash_or_dot(p);
	subpart = p;
	if (next_slash == p) {
            IF_VERBOSE_ASCODING_ERRORS(
	    log_aserror(_("invalid path '%s'"), path.c_str());
	    );
	    return NULL;
	    //break;
	} else if (next_slash) {
	    // Cut off the slash and everything after it.
	    subpart.resize(next_slash - p);
	}
	
	// No more components to scan
	if ( subpart.empty() )
	{
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("No more subparts, env is %p"), (void*)env);
#endif
		break;
	}

#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Invoking get_relative_target(%s) on object %p (%s)"), subpart.c_str(), (void *)env, env->get_name().c_str());
#endif
	env = env->get_relative_target(subpart);
	//@@   _level0 --> root, .. --> parent, . --> this, other == character
	
	if (env == NULL || next_slash == NULL) {
	    break;
	}
	
	p = next_slash + 1;
    }
    return env;
}

as_object*
as_environment::find_object_dotsyntax(const std::string& path) const
{
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("find_object_dotsyntax(%s) called"), path.c_str());
#endif

    if (path.length() <= 0) {
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Returning m_target (empty path)"));
#endif
	return m_target;
    }
    
    // we'd have returned m_target in this case
    //assert(path.length() > 0);
    
    as_object* env = m_target;
    assert(env);
    
    if ( path.empty() ) {
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Null path, returning m_target"));
#endif
	return env;
    }

    const char*	p = path.c_str();
    unsigned int depth=0; // number of iterations
    while (env)
    {
	const char* next_dot = find_next_dot(p); // TODO: use std::string::find
    	std::string subpart = p;
	if (next_dot == p) {
	    IF_VERBOSE_ASCODING_ERRORS(
	    log_aserror(_("invalid path '%s'"), path.c_str());
	    );
	    return NULL; // TODO: check me
	    //break;
	} else if (next_dot) {
	    // Cut off the slash and everything after it.
	    subpart.resize(next_dot - p);
	}

#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Subpart ==  %s"), subpart.c_str());
#endif
	
	// No more components to scan
	if ( subpart.empty() )
	{
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("No more subparts, env is %p"), (void*)env);
#endif
		break;
	}

#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Invoking get_member(%s) on object %p"), subpart.c_str(), (void *)env);
#endif
	as_value tmp;
	// TODO: make sure sprite_instances know about ".."
	if ( ! env->get_member(subpart.c_str(), &tmp) )
	{
		// Try _global, but only at first iteration...
		if ( depth > 0 ) 
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Member %s for object %p not found (dotsyntax).  Path was %s"),
				subpart.c_str(), env, path.c_str());
			);
			return NULL;
		}

		if ( ! VM::get().getGlobal()->get_member(subpart.c_str(), &tmp) )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Element '%s' of variable '%s' not found in object %p nor in _global (dotsyntax)"),
				subpart.c_str(), path.c_str(), env);
			);
			return NULL;
		}
	} 

	// Debugging only:
	if ( ! tmp.is_object() ) {
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("Member %s of object %p doesn't cast to an Object (%s) "
			"evaluating target path %s (dotsyntax)"),
			subpart.c_str(), env, tmp.to_debug_string().c_str(), path.c_str());
		);
		return NULL;
	}

	env = tmp.to_object().get();
	assert(env->get_ref_count() > 0); // still alive...


	//@@   _level0 --> root, .. --> parent, . --> this, other == character
	
	if (next_dot == NULL) {
	    break;
	}
	
	p = next_dot + 1;
	++depth;
    }
    return env;
}

as_object*
as_environment::find_object_slashsyntax(const std::string& path) const
{
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("find_object_slashsyntax(%s) called"), path.c_str());
#endif

    if (path.length() <= 0) {
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Returning m_target (empty path)"));
#endif
	return m_target;
    }
    
    // we'd have returned m_target in this case
    //assert(path.length() > 0);
    
    as_object* env = m_target;
    assert(env);
    
    const char*	p = path.c_str();

    if (*p == '/') {
	// Absolute path.  Start at the *absolute* root.
	// TODO: should this be VM::get().getRoot().get_root_movie() ?
	env = m_target->get_root_movie();
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Absolute path, start at the root (%p)"), (void*)env);
#endif
	p++;
    }
    
    if (*p == '\0') {
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Null path, returning m_target"));
#endif
	return env;
    }

    unsigned int depth=0; // number of iterations
    while (env) {
    	std::string	subpart;
	const char* next_slash = find_next_slash(p); // TODO: use std::string::find
	subpart = p;
	if (next_slash == p) {
            IF_VERBOSE_ASCODING_ERRORS(
	    log_aserror(_("invalid path '%s'"), path.c_str());
	    );
	    return NULL; // TODO: check me
	    //break;
	} else if (next_slash) {
	    // Cut off the slash and everything after it.
	    subpart.resize(next_slash - p);
	}

#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("Subpart ==  %s"), subpart.c_str());
#endif
	
	// No more components to scan
	if ( subpart.empty() )
	{
#ifdef DEBUG_TARGET_FINDING 
	log_msg(_("No more subparts, env is %p"), (void*)env);
#endif
		break;
	}

	if ( subpart == ".." )
	{
		character* ch = dynamic_cast<character*>(env);
		if ( ! ch ) {
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("'..' element in path '%s' follows a non-character object %p"),
					path.c_str(), env);
			);
			return NULL;
		}
		env = ch->get_parent();
		if ( ! env ) // root movie doesn't have a parent
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("'..' in path '%s' follows a character with no parent (%s : %p) (root is %p)"),
					path.c_str(), ch->get_text_value().c_str(), ch, VM::get().getRoot().get_root_movie());
			);
			// if we override env, getvariable.as fails [line 57]
			//env = ch;
		}
	}
	else
	{

#ifdef DEBUG_TARGET_FINDING 
		log_msg(_("Invoking get_member(%s) on object %p"), subpart.c_str(), (void *)env);
#endif
		as_value tmp;
		// TODO: make sure sprite_instances know about ".."
		if ( ! env->get_member(subpart.c_str(), &tmp) )
		{
			// Try _global, but only at first iteration...
			if ( depth > 0 ) 
			{
				IF_VERBOSE_ASCODING_ERRORS(
				log_aserror(_("Member %s for object %p not found (slashsyntax). Path was %s"),
					subpart.c_str(), env, path.c_str());
				);
				return NULL;
			}

			if ( ! VM::get().getGlobal()->get_member(subpart.c_str(), &tmp) )
			{
				IF_VERBOSE_ASCODING_ERRORS(
				log_aserror(_("Element '%s' of variable '%s' not found in object %p nor in _global (slashsyntax)"),
					subpart.c_str(), path.c_str(), env);
				);
				return NULL;
			}
		} 

		// Debugging only:
		if ( ! tmp.is_object() ) {
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Member %s of object %p doesn't cast to an Object (%s) "
				"evaluating target path %s (slashsyntax)"),
				subpart.c_str(), env, tmp.to_debug_string().c_str(), path.c_str());
				);
			return NULL;
		}

		env = tmp.to_object().get();
		assert(env->get_ref_count() > 0);
	}

	//@@   _level0 --> root, .. --> parent, . --> this, other == character
	
	if (next_slash == NULL) {
	    break;
	}
	
	p = next_slash + 1;
	++depth;
    }
    return env;
}

int
as_environment::get_version() const
{
	return VM::get().getSWFVersion();
}

static void
dump(const as_environment::Registers& r, std::ostream& out) 
{
	for (size_t i=0; i<r.size(); ++i)
	{
		if (i) out << ", ";
		out << i << ':' << '"' << r[i] << '"';
	}
}

void
as_environment::dump_local_registers(std::ostream& out) const
{
	if ( _localFrames.empty() ) return;
	out << "Local registers: ";
	for (CallStack::const_iterator it=_localFrames.begin(),
			itEnd=_localFrames.end();
			it != itEnd; ++it)
	{
		if ( it != _localFrames.begin() ) out << " | ";
		dump(it->registers, out);
	}
	out << std::endl;
}

static void
dump(const as_environment::LocalVars& locals, std::ostream& out)
{
	log_msg("FIXME: implement dumper for local variables now that they are simple objects");
#if 0
	for (size_t i=0; i<locals.size(); ++i)
	{
		const as_environment::frame_slot& slot = locals[i];
		if (i) out << ", ";
		// TODO: define output operator for as_value !
		out << slot.m_name << "==" << slot.m_value;
	}
	out << std::endl;
#endif
}

void
as_environment::dump_local_variables(std::ostream& out) const
{
	if ( _localFrames.empty() ) return;
	out << "Local variables: ";
	for (CallStack::const_iterator it=_localFrames.begin(),
			itEnd=_localFrames.end();
			it != itEnd; ++it)
	{
		if ( it != _localFrames.begin() ) out << " | ";
		dump(it->locals, out);
	}
	out << std::endl;
}

void
as_environment::dump_global_registers(std::ostream& out) const
{
	std::string registers;
	int defined=0;
	for (unsigned int i=0; i<4; ++i)
	{
		if (i) registers += std::string(" | ");
		registers += std::string("\"") +
			m_global_register[i].to_debug_string() +
			std::string("\"");
		if ( ! m_global_register[i].is_undefined() ) defined++;
	}
	if ( defined ) out << "Global registers (" << defined << "): " << registers << std::endl;
}

/*private*/
bool
as_environment::findLocal(const std::string& varname, as_value& ret, bool descend, as_object** retTarget)
{
	if ( _localFrames.empty() ) return false;
	if ( ! descend )
	{
		if ( findLocal(_localFrames.back().locals, varname, ret) )
		{
			if ( retTarget ) *retTarget = _localFrames.back().locals.get();
			return true;
		}
		return false;
	}

	for (CallStack::reverse_iterator it=_localFrames.rbegin(),
			itEnd=_localFrames.rend();
			it != itEnd;
			++it)
	{
		LocalVars& locals = it->locals;
		if ( findLocal(locals, varname, ret) )
		{
			if ( retTarget ) *retTarget = locals.get();
			return true;
		}
	}
	return false;
}

/* static private */
bool
as_environment::findLocal(LocalVars& locals, const std::string& name, as_value& ret)
{
	return locals->get_member(name, &ret);
#if 0
	for (size_t i=0; i<locals.size(); ++i)
	{
		const as_environment::frame_slot& slot = locals[i];
		if (slot.m_name == name)
		{
			ret = slot;
			return true;
		}
	}
	return false;
#endif
}

/* static private */
bool
as_environment::delLocal(LocalVars& locals, const std::string& varname)
{
	return locals->delProperty(varname).second;
#if 0
	for (size_t i=0; i<locals.size(); ++i)
	{
		const as_environment::frame_slot& slot = locals[i];
		if (slot.m_name == varname)
		{
			locals.erase(locals.begin()+i);
			return true;
		}
	}
	return false;
#endif
}

/* private */
bool
as_environment::delLocal(const std::string& varname, bool descend)
{
	if ( _localFrames.empty() ) return false;
	if ( ! descend ) return delLocal(_localFrames.back().locals, varname);

	for (CallStack::reverse_iterator it=_localFrames.rbegin(),
			itEnd=_localFrames.rend();
			it != itEnd;
			++it)
	{
		LocalVars& locals = it->locals;
		if ( delLocal(locals, varname) )
		{
			return true;
		}
	}
	return false;
}

/* private */
bool
as_environment::setLocal(const std::string& varname, const as_value& val, bool descend)
{
	if ( _localFrames.empty() ) return false;
	if ( ! descend ) return setLocal(_localFrames.back().locals, varname, val);

	for (CallStack::reverse_iterator it=_localFrames.rbegin(),
			itEnd=_localFrames.rend();
			it != itEnd;
			++it)
	{
		LocalVars& locals = it->locals;
		if ( setLocal(locals, varname, val) )
		{
			return true;
		}
	}
	return false;
}

/* static private */
bool
as_environment::setLocal(LocalVars& locals,
		const std::string& varname, const as_value& val)
{
	Property* prop = locals->getOwnProperty(varname);
	if ( ! prop ) return false;
	prop->setValue(*locals, val);
	return true;
#if 0
	for (size_t i=0; i<locals.size(); ++i)
	{
		as_environment::frame_slot& slot = locals[i];
		if (slot.m_name == varname)
		{
			slot.m_value = val;
			return true;
		}
	}
	return false;
#endif
}


void
as_environment::padStack(size_t offset, size_t count)
{
	assert( offset <= m_stack.size() );
	m_stack.insert(m_stack.begin()+offset, count, as_value());
}

void
as_environment::pushCallFrame(as_function* func)
{
	const unsigned maxstacksize = 255;

	if ( _localFrames.size() == maxstacksize )
	{
		log_error(_("Max stack count reached (%u) - should abort execution"),
				maxstacksize);
		// throw something
		throw ActionLimitException("Call stack limit exceeded");
	}
	_localFrames.push_back(CallFrame(func));
}

void
as_environment::set_target(character* target)
{
	assert(target);
	if ( ! m_target ) _original_target = target;
	m_target = target;
}

void
as_environment::add_local(const std::string& varname, const as_value& val)
{
	assert(varname.length() > 0);	// null varnames are invalid!
	assert(_localFrames.size());
	LocalVars& locals = _localFrames.back().locals;
	//locals.push_back(frame_slot(varname, val));
	locals->set_member(varname, val);
}

as_environment::CallFrame::CallFrame(as_function* funcPtr)
	:
	locals(new as_object()),
	func(funcPtr)
{
}

} // end of gnash namespace



// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
