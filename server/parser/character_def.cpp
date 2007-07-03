// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

/* $Id: character_def.cpp,v 1.7 2007/07/03 05:46:02 strk Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "character_def.h"
#include "generic_character.h"
#include "render_handler.h" // for destruction of render_cache_manager

namespace gnash
{

character*
character_def::create_character_instance(character* parent, int id)
{
	return new generic_character(this, parent, id);
}

character_def::~character_def()
{
	delete m_render_cache;
}

} // end of namespace gnash

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
