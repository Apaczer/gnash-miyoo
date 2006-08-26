// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
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

// Linking Gnash statically or dynamically with other modules is making a
// combined work based on Gnash. Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Gnash give you
// permission to combine Gnash with free software programs or libraries
// that are released under the GNU LGPL and with code included in any
// release of Talkback distributed by the Mozilla Foundation. You may
// copy and distribute such a system following the terms of the GNU GPL
// for all but the LGPL-covered parts and Talkback, and following the
// LGPL for the LGPL-covered parts.
//
// Note that people who make modified versions of Gnash are not obligated
// to grant this special exception for their modified versions; it is their
// choice whether to do so. The GNU General Public License gives permission
// to release a modified version without this exception; this exception
// also makes it possible to release a modified version which carries
// forward this exception.
// 
//
//

#ifndef __TEXTFORMAT_H__
#define __TEXTFORMAT_H__


//#include "action.h"
//#include "impl.h"
#include "as_object.h" // for textformat_as_object inheritance


namespace gnash {  

/// new text_format([font, [size, [color, [bold, [italic, [underline, [url, [target, [align,[leftMargin, [rightMargin, [indent, [leading]]]]]]]]]]]]])
class text_format
{
public:
  
	text_format();
	// tulrich: TODO need to take const ref!
	text_format(tu_string font);
	text_format(tu_string font, int size);
	text_format(tu_string font, int size, int color);
	~text_format();
  
	/// Return a Boolean value that indicates whether the text is underlined.
	bool underlined()  { return _underline; }

	/// Return a Boolean value that indicates whether the text is italicized.
	bool italiced()    { return _italic; }

	/// Return a Boolean value that indicates whether the text is boldface.
	bool bold()        { return _bold; }

	bool bullet()      { return _bullet; }

	/// Return the color of text using this text format.
	//
	/// A number containing three 8-bit RGB components; for example,
        /// 0xFF0000 is red, 0x00FF00 is green.
	uint32 color() const { return _color; }

	/// \brief
	/// Return ann integer that indicates the indentation from the left
        /// margin to the first character in the paragraph
	float indent() const { return _indent; }

	/// Return the alignment of the paragraph, represented as a string.
	//
	/// If "left", the paragraph is left-aligned. If "center", the
	/// paragraph is centered. If "right", the paragraph is
	/// right-aligned.
	///
	const tu_string& align() const { return _align; }

	/// Return the name of a font for text as a string.
	const tu_string& font() const { return _font; }

	///
	float blockIndent() { return _block_indent; }

	/// Return a number that indicates the amount of leading vertical
	/// space between lines.
	float leading()     { return _leading; }

	/// Indicates the left margin of the paragraph, in points.
	float leftMargin()  { return _left_margin; }

	/// Indicates the right margin of the paragraph, in points.
	float RightMargin() { return _right_margin; }

	/// Return a float that indicates the point size.
	float size()        { return _point_size; }

	void underlinedSet(bool x)   { _underline = x; }
	void italicedSet(bool x)     { _italic = x; }
	void boldSet(bool x)         { _bold = x; }
	void bulletSet(bool x)       { _bullet = x; }
	void colorSet(uint32 x)      { _color = x; }
	void indentSet(float x)      { _indent = x; }
	void alignSet(tu_string x)  { _align = x; }
	void blockIndentSet(float x)   { _block_indent = x; }
	void leadingSet(float x)     { _leading = x; }
	void leftMarginSet(float x)  { _left_margin = x; }
	void rightMarginSet(float x) { _right_margin = x; }
	void sizeSet(float x)        { _point_size = x; }

	// In a paragraph, change the format of a range of characters.
	void setTextFormat (text_format &format);
	void setTextFormat (int index, text_format &format);
	void setTextFormat (int start, int end, text_format &format);

	text_format &getTextFormat ();
	text_format &getTextFormat (int index);
	text_format &getTextFormat (int start, int end);

	int getTextExtant();
	text_format *operator = (text_format &format);
  
 private:
	/// A Boolean value that indicates whether the text is underlined.
	bool          _underline;

	/// A Boolean value that indicates whether the text is boldface.
	bool          _bold;	

	/// A Boolean value that indicates whether the text is italicized.
	bool          _italic;

	// 
	bool          _bullet;
  
	/// The alignment of the paragraph, represented as a string.
	//
	/// If "left", the paragraph is left-aligned. If "center", the
	/// paragraph is centered. If "right", the paragraph is
	/// right-aligned.
	///
	tu_string	 _align;

	// 
	float		_block_indent;

	/// The color of text using this text format.
	//
	/// A number containing three 8-bit RGB components; for example,
        /// 0xFF0000 is red, 0x00FF00 is green.
	uint32	_color;	

	// The name of a font for text as a string.
	tu_string _font;	

	/// An integer that indicates the indentation from the left
        /// margin to the first character in the paragraph
	float		_indent;

	/// A number that indicates the amount of leading vertical
	/// space between lines.
	float		_leading;

	/// Indicates the left margin of the paragraph, in points.
	float		_left_margin;

	/// Indicates the right margin of the paragraph, in points.
	float		_right_margin;

	/// An float that indicates the point size.
	float		_point_size;

	///
	int		_tab_stops;

	/// The target window where the hyperlink is displayed. 
        /// If the target window is an empty string, the text is displayed in
        /// the default target window _self. If the url parameter is
        /// set to an empty string or to the value null, you can get
        /// or set this property, but the property will have no effect.
	int		_target;

	/// The URL to which the text in this text format hyperlinks.
	/// If url is an empty string, the text does not have a hyperlink
	tu_string	 _url;	

};
 
class textformat_as_object : public gnash::as_object
{
public:
	text_format obj;
};

void textformat_new(const fn_call& fn);

void textformat_setformat(const fn_call& fn);

void textformat_getformat(const fn_call& fn);

} // end of gnash namespace

#endif	// __TEXTFORMAT_H__
