// RemoveObjectTag.cpp: RemoveObject* tag for Gnash.
//
//   Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "RemoveObjectTag.h"
#include "sprite_instance.h"
#include "swf.h" // for tag_type definition
#include "log.h"
#include "stream.h"

namespace gnash {
namespace SWF {

void
RemoveObjectTag::read(stream* in, tag_type tag)
{
	assert(tag == SWF::REMOVEOBJECT || tag == SWF::REMOVEOBJECT2);

	if (tag == SWF::REMOVEOBJECT)
	{
		// Older SWF's allow multiple objects at the same depth;
		// this m_id disambiguates.  Later SWF's just use one
		// object per depth.
		m_id = in->read_u16();
	}

	m_depth = in->read_u16()+character::staticDepthOffset;
}

void
RemoveObjectTag::execute(sprite_instance* m) const
{
	m->remove_display_object(m_depth, m_id);
}

/* public static */
void
RemoveObjectTag::loader(stream* in, tag_type tag, movie_definition* m)
{
    assert(tag == SWF::REMOVEOBJECT || tag == SWF::REMOVEOBJECT2);

    std::auto_ptr<RemoveObjectTag> t ( new RemoveObjectTag );
    t->read(in, tag);

    int depth = t->getDepth();

    IF_VERBOSE_PARSE
    (
	log_parse(_("  remove_object_2(%d)"), depth);
    );

    // Ownership transferred to movie_definition
    m->addControlTag(t.release());
}

} // namespace gnash::SWF
} // namespace gnash

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
