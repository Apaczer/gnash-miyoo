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

// 
//
//

// Implementation for ActionScript Number object.

#ifndef GNASH_NUMBER_H
#define GNASH_NUMBER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smart_ptr.h"

#include <memory> // for auto_ptr

namespace gnash {

class as_object;

/// Initialize the global Number class
void number_class_init(as_object& global);

/// Return a Number instance
boost::intrusive_ptr<as_object> init_number_instance(double val);

}

#endif // GNASH_NUMBER_H
