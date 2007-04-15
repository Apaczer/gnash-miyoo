// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

/* $Id: character.cpp,v 1.35 2007/04/15 14:31:19 strk Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "character.h"
#include "sprite_instance.h"
#include "drag_state.h" // for do_mouse_drag (to be moved in movie_root)
#include "VM.h" // for do_mouse_drag (to be moved in movie_root)
#include "fn_call.h" // for shared ActionScript getter-setters
#include "GnashException.h" // for shared ActionScript getter-setters (ensure_character)
#include "ExecutableCode.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace gnash
{

// Initialize unnamed instance count
unsigned int character::_lastUnnamedInstanceNum=0;

/*protected static*/
std::string
character::getNextUnnamedInstanceName()
{
	std::stringstream ss;
	ss << "instance" << ++_lastUnnamedInstanceNum;
	return ss.str();
}


// TODO: this should likely go in movie_root instead !
void
character::do_mouse_drag()
{
	drag_state st;
	_vm.getRoot().get_drag_state(st);
	if ( this == st.getCharacter() )
	{
		// We're being dragged!
		int	x, y, buttons;
		get_root_movie()->get_mouse_state(x, y, buttons);

		point world_mouse(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
		if ( st.hasBounds() )
		{
			// Clamp mouse coords within a defined rect.
			// (it is assumed that drag_state keeps
			st.getBounds().clamp(world_mouse);
		}

		if (st.isLockCentered())
		{
		    matrix	world_mat = get_world_matrix();
		    point	local_mouse;
		    world_mat.transform_by_inverse(&local_mouse, world_mouse);

		    matrix	parent_world_mat;
		    if (m_parent != NULL)
			{
			    parent_world_mat = m_parent->get_world_matrix();
			}

		    point	parent_mouse;
		    parent_world_mat.transform_by_inverse(&parent_mouse, world_mouse);
					
		    // Place our origin so that it coincides with the mouse coords
		    // in our parent frame.
		    matrix	local = get_matrix();
		    local.set_translation( parent_mouse.m_x, parent_mouse.m_y );
		    set_matrix(local);
		}
		else
		{
			// Implement relative drag...
			static bool warned_relative_drag = false;
			if ( ! warned_relative_drag )
			{
				log_warning("FIXME: Relative drag unsupported");
		    		warned_relative_drag = true;
		    	}
		}
	}
}

matrix
character::get_world_matrix() const
{
	matrix m;
	if (m_parent != NULL)
	{
	    m = m_parent->get_world_matrix();
	}
	m.concatenate(get_matrix());

	return m;
}

cxform
character::get_world_cxform() const
{
	cxform	m;
	if (m_parent != NULL)
	{
	    m = m_parent->get_world_cxform();
	}
	m.concatenate(get_cxform());

	return m;
}

sprite_instance*
character::get_root_movie()
{
	assert(m_parent != NULL);
	assert(m_parent->get_ref_count() > 0);
	return m_parent->get_root_movie();
}

void
character::get_mouse_state(int& x, int& y, int& buttons)
{
	assert(m_parent != NULL);
	assert(m_parent->get_ref_count() > 0);
	get_parent()->get_mouse_state(x, y, buttons);
}

character*
character::get_relative_target_common(const std::string& name)
{
	if (name == "." || name == "this")
	{
	    return this;
	}
	else if (name == ".." || name == "_parent")
	{
		// Never NULL
		character* parent = get_parent();
		if ( ! parent )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			// AS code trying to access something before the root
			log_warning("ActionScript code trying to reference"
				" an unexistent parent with '..' "
				" (an unexistent parent probably only "
				"occurs in the root MovieClip)."
				" Returning a reference to top parent "
				"(probably the root clip).");
			);
			//parent = this;
			assert(this == get_root_movie());
			return this;
		}
		return parent;
	}
	else if (name == "_level0"
	     || name == "_root")
	{
		return get_root_movie();
	}

	return NULL;
}

void
character::set_invalidated()
{
  // Set the invalidated-flag of the parent. Note this does not mean that
  // the parent must re-draw itself, it just means that one of it's childs
  // needs to be re-drawn.
	if ( m_parent ) m_parent->set_child_invalidated(); 
  
  
	// Ok, at this point the instance will change it's
	// visual aspect after the
	// call to set_invalidated(). We save the *current*
	// position of the instance because this region must
	// be updated even (or first of all) if the character
	// moves away from here.
	// 
	if ( ! m_invalidated )
	{
		m_invalidated = true;
		
		// NOTE: we need to set snap_distance in order to avoid too tight 
		// invalidated ranges. The GUI chooses the appropriate distance in base
		// of various parameters but for this internal ranges list we don't know
		// that value. So we set snap_distance to some generic value and hope this
		// does not produce too many nor too coarse ranges. Note when calculating
		// the actual invalidated ranges the correct distance is used (but there
		// may be problems when the GUI chooses a smaller value). Needs to be 
		// fixed. 
		m_old_invalidated_ranges.snap_distance = 200.0; 
				
		m_old_invalidated_ranges.setNull();
		add_invalidated_bounds(m_old_invalidated_ranges, true);
	}

}

void
character::set_child_invalidated()
{
  if ( ! m_child_invalidated ) 
  {
    m_child_invalidated=true;
  	if ( m_parent ) m_parent->set_child_invalidated();
  } 
}

//---------------------------------------------------------------------
//
// Shared ActionScript getter-setters
//
//---------------------------------------------------------------------

as_value
character::x_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		matrix m = ptr->get_matrix();
		rv = as_value(TWIPS_TO_PIXELS(m.get_x_translation()));
	}
	else // setter
	{
		double newx = fn.arg(0).to_number(&(fn.env()));
		matrix m = ptr->get_matrix();
		m.set_x_translation(infinite_to_fzero(PIXELS_TO_TWIPS(newx)));
		ptr->set_matrix(m);
		ptr->transformedByScript(); // m_accept_anim_moves = false; 
	}
	return rv;

}

as_value
character::y_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		matrix m = ptr->get_matrix();
		rv = as_value(TWIPS_TO_PIXELS(m.get_y_translation()));
	}
	else // setter
	{
		double newy = fn.arg(0).to_number(&(fn.env()));
		matrix m = ptr->get_matrix();
		m.set_y_translation(infinite_to_fzero(PIXELS_TO_TWIPS(newy)));
		ptr->set_matrix(m);
		ptr->transformedByScript(); // m_accept_anim_moves = false; 
	}
	return rv;

}

as_value
character::xscale_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		matrix m = ptr->get_matrix();
		float xscale = m.get_x_scale();
		rv = as_value(xscale * 100); // result in percent
	}
	else // setter
	{
		matrix m = ptr->get_matrix();

		double scale_percent = fn.arg(0).to_number(&(fn.env()));

		// Handle bogus values
		if (isnan(scale_percent))
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror("Attempt to set _xscale to %g, refused",
                            scale_percent);
			);
                        return as_value();
		}
		else if (scale_percent < 0 )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror("Attempt to set _xscale to %g, use 0",
                            scale_percent);
			);
                        scale_percent = 0;
		}

		// input is in percent
		float scale = (float)scale_percent/100.f;
		ptr->set_x_scale(scale);
	}
	return rv;

}

as_value
character::yscale_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		matrix m = ptr->get_matrix();
		float yscale = m.get_y_scale();
		rv = as_value(yscale * 100); // result in percent
	}
	else // setter
	{
		matrix m = ptr->get_matrix();

		double scale_percent = fn.arg(0).to_number(&(fn.env()));

		// Handle bogus values
		if (isnan(scale_percent))
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror("Attempt to set _yscale to %g, refused",
                            scale_percent);
			);
                        return as_value();
		}
		else if (scale_percent < 0 )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror("Attempt to set _yscale to %g, use 0",
                            scale_percent);
			);
                        scale_percent = 0;
		}

		// input is in percent
		float scale = (float)scale_percent/100.f;
		ptr->set_y_scale(scale);
	}
	return rv;

}

as_value
character::xmouse_get(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	// Local coord of mouse IN PIXELS.
	int x, y, buttons;
	VM::get().getRoot().get_mouse_state(x, y, buttons);

	matrix m = ptr->get_world_matrix();

	point a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
	point b;
		
	m.transform_by_inverse(&b, a);

	return as_value(TWIPS_TO_PIXELS(b.m_x));
}

as_value
character::ymouse_get(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	// Local coord of mouse IN PIXELS.
	int x, y, buttons;
	VM::get().getRoot().get_mouse_state(x, y, buttons);

	matrix m = ptr->get_world_matrix();

	point a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
	point b;
		
	m.transform_by_inverse(&b, a);

	return as_value(TWIPS_TO_PIXELS(b.m_y));
}

as_value
character::alpha_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		rv = as_value(ptr->get_cxform().m_[3][0] * 100.f);
	}
	else // setter
	{
		// Set alpha modulate, in percent.
		cxform	cx = ptr->get_cxform();
		cx.m_[3][0] = infinite_to_fzero(fn.arg(0).to_number()) / 100.f;
		ptr->set_cxform(cx);
		ptr->transformedByScript(); // m_accept_anim_moves = false; 
	}
	return rv;

}

as_value
character::visible_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		rv = as_value(ptr->get_visible());
	}
	else // setter
	{
		ptr->set_visible(fn.arg(0).to_bool());
		ptr->transformedByScript(); // m_accept_anim_moves = false; 
	}
	return rv;

}

as_value
character::width_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	// Bounds are used for both getter and setter
	geometry::Range2d<float> bounds = ptr->getBounds();

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		double w = 0;
		if ( bounds.isFinite() )
		{
			matrix m = ptr->get_matrix();
			m.transform(bounds);
			assert(bounds.isFinite());
			w = TWIPS_TO_PIXELS(rint(bounds.width()));
		}
		rv = as_value(w);
	}
	else // setter
	{
		if ( ! bounds.isFinite() )
		{
			log_error("FIXME: can't set _width on character with null or world bounds");
			return rv;
		}

		double oldwidth = bounds.width();
		assert(oldwidth>0);

		double newwidth = PIXELS_TO_TWIPS(fn.arg(0).to_number(&(fn.env())));
		if ( newwidth <= 0 )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror("Setting _width=%g ?", newwidth/20);
			);
		}

		ptr->set_x_scale(newwidth/oldwidth);
	}
	return rv;
}

as_value
character::height_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	// Bounds are used for both getter and setter
	geometry::Range2d<float> bounds = ptr->getBounds();

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		double h = 0;
		if ( bounds.isFinite() )
		{
			matrix m = ptr->get_matrix();
			m.transform(bounds);
			assert(bounds.isFinite());
			h = TWIPS_TO_PIXELS(rint(bounds.height()));
		}
		rv = as_value(h);
	}
	else // setter
	{
		if ( ! bounds.isFinite() )
		{
			log_error("FIXME: can't set _height on character with null or world bounds");
			return rv;
		}

		double oldheight = bounds.height();
		assert(oldheight>0);

		double newheight = PIXELS_TO_TWIPS(fn.arg(0).to_number(&(fn.env())));
		if ( newheight <= 0 )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror("Setting _height=%g ?", newheight/20);
			);
		}

		ptr->set_y_scale(newheight/oldheight);
	}

	return rv;
}

as_value
character::rotation_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		// Verified against Macromedia player using samples/test_rotation.swf
		float	angle = ptr->get_matrix().get_rotation();

		// Result is CLOCKWISE DEGREES, [-180,180]
		angle *= 180.0f / float(M_PI);

		rv = as_value(angle);
	}
	else // setter
	{
		// @@ tulrich: is parameter in world-coords or local-coords?
		matrix m = ptr->get_matrix();

		// input is in degrees
		float rotation = (float) fn.arg(0).to_number(&(fn.env())) * float(M_PI) / 180.f;
		m.set_rotation(rotation);

		ptr->set_matrix(m);
		ptr->transformedByScript(); // m_accept_anim_moves = false; 
	}
	return rv;
}

as_value
character::parent_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	boost::intrusive_ptr<as_object> p = ptr->get_parent();
	as_value rv;
	if (p)
	{
		rv = as_value(p);
	}

	return rv;
}

void
character::set_event_handlers(const Events& copyfrom)
{
	for (Events::const_iterator it=copyfrom.begin(), itE=copyfrom.end();
			it != itE; ++it)
	{
		const event_id& ev = it->first;
		const BufferList& bufs = it->second;
		for (size_t i=0; i<bufs.size(); ++i)
		{
			const action_buffer* buf = bufs[i];
			assert(buf);
			add_event_handler(ev, *buf);
		}	
	}
}

void
character::add_event_handler(const event_id& id, const action_buffer& code)
{
	_event_handlers[id].push_back(&code);

	//log_msg("Setting handler for event %s", id.get_function_name().c_str());

	// Set the character as a listener iff the
	// kind of event is a KEY or MOUSE one 
	switch (id.m_id)
	{
		case event_id::KEY_PRESS:
			has_keypress_event();
			break;
		case event_id::MOUSE_UP:
		case event_id::MOUSE_DOWN:
		case event_id::MOUSE_MOVE:
			//log_msg("Registering character as having mouse events");
			has_mouse_event();
			break;
		default:
			break;
	}

	// todo: drop the character as a listener
	//       if it gets no valid handlers for
	//       mouse or keypress events.
}

std::auto_ptr<ExecutableCode>
character::get_event_handler(const event_id& id) const
{
	std::auto_ptr<ExecutableCode> handler;

	Events::const_iterator it = _event_handlers.find(id);
	if ( it == _event_handlers.end() ) return handler;

	assert(get_ref_count() > 0);
	boost::intrusive_ptr<character> this_ptr = const_cast<character*>(this);

	handler.reset( new EventCode(this_ptr, it->second) );
	return handler;
}

character::~character()
{
}

void
character::unload()
{
	_unloaded = true;
	//log_msg("Queuing unload event for character %p", this);
	queueEventHandler(event_id::UNLOAD);
	//on_event(event_id::UNLOAD);
}

void
character::queueEventHandler(const event_id& id)
{
	bool called=false;

	movie_root& root = VM::get().getRoot();

	std::auto_ptr<ExecutableCode> code ( get_event_handler(id) );
	if ( code.get() )
	{
		root.pushAction(code);
		called=true;
	}

	// This is likely wrong, we use it as a workaround
	// to the fact that we don't distinguish between
	// ActionScript and SWF defined events
	// (for example: onClipLoad vs. onLoad)
	//
	//if (called) return;

	// Check for member function.
	boost::intrusive_ptr<as_function> method = getUserDefinedEventHandler(id.get_function_name());
	if ( method )
	{
		root.pushAction(method, boost::intrusive_ptr<character>(this));
	}

}

boost::intrusive_ptr<as_function>
character::getUserDefinedEventHandler(const std::string& name) const
{
	std::string method_name = name;
	if ( _vm.getSWFVersion() < 7 )
	{
		boost::to_lower(method_name, _vm.getLocale());
	}

	as_value tmp;

	boost::intrusive_ptr<as_function> func;

	// const cast is needed due to getter/setter members possibly
	// modifying this object even when only get !
	if ( const_cast<character*>(this)->get_member(method_name, &tmp) )
	{
		func = tmp.to_as_function();
	}
	return func;
}

void
character::set_x_scale(float x_scale)
{
	matrix m = get_matrix();

	m.set_x_scale(x_scale);

	set_matrix(m);
	transformedByScript(); // m_accept_anim_moves = false; 
}

void
character::set_y_scale(float y_scale)
{
	matrix m = get_matrix();

	m.set_y_scale(y_scale);

	set_matrix(m);
	transformedByScript(); // m_accept_anim_moves = false; 
}

} // namespace gnash

// local variables:
// mode: c++
// indent-tabs-mode: t
// end:
