// sprite_instance.cpp:  Stateful live Sprite instance, for Gnash.
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
#include "gnashconfig.h"
#endif

#include "log.h" 
#include "action.h" // for call_method_parsed (call_method_args)
#include "sprite_instance.h"
#include "movie_definition.h"
#include "MovieClipLoader.h" // @@ temp hack for loading tests
#include "as_value.h"
#include "as_function.h"
#include "edit_text_character.h" // for registered variables
#include "edit_text_character_def.h" // @@ temp hack for createTextField exp.
#include "ControlTag.h"
#include "fn_call.h"
#include "Key.h"
#include "movie_root.h"
#include "movie_instance.h"
#include "swf_event.h"
#include "sprite_definition.h"
#include "ActionExec.h"
#include "builtin_function.h"
#include "smart_ptr.h"
#include "VM.h"
#include "Range2d.h" // for getBounds
#include "GnashException.h"
#include "URL.h"
#include "sound_handler.h"
#include "StreamProvider.h"
#include "URLAccessManager.h" // for loadVariables
#include "LoadVariablesThread.h" 
#include "ExecutableCode.h" // for inheritance of ConstructEvent
#include "gnash.h" // for point class !
#include "Object.h" // for getObjectInterface
#include "DynamicShape.h" // for composition
#include "namedStrings.h"
#include "fill_style.h" // for beginGradientFill

#ifdef USE_SWFTREE
# include "tree.hh"
#endif

#include <vector>
#include <string>
#include <cmath>

#ifdef __sgi
extern double round(double);
#pragma optional round
#endif

#include <functional> // for mem_fun, bind1st
#include <algorithm> // for for_each
#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

namespace gnash {

//#define GNASH_DEBUG 1
//#define GNASH_DEBUG_TIMELINE 1
//#define GNASH_DEBUG_REPLACE 1
//#define  DEBUG_DYNTEXT_VARIABLES 1
//#define GNASH_DEBUG_HITTEST 1
//#define DEBUG_LOAD_VARIABLES 1

// Defining the following macro you'll get a DEBUG lien
// for each call to the drawing API, in a format which is
// easily re-compilable to obtain a smaller testcase
//#define DEBUG_DRAWING_API 1

// Define this to make mouse entity finding verbose
// This includes get_topmost_mouse_entity and findDropTarget
//
//#define DEBUG_MOUSE_ENTITY_FINDING 1

#define ONCE(x) { static bool warned=false; if (!warned) { warned=true; x; } }

// Forward declarations
static as_object* getMovieClipInterface();
static void attachMovieClipInterface(as_object& o);
static void attachMovieClipProperties(character& o);

/// Anonymous namespace for module-private definitions
namespace
{

  /// ConstructEvent, used for queuing construction
  //
  /// It's execution will call constructAsScriptObject() 
  /// on the target sprite
  ///
  class ConstructEvent: public ExecutableCode {

  public:

    ConstructEvent(sprite_instance* nTarget)
      :
      _target(nTarget)
    {}


    ExecutableCode* clone() const
    {
      return new ConstructEvent(*this);
    }

    virtual void execute()
    {
      _target->constructAsScriptObject();
    }

  #ifdef GNASH_USE_GC
    /// Mark reachable resources (for the GC)
    //
    /// Reachable resources are:
    ///  - the action target (_target)
    ///
    virtual void markReachableResources() const
    {
      _target->setReachable();
    }
  #endif // GNASH_USE_GC

  private:

    sprite_instance* _target;

  };

} // anonymous namespace


//------------------------------------------------
// Utility funx
//------------------------------------------------

// Execute the actions in the action list, in the given
// environment. The list of action will be consumed
// starting from the first element. When the function returns
// the list should be empty.
void
sprite_instance::execute_actions(sprite_instance::ActionList& action_list)
{
  // action_list may be changed due to actions (appended-to)
  // this loop might be optimized by using an iterator
  // and a final call to .clear() 
  while ( ! action_list.empty() )
  {
    const action_buffer* ab = action_list.front();
    action_list.pop_front(); 

    execute_action(*ab);
  }
}

static as_value sprite_play(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  sprite->set_play_state(sprite_instance::PLAY);
  return as_value();
}

static as_value sprite_stop(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  sprite->set_play_state(sprite_instance::STOP);

  return as_value();
}

//removeMovieClip() : Void
static as_value sprite_remove_movieclip(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  sprite->removeMovieClip();
  return as_value();
}

// attachMovie(idName:String, newName:String,
//             depth:Number [, initObject:Object]) : MovieClip
static as_value sprite_attach_movie(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  as_value rv;

  if (fn.nargs < 3 || fn.nargs > 4)
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("attachMovie called with wrong number of arguments"
      " expected 3 to 4, got (%d) - returning undefined"),
      fn.nargs);
    );
    return rv;
  }

  // Get exported resource 
  const std::string& id_name = fn.arg(0).to_string();

  boost::intrusive_ptr<resource> exported = sprite->get_movie_definition()->get_exported_resource(id_name);
  if ( exported == NULL )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("attachMovie: '%s': no such exported resource - "
      "returning undefined"), id_name);
    );
    return rv; 
  }
  
  character_def* exported_movie = dynamic_cast<character_def*>(exported.get());
  if ( ! exported_movie )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("attachMovie: exported resource '%s' "
      "is not a character definition (%s) -- "
      "returning undefined"), id_name,
      typeid(*(exported.get())).name());
    );
    return rv;
  }

  const std::string& newname = fn.arg(1).to_string();

  // should we support negative depths ? YES !
  int depth_val = boost::uint16_t(fn.arg(2).to_number());

  boost::intrusive_ptr<character> newch = exported_movie->create_character_instance(sprite.get(), depth_val);
  assert( newch.get() > (void*)0xFFFF );
#ifndef GNASH_USE_GC
  assert(newch->get_ref_count() > 0);
#endif // ndef GNASH_USE_GC

  newch->set_name(newname);
  newch->setDynamic();

  // place_character() will set depth on newch
  if ( ! sprite->attachCharacter(*newch, depth_val) )
  {
    log_error(_("Could not attach character at depth %d"), depth_val);
    return rv;
  }

  /// Properties must be copied *after* the call to attachCharacter
  /// because attachCharacter() will reset matrix !!
  if (fn.nargs > 3 ) {
    boost::intrusive_ptr<as_object> initObject = fn.arg(3).to_object();
    if ( initObject ) {
      //log_debug(_("Initializing properties from object"));
      newch->copyProperties(*initObject);
    } else {
      // This is actually a valid thing to do,
      // the documented behaviour is to just NOT
      // initializing the properties in this
      // case.
      IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("Fourth argument of attachMovie "
        "doesn't cast to an object (%s), we'll act as if it wasn't given"),
        fn.arg(3).to_debug_string());
      );
    }
  }
  rv = as_value(newch.get());
  return rv;
}

// attachAudio(id:Object) : Void
static as_value sprite_attach_audio(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  ONCE( log_unimpl("MovieClip.attachAudio()") );
  return as_value();
}

// MovieClip.attachVideo
static as_value sprite_attach_video(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  ONCE( log_unimpl("MovieClip.attachVideo()") );
  return as_value();
}

//createEmptyMovieClip(name:String, depth:Number) : MovieClip
static as_value sprite_create_empty_movieclip(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if (fn.nargs != 2)
  {
    if (fn.nargs < 2)
    {
      IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("createEmptyMovieClip needs "
          "2 args, but %d given,"
          " returning undefined"),
          fn.nargs);
      );
      return as_value();
    }
    else
    {
      IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("createEmptyMovieClip takes "
          "2 args, but %d given, discarding"
          " the excess"),
          fn.nargs);
      )
    }
  }

  // TODO: Why not use to_int()?
  character* ch = sprite->add_empty_movieclip(fn.arg(0).to_string().c_str(),
                                              fn.arg(1).to_int());
  return as_value(ch);
}

static as_value sprite_get_depth(const fn_call& fn)
{
  // TODO: make this a character::getDepth_method function...
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  int n = sprite->get_depth();

  return as_value(n);
}

//swapDepths(target:Object|target:Number) : Void
static as_value sprite_swap_depths(const fn_call& fn)
{
  typedef boost::intrusive_ptr<sprite_instance> SpritePtr;
  typedef boost::intrusive_ptr<character> CharPtr;

  SpritePtr sprite = ensureType<sprite_instance>(fn.this_ptr);
  int this_depth = sprite->get_depth();

  as_value rv;

  if (fn.nargs < 1)
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("%s.swapDepths() needs one arg"), sprite->getTarget());
    );
    return rv;
  }

  // Lower bound of source depth below which swapDepth has no effect (below Timeline/static zone)
  if ( this_depth < character::staticDepthOffset )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    stringstream ss; fn.dump_args(ss);
    log_aserror(_("%s.swapDepths(%s): won't swap a clip below depth %d (%d)"),
      sprite->getTarget(), ss.str(), character::staticDepthOffset, this_depth);
    );
    return rv;
  }


  SpritePtr this_parent = dynamic_cast<sprite_instance*>(sprite->get_parent());

  //CharPtr target = NULL;
  int target_depth = 0;

  // sprite.swapDepth(sprite)
  if ( SpritePtr target_sprite = fn.arg(0).to_sprite() )
  {
    if ( sprite == target_sprite )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("%s.swapDepths(%s): invalid call, swapping to self?"),
        sprite->getTarget(), target_sprite->getTarget());
      );
      return rv;
    }

    SpritePtr target_parent = dynamic_cast<sprite_instance*>(sprite->get_parent());
    if ( this_parent != target_parent )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("%s.swapDepths(%s): invalid call, the two characters don't have the same parent"),
        sprite->getTarget(), target_sprite->getTarget());
      );
      return rv;
    }

    target_depth = target_sprite->get_depth();

    // Check we're not swapping the our own depth so
    // to avoid unecessary bounds invalidation and immunizing
    // the instance from subsequent PlaceObjec tags attempting
    // to transform it.
    if ( sprite->get_depth() == target_depth )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      stringstream ss; fn.dump_args(ss);
      log_aserror(_("%s.swapDepths(%s): ignored, source and target characters have the same depth %d"),
        sprite->getTarget(), ss.str(), target_depth);
      );
      return rv;
    }

    //target = boost::dynamic_pointer_cast<character>(target_sprite);
  }

  // sprite.swapDepth(depth)
  else
  {
    double td = fn.arg(0).to_number();
    if ( isnan(td) )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      stringstream ss; fn.dump_args(ss);
      log_aserror(_("%s.swapDepths(%s): first argument invalid "
        "(neither a sprite nor a number)"),
        sprite->getTarget(), ss.str());
      );
      return rv;
    }

    target_depth = int(td);

    // Check we're not swapping the our own depth so
    // to avoid unecessary bounds invalidation and immunizing
    // the instance from subsequent PlaceObjec tags attempting
    // to transform it.
    if ( sprite->get_depth() == target_depth )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      stringstream ss; fn.dump_args(ss);
      log_aserror(_("%s.swapDepths(%s): ignored, character already at depth %d"),
        sprite->getTarget(), ss.str(), target_depth);
      );
      return rv;
    }


    // TODO : check other kind of validities ?


  }

  if ( this_parent )
  {
    this_parent->swapDepths(sprite.get(), target_depth);
  }
  else
  {
    movie_root& root = VM::get().getRoot();
    root.swapLevels(sprite, target_depth);
    return rv;
  }

  return rv;

}

// TODO: wrap the functionality in a sprite_instance method
//       and invoke it from here, this should only be a wrapper
//
//duplicateMovieClip(name:String, depth:Number, [initObject:Object]) : MovieClip
static as_value sprite_duplicate_movieclip(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  
  if (fn.nargs < 2)
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.duplicateMovieClip() needs 2 or 3 args"));
        );
    return as_value();
  }

  const std::string& newname = fn.arg(0).to_string();
  int depth = int(fn.arg(1).to_number());

  boost::intrusive_ptr<sprite_instance> ch;

  // Copy members from initObject
  if (fn.nargs == 3)
  {
    boost::intrusive_ptr<as_object> initObject = fn.arg(2).to_object();
    ch = sprite->duplicateMovieClip(newname, depth, initObject.get());
  }
  else
  {
    ch = sprite->duplicateMovieClip(newname, depth);
  }

  return as_value(ch.get());
}

static as_value sprite_goto_and_play(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if (fn.nargs < 1)
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("sprite_goto_and_play needs one arg"));
    );
    return as_value();
  }

  size_t frame_number;
  if ( ! sprite->get_frame_number(fn.arg(0), frame_number) )
  {
    // No dice.
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("sprite_goto_and_play('%s') -- invalid frame"),
          fn.arg(0).to_debug_string());
    );
    return as_value();
  }

  // Convert to 0-based
  sprite->goto_frame(frame_number);
  sprite->set_play_state(sprite_instance::PLAY);
  return as_value();
}

static as_value sprite_goto_and_stop(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if (fn.nargs < 1)
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("sprite_goto_and_stop needs one arg"));
    );
    return as_value();
  }

  size_t frame_number;
  if ( ! sprite->get_frame_number(fn.arg(0), frame_number) )
  {
    // No dice.
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("sprite_goto_and_stop('%s') -- invalid frame"),
          fn.arg(0).to_debug_string());
    );
    return as_value();
  }

  // Convert to 0-based
  sprite->goto_frame(frame_number);
  sprite->set_play_state(sprite_instance::STOP);
  return as_value();
}

static as_value sprite_next_frame(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  size_t frame_count = sprite->get_frame_count();
  size_t current_frame = sprite->get_current_frame();
  if (current_frame < frame_count)
  {
      sprite->goto_frame(current_frame + 1);
  }
  sprite->set_play_state(sprite_instance::STOP);
  return as_value();
}

static as_value sprite_prev_frame(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  size_t current_frame = sprite->get_current_frame();
  if (current_frame > 0)
  {
      sprite->goto_frame(current_frame - 1);
  }
  sprite->set_play_state(sprite_instance::STOP);
  return as_value();
}

static as_value sprite_get_bytes_loaded(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  return as_value(sprite->get_bytes_loaded());
}

static as_value sprite_get_bytes_total(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  // @@ horrible uh ?
  return as_value(sprite->get_bytes_total());
}

// my_mc.loadMovie(url:String [,variables:String]) : Void
static as_value sprite_load_movie(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  if (fn.nargs < 1) // url
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.loadMovie() "
      "expected 1 or 2 args, got %d - returning undefined"),
      fn.nargs);
    );
    return as_value();
  }

  const std::string& urlstr = fn.arg(0).to_string();
  if (urlstr.empty())
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror(_("First argument of MovieClip.loadMovie(%s) "
      "evaluates to an empty string - "
      "returning undefined"),
      ss.str());
    );
    return as_value();
  }
  const URL& baseurl = get_base_url();
  URL url(urlstr, baseurl);

  movie_root& mr = sprite->getVM().getRoot();
  std::string target = sprite->getTarget();


  // TODO: if GET/POST should send variables of *this* movie,
  // no matter if the target will be replaced by another movie !!
  bool usePost = false;
  bool sendVars = false;
  if (fn.nargs > 1)
  {
	as_value arg = fn.arg(1);
	std::string methodString = arg.to_string();
	boost::to_lower(methodString);
	if ( methodString == "post" ) 
	{
		usePost = true;
		sendVars = true;
	}
	else if ( methodString == "get" ) 
	{
		sendVars = true;
	}
	else
	{
		IF_VERBOSE_ASCODING_ERRORS(
		std::stringstream ss;
		fn.dump_args(ss);
		log_aserror(_("MovieClip.loadMovie(%s): second argument (if any) must be 'post' or 'get' [got %s]"),
			ss.str(), methodString);
		);
	}
  }

  if ( ! sendVars )
  {
	//log_debug("Not sending vars");
  	mr.loadMovie(url, target); 
  }
  else
  {
  	std::string data;
	sprite->getURLEncodedVars(data);
 
	if ( usePost )
	{
		log_debug("POSTING: %s", data);
		mr.loadMovie(url, target, &data);
	}
	else
	{
		std::string qs = url.querystring();
		if ( qs.empty() ) data.insert(0, 1, '?');
		else data.insert(0, 1, '&');
		url.set_querystring(qs+data);
		log_debug("GETTING: %s", url.str());
		mr.loadMovie(url, target); 
	}
  }

  return as_value();
}

// my_mc.loadVariables(url:String [, variables:String]) : Void
static as_value sprite_load_variables(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  if (fn.nargs < 1) // url
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.loadVariables() "
      "expected 1 or 2 args, got %d - returning undefined"),
      fn.nargs);
    );
    return as_value();
  }

  const std::string& urlstr = fn.arg(0).to_string();
  if (urlstr.empty())
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror(_("First argument passed to MovieClip.loadVariables(%s) "
      "evaluates to an empty string - "
      "returning undefined"),
      ss.str());
    );
    return as_value();
  }
  const URL& baseurl = get_base_url();
  URL url(urlstr, baseurl);

  short method = 0;

  if (fn.nargs > 1)
  {  
  
    boost::intrusive_ptr<as_object> methodstr = fn.arg(1).to_object();
    assert(methodstr);
    
    string_table& st = sprite->getVM().getStringTable();
    as_value lc = methodstr->callMethod(st.find(PROPNAME("toLowerCase")));
    std::string methodstring = lc.to_string(); 
  
    if ( methodstring == "get" ) method = 1;
    else if ( methodstring == "post" ) method = 2;
  }

  sprite->loadVariables(url, method);
  log_debug("MovieClip.loadVariables(%s) - TESTING ", url.str());


  //log_unimpl(__PRETTY_FUNCTION__);
  //moviecliploader_loadclip(fn);
  return as_value();
}

// my_mc.unloadMovie() : Void
static as_value sprite_unload_movie(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  // See http://sephiroth.it/reference.php?id=429

  ONCE( log_unimpl("MovieClip.unloadMovie()") );
  return as_value();
}

static as_value sprite_hit_test(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  switch (fn.nargs)
  {
    case 1: // target
    {
      as_value& tgt_val = fn.arg(0);
      character* target = fn.env().find_target(tgt_val.to_string());
      if ( ! target )
      {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Can't find hitTest target %s"),
          tgt_val.to_debug_string());
        );
        return as_value();
      }

      geometry::Range2d<float> thisbounds = sprite->getBounds();
      matrix thismat = sprite->get_world_matrix();
      thismat.transform(thisbounds);

      geometry::Range2d<float> tgtbounds = target->getBounds();
      matrix tgtmat = target->get_world_matrix();
      tgtmat.transform(tgtbounds);

      return thisbounds.intersects(tgtbounds);

      break;
    }

    case 2: // x, y
    {
      float x = PIXELS_TO_TWIPS(fn.arg(0).to_number());
      float y = PIXELS_TO_TWIPS(fn.arg(1).to_number());

      return sprite->pointInBounds(x, y);
    }

    case 3: // x, y, shapeFlag
    {
      double x = PIXELS_TO_TWIPS(fn.arg(0).to_number());
      double y = PIXELS_TO_TWIPS(fn.arg(1).to_number());
      bool shapeFlag = fn.arg(2).to_bool();

      if ( ! shapeFlag ) return sprite->pointInBounds(x, y);
      else return sprite->pointInVisibleShape(x, y);
    }

    default:
    {
      IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("hitTest() called with %u args"),
          fn.nargs);
      );
      break;
    }
  }

  return as_value();

}

static as_value
sprite_create_text_field(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if (fn.nargs < 6) // name, depth, x, y, width, height
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("createTextField called with %d args, "
      "expected 6 - returning undefined"), fn.nargs);
    );
    return as_value();
  }

  std::string txt_name = fn.arg(0).to_string();

  int txt_depth = fn.arg(1).to_int();

  int txt_x = fn.arg(2).to_int();

  int txt_y = fn.arg(3).to_int();

  int txt_width = fn.arg(4).to_int();
  if ( txt_width < 0 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("createTextField: negative width (%d)"
      " - reverting sign"), txt_width);
    );
    txt_width = -txt_width;
  }

  int txt_height = fn.arg(5).to_int();
  if ( txt_height < 0 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("createTextField: negative height (%d)"
      " - reverting sign"), txt_height);
    );
    txt_height = -txt_height;
  }

  boost::intrusive_ptr<character> txt = sprite->add_textfield(txt_name,
      txt_depth, txt_x, txt_y, txt_width, txt_height);

  // createTextField returns void, it seems
  if ( VM::get().getSWFVersion() > 7 ) return as_value(txt.get());
  else return as_value(); 
}

//getNextHighestDepth() : Number
static as_value
sprite_getNextHighestDepth(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  int nextdepth = sprite->getNextHighestDepth();
  return as_value(static_cast<double>(nextdepth));
}

//getInstanceAtDepth(depth:Number) : MovieClip
static as_value
sprite_getInstanceAtDepth(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if ( fn.nargs < 1 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror("MovieClip.getInstanceAtDepth(): missing depth argument");
    );
    return as_value();
  }

  int depth = fn.arg(0).to_number<int>();
  boost::intrusive_ptr<character> ch = sprite->get_character_at_depth(depth);
  if ( ! ch ) return as_value(); // we want 'undefined', not 'null'
  return as_value(ch.get());
}

// getURL(url:String, [window:String], [method:String]) : Void
static as_value
sprite_getURL(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  ONCE( log_unimpl("MovieClip.getURL()") );
  return as_value();
}

// getSWFVersion() : Number
static as_value
sprite_getSWFVersion(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  return as_value(sprite->getSWFVersion());
}

// MovieClip.meth(<string>) : Number
//
// Parses case-insensitive "get" and "post" into 1 and 2, 0 anything else
// 
static as_value
sprite_meth(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if ( ! fn.nargs ) return as_value(0); // optimization ...

  as_value v = fn.arg(0);
  boost::intrusive_ptr<as_object> o = v.to_object();
  if ( ! o )
  {
    log_debug("meth(%s): first argument doesn't cast to object", v.to_debug_string());
    return as_value(0);
  }

  string_table& st = sprite->getVM().getStringTable();
  as_value lc = o->callMethod(st.find(PROPNAME("toLowerCase")));

  log_debug("after call to toLowerCase with arg %s we got %s", v.to_debug_string(), lc.to_debug_string());

  //if ( ! v.is_string() ) return as_value(0);
  std::string s = lc.to_string();

  if ( s == "get" ) return as_value(1);
  if ( s == "post" )  return as_value(2);
  return as_value(0);
}

// getTextSnapshot() : TextSnapshot
static as_value
sprite_getTextSnapshot(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  ONCE( log_unimpl("MovieClip.getTextSnapshot()") );
  return as_value();
}

// getBounds(targetCoordinateSpace:Object) : Object
static as_value
sprite_getBounds(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);


  geometry::Range2d<float> bounds  = sprite->getBounds();

  if ( fn.nargs > 0 )
  {
    boost::intrusive_ptr<sprite_instance> target = fn.arg(0).to_sprite();
    if ( ! target )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("MovieClip.getBounds(%s): invalid call, first arg must be a sprite"),
        fn.arg(0).to_debug_string());
      );
      return as_value();
    }

    matrix tgtwmat = target->get_world_matrix();
    matrix srcwmat = sprite->get_world_matrix();
    matrix invtgtwmat; invtgtwmat.set_inverse(tgtwmat);
    matrix m = srcwmat;
    m.concatenate(invtgtwmat);


    srcwmat.transform(bounds);
    tgtwmat.transform_by_inverse(bounds);
  }

  // Magic numbers here... dunno why
  double xMin = 6710886.35;
  double yMin = 6710886.35;
  double xMax = 6710886.35;
  double yMax = 6710886.35;

  if ( bounds.isFinite() )
  {
    // Round to the twip
    xMin = int(rint(bounds.getMinX())) / 20.0f;
    yMin = int(rint(bounds.getMinY())) / 20.0f;
    xMax = int(rint(bounds.getMaxX())) / 20.0f;
    yMax = int(rint(bounds.getMaxY())) / 20.0f;
  }

  boost::intrusive_ptr<as_object> bounds_obj(new as_object());
  bounds_obj->init_member("xMin", as_value(xMin));
  bounds_obj->init_member("yMin", as_value(yMin));
  bounds_obj->init_member("xMax", as_value(xMax));
  bounds_obj->init_member("yMax", as_value(yMax));

  return as_value(bounds_obj.get());
}

static as_value
sprite_globalToLocal(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  as_value ret;

  if ( fn.nargs < 1 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.globalToLocal() takes one arg"));
    );
    return ret;
  }

  boost::intrusive_ptr<as_object> obj = fn.arg(0).to_object();
  if ( ! obj )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.globalToLocal(%s): "
        "first argument doesn't cast to an object"),
      fn.arg(0).to_debug_string());
    );
    return ret;
  }

  as_value tmp;
  float x = 0;
  float y = 0;

  if ( ! obj->get_member(NSV::PROP_X, &tmp) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.globalToLocal(%s): "
        "object parameter doesn't have an 'x' member"),
      fn.arg(0).to_debug_string());
    );
    return ret;
  }
  x = PIXELS_TO_TWIPS(tmp.to_number());

  if ( ! obj->get_member(NSV::PROP_Y, &tmp) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.globalToLocal(%s): "
        "object parameter doesn't have an 'y' member"),
      fn.arg(0).to_debug_string());
    );
    return ret;
  }
  y = PIXELS_TO_TWIPS(tmp.to_number());

  point pt(x, y);
  matrix world_mat = sprite->get_world_matrix();
  world_mat.transform_by_inverse(pt);

  obj->set_member(NSV::PROP_X, TWIPS_TO_PIXELS(round(pt.x)));
  obj->set_member(NSV::PROP_Y, TWIPS_TO_PIXELS(round(pt.y)));

  return ret;
}

static as_value
sprite_localToGlobal(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  as_value ret;

  if ( fn.nargs < 1 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.localToGlobal() takes one arg"));
    );
    return ret;
  }

  boost::intrusive_ptr<as_object> obj = fn.arg(0).to_object();
  if ( ! obj )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.localToGlobal(%s): "
        "first argument doesn't cast to an object"),
      fn.arg(0).to_debug_string());
    );
    return ret;
  }

  as_value tmp;
  float x = 0;
  float y = 0;

  if ( ! obj->get_member(NSV::PROP_X, &tmp) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.localToGlobal(%s): "
        "object parameter doesn't have an 'x' member"),
      fn.arg(0).to_debug_string());
    );
    return ret;
  }
  x = PIXELS_TO_TWIPS(tmp.to_number());

  if ( ! obj->get_member(NSV::PROP_Y, &tmp) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("MovieClip.localToGlobal(%s): "
        "object parameter doesn't have an 'y' member"),
      fn.arg(0).to_debug_string());
    );
    return ret;
  }
  y = PIXELS_TO_TWIPS(tmp.to_number());

  point pt(x, y);
  matrix world_mat = sprite->get_world_matrix();
  world_mat.transform(pt);

  obj->set_member(NSV::PROP_X, TWIPS_TO_PIXELS(round(pt.x)));
  obj->set_member(NSV::PROP_Y, TWIPS_TO_PIXELS(round(pt.y)));

  return ret;

}

static as_value
sprite_setMask(const fn_call& fn)
{
  // swfdec/test/image/mask-textfield-6.swf shows that setMask should also
  // work against TextFields, we have no tests for other character types so
  // we generalize it for any character.
  boost::intrusive_ptr<character> maskee = ensureType<character>(fn.this_ptr);

  if ( ! fn.nargs )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("%s.setMask() : needs an argument"), maskee->getTarget());
    );
    return as_value();
  }

  as_value& arg = fn.arg(0);
  if ( arg.is_null() || arg.is_undefined() )
  {
    // disable mask
    maskee->setMask(NULL);
  }
  else
  {

    boost::intrusive_ptr<as_object> obj ( arg.to_object() );
    character* mask = dynamic_cast<character*>(obj.get());
    if ( ! mask )
    {
      IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("%s.setMask(%s) : first argument is not a character"),
        maskee->getTarget(), arg.to_debug_string());
      );
      return as_value();
    }

    // ch is possibly NULL, which is intended
    maskee->setMask(mask);
  }

  //log_debug("MovieClip.setMask() TESTING");

  return as_value(true);
}

static as_value
sprite_endFill(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
#ifdef DEBUG_DRAWING_API
  log_debug("%s.endFill();", sprite->getTarget());
#endif
  sprite->endFill();
  return as_value();
}

static as_value
sprite_lineTo(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if ( fn.nargs < 2 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("MovieClip.lineTo() takes two args"));
    );
    return as_value();
  }

  float x = PIXELS_TO_TWIPS(fn.arg(0).to_number());
  float y = PIXELS_TO_TWIPS(fn.arg(1).to_number());

  if ( ! isfinite(x) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.lineTo(%s) : non-finite first argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(0).to_debug_string());
    );
    x = 0;
  }
   
  if ( ! isfinite(y) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.lineTo(%s) : non-finite second argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(1).to_debug_string());
    );
    y = 0;
  }

#ifdef DEBUG_DRAWING_API
  log_debug("%s.lineTo(%g,%g);", sprite->getTarget(), x, y);
#endif
  sprite->lineTo(x, y);

  return as_value();
}

static as_value
sprite_moveTo(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if ( fn.nargs < 2 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("MovieClip.moveTo() takes two args"));
    );
    return as_value();
  }

  float x = PIXELS_TO_TWIPS(fn.arg(0).to_number());
  float y = PIXELS_TO_TWIPS(fn.arg(1).to_number());

  if ( ! isfinite(x) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.moveTo(%s) : non-finite first argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(0).to_debug_string());
    );
    x = 0;
  }
   
  if ( ! isfinite(y) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.moveTo(%s) : non-finite second argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(1).to_debug_string());
    );
    y = 0;
  }

#ifdef DEBUG_DRAWING_API
  log_debug("%s.moveTo(%g,%g);", sprite->getTarget(), x, y);
#endif
  sprite->moveTo(x, y);

  return as_value();
}

static as_value
sprite_lineStyle(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  boost::uint16_t thickness = 0;
  boost::uint8_t r = 0;
  boost::uint8_t g = 0;
  boost::uint8_t b = 0;
  boost::uint8_t a = 255;


  if ( ! fn.nargs )
  {
    sprite->resetLineStyle();
    return as_value();
  }

  thickness = boost::uint16_t(PIXELS_TO_TWIPS(boost::uint16_t(fclamp(fn.arg(0).to_number(), 0, 255))));

  if ( fn.nargs > 1 )
  {
    // 2^24 is the max here
    boost::uint32_t rgbval = boost::uint32_t(fclamp(fn.arg(1).to_number(), 0, 16777216));
    r = boost::uint8_t( (rgbval&0xFF0000) >> 16);
    g = boost::uint8_t( (rgbval&0x00FF00) >> 8);
    b = boost::uint8_t( (rgbval&0x0000FF) );

    if ( fn.nargs > 2 )
    {
      float alphaval = fclamp(fn.arg(2).to_number(), 0, 100);
      a = boost::uint8_t( 255 * (alphaval/100) );
    }
  }


  rgba color(r, g, b, a);
  //log_debug("Color: %s", color.toString());

#ifdef DEBUG_DRAWING_API
  log_debug("%s.lineStyle(%d,%d,%d,%d);", sprite->getTarget(), thickness, r, g, b);
#endif
  sprite->lineStyle(thickness, color);

  return as_value();
}

static as_value
sprite_curveTo(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if ( fn.nargs < 4 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("MovieClip.curveTo() takes four args"));
    );
    return as_value();
  }

  float cx = PIXELS_TO_TWIPS(fn.arg(0).to_number());
  float cy = PIXELS_TO_TWIPS(fn.arg(1).to_number());
  float ax = PIXELS_TO_TWIPS(fn.arg(2).to_number());
  float ay = PIXELS_TO_TWIPS(fn.arg(3).to_number());

  if ( ! isfinite(cx) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.curveTo(%s) : non-finite first argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(0).to_debug_string());
    );
    cx = 0;
  }
   
  if ( ! isfinite(cy) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.curveTo(%s) : non-finite second argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(1).to_debug_string());
    );
    cy = 0;
  }

  if ( ! isfinite(ax) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.curveTo(%s) : non-finite third argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(0).to_debug_string());
    );
    ax = 0;
  }
   
  if ( ! isfinite(ay) )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror("%s.curveTo(%s) : non-finite fourth argument (%s), "
      "converted to zero", sprite->getTarget(),
      ss.str(), fn.arg(1).to_debug_string());
    );
    ay = 0;
  }

#ifdef DEBUG_DRAWING_API
  log_debug("%s.curveTo(%g,%g,%g,%g);", sprite->getTarget(), cx, cy, ax, ay);
#endif
  sprite->curveTo(cx, cy, ax, ay);

  return as_value();
}

static as_value
sprite_clear(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

#ifdef DEBUG_DRAWING_API
  log_debug("%s.clear();", sprite->getTarget());
#endif
  sprite->clear();

  return as_value();
}

static as_value
sprite_beginFill(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  boost::uint8_t r = 0;
  boost::uint8_t g = 0;
  boost::uint8_t b = 0;
  boost::uint8_t a = 255;

  if ( fn.nargs > 0 )
  {
    // 2^24 is the max here
    boost::uint32_t rgbval = boost::uint32_t(fclamp(fn.arg(0).to_number(), 0, 16777216));
    r = boost::uint8_t( (rgbval&0xFF0000) >> 16);
    g = boost::uint8_t( (rgbval&0x00FF00) >> 8);
    b = boost::uint8_t( (rgbval&0x0000FF) );

    if ( fn.nargs > 1 )
    {
      a = 255 * iclamp(fn.arg(1).to_int(), 0, 100) / 100;
    }

  }

  rgba color(r, g, b, a);

#ifdef DEBUG_DRAWING_API
  log_debug("%s.beginFill(%d,%d,%d);", sprite->getTarget(), r, g, b);
#endif
  sprite->beginFill(color);

  return as_value();
}

static as_value
sprite_beginGradientFill(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

  if ( fn.nargs < 5 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror(_("%s.beginGradientFill(%s): invalid call: 5 arguments needed"),
      sprite->getTarget(), ss.str());
    );
    return as_value();
  }

  bool radial = false;
  string typeStr = fn.arg(0).to_string();
  // Case-sensitive comparison needed for this ...
  if ( typeStr == "radial" ) radial = true;
  else if ( typeStr == "linear" ) radial = false;
  else
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror(_("%s.beginGradientFill(%s): first arg must be "
      "'radial' or 'linear'"),
      sprite->getTarget(), ss.str());
    );
    return as_value();
  }

  typedef boost::intrusive_ptr<as_object> ObjPtr;

  ObjPtr colors = fn.arg(1).to_object();
  ObjPtr alphas = fn.arg(2).to_object();
  ObjPtr ratios = fn.arg(3).to_object();
  ObjPtr matrixArg = fn.arg(4).to_object();

  if ( ! colors || ! alphas || ! ratios || ! matrixArg )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror(_("%s.beginGradientFill(%s): one or more of the "
      " args from 2nd to 5th don't cast to objects"),
      sprite->getTarget(), ss.str());
    );
    return as_value();
  }

  VM& vm = sprite->getVM();
  string_table& st = vm.getStringTable();

  // ----------------------------
  // Parse matrix
  // ----------------------------
  
  //
  // TODO: fix the matrix build-up, it is NOT correct for
  //       rotation.
  //       For the "boxed" matrixType and radial fills this
  //       is not a problem as this code just discards the
  //       rotation (which doesn't make sense), but for
  //       the explicit matrix type (a..i) it is a problem.
  //       The whole code can likely be simplified by 
  //       always transforming the gnash gradients to the
  //       expected gradients and subsequently applying
  //       user-specified matrix; for 'boxed' matrixType
  //       this simplification would increas cost, but
  //       it's too early to apply optimizations to the
  //       code (correctness first!!).
  //

  matrix mat;
  matrix input_matrix;

  string_table::key keyT = st.find(PROPNAME("matrixType"));
  if ( matrixArg->getMember(keyT).to_string() == "box" )
  {
    
    // TODO: add to namedStrings.{cpp,h}
    static const string_table::key keyX = st.find("x");
    static const string_table::key keyY = st.find("y");
    static const string_table::key keyW = st.find("w");
    static const string_table::key keyH = st.find("h");
    static const string_table::key keyR = st.find("r");

    float valX = PIXELS_TO_TWIPS(matrixArg->getMember(keyX).to_number()); 
    float valY = PIXELS_TO_TWIPS(matrixArg->getMember(keyY).to_number()); 
    float valW = PIXELS_TO_TWIPS(matrixArg->getMember(keyW).to_number()); 
    float valH = PIXELS_TO_TWIPS(matrixArg->getMember(keyH).to_number()); 
    float valR = matrixArg->getMember(keyR).to_number(); 

    if ( radial )
    {
      // Radial gradient is 64x64 twips.
      input_matrix.set_scale(64.0f/valW, 64.0f/valH);

      // For radial gradients, dunno why translation must be negative...
      input_matrix.concatenate_translation( -valX, -valY );

      // NOTE: rotation is intentionally discarded as it would
      //       have no effect (theoretically origin of the radial
      //       fill is at 0,0 making any rotation meaningless).

    }
    else
    {
      // Linear gradient is 256x1 twips.
      //
      // No idea why we should use the 256 value for Y scale, but empirically
      // seems to give closer results. Note that it only influences rotation,
      // which is still not correct... TODO: fix it !
      //
      input_matrix.set_scale_rotation(256.0f/valW, 256.0f/valH, -valR);

      // For linear gradients, dunno why translation must be negative...
      input_matrix.concatenate_translation( -valX, -valY );
      //cout << "inpt matrix with concatenated translation: " << input_matrix << endl;

    }

    mat.concatenate(input_matrix);
  }
  else
  {
    // TODO: add to namedStrings.{cpp,h}
    static const string_table::key keyA = st.find("a");
    static const string_table::key keyB = st.find("b");
    static const string_table::key keyD = st.find("d");
    static const string_table::key keyE = st.find("e");
    static const string_table::key keyG = st.find("g");
    static const string_table::key keyH = st.find("h");

    float valA = matrixArg->getMember(keyA).to_number() ; // xx
    float valB = matrixArg->getMember(keyB).to_number() ; // yx
    float valD = matrixArg->getMember(keyD).to_number() ; // xy
    float valE = matrixArg->getMember(keyE).to_number() ; // yy
    float valG = PIXELS_TO_TWIPS(matrixArg->getMember(keyG).to_number()); // x0
    float valH = PIXELS_TO_TWIPS(matrixArg->getMember(keyH).to_number()); // y0

    input_matrix.m_[0][0] = valA; // xx
    input_matrix.m_[1][0] = valB; // yx
    input_matrix.m_[0][1] = valD; // xy
    input_matrix.m_[1][1] = valE; // yy
    input_matrix.m_[0][2] = valG; // x0
    input_matrix.m_[1][2] = valH; // y0

    // This is the matrix that would transform the gnash
    // gradient to the expected flash gradient.
    // Transformation is different for linear and radial
    // gradient for Gnash (in flash they should be the same)
    matrix gnashToFlash;

    if ( radial )
    {

      // Gnash radial gradients are 64x64 with center at 32,32
      // Should be 20x20 with center at 0,0
      float g2fs = 20.0/64.0; // gnash to flash scale
      gnashToFlash.set_scale(g2fs, g2fs);
      gnashToFlash.concatenate_translation(-32.0, -32.0);

    }
    else
    {
      // First define a matrix that would transform
      // the gnash gradient to the expected flash gradient:
      // this means translating our gradient to put the
      // center of gradient at 0,0 and then scale it to
      // have a size of 20x20 instead of 256x1 as it is
      //
      // Gnash linear gradients are 256x1 with center at 128,0
      // Should be 20x20 with center at 0,0
      gnashToFlash.set_scale(20.0/256.0, 20.0/1);
      gnashToFlash.concatenate_translation(-128.0, 0.0);

    }

    // Apply gnash to flash matrix before user-defined one
    input_matrix.concatenate(gnashToFlash);

    // Finally, and don't know why, take
    // the inverse of the resulting matrix as
    // the one which would be used
    mat.set_inverse( input_matrix );

  }

  //cout << mat << endl;

  // ----------------------------
  // Create the gradients vector
  // ----------------------------

  size_t ngradients = colors->getMember(NSV::PROP_LENGTH).to_int();
  // Check length compatibility of all args
  if ( ngradients != (size_t)alphas->getMember(NSV::PROP_LENGTH).to_int() ||
    ngradients != (size_t)ratios->getMember(NSV::PROP_LENGTH).to_int() )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    std::stringstream ss; fn.dump_args(ss);
    log_aserror(_("%s.beginGradientFill(%s): colors, alphas and "
      "ratios args don't have same length"),
      sprite->getTarget(), ss.str());
    );
    return as_value();
  }

  // TODO: limit ngradients to a max ?
  if ( ngradients > 8 )
  {
    std::stringstream ss; fn.dump_args(ss);
    log_debug("%s.beginGradientFill(%s) : too many array elements"
      " for colors and ratios (%d), trim to 8", 
      sprite->getTarget(), ss.str(), ngradients); 
    ngradients = 8;
  }

  std::vector<gradient_record> gradients;
  gradients.reserve(ngradients);
  for (size_t i=0; i<ngradients; ++i)
  {
    char buf[32];
    sprintf(buf, SIZET_FMT, i);
    string_table::key key = st.find(buf);

    as_value colVal = colors->getMember(key);
    boost::uint32_t col = colVal.is_number() ? colVal.to_int() : 0;

    as_value alpVal = alphas->getMember(key);
    boost::uint8_t alp = alpVal.is_number() ? iclamp(alpVal.to_int(), 0, 255) : 0;

    as_value ratVal = ratios->getMember(key);
    boost::uint8_t rat = ratVal.is_number() ? iclamp(ratVal.to_int(), 0, 255) : 0;

    rgba color;
    color.parseRGB(col);
    color.m_a = alp;

    gradients.push_back(gradient_record(rat, color));
  }

  if ( radial )
  {
    sprite->beginRadialGradientFill(gradients, mat);
  }
  else
  {
    sprite->beginLinearGradientFill(gradients, mat);
  }

  ONCE( log_debug("MovieClip.beginGradientFill() TESTING") );
  return as_value();
}

// startDrag([lockCenter:Boolean], [left:Number], [top:Number],
//  [right:Number], [bottom:Number]) : Void`
static as_value
sprite_startDrag(const fn_call& fn)
{
    boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);

    drag_state st;
    st.setCharacter( sprite.get() );

    // mark this character is transformed.
    sprite->transformedByScript();

    if ( fn.nargs )
    {
        st.setLockCentered( fn.arg(0).to_bool() );

        if ( fn.nargs >= 5)
        {
            float x0 = PIXELS_TO_TWIPS(fn.arg(1).to_number());
            float y0 = PIXELS_TO_TWIPS(fn.arg(2).to_number());
            float x1 = PIXELS_TO_TWIPS(fn.arg(3).to_number());
            float y1 = PIXELS_TO_TWIPS(fn.arg(4).to_number());

            // check for infinite values
            bool gotinf = false;
            if ( ! isfinite(x0) ) { x0=0; gotinf=true; }
            if ( ! isfinite(y0) ) { y0=0; gotinf=true; }
            if ( ! isfinite(x1) ) { x1=0; gotinf=true; }
            if ( ! isfinite(y1) ) { y1=0; gotinf=true; }

            // check for swapped values
            bool swapped = false;
            if ( y1 < y0 )
            {
                swap(y1, y0);
                swapped = true;
            }

            if ( x1 < x0 )
            {
                swap(x1, x0);
                swapped = true;
            }

            IF_VERBOSE_ASCODING_ERRORS(
            if ( gotinf || swapped ) {
                std::stringstream ss; fn.dump_args(ss);
		if ( swapped ) 
		  log_aserror(_("min/max bbox values in MovieClip.startDrag(%s) swapped, fixing"), ss.str());
		if ( gotinf )
                  log_aserror(_("non-finite bbox values in MovieClip.startDrag(%s), took as zero"), ss.str());
            }
            );

            rect bounds(x0, y0, x1, y1);
            st.setBounds(bounds);
        }
    }

    VM::get().getRoot().set_drag_state(st);

    log_debug("MovieClip.startDrag() TESTING");
    return as_value();
}

// stopDrag() : Void
static as_value
sprite_stopDrag(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> sprite = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(sprite);

  VM::get().getRoot().stop_drag();

  log_debug("MovieClip.stopDrag() TESTING");
  return as_value();
}

static as_value
movieclip_ctor(const fn_call& /* fn */)
{
  boost::intrusive_ptr<as_object> clip = new as_object(getMovieClipInterface());
  //attachMovieClipProperties(*clip);
  return as_value(clip.get());
}


static as_value
sprite_currentframe_get(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);

  return as_value(std::min(ptr->get_loaded_frames(), ptr->get_current_frame() + 1));
}

static as_value
sprite_totalframes_get(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);

  return as_value(ptr->get_frame_count());
}

static as_value
sprite_framesloaded_get(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);

  return as_value(ptr->get_loaded_frames());
}

static as_value
sprite_droptarget_getset(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);

  return ptr->getDropTarget();
}

static as_value
sprite_url_getset(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);

  return as_value(ptr->get_movie_definition()->get_url());
}

static as_value
sprite_highquality_getset(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(ptr);

  if ( fn.nargs == 0 ) // getter
  {
    // We don't support quality settings
    return as_value(true);
  }
  else // setter
  {
    ONCE( log_unimpl("MovieClip._highquality setting") );
  }
  return as_value();
}

// TODO: move this to character class, _focusrect seems a generic property
static as_value
sprite_focusrect_getset(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(ptr);

  if ( fn.nargs == 0 ) // getter
  {
    // Is a yellow rectangle visible around a focused movie clip (?)
    // We don't support focuserct settings
    return as_value(false);
  }
  else // setter
  {
    ONCE( log_unimpl("MovieClip._focusrect setting") );
  }
  return as_value();
}

static as_value
sprite_soundbuftime_getset(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);
  UNUSED(ptr);

  if ( fn.nargs == 0 ) // getter
  {
    // Number of seconds before sound starts to stream.
    return as_value(0.0);
  }
  else // setter
  {
    ONCE( log_unimpl("MovieClip._soundbuftime setting") );
  }
  return as_value();
}

static void registerNatives(VM& vm)
{
  // Natives are always here  (at least in swf5 I guess)
  vm.registerNative(sprite_attach_movie, 900, 0); 
  vm.registerNative(sprite_swap_depths, 900, 1); // TODO: generalize to character::swapDepths_method ?
  vm.registerNative(sprite_localToGlobal, 900, 2);
  vm.registerNative(sprite_globalToLocal, 900, 3);
  vm.registerNative(sprite_hit_test, 900, 4);
  vm.registerNative(sprite_getBounds, 900, 5);
  vm.registerNative(sprite_get_bytes_total, 900, 6);
  vm.registerNative(sprite_get_bytes_loaded, 900, 7);
  vm.registerNative(sprite_attach_audio, 900, 8);
  vm.registerNative(sprite_attach_video, 900, 9);
  vm.registerNative(sprite_get_depth, 900, 10); // TODO: generalize to character::getDepth_method ?
  vm.registerNative(sprite_setMask, 900, 11); 
  vm.registerNative(sprite_play, 900, 12); 
  vm.registerNative(sprite_stop, 900, 13);
  vm.registerNative(sprite_next_frame, 900, 14);
  vm.registerNative(sprite_prev_frame, 900, 15);
  vm.registerNative(sprite_goto_and_play, 900, 16);
  vm.registerNative(sprite_goto_and_stop, 900, 17);
  vm.registerNative(sprite_duplicate_movieclip, 900, 18);
  vm.registerNative(sprite_remove_movieclip, 900, 19);
  vm.registerNative(sprite_startDrag, 900, 20);
  vm.registerNative(sprite_stopDrag, 900, 21);

  // TODO: tabIndex getter-setter

  vm.registerNative(sprite_create_empty_movieclip, 901, 0);
  vm.registerNative(sprite_beginFill, 901, 1);
  vm.registerNative(sprite_beginGradientFill, 901, 2);
  vm.registerNative(sprite_moveTo, 901, 3);
  vm.registerNative(sprite_lineTo, 901, 4);
  vm.registerNative(sprite_curveTo, 901, 5);
  vm.registerNative(sprite_lineStyle, 901, 6);
  vm.registerNative(sprite_endFill, 901, 7);
  vm.registerNative(sprite_clear, 901, 8);

  vm.registerNative(sprite_create_text_field, 104, 200);

}

/// Properties (and/or methods) *inherited* by MovieClip instances
static void
attachMovieClipInterface(as_object& o)
{
  VM& vm = o.getVM();
  int target_version = vm.getSWFVersion();

  // SWF5 or higher
  o.init_member("attachMovie", vm.getNative(900, 0)); 
  o.init_member("swapDepths", vm.getNative(900, 1));
  o.init_member("localToGlobal", vm.getNative(900, 2));
  o.init_member("globalToLocal", vm.getNative(900, 3));
  o.init_member("hitTest", vm.getNative(900, 4));
  o.init_member("getBounds", vm.getNative(900, 5));
  o.init_member("getBytesTotal", vm.getNative(900, 6));
  o.init_member("getBytesLoaded", vm.getNative(900, 7));
  o.init_member("play", vm.getNative(900, 12));
  o.init_member("stop", vm.getNative(900, 13));
  o.init_member("nextFrame", vm.getNative(900, 14));
  o.init_member("prevFrame", vm.getNative(900, 15));
  o.init_member("gotoAndPlay", vm.getNative(900, 16));
  o.init_member("gotoAndStop", vm.getNative(900, 17));
  o.init_member("duplicateMovieClip", vm.getNative(900, 18));
  o.init_member("removeMovieClip", vm.getNative(900, 19));
  o.init_member("startDrag", vm.getNative(900, 20));
  o.init_member("stopDrag", vm.getNative(900, 21));
  o.init_member("loadMovie", new builtin_function(sprite_load_movie));
  o.init_member("loadVariables", new builtin_function(sprite_load_variables));
  o.init_member("unloadMovie", new builtin_function(sprite_unload_movie));
  o.init_member("getURL", new builtin_function(sprite_getURL));
  o.init_member("getSWFVersion", new builtin_function(sprite_getSWFVersion));
  o.init_member("meth", new builtin_function(sprite_meth));
  o.init_member("enabled", true); // see MovieClip.as testcase

  as_c_function_ptr gettersetter = &sprite_instance::lockroot_getset;
  o.init_property("_lockroot", *gettersetter, *gettersetter); // see MovieClip.as testcase

  if ( target_version  < 6 ) return;

  // SWF6 or higher
  o.init_member("attachAudio", vm.getNative(900, 8));
  o.init_member("attachVideo", vm.getNative(900, 9));
  o.init_member("getDepth", vm.getNative(900, 10));
  o.init_member("setMask", vm.getNative(900, 11));
  o.init_member("createEmptyMovieClip", vm.getNative(901, 0));
  o.init_member("beginFill", vm.getNative(901, 1));
  o.init_member("beginGradientFill", vm.getNative(901, 2));
  o.init_member("moveTo", vm.getNative(901, 3));
  o.init_member("lineTo", vm.getNative(901, 4));
  o.init_member("curveTo", vm.getNative(901, 5));
  o.init_member("lineStyle", vm.getNative(901, 6));
  o.init_member("endFill", vm.getNative(901, 7));
  o.init_member("clear", vm.getNative(901, 8));

  o.init_member("createTextField", vm.getNative(104, 200));
  o.init_member("getTextSnapshot", new builtin_function(sprite_getTextSnapshot)); // unknown native
  if ( target_version  < 7 ) return;

  // SWF7 or higher
  o.init_member("getNextHighestDepth", new builtin_function(sprite_getNextHighestDepth));
  o.init_member("getInstanceAtDepth", new builtin_function(sprite_getInstanceAtDepth));
  if ( target_version  < 8 ) return;

  // TODO: many more methods, see MovieClip class ...

}

/// Properties (and/or methods) attached to every *instance* of a MovieClip 
static void
attachMovieClipProperties(character& o)
{
  //int target_version = o.getVM().getSWFVersion();

  // This is a normal property, can be overridden, deleted and enumerated
  // See swfdec/test/trace/movieclip-version-#.swf for why we only initialize this
  // if we don't have a parent
  if ( ! o.get_parent() ) o.init_member( "$version", VM::get().getPlayerVersion(), 0); 

  //
  // Properties (TODO: move to appropriate SWF version section)
  //
  
  as_c_function_ptr gettersetter;

  gettersetter = character::x_getset;
  o.init_property(NSV::PROP_uX, gettersetter, gettersetter);

  gettersetter = character::y_getset;
  o.init_property(NSV::PROP_uY, gettersetter, gettersetter);

  gettersetter = character::xscale_getset;
  o.init_property(NSV::PROP_uXSCALE, gettersetter, gettersetter);

  gettersetter = character::yscale_getset;
  o.init_property(NSV::PROP_uYSCALE, gettersetter, gettersetter);

  gettersetter = character::xmouse_get;
  o.init_readonly_property(NSV::PROP_uXMOUSE, gettersetter);

  gettersetter = character::ymouse_get;
  o.init_readonly_property(NSV::PROP_uYMOUSE, gettersetter);

  gettersetter = character::alpha_getset;
  o.init_property(NSV::PROP_uALPHA, gettersetter, gettersetter);

  gettersetter = character::visible_getset;
  o.init_property(NSV::PROP_uVISIBLE, gettersetter, gettersetter);

  gettersetter = character::width_getset;
  o.init_property(NSV::PROP_uWIDTH, gettersetter, gettersetter);

  gettersetter = character::height_getset;
  o.init_property(NSV::PROP_uHEIGHT, gettersetter, gettersetter);

  gettersetter = character::rotation_getset;
  o.init_property(NSV::PROP_uROTATION, gettersetter, gettersetter);

  gettersetter = character::parent_getset;
  o.init_property(NSV::PROP_uPARENT, gettersetter, gettersetter);

  gettersetter = sprite_currentframe_get;
  o.init_property(NSV::PROP_uCURRENTFRAME, gettersetter, gettersetter);

  gettersetter = sprite_totalframes_get;
  o.init_property(NSV::PROP_uTOTALFRAMES, gettersetter, gettersetter);

  gettersetter = sprite_framesloaded_get;
  o.init_property(NSV::PROP_uFRAMESLOADED, gettersetter, gettersetter);

  gettersetter = character::target_getset;
  o.init_property(NSV::PROP_uTARGET, gettersetter, gettersetter);

  gettersetter = character::name_getset;
  o.init_property(NSV::PROP_uNAME, gettersetter, gettersetter);

  gettersetter = sprite_droptarget_getset;
  o.init_property(NSV::PROP_uDROPTARGET, gettersetter, gettersetter);

  gettersetter = sprite_url_getset;
  o.init_property(NSV::PROP_uURL, gettersetter, gettersetter);

  gettersetter = sprite_highquality_getset;
  o.init_property(NSV::PROP_uHIGHQUALITY, gettersetter, gettersetter);

  gettersetter = sprite_focusrect_getset;
  o.init_property(NSV::PROP_uFOCUSRECT, gettersetter, gettersetter);

  gettersetter = sprite_soundbuftime_getset;
  o.init_property(NSV::PROP_uSOUNDBUFTIME, gettersetter, gettersetter);

}

static as_object*
getMovieClipInterface()
{
  static boost::intrusive_ptr<as_object> proto;
  if ( proto == NULL )
  {
    proto = new as_object(getObjectInterface());
    VM& vm = VM::get();
    vm.addStatic(proto.get());
    registerNatives(vm);
    attachMovieClipInterface(*proto);
    //proto->init_member("constructor", new builtin_function(movieclip_ctor));
  }
  return proto.get();
}

void
movieclip_class_init(as_object& global)
{
  // This is going to be the global MovieClip "class"/"function"
  static boost::intrusive_ptr<builtin_function> cl=NULL;

  if ( cl == NULL )
  {
    cl=new builtin_function(&movieclip_ctor, getMovieClipInterface());
    VM::get().addStatic(cl.get());

    // replicate all interface to class, to be able to access
    // all methods as static functions
    //attachMovieClipInterface(*cl);
         
  }

  // Register _global.MovieClip
  global.init_member("MovieClip", cl.get());
}


//------------------------------------------------
// sprite_instance helper classes
//------------------------------------------------

/// A DisplayList visitor used to compute its overall bounds.
//
class BoundsFinder {
public:
  geometry::Range2d<float>& _bounds;
  BoundsFinder(geometry::Range2d<float>& b)
    :
    _bounds(b)
  {}
  void operator() (character* ch)
  {
    // don't include bounds of unloaded characters
    if ( ch->isUnloaded() ) return;
    geometry::Range2d<float> chb = ch->getBounds();
    matrix m = ch->get_matrix();
    m.transform(chb);
    _bounds.expandTo(chb);
  }
};

/// A DisplayList visitor used to extract script characters
//
/// Script characters are characters created or transformed
/// by ActionScript. 
///
class ScriptObjectsFinder {
  std::vector<character*>& _dynamicChars;
  std::vector<character*>& _staticChars;
public:
  ScriptObjectsFinder(std::vector<character*>& dynamicChars,
      std::vector<character*>& staticChars)
    :
    _dynamicChars(dynamicChars),
    _staticChars(staticChars)
  {}

  void operator() (character* ch) 
  {
    // don't include bounds of unloaded characters
    if ( ch->isUnloaded() ) return;

    // TODO: Are script-transformed object to be kept ?
    //       Need a testcase for this
    //if ( ! ch->get_accept_anim_moves() )
    //if ( ch->isDynamic() )
    int depth = ch->get_depth();
    if ( depth < -16384 || depth >= 0 )
    {
      _dynamicChars.push_back(ch);
    }
    else
    {
      _staticChars.push_back(ch);
    }
  }
};

//------------------------------------------------
// sprite_instance
//------------------------------------------------

sprite_instance::sprite_instance(
    movie_definition* def, movie_instance* r,
    character* parent, int id)
  :
  character(parent, id),
  m_root(r),
  m_display_list(),
  _drawable(new DynamicShape),
  _drawable_inst(_drawable->create_character_instance(this, 0)),
  //m_goto_frame_action_list(),
  m_play_state(PLAY),
  m_current_frame(0),
  m_has_looped(false),
  is_jumping_back(false),
  _callingFrameActions(false),
  m_as_environment(),
  _text_variables(),
  m_sound_stream_id(-1),
  _userCxform(),
  _droptarget(),
  _lockroot(false),
  m_def(def)
{
  assert(m_def != NULL);
  assert(m_root != NULL);

  set_prototype(getMovieClipInterface());
      
  //m_root->add_ref();  // @@ circular!
  m_as_environment.set_target(this);

  // TODO: have the 'MovieClip' constructor take care of this !
  attachMovieClipProperties(*this);

}

sprite_instance::~sprite_instance()
{
  stopStreamSound();

  // We might have been deleted by Quit... 
  //assert(isDestroyed());

  _vm.getRoot().remove_key_listener(this);
  _vm.getRoot().remove_mouse_listener(this);

  for (LoadVariablesThreads::iterator it=_loadVariableRequests.begin();
      it != _loadVariableRequests.end(); ++it)
  {
    delete *it;
  }
}

character* sprite_instance::get_character_at_depth(int depth)
{
  return m_display_list.get_character_at_depth(depth);
}

// Set *val to the value of the named member and
// return true, if we have the named member.
// Otherwise leave *val alone and return false.
bool sprite_instance::get_member(string_table::key name_key, as_value* val,
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
    movie_instance* mo = _vm.getRoot().getLevel(levelno).get();
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

  // Try object members, BEFORE display list items!
  // (see testcase VarAndCharClash.swf in testsuite/misc-ming.all)
  //
  // TODO: simplify the next line when get_member_default takes
  //       a std::string
  if (get_member_default(name_key, val, nsname))
  {

// ... trying to be useful to Flash coders ...
// The check should actually be performed before any return
// prior to the one due to a match in the DisplayList.
// It's off by default anyway, so not a big deal.
// See bug #18457
//#define CHECK_FOR_NAME_CLASHES 1
#ifdef CHECK_FOR_NAME_CLASHES
    IF_VERBOSE_ASCODING_ERRORS(
    if (  m_display_list.get_character_by_name_i(name) )
    {
      log_aserror(_("A sprite member (%s) clashes with "
          "the name of an existing character "
          "in its display list.  "
          "The member will hide the "
          "character"), name);
    }
    );
#endif

    return true;
  }


  // Try items on our display list.
  character* ch;
  if ( _vm.getSWFVersion() >= 7 ) ch =  m_display_list.get_character_by_name(name);
  else ch = m_display_list.get_character_by_name_i(name);

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

  // Try textfield variables
  TextFieldPtrVect* etc = get_textfield_variable(name);
  if ( etc )
  {
    for (TextFieldPtrVect::const_iterator i=etc->begin(), e=etc->end(); i!=e; ++i)
    {
	TextFieldPtr tf = *i;
	if ( tf->getTextDefined() )
	{
		val->set_string(tf->get_text_value());
    		return true;
	}
    }
  }

  return false;

}

bool
sprite_instance::get_frame_number(const as_value& frame_spec, size_t& frameno) const
{
  //GNASH_REPORT_FUNCTION;

  std::string fspecStr = frame_spec.to_string();

  as_value str(fspecStr);

  double num =  str.to_number();

  //log_debug("get_frame_number(%s), num: %g", frame_spec.to_debug_string(), num);

  if ( ! isfinite(num) || int(num) != num || num == 0)
  {
    bool ret = m_def->get_labeled_frame(fspecStr, frameno);
    //log_debug("get_labeled_frame(%s) returned %d, frameno is %d", fspecStr, ret, frameno);
    return ret;
  }

  if ( num < 0 ) return false;

  // all frame numbers > 0 are valid, but a valid frame number may still
  // reference a non-exist frame(eg. frameno > total_frames).
  frameno = size_t(num) - 1;

  return true;
}

/// Execute the actions for the specified frame. 
//
/// The frame_spec could be an integer or a string.
///
void sprite_instance::call_frame_actions(const as_value& frame_spec)
{
  size_t frame_number;
  if ( ! get_frame_number(frame_spec, frame_number) )
  {
    // No dice.
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("call_frame('%s') -- invalid frame"),
          frame_spec.to_debug_string());
    );
    return;
  }

#if 0 // why would we want to do this ?
  // Set the current sound_stream_id to -1, meaning that no stream are
  // active. If there are an active stream it will be updated while
  // executing the ControlTags.
  set_sound_stream_id(-1);
#endif

  // Execute the ControlTag actions
  // We set _callingFrameActions to true so that add_action_buffer
  // will execute immediately instead of queuing them.
  // NOTE: in case gotoFrame is executed by code in the called frame
  //       we'll temporarly clear the _callingFrameActions flag
  //       to properly queue actions back on the global queue.
  //
  _callingFrameActions=true;
  const PlayList* playlist = m_def->getPlaylist(frame_number);
  if ( playlist )
  {
    std::for_each(playlist->begin(), playlist->end(),
      boost::bind(&ControlTag::execute_action, _1, this)); 
  }
  _callingFrameActions=false;

}

character* sprite_instance::add_empty_movieclip(const char* name, int depth)
{
  cxform color_transform;
  matrix matrix;

  // empty_sprite_def will be deleted during deliting sprite
  sprite_definition* empty_sprite_def = new sprite_definition(get_movie_definition(), NULL);

  sprite_instance* sprite = new sprite_instance(empty_sprite_def, m_root, this, 0);
  sprite->set_name(name);
  sprite->setDynamic();

  // TODO: only call set_invalidated if this character actually overrides
  //       an existing one !
  set_invalidated(); 

  m_display_list.place_character(
    sprite,
    depth,
    color_transform,
    matrix,
    0,
    character::noClipDepthValue); 

  return sprite;
}

boost::intrusive_ptr<character>
sprite_instance::add_textfield(const std::string& name, int depth, float x, float y, float width, float height)
{
  matrix txt_matrix;

  // Create a definition (TODO: cleanup this thing, definitions should be immutable!)
  boost::intrusive_ptr<edit_text_character_def> txt = new edit_text_character_def(get_movie_definition());

  // Set textfield bounds
  txt->set_bounds(rect(0, 0, PIXELS_TO_TWIPS(width), PIXELS_TO_TWIPS(height)));

  // Set font height (shouldn't be dependent on font ?)
  // TODO: 10 pixels is an arbitrary number here...
  txt->set_font_height(10*20);


  // Create an instance
  boost::intrusive_ptr<character> txt_char = txt->create_character_instance(this, 0);

  // Give name and mark as dynamic
  txt_char->set_name(name);
  txt_char->setDynamic();

  // Set _x and _y
  txt_matrix.set_translation(
      infinite_to_fzero(PIXELS_TO_TWIPS(x)),
      infinite_to_fzero(PIXELS_TO_TWIPS(y)));

  // Here we add the character to the displayList.
  m_display_list.place_character(
    txt_char.get(),
    depth,
    cxform(),
    txt_matrix,
    0,
    character::noClipDepthValue);

  return txt_char;
}

boost::intrusive_ptr<sprite_instance> 
sprite_instance::duplicateMovieClip(const std::string& newname, int depth,
    as_object* initObject)
{
  character* parent_ch = get_parent();
  if ( ! parent_ch )
  {
    log_error(_("Can't clone root of the movie"));
    return NULL;
  }
  sprite_instance* parent = parent_ch->to_movie();
  if ( ! parent )
  {
    log_error(_("%s parent is not a sprite, can't clone"), getTarget());
    return NULL;
  }

  boost::intrusive_ptr<sprite_instance> newsprite = new sprite_instance(m_def.get(),
      m_root, parent, get_id());
  newsprite->set_name(newname);

  newsprite->setDynamic();

  if ( initObject ) newsprite->copyProperties(*initObject);
  //else newsprite->copyProperties(*this);

  // Copy event handlers from sprite
  // We should not copy 'm_action_buffer' since the 'm_method' already contains it
  newsprite->set_event_handlers(get_event_handlers());

  // Copy drawable
  newsprite->_drawable = new DynamicShape(*_drawable);

  parent->m_display_list.place_character(
    newsprite.get(),
    depth,
    get_cxform(),
    get_matrix(),
    get_ratio(),
    get_clip_depth());
  
  return newsprite;
}

/* public */
void
sprite_instance::queueAction(const action_buffer& action)
{
  movie_root& root = _vm.getRoot();
  root.pushAction(action, boost::intrusive_ptr<sprite_instance>(this));
}

/* private */
void
sprite_instance::queueActions(ActionList& actions)
{
  for(ActionList::const_iterator it=actions.begin(), itEnd=actions.end();
           it != itEnd; ++it)
  {
    const action_buffer* buf = *it;
    queueAction(*buf);
  }
}

bool
sprite_instance::on_event(const event_id& id)
{
  testInvariant();

#ifdef GNASH_DEBUG
  log_debug("Event %s invoked for sprite %s", id.get_function_name(), getTarget());
#endif

  // We do not execute ENTER_FRAME if unloaded
  if ( id.m_id == event_id::ENTER_FRAME && isUnloaded() )
  {
#ifdef GNASH_DEBUG
    log_debug("Sprite %s ignored ENTER_FRAME event (is unloaded)", getTarget());
#endif
    return false;
  }

  if ( id.is_button_event() && ! isEnabled() )
  {
#ifdef GNASH_DEBUG
    log_debug("Sprite %s ignored button-like event %s as not 'enabled'",
      getTarget(), id.get_function_name());
#endif
    return false;
  }

  bool called = false;
      
  // First, check for clip event handler.
  {
    std::auto_ptr<ExecutableCode> code ( get_event_handler(id) );
    if ( code.get() )
    {
      // Dispatch.
      code->execute();

      called = true;
    }
  }

  // Fall through and call the function also, if it's defined!


  // user-defined onInitialize is never called
  if ( id.m_id == event_id::INITIALIZE )
  {
      testInvariant();
      return called;
  }


  // NOTE: user-defined onLoad is not invoked for static
  //       clips on which no clip-events are defined.
  //       see testsuite/misc-ming.all/action_execution_order_extend_test.swf
  //
  //   Note that this can't be true for sprites
  //   not placed by PlaceObject, see
  //   testsuite/misc-ming.all/registerClassTest.swf
  //
  //   Note that this is also not true for sprites which have
  //   a registered class on them, see
  //   testsuite/misc-ming.all/registerClassTest2.swf
  //
  //   TODO: test the case in which it's MovieClip.prototype.onLoad defined !
  //
  if ( id.m_id == event_id::LOAD )
  {
    // TODO: we're likely making too much noise for nothing here,
    // there must be some action-execution-order related problem instead....
    // See testsuite/misc-ming.all/registerClassTest2.swf for an onLoad execution
    // order related problem ...
    do
    {
        if ( ! get_parent() ) break; // we don't skip calling user-defined onLoad for top-level movies
        if ( ! get_event_handlers().empty() ) break; // nor if there are clip-defined handler
        if ( isDynamic() ) break; // nor if it's dynamic
        sprite_definition* def = dynamic_cast<sprite_definition*>(m_def.get());
        if ( ! def ) break; // must be a loaded movie (loadMovie doesn't mark it as "dynamic" - should it? no, or getBytesLoaded will always return 0)
        if ( def->getRegisteredClass() ) break; // if it has a registered class it can have an onLoad in prototype...

#ifdef GNASH_DEBUG
        log_debug("Sprite %s (depth %d) won't check for user-defined LOAD event (is not dynamic, has a parent, "
		"no registered class and no clip events defined)",
		getTarget(), get_depth());
        testInvariant();
#endif
        return called;
    } while (0);
      
  }

  // Check for member function.
  if( ! id.is_key_event ())
  {
    boost::intrusive_ptr<as_function> method = 
      getUserDefinedEventHandler(id.get_function_key());

    if ( method )
    {
      call_method0(as_value(method.get()), &m_as_environment, this);
      called = true;
    }
  }

  // TODO: if this was UNLOAD release as much memory as possible ?
  //       Verify if this is possible, in particular check order in
  //       which unload handlers of parent and childs is performed
  //       and wheter unload of child can access members of parent.

  testInvariant();

  return called;
}

as_object*
sprite_instance::get_path_element(string_table::key key)
{
  //log_debug("%s.get_path_element(%s) called", getTarget(), _vm.getStringTable().value(key));
  as_object* obj = get_path_element_character(key);
  if ( obj )
  {
    return obj;
  }

  string name = _vm.getStringTable().value(key);

  // See if we have a match on the display list.
  character* ch;
  if ( _vm.getSWFVersion() >= 7 ) ch =  m_display_list.get_character_by_name(name);
  else ch = m_display_list.get_character_by_name_i(name);

      // TODO: should we check for isActionScriptReferenceable here ?
  if ( ch )
  {
    // If the object is an ActionScript referenciable one we
    // return it, otherwise we return ourselves
    if ( ch->isActionScriptReferenceable() ) return ch;
    else return this;
  }

  // See if it's a member

  // NOTE: direct use of get_member_default avoids
  //       triggering a call to sprite_instance::get_member
  //       which would scan the child characters again
  //       w/out a need for it
  //return as_object::get_path_element(key);

  as_value tmp;
  if ( ! get_member_default(key, &tmp, 0) )
  {
    return NULL;
  }
  if ( ! tmp.is_object() )
  {
    return NULL;
  }
  if ( tmp.is_sprite() )
  {
    return tmp.to_sprite(true);
  }

  return tmp.to_object().get();
}

void sprite_instance::set_member(string_table::key name,
    const as_value& val, string_table::key nsname)
{
#ifdef DEBUG_DYNTEXT_VARIABLES
  //log_debug(_("sprite[%p]::set_member(%s, %s)"), (void*)this, VM::get().getStringTable().value(name), val.to_debug_string());
#endif

  //if ( val.is_function() )
  //{
  //  checkForKeyOrMouseEvent(VM::get().getStringTable().value(name));
  //}

  // Try textfield variables
  //
  // FIXME: Turn textfield variables into Getter/Setters (Properties)
  //        so that set_member_default will do this automatically.
  //        The problem is that setting a TextVariable named after
  //        a builtin property will prevent *any* setting for the
  //        property (ie: have a textfield use _x as variable name and
  //        be scared)
  //
  TextFieldPtrVect* etc = get_textfield_variable(VM::get().getStringTable().value(name));
  if ( etc )
  {
#ifdef DEBUG_DYNTEXT_VARIABLES
    log_debug(_("it's a Text Variable, associated with %d TextFields"), etc->size());
#endif
    for (TextFieldPtrVect::iterator i=etc->begin(), e=etc->end(); i!=e; ++i)
    {
      TextFieldPtr tf = *i;
      tf->updateText(val.to_string());
    }
  }
#ifdef DEBUG_DYNTEXT_VARIABLES
  else
  {
    log_debug(_("it's NOT a Text Variable"));
  }
#endif

  // If that didn't work call the default set_member
  set_member_default(name, val, nsname);

}

void sprite_instance::advance_sprite()
{
  //GNASH_REPORT_FUNCTION;

  assert(!isUnloaded());
  assert(!_callingFrameActions); // call_frame shoudl never trigger advance_sprite

  // We might have loaded NO frames !
  if ( get_loaded_frames() == 0 )
  {
    IF_VERBOSE_MALFORMED_SWF(
    ONCE( log_swferror(_("advance_sprite: no frames loaded for sprite/movie %s"), getTarget()) );
    );
    return;
  }


  // Process any pending loadVariables request
  processCompletedLoadVariableRequests();

#ifdef GNASH_DEBUG
  size_t frame_count = m_def->get_frame_count();

  log_debug(_("Advance_sprite for sprite '%s' - frame %u/%u "),
    getTarget(), m_current_frame,
    frame_count);
#endif

  // I'm not sure ENTERFRAME goes in a different queue then DOACTION...
  queueEvent(event_id::ENTER_FRAME, movie_root::apDOACTION);
  //queueEvent(event_id::ENTER_FRAME, movie_root::apENTERFRAME);

  // Update current and next frames.
  if (m_play_state == PLAY)
  {
#ifdef GNASH_DEBUG
    log_debug(_("sprite_instance::advance_sprite we're in PLAY mode"));
#endif

    int prev_frame = m_current_frame;

#ifdef GNASH_DEBUG
    log_debug(_("on_event_load called, incrementing"));
#endif
    increment_frame_and_check_for_loop();
#ifdef GNASH_DEBUG
    log_debug(_("after increment we are at frame %u/%u"), m_current_frame, frame_count);
#endif

    // Execute the current frame's tags.
    // First time execute_frame_tags(0) executed in dlist.cpp(child) or movie_def_impl(root)
    if (m_current_frame != (size_t)prev_frame)
    {
      if ( m_current_frame == 0 && has_looped() )
      {
#ifdef GNASH_DEBUG
        log_debug("Jumping back to frame 0 of sprite %s", getTarget());
#endif
        restoreDisplayList(0); // seems OK to me.
      }
      else
      {
#ifdef GNASH_DEBUG
        log_debug("Executing frame%d (0-based) tags of sprite %s", m_current_frame, getTarget());
#endif
        // Make sure m_current_frame is 0-based during execution of DLIST tags
        execute_frame_tags(m_current_frame, TAG_DLIST|TAG_ACTION);
      }
    }
  }
#ifdef GNASH_DEBUG
  else
  {
    log_debug(_("sprite_instance::advance_sprite we're in STOP mode"));
    // shouldn't we execute frame tags anyway when in STOP mode ?
    //execute_frame_tags(m_current_frame);
  }
#endif
}

// child movieclip advance
void sprite_instance::advance()
{
//  GNASH_REPORT_FUNCTION;

#ifdef GNASH_DEBUG
  log_debug(_("Advance sprite '%s' at frame %u/%u"),
    getTargetPath(), m_current_frame,
    get_frame_count());
#endif

  // child movieclip frame rate is the same the root movieclip frame rate
  // that's why it is not needed to analyze 'm_time_remainder'

  advance_sprite();

}

void
sprite_instance::execute_init_action_buffer(const action_buffer& a, int cid)
{
  movie_instance* mi = m_root; // get_root(); // WARNING! get_root() would depend on _lockroot !!
  if ( mi->setCharacterInitialized(cid) )
  {
#ifdef GNASH_DEBUG
    log_debug("Queuing init actions in frame %d of sprite %s", m_current_frame, getTarget());
#endif
    std::auto_ptr<ExecutableCode> code ( new GlobalCode(a, boost::intrusive_ptr<sprite_instance>(this)) );

    // NOTE: we should really push these actions, but I still don't understand
    //       why doing so breaks youtube :/
    //       Undefining the YOUTUBE_TAKES_PRECEDENCE you'll get many XPASS
    //       in our testsuite, and no failures, but youtube would break.
    //
//#define YOUTUBE_TAKES_PRECEDENCE 1
#ifdef YOUTUBE_TAKES_PRECEDENCE
    code->execute();
#else
    movie_root& root = _vm.getRoot();
    root.pushAction(code, movie_root::apINIT);
#endif
  }
  else
  {
#ifdef GNASH_DEBUG
    log_debug("Init actions for character %d already executed", cid);
#endif
  }
}

void
sprite_instance::execute_action(const action_buffer& ab)
{
  as_environment& env = m_as_environment; // just type less

  ActionExec exec(ab, env);
  exec();
}

/*private*/
void
sprite_instance::restoreDisplayList(size_t tgtFrame)
{
  // This is not tested as usable for jump-forwards (yet)...
  // TODO: I guess just moving here the code currently in goto_frame
  //       for jump-forwards would do
  assert(tgtFrame <= m_current_frame);

  // Just invalidate this character before jumping back.
  // Should be optimized, but the invalidating model is not clear enough,
  // and there are some old questions spreading the source files.
  set_invalidated();

  is_jumping_back = true; //remember we are jumping back

  for (size_t f = 0; f<tgtFrame; ++f)
  {
    m_current_frame = f;
    execute_frame_tags(f, TAG_DLIST);
  }

  // Execute both action tags and DLIST tags of the target frame
  m_current_frame = tgtFrame;
  execute_frame_tags(tgtFrame, TAG_DLIST|TAG_ACTION);

  is_jumping_back = false; // finished jumping back

  m_display_list.mergeDisplayList(m_tmp_display_list);
}

// 0-based frame number !
void
sprite_instance::execute_frame_tags(size_t frame, int typeflags)
{
  testInvariant();

  //assert(frame < get_loaded_frames());

  assert(typeflags);

  const PlayList* playlist = m_def->getPlaylist(frame);
  if ( playlist )
  {
    IF_VERBOSE_ACTION(
      // Use 1-based frame numbers
      log_action(_("Executing %d tags in frame %d/%d of sprite %s"),
        playlist->size(), frame + 1, get_frame_count(),
        getTargetPath());
    );

    if ( (typeflags&TAG_DLIST) && (typeflags&TAG_ACTION) )
    {
      std::for_each( playlist->begin(), playlist->end(), boost::bind(&ControlTag::execute, _1, this) );
    }
    else if ( typeflags & TAG_DLIST )
    {
      assert( ! (typeflags & TAG_ACTION) );
      std::for_each( playlist->begin(), playlist->end(), boost::bind(&ControlTag::execute_state, _1, this) );
    }
    else
    {
      assert(typeflags & TAG_ACTION);
      std::for_each(playlist->begin(), playlist->end(), boost::bind(&ControlTag::execute_action, _1, this));
    }
  }

  testInvariant();
}

void
sprite_instance::goto_frame(size_t target_frame_number)
{
#if defined(DEBUG_GOTOFRAME) || defined(GNASH_DEBUG_TIMELINE)
  log_debug(_("sprite %s ::goto_frame(%d) - current frame is %d"),
    getTargetPath(), target_frame_number, m_current_frame);
#endif

  // goto_frame stops by default.
  // ActionGotoFrame tells the movieClip to go to the target frame 
  // and stop at that frame. 
  set_play_state(STOP);

  if ( target_frame_number > m_def->get_frame_count() - 1)
  {
    target_frame_number = m_def->get_frame_count() - 1;

    if ( ! m_def->ensure_frame_loaded(target_frame_number+1) )
    {
      log_error("Target frame of a gotoFrame(%d) was never loaded, although frame count in header (%d) said we would have found it",
        target_frame_number+1, m_def->get_frame_count());
      return; // ... I guess, or not ?
    }

    // Just set _currentframe and return.
    m_current_frame = target_frame_number;

    // don't push actions, already tested.
    return;
  }

  if(target_frame_number == m_current_frame)
  {
    // don't push actions
    return;
  }

  // Unless the target frame is the next one, stop playback of soundstream
  if (target_frame_number != m_current_frame+1 )
  {
	stopStreamSound();
  }

  size_t loaded_frames = get_loaded_frames();
  // target_frame_number is 0-based, get_loaded_frames() is 1-based
  // so in order to goto_frame(3) loaded_frames must be at least 4
  // if goto_frame(4) is called, and loaded_frames is 4 we're jumping
  // forward
  if ( target_frame_number >= loaded_frames )
  {
    IF_VERBOSE_ASCODING_ERRORS(
      log_aserror(_("GotoFrame(%d) targets a yet "
      "to be loaded frame (%d) loaded). "
      "We'll wait for it but a more correct form "
      "is explicitly using WaitForFrame instead"),
      target_frame_number+1,
      loaded_frames);

    );
    if ( ! m_def->ensure_frame_loaded(target_frame_number+1) )
    {
      log_error("Target frame of a gotoFrame(%d) was never loaded, although frame count in header (%d) said we would have found it",
        target_frame_number+1, m_def->get_frame_count());
      return; // ... I guess, or not ?
    }
  }


  //
  // Construct the DisplayList of the target frame
  //

  if (target_frame_number < m_current_frame)
  // Go backward to a previous frame
  {
    // NOTE: just in case we're being called by code in a called frame
    //       we'll backup and resume the _callingFrameActions flag
    bool callingFrameActionsBackup = _callingFrameActions;
    _callingFrameActions = false;
    // restoreDisplayList takes care of properly setting the m_current_frame variable
    restoreDisplayList(target_frame_number);
    assert(m_current_frame == target_frame_number);
    _callingFrameActions = callingFrameActionsBackup;
  }
  else
  // Go forward to a later frame
  {
    // We'd immediately return if target_frame_number == m_current_frame
    assert(target_frame_number > m_current_frame);
    while (++m_current_frame < target_frame_number)
    {
      //for (size_t f = m_current_frame+1; f<target_frame_number; ++f) 
      // Second argument requests that only "DisplayList" tags
      // are executed. This means NO actions will be
      // pushed on m_action_list.
      execute_frame_tags(m_current_frame, TAG_DLIST);
    }
    assert(m_current_frame == target_frame_number);


    // Now execute target frame tags (queuing actions)
    // NOTE: just in case we're being called by code in a called frame
    //       we'll backup and resume the _callingFrameActions flag
    bool callingFrameActionsBackup = _callingFrameActions;
    _callingFrameActions = false;
    execute_frame_tags(target_frame_number, TAG_DLIST|TAG_ACTION);
    _callingFrameActions = callingFrameActionsBackup;
  }

  assert(m_current_frame == target_frame_number);
}

bool sprite_instance::goto_labeled_frame(const std::string& label)
{
  size_t target_frame;
  if (m_def->get_labeled_frame(label, target_frame))
  {
    goto_frame(target_frame);
    return true;
  }

    IF_VERBOSE_MALFORMED_SWF(
    log_swferror(_("sprite_instance::goto_labeled_frame('%s') "
      "unknown label"), label);
    );
    return false;
}

void sprite_instance::display()
{
  //GNASH_REPORT_FUNCTION;

  // Note: 
  // DisplayList::Display() will take care of the visibility checking.
  //
  // Whether a character should be rendered or not is dependent on its parent.
  // i.e. if its parent is a mask, this character should be rendered to the mask
  // buffer even it is invisible.
  //

  
  // render drawable (ActionScript generated graphics)
  
  _drawable->finalize();
  // TODO: I'd like to draw the definition directly..
  //       but it seems that the backend insists in
  //       accessing the *parent* of the character
  //       passed as "instance" for the drawing.
  //       When displaying top-level movie this will
  //       be NULL and gnash will segfault
  //       Thus, this drawable_instance is basically just
  //       a container for a parent :(
  _drawable_inst->display();
  
  
  // descend the display list
  m_display_list.display();
    
   
  clear_invalidated();
    
}

bool
sprite_instance::attachCharacter(character& newch, int depth)
{

  // place_character() will set depth on newch
  m_display_list.place_character(
    &newch,
    depth,
    cxform(),
    matrix(),
    65535,
    character::noClipDepthValue);

  return true; // FIXME: check return from place_character above ?
}

character*
sprite_instance::add_display_object(
    boost::uint16_t character_id,
    const std::string* name,
    const std::vector<swf_event*>& event_handlers,
    int depth, 
    const cxform& color_transform, const matrix& mat,
    int ratio, int clip_depth)
{
//GNASH_REPORT_FUNCTION;
    assert(m_def != NULL);

    character_def*  cdef = m_def->get_character_def(character_id);
    if (cdef == NULL)
    {
        IF_VERBOSE_MALFORMED_SWF(
            log_swferror(_("sprite_instance::add_display_object(): "
                "unknown cid = %d"), character_id);
        );
        return NULL;
    }
  
  DisplayList& dlist = const_cast<DisplayList &>( getDisplayList() );
    character* existing_char = dlist.get_character_at_depth(depth);
    
    if(existing_char)
    {
    return NULL;
    }
  else
  {
    boost::intrusive_ptr<character> ch = cdef->create_character_instance(this, character_id);
    
    if(name)
        {
            ch->set_name(*name);
        }
    else if(ch->wantsInstanceName())
        {
            std::string instance_name = getNextUnnamedInstanceName();
            ch->set_name(instance_name);
        }
      
    // Attach event handlers (if any).
        for (size_t i = 0, n = event_handlers.size(); i < n; i++)
        {
            swf_event* ev = event_handlers[i];
            ch->add_event_handler(ev->event(), ev->action());
        }

    dlist.place_character(
            ch.get(),
            depth,
            color_transform,
            mat,
            ratio,
            clip_depth);

        return ch.get();
  }
}

void
sprite_instance::replace_display_object(
        boost::uint16_t character_id,
        const std::string* name,
        int depth,
        const cxform* color_transform,
        const matrix* mat,
        int ratio,
        int clip_depth)
{
    assert(m_def != NULL);

    character_def*  cdef = m_def->get_character_def(character_id);
    if (cdef == NULL)
    {
        log_error(_("sprite::replace_display_object(): "
            "unknown cid = %d"), character_id);
        return;
    }
    assert(cdef);

    DisplayList& dlist = const_cast<DisplayList &>( getDisplayList() );
    character* existing_char = dlist.get_character_at_depth(depth);

    if (existing_char)
    {
        // if the existing character is not a shape, move it instead of replace
        if ( existing_char->isActionScriptReferenceable() )
        {
            move_display_object(depth, color_transform, mat, ratio, clip_depth);
            return;
        }
        else
        {
            boost::intrusive_ptr<character> ch = cdef->create_character_instance(this, character_id);
   
            replace_display_object(
                ch.get(), 
        	name, 
        	depth,
                color_transform,
                mat,
                ratio, 
        clip_depth);
        }
    }
    else // non-existing character
    {
    log_error("sprite_instance::replace_display_object: could not find any character at depth %d", depth);
    } 
}

void sprite_instance::replace_display_object(
        character* ch,
        const std::string* name,
        int depth,
        const cxform* color_transform,
        const matrix* mat,
        int ratio,
        int clip_depth)
{
    //printf("%s: character %s, id is %d\n", __FUNCTION__, name, ch->get_id()); // FIXME:

    assert(ch != NULL);

    if (name)
    {
	ch->set_name(*name);
    }
    else if(ch->wantsInstanceName())
    {
	std::string instance_name = getNextUnnamedInstanceName();
	ch->set_name(instance_name);
    }

  DisplayList& dlist = const_cast<DisplayList &>( getDisplayList() );

    dlist.replace_character(
    ch,
    depth,
    color_transform,
    mat,
    ratio,
    clip_depth);
    
}

int sprite_instance::get_id_at_depth(int depth)
{
    character*  ch = m_display_list.get_character_at_depth(depth);
    if ( ! ch ) return -1;
    else return ch->get_id();
}

void sprite_instance::increment_frame_and_check_for_loop()
{
  //GNASH_REPORT_FUNCTION;

  //size_t frame_count = m_def->get_frame_count();
  size_t frame_count = get_loaded_frames(); 
  if ( ++m_current_frame >= frame_count )
  {
    // Loop.
    m_current_frame = 0;
    m_has_looped = true;
    if (frame_count > 1)
    {
      //m_display_list.reset();
    }
  }

#if 0 // debugging
  log_debug(_("Frame %u/%u, bytes %u/%u"),
    m_current_frame, frame_count,
    get_bytes_loaded(), get_bytes_total());
#endif
}

/// Find a character hit by the given coordinates.
//
/// This class takes care about taking masks layers into
/// account, but nested masks aren't properly tested yet.
///
class MouseEntityFinder {

  /// Highest depth hidden by a mask
  //
  /// This will be -1 initially, and set
  /// the the depth of a mask when the mask
  /// doesn't contain the query point, while
  /// scanning a DisplayList bottom-up
  ///
  int _highestHiddenDepth;

  character* _m;

  typedef std::vector<character*> Candidates;
  Candidates _candidates;

  /// Query point in world coordinate space
  point _wp;

  /// Query point in parent coordinate space
  point _pp;

  bool _checked;

public:

  /// @param wp
  ///   Query point in world coordinate space
  ///
  /// @param pp
  ///   Query point in parent coordinate space
  ///
  MouseEntityFinder(point wp, point pp)
    :
    _highestHiddenDepth(std::numeric_limits<int>::min()),
    _m(NULL),
    _candidates(),
    _wp(wp),
    _pp(pp),
    _checked(false)
  {}

  void operator() (character* ch)
  {
    assert(!_checked);
    if ( ch->get_depth() <= _highestHiddenDepth )
    {
      if ( ch->isMaskLayer() )
      {
        log_debug("CHECKME: nested mask in MouseEntityFinder. This mask is %s at depth %d outer mask masked up to depth %d.",
          ch->getTarget(), ch->get_depth(), _highestHiddenDepth);
        // Hiding mask still in effect...
      }
      return;
    }

    if ( ch->isMaskLayer() )
    {
      //if ( ! ch->get_visible() ) log_debug("invisible mask in MouseEntityFinder.");
      if ( ! ch->pointInShape(_wp.x, _wp.y) )
      {
#ifdef DEBUG_MOUSE_ENTITY_FINDING
        log_debug("Character %s at depth %d is a mask not hitting the query point %g,%g and masking up to depth %d",
          ch->getTarget(), ch->get_depth(), _wp.x, _wp.y, ch->get_clip_depth());
#endif // DEBUG_MOUSE_ENTITY_FINDING
        _highestHiddenDepth = ch->get_clip_depth();
      }
      else
      {
#ifdef DEBUG_MOUSE_ENTITY_FINDING
        log_debug("Character %s at depth %d is a mask hitting the query point %g,%g",
          ch->getTarget(), ch->get_depth(), _wp.x, _wp.y);
#endif // DEBUG_MOUSE_ENTITY_FINDING
      }

      return;
    }

    if ( ! ch->get_visible() ) return;

    _candidates.push_back(ch);

  }

  void checkCandidates()
  {
    if ( _checked ) return;
    for (Candidates::reverse_iterator i=_candidates.rbegin(),
            e=_candidates.rend(); i!=e; ++i)
    {
      character* ch = *i;
      character* te = ch->get_topmost_mouse_entity(_pp.x, _pp.y);
      if ( te )
      {
        _m = te;
        break;
      }
    }
    _checked = true;
  }

  character* getEntity()
  {
    checkCandidates();
#ifdef DEBUG_MOUSE_ENTITY_FINDING
    if ( _m ) 
    {
      log_debug("MouseEntityFinder found character %s (depth %d) hitting point %g,%g",
        _m->getTarget(), _m->get_depth(), _wp.x, _wp.y);
    }
#endif // DEBUG_MOUSE_ENTITY_FINDING
    return _m;
  }
    
};

/// Find the first character whose shape contain the point
//
/// Point coordinates in world TWIPS
///
class ShapeContainerFinder {

  bool _found;
  float _x;
  float _y;

public:

  ShapeContainerFinder(float x, float y)
    :
    _found(false),
    _x(x),
    _y(y)
  {}

  bool operator() (character* ch)
  {
    if ( ch->pointInShape(_x, _y) )
    {
      _found = true;
      return false;
    }
    else return true;
  }

  bool hitFound() { return _found; }
    
};

/// Find the first visible character whose shape contain the point
//
/// Point coordinates in world TWIPS
///
class VisibleShapeContainerFinder {

  bool _found;
  float _x;
  float _y;

public:

  VisibleShapeContainerFinder(float x, float y)
    :
    _found(false),
    _x(x),
    _y(y)
  {}

  bool operator() (character* ch)
  {
    if ( ch->pointInVisibleShape(_x, _y) )
    {
      _found = true;
      return false;
    }
    else return true;
  }

  bool hitFound() { return _found; }
    
};

bool
sprite_instance::pointInShape(float x, float y) const
{
  ShapeContainerFinder finder(x, y);
  const_cast<DisplayList&>(m_display_list).visitBackward(finder);
  if ( finder.hitFound() ) return true;
  return _drawable_inst->pointInShape(x, y); 
}

bool
sprite_instance::pointInVisibleShape(float x, float y) const
{
  if ( ! get_visible() ) return false;
  if ( isDynamicMask() && ! can_handle_mouse_event() )
  {
      // see testsuite/misc-ming.all/masks_test.swf
#ifdef GNASH_DEBUG_HITTEST
      log_debug("%s is a dynamic mask and can't handle mouse events, no point will hit it", getTarget());
#endif
      return false;
  }
  character* mask = getMask(); // dynamic one
  if ( mask && mask->get_visible() && ! mask->pointInShape(x, y) )
  {
#ifdef GNASH_DEBUG_HITTEST
    log_debug("%s is dynamically masked by %s, which doesn't hit point %g,%g", getTarget(), mask->getTarget(), x, y);
#endif
    return false;
  }
  VisibleShapeContainerFinder finder(x, y);
  const_cast<DisplayList&>(m_display_list).visitBackward(finder);
  if ( finder.hitFound() ) return true;
  return _drawable_inst->pointInVisibleShape(x, y); 
}

character*
sprite_instance::get_topmost_mouse_entity(float x, float y)
{
  //GNASH_REPORT_FUNCTION;

  if (get_visible() == false)
  {
    return NULL;
  }

  // point is in parent's space,
  // we need to convert it in world space
  point wp(x,y);
  character* parent = get_parent();
  if ( parent ) parent->get_world_matrix().transform(wp);
  // WARNING: if we have NO parent, our parent it the Stage (movie_root)
  //          so, in case we'll add a "stage" matrix, we'll need to take
  //          it into account here.
  // TODO: actually, why are we insisting in using parent's coordinates for
  //       this method at all ?
  //

  if ( can_handle_mouse_event() )
  {
    if ( pointInVisibleShape(wp.x, wp.y) ) return this;
    else return NULL;
  }


  matrix  m = get_matrix();
  point pp;
  m.transform_by_inverse(&pp, point(x, y));

  MouseEntityFinder finder(wp, pp);
  //m_display_list.visitBackward(finder);
  m_display_list.visitAll(finder);
  character* ch = finder.getEntity();
  if ( ! ch ) 
  {
    ch = _drawable_inst->get_topmost_mouse_entity(pp.x, pp.y);
  }

  return ch; // might be NULL
}

/// Find the first visible character whose shape contain the point
/// and is not the character being dragged or any of its childs
//
/// Point coordinates in world TWIPS
///
class DropTargetFinder {

  /// Highest depth hidden by a mask
  //
  /// This will be -1 initially, and set
  /// the the depth of a mask when the mask
  /// doesn't contain the query point, while
  /// scanning a DisplayList bottom-up
  ///
  int _highestHiddenDepth;

  float _x;
  float _y;
  character* _dragging;
  mutable const character* _dropch;

  typedef std::vector<const character*> Candidates;
  Candidates _candidates;

  mutable bool _checked;

public:

  DropTargetFinder(float x, float y, character* dragging)
    :
    _highestHiddenDepth(std::numeric_limits<int>::min()),
    _x(x),
    _y(y),
    _dragging(dragging),
    _dropch(0),
		_candidates(),
		_checked(false)
  {}

  void operator() (const character* ch)
  {
		assert(!_checked);
    if ( ch->get_depth() <= _highestHiddenDepth )
    {
      if ( ch->isMaskLayer() )
      {
        log_debug("CHECKME: nested mask in DropTargetFinder. This mask is %s at depth %d outer mask masked up to depth %d.",
          ch->getTarget(), ch->get_depth(), _highestHiddenDepth);
        // Hiding mask still in effect...
      }
      return;
    }

    if ( ch->isMaskLayer() )
    {
      if ( ! ch->get_visible() )
      {
        log_debug("FIXME: invisible mask in MouseEntityFinder.");
      }
      if ( ! ch->pointInShape(_x, _y) )
      {
#ifdef DEBUG_MOUSE_ENTITY_FINDING
        log_debug("Character %s at depth %d is a mask not hitting the query point %g,%g and masking up to depth %d",
          ch->getTarget(), ch->get_depth(), _x, _y, ch->get_clip_depth());
#endif // DEBUG_MOUSE_ENTITY_FINDING
        _highestHiddenDepth = ch->get_clip_depth();
      }
      else
      {
#ifdef DEBUG_MOUSE_ENTITY_FINDING
        log_debug("Character %s at depth %d is a mask hitting the query point %g,%g",
          ch->getTarget(), ch->get_depth(), _x, _y);
#endif // DEBUG_MOUSE_ENTITY_FINDING
      }

      return;
    }

    _candidates.push_back(ch);

  }

  void checkCandidates() const
  {
    if ( _checked ) return;
    for (Candidates::const_reverse_iterator i=_candidates.rbegin(),
            e=_candidates.rend(); i!=e; ++i)
    {
      const character* ch = *i;
			const character* dropChar = ch->findDropTarget(_x, _y, _dragging);
			if ( dropChar )
			{
				_dropch = dropChar;
				break;
			}
    }
    _checked = true;
  }

  const character* getDropChar() const
	{
		checkCandidates();
		return _dropch;
	}
};

const character*
sprite_instance::findDropTarget(float x, float y, character* dragging) const
{
  //GNASH_REPORT_FUNCTION;

  if ( this == dragging ) return 0; // not here...

  if ( ! get_visible() ) return 0; // isn't me !

  DropTargetFinder finder(x, y, dragging);
  m_display_list.visitAll(finder);

  // does it hit any child ?
  const character* ch = finder.getDropChar();
  if ( ch )
  {
    // TODO: find closest actionscript referenceable container
    //       (possibly itself)
    return ch;
  }

  // does it hit us ?
  if ( _drawable_inst->pointInVisibleShape(x, y) )
  {
    return this;
  }

  return NULL;
}

bool
sprite_instance::can_handle_mouse_event() const
{
  if ( ! isEnabled() ) return false;

  // Event handlers that qualify as mouse event handlers.
  static const event_id EH[] =
  {
    event_id(event_id::PRESS),
    event_id(event_id::RELEASE),
    event_id(event_id::RELEASE_OUTSIDE),
    event_id(event_id::ROLL_OVER),
    event_id(event_id::ROLL_OUT),
    event_id(event_id::DRAG_OVER),
    event_id(event_id::DRAG_OUT),
  };

  for (unsigned int i = 0; i < ARRAYSIZE(EH); i++)
  {
    const event_id &event = EH[i];

    // Check event handlers
    if ( get_event_handler(event.id()).get() )
    {
      return true;
    }

    // Check user-defined event handlers
    if ( getUserDefinedEventHandler(event.get_function_key()) )
    {
      return true;
    }
  }

  return false;
}
    
void sprite_instance::restart()
{
// see Whack-a-doc.swf, we tried to restart an unloaded character.
// It shouldn't happen anyway.
// TODO: drop this function.

  // Stop any streaming sound associated with us
  stopStreamSound();

  if( ! isUnloaded() )
  {
    restoreDisplayList(0); 
  }

  m_play_state = PLAY;
}

character*
sprite_instance::get_character(int /* character_id */)
{
  //return m_def->get_character_def(character_id);
  // @@ TODO -- look through our dlist for a match
  log_unimpl(_("%s doesn't even check for a char"),
    __PRETTY_FUNCTION__);
  return NULL;
}

float
sprite_instance::get_pixel_scale() const
{
  return _vm.getRoot().get_pixel_scale();
}

void
sprite_instance::get_mouse_state(int& x, int& y, int& buttons)
{
  _vm.getRoot().get_mouse_state(x, y, buttons);
}

void
sprite_instance::stop_drag()
{
  //assert(m_parent == NULL); // why should we care ?
  _vm.getRoot().stop_drag();
}

float
sprite_instance::get_background_alpha() const
{
    // @@ this doesn't seem right...
    return _vm.getRoot().get_background_alpha();
}

void
sprite_instance::set_background_color(const rgba& color)
{
  _vm.getRoot().set_background_color(color);
}


/* public */
void
sprite_instance::set_textfield_variable(const std::string& name,
    edit_text_character* ch)
{
  assert(ch);

  // lazy allocation
  if ( ! _text_variables.get() )
  {
    _text_variables.reset(new TextFieldMap);
  }
  
  (*_text_variables)[name].push_back(ch);
}

/* private */
sprite_instance::TextFieldPtrVect*
sprite_instance::get_textfield_variable(const std::string& name)
{
  // nothing allocated yet...
  if ( ! _text_variables.get() ) return NULL;

  // TODO: should variable name be considered case-insensitive ?
  TextFieldMap::iterator it = _text_variables->find(name);
  if ( it == _text_variables->end() )
  {
    return 0;
  }
  else
  {
    return &(it->second);
  }
} 


void 
sprite_instance::add_invalidated_bounds(InvalidatedRanges& ranges, 
  bool force)
{

  // nothing to do if this sprite is not visible
  if (!m_visible || get_cxform().is_invisible() )
  {
    ranges.add(m_old_invalidated_ranges); // (in case we just hided)
    return;
  }

  if ( ! m_invalidated && ! m_child_invalidated && ! force )
  {
    return;
  }
  
 
  // m_child_invalidated does not require our own bounds
  if ( m_invalidated || force )      
  {
    // Add old invalidated bounds
    ranges.add(m_old_invalidated_ranges); 
  }
  
  
  m_display_list.add_invalidated_bounds(ranges, force||m_invalidated);

  _drawable_inst->add_invalidated_bounds(ranges, force||m_invalidated);

}

void 
sprite_instance::dump_character_tree(const std::string prefix) const
{
  character::dump_character_tree(prefix);
  m_display_list.dump_character_tree(prefix+" ");
}

const char*
sprite_instance::call_method_args(const char* method_name,
    const char* method_arg_fmt, va_list args)
{
    // Keep m_as_environment alive during any method calls!
    boost::intrusive_ptr<as_object> this_ptr(this);

    return call_method_parsed(&m_as_environment, this,
    method_name, method_arg_fmt, args);
}

/// register characters as key listeners if they have clip key events defined.
/// Don't call twice for the same chracter.
void
sprite_instance::registerAsListener()
{
    _vm.getRoot().add_key_listener(this);
    _vm.getRoot().add_mouse_listener(this);
}
  


// WARNING: THIS SNIPPET NEEDS THE CHARACTER TO BE "INSTANTIATED", which is
//          it's target path needs to exist, or any as_value for it will be
//          a dangling reference to an unexistent sprite !
//          NOTE: this is just due to the wrong steps, see comment in header
void
sprite_instance::stagePlacementCallback()
{
  assert(!isUnloaded());

  saveOriginalTarget();

#ifdef GNASH_DEBUG
  log_debug(_("Sprite '%s' placed on stage"), getTarget());
#endif

  // Register this sprite as a live one
  _vm.getRoot().addLiveChar(this);

  // Register this sprite as a core broadcasters listener
  registerAsListener();

  // It seems it's legal to place 0-framed sprites on stage.
  // See testsuite/misc-swfmill.all/zeroframe_definesprite.swf
  //m_def->ensure_frame_loaded(0);

#if 0
  // We might have loaded NO frames !
  bool hasFrames = get_loaded_frames();
  if ( ! hasFrames )
  {
    IF_VERBOSE_MALFORMED_SWF(
    ONCE( log_swferror(_("stagePlacementCallback: no frames loaded for sprite/movie %s"), getTarget()) );
    );
  }
#endif

  // We execute events immediately when the stage-placed character is dynamic.
  // This is becase we assume that this means that the character is placed during
  // processing of actions (opposed that during advancement iteration).
  //
  // A more general implementation might ask movie_root about it's state
  // (iterating or processing actions?)
  // Another possibility to inspect could be letting movie_root decide
  // when to really queue and when rather to execute immediately the 
  // events with priority INITIALIZE or CONSTRUCT ...
  //
  if ( isDynamic() )
  {
#ifdef GNASH_DEBUG
    log_debug("Sprite %s is dynamic, sending INITIALIZE and CONSTRUCT events immediately", getTarget());
#endif
    on_event(event_id::INITIALIZE);
    constructAsScriptObject(); 
  }
  else
  {
#ifdef GNASH_DEBUG
    log_debug("Queuing INITIALIZE event for sprite %s", getTarget());
#endif
    queueEvent(event_id::INITIALIZE, movie_root::apINIT);

#ifdef GNASH_DEBUG
    log_debug("Queuing CONSTRUCT event for sprite %s", getTarget());
#endif
    std::auto_ptr<ExecutableCode> code ( new ConstructEvent(this) );
    _vm.getRoot().pushAction(code, movie_root::apCONSTRUCT);
  }

  // Now execute frame tags and take care of queuing the LOAD event.
  //
  // DLIST tags are executed immediately while ACTION tags are queued.
  //
  // For _root movie, LOAD event is invoked *after* actions in first frame
  // See misc-ming.all/action_execution_order_test4.{c,swf}
  //
  assert(!_callingFrameActions); // or will not be queuing actions
  if ( get_parent() == 0 )
  {
#ifdef GNASH_DEBUG
      log_debug(_("Executing tags of frame0 in sprite %s"), getTarget());
#endif
      execute_frame_tags(0, TAG_DLIST|TAG_ACTION);

    if ( _vm.getSWFVersion() > 5 )
    {
#ifdef GNASH_DEBUG
      log_debug(_("Queuing ONLOAD event for sprite %s"), getTarget());
#endif
      queueEvent(event_id::LOAD, movie_root::apDOACTION);
    }

  }
  else
  {

#ifdef GNASH_DEBUG
    log_debug(_("Queuing ONLOAD event for sprite %s"), getTarget());
#endif
    queueEvent(event_id::LOAD, movie_root::apDOACTION);

#ifdef GNASH_DEBUG
      log_debug(_("Executing tags of frame0 in sprite %s"), getTarget());
#endif
      execute_frame_tags(0, TAG_DLIST|TAG_ACTION);
  }

}

/*private*/
void
sprite_instance::constructAsScriptObject()
{
#ifdef GNASH_DEBUG
  log_debug("constructAsScriptObject called for sprite %s", getTarget());
#endif

  bool eventHandlersInvoked = false;

  do {
    if ( _name.empty() )
    {
      // instance name will be needed for properly setting up
      // a reference to 'this' object for ActionScript actions.
      // If the instance doesn't have a name, it will NOT be
      // an ActionScript referenciable object so we don't have
      // anything more to do.
      break;
    }

    sprite_definition* def = dynamic_cast<sprite_definition*>(m_def.get());

    // We won't "construct" top-level movies
    if ( ! def ) break;

    as_function* ctor = def->getRegisteredClass();
#ifdef GNASH_DEBUG
    log_debug(_("Attached sprites %s registered class is %p"), getTarget(), (void*)ctor); 
#endif

    // TODO: builtin constructors are different from user-defined ones
    // we should likely change that. See also vm/ASHandlers.cpp (construct_object)
    if ( ctor && ! ctor->isBuiltin() )
    {
      // Set the new prototype *after* the constructor was called
      boost::intrusive_ptr<as_object> proto = ctor->getPrototype();
      set_prototype(proto);

      // Call event handlers *after* setting up the __proto__
      // but *before* calling the registered class constructor
      on_event(event_id::CONSTRUCT);
      eventHandlersInvoked = true;

      int swfversion = _vm.getSWFVersion();

      // Set the '__constructor__' and 'constructor' members, as well as call
      // the actual constructor.
      //
      // TODO: this would be best done by an as_function::constructInstance()
      //       method. We have one but it returns a new object rather then
      //       initializing a given object, we just need to add another one...
      //
      if ( swfversion > 5 )
      {
        //log_debug(_("Calling the user-defined constructor against this sprite_instance"));

	// Provide a 'super' reference..
	as_object* super = NULL;
	as_object* iface = ctor->getPrototype().get(); // this function's prototype
	if ( iface ) super = iface->get_super();

        fn_call call(this, &(get_environment()), 0, 0, super);

        // we don't use the constructor return (should we?)
        (*ctor)(call);


        set_member(NSV::PROP_uuCONSTRUCTORuu, ctor);
        if ( swfversion == 6 )
        {
          set_member(NSV::PROP_CONSTRUCTOR, ctor);
        }
      }
    }

  } while (0);

  /// Invoke event handlers if not done yet
  if ( ! eventHandlersInvoked )
  {
    on_event(event_id::CONSTRUCT);
  }
}

bool
sprite_instance::unload()
{
#ifdef GNASH_DEBUG
  log_debug(_("Unloading sprite '%s'"), getTargetPath());
#endif

  // stop any pending streaming sounds
  stopStreamSound();

  bool childHaveUnloadHandler = m_display_list.unload();

  // We won't be displayed again, so worth releasing
  // some memory. The drawable might take a lot of memory
  // on itself.
  _drawable->clear();
  // TODO: drop the _drawable_inst too ?
  //       it would require _drawable_inst to possibly be NULL,
  //       which wouldn't be bad at all actually...

  bool selfHaveUnloadHandler = character::unload();

  bool shouldKeepAlive =  ( selfHaveUnloadHandler || childHaveUnloadHandler );

  return shouldKeepAlive;
}

bool
sprite_instance::loadMovie(const URL& url, const std::string* postdata)
{
  // Get a pointer to our own parent 
  character* parent = get_parent();
  if ( parent )
  {
	if ( postdata ) log_debug("Posting data '%s' to url '%s'", postdata, url.str());
    boost::intrusive_ptr<movie_definition> md ( create_library_movie(url, NULL, true, postdata) );
    if (md == NULL)
    {
      log_error(_("can't create movie_definition for %s"),
        url.str());
      return false;
    }

    boost::intrusive_ptr<movie_instance> extern_movie;
    extern_movie = md->create_movie_instance(parent);
    if (extern_movie == NULL)
    {
      log_error(_("can't create extern movie_instance "
        "for %s"), url.str());
      return false;
    }

    // Parse query string
    VariableMap vars;
    url.parse_querystring(url.querystring(), vars);
    extern_movie->setVariables(vars);

    // Set lockroot to our value of it
    extern_movie->setLockRoot( getLockRoot() );

    // Copy event handlers
    // see testsuite/misc-ming.all/loadMovieTest.swf
    const Events& clipEvs = get_event_handlers();
    // top-level movies can't have clip events, right ?
    assert ( extern_movie->get_event_handlers().empty() );
    extern_movie->set_event_handlers(clipEvs);

    save_extern_movie(extern_movie.get());

    const std::string& name = get_name();
    int depth = get_depth();
    bool use_cxform = false;
    cxform color_transform = get_cxform();
    bool use_matrix = false;
    matrix mat = get_matrix();
    int ratio = get_ratio();
    int clip_depth = get_clip_depth();

    assert ( parent == extern_movie->get_parent() );

    sprite_instance* parent_sp = parent->to_movie();
    assert(parent_sp);
    parent_sp->replace_display_object(
           extern_movie.get(),
           name.empty() ? NULL : &name, // TODO: check empty != none...
           depth,
           use_cxform ? &color_transform : NULL,
           use_matrix ? &mat : NULL,
           ratio,
           clip_depth);
  }
  else
  {
    movie_root& root = _vm.getRoot();
    unsigned int level = get_depth()-character::staticDepthOffset;
    
#ifndef GNASH_USE_GC
    // Make sure we won't kill ourself !
    assert(get_ref_count() > 1);
#endif // ndef GNASH_USE_GC

    // how about lockRoot here ?
    root.loadLevel(level, url); // extern_movie.get());
  }

  return true;
}

void 
sprite_instance::loadVariables(URL url, short sendVarsMethod)
{
  // Check host security
  // will be done by LoadVariablesThread (down by getStream, that is)
  //if ( ! URLAccessManager::allow(url) ) return;
  
  std::string postdata;
  
  if ( sendVarsMethod )    // 1=GET, 2=POST
  {
	getURLEncodedVars(postdata);
  }

  try 
  {
    if ( sendVarsMethod == 2 )
    {
        // use POST method
        _loadVariableRequests.push_back(new LoadVariablesThread(url, postdata));
    }
    else
    {
        // use GET method
        if ( sendVarsMethod == 1 )
	{
		// Append variables
		string qs = url.querystring();
		if ( qs.empty() ) url.set_querystring(postdata);
		else url.set_querystring(qs + std::string("&") + postdata);
	}
    	_loadVariableRequests.push_back(new LoadVariablesThread(url));
    }
    _loadVariableRequests.back()->process();
  }
  catch (NetworkException& ex)
  {
    log_error(_("Could not load variables from %s"), url.str());
  }

  //log_debug(_(%d loadVariables requests pending"), _loadVariableRequests.size());

}

/*private*/
void
sprite_instance::processCompletedLoadVariableRequest(LoadVariablesThread& request)
{
  assert(request.completed());

  // TODO: consider adding a setVariables(std::map) for use by this
  //       and by Player class when dealing with -P command-line switch

  string_table& st = _vm.getStringTable();
  LoadVariablesThread::ValuesMap& vals = request.getValues();
  for (LoadVariablesThread::ValuesMap::const_iterator it=vals.begin(),
      itEnd=vals.end();
    it != itEnd; ++it)
  {
    const string name = PROPNAME(it->first);
    const string& val = it->second;
#ifdef DEBUG_LOAD_VARIABLES
    log_debug(_("Setting variable '%s' to value '%s'"), name, val);
#endif
    set_member(st.find(name), val);
  }

  // We want to call a clip-event too if available, see bug #22116
  on_event(event_id::DATA);
}

/*private*/
void
sprite_instance::processCompletedLoadVariableRequests()
{
  // Nothing to do (just for clarity)
  if ( _loadVariableRequests.empty() ) return;

  for (LoadVariablesThreads::iterator it=_loadVariableRequests.begin();
      it != _loadVariableRequests.end(); )
  {
    LoadVariablesThread& request = *(*it);
    if ( request.completed() )
    {
      processCompletedLoadVariableRequest(request);
      it = _loadVariableRequests.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void
sprite_instance::setVariables(VariableMap& vars)
{
  string_table& st = _vm.getStringTable();
  for (VariableMap::const_iterator it=vars.begin(), itEnd=vars.end();
    it != itEnd; ++it)
  {
    const string& name = it->first;
    const string& val = it->second;
    set_member(st.find(PROPNAME(name)), val);
  }
}

void
sprite_instance::removeMovieClip()
{
  int depth = get_depth();
  if ( depth < 0 || depth > 1048575 )
  {
    IF_VERBOSE_ASCODING_ERRORS(
    log_aserror(_("removeMovieClip(%s): sprite depth (%d) out of the "
      "'dynamic' zone [0..1048575], won't remove"),
      getTarget(), depth);
    );
    return;
  }

  sprite_instance* parent = dynamic_cast<sprite_instance*>(get_parent());
  if (parent)
  {
    // second argument is arbitrary, see comments above
    // the function declaration in sprite_instance.h
    parent->remove_display_object(depth, 0);
  }
  else
  {
    // removing _level#
    _vm.getRoot().dropLevel(depth);
    // I guess this can only happen if someone uses _root.swapDepth([0..1048575])
    //log_error(_("Can't remove sprite %s as it has no parent"), getTarget());
  }

}

geometry::Range2d<float>
sprite_instance::getBounds() const
{
  typedef geometry::Range2d<float> Range;

  Range bounds;
  BoundsFinder f(bounds);
  const_cast<DisplayList&>(m_display_list).visitAll(f);
  Range drawableBounds = _drawable->get_bound().getRange();
  bounds.expandTo(drawableBounds);
  
  return bounds;
}

bool
sprite_instance::isEnabled() const
{
  as_value enabled;
  // const_cast needed due to get_member being non-const due to the 
  // possibility that a getter-setter would actually modify us ...
  if ( ! const_cast<sprite_instance*>(this)->get_member(NSV::PROP_ENABLED, &enabled) )
  {
     // We're enabled if there's no 'enabled' member...
     return true;
  }
  return enabled.to_bool();
}

class EnumerateVisitor {

  as_environment& _env;

public:
  EnumerateVisitor(as_environment& env)
    :
    _env(env)
  {}

  void operator() (character* ch)
  {
    // don't enumerate unloaded characters
    if ( ch->isUnloaded() ) return;

    _env.push(ch->get_name());
  }
};

void
sprite_instance::enumerateNonProperties(as_environment& env) const
{
  EnumerateVisitor visitor(env);
  m_display_list.visitAll(visitor);
}

void
sprite_instance::cleanupDisplayList()
{
        //log_debug("%s.cleanDisplayList() called, current dlist is %p", getTarget(), (void*)&m_display_list);
  m_display_list.removeUnloaded();
}

#ifdef GNASH_USE_GC
struct ReachableMarker {
  void operator() (character *ch)
  {
    ch->setReachable();
  }
};
void
sprite_instance::markReachableResources() const
{
  ReachableMarker marker;

  m_display_list.visitAll(marker);

  assert(m_tmp_display_list.empty());

  _drawable->setReachable();

  _drawable_inst->setReachable();

  m_as_environment.markReachableResources();

  // Mark our own definition
  if ( m_def.get() ) m_def->setReachable();

  // Mark textfields in the TextFieldMap
  if ( _text_variables.get() )
  {
    for(TextFieldMap::const_iterator i=_text_variables->begin(),
          e=_text_variables->end();
        i!=e; ++i)
    {
      const TextFieldPtrVect& tfs=i->second;
      for (TextFieldPtrVect::const_iterator j=tfs.begin(), je=tfs.end(); j!=je; ++j)
      {
        (*j)->setReachable();
      }
    }
  }

  // Mark our relative root
  assert(m_root != NULL);
  m_root->setReachable();

  markCharacterReachable();

}
#endif // GNASH_USE_GC

void
sprite_instance::destroy()
{
  stopStreamSound();

  m_display_list.destroy();

  /// We don't need these anymore
  clearProperties();

  character::destroy();
}

cxform
sprite_instance::get_world_cxform() const
{
  cxform cf = character::get_world_cxform();
  cf.concatenate(_userCxform); 
  return cf;
}

movie_instance*
sprite_instance::get_root() const
{
	return m_root;
}

const sprite_instance*
sprite_instance::getAsRoot() const
{
	//log_debug("getAsRoot called for sprite %s, with _lockroot %d and version %d", getTarget(), getLockRoot(), getSWFVersion());

	// TODO1: as an optimization, if swf version < 7 
	//        we might as well just return m_root, 
	//        the whole chain from this sprite to it's
	//        m_root should have the same version...
	//
	// TODO2: implement this with iteration rather
	//        then recursion.
	//        

	character* parent = get_parent();
	if ( ! parent ) return this; // no parent, we're the root

	// If we have a parent, we descend to it unless 
	// our _lockroot is true AND our or the VM's
	// SWF version is > 6
	//
	if ( getSWFVersion() > 6 || getVM().getSWFVersion() > 6 )
	{
		if ( getLockRoot() )
		{
			return this; // locked
		}
	}

	return parent->getAsRoot();
}

as_value
sprite_instance::lockroot_getset(const fn_call& fn)
{
  boost::intrusive_ptr<sprite_instance> ptr = ensureType<sprite_instance>(fn.this_ptr);

  as_value rv;
  if ( fn.nargs == 0 ) // getter
  {
    rv.set_bool(ptr->getLockRoot());
  }
  else // setter
  {
    ptr->setLockRoot(fn.arg(0).to_bool());
  }
  return rv;

}

void
sprite_instance::setStreamSoundId(int id)
{
	if ( id != m_sound_stream_id )
	{
		log_debug("Stream sound id from %d to %d, stopping old", m_sound_stream_id, id);
		stopStreamSound();
	}
	m_sound_stream_id = id;
}

void
sprite_instance::stopStreamSound()
{
	if ( m_sound_stream_id == -1 ) return; // nothing to do

	media::sound_handler* handler = get_sound_handler(); // TODO: chache ?
	if (handler)
	{
		handler->stop_sound(m_sound_stream_id);
	}

	m_sound_stream_id = -1;
}

void
sprite_instance::set_play_state(play_state s)
{
	if ( s == m_play_state ) return; // nothing to do
	if ( s == sprite_instance::STOP ) stopStreamSound();
	m_play_state = s;
}

#ifdef USE_SWFTREE

class MovieInfoVisitor {

  character::InfoTree& _tr;
  character::InfoTree::iterator _it;

public:
  MovieInfoVisitor(character::InfoTree& tr, character::InfoTree::iterator it)
    :
    _tr(tr),
    _it(it)
  {}

  void operator() (character* ch)
  {
    //if ( ch->isUnloaded() ) return; // or maybe we should stil print these..
    ch->getMovieInfo(_tr, _it);
  }
};

character::InfoTree::iterator 
sprite_instance::getMovieInfo(InfoTree& tr, InfoTree::iterator it)
{
	InfoTree::iterator selfIt = character::getMovieInfo(tr, it);
	std::ostringstream os;
	os << m_display_list.size();
	InfoTree::iterator localIter = tr.append_child(selfIt, StringPair(_("Childs"), os.str()));	    
	//localIter = tr.append_child(localIter, StringPair("child1", "fake"));
	//localIter = tr.append_child(localIter, StringPair("child2", "fake"));

	MovieInfoVisitor v(tr, localIter);
	m_display_list.visitAll(v);

	return selfIt;

}

#endif // USE_SWFTREE

} // namespace gnash
