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
//
//

// Implementation for ActionScript int object.

#ifndef GNASH_INT_AS_H
#define GNASH_INT_AS_H

#include <memory> // for auto_ptr

namespace gnash {

class as_object;
struct ObjectURI;

/// Initialize the global int class
void int_class_init(as_object& global, const ObjectURI& uri);

/// Return an int instance
std::auto_ptr<as_object> init_int_instance();

as_object* getintInterface();


}

#endif // GNASH_INT_AS_H
