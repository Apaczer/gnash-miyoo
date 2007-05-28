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
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// 
//

/* $Id: BitmapMovieInstance.h,v 1.4 2007/05/28 15:41:04 ann Exp $ */

#ifndef GNASH_BITMAPMOVIEINSTANCE_H
#define GNASH_BITMAPMOVIEINSTANCE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "movie_instance.h" // for inheritance

// Forward declarations
namespace gnash
{
	class BitmapMovieDefinition;
}

namespace gnash
{


/// Instance of a BitmapMovieDefinition
class BitmapMovieInstance : public movie_instance
{

public:

	BitmapMovieInstance(BitmapMovieDefinition* def); 

	virtual ~BitmapMovieInstance() {}

	/// Do nothing on restart. Especially don't trash the DisplayList 
	//
	/// TODO: this is needed due to the implementation detail of 
	///       using the DisplayList to store our bitmap-filled
	///       shape. Using the _drawable instead, or overriding
	///       ::display to simply display our definition is likely
	///	  the best way to go instead (we'd also reuse the same
	///       bitmap info rather then creating new instances..)
	void restart() {}

};

} // end of namespace gnash

#endif // GNASH_BITMAPMOVIEINSTANCE_H
