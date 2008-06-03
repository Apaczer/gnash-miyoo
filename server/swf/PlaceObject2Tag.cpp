// PlaceObject2Tag.cpp:  for Gnash.
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

#include "PlaceObject2Tag.h"
#include "character.h"
#include "sprite_instance.h"
#include "swf_event.h"
#include "log.h"
#include "stream.h"
#include "filter_factory.h"

namespace gnash {
namespace SWF {

void
PlaceObject2Tag::readPlaceObject(stream& in)
{
    // Original place_object tag; very simple.
    in.ensureBytes(2 + 2);
    m_character_id = in.read_u16();
    m_depth = in.read_u16()+character::staticDepthOffset;
    m_matrix.read(in);

    IF_VERBOSE_PARSE
    (
            log_parse(_("  PLACEOBJECT: depth=%d(%d) char=%d"),
            m_depth, m_depth-character::staticDepthOffset,
            m_character_id);
            log_parse("%s", m_matrix);
    );

    if (in.get_position() < in.get_tag_end_position())
    {
        m_color_transform.read_rgb(in);

        IF_VERBOSE_PARSE
        (
            log_parse(_("  cxform:"));
            m_color_transform.print();
        );

    }
}

// read placeObject2 actions
void
PlaceObject2Tag::readPlaceActions(stream& in)
{
    int movie_version = _movie_def.get_version();

    in.ensureBytes(2);
    boost::uint16_t reserved = in.read_u16();
    IF_VERBOSE_MALFORMED_SWF (
        if ( reserved != 0 ) // must be 0
        {
            log_swferror(_("Reserved field in PlaceObject actions == %u (expected 0)"), reserved);
        }
    );
    
    // The logical 'or' of all the following handlers.
    if (movie_version >= 6)
    {
        in.ensureBytes(4);
        all_event_flags = in.read_u32();
    }
    else
    {
        in.ensureBytes(2);
        all_event_flags = in.read_u16();        
    }

    IF_VERBOSE_PARSE (
        log_parse(_("  actions: flags = 0x%X"), all_event_flags);
    );

    // Read swf_events.
    for (;;)
    {
        // Handle SWF malformations locally, by just prematurely interrupting
        // parsing of action events.
        // TODO: a possibly improvement would be using local code for the
        //       equivalent of ensureBytes which has the cost of a function
        //       call for itself plus a repeated useless function call for
        //       get_end_tag_position (which could be cached).
        //       
        try
        {
            // Read event.
            in.align();
    
            boost::uint32_t flags;
            if (movie_version >= 6)
            {
                in.ensureBytes(4);
                flags = in.read_u32();
            }
            else
            {
                in.ensureBytes(2);
                flags = in.read_u16();        
            }
    
            if (flags == 0) // no other events
            {
                break;
            }
    
            in.ensureBytes(4);
            boost::uint32_t event_length = in.read_u32();
            if ( in.get_tag_end_position() - in.get_position() <  event_length )
            {
                IF_VERBOSE_MALFORMED_SWF(
                log_swferror(_("swf_event::read(), "
                    "even_length = %u, but only %lu bytes left "
                    "to the end of current tag."
                    " Breaking for safety."),
                    event_length, in.get_tag_end_position() - in.get_position());
                );
                break;
            }
    
            boost::uint8_t ch = key::INVALID;
    
            if (flags & (1 << 17))  // has KeyPress event
            {
                in.ensureBytes(1);
                ch = in.read_u8();
                event_length--;
            }
    
            // Read the actions for event(s)
            // auto_ptr here prevents leaks on malformed swf
            std::auto_ptr<action_buffer> action ( new action_buffer(_movie_def) );
            action->read(in, in.get_position()+event_length);
            _actionBuffers.push_back(action.release()); // take ownership
    
            // If there is no end tag, action_buffer appends a null-terminator,
            // and fails this check. As action_buffer should check bounds, we
            // can just continue here.
            //assert (action->size() == event_length);
    
            // 13 bits reserved, 19 bits used
            const int total_known_events = 19;
            static const event_id s_code_bits[total_known_events] =
            {
                event_id::LOAD,
                event_id::ENTER_FRAME,
                event_id::UNLOAD,
                event_id::MOUSE_MOVE,
                event_id::MOUSE_DOWN,
                event_id::MOUSE_UP,
                event_id::KEY_DOWN,
                event_id::KEY_UP,
    
                event_id::DATA,
                event_id::INITIALIZE,
                event_id::PRESS,
                event_id::RELEASE,
                event_id::RELEASE_OUTSIDE,
                event_id::ROLL_OVER,
                event_id::ROLL_OUT,
                event_id::DRAG_OVER,
    
                event_id::DRAG_OUT,
                event_id(event_id::KEY_PRESS, key::CONTROL),
                event_id::CONSTRUCT
            };
    
            // Let's see if the event flag we received is for an event that we know of
    
            // Integrity check: all reserved bits should be zero
            if( flags >> total_known_events ) 
            {
                IF_VERBOSE_MALFORMED_SWF(
                log_swferror(_("swf_event::read() -- unknown / unhandled event type received, flags = 0x%x"), flags);
                );
            }
    
            // Aah! same action for multiple events !
            for (int i = 0, mask = 1; i < total_known_events; i++, mask <<= 1)
            {
                if (flags & mask)
                {
                    /// Yes, swf_event stores a reference to an element in _actionBuffers.
                    /// A case of remote ownership, but both swf_event and the actions
                    /// are owned by this class, so shouldn't be a problem.
                    action_buffer* thisAction = _actionBuffers.back();
                    std::auto_ptr<swf_event> ev ( new swf_event(s_code_bits[i], *thisAction) );
                    IF_VERBOSE_PARSE (
                    log_parse("---- actions for event %s", ev->event().get_function_name().c_str());
                    );
    
                    if (i == 17)    // has KeyPress event
                    {
                        ev->event().setKeyCode(ch);
                    }
    
                    m_event_handlers.push_back(ev.release());
                }
            }
        }
        catch (ParserException& what)
        {
            IF_VERBOSE_MALFORMED_SWF(
            log_swferror(_("Unexpected end of tag while parsing PlaceObject tag events"));
            );
            break;
        }
    } //end of for(;;)
}

// read SWF::PLACEOBJECT2
void
PlaceObject2Tag::readPlaceObject2(stream& in)
{
    in.align();

    in.ensureBytes(1 + 2); // PlaceObject2, depth

    // PlaceObject2 specific flags
    m_has_flags2 = in.read_u8();

    m_depth = in.read_u16()+character::staticDepthOffset;

    if ( hasCharacter( ))
    {
        in.ensureBytes(2);
        m_character_id = in.read_u16();
    }

    if ( hasMatrix() )
    {
        m_matrix.read(in);
    }

    if ( hasCxform() )
    {
        m_color_transform.read_rgba(in);
    }

    if ( hasRatio() )
    {
        in.ensureBytes(2);
        m_ratio = in.read_u16();
    }

    if ( hasName() ) 
    {
        in.read_string(m_name);
    }

    if ( hasClipDepth() )
    {
        in.ensureBytes(2);
        m_clip_depth = in.read_u16() + character::staticDepthOffset;
    }
    else
    {
        m_clip_depth = character::noClipDepthValue;
    }

    if ( hasClipActions() )
    {
        readPlaceActions(in);
    }

    IF_VERBOSE_PARSE (
        log_parse(_("  PLACEOBJECT2: depth = %d (%d)"), m_depth, m_depth-character::staticDepthOffset);
        if ( hasCharacter() ) log_parse(_("  char id = %d"), m_character_id);
        if ( hasMatrix() )
        {
            log_parse(_("  matrix: %s"), m_matrix);
        }
        if ( hasCxform() )
        {
            log_parse(_("  cxform:"));
            m_color_transform.print();
        }
        if ( hasRatio() ) log_parse(_("  ratio: %d"), m_ratio);
        if ( hasName() ) log_parse(_("  name = %s"), m_name.c_str());
        if ( hasClipDepth() ) log_parse(_("  clip_depth = %d (%d)"), m_clip_depth, m_clip_depth-character::staticDepthOffset);
        log_parse(_(" m_place_type: %d"), getPlaceType() );
    );

    //log_debug("place object at depth %i", m_depth);
}

// read SWF::PLACEOBJECT3
void
PlaceObject2Tag::readPlaceObject3(stream& in)
{
    in.align();

    in.ensureBytes(1 + 1 + 2); // PlaceObject2, PlaceObject3, depth

    // PlaceObject2 specific flags
    m_has_flags2 = in.read_u8();

    // PlaceObject3 specific flags, first 3 bits are unused
    m_has_flags3 = in.read_u8();
    
    boost::uint8_t blend_mode = 0;
    boost::uint8_t bitmask = 0;
    std::string className;

    m_depth = in.read_u16() + character::staticDepthOffset;

    IF_VERBOSE_PARSE (
        log_parse(_("  PLACEOBJECT3: depth = %d (%d)"), m_depth, m_depth-character::staticDepthOffset);
    // TODO: add more info here
    );

    if ( hasCharacter() )
    {
        in.ensureBytes(2);
        m_character_id = in.read_u16();
        IF_VERBOSE_PARSE (
        log_parse("   char:%d", m_character_id);
        );
    }

    if ( hasClassName() || ( hasImage() && hasCharacter() ) )
    {
        log_unimpl("PLACEOBJECT3 with associated class name");
        in.read_string(className);
        IF_VERBOSE_PARSE (
        log_parse("   className:%s", className.c_str());
        );
    }

    if ( hasMatrix() )
    {
        m_matrix.read(in);
    }

    if ( hasCxform() )
    {
        m_color_transform.read_rgba(in);
    }

    if ( hasRatio() ) 
    {
        in.ensureBytes(2);
        m_ratio = in.read_u16();
    }
    
    if ( hasName() )
    {
        in.read_string(m_name);
    }

    if ( hasClipDepth() )
    {
        in.ensureBytes(2);
        m_clip_depth = in.read_u16()+character::staticDepthOffset;
    }
    else
    {
        m_clip_depth = character::noClipDepthValue;
    }

    IF_VERBOSE_PARSE
    (
        if ( hasMatrix() ) {
            log_parse("   matrix: %s", m_matrix);
    }
        if ( hasCxform() ) {
            log_parse("   cxform:");
            m_color_transform.print();
    }
        if ( hasRatio() )  log_parse("   ratio:%d", m_ratio);
        if ( hasName() ) log_parse("   name:%s", m_name);

        if ( hasClipDepth() ) {
            log_parse("   clip_depth:%d(%d)", m_clip_depth,
                      m_clip_depth-character::staticDepthOffset);
        }
    );

    if ( hasFilters() )
    {
        Filters v; // TODO: Attach the filters to the display object.
        filter_factory::read(in, true, &v);
    }

    if ( hasBlendMode() )
    {
        in.ensureBytes(1);
        blend_mode = in.read_u8();
    }

    if ( hasBitmapCaching() )
    {
        // It is not certain that this actually exists, so if this reader
        // is broken, it is probably here!
        in.ensureBytes(1);
        bitmask = in.read_u8();
    }

    if ( hasClipActions() )
    {
        readPlaceActions(in);
    }

    IF_VERBOSE_PARSE (
        log_parse(_("  PLACEOBJECT3: depth = %d (%d)"), m_depth, m_depth-character::staticDepthOffset);
        if ( hasCharacter() ) log_parse(_("  char id = %d"), m_character_id);
        if ( hasMatrix() )
        {
            log_parse(_("  matrix: %s"), m_matrix);
        }
        if ( hasCxform() )
        {
            log_parse(_("  cxform:"));
            m_color_transform.print();
        }
        if ( hasRatio() ) log_parse(_("  ratio: %d"), m_ratio);
        if ( hasName() ) log_parse(_("  name = %s"), m_name);
        if ( hasClassName() ) log_parse(_("  class name = %s"), className);
        if ( hasClipDepth() ) log_parse(_("  clip_depth = %d (%d)"),
                                m_clip_depth, m_clip_depth-character::staticDepthOffset);
        log_parse(_(" m_place_type: %d"), getPlaceType());
    );

    //log_debug("place object at depth %i", m_depth);
}

void
PlaceObject2Tag::read(stream& in, tag_type tag)
{

    m_tag_type = tag;

    if (tag == SWF::PLACEOBJECT)
    {
        readPlaceObject(in);
    }
    else if ( tag == SWF::PLACEOBJECT2 )
    {
        readPlaceObject2(in);
    }
    else
    {
        readPlaceObject3(in);
    }
}

/// Place/move/whatever our object in the given movie.
void
PlaceObject2Tag::execute(sprite_instance* m, DisplayList& dlist) const
{
    switch ( getPlaceType() ) 
    {
      case PLACE:
          m->add_display_object(this, dlist);
      break;

      case MOVE:
          m->move_display_object(this, dlist);
      break;

      case REPLACE:
          m->replace_display_object(this, dlist);
      break;

      case REMOVE:
		  m->remove_display_object(this, dlist);
      break;
    }
}

PlaceObject2Tag::~PlaceObject2Tag()
{

    for(size_t i=0; i<m_event_handlers.size(); ++i)
    {
        delete m_event_handlers[i];
    }

    for(size_t i=0; i<_actionBuffers.size(); ++i)
    {
        delete _actionBuffers[i];
    }
}

/* public static */
void
PlaceObject2Tag::loader(stream* in, tag_type tag, movie_definition* m)
{
    assert(tag == SWF::PLACEOBJECT || tag == SWF::PLACEOBJECT2 || tag == SWF::PLACEOBJECT3);

    // TODO: who owns and is going to remove this tag ?
    PlaceObject2Tag* ch = new PlaceObject2Tag(*m);
    ch->read(*in, tag);

    m->addControlTag(ch);
}

} // namespace gnash::SWF
} // namespace gnash

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
