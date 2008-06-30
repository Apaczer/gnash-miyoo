// button_character_instance.cpp:  Mouse-sensitive buttons, for Gnash.
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
#include "gnashconfig.h" // USE_SWF_TREE
#endif

#include "smart_ptr.h" // GNASH_USE_GC
#include "button_character_instance.h"
#include "button_character_def.h"
#include "action.h" // for as_standard_member enum
#include "as_value.h"

#include "ActionExec.h"
#include "sprite_instance.h"
#include "movie_root.h"
#include "VM.h"
#include "builtin_function.h"
#include "fn_call.h" // for shared ActionScript getter-setters
#include "GnashException.h" // for shared ActionScript getter-setters
#include "ExecutableCode.h"
#include "namedStrings.h"
#include "Object.h" // for getObjectInterface
#include "StringPredicates.h"
#include "GnashKey.h" // key::code

/** \page buttons Buttons and mouse behaviour

Observations about button & mouse behavior

Entities that receive mouse events: only buttons and sprites, AFAIK

When the mouse button goes down, it becomes "captured" by whatever
element is topmost, directly below the mouse at that moment.  While
the mouse is captured, no other entity receives mouse events,
regardless of how the mouse or other elements move.

The mouse remains captured until the mouse button goes up.  The mouse
remains captured even if the element that captured it is removed from
the display list.

If the mouse isn't above a button or sprite when the mouse button goes
down, then the mouse is captured by the background (i.e. mouse events
just don't get sent, until the mouse button goes up again).

Mouse events:

+------------------+---------------+-------------------------------------+
| Event            | Mouse Button  | description                         |
=========================================================================
| onRollOver       |     up        | sent to topmost entity when mouse   |
|                  |               | cursor initially goes over it       |
+------------------+---------------+-------------------------------------+
| onRollOut        |     up        | when mouse leaves entity, after     |
|                  |               | onRollOver                          |
+------------------+---------------+-------------------------------------+
| onPress          |  up -> down   | sent to topmost entity when mouse   |
|                  |               | button goes down.  onRollOver       |
|                  |               | always precedes onPress.  Initiates |
|                  |               | mouse capture.                      |
+------------------+---------------+-------------------------------------+
| onRelease        |  down -> up   | sent to active entity if mouse goes |
|                  |               | up while over the element           |
+------------------+---------------+-------------------------------------+
| onDragOut        |     down      | sent to active entity if mouse      |
|                  |               | is no longer over the entity        |
+------------------+---------------+-------------------------------------+
| onReleaseOutside |  down -> up   | sent to active entity if mouse goes |
|                  |               | up while not over the entity.       |
|                  |               | onDragOut always precedes           |
|                  |               | onReleaseOutside                    |
+------------------+---------------+-------------------------------------+
| onDragOver       |     down      | sent to active entity if mouse is   |
|                  |               | dragged back over it after          |
|                  |               | onDragOut                           |
+------------------+---------------+-------------------------------------+

There is always one active entity at any given time (considering NULL to
be an active entity, representing the background, and other objects that
don't receive mouse events).

When the mouse button is up, the active entity is the topmost element
directly under the mouse pointer.

When the mouse button is down, the active entity remains whatever it
was when the button last went down.

The active entity is the only object that receives mouse events.

!!! The "trackAsMenu" property alters this behavior!  If trackAsMenu
is set on the active entity, then onReleaseOutside is filtered out,
and onDragOver from another entity is allowed (from the background, or
another trackAsMenu entity). !!!


Pseudocode:

active_entity = NULL
mouse_button_state = UP
mouse_inside_entity_state = false
frame loop:
  if mouse_button_state == DOWN

    // Handle trackAsMenu
    if (active_entity->trackAsMenu)
      possible_entity = topmost entity below mouse
      if (possible_entity != active_entity && possible_entity->trackAsMenu)
        // Transfer to possible entity
	active_entity = possible_entity
	active_entity->onDragOver()
	mouse_inside_entity_state = true;

    // Handle onDragOut, onDragOver
    if (mouse_inside_entity_state == false)
      if (mouse is actually inside the active_entity)
        // onDragOver
	active_entity->onDragOver()
        mouse_inside_entity_state = true;

    else // mouse_inside_entity_state == true
      if (mouse is actually outside the active_entity)
        // onDragOut
	active_entity->onDragOut()
	mouse_inside_entity_state = false;

    // Handle onRelease, onReleaseOutside
    if (mouse button is up)
      if (mouse_inside_entity_state)
        // onRelease
        active_entity->onRelease()
      else
        // onReleaseOutside
	if (active_entity->trackAsMenu == false)
          active_entity->onReleaseOutside()
      mouse_button_state = UP
    
  if mouse_button_state == UP
    new_active_entity = topmost entity below the mouse
    if (new_active_entity != active_entity)
      // onRollOut, onRollOver
      active_entity->onRollOut()
      active_entity = new_active_entity
      active_entity->onRollOver()
    
    // Handle press
    if (mouse button is down)
      // onPress
      active_entity->onPress()
      mouse_inside_entity_state = true
      mouse_button_state = DOWN

*/


namespace gnash {

namespace {

class ButtonActionExecutor {
public:
	ButtonActionExecutor(as_environment& env)
		:
		_env(env)
	{}

	void operator() (const action_buffer& ab)
	{
		ActionExec exec(ab, _env);
		exec();
	}
private:
	as_environment& _env;
};

class ButtonActionPusher {
public:
	ButtonActionPusher(movie_root& mr, character* this_ptr)
		:
		called(false),
		_mr(mr),
		_tp(this_ptr)
	{}

	void operator() (const action_buffer& ab)
	{
		_mr.pushAction(ab, boost::intrusive_ptr<character>(_tp));
		called = true;
	}

	bool called;

private:
	movie_root& _mr;
	character* _tp;
};

}

// Forward declarations
static as_object* getButtonInterface();

/// A couple of typedefs to make code neater
typedef button_character_instance Button;
typedef boost::intrusive_ptr<Button> ButtonPtr;

static bool charDepthLessThen(const character* ch1, const character* ch2) 
{
	return ch1->get_depth() < ch2->get_depth();
}

static void
attachButtonInterface(as_object& o)
{
	//int target_version = o.getVM().getSWFVersion();

	as_c_function_ptr gettersetter;

	//
	// Properties (TODO: move to appropriate SWF version section)
	//

	gettersetter = &character::x_getset;
	o.init_property("_x", *gettersetter, *gettersetter);

	gettersetter = &character::y_getset;
	o.init_property("_y", *gettersetter, *gettersetter);

	gettersetter = &character::xscale_getset;
	o.init_property("_xscale", *gettersetter, *gettersetter);

	gettersetter = &character::yscale_getset;
	o.init_property("_yscale", *gettersetter, *gettersetter);

	gettersetter = &character::xmouse_get;
	o.init_readonly_property("_xmouse", *gettersetter);

	gettersetter = &character::ymouse_get;
	o.init_readonly_property("_ymouse", *gettersetter);

	gettersetter = &character::alpha_getset;
	o.init_property("_alpha", *gettersetter, *gettersetter);

	gettersetter = &character::visible_getset;
	o.init_property("_visible", *gettersetter, *gettersetter);

	gettersetter = &character::width_getset;
	o.init_property("_width", *gettersetter, *gettersetter);

	gettersetter = &character::height_getset;
	o.init_property("_height", *gettersetter, *gettersetter);

	gettersetter = &character::rotation_getset;
	o.init_property("_rotation", *gettersetter, *gettersetter);

	gettersetter = &character::parent_getset;
	o.init_property("_parent", *gettersetter, *gettersetter);
	
	gettersetter = &character::target_getset;
	o.init_property("_target", *gettersetter, *gettersetter);

	gettersetter = character::name_getset;
	o.init_property("_name", gettersetter, gettersetter);
	
	gettersetter = &button_character_instance::enabled_getset;
	o.init_property("enabled", *gettersetter, *gettersetter);

}

button_character_instance::button_character_instance(
		button_character_definition* def,
		character* parent, int id)
	:
	character(parent, id),
	m_def(def),
	m_last_mouse_flags(IDLE),
	m_mouse_flags(IDLE),
	m_mouse_state(UP),
	m_enabled(true)
{
	assert(m_def);

	set_prototype(getButtonInterface());

	// check up presence Key events
	if ( m_def->hasKeyPressHandler() )
	{
		_vm.getRoot().add_key_listener(this);
	}

}

button_character_instance::~button_character_instance()
{
	_vm.getRoot().remove_key_listener(this);
}


bool 
button_character_instance::get_enabled()
{
	return m_enabled;
}

void 
button_character_instance::set_enabled(bool value)
{
	if (value == m_enabled) return;
	m_enabled = value; 
	
	// NOTE: no visual change
}


as_value
button_character_instance::enabled_getset(const fn_call& fn)
{
	ButtonPtr ptr = ensureType<Button>(fn.this_ptr);

	as_value rv;

	if ( fn.nargs == 0 ) // getter
	{
		rv = as_value(ptr->get_enabled());
	}
	else // setter
	{
		ptr->set_enabled(fn.arg(0).to_bool());
	}
	return rv;
}



// called from Key listener only
// (the above line is wrong, it's also called with onConstruct, for instance)
bool
button_character_instance::on_event(const event_id& id)
{
	if ( isUnloaded() )
	{
		// We dont' respond to events while unloaded
		// See bug #22982
#if 0 // debugging..
		log_debug("Button %s received %s event while unloaded: ignored",
			getTarget(), id.get_function_name());
#endif
		return false; 
	}

	// We only respond keypress events
	if ( id.m_id != event_id::KEY_PRESS ) return false;

	// We only respond to valid key code (should we assert here?)
	if ( id.keyCode == key::INVALID ) return false;

	ButtonActionPusher xec(getVM().getRoot(), this); 
	m_def->forEachTrigger(id, xec);

	return xec.called;
}

void
button_character_instance::restart()
{
	log_error("button_character_instance::restart called, from whom??");
}

void
button_character_instance::display()
{
//	GNASH_REPORT_FUNCTION;

	std::vector<character*> actChars;
	get_active_characters(actChars);

	//log_debug("At display time, button %s got %d currently active chars", getTarget(), actChars.size());

	// TODO: by keeping chars sorted by depth we'd avoid the sort on display
	std::sort(actChars.begin(), actChars.end(), charDepthLessThen);

	std::for_each(actChars.begin(), actChars.end(), std::mem_fun(&character::display)); 

	clear_invalidated();
}


character*
button_character_instance::get_topmost_mouse_entity(float x, float y)
// Return the topmost entity that the given point covers.  NULL if none.
// I.e. check against ourself.
{
	if ( (!get_visible()) || (!get_enabled()))
	{
		return 0;
	}

	//-------------------------------------------------
	// Check our active and visible childrens first
	//-------------------------------------------------

	typedef std::vector<character*> Chars;
	Chars actChars;
	get_active_characters(actChars);

	if ( ! actChars.empty() )
	{
		std::sort(actChars.begin(), actChars.end(), charDepthLessThen);

		matrix  m = get_matrix();
		point p(x, y);
        m.invert().transform(p);

		for (Chars::reverse_iterator it=actChars.rbegin(), itE=actChars.rend(); it!=itE; ++it)
		{
			character* ch = *it;
			if ( ! ch->get_visible() ) continue;
			character *hit = ch->get_topmost_mouse_entity(p.x, p.y);
			if ( hit ) return hit;
		}
	}

	//-------------------------------------------------
	// If that failed, check our hit area
	//-------------------------------------------------

	// Find hit characters
	const CharsVect& hitChars = getHitCharacters();
	if ( hitChars.empty() ) return 0;

	// point is in parent's space,
	// we need to convert it in world space
	point wp(x,y);
	character* parent = get_parent();
	if ( parent )
	{
		parent->get_world_matrix().transform(wp);
	}

	for (size_t i=0, e=hitChars.size(); i<e; ++i)
	{
		const character* ch = hitChars[i];

		if ( ch->pointInVisibleShape(wp.x, wp.y) )
		{
			// The mouse is inside the shape.
			return this;
		}
	}

	return NULL;
}


void
button_character_instance::on_button_event(const event_id& event)
{
	if ( isUnloaded() )
	{
		// We dont' respond to events while unloaded
		// See bug #22982
		log_debug("Button %s received %s button event while unloaded: ignored",
			getTarget(), event.get_function_name());
		return;
	}

	e_mouse_state new_state = m_mouse_state;
  
	// Set our mouse state (so we know how to render).
	switch (event.m_id)
	{
	case event_id::ROLL_OUT:
	case event_id::RELEASE_OUTSIDE:
		new_state = UP;
		break;

	case event_id::RELEASE:
	case event_id::ROLL_OVER:
	case event_id::DRAG_OUT:
	case event_id::MOUSE_UP:
		new_state = OVER;
		break;

	case event_id::PRESS:
	case event_id::DRAG_OVER:
	case event_id::MOUSE_DOWN:
		new_state = DOWN;
		break;

	default:
		//abort();	// missed a case?
		log_error(_("Unhandled button event %s"), event.get_function_name().c_str());
		break;
	};
	
	
	set_current_state(new_state);
    
	// Button transition sounds.
	if (m_def->m_sound != NULL)
	{
		int bi; // button sound array index [0..3]
		media::sound_handler* s = get_sound_handler();

		// Check if there is a sound handler
		if (s != NULL) {
			switch (event.m_id)
			{
			case event_id::ROLL_OUT:
				bi = 0;
				break;
			case event_id::ROLL_OVER:
				bi = 1;
				break;
			case event_id::PRESS:
				bi = 2;
				break;
			case event_id::RELEASE:
				bi = 3;
				break;
			default:
				bi = -1;
				break;
			}
			if (bi >= 0)
			{
				button_character_definition::button_sound_info& bs = m_def->m_sound->m_button_sounds[bi];
				// character zero is considered as null character
				if (bs.m_sound_id > 0)
				{
					if (m_def->m_sound->m_button_sounds[bi].m_sam != NULL)
					{
						if (bs.m_sound_style.m_stop_playback)
						{
							s->stop_sound(bs.m_sam->m_sound_handler_id);
						}
						else
						{
							s->play_sound(bs.m_sam->m_sound_handler_id, bs.m_sound_style.m_loop_count, 0, 0, (bs.m_sound_style.m_envelopes.size() == 0 ? NULL : &bs.m_sound_style.m_envelopes));
						}
					}
				}
			}
		}
	}

	// From: "ActionScript - The Definiteve Guide" by Colin Moock
	// (chapter 10: Events and Event Handlers)
	//
	// "Event-based code [..] is said to be executed asynchronously
	//  because the triggering of events can occur at arbitrary times."
	//
	// We'll push to the global list. The movie_root will process
	// the action queue on mouse event.
	//

	movie_root& mr = getVM().getRoot();

	ButtonActionPusher xec(mr, this); 
	m_def->forEachTrigger(event, xec);

	// check for built-in event handler.
	std::auto_ptr<ExecutableCode> code ( get_event_handler(event) );
	if ( code.get() )
	{
		//log_debug(_("Got statically-defined handler for event: %s"), event.get_function_name().c_str());
		mr.pushAction(code, movie_root::apDOACTION);
		//code->execute();
	}
	//else log_debug(_("No statically-defined handler for event: %s"), event.get_function_name().c_str());

	// Call conventional attached method.
	boost::intrusive_ptr<as_function> method = getUserDefinedEventHandler(event.get_function_key());
	if ( method )
	{
		//log_debug(_("Got user-defined handler for event: %s"), event.get_function_name().c_str());
		mr.pushAction(method, this, movie_root::apDOACTION);
		//call_method0(as_value(method.get()), &(get_environment()), this);
	}
	//else log_debug(_("No statically-defined handler for event: %s"), event.get_function_name().c_str());
}

void 
button_character_instance::get_active_characters(std::vector<character*>& list, bool includeUnloaded)
{
	list.clear();
	
	for (size_t i=0,e=m_record_character.size(); i<e; ++i)
	{
		character* ch = m_record_character[i];
		if (ch == NULL) continue;
		if ( ! includeUnloaded && ch->isUnloaded() ) continue;
		list.push_back(ch);
	} 
}

void 
button_character_instance::get_active_records(RecSet& list, e_mouse_state state)
{
	list.clear();
	
	size_t nrecs = m_def->m_button_records.size();

	//log_debug("%s.get_active_records(%s) - def has %d records", getTarget(), mouseStateName(state), m_def->m_button_records.size());
	for (size_t i=0; i<nrecs; ++i)
	{
		button_record&	rec = m_def->m_button_records[i];
		//log_debug(" rec %d has hit:%d down:%d over:%d up:%d", i, rec.m_hit_test, rec.m_down, rec.m_over, rec.m_up);

		if ((state == UP && rec.m_up)
		    || (state == DOWN && rec.m_down)
		    || (state == OVER && rec.m_over)
		    || (state == HIT && rec.m_hit_test))
		{
			list.insert(i);
		}
	}
}

#ifdef GNASH_DEBUG_BUTTON_DISPLAYLIST
static void dump(std::vector< character* >& chars, std::stringstream& ss)
{
	for (size_t i=0, e=chars.size(); i<e; ++i)
	{
		ss << "Record" << i << ": ";
		character* ch = chars[i];
		if ( ! ch ) ss << "NULL.";
		else
		{
			ss << ch->getTarget() << " (depth:" << ch->get_depth()-character::staticDepthOffset-1 << " unloaded:" << ch->isUnloaded() << " destroyed:" << ch->isDestroyed() << ")";
		}
		ss << std::endl;
	}
}
#endif

void
button_character_instance::set_current_state(e_mouse_state new_state)
{
	if (new_state == m_mouse_state)
		return;
		
#ifdef GNASH_DEBUG_BUTTON_DISPLAYLIST
	std::stringstream ss;
	ss << "at set_current_state enter: " << std::endl;
	dump(m_record_character, ss);
	log_debug("%s", ss.str());
#endif

	// Get new state records
	RecSet newChars;
	get_active_records(newChars, new_state);

	// For each possible record, check if it should still be there
	for (size_t i=0, e=m_record_character.size(); i<e; ++i)
	{
		character* oldch = m_record_character[i];
		bool shouldBeThere = ( newChars.find(i) != newChars.end() );

		if ( ! shouldBeThere )
		{
			// is there, but is unloaded: destroy, clear slot and go on
			if ( oldch && oldch->isUnloaded() )
			{
				if ( ! oldch->isDestroyed() ) oldch->destroy();
				m_record_character[i] = NULL;
				oldch = NULL;
			}

			if ( oldch ) // the one we have should not be there... unload!
			{
				set_invalidated();

				if ( ! oldch->unload() )
				{
					// No onUnload handler: destroy and clear slot
					if ( ! oldch->isDestroyed() ) oldch->destroy();
					m_record_character[i] = NULL;
				}
				else
				{
					// onUnload handler: shift depth and keep slot
					int oldDepth = oldch->get_depth();
					int newDepth = character::removedDepthOffset - oldDepth;
#ifdef GNASH_DEBUG_BUTTON_DISPLAYLIST
					log_debug("Removed button record shifted from depth %d to depth %d", oldDepth, newDepth);
#endif
					oldch->set_depth(newDepth);
				}
			}
		}
		else // should be there
		{
			// Is there already, but is unloaded: destroy and consider as gone
			if ( oldch && oldch->isUnloaded() )
			{
				if ( ! oldch->isDestroyed() ) oldch->destroy();
				m_record_character[i] = NULL;
				oldch = NULL;
			}

			if ( ! oldch )
			{
				// Not there, instantiate
				button_record& bdef = m_def->m_button_records[i];

				const matrix&	mat = bdef.m_button_matrix;
				const cxform&	cx = bdef.m_button_cxform;
				int ch_depth = bdef.m_button_layer+character::staticDepthOffset+1;
				int ch_id = bdef.m_character_id;

				character* ch = bdef.m_character_def->create_character_instance(this, ch_id);
				ch->set_matrix(mat); 
				ch->set_cxform(cx); 
				ch->set_depth(ch_depth); 
				assert(ch->get_parent() == this);
				assert(ch->get_name().empty()); // no way to specify a name for button chars anyway...

				if ( ch->wantsInstanceName() )
				{
					//std::string instance_name = getNextUnnamedInstanceName();
					ch->set_name(getNextUnnamedInstanceName());
				}

				set_invalidated();

				m_record_character[i] = ch;
				ch->stagePlacementCallback(); // give this character a life

			}
		}
	}

#ifdef GNASH_DEBUG_BUTTON_DISPLAYLIST
	ss.str("");
	ss << "at set_current_state end: " << std::endl;
	dump(m_record_character, ss);
	log_debug("%s", ss.str());
#endif

	// Remember current state
	m_mouse_state=new_state;
	 
}

//
// ActionScript overrides
//



void 
button_character_instance::add_invalidated_bounds(InvalidatedRanges& ranges, 
	bool force)
{
	if (!m_visible)
	{
		//log_debug("button %s not visible on add_invalidated_bounds", getTarget());
		return; // not visible anyway
	}
	//log_debug("button %s add_invalidated_bounds called", getTarget());

	ranges.add(m_old_invalidated_ranges);  

	std::vector<character*> actChars;
	get_active_characters(actChars);
	std::for_each(actChars.begin(), actChars.end(),
		boost::bind(&character::add_invalidated_bounds, _1,
			    boost::ref(ranges), force||m_invalidated)
	);
}

rect
button_character_instance::getBounds() const
{
	rect allBounds;

	typedef std::vector<character*> CharVect;
	CharVect actChars;
	const_cast<button_character_instance*>(this)->get_active_characters(actChars);
	for(CharVect::iterator i=actChars.begin(),e=actChars.end(); i!=e; ++i)
	{
		const character* ch = *i;
		// Child bounds need be transformed in our coordinate space
		rect lclBounds = ch->getBounds();
		matrix m = ch->get_matrix();
		allBounds.expand_to_transformed_rect(m, lclBounds);
	}

	return allBounds;
}

bool
button_character_instance::pointInShape(float x, float y) const
{
	typedef std::vector<character*> CharVect;
	CharVect actChars;
	const_cast<button_character_instance*>(this)->get_active_characters(actChars);
	for(CharVect::iterator i=actChars.begin(),e=actChars.end(); i!=e; ++i)
	{
		const character* ch = *i;
		if ( ch->pointInShape(x,y) ) return true;
	}
	return false; 
}

as_object*
button_character_instance::get_path_element(string_table::key key)
{
	as_object* ch = get_path_element_character(key);
	if ( ch ) return ch;

	const std::string& name = _vm.getStringTable().value(key);
	return getChildByName(name); // possibly NULL
}

character *
button_character_instance::getChildByName(const std::string& name) const
{
	// Get all currently active characters, including unloaded
	CharsVect actChars;
	// TODO: fix the const_cast
	const_cast<button_character_instance*>(this)->get_active_characters(actChars, true);

	// Lower depth first for duplicated names, so we sort
	std::sort(actChars.begin(), actChars.end(), charDepthLessThen);

	for (CharsVect::iterator i=actChars.begin(), e=actChars.end(); i!=e; ++i)
	{

		character* const child = *i;
		const std::string& childname = child->get_name();
 
   		if ( _vm.getSWFVersion() >= 7 )
 		{
			if ( childname == name ) return child;
 		}
 		else
 		{
		    StringNoCaseEqual noCaseCompare;
			if ( noCaseCompare(childname, name) ) return child;
 		}
	}

	return NULL;
}

void
button_character_instance::stagePlacementCallback()
{
	saveOriginalTarget(); // for soft refs

	// Register this button instance as a live character
	// do we need this???
	//_vm.getRoot().addLiveChar(this);

	// Instantiate the hit characters
	RecSet hitChars;
	get_active_records(hitChars, HIT);
	for (RecSet::iterator i=hitChars.begin(),e=hitChars.end(); i!=e; ++i)
	{
		button_record& bdef = m_def->m_button_records[*i];

		const matrix& mat = bdef.m_button_matrix;
		const cxform& cx = bdef.m_button_cxform;
		int ch_depth = bdef.m_button_layer+character::staticDepthOffset+1;
		int ch_id = bdef.m_character_id;

		character* ch = bdef.m_character_def->create_character_instance(this, ch_id);
		ch->set_matrix(mat); 
		ch->set_cxform(cx); // TODO: who cares about color ?
		ch->set_depth(ch_depth); // TODO: check if we care about depth, and why ...
		assert(ch->get_parent() == this);
		assert(ch->get_name().empty()); // no way to specify a name for button chars anyway...

		_hitCharacters.push_back(ch);
	}

	// Setup the state characters container
	// It will have a slot for each character record.
	// Some slots will probably be never used (consider HIT-only records)
	// but for now this direct corrispondence between record number
	// and active character will be handy.
	m_record_character.resize(m_def->m_button_records.size());

	// Instantiate the default state characters 
	RecSet upChars;
	get_active_records(upChars, UP);
	//log_debug("At StagePlacementCallback, button %s got %d active chars for state UP", getTarget(), upChars.size());
	for (RecSet::iterator i=upChars.begin(),e=upChars.end(); i!=e; ++i)
	{
		int rno = *i;
		button_record& bdef = m_def->m_button_records[rno];

		const matrix&	mat = bdef.m_button_matrix;
		const cxform&	cx = bdef.m_button_cxform;
		int ch_depth = bdef.m_button_layer+character::staticDepthOffset+1;
		int ch_id = bdef.m_character_id;

		character* ch = bdef.m_character_def->create_character_instance(this, ch_id);
		ch->set_matrix(mat); 
		ch->set_cxform(cx); 
		ch->set_depth(ch_depth); 
		assert(ch->get_parent() == this);
		assert(ch->get_name().empty()); // no way to specify a name for button chars anyway...

		if ( ch->wantsInstanceName() )
		{
			//std::string instance_name = getNextUnnamedInstanceName();
			ch->set_name(getNextUnnamedInstanceName());
		}

		m_record_character[rno] = ch;
		ch->stagePlacementCallback(); // give this character a life
	}

	// there's no INITIALIZE/CONSTRUCT/LOAD/ENTERFRAME/UNLOAD events for buttons
}

#ifdef GNASH_USE_GC
void
button_character_instance::markReachableResources() const
{
	assert(isReachable());

	m_def->setReachable();

	// Mark state characters as reachable
	for (CharsVect::const_iterator i=m_record_character.begin(), e=m_record_character.end();
			i!=e; ++i)
	{
		character* ch = *i;
		if ( ch ) ch->setReachable();
	}

	// Mark hit characters as reachable
	for (CharsVect::const_iterator i=_hitCharacters.begin(), e=_hitCharacters.end();
			i!=e; ++i)
	{
		character* ch = *i;
		assert ( ch );
		ch->setReachable();
	}

	// character class members
	markCharacterReachable();
}
#endif // GNASH_USE_GC

bool
button_character_instance::unload()
{
	//log_debug("Button %s being unloaded", getTarget());

	bool childsHaveUnload = false;

	// We need to unload all childs, or the global instance list will keep growing forever !
	//std::for_each(m_record_character.begin(), m_record_character.end(), boost::bind(&character::unload, _1));
	for (CharsVect::iterator i=m_record_character.begin(), e=m_record_character.end(); i!=e; ++i)
	{
		character* ch = *i;
		if ( ! ch ) continue;
		if ( ch->isUnloaded() ) continue;
		if ( ch->unload() ) childsHaveUnload = true;
		//log_debug("Button child %s (%s) unloaded", ch->getTarget().c_str(), typeName(*ch).c_str());
	}

	// NOTE: we don't need to ::unload or ::destroy here
	//       as the _hitCharacters are never placed on stage.
	//       As an optimization we might not even instantiate
	//       them, and only use the definition and the 
	//       associated transform matrix... (would take
	//       hit instance off the GC).
	_hitCharacters.clear();

	bool hasUnloadEvent = character::unload();

	return hasUnloadEvent || childsHaveUnload;
}

void
button_character_instance::destroy()
{
	//log_debug("Button %s being destroyed", getTarget());

	for (CharsVect::iterator i=m_record_character.begin(), e=m_record_character.end(); i!=e; ++i)
	{
		character* ch = *i;
		if ( ! ch ) continue;
		if ( ch->isDestroyed() ) continue;
		ch->destroy();
		*i = NULL;
	}

	// NOTE: we don't need to ::unload or ::destroy here
	//       as the _hitCharacters are never placed on stage.
	//       As an optimization we might not even instantiate
	//       them, and only use the definition and the 
	//       associated transform matrix... (would take
	//       hit instance off the GC).
	_hitCharacters.clear();

	character::destroy();
}

bool
button_character_instance::get_member(string_table::key name_key, as_value* val,
  string_table::key nsname)
{
  // FIXME: use addProperty interface for these !!
  // TODO: or at least have a character:: protected method take
  //       care of these ?
  //       Duplicates code in character::get_path_element_character too..
  //
  if (name_key == NSV::PROP_uROOT)
  {
    // getAsRoot() will take care of _lockroot
    val->set_as_object( const_cast<sprite_instance*>( getAsRoot() )  );
    return true;
  }

  // NOTE: availability of _global doesn't depend on VM version
  //       but on actual movie version. Example: if an SWF4 loads
  //       an SWF6 (to, say, _level2), _global will be unavailable
  //       to the SWF4 code but available to the SWF6 one.
  //
  if ( getSWFVersion() > 5 && name_key == NSV::PROP_uGLOBAL ) // see MovieClip.as
  {
    // The "_global" ref was added in SWF6
    val->set_as_object( _vm.getGlobal() );
    return true;
  }

  const std::string& name = _vm.getStringTable().value(name_key);

  movie_root& mr = _vm.getRoot();
  unsigned int levelno;
  if ( mr.isLevelTarget(name, levelno) )
  {
    movie_instance* mo = mr.getLevel(levelno).get();
    if ( mo )
    {
      val->set_as_object(mo);
      return true;
    }
    else
    {
      return false;
    }
	}

  // TOCHECK : Try object members, BEFORE display list items
  //
  if (get_member_default(name_key, val, nsname))
  {

// ... trying to be useful to Flash coders ...
// The check should actually be performed before any return
// prior to the one due to a match in the DisplayList.
// It's off by default anyway, so not a big deal.
// See bug #18457
#define CHECK_FOR_NAME_CLASHES 1
#ifdef CHECK_FOR_NAME_CLASHES
    IF_VERBOSE_ASCODING_ERRORS(
    if ( getChildByName(name) )
    {
      log_aserror(_("A button member (%s) clashes with "
          "the name of an existing character "
          "in its display list.  "
          "The member will hide the "
          "character"), name.c_str());
    }
    );
#endif

    return true;
  }


  // Try items on our display list.
  character* ch = getChildByName(name);

  if (ch)
  {
      // Found object.

      // If the object is an ActionScript referenciable one we
      // return it, otherwise we return ourselves
      if ( ch->isActionScriptReferenceable() )
      {
        val->set_as_object(ch);
      }
      else
      {
        val->set_as_object(this);
      }

      return true;
  }

  return false;

}

int
button_character_instance::getSWFVersion() const
{
	return m_def->getSWFVersion();
}

static as_object*
getButtonInterface()
{
  static boost::intrusive_ptr<as_object> proto;
  if ( proto == NULL )
  {
    proto = new as_object(getObjectInterface());
    VM::get().addStatic(proto.get());

    attachButtonInterface(*proto);
  }
  return proto.get();
}

static as_value
button_ctor(const fn_call& /* fn */)
{
  boost::intrusive_ptr<as_object> clip = new as_object(getButtonInterface());
  return as_value(clip.get());
}

void
button_class_init(as_object& global)
{
  // This is going to be the global Button "class"/"function"
  static boost::intrusive_ptr<builtin_function> cl=NULL;

  if ( cl == NULL )
  {
    cl=new builtin_function(&button_ctor, getButtonInterface());
    VM::get().addStatic(cl.get());
  }

  // Register _global.MovieClip
  global.init_member("Button", cl.get());
}

#ifdef USE_SWFTREE
character::InfoTree::iterator 
button_character_instance::getMovieInfo(InfoTree& tr, InfoTree::iterator it)
{
	InfoTree::iterator selfIt = character::getMovieInfo(tr, it);
	std::ostringstream os;

	std::vector<character*> actChars;
	get_active_characters(actChars, true);
	std::sort(actChars.begin(), actChars.end(), charDepthLessThen);

	os << actChars.size() << " active characters for state " << mouseStateName(m_mouse_state);
	InfoTree::iterator localIter = tr.append_child(selfIt, StringPair(_("Button state"), os.str()));	    
	std::for_each(actChars.begin(), actChars.end(), boost::bind(&character::getMovieInfo, _1, tr, localIter)); 

	return selfIt;

}
#endif

const char*
button_character_instance::mouseStateName(e_mouse_state s)
{
	switch (s)
	{
		case UP: return "UP";
		case DOWN: return "DOWN";
		case OVER: return "OVER";
		case HIT: return "HIT";
		default: return "UNKNOWN (error?)";
	}
}

} // end of namespace gnash


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
