// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef GNASH_SWF_FUNCTION_H
#define GNASH_SWF_FUNCTION_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h" // GNASH_USE_GC
#endif

#include "as_function.h" // for inheritance
#include "as_object.h" // for composition (vector element)
#include "VM.h" //fow SWF version

#include <boost/algorithm/string/case_conv.hpp> 
#include <cassert>
#include <string>

// Forward declarations
namespace gnash {
	class action_buffer;
	class as_environmnet;
}

namespace gnash {

/// SWF-defined Function 
class swf_function : public as_function
{

public:

	typedef std::vector<as_object*> ScopeStack;

	enum SWFDefineFunction2Flags
	{
		/// Bind one register to "this" 
		PRELOAD_THIS = 0x01, // 1

		/// No "this" variable accessible by-name 
		SUPPRESS_THIS = 0x02, // 2

		/// Bind one register to "arguments" 
		PRELOAD_ARGUMENTS = 0x04, // 4

		/// No "argument" variable accessible by-name 
		SUPPRESS_ARGUMENTS = 0x08, // 8

		/// Bind one register to "super" 
		PRELOAD_SUPER = 0x10, // 16

		/// No "super" variable accessible by-name 
		SUPPRESS_SUPER = 0x20, // 32

		/// Bind one register to "_root" 
		PRELOAD_ROOT = 0x40, // 64

		/// Bind one register to "_parent" 
		PRELOAD_PARENT = 0x80, // 128

		/// Bind one register to "_global" 
		//
		/// TODO: check this.
        /// See:
        /// http://sswf.sourceforge.net/SWFalexref.html#action_declare_function2
		///       Looks like flags would look swapped
		PRELOAD_GLOBAL = 256 // 0x100

	};

	/// \brief
	/// Create an ActionScript function as defined in an
	/// action_buffer starting at offset 'start'
	//
	swf_function(const action_buffer& ab, as_environment& env, size_t start,
		const ScopeStack& with_stack);

	virtual ~swf_function() {}

	const ScopeStack& getScopeStack() const
	{
		return _scopeStack;
	}

	const action_buffer& getActionBuffer() const
	{
		return m_action_buffer;
	}

	size_t getStartPC() const
	{
		return m_start_pc;
	}

	size_t getLength() const
	{
		return m_length;
	}

	bool isFunction2() const
	{
		return m_is_function2;
	}

	void set_is_function2() { m_is_function2 = true; }

	void set_local_register_count(boost::uint8_t ct) {
        assert(m_is_function2);
        m_local_register_count = ct;
    }

	void set_function2_flags(boost::uint16_t flags) {
        assert(m_is_function2);
        m_function2_flags = flags;
    }

	void add_arg(int arg_register, const char* name)
	{
		assert(arg_register == 0 || m_is_function2 == true);
		m_args.resize(m_args.size() + 1);
		m_args.back().m_register = arg_register;
		m_args.back().m_name = PROPNAME(name);
	}

	void set_length(int len);

	/// Dispatch.
	virtual as_value call(const fn_call& fn);

#ifdef GNASH_USE_GC
	/// Mark reachable resources. Override from as_object
	//
	/// Reachable resources from this object are its scope stack
	/// and the prototype.
	///
	virtual void markReachableResources() const;
#endif // GNASH_USE_GC

private:

	/// Action buffer containing the function definition
	const action_buffer& m_action_buffer;

	/// @@ might need some kind of ref count here, but beware cycles
	as_environment& m_env;

	/// Scope stack on function definition.
	ScopeStack _scopeStack;

	/// \brief
	/// Offset within the action_buffer where
	/// start of the function is found.
	size_t	m_start_pc;

	/// Length of the function within the action_buffer
	//
	/// This is currently expressed in bytes as the
	/// action_buffer is just a blog of memory corresponding
	/// to a DoAction block
	size_t m_length;

	struct arg_spec
	{
		int m_register;
		std::string m_name;
	};
	std::vector<arg_spec>	m_args;
	bool m_is_function2;
	boost::uint8_t m_local_register_count;

	/// used by function2 to control implicit arg register assignments
	// 
	/// See http://sswf.sourceforge.net/SWFalexref.html#action_declare_function2
	boost::uint16_t	m_function2_flags;

};


} // end of gnash namespace

// __GNASH_SWF_FUNCTION_H__
#endif

