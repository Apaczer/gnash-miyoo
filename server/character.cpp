// character.cpp:  ActionScript Character class, for Gnash.
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
// 


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h" // USE_SWFTREE
#endif

#include "smart_ptr.h" // GNASH_USE_GC
#include "character.h"
#include "sprite_instance.h"
#include "drag_state.h" // for do_mouse_drag (to be moved in movie_root)
#include "VM.h" // for do_mouse_drag (to be moved in movie_root)
#include "fn_call.h" // for shared ActionScript getter-setters
#include "GnashException.h" // for shared ActionScript getter-setters (ensure_character)
#include "render.h"  // for bounds_in_clipping_area()
#include "ExecutableCode.h"
#include "namedStrings.h"

#ifdef USE_SWFTREE
# include "tree.hh"
#endif

#include <boost/algorithm/string/case_conv.hpp>

#undef set_invalidated

namespace gnash
{

// Define static const members or there will be linkage problems.
const int character::staticDepthOffset;
const int character::removedDepthOffset;
const int character::noClipDepthValue;
const int character::dynClipDepthValue;

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

#if 0
void
character::get_mouse_state(int& x, int& y, int& buttons)
{
	assert(m_parent != NULL);
#ifndef GNASH_USE_GC
	assert(m_parent->get_ref_count() > 0);
#endif // GNASH_USE_GC
	get_parent()->get_mouse_state(x, y, buttons);
}
#endif

as_object*
character::get_path_element_character(string_table::key key)
{
	if (key == NSV::PROP_uROOT)
	{
		// getAsRoot() will handle _lockroot 
		return const_cast<sprite_instance*>(getAsRoot());
	}

	const std::string& name = _vm.getStringTable().value(key);

	if (name == ".." || key == NSV::PROP_uPARENT )
	{
		// Never NULL
		character* parent = get_parent();
		if ( ! parent )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			// AS code trying to access something before the root
			log_aserror(_("ActionScript code trying to reference"
				" a nonexistent parent with '..' "
				" (a nonexistent parent probably only "
				"occurs in the root MovieClip)."
				" Returning NULL. "));
			);
			return NULL;
		}
		return parent;
	}

	// TODO: is it correct to check for _level here ?
	//       would it be valid at all if not the very first element
	//       in a path ?
	unsigned int levelno;
	if ( _vm.getRoot().isLevelTarget(name, levelno) )
	{
		return _vm.getRoot().getLevel(levelno).get();
	}


	std::string namei = name;
	if ( _vm.getSWFVersion() < 7 ) boost::to_lower(namei);

	if (name == "." || namei == "this") 
	{
	    return this;
	}

	return NULL;
}

void 
character::set_invalidated()
{
	set_invalidated("unknown", -1);
}

void
character::set_invalidated(const char* debug_file, int debug_line)
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
		
		#ifdef DEBUG_SET_INVALIDATED
		log_debug("%p set_invalidated() of %s in %s:%d",
			(void*)this, get_name(), debug_file, debug_line);
		#else
		UNUSED(debug_file);
		UNUSED(debug_line);
		#endif
		
		// NOTE: the SnappingRanges instance used here is not initialized by the
		// GUI and therefore uses the default settings. This should not be a 
		// problem but special snapping ranges configuration done in gui.cpp
		// is ignored here... 
				
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

void 
character::dump_character_tree(const std::string prefix) const
{
  log_debug("%s%s<%p> I=%d,CI=%d", prefix, typeName(*this), this,
    m_invalidated, m_child_invalidated);  
}

void
character::extend_invalidated_bounds(const InvalidatedRanges& ranges)
{
	set_invalidated(__FILE__, __LINE__);
	m_old_invalidated_ranges.add(ranges);
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
		const double newx = fn.arg(0).to_number();
		matrix m = ptr->get_matrix();
        m.set_x_translation(PIXELS_TO_TWIPS(utility::infinite_to_zero(newx)));
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
		const double newy = fn.arg(0).to_number();
		matrix m = ptr->get_matrix();
		m.set_y_translation(PIXELS_TO_TWIPS(utility::infinite_to_zero(newy)));
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
		const double xscale = m.get_x_scale();
		rv = as_value(xscale * 100); // result in percent
	}
	else // setter
	{
		matrix m = ptr->get_matrix();

		const double scale_percent = fn.arg(0).to_number();

		// Handle bogus values
		if (isnan(scale_percent))
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Attempt to set _xscale to %g, refused"),
                            scale_percent);
			);
            return as_value();
		}

		// input is in percent
		double scale = scale_percent / 100.0;
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
		const float yscale = m.get_y_scale();
		rv = as_value(yscale * 100); // result in percent
	}
	else // setter
	{
		matrix m = ptr->get_matrix();

		const double scale_percent = fn.arg(0).to_number();

		// Handle bogus values
		if (isnan(scale_percent))
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Attempt to set _yscale to %g, refused"),
                            scale_percent);
			);
                        return as_value();
		}

		// input is in percent
		float scale = static_cast<float>(scale_percent) / 100.f;
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
    
    m.invert().transform(a);
    return as_value(TWIPS_TO_PIXELS(a.x));
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
    m.invert().transform(a);
    return as_value(TWIPS_TO_PIXELS(a.y));
}

as_value
character::alpha_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	as_value rv;
	if ( fn.nargs == 0 ) // getter
	{
		rv = as_value(ptr->get_cxform().aa / 2.56);
	}
	else // setter
	{
		const as_value& inval = fn.arg(0);
		const double input = inval.to_number();
		if ( inval.is_undefined() || inval.is_null() || ! utility::isFinite(input) )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Ignored attempt to set %s.%s=%s"),
				ptr->getTarget(),
				_("_alpha"), // trying to reuse translations
				fn.arg(0));
			);
			return rv;
		}
		// set alpha = input / 100.0 * 256 = input * 2.56;
		cxform	cx = ptr->get_cxform();
		cx.aa = (boost::int16_t)(input * 2.56); 
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
			w = TWIPS_TO_PIXELS(std::floor(bounds.width() + 0.5));
		}
		rv = as_value(w);
	}
	else // setter
	{
		if ( ! bounds.isFinite() )
		{
			log_unimpl(_("FIXME: can't set _width on character %s (%s) with null or world bounds"),
				ptr->getTarget(), typeName(*ptr));
			return rv;
		}

		const double oldwidth = bounds.width();
		if ( oldwidth <= 0 )
		{
			log_unimpl(_("FIXME: can't set _width on character %s (%s) with width %d"),
				ptr->getTarget(), typeName(*ptr), oldwidth);
			return rv;
		}

		const double newwidth = PIXELS_TO_TWIPS(fn.arg(0).to_number());
		if ( newwidth <= 0 )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Setting _width=%g of character %s (%s)"),
				newwidth/20, ptr->getTarget(), typeName(*ptr));
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
			h = TWIPS_TO_PIXELS(std::floor(bounds.height() + 0.5));
		}
		rv = as_value(h);
	}
	else // setter
	{
		if ( ! bounds.isFinite() )
		{
			log_unimpl(_("FIXME: can't set _height on character %s (%s) with null or world bounds"),
				ptr->getTarget(), typeName(*ptr));
			return rv;
		}

		const double oldheight = bounds.height();
		if ( oldheight <= 0 )
		{
			log_unimpl(_("FIXME: can't set _height on character %s (%s) with height %d"),
				ptr->getTarget(), typeName(*ptr), oldheight);
			return rv;
		}

		const double newheight = PIXELS_TO_TWIPS(fn.arg(0).to_number());
		if ( newheight <= 0 )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Setting _height=%g of character %s (%s)"),
			                newheight / 20,ptr->getTarget(), typeName(*ptr));
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
		const float rotation = (float) fn.arg(0).to_number() * float(M_PI) / 180.f;
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

as_value
character::target_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	return as_value(ptr->getTargetPath());
}

as_value
character::name_getset(const fn_call& fn)
{
	boost::intrusive_ptr<character> ptr = ensureType<character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		const VM& vm = VM::get(); // TODO: fetch VM from ptr 
		const std::string& name = ptr->get_name();
		if ( vm.getSWFVersion() < 6 && name.empty() )
		{
			return as_value();
		} 
		else
		{
			return as_value(name);
		}
	}
	else // setter
	{
		ptr->set_name(fn.arg(0).to_string().c_str());
	}

	return as_value();
}

void
character::set_event_handlers(const Events& copyfrom)
{
	for (Events::const_iterator it=copyfrom.begin(), itE=copyfrom.end();
			it != itE; ++it)
	{
		const event_id& ev = it->first;
		const BufferList& bufs = it->second;
		for (size_t i = 0, e = bufs.size(); i < e; ++i)
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

	//log_debug(_("Setting handler for event %s"), id.get_function_name().c_str());

	// Set the character as a listener iff the
	// kind of event is a KEY or MOUSE one 
	switch (id.m_id)
	{
		case event_id::KEY_DOWN:  
		case event_id::KEY_PRESS:
		case event_id::KEY_UP:    
			has_key_event();
			break;
		case event_id::MOUSE_UP:
		case event_id::MOUSE_DOWN:
		case event_id::MOUSE_MOVE:
			//log_debug(_("Registering character as having mouse events"));
			has_mouse_event();
			break;
		default:
			break;
	}

	// todo: drop the character as a listener
	//       if it gets no valid handlers for
	//       mouse or Key events.
}

std::auto_ptr<ExecutableCode>
character::get_event_handler(const event_id& id) const
{
	std::auto_ptr<ExecutableCode> handler;

	Events::const_iterator it = _event_handlers.find(id);
	if ( it == _event_handlers.end() ) return handler;

#ifndef GNASH_USE_GC
	assert(get_ref_count() > 0);
#endif // GNASH_USE_GC
	boost::intrusive_ptr<character> this_ptr = const_cast<character*>(this);

	handler.reset( new EventCode(this_ptr, it->second) );
	return handler;
}

bool
character::unload()
{
	if ( ! _unloaded )
	{
		queueEvent(event_id::UNLOAD, movie_root::apDOACTION);
	}

	bool hasEvent = hasEventHandler(event_id::UNLOAD);

	_unloaded = true;

	return hasEvent;
}

void
character::queueEvent(const event_id& id, int lvl)
{

	movie_root& root = _vm.getRoot();
	std::auto_ptr<ExecutableCode> event(new QueuedEvent(boost::intrusive_ptr<character>(this), id));
	root.pushAction(event, lvl);
}

bool
character::hasEventHandler(const event_id& id) const
{
	Events::const_iterator it = _event_handlers.find(id);
	if ( it != _event_handlers.end() ) return true;

	boost::intrusive_ptr<as_function> method = getUserDefinedEventHandler(id.get_function_key());
	if (method) return true;

	return false;
}

boost::intrusive_ptr<as_function>
character::getUserDefinedEventHandler(const std::string& name) const
{
	string_table::key key = _vm.getStringTable().find(PROPNAME(name));
	return getUserDefinedEventHandler(key);
}

boost::intrusive_ptr<as_function>
character::getUserDefinedEventHandler(string_table::key key) const 
{
	as_value tmp;

	boost::intrusive_ptr<as_function> func;

	// const cast is needed due to getter/setter members possibly
	// modifying this object even when only get !
	if ( const_cast<character*>(this)->get_member(key, &tmp) )
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


/*private*/
std::string
character::computeTargetPath() const
{

	// TODO: check what happens when this character
	//       is a movie_instance loaded into another
	//       running movie.
	
	typedef std::vector<std::string> Path;
	Path path;

	// Build parents stack
	const character* topLevel = 0;
	const character* ch = this;
	for (;;)
	{
		const character* parent = ch->get_parent();

		// Don't push the _root name on the stack
		if ( ! parent )
		{
			// it is completely legal to set root's _name
			//assert(ch->get_name().empty());
			topLevel = ch;
			break;
		}

		path.push_back(ch->get_name());
		ch = parent;
	} 

	assert(topLevel);

	if ( path.empty() )
	{
		if ( _vm.getRoot().getRootMovie() == this ) return "/";
		std::stringstream ss;
		ss << "_level" << m_depth-character::staticDepthOffset;
		return ss.str();
	}

	// Build the target string from the parents stack
	std::string target;
	if ( topLevel != _vm.getRoot().getRootMovie() )
	{
		std::stringstream ss;
		ss << "_level" << topLevel->get_depth()-character::staticDepthOffset;
		target = ss.str();
	}
	for ( Path::reverse_iterator
			it=path.rbegin(), itEnd=path.rend();
			it != itEnd;
			++it )
	{
		target += "/" + *it; 
	}

	return target;
}

/*public*/
std::string
character::getTargetPath() const
{

  // TODO: maybe cache computed target?

	return computeTargetPath();
}


/*public*/
std::string
character::getTarget() const
{

	// TODO: check what happens when this character
	//       is a movie_instance loaded into another
	//       running movie.
	
	typedef std::vector<std::string> Path;
	Path path;

	// Build parents stack
	const character* ch = this;
	for (;;)
	{
		const character* parent = ch->get_parent();

		// Don't push the _root name on the stack
		if ( ! parent )
		{
			assert(dynamic_cast<const movie_instance*>(ch));
			std::stringstream ss;
			ss << "_level" << ch->get_depth()-character::staticDepthOffset;
			path.push_back(ss.str());
			// it is completely legal to set root's _name
			//assert(ch->get_name().empty());
			break;
		}

		path.push_back(ch->get_name());
		ch = parent;
	} 

	assert ( ! path.empty() );

	// Build the target string from the parents stack
	std::string target;
	for ( Path::const_reverse_iterator
			it=path.rbegin(), itEnd=path.rend();
			it != itEnd;
			++it )
	{
		if ( ! target.empty() ) target += ".";
		target += *it; 
	}

	return target;
}

#if 0
/*public*/
std::string
character::get_text_value() const
{
	return getTarget();
}
#endif

void
character::destroy()
{
	// in case we are destroyed w/out being unloaded first
	// see bug #21842
	_unloaded = true;

	/// we may destory a character that's not unloaded.
	///(we don't have chance to unload it in current model, see new_child_in_unload_test.c)
	/// We don't destroy ourself twice, right ?
	assert(!_destroyed);
	_destroyed = true;
}

void
character::markCharacterReachable() const
{
	if ( m_parent ) m_parent->setReachable();
	if ( _mask )
	{
		// Stop being masked if the mask was unloaded
		if ( _mask->isUnloaded() )
		{
			const_cast<character*>(this)->setMask(0);
		}
		else _mask->setReachable();
	}
	if ( _maskee )
	{
		// Stop masking if the masked character was unloaded
		if ( _maskee->isUnloaded() )
		{
			const_cast<character*>(this)->setMaskee(0);
		}
		else _maskee->setReachable();
	}
	markAsObjectReachable();
}

void
character::setMask(character* mask)
{
	if ( _mask != mask )
	{
		set_invalidated();
	}

	// Backup this before setMaskee has a chance to change it..
	character* prevMaskee = _maskee;

	// If we had a previous mask unregister with it
	if ( _mask && _mask != mask )
	{
		// the mask will call setMask(NULL) 
		// on any previously registered maskee
		// so we make sure to set our _mask to 
		// NULL before getting called again
		_mask->setMaskee(NULL);
	}

	// if we had a maskee, notify it to stop using
	// us as a mask
	if ( prevMaskee )
	{
		prevMaskee->setMask(0); 
	}

	// TODO: should we reset any original clip depth
	//       specified by PlaceObject tag ?
	set_clip_depth(noClipDepthValue); // this will set _mask !!
	_mask = mask;
	_maskee = 0;

	if ( _mask )
	{
		log_debug(" %s.setMask(%s): registering with new mask %s",
			getTarget(),
			mask ? mask->getTarget() : "null",
			_mask->getTarget());
		/// Register as as masked by the mask
		_mask->setMaskee(this);
	}
}

/*private*/
void
character::setMaskee(character* maskee)
{
	if ( _maskee == maskee )
	{
		return;
	}
	if ( _maskee )
	{
		// We don't want the maskee to call setMaskee(null)
		// on us again
		log_debug(" %s.setMaskee(%s) : previously masked char %s being set as non-masked",
			getTarget(), maskee ? maskee->getTarget() : "null", _maskee->getTarget());
		_maskee->_mask = NULL;
	}

	_maskee = maskee;

	if ( maskee )
	{
		set_clip_depth(dynClipDepthValue);
	}
	else
	{
		// TODO: should we reset any original clip depth
		//       specified by PlaceObject tag ?
		set_clip_depth(noClipDepthValue);
	}
}


bool 
character::boundsInClippingArea() const 
{
  geometry::Range2d<float> mybounds = getBounds();
  get_world_matrix().transform(mybounds);
  
  return gnash::render::bounds_in_clipping_area(mybounds);  
}

#ifdef USE_SWFTREE
character::InfoTree::iterator
character::getMovieInfo(InfoTree& tr, InfoTree::iterator it)
{
	const std::string yes = _("yes");
	const std::string no = _("no");

	it = tr.append_child(it, StringPair(getTarget(), typeName(*this)));

	std::ostringstream os;
	os << get_depth();
	tr.append_child(it, StringPair(_("Depth"), os.str()));

        /// Don't add if the character has no ratio value
        if (get_ratio() >= 0)
        {
            os.str("");
            os << get_ratio();
	        tr.append_child(it, StringPair(_("Ratio"), os.str()));
	    }	    

        /// Don't add if it's not a real clipping depth
        if (int cd = get_clip_depth() != noClipDepthValue )
        {
		os.str("");
		if (cd == dynClipDepthValue) os << "Dynamic mask";
		else os << cd;

		tr.append_child(it, StringPair(_("Clipping depth"), os.str()));	    
        }

        os.str("");
        os << get_width() << "x" << get_height();
	tr.append_child(it, StringPair(_("Dimensions"), os.str()));	

	tr.append_child(it, StringPair(_("Dynamic"), isDynamic() ? yes : no));	
	tr.append_child(it, StringPair(_("Mask"), isMaskLayer() ? yes : no));	    
	tr.append_child(it, StringPair(_("Destroyed"), isDestroyed() ? yes : no));
	tr.append_child(it, StringPair(_("Unloaded"), isUnloaded() ? yes : no));

	return it;
}
#endif

const sprite_instance*
character::getAsRoot() const
{
    return get_root();
}


} // namespace gnash

// local variables:
// mode: c++
// indent-tabs-mode: t
// end:
