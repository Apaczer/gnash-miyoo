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

/* $Id: DefineFontAlignZonesTag.h,v 1.6 2008/01/21 20:56:02 rsavoye Exp $ */

#ifndef GNASH_SWF_DEFINEFONTALIGNZONESTAG_H
#define GNASH_SWF_DEFINEFONTALIGNZONESTAG_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

// Forward declarations
namespace gnash {
	class movie_definition;
	class stream;
}

namespace gnash {
namespace SWF {

class DefineFontAlignZonesTag {
public:

	enum {
		THIN = 0,
		MEDIUM = 1,
		THICK = 2
	};

	DefineFontAlignZonesTag(movie_definition& m, stream& in);

	static void loader(stream* in, tag_type tag, movie_definition* m);

private:

	unsigned short _font2_id_ref;

	unsigned _csm_table_int:2;

};

} // namespace gnash::SWF
} // namespace gnash


#endif // GNASH_SWF_DEFINEFONTALIGNZONESTAG_H


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
