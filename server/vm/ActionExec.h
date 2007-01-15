// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
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

//

#ifndef GNASH_ACTIONEXEC_H
#define GNASH_ACTIONEXEC_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "with_stack_entry.h"
#include "as_environment.h" // for ensureStack

#include <vector>

// Forward declarations
namespace gnash {
	class action_buffer;
	class as_value;
	class swf_function;
}

namespace gnash {

/// Executor of an action_buffer 
class ActionExec {

private: 

	/// the 'with' stack associated with this execution thread
	std::vector<with_stack_entry> with_stack;

	/// Limit of with stack
	//
	/// This is 7 for SWF up to 5 and 15 for SWF 6 and up
	/// See: http://sswf.sourceforge.net/SWFalexref.html#action_with
	///
	/// Actually, there's likely NO point in using the limit.
	/// The spec say that a player6 is ensured to provide at least 15 elements
	/// in a 'with' stack, while player5 can support at most 7.
	/// There is no provision of a limit though.
	///
	/// Gnash will use this information only to generate useful
	/// warnings for coders (if ActionScript errors verbosity is
	/// enabled).
	///
	size_t _with_stack_limit;

	/// 1 for function execution, 2 for function2 execution, 0 otherwise.
	int _function_var;

	/// A pointer to the function being executed, or NULL
	/// for non-function execution
	///
	/// TODO: 
	/// This should likely be put in a larger
	/// structure including return address 
	/// and maintained in a stack (the call stack)
	///
	const swf_function* _func;

	size_t _initial_stack_size;

	/// Warn about a stack underrun and fix it 
	//
	/// The fix is padding the stack with undefined
	/// values for the missing slots.
	/// 
	/// @param required
	///	Number of items required.
	///
	void ActionExec::fixStackUnderrun(size_t required);

public:

	/// \brief
	/// Ensure the current stack frame has at least 'required' elements,
	/// fixing it if required.
	//
	/// Underruns are fixed by inserting elements at the start of
	/// stack frame, so that undefined function arguments are the
	/// *last* ones in the list.
	///
	void ensureStack(size_t required)
	{
		// The stack_size() < _initial_stack_size case should
		// be handled this by stack smashing checks
		assert( env.stack_size() >= _initial_stack_size );

		size_t slots_left = env.stack_size() - _initial_stack_size;
		if ( slots_left < required )
		{
			fixStackUnderrun(required);
		}
	}

	/// The actual action buffer
	//
	/// TODO: provide a getter and make private
	///
	const action_buffer& code;

	/// Program counter (offset of current action tag)
	//
	/// TODO: provide mutator funx and make private
	///
	size_t pc;

	/// End of current function execution
	//
	/// TODO: make private
	///
	size_t stop_pc;

	/// Offset to next action tag
	size_t next_pc;

	/// TODO: provide a getter and make private ?
	as_environment& env;

	/// TODO: provide a setter and make private ?
	as_value* retval;

	/// Create an execution thread 
	//
	/// @param abuf
	///	the action code
	///
	/// @param newEnv
	///	the execution environment (variables scope, stack etc.)
	///
	ActionExec(const action_buffer& abuf, as_environment& newEnv);

	/// Create an execution thread for a function call.
	//
	/// @param func
	///	The function 
	///
	/// @param newEnv
	///	The execution environment (variables scope, stack etc.)
	///
	/// @param nRetval
	///	Where to return a value. If NULL any return will be discarded.
	///
	ActionExec(const swf_function& func, as_environment& newEnv, as_value* nRetVal);

#if 0
	/// Create an execution thread for a function call.
	//
	/// @param abuf
	///	the action code
	///
	/// @param newEnv
	///	the execution environment (variables scope, stack etc.)
	///
	/// @param nStartPC
	///	where to start execution (offset from start of action code)
	///
	/// @param nExecBytes
	///	Number of bytes to run this is probably a redundant
	///	information, as an ActionEnd should tell us when to stop.
	///	We'll keep this parameter as an SWF integrity checker.
	///
	/// @param nRetval
	///	where to return a value, if this is a function call (??)
	///
	/// @param initial_with_stack
	///	the 'with' stack to use
	///
	/// @param nIsFunction2
	///	wheter the given action code is actually a Function2
	///	
	ActionExec(const action_buffer& abuf, as_environment& newEnv,
		size_t nStartPC, size_t nExecBytes, as_value* nRetval,  
		const std::vector<with_stack_entry>& initial_with_stack,
		bool nIsFunction2);
#endif

	/// Is this execution thread a function2 call ?
	bool isFunction2() { return _function_var==2; }

	/// Is this execution thread a function call ?
	bool isFunction() { return _function_var!=0; }

	/// Returns 'with' stack associated with this execution thread
	// 
	/// If you need to modify it, use the pushWithEntry() function.
	///
	const std::vector<with_stack_entry>& getWithStack() const
	{
		return with_stack;
	}

	/// Return the maximum allowed 'with' stack limit.
	//
	/// See http://sswf.sourceforge.net/SWFalexref.html#action_with
	/// for more info.
	///
	/// Note that Gnash have NO limit on the number of 'with'
	/// stack entries, this information will only be used to
	/// generate useful warnings for coders (if ActionScript errors
	/// verbosity is enabled).
	///
	size_t getWithStackLimit() const 
	{
		return _with_stack_limit;
	}

	/// Push an entry to the with stack
	//
	/// @return
	///	true if the entry was pushed, false otherwise.
	///	Note that false will *never* be returned as Gnash
	///	removed the 'with' stack limit. If the documented
	///	limit for stack (SWF# bound) is reached, and ActionScript
	///	errors verbosity is enabled, a warning will be raised,
	///	but the call will still succeed.
	///	
	bool pushWithEntry(const with_stack_entry& entry);

	/// Skip the specified number of action tags 
	//
	/// The offset is relative to next_pc
	///
	void skip_actions(size_t offset);

	/// Delete named variable, seeking for it in the with stack if any
	//
	/// @param name
	///	Name of the variable. Supports slash and dot syntax.
	///	Name is converted to lowercase if SWF version is < 7.
	///
	bool delVariable(const std::string& name);

	/// Set a named variable, seeking for it in the with stack if any.
	//
	/// @param name
	///	Name of the variable. Supports slash and dot syntax.
	///	Name is converted to lowercase if SWF version is < 7.
	///
	void setVariable(const std::string& name, const as_value& val);

	/// \brief
	/// If in a function context set a local variable,
	/// otherwise, set a normal variable.
	//
	/// @param name
	///	Name of the variable. Supports slash and dot syntax.
	///	Name is converted to lowercase if SWF version is < 7.
	///
	void setLocalVariable(const std::string& name, const as_value& val);

	/// Get a named variable, seeking for it in the with stack if any.
	//
	/// @param name
	///	Name of the variable. Supports slash and dot syntax.
	///	Name is converted to lowercase if SWF version is < 7.
	///
	as_value getVariable(const std::string& name);

	/// Execute.
	void operator() ();
};

} // namespace gnash

#endif // GNASH_ACTIONEXEC_H

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
