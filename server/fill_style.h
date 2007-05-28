// 
//   Copyright (C) 2007 Free Software Foundation, Inc.
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

// Based on work of Thatcher Ulrich <tu@tulrich.com> 2003

/* $Id: fill_style.h,v 1.5 2007/05/28 15:41:06 ann Exp $ */

#ifndef GNASH_FILL_STYLE_H
#define GNASH_FILL_STYLE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include "impl.h"
#include "types.h"
#include "matrix.h"
#include "bitmap_character_def.h"

namespace gnash {

class stream;

class gradient_record
{
public:
	gradient_record();
	void	read(stream* in, int tag_type);
	
	//data:
	uint8_t	m_ratio;
	rgba	m_color;
};


/// For the interior of outline shapes.
//
class DSOEXPORT fill_style 
{
public:

	/// Create a solid opaque white fill.
	fill_style();

	/// Construct a clipped bitmap fill style, for
	/// use by bitmap shape character.
	///
	/// TODO: use a subclass for this
	///
	fill_style(bitmap_character_def* bitmap);

	void setSolid(const rgba& color);

	virtual ~fill_style();
	
	/// Read the fill style from a stream
	//
	/// TODO: use a subclass for this (swf_fill_style?)
	///
	void	read(stream* in, int tag_type, movie_definition* m);

	/// \brief
	/// Make a bitmap_info* corresponding to our gradient.
	/// We can use this to set the gradient fill style.
	gnash::bitmap_info*	create_gradient_bitmap() const;
	
	/// \brief
	/// Makes sure that m_gradient_bitmap_info is not NULL. Calls 
	/// create_gradient_bitmap() if necessary and returns m_gradient_bitmap_info.
	gnash::bitmap_info* need_gradient_bitmap() const; 
	
	rgba	get_color() const { return m_color; }

	void	set_color(rgba new_color) { m_color = new_color; }

	/// Get fill type, see SWF::fill_style_type
	int	get_type() const { return m_type; }
	
	/// Sets this style to a blend of a and b.  t = [0,1] (for shape morphing)
	void	set_lerp(const fill_style& a, const fill_style& b, float t);
	
	/// Returns the bitmap info for all styles except solid fills
	//
	/// NOTE: calling this method against a solid fill style will
	///       result in a failed assertion.
	/// 
	/// NOTE2: this function can return NULL if the character_id
	///        specified for the style in the SWF does not resolve
	///        to a character defined in the characters dictionary.
	///        (it happens..)
	///
	bitmap_info* get_bitmap_info() const;
	
	/// Returns the bitmap transformation matrix
	matrix get_bitmap_matrix() const; 
	
	/// Returns the gradient transformation matrix
	matrix get_gradient_matrix() const; 
	
	/// Returns the number of color stops in the gradient
	int get_color_stop_count() const;
	
	/// Returns the color stop value at a specified index
	const gradient_record& get_color_stop(int index) const;
	
private:

	/// Return the color at the specified ratio into our gradient.
	//
	/// @param ratio
	///	Ratio is in the range [0, 255].
	///
	rgba sample_gradient(uint8_t ratio) const;

	friend class morph2_character_def;
	friend class triangulating_render_handler;
	
	/// Fill type, see SWF::fill_style_type
	int	m_type;
	rgba	m_color;
	matrix	m_gradient_matrix;
	std::vector<gradient_record>	m_gradients;
	boost::intrusive_ptr<gnash::bitmap_info>	m_gradient_bitmap_info;
	boost::intrusive_ptr<bitmap_character_def>	m_bitmap_character;
	matrix	m_bitmap_matrix;
};


} // namespace gnash


#endif // GNASH_FILL_STYLE_H


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
