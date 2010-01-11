// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
//   Foundation, Inc
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
//

#ifndef GNASH_TEXTFORMAT_H
#define GNASH_TEXTFORMAT_H

#include "as_object.h" 
#include "TextField.h"
#include "RGBA.h" 

#include <boost/cstdint.hpp> 
#include <string>

namespace gnash {  

/// TODO: SWF8 has two additional members: kerning and letterSpacing.
class TextFormat_as : public Relay
{
public:
  
	TextFormat_as();
	~TextFormat_as() {}

	/// Return a Boolean value that indicates whether the text is underlined.
	bool underlined() const { return _underline; }
	bool underlinedDefined() const { return _flags&DEFunderline; }

	/// Return a Boolean value that indicates whether the text is italicized.
	bool italiced() const { return _italic; }
	bool italicedDefined() const { return _flags&DEFitalic; }

	/// Return a Boolean value that indicates whether the text is boldface.
	bool bold() const { return _bold; }
	bool boldDefined() const { return _flags&DEFbold; }

	bool bullet() const { return _bullet; }
	bool bulletDefined() const { return _flags&DEFbullet; }

	/// Return the color of text using this text format.
	const rgba& color() const { return _color; }
	bool colorDefined() const { return _flags&DEFcolor; }

	TextField::TextFormatDisplay display() const {return _display; }
	bool displayDefined() const { return _flags&DEFdisplay; }
	
	std::vector<int> tabStops() const { return _tabStops; }
	bool tabStopsDefined() const { return _flags&DEFtabStops; }
	
    void tabStopsSet(const std::vector<int>& tabStops) { 
        _tabStops = tabStops;
		_flags |= DEFtabStops; 
	}

	/// \brief
	/// Return an integer that indicates the indentation from the left
    /// margin to the first DisplayObject in the paragraph
	boost::uint16_t indent() const { return _indent; }
	bool indentDefined() const { return _flags&DEFindent; }

	/// Return the alignment of the paragraph.
	TextField::TextAlignment align() const { return _align; }
	bool alignDefined() const { return _flags&DEFalign; }

	/// Return the name of a font for text as a string.
	const std::string& font() const { return _font; }
	bool fontDefined() const { return _flags&DEFfont; }

	// See doc for _target member
	const std::string& target() const { return _target; }
	bool targetDefined() const { return _flags&DEFtarget; }

	// See doc for _target member
	void targetSet(const std::string& s) { _target=s; _flags |= DEFtarget; }

	// See doc for _url member
	const std::string& url() const { return _url; }
	bool urlDefined() const { return _flags&DEFurl; }

	// See doc for _url member
	void urlSet(const std::string& s) { _url=s; _flags |= DEFurl; }

	///
	boost::uint16_t blockIndent() { return _blockIndent; }
	bool blockIndentDefined() const { return _flags&DEFblockIndent; }

	/// Return a number that indicates the amount of leading vertical
	/// space between lines.
	boost::uint16_t leading()     { return _leading; }
	bool leadingDefined() const { return _flags&DEFleading; }

	/// Indicates the left margin of the paragraph, in points.
	boost::uint16_t leftMargin()  { return _leftMargin; }
	bool leftMarginDefined() const { return _flags&DEFleftMargin; }

	/// Indicates the right margin of the paragraph, in points.
	boost::uint16_t rightMargin() { return _rightMargin; }
	bool rightMarginDefined() const { return _flags&DEFrightMargin; }

	/// Return a float that indicates the point size in twips.
	boost::uint16_t size()        { return _pointSize; }
	bool sizeDefined() const { return _flags&DEFsize; }

	void underlinedSet(bool x)   { _underline = x; _flags |= DEFunderline; }
	void italicedSet(bool x)     { _italic = x; _flags |= DEFitalic; }
	void boldSet(bool x)         { _bold = x; _flags |= DEFbold; }
	void bulletSet(bool x)       { _bullet = x; _flags |= DEFbullet; }
	void colorSet(const rgba& x)      { _color = x; _flags |= DEFcolor; }
	void indentSet(boost::uint16_t x)      { _indent = x; _flags |= DEFindent; }
	void fontSet(const std::string& font) { _font=font; _flags |= DEFfont; }
	void displaySet(TextField::TextFormatDisplay x) {
		_display = x;
		_flags |= DEFdisplay;
	}
	
	void displaySet(const std::string& display);
	
    void alignSet(TextField::TextAlignment x) {
        _align = x;
        _flags |= DEFalign;
    }

	void alignSet(const std::string& align);

	void blockIndentSet(boost::uint16_t x)   { 
        _blockIndent = x;
        _flags |= DEFblockIndent;
    }

	void leadingSet(boost::uint16_t x) {
        _leading = x;
        _flags |= DEFleading;
    }

	void leftMarginSet(boost::uint16_t x) {
        _leftMargin = x;
        _flags |= DEFleftMargin;
    }
	
    void rightMarginSet(boost::uint16_t x) {
        _rightMargin = x;
        _flags |= DEFrightMargin;
    }

	/// Set font point size in twips
	void sizeSet(boost::uint16_t x) {
        _pointSize = x;
        _flags |= DEFsize;
    }

private:

    enum {
		DEFunderline	=1<<0,
		DEFbold		=1<<1,
		DEFitalic	=1<<2,
		DEFbullet	=1<<3,
		DEFalign	=1<<4,
		DEFblockIndent	=1<<5,
		DEFcolor	=1<<6,
		DEFfont		=1<<7,
		DEFindent	=1<<8,
		DEFleading	=1<<9,
		DEFleftMargin	=1<<10,
		DEFrightMargin	=1<<11,
		DEFpointSize	=1<<12,
		DEFtabStops	=1<<13,
		DEFtarget	=1<<14,
		DEFurl		=1<<15,
		DEFsize		=1<<16,
		DEFdisplay  =1<<17
	};

    // need at least 17 bit here... (1<<16)
	boost::uint64_t _flags;
    //boost::uint32_t _flags; 

	/// A Boolean value that indicates whether the text is underlined.
	bool _underline;

	/// A Boolean value that indicates whether the text is boldface.
	bool _bold;

	/// A Boolean value that indicates whether the text is italicized.
	bool _italic;

	// 
	bool _bullet;
	
	TextField::TextFormatDisplay _display;
  
	/// The alignment of the paragraph, represented as a string.
	//
	/// If "left", the paragraph is left-aligned. If "center", the
	/// paragraph is centered. If "right", the paragraph is
	/// right-aligned. If "justify", the paragraph is justified.
	///
	TextField::TextAlignment _align;

	// 
	boost::uint16_t _blockIndent;

	/// The color of text using this text format.
	//
	/// A number containing three 8-bit RGB components; for example,
        /// 0xFF0000 is red, 0x00FF00 is green.
	rgba _color;	

	// The name of a font for text as a string.
	std::string _font;	

	/// An integer that indicates the indentation from the left
    /// margin to the first DisplayObject in the paragraph (twips)
	boost::uint16_t _indent;

	/// A number that indicates the amount of leading vertical
	/// space between lines (twips)
	boost::uint16_t _leading;

	/// Indicates the left margin of the paragraph, in points (twips)
	boost::uint16_t _leftMargin;

	/// Indicates the right margin of the paragraph, in points (twips).
	boost::uint16_t _rightMargin;

	/// Point size in twips.
	boost::uint16_t	_pointSize;

	///
	std::vector<int> _tabStops;

	/// The target window where the hyperlink is displayed. 
        /// If the target window is an empty string, the text is displayed in
        /// the default target window _self. If the url parameter is
        /// set to an empty string or to the value null, you can get
        /// or set this property, but the property will have no effect.
	std::string	_target;

	/// The URL to which the text in this text format hyperlinks.
	/// If url is an empty string, the text does not have a hyperlink
	std::string	 _url;	
};

void textformat_class_init(as_object& global, const ObjectURI& uri);

void registerTextFormatNative(as_object& global);

} // end of gnash namespace

#endif

