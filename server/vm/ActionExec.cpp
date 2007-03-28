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

/* $Id: ActionExec.cpp,v 1.23 2007/03/28 16:24:39 strk Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ActionExec.h"
#include "action_buffer.h"
#include "swf_function.h" 
#include "log.h"
#include "VM.h"
#include "GnashException.h"

#include "swf.h"
#include "ASHandlers.h"
#include "as_environment.h"
#include "debugger.h"

#include <typeinfo> 
#include <boost/algorithm/string/case_conv.hpp>

#if !defined(_WIN32) && !defined(WIN32)
# include <pthread.h> 
#endif

#include <sstream>
#include <string>

#ifndef DEBUG_STACK
// temporarly disabled as will produce lots of output with -v
// we'd need another switch maybe, as -va does also produce
// too much information for my tastes. I really want just
// to see how stack changes while executing actions...
// --strk Fri Jun 30 02:28:46 CEST 2006
#define DEBUG_STACK 1
#endif

using namespace gnash;
using namespace SWF;
using std::string;
using std::endl;
using std::stringstream;


namespace gnash {

static const SWFHandlers& ash = SWFHandlers::instance();
static LogFile& dbglogfile = LogFile::getDefaultInstance();
#ifdef USE_DEBUGGER
static Debugger& debugger = Debugger::getDefaultInstance();
#endif

// External interface (to be moved under swf/ASHandlers)
fscommand_callback s_fscommand_handler = NULL;
void	register_fscommand_callback(fscommand_callback handler)
{
    s_fscommand_handler = handler;
}

ActionExec::ActionExec(const swf_function& func, as_environment& newEnv, as_value* nRetVal, as_object* this_ptr)
	:
	with_stack(func.getWithStack()),
	// See comment in header
	_with_stack_limit(7),
	_function_var(func.isFunction2() ? 2 : 1),
	_func(&func),
	_this_ptr(this_ptr),
	code(func.getActionBuffer()),
	pc(func.getStartPC()),
	stop_pc(pc+func.getLength()),
	next_pc(pc), 
	env(newEnv),
	retval(nRetVal)
{
	//GNASH_REPORT_FUNCTION;
    
	// See comment in header
	if ( env.get_version() > 5 ) {
	    _with_stack_limit = 15;
	}
}

ActionExec::ActionExec(const action_buffer& abuf, as_environment& newEnv)
	:
	with_stack(),
	_with_stack_limit(7),
	_function_var(0),
	_func(NULL),
	code(abuf),
	pc(0),
	stop_pc(code.size()),
	next_pc(0),
	env(newEnv),
	retval(0)
{
	//GNASH_REPORT_FUNCTION;

	/// See comment in header
	if ( env.get_version() > 5 ) {
	    _with_stack_limit = 15;
	}
}

void
ActionExec::operator() ()
{

#if 0
    // Check the time
    if (periodic_events.expired()) {
	periodic_events.poll_event_handlers(&env);
    }
#endif
		
    _original_target = env.get_target();

    _initial_stack_size = env.stack_size();

#if DEBUG_STACK
	IF_VERBOSE_ACTION (
        	log_action("at ActionExec operator() start, pc=" SIZET_FMT
		           ", stop_pc=" SIZET_FMT ", code.size=" SIZET_FMT
			   ".", pc, 
			   stop_pc, code.size());
		stringstream ss;
		env.dump_stack(ss);
		env.dump_global_registers(ss);
		env.dump_local_registers(ss);
		env.dump_local_variables(ss);
		log_action("%s", ss.str().c_str());
	);
#endif

	try {
	while (pc<stop_pc) {
	    // Cleanup any expired "with" blocks.
	    while ( ! with_stack.empty() && pc >= with_stack.back().end_pc() ) {
		// Drop last stack element
		with_stack.pop_back();
	    }
	    
	// Get the opcode.
	uint8_t action_id = code[pc];

	IF_VERBOSE_ACTION (
		dbglogfile << std::endl << "PC:" << pc << " - EX: ";
		code.log_disasm(pc);
	);

	// Set default next_pc offset, control flow action handlers
	// will be able to reset it. 
	if ((action_id & 0x80) == 0) {
		// action with no extra data
		next_pc = pc+1;
	} else {
		// action with extra data
		uint16_t length = uint16_t(code.read_int16(pc+1));
		next_pc = pc + length + 3;
		if ( next_pc > stop_pc )
		{
			IF_VERBOSE_MALFORMED_SWF(
			std::stringstream ss;
			ss << "Length " << length << " (" << (int)length << ") of action tag"
				<< " id " << (unsigned)action_id
				<< " at pc " << pc
				<< " overflows actions buffer size "
				<< stop_pc;
			//throw ActionException(ss.str());;
			log_swferror("%s", ss.str().c_str());
			);
			// Give this action handler a chance anyway.
			// Maybe it will be able to do something about 
			// this anyway..
		}
	}

	// Do we still need this ?
	if ( action_id == SWF::ACTION_END ) {
		// this would turn into an assertion (next_pc==stop_pc)
		//		log_msg("At ACTION_END next_pc=%d, stop_pc=%d", next_pc, stop_pc);
		break;
	}

	ash.execute((action_type)action_id, *this);

#ifdef USE_DEBUGGER
 	debugger.setFramePointer(code.getFramePointer(pc));
 	debugger.setEnvStack(&env);
	if (debugger.isTracing()) {
	    debugger.dissasemble();
	}
#endif
	
#if DEBUG_STACK
	IF_VERBOSE_ACTION (
		log_action( " After execution, PC is " SIZET_FMT ".", pc);
		stringstream ss;
		env.dump_stack(ss);
		env.dump_global_registers(ss);
		env.dump_local_registers(ss);
		env.dump_local_variables(ss);
		log_action("%s", ss.str().c_str());
	);
#endif

	// Control flow actions will change the PC (next_pc)
	pc = next_pc;

    }

    }
    catch (ActionLimitException& ex)
    {
	    log_aserror("%s", ex.what());
    }
    
    cleanupAfterRun();

}

/*private*/
void
ActionExec::cleanupAfterRun()
{
    assert(_original_target);
    env.set_target(_original_target);
    _original_target = NULL;

    // check the call stack if not in a function context
    if ( ! isFunction() && env.callStackDepth() > 0 )
    {
	log_warning("Call stack non-empty at end of ExecutableCode run (limits hit?)");
	env.clearCallFrames();
    }

    // check if the stack was smashed
    if ( _initial_stack_size > env.stack_size() ) {
	log_warning("Stack smashed (ActionScript compiler bug?)."
		    "Fixing by pushing undefined values to the missing slots, "
		    " but don't expect things to work afterwards.");
	size_t missing = _initial_stack_size - env.stack_size();
	for (size_t i=0; i<missing; ++i) {
	    env.push(as_value());
	}
    } else if ( _initial_stack_size < env.stack_size() ) {
	// We can argue this would be an "size-optimized" SWF instead...
	IF_VERBOSE_MALFORMED_SWF(
	    log_warning("Elements left on the stack after block execution. Cleaning up.");
	    );
	env.drop(env.stack_size()-_initial_stack_size);
    }
}

void
ActionExec::skip_actions(size_t offset)
{
	//pc = next_pc;

	for(size_t i=0; i<offset; ++i) {
#if 1
	    // we need to check at every iteration because
	    // an action can be longer then a single byte
	    if ( next_pc >= stop_pc ) {
		log_error("End of DoAction block hit while skipping "
			  SIZET_FMT " action tags (pc:" SIZET_FMT 
			  ", stop_pc:" SIZET_FMT ") - Mallformed SWF ?"
			  "(WaitForFrame, probably)", offset, next_pc,
			  stop_pc);
		next_pc = stop_pc;
		return;
	    }
#endif
	    
	    // Get the opcode.
	    uint8_t action_id = code[next_pc];
	    
	    // Set default next_pc offset, control flow action handlers
	    // will be able to reset it. 
	    if ((action_id & 0x80) == 0) {
		// action with no extra data
		next_pc++;
	    } else {
		// action with extra data
		int16_t length = code.read_int16(next_pc+1);
		assert( length >= 0 );
		next_pc += length + 3;
	    }
	    
	    //pc = next_pc;
	}
}

bool
ActionExec::pushWithEntry(const with_stack_entry& entry)
{
	// See comment in header about _with_stack_limit
	IF_VERBOSE_ASCODING_ERRORS (
	if (with_stack.size() >= _with_stack_limit) {
	    log_aserror("'With' stack depth (" SIZET_FMT ") "
			"exceeds the allowed limit for current SWF "
			"target version (" SIZET_FMT " for version %d)."
			" Don't expect this movie to work with all players.",
			with_stack.size()+1, _with_stack_limit,
			env.get_version());
	}
	);
	
	with_stack.push_back(entry);
	return true;
}

bool
ActionExec::delVariable(const std::string& name)
{
	VM& vm = VM::get(); // cache this ?
	std::string namei = name;
	if ( vm.getSWFVersion() < 7 ) {
	    boost::to_lower(namei, vm.getLocale());
	}
	
	return env.del_variable_raw(namei, with_stack);
}

void
ActionExec::setVariable(const std::string& name, const as_value& val)
{
	VM& vm = VM::get(); // cache this ?
	std::string namei = name;
	if ( vm.getSWFVersion() < 7 ) {
	    boost::to_lower(namei, vm.getLocale());
	}
	
	return env.set_variable(namei, val, getWithStack());
}

as_value
ActionExec::getVariable(const std::string& name)
{
	VM& vm = VM::get();

	std::string namei = name;
	if ( vm.getSWFVersion() < 7 ) {
	    boost::to_lower(namei, vm.getLocale());
	}
	
	return env.get_variable(namei, getWithStack());
}

void
ActionExec::setLocalVariable(const std::string& name_, const as_value& val)
{
	VM& vm = VM::get(); // cache this ?

	std::string name = name_;
	if ( vm.getSWFVersion() < 7 ) {
	    boost::to_lower(name, vm.getLocale());
	}

	if ( isFunction() ) {
	    // TODO: set local in the function object?
	    env.set_local(name, val);
	} else {
	    // TODO: set target member  ?
	    //       what about 'with' stack ?
	    env.set_variable(name, val);
	}
}

void
ActionExec::setObjectMember(as_object& obj, const std::string& var, const as_value& val)
{
	VM& vm = VM::get();

	if ( vm.getSWFVersion() < 7 ) {
	    std::string vari = var;
	    boost::to_lower(vari, vm.getLocale());
	    obj.set_member(vari, val);
	} else {
	    obj.set_member(var, val);
	}
	
}

bool
ActionExec::getObjectMember(as_object& obj, const std::string& var, as_value& val)
{
	VM& vm = VM::get();

	if ( vm.getSWFVersion() < 7 ) {
	    std::string vari = var;
	    boost::to_lower(vari, vm.getLocale());
	    return obj.get_member(vari, &val);
	} else {
	    return obj.get_member(var, &val);
	}

}

/*private*/
void
ActionExec::fixStackUnderrun(size_t required)
{
	size_t slots_left = env.stack_size() - _initial_stack_size;
	size_t missing = required-slots_left;

	//IF_VERBOSE_ASCODING_ERRORS(
	log_warning("Stack underrun: " SIZET_FMT " elements required, "
		SIZET_FMT "/" SIZET_FMT " available. "
		"Fixing by inserting " SIZET_FMT " undefined values on the"
		" missing slots.",
		required, _initial_stack_size, env.stack_size(),
		missing);
	//);

	env.padStack(_initial_stack_size, missing);
}

as_object*
ActionExec::getTarget()
{
	if ( ! with_stack.empty() )
	{
		//return const_cast<as_object*>(with_stack.back().object());
		return with_stack.back().object();
	}
	else
	{
		return env.get_target();
	}
}

} // end of namespace gnash


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
