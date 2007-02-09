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

#include <string>
#include <sstream>
#include <map>
#include <boost/algorithm/string/case_conv.hpp>

#include "VM.h"
#include "rc.h"
#include "debugger.h"
#include "log.h"
#include "as_value.h"
#include "as_environment.h"
#include "swf.h"
#include "ASHandlers.h"
#include "movie_root.h"

namespace {
gnash::LogFile& dbglogfile = gnash::LogFile::getDefaultInstance();
gnash::RcInitFile& rcfile = gnash::RcInitFile::getDefaultInstance();
const gnash::SWF::SWFHandlers& ash = gnash::SWF::SWFHandlers::instance();
}

using namespace std;
using namespace gnash::SWF;

namespace gnash 
{

const char *as_arg_strs[] = {
    "ARG_NONE",
    "ARG_STR",
    "ARG_HEX",
    "ARG_U8",
    "ARG_U16",
    "ARG_S16",
    "ARG_PUSH_DATA",
    "ARG_DECL_DICT",
    "ARG_FUNCTION2"
};
const char *state_strs[] = { "rw", "w", "r" };

Debugger::Debugger() 
    : _enabled(false), _tracing(false), _state(NONE), _skipb(0), _env(0), _pc(0)
{
//    GNASH_REPORT_FUNCTION;
}

Debugger::~Debugger()
{
//    GNASH_REPORT_FUNCTION;
}

Debugger&
Debugger::getDefaultInstance()
{
	static Debugger dbg;
	return dbg;
}

void
Debugger::usage()
{
//    GNASH_REPORT_FUNCTION;
    cerr << "Gnash Debugger" << endl;
    cerr << "\t? - help" << endl;
    cerr << "\tq - Quit" << endl;
    cerr << "\tt - Toggle Trace Mode" << endl;
    cerr << "\tc - Continue" << endl;
    cerr << "\td - Dissasemble current line" << endl;
    // info commands
    cerr << "\ti i - Dump Movie Info" << endl;
    cerr << "\ti f - Dump Stack Frame" << endl;
    cerr << "\ti s - Dump symbols" << endl;
    cerr << "\ti g - Global Regs" << endl;
    cerr << "\ti l - Local Variables" << endl;
    cerr << "\ti w - Dump watch points" << endl;
    cerr << "\ti b - Dump break points" << endl;
    // break and watch points
    cerr << "\tw name [r:w:b] - set variable watchpoint" << endl;
    cerr << "\tw name d - delete variable watchpoint" << endl;
    cerr << "\tb name - set function break point" << endl;
    // change data
    cerr << "\tset var [name] [value] - set a local variable" << endl;
    cerr << "\tset stack [index] [value] - set a stack entry" << endl;
    cerr << "\tset reg [index] [value] - set a local register" << endl;
    cerr << "\tset global [index] [value] - set a global register" << endl;
}

void
Debugger::console()
{
//    GNASH_REPORT_FUNCTION;
    console(*_env);
}

// Open a user console for debugging commands. The abbreviations and
// commands are roughly based on GDBs.
void
Debugger::console(as_environment &env)
{
//    GNASH_REPORT_FUNCTION;

    // If the debugger isn't enabled, there is nothing to do.
    if (!this->isEnabled()) {
 	return;
    }
    
    if (!_env) {
	cerr << "WARNING: environment not set yet";
	cerr << "\nOnly watch point commands will work untill after you continue." << endl;
    }
//     if (this->isContinuing()) {
//  	this->hitBreak();
//     } else {
    string action;
    string var, val, sstate;
    int index;
    Debugger::watch_state_e wstate;
    bool keep_going = true;

    dbglogfile << "Debugger enabled >> " << endl;
    while (keep_going) {
	cerr << "gnashdbg> ";
	cin >> action;
	switch (action[0]) {
	    // Quit Gnash.
	  case 'Q':
	  case 'q':
	      exit(0);
	      break;
	      // Continue executing.
	  case 'c':
	      this->go(10);
	      keep_going = false;
	      break;
	      // Change the value of a variable on the stack
	  case 's':
	      if (action == "set") {
		  cin >> var;
		  as_value asval;
		  switch(var[0]) {
		        // change a parameter on the stack
		    case 's':
			cin >> index >> val;
			asval.set_std_string(val);
			this->changeStackValue(index, asval);
			break;
			// change a local variable
		    case 'v':
			cin >> var >> val;
			asval.set_std_string(val);
			this->changeLocalVariable(var, asval);
			break;
			// change a local register
		    case 'r':
			cin >> index >> val;
			asval.set_std_string(val);
			this->changeLocalRegister(index, asval);
			break;
			// change a global register
		    case 'g':
			cin >> index >> val;
			asval.set_std_string(val);
			this->changeGlobalRegister(index, asval);
			break;
		    default:
			break;
		  }
	      }
	      break;
	      // Informational commands.
	  case 'i':
	      cin >> var;
	      switch (var[0]) {
		case 'd':
		    this->dissasemble();
		  break;
		case 'i':
		    this->dumpMovieInfo();
		    break;
		case 'b':
		    this->dumpBreakPoints();
		    break;
		case 'w':
		    this->dumpWatchPoints();
		    break;
		case 'r':
		    this->dumpLocalRegisters(env);
		    break;
		case 'g':
		    this->dumpGlobalRegisters(env);
		    break;
		case 'l':
		    this->dumpLocalVariables(env);
		    break;
		case 'f':
		    this->dumpStackFrame(env);
		    break;
		case 's':
		    this->dumpSymbols();
		    break;
	      };
	      break;
	      // Tracing mode. This prints a disasembly every time
	      // Gnash stops at a watch or break point.
	  case 't':
	      if (this->isTracing()) {
		  this->traceMode(false);
	      } else {
		  this->traceMode(true);
	      }
	      break;
	      // Print a help screen
	  case '?':
	      this->usage();
	      break;
	      // Set a watchpoint
	  case 'w':
	      cin >> var >> sstate;
	      switch (sstate[0]) {
		  // Break on reads
		case 'r':
		    wstate = Debugger::READS;
		    break;
		    // Break on writes
		case 'w':
		    wstate = Debugger::WRITES;
		    break;
		    // Break on any accesses
		case 'b':
		default:
		    wstate = Debugger::BOTH;
		    break;
	      };
	      // Delete a watch point
	      if (sstate[0] == 'd') {
		  this->removeWatchPoint(var);
	      } else {
		  this->setWatchPoint(var, wstate);
	      }
	      break;
	  default:
	      break;
	};
    }
}

void
Debugger::dumpMovieInfo()
{
//    GNASH_REPORT_FUNCTION;
    if (VM::isInitialized()) {
	VM& vm = VM::get();
	movie_root &mr = vm.getRoot();
	int x, y, buttons;
	mr.get_mouse_state(x, y, buttons);
	
	cerr << "Movie is Flash v" << vm.getSWFVersion() << endl;
	cerr << "Mouse coordinates are: X=" << x << ", Y=" << y << endl;
	vm.getGlobal()->dump_members();
    }
}

void
Debugger::dissasemble()
{
//    GNASH_REPORT_FUNCTION;
    this->dissasemble(_pc);
}

void
Debugger::dissasemble(const unsigned char *data)
{
//    GNASH_REPORT_FUNCTION;
    as_arg_t fmt = ARG_HEX;
    action_type	action_id = static_cast<action_type>(data[0]);
    int val = 0;
    string str;
    unsigned char num[10];
    memset(num, 0, 10);

    if (_pc == 0) {
    }
    
    // Show instruction.
    if (action_id > ash.lastType()) {
	cerr << "WARNING: <unknown>[0x" << action_id  << "]" << endl;
    } else {
	if (ash[action_id].getName().size() > 0) {
	    cerr << "Action: " << (void *)action_id << ": " << ash[action_id].getName().c_str() << endl;
	} else {
	    cerr << "Action: " << (void *)action_id << ": " << "WARNING: unknown ID" << endl;
	}
	fmt = ash[action_id].getArgFormat();
    }

    // If we get a ActionPushData opcode, get the parameters
    if (action_id & 0x80) {
	int length = data[1] | (data[2] << 8);
	cerr << "\tArg format is: " << as_arg_strs[fmt] << " Length is: " << length << endl;
	switch (fmt) {
	  case ARG_NONE:
	      dbglogfile << "ERROR: No format flag!" << endl;
	      break;
	  case ARG_STR:
	      if ((length == 1) && (data[3] == 0)) {
		  str = "null";
	      } else {
		  for (int i = 0; i < length; i++) {
		      if (data[3 + i] != 0) {
			  str += data[3 + i];
		      } else {
			  break;
		      }
		  }
	      }
	      cerr << "Got string (" << length << " bytes): " << "\"" << str << "\"" << endl;
	      break;
	  case ARG_HEX:
	      for (int i = 0; i < length; i++) {
		  hexify(num, (const unsigned char *)&data[3 + i], 1, false);
		  cerr << "0x" << num << " ";
	      }
	      cerr << endl;
//	      cerr << "FIXME: Got hex: " << num << endl;
	      break;
	  case ARG_U8:
	      val = data[3];
//	      cerr << "FIXME: Got u8: " << val << endl;
	      break;
	  case ARG_U16:
	      val = data[3] | (data[4] << 8);
//	      cerr << "FIXME: Got u16: " << val << endl;
	      break;
	  case ARG_S16:
	      val = data[3] | (data[4] << 8);
	      if (val & 0x8000) {
		  val |= ~0x7FFF;	// sign-extend
	      }
//	      cerr << "FIXME: Got s16: " << val << endl;
	      break;
	  case ARG_PUSH_DATA:
	      break;
	  case ARG_DECL_DICT:
	      break;
	  case ARG_FUNCTION2:
	      break;
	  default:
	      dbglogfile << "ERROR: No format flag!" << endl;
	      break;
	} // end of switch(fmt)
    }
}

void
Debugger::setBreakPoint(std::string & /* asname */)
{
//    GNASH_REPORT_FUNCTION;
    dbglogfile << "WARNING: " << __PRETTY_FUNCTION__ << " is unimplemented!" << endl;
}

void
Debugger::removeBreakPoint(std::string &var)
{
//    GNASH_REPORT_FUNCTION;
    
    string name;
    std::map<std::string, bool>::const_iterator it;
    it = _breakpoints.find(var);
    if (it != _breakpoints.end()) {
	_breakpoints.erase(var);
    }
}

void
Debugger::dumpBreakPoints()
{
//    GNASH_REPORT_FUNCTION;
    string name;
    bool val;
    std::map<std::string, bool>::const_iterator it;
    
    for (it=_breakpoints.begin(); it != _breakpoints.end(); it++) {
	name = it->first;
	val = it->second;
	if (name.size()) {
	    dbglogfile << name << endl;
	}
    }
}

void
Debugger::setWatchPoint(std::string &var, watch_state_e state)
{
//    GNASH_REPORT_FUNCTION;
    _watchpoints[var] = state;
    dbglogfile << "Setting watchpoint for variable: \"" << var << "" << endl;
}

void
Debugger::removeWatchPoint(std::string &var)
{
//    GNASH_REPORT_FUNCTION;
    
    string name;
    std::map<std::string, watch_state_e>::const_iterator it;
    it = _watchpoints.find(var);
    if (it != _watchpoints.end()) {
	_watchpoints.erase(var);
    }
}

void
Debugger::dumpWatchPoints()
{
//    GNASH_REPORT_FUNCTION;
    string name;
    watch_state_e state;
    int index = 0;
    std::map<std::string, watch_state_e>::const_iterator it;
    
    for (it=_watchpoints.begin(); it != _watchpoints.end(); it++) {
	name = it->first;
	state = it->second;
	index++;
	if (name.size()) {
	    cerr << "\twatch #" << index << ": " << name
		 << " \"" << state_strs[state] << "\"" << endl;
	}
    }
}

bool
Debugger::matchWatchPoint(std::string &var, watch_state_e state)
{
//    GNASH_REPORT_FUNCTION;
    std::map<std::string, watch_state_e>::const_iterator it;
    it =_watchpoints.find(var);
    if (it == _watchpoints.end()) {
//	dbglogfile << "No Match for variable \"" << var << "\"" << endl;
 	return false;
    } else {
	if (state == _watchpoints[var]) {
	    dbglogfile << "Matched for variable \"" << var << "\": \""
		       << state_strs[state] << "\"" << endl;
	    this->console();
	    return true;
	}
	return false;
    }
}

// These functions manipulate the environment stack
void
Debugger::dumpStackFrame()
{
//    GNASH_REPORT_FUNCTION;
    if (_env == 0) {
	dbglogfile << "WARNING: environment not set in " << __PRETTY_FUNCTION__ << endl;
	return;
    }
    this->dumpStackFrame(*_env);
}

// Change the value of a parameter on the stack
void
Debugger::changeStackValue(int index, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    changeStackValue(*_env, index, val);
}

void
Debugger::changeStackValue(as_environment &env, int index, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    if (!_env) {
	dbglogfile << "WARNING: environment not set in " << __PRETTY_FUNCTION__ << endl;
	return;
    }
    if (env.stack_size()) {
	env.m_stack[index] = val;
    }
}

void
Debugger::dumpStackFrame(as_environment &env)
{
//    GNASH_REPORT_FUNCTION;    
    if (!_env) {
	dbglogfile << "WARNING: environment not set in " << __PRETTY_FUNCTION__ << endl;
	return;
    }
    if (env.stack_size()) {
        dbglogfile << "Stack Dump of: " << (void *)&env << endl;
        for (unsigned int i=0, n=env.stack_size(); i<n; i++) {    
            cerr << "\t" << i << ": ";
	    as_value val = env.m_stack[i];
// FIXME: we want to print the name of the function
//  	    if (val.is_as_function()) {
// //		cerr << val.get_symbol_handle() << endl;
// 		string name = this->lookupSymbol(val.to_object());
// 		if (name.size()) {
// 		    cerr << name << " ";
// 		}
// 	    }
            cerr << env.m_stack[i].to_string();
	    if (val.is_object()) {
		string name = this->lookupSymbol(val.to_object());
		if (name.size()) {
		    cerr << " \"" << name << "\"";
		}
		cerr << " has #" << val.to_object()->get_ref_count() << " references";
	    }
	    cerr << endl;
	}
    }
    else {
	dbglogfile << "Stack Dump of 0x" << (void *)&env << ": empty" << endl;
    }
}

void
Debugger::dumpLocalRegisters()
{
//    GNASH_REPORT_FUNCTION;
    this->dumpLocalRegisters(*_env);
}

void
Debugger::dumpLocalRegisters(as_environment &env)
{
//    GNASH_REPORT_FUNCTION;
    if (!_env) {
	dbglogfile << "WARNING: environment not set in " << __PRETTY_FUNCTION__ << endl;
	return;
    }
    size_t n=env.num_local_registers();
    if ( ! n ) {
        return;
    }
    dbglogfile << "Local Register Dump: " << endl;
    for (unsigned int i=0; i<n; i++) {
        cerr << "\treg #" << i << ": \"" << env.local_register(i).to_string() << "\"" << endl;
    }
}

void
Debugger::dumpGlobalRegisters()
{
//    GNASH_REPORT_FUNCTION;
    this->dumpGlobalRegisters(*_env);
}

void
Debugger::dumpGlobalRegisters(as_environment &env)
{
//    GNASH_REPORT_FUNCTION;  
    if (!_env) {
	dbglogfile << "WARNING: environment not set in " << __PRETTY_FUNCTION__ << endl;
	return;
    }
    std::string registers;
    stringstream ss;
    dbglogfile << "Global Registers Dump:" << endl;
    for (unsigned int i=0; i<4; ++i) {
	ss << "\treg #" << i << ": \"";
	ss << env.global_register(i).to_std_string() << "\"" << endl;
    }
    cerr << ss.str().c_str() << endl;
}

    // Change the value of a local variable
void
Debugger::changeLocalVariable(std::string &var, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    changeLocalVariable(*_env, var, val);
}

void
Debugger::changeLocalVariable(as_environment &env, std::string &var, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    env.set_local(var, val);
}

// Change the value of a local variable
void
Debugger::changeLocalRegister(int index, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    this->changeLocalRegister(*_env, index, val);
}

void
Debugger::changeLocalRegister(as_environment &env, int index, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    if (index <= env.num_local_registers()) {
	env.set_local_register(static_cast<uint8_t>(index), val);
    }
}   

// Change the value of a global variable
void
Debugger::changeGlobalRegister(int index, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    this->changeLocalRegister(*_env, index, val);
}

void
Debugger::changeGlobalRegister(as_environment &env, int index, as_value &val)
{
//    GNASH_REPORT_FUNCTION;
    env.set_global_register(index, val);
}   

void
Debugger::dumpLocalVariables()
{
//    GNASH_REPORT_FUNCTION;
    this->dumpLocalVariables(*_env);
}

void
Debugger::dumpLocalVariables(as_environment &env)
{
//    GNASH_REPORT_FUNCTION;
    if (!_env) {
	dbglogfile << "WARNING: environment not set in " << __PRETTY_FUNCTION__ << endl;
	return;
    }
    int index = 0;
    dbglogfile << "Local variable Dump:" << endl;
    as_environment::frame_slot slot;
    for (size_t i = 0, n=env.get_local_frame_top(); i < n; ++i) {
        slot  = env.m_local_frames[i];
	string var = slot.m_value.to_std_string();
	cerr << "\tvar #" << index << ": ";
	if (slot.m_name.size()) {
	    cerr << slot.m_name << " = \"" << var << "\"" << endl;
	} else {
	    cerr << "\"null\"" << " = " << var << endl;
	}
	index++;
    }
}

/// Get the address associated with a name
void *
Debugger::lookupSymbol(std::string &name)
{
//    GNASH_REPORT_FUNCTION;
    if (_symbols.size()) {
	VM& vm = VM::get(); // cache this ?
	std::string namei = name;
	if ( vm.getSWFVersion() < 7 ) {
	    boost::to_lower(namei, vm.getLocale());
	}
	std::map<void *, std::string>::const_iterator it;
	for (it=_symbols.begin(); it != _symbols.end(); it++) {
	    if (it->second == namei) {
//		dbglogfile << "Found symbol " << namei.c_str() << " at address: " << it->first << endl;
		return it->first;
	    }
	}
    }
    return NULL; 
}

void
Debugger::addSymbol(void *ptr, std::string name)
{
//    GNASH_REPORT_FUNCTION;
    VM& vm = VM::get(); // cache this ?
    std::string namei = name;
    if ( vm.getSWFVersion() < 7 ) {
	boost::to_lower(namei, vm.getLocale());
    }
    if (namei.size() > 1) {
	//dbglogfile << "Adding symbol " << namei << " at address: " << ptr << endl;
	_symbols[ptr] = namei;
    }
    
}

/// Get the name associated with an address
std::string
Debugger::lookupSymbol(void *ptr)
{
//    GNASH_REPORT_FUNCTION;

    string str;
    if (_symbols.size()) {
	std::map<void *, std::string>::const_iterator it;
	it = _symbols.find(ptr);
	dbglogfile.setStamp(false);
	if (it != _symbols.end()) {
//	    dbglogfile << "Found symbol " << it->second.c_str() << " at address: " << ptr << endl;
	    str = it->second;
// 	} else {
// 	    dbglogfile << "No symbol found for address " << ptr << endl;
	}
    }
    dbglogfile.setStamp(false);
    return str;
}

void
Debugger::dumpSymbols()
{
//    GNASH_REPORT_FUNCTION;
    int index = 0;
    std::map<void *, std::string>::const_iterator it;    
    for (it=_symbols.begin(); it != _symbols.end(); it++) {
	string name = it->second;
	void *addr = it->first;
	if (name.size()) {
	    cerr << "\tsym #" << index << ": " << name << " <" << addr << ">" << endl;
	}
	index++;
    }
}

} // end of gnash namespace

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
