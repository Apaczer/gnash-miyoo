// 
//   Copyright (C) 2007 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "BitmapMovieInstance.h"
#include "BitmapMovieDefinition.h"
#include "fill_style.h"
#include "shape.h" // for class path and class edge
#include "render.h" // for ::display

using namespace std;

namespace gnash {

BitmapMovieInstance::BitmapMovieInstance(BitmapMovieDefinition* def)
	:
	movie_instance(def, NULL)
{
  matrix mat;
  mat.concatenate_scale(20.0);
  
	// We need to assign a character id to the instance, or an assertion
	// will fail in character.cpp (parent==NULL || id != -1)
	character_def* chdef = def->get_character_def(1); 
	assert(chdef);
	boost::intrusive_ptr<character> ch = chdef->create_character_instance(this, 1);
	//log_msg("Created char in BitmapMovieInstance is a %s", typeid(*ch).name());
	int depth = 1+character::staticDepthOffset;
	place_character(ch.get(), depth, cxform(), mat, 1.0, 0);
}


} // namespace gnash
