// -- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF buttons.  Mouse-sensitive update/display, actions, etc.


#ifndef GNASH_BUTTON_CHARACTER_DEF_H
#define GNASH_BUTTON_CHARACTER_DEF_H


#include "character_def.h"
#include "sound.h"
#include "rect.h" // for get_bound

#ifndef UNUSED
#define UNUSED(x) ((x) = (x))
#endif

// Forward declarations
namespace gnash {
	class sprite_instance;
}

namespace gnash {

class button_record
{

// TODO: make private, provide accessors 
public:

	bool	m_hit_test;
	bool	m_down;
	bool	m_over;
	bool	m_up;
	int	m_character_id;
	character_def* m_character_def;
	int	m_button_layer;
	matrix	m_button_matrix;
	cxform	m_button_cxform;

public:

	/// Read a button record from the SWF stream.
	//
	/// Return true if we read a record; false if this is a null
	bool	read(stream* in, int tag_type, movie_definition* m);

	/// Return true if the button_record is valid
	//
	/// A button record is invalid if it refers to a character
	/// which has not been defined.
	bool is_valid();

};
	
class button_action
{
public:
	enum condition
	{
		IDLE_TO_OVER_UP = 1 << 0,
		OVER_UP_TO_IDLE = 1 << 1,
		OVER_UP_TO_OVER_DOWN = 1 << 2,
		OVER_DOWN_TO_OVER_UP = 1 << 3,
		OVER_DOWN_TO_OUT_DOWN = 1 << 4,
		OUT_DOWN_TO_OVER_DOWN = 1 << 5,
		OUT_DOWN_TO_IDLE = 1 << 6,
		IDLE_TO_OVER_DOWN = 1 << 7,
		OVER_DOWN_TO_IDLE = 1 << 8
	};
	int	m_conditions;
	std::vector<action_buffer*>	m_actions;

	~button_action();
	void	read(stream* in, int tag_type);
};


class button_character_definition : public character_def
{
public:

  /// Smallest layer number used for button records 
  int m_min_layer;

  /// Greatest layer number used for button records 
  int m_max_layer;

	struct sound_info
	{
		void read(stream* in);

		bool m_no_multiple;
		bool m_stop_playback;
		bool m_has_envelope;
		bool m_has_loops;
		bool m_has_out_point;
		bool m_has_in_point;
		uint32_t m_in_point;
		uint32_t m_out_point;
		uint16_t m_loop_count;
		std::vector<sound_handler::sound_envelope> m_envelopes;
	};

	struct button_sound_info
	{
		uint16_t m_sound_id;
		sound_sample_impl*	m_sam;
		sound_info m_sound_style;
	};

	struct button_sound_def
	{
		void	read(stream* in, movie_definition* m);
		button_sound_info m_button_sounds[4];
	};


	bool m_menu;
	std::vector<button_record>	m_button_records;
	std::vector<button_action>	m_button_actions;
	button_sound_def*	m_sound;

	button_character_definition();
	virtual ~button_character_definition();

	/// Create a mutable instance of our definition.
	character* create_character_instance(character* parent, int id);

	void	read(stream* in, int tag_type, movie_definition* m);
	
	const rect&	get_bound() const {
    // It is required that get_bound() is implemented in character definition
    // classes. However, button character definitions do not have shape 
    // definitions themselves. Instead, they hold a list of shape_character_def.
    // get_bound() is currently only used by generic_character which normally
    // is used only shape character definitions. See character_def.h to learn
    // why it is virtual anyway.
    // get_button_bound() is used for buttons.
    assert(0); // should not be called  
		static rect unused;
		return unused;
  }
  
  const rect&	get_button_bound(int id) const {
    UNUSED(id);
    assert(0); // not implemented
  }
	
};

}	// end namespace gnash


#endif // GNASH_BUTTON_CHARACTER_DEF_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
