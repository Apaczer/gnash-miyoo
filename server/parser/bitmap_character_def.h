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
//

#ifndef GNASH_BITMAP_CHARACTER_DEF_H
#define GNASH_BITMAP_CHARACTER_DEF_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gnash.h" // for bitmap_info definition
#include "ref_counted.h" // for character_def inheritance
#include "types.h"
#include "container.h"
#include "utility.h"
#include "smart_ptr.h"

#include <cstdarg>
#include <cassert>
#include <memory> // for auto_ptr

namespace gnash {

/// Definition of a bitmap character
//
/// This includes:
///
///	- SWF::DEFINEBITS
///	- SWF::DEFINEBITSJPEG2
///	- SWF::DEFINEBITSJPEG3
///	- SWF::DEFINELOSSLESS
///	- SWF::DEFINELOSSLESS2
///
/// The definition currently only takes an image::rgb 
/// or image::rgba pointer. We should probably move
/// the methods for actually reading such tags instead.
///
/// One problem with this class is that it relies on the
/// availability of a render_handler in order to transform
/// image::rgb or image::rgba to a bitmap_info.
///
class bitmap_character_def : public ref_counted
{

public:

	/// Construct a bitmap_character_def from an image::rgb
	//
	/// NOTE: uses currently registered render_handler to
	///       create a bitmap_info, don't call before a renderer
	///	  has been registered
	///
 	bitmap_character_def(std::auto_ptr<image::rgb> image);

	/// Construct a bitmap_character_def from an image::rgba
	//
	/// NOTE: uses currently registered render_handler to
	///       create a bitmap_info, don't call before a renderer
	///	  has been registered
	///
 	bitmap_character_def(std::auto_ptr<image::rgba> image);

	gnash::bitmap_info* get_bitmap_info() {
		return _bitmap_info.get();
	}

private:

	boost::intrusive_ptr<gnash::bitmap_info> _bitmap_info;
};



}	// end namespace gnash


#endif // GNASH_BITMAP_CHARACTER_DEF_H


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
