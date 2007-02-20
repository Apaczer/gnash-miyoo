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
//
//

#ifndef GNASH_WITH_STACK_ENTRY_H
#define GNASH_WITH_STACK_ENTRY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "as_object.h" // for dtor visibility by boost::intrusive_ptr
#include "smart_ptr.h"

namespace gnash {

//
// with_stack_entry
//

/// The "with" stack is for Pascal-like with-scoping.
class with_stack_entry
{
public:	
	with_stack_entry()
		:
		_object(NULL),
		_block_end_pc(0)
	{
	}

	with_stack_entry(as_object* obj, size_t end)
		:
		_object(obj),
		_block_end_pc(end)
	{
	}

	size_t end_pc()
	{
		return _block_end_pc;
	}

	const as_object* object() const
	{
		return _object.get();
	}

	as_object* object() 
	{
		return _object.get();
	}

private:

	boost::intrusive_ptr<as_object>	_object;
	
	size_t _block_end_pc;

	

};

}	// end namespace gnash


#endif // GNASH_WITH_STACK_ENTRY_H


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
