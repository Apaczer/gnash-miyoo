// sprite_definition.h:  Holds immutable data for a sprite, for Gnash.
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
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef GNASH_SPRITE_DEFINITION_H
#define GNASH_SPRITE_DEFINITION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>

#include "movie_definition.h"
#include "stream.h"
#include "log.h"
#include "rect.h"
#include "Timeline.h" // for composition 

namespace gnash
{


/// \brief
/// Holds the immutable data for a sprite, as read from
/// as SWF stream.
/// @@ should *not* derive from movie_definition, probably!
///
class sprite_definition : public movie_definition
{

public:

	/// \brief
	/// Read the sprite info from input stream.
	//
	/// A sprite definition consists of a series control tags.
	///
	/// @param m
	///	the Top-Level movie_definition this sprite is read
	///	from (not a sprite_definition!)
	///
	/// @param in
	///	The stream associated with the sprite. It is assumed
	///	to be already positioned right before the frame count
	///
	sprite_definition(movie_definition* m, stream* in);

	/// Destructor, releases playlist data
	~sprite_definition();

	/// Register a class to this definition.
	//
	/// New instances of this symbol will get the given class
	/// interface.
	///
	/// @param the_class
	///	The class constructor to associate with
	///	new instances of this character. If NULL
	///	new instances will get the MovieClip interface.
	///
	void registerClass(as_function* the_class);

	/// Get the Class registered to this definition.
	as_function* getRegisteredClass()
	{
		return registeredClass.get();
	}

	// See dox in base class
	//
	// TODO: implement in base class ?
	//
	void addTimelineDepth(int depth)
	{
		_timeline.addDepth(depth);
	}

	// See dox in base class
	//
	// TODO: implement in base class ?
	//
	void removeTimelineDepth(int depth)
	{
		_timeline.removeDepth(depth);
	}

	// See dox in base class
	//
	// TODO: implement in base class ?
	//
	void getTimelineDepths(size_t frameno, std::vector<int>& depths)
	{
		_timeline.getFrameDepths(frameno, depths);
	}

private:

	void read(stream* in);

	/// Tags loader table.
	//
	/// TODO: make it a static member, specific to sprite_definition
	SWF::TagLoadersTable& _tag_loaders;

	/// Top-level movie definition
	/// (the definition read from SWF stream)
	movie_definition* m_movie_def;

	/// movie control events for each frame.
	std::vector<PlayList> m_playlist;

	// stores 0-based frame #'s
	typedef std::map<std::string, size_t> NamedFrameMap;
	NamedFrameMap m_named_frames;

	size_t m_frame_count;

	size_t m_loading_frame;

	// overloads from movie_definition
	virtual float	get_width_pixels() const { return 1; }
	virtual float	get_height_pixels() const { return 1; }

	virtual size_t	get_frame_count() const
	{
		return m_frame_count;
	}

	/// \brief
	/// Return total bytes of the movie from which this sprite
	/// has been read.
	///
	virtual size_t get_bytes_total() const
	{
		return m_movie_def->get_bytes_total();
	}

	/// \brief
	/// Return the number of bytes loaded from the stream of the
	/// the movie from which this sprite is being read.
	///
	virtual size_t get_bytes_loaded() const
	{
		return m_movie_def->get_bytes_loaded();
	}

	virtual float	get_frame_rate() const
	{
		return m_movie_def->get_frame_rate();
	}

	const rect& get_frame_size() const
	{
		assert(0);
		static const rect unused;
		return unused;
	}

	// Return number of frames loaded (of current sprite)
	virtual size_t	get_loading_frame() const { return m_loading_frame; }

	virtual int	get_version() const
	{
		return m_movie_def->get_version();
	}

	/// Overridden just for complaining  about malformed SWF
	virtual void add_font(int /*id*/, font* /*ch*/)
	{
		IF_VERBOSE_MALFORMED_SWF (
		log_swferror(_("add_font tag appears in sprite tags"));
		);
	}

	/// Delegate call to associated root movie
	virtual font* get_font(int id)
	{
		return m_movie_def->get_font(id);
	}

	/// Delegate call to associated root movie
	virtual bitmap_character_def* get_bitmap_character_def(int id)
	{
		return m_movie_def->get_bitmap_character_def(id);
	}

	/// Overridden just for complaining  about malformed SWF
	virtual void add_bitmap_character_def(int /*id*/,
			bitmap_character_def* /*ch*/)
	{
		IF_VERBOSE_MALFORMED_SWF (
		log_swferror(_("add_bitmap_character_def appears in sprite tags"));
		);
	}

	/// Delegate call to associated root movie
	virtual sound_sample* get_sound_sample(int id)
	{
		return m_movie_def->get_sound_sample(id);
	}

	/// Overridden just for complaining  about malformed SWF
	virtual void add_sound_sample(int /*id*/, sound_sample* /*sam*/)
	{
		IF_VERBOSE_MALFORMED_SWF (
		log_swferror(_("add sam appears in sprite tags"));
		);
	}

	/// Delegate call to associated root movie
	virtual void set_loading_sound_stream_id(int id)
	{
		m_movie_def->set_loading_sound_stream_id(id);

	}

	/// Delegate call to associated root movie, or return -1
	virtual int get_loading_sound_stream_id()
	{
		return m_movie_def->get_loading_sound_stream_id();
	}


	/// Overridden just for complaining  about malformed SWF
	virtual void export_resource(const std::string& /*symbol*/,
			resource* /*res*/)
	{
		IF_VERBOSE_MALFORMED_SWF (
		log_swferror(_("Can't export from sprite"));
		);
	}

	/// Delegate call to associated root movie
	virtual boost::intrusive_ptr<resource> get_exported_resource(const std::string& sym)
	{
		return m_movie_def->get_exported_resource(sym);
	}

	/// \brief
	/// Get a character_def from this Sprite's root movie
	/// CharacterDictionary.
	///
	virtual character_def*	get_character_def(int id)
	{
	    return m_movie_def->get_character_def(id);
	}

	/// Overridden just for complaining  about malformed SWF
	//
	/// Calls to this function should only be made when
	/// an invalid SWF is being read, as it would mean
	/// that a Definition tag is been found as part of
	/// a Sprite definition
	///
	virtual void add_character(int /*id*/, character_def* /*ch*/)
	{
		IF_VERBOSE_MALFORMED_SWF (
		log_swferror(_("add_character tag appears in sprite tags"));
		);
	}


	virtual sprite_instance* create_instance()
	{
	    return NULL;
	}

	// Create a (mutable) instance of our definition.  The
	// instance is created to live (temporarily) on some level on
	// the parent movie's display list.
	//
	// overloads from character_def
	virtual character* create_character_instance(
		character* parent, int id);


	virtual void	add_execute_tag(execute_tag* c)
	{
		m_playlist[m_loading_frame].push_back(c);
	}

	/// Overridden just for complaining  about malformed SWF
	//
	/// Sprite def's should not have do_init_action tags in them!  (@@ correct?)
	virtual void	add_init_action(execute_tag* /*c*/)
	{
		IF_VERBOSE_MALFORMED_SWF (
		log_swferror(_("sprite_definition::add_init_action called!  Ignored"));
		);
	}

	// See dox in movie_definition.h
	virtual void	add_frame_name(const std::string& name);

	// See dox in movie_definition
	bool get_labeled_frame(const std::string& label, size_t& frame_number);

	/// frame_number is 0-based
	const PlayList& get_playlist(size_t frame_number)
	{
		return m_playlist[frame_number];
	}

	// Sprites do not have init actions in their
	// playlists!  Only the root movie
	// (movie_def_impl) does (@@ correct?)
	// .. no need to override as returning NULL is the default behaviour ..
	//virtual const PlayList* get_init_actions(size_t /*frame_number*/)

	virtual const std::string& get_url() const
	{
		return m_movie_def->get_url();
	}

	/// \brief
	/// Ensure framenum frames of this sprite
	/// have been loaded.
	///
	virtual bool ensure_frame_loaded(size_t framenum)
	{
		// TODO: return false on timeout
		while ( m_loading_frame < framenum )
		{
			log_msg(_("sprite_definition: "
				"loading of frame " SIZET_FMT " requested "
				"(we are at " SIZET_FMT "/" SIZET_FMT ")"),
				framenum, m_loading_frame, m_frame_count);
			// Could this ever happen ?
			assert(0);
		}
		return true;
	}

	virtual void load_next_frame_chunk()
	{
		/// We load full sprite definitions at once, so
		/// this function is a no-op.
	}

	/// Return the top-level movie definition
	/// (the definition read from SWF stream)
	movie_definition* get_movie_definition() {
		return m_movie_def;
	}

	const rect&	get_bound() const {
    // It is required that get_bound() is implemented in character definition
    // classes. However, it makes no sense to call it for sprite definitions.
    // get_bound() is currently only used by generic_character which normally
    // is used only shape character definitions. See character_def.h to learn
    // why it is virtual anyway.
    assert(0); // should not be called
		static rect unused;
		return unused;
  }

	/// \brief
	/// The constructor to use for setting up the interface
	/// for new instances of this sprite
	//
	/// If NULL, new instances will have the default MovieClip
	/// interface.
	///
	boost::intrusive_ptr<as_function> registeredClass;

	/// Timeline depth info
	//
	/// TODO: move to base class ?
	///
	Timeline _timeline;

};


} // end of namespace gnash

#endif // GNASH_SPRITE_H
