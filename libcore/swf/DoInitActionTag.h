// 
//   Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef GNASH_SWF_DOINITACTIONTAG_H
#define GNASH_SWF_DOINITACTIONTAG_H

#include "ControlTag.h" // for inheritance
#include "SWF.h" // for TagType definition
#include "action_buffer.h" // for composition
#include "MovieClip.h" // for inlines
#include "SWFStream.h" // for inlines

// Forward declarations
namespace gnash {
    class movie_definition;
}

namespace gnash {
namespace SWF {

/// SWF Tag DoInitAction (59)
//
/// Thin wrapper around action_buffer.
///
class DoInitActionTag : public ControlTag
{
public:

    DoInitActionTag(SWFStream& in, movie_definition& md, int cid)
        :
        _buf(md),
        _cid(cid)
    {
        read(in);
    }

    /// Execute 'state' tags.
    //
    /// State tags change the current state of a MovieClip. They are executed
    /// even for skipped frames to ensure that the state is consistent.
    //
    /// Even though DoInitAction tags contain ActionScript, they are considered
    /// to be state tags. They are executed only once.
    virtual void executeState(MovieClip* m, DisplayList& /*dlist*/) const {
        m->execute_init_action_buffer(_buf, _cid);
    }

    static void loader(SWFStream& in, TagType tag, movie_definition& m,
            const RunResources& /*r*/)
    {
        if (m.isAS3()) {
            IF_VERBOSE_MALFORMED_SWF(
                log_swferror("SWF contains DoInitAction tag, but is an "
                    "AS3 SWF!");
            );
            throw ParserException("DoInitAction tag found in AS3 SWF!");
        }
        
        in.ensureBytes(2);
        const boost::uint16_t cid = in.read_u16();
        
        // Tags should only be executed for exported characters. Current
        // tests show that the tag is only ever executed if the export
        // is known at parsing time.
        //
        // If this is true, there is no need to parse or store the tag. If
        // it's not true, this check will have to be done at runtime.
        DoInitActionTag* da = new DoInitActionTag(in, m, cid);

        IF_VERBOSE_PARSE(
            log_parse(_("  tag %d: do_init_action_loader"), tag);
            log_parse(_("  -- init actions for sprite %d"), cid);
        );

        m.addControlTag(da); // ownership transferred
    }

private:

    /// Read a DoInitAction block from the SWFStream
    //
    void read(SWFStream& in)
    {
        _buf.read(in, in.get_tag_end_position());
    }


    action_buffer _buf;

    // id of referenced DisplayObject definition
    int _cid;
};

} // namespace gnash::SWF
} // namespace gnash


#endif // GNASH_SWF_DOINITACTIONTAG_H


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

