// Point_as.h:  ActionScript "Point" class, for Gnash.
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

#ifndef GNASH_ASOBJ_POINT_H
#define GNASH_ASOBJ_POINT_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "as_object.h" // For visibility to auto_ptr
#include <memory> // for auto_ptr

namespace gnash {

class as_function;

/// Initialize the global Point class
void Point_class_init(as_object& global);

/// Return a Point instance (in case the core lib needs it)
boost::intrusive_ptr<as_object> init_Point_instance();

/// Return the Point constructor, for use by Rectangle 
as_function* getFlashGeomPointConstructor();

} // end of gnash namespace

// __GNASH_ASOBJ_POINT_H__
#endif
