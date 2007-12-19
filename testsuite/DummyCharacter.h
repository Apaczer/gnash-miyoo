// 
//   Copyright (C) 2007 Free Software Foundation, Inc.
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


#ifndef GNASH_DUMMYCHARACTER_H
#define GNASH_DUMMYCHARACTER_H

#include "character.h" // for inheritance
#include "rect.h" // for composition
#include "movie_instance.h" // for create_movie_instance
#include "snappingrange.h" // for InvalidatedRanges typedef (don't like it)

#include <string>
#include <memory> // for auto_ptr

// Forward declarations
namespace gnash {
}

namespace gnash
{

/// A dummy character instance, for use by unit tests
//
/// This class provides implementation of all virtual
/// methods of movie_definition by returning user-defined
/// values for XXXXXXXXXXXXXXXXXXXXXXXXXX etc..
///
///
class DummyCharacter : public character
{

public:

	DummyCharacter(character* parent)
		:
		character(parent, parent ? 0 : -1)
	{
	}

	void add_invalidated_bounds(InvalidatedRanges& /*bounds*/, bool /*force*/) {}

};

} // namespace gnash

#endif // GNASH_DUMMYCHARACTER_H
