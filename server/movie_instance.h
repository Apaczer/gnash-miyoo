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

// Stateful live Movie instance 


#ifndef GNASH_MOVIE_INSTANCE_H
#define GNASH_MOVIE_INSTANCE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>

#include "sprite_instance.h" // for inheritance
#include "smart_ptr.h" // for composition
#include "movie_definition.h" // for dtor visibility by smart ptr

// Forward declarations
namespace gnash {
	class movie_root; 
	class character; 
	class movie_def_impl;
}

namespace gnash
{

/// Stateful Movie object (a special kind of sprite)
class movie_instance : public sprite_instance
{

public:

	// We take a generic movie_definition to allow
	// for subclasses for other then SWF movies
	movie_instance(movie_definition* def, character* parent);

	virtual ~movie_instance() {}

	virtual void advance(float delta_time);

	// Could be implemented in sprite_instance too,
	// returning m_root like it is done for get_root_movie...
	virtual movie_instance* get_root()
	{
		return this;
	}

	//virtual sprite_instance* get_root_movie()
	//{
		//return this;
	//}

private:

	boost::intrusive_ptr<movie_definition> _def;
};


} // end of namespace gnash

#endif // GNASH_MOVIE_INSTANCE_H
