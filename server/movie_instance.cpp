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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "movie_instance.h"
#include "movie_definition.h"
#include "movie_root.h"

#include <vector>
#include <string>
#include <cmath>

#include <functional> // for mem_fun, bind1st
#include <algorithm> // for for_each

using namespace std;

namespace gnash {

movie_instance::movie_instance(movie_definition* def, character* parent)
	:
	sprite_instance(def, this, parent, -1),
	_def(def)
{
}

void
movie_instance::stagePlacementCallback()
{
	//GNASH_REPORT_FUNCTION;

	assert ( get_root()->get_root_movie() == this );

	//_def->stopLoader();

	// Load first frame  (1-based index)
	size_t nextframe = 1;
	if ( !_def->ensure_frame_loaded(nextframe) )
	{
		IF_VERBOSE_MALFORMED_SWF(
		log_swferror("Frame " SIZET_FMT " never loaded. Total frames: "
				SIZET_FMT ".", nextframe, get_frame_count());
		);
	}

	// Invoke parent placement event handler
	sprite_instance::stagePlacementCallback();  
}

// Advance of an SWF-defined movie instance
void
movie_instance::advance()
{
	//GNASH_REPORT_FUNCTION;

	assert ( get_root()->get_root_movie() == this );

	//_def->stopLoader();

	// Load next frame if available (+2 as m_current_frame is 0-based)
	//
	// We do this inside advance_root to make sure
	// it's only for a root sprite (not a sprite defined
	// by DefineSprite!)
	size_t nextframe = min(get_current_frame()+2, get_frame_count());
	if ( !_def->ensure_frame_loaded(nextframe) )
	{
		IF_VERBOSE_MALFORMED_SWF(
		log_swferror("Frame " SIZET_FMT " never loaded. Total frames: "
				SIZET_FMT ".", nextframe, get_frame_count());
		);
	}

	advance_sprite(); 

	//_def->resumeLoader();
}

} // namespace gnash
