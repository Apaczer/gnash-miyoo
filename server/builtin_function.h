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

#ifndef __GNASH_BUILTIN_FUNCTION_H__
#define __GNASH_BUILTIN_FUNCTION_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "as_function.h" // for inheritance

#include <cassert>

namespace gnash {

typedef as_value (*as_c_function_ptr)(const fn_call& fn);


/// Any built-in function/class should be of this type
class builtin_function : public as_function
{

public:

	/// Construct a builtin function/class
	//
	///
	/// @param func
	///	The C function to call when this as_function is invoked.
	/// 	For classes, the function pointer is the constructor.
	///
	/// @param iface
	///	The interface of this class (will be inherited by
	///	instances of this class)
	/// 	If the given interface is NULL a default one
	/// 	will be provided, with constructor set as 'this'.
	///
	builtin_function(as_c_function_ptr func, as_object* iface=NULL)
		:
		as_function(iface),
		_func(func)
	{
		init_member("constructor", this);
	}

	/// Invoke this function or this Class constructor
	virtual as_value operator()(const fn_call& fn)
	{
		assert(_func);
		return _func(fn);
	}

	bool isBuiltin()  { return true; }

private:

	as_c_function_ptr _func;
};

} // end of gnash namespace

// __GNASH_BUILTIN_FUNCTION_H__
#endif

