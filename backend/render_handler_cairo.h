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
//

/* $Id: render_handler_cairo.h,v 1.5 2007/05/28 15:40:57 ann Exp $ */

#ifndef BACKEND_RENDER_HANDLER_CAIRO_H
#define BACKEND_RENDER_HANDLER_CAIRO_H

#include "tu_config.h"

namespace gnash {
namespace renderer {

/// Cairo renderer namespace
namespace cairo {

/// Create a render handler
gnash::render_handler* create_handler();

/// Make sure to call this before starting display
void set_handle(cairo_t* handle);

} // namespace gnash::renderer::cairo
} // namespace gnash::renderer
} // namespace gnash

#endif // BACKEND_RENDER_HANDLER_CAIRO_H
