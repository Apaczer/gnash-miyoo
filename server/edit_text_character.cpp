// edit_text_character.cpp:  User-editable text regions, for Gnash.
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

#include "utf8.h"
#include "log.h"
#include "render.h" // for display()
#include "movie_definition.h" // to extract version info
#include "sprite_instance.h"
#include "edit_text_character.h"
#include "Key.h" // for keyboard events
#include "movie_root.h"	 // for killing focus
#include "as_environment.h" // for parse_path
#include "action.h" // for as_standard_member enum
#include "VM.h"
#include "builtin_function.h" // for getter/setter properties
#include "font.h" // for using the _font member
#include "fontlib.h" // for searching or adding fonts the _font member
#include "Object.h" // for getObjectInterface
#include "namedStrings.h"
#include "array.h" // for _listeners construction
#include "AsBroadcaster.h" // for initializing self as a broadcaster
#include "StringPredicates.h"
#include "TextFormat.h" // for getTextFormat/setTextFormat

#include <algorithm> // std::min
#include <string>
#include <boost/algorithm/string/case_conv.hpp>

// Text fields have a fixed 2 pixel padding for each side (regardless of border)
#define PADDING_TWIPS 40.0f

// Define the following macro to maintain compatibility with the proprietary
// player when it comes to opacity of textfields using device fonts.
// See http://gnashdev.org/wiki/index.php/DeviceFonts#Differences_with_proprietary_player_implementation
//
// This is now disabled by default because INCOMPLETE and unclean
// A clean implementation (IMHO) would warn user about the fact
// it is doing a stupid thing for compatibilty reason.
// Also, for good compatibility, we should skip rendering
// of rotated or skewed text.
//
//#define PP_COMPATIBLE_DEVICE_FONT_HANDLING 1

// Define the following to get detailed log information about
// textfield bounds and HTML tags:
//#define GNASH_DEBUG_TEXTFIELDS 1

namespace gnash {

// Forward declarations
static as_value textfield_get_variable(const fn_call& fn);
static as_value textfield_set_variable(const fn_call& fn);
static as_value textfield_setTextFormat(const fn_call& fn);
static as_value textfield_getTextFormat(const fn_call& fn);
static as_value textfield_setNewTextFormat(const fn_call& fn);
static as_value textfield_getNewTextFormat(const fn_call& fn);

static as_value textfield_getDepth(const fn_call& fn);
static as_value textfield_getFontList(const fn_call& fn);
static as_value textfield_removeTextField(const fn_call& fn);
static as_value textfield_replaceSel(const fn_call& fn);
static as_value textfield_replaceText(const fn_call& fn);
static as_object* getTextFieldInterface();

static as_value textfield_background_getset(const fn_call& fn);
static as_value textfield_border_getset(const fn_call& fn);
static as_value textfield_backgroundColor_getset(const fn_call& fn);
static as_value textfield_borderColor_getset(const fn_call& fn);
static as_value textfield_textColor_getset(const fn_call& fn);
static as_value textfield_embedFonts_getset(const fn_call& fn);
static as_value textfield_autoSize_getset(const fn_call& fn);
static as_value textfield_type_getset(const fn_call& fn);
static as_value textfield_wordWrap_getset(const fn_call& fn);
static as_value textfield_html_getset(const fn_call& fn);
static as_value textfield_selectable_getset(const fn_call& fn);
static as_value textfield_length_getset(const fn_call& fn);
static as_value textfield_textWidth_getset(const fn_call& fn);
static as_value textfield_textHeight_getset(const fn_call& fn);


//
// TextField interface functions
//

static as_value
textfield_get_variable(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);

	return as_value(text->get_variable_name());

}

static as_value
textfield_set_variable(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);

	assert ( fn.nargs > 0 );
	const std::string& newname = fn.arg(0).to_string();

	text->set_variable_name(newname);

	return as_value();
}

static as_value
textfield_getDepth(const fn_call& fn)
{
	// TODO: make this a character::getDepth_method function...
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);

	int n = text->get_depth();

	return as_value(n);

}

static as_value
textfield_getFontList(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);
	UNUSED(text);

	static bool warned = false;
	if ( ! warned ) {
		log_unimpl("TextField.getFontList()");
		warned = true;
	}

	return as_value();
}

static as_value
textfield_getNewTextFormat(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);
	UNUSED(text);

	LOG_ONCE(log_unimpl("TextField.getNewTextFormat()"));

	return as_value();
}

static as_value
textfield_getTextFormat(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);

	boost::intrusive_ptr<TextFormat> tf = new TextFormat();
	tf->alignSet(text->getTextAlignment());
	tf->sizeSet(text->getFontHeight());
	tf->indentSet(text->getIndent());
	tf->blockIndentSet(text->getBlockIndent());
	tf->leadingSet(text->getLeading());
	tf->leftMarginSet(text->getLeftMargin());
	tf->rightMarginSet(text->getRightMargin());
	tf->colorSet(text->getTextColor());
	tf->underlinedSet(text->getUnderlined());

	const font* font = text->getFont();
	if (font)
	{
		tf->fontSet(font->get_name());
		tf->italicedSet(font->isItalic());
		tf->boldSet(font->isBold());
	}

	// TODO: add font color and some more

	LOG_ONCE( log_unimpl("TextField.getTextFormat() discards url, target, tabStops, bullet and display") );

	return as_value(tf.get());
}

static as_value
textfield_setTextFormat(const fn_call& fn)
{
	//GNASH_REPORT_FUNCTION;

	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);

	if ( ! fn.nargs )
	{
		IF_VERBOSE_ASCODING_ERRORS(
		std::stringstream ss; fn.dump_args(ss);
		log_aserror("TextField.setTextFormat(%s) : %s", ss.str(), _("missing arg"))
		);
		return as_value();
	}
	else if ( fn.nargs > 2 )
	{
		std::stringstream ss; fn.dump_args(ss);
		log_debug("TextField.setTextFormat(%s) : args past the first are unhandled by Gnash", ss.str());
	}

	as_object* obj = fn.arg(0).to_object().get();
	if ( ! obj )
	{
		IF_VERBOSE_ASCODING_ERRORS(
		std::stringstream ss; fn.dump_args(ss);
		log_aserror("TextField.setTextFormat(%s) : %s", ss.str(), _("first argument is not an object"))
		);
		return as_value();
	}

	TextFormat* tf = dynamic_cast<TextFormat*>(obj);
	if ( ! tf )
	{
		IF_VERBOSE_ASCODING_ERRORS(
		std::stringstream ss; fn.dump_args(ss);
		log_aserror("TextField.setTextFormat(%s) : %s", ss.str(), _("first argument is not a TextFormat"))
		);
		return as_value();
	}

	if ( tf->alignDefined() ) text->setAlignment(tf->align());
	if ( tf->sizeDefined() ) text->setFontHeight(tf->size()); // keep twips
	if ( tf->indentDefined() ) text->setIndent(tf->indent());
	if ( tf->blockIndentDefined() ) text->setBlockIndent(tf->blockIndent());
	if ( tf->leadingDefined() ) text->setLeading(tf->leading());
	if ( tf->leftMarginDefined() ) text->setLeftMargin(tf->leftMargin());
	if ( tf->rightMarginDefined() ) text->setRightMargin(tf->rightMargin());
	if ( tf->colorDefined() ) text->setTextColor(tf->color());
	if ( tf->underlinedDefined() ) text->setUnderlined(tf->underlined());

	if ( tf->fontDefined() )
	{
		const std::string& fontName = tf->font();
		if ( ! fontName.empty() )
		{
			bool bold = tf->bold();
			bool italic = tf->italiced();

			font* f = fontlib::get_font(fontName, bold, italic);
			text->setFont( f );
		}
	}

	// TODO: add font color and some more

	LOG_ONCE( log_unimpl("TextField.setTextFormat() discards url, target, tabStops, bullet and display") );

	return as_value();

}

static as_value
textfield_setNewTextFormat(const fn_call& fn)
{
	//boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);
	//UNUSED(text);

	LOG_ONCE( log_unimpl("TextField.setNewTextFormat(), we'll delegate to setTextFormat") );
	return textfield_setTextFormat(fn);

	//return as_value();
}

static as_value
textfield_replaceSel(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);
	UNUSED(text);

	static bool warned = false;
	if ( ! warned ) {
		log_unimpl("TextField.replaceSel()");
		warned = true;
	}

	return as_value();
}

static as_value
textfield_replaceText(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);
	UNUSED(text);

	static bool warned = false;
	if ( ! warned ) {
		log_unimpl("TextField.replaceText()");
		warned = true;
	}

	return as_value();
}

static as_value
textfield_removeTextField(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> text = ensureType<edit_text_character>(fn.this_ptr);

	text->removeTextField();

	static bool warned = false;
	if ( ! warned ) {
		log_debug("TextField.removeTextField() TESTING");
		warned = true;
	}

	return as_value();
}

static as_value
textfield_ctor(const fn_call& /* fn */)
{
	as_object* proto = getTextFieldInterface();
	if ( ! proto ) proto = getObjectInterface(); // for SWF5 and below I guess...
	boost::intrusive_ptr<as_object> obj = new as_object(proto);
	return as_value(obj);
}

//
// TextField interface initialization
//

static void
attachTextFieldInterface(as_object& o)
{
	boost::intrusive_ptr<builtin_function> getset;

	int target_version = o.getVM().getSWFVersion();

	// TextField is an AsBroadcaster
        AsBroadcaster::initialize(o);

	int propFlags = as_prop_flags::dontDelete
		|as_prop_flags::dontEnum
		|as_prop_flags::readOnly
		|as_prop_flags::isProtected;

	// Parent seems to not be a normal property
	getset = new builtin_function(&character::parent_getset, NULL);
	o.init_property(NSV::PROP_uPARENT, *getset, *getset);

	// Target seems to not be a normal property
	getset = new builtin_function(&character::target_getset, NULL);
	o.init_property(NSV::PROP_uTARGET, *getset, *getset);

	// _name should be a property of the instance, not the prototype
	getset = new builtin_function(&character::name_getset, NULL);
	o.init_property(NSV::PROP_uNAME, *getset, *getset);

	o.init_property(NSV::PROP_uXMOUSE, character::xmouse_get, character::xmouse_get, propFlags);
	o.init_property(NSV::PROP_uYMOUSE, character::ymouse_get, character::ymouse_get, propFlags);
	o.init_property(NSV::PROP_uXSCALE, character::xscale_getset, character::xscale_getset);
	o.init_property(NSV::PROP_uYSCALE, character::yscale_getset, character::yscale_getset);
	o.init_property(NSV::PROP_TEXT_WIDTH, textfield_textWidth_getset, textfield_textWidth_getset);
	o.init_property(NSV::PROP_TEXT_HEIGHT, textfield_textHeight_getset, textfield_textHeight_getset);


	// SWF5 or higher (TODO: check textfields in SWF5 !!! we miss tests here !)
	if ( target_version  < 6 ) return;

	// SWF6 or higher

	boost::intrusive_ptr<builtin_function> variable_getter(new builtin_function(&textfield_get_variable, NULL));
	boost::intrusive_ptr<builtin_function> variable_setter(new builtin_function(&textfield_set_variable, NULL));
	o.init_property("variable", *variable_getter, *variable_setter);
	o.init_member("setTextFormat", new builtin_function(textfield_setTextFormat));
	o.init_member("getTextFormat", new builtin_function(textfield_getTextFormat));

	o.init_member("setNewTextFormat", new builtin_function(textfield_setNewTextFormat));
	o.init_member("getNewTextFormat", new builtin_function(textfield_getNewTextFormat));
	o.init_member("getNewTextFormat", new builtin_function(textfield_getNewTextFormat));
	o.init_member("getDepth", new builtin_function(textfield_getDepth));
	o.init_member("removeTextField", new builtin_function(textfield_removeTextField));
	o.init_member("replaceSel", new builtin_function(textfield_replaceSel));

	// The following properties should only be attached to the prototype
	// on first textfield creation. We won't get to that detail of compatibility,
	// seeming not important
	getset = new builtin_function(textfield_background_getset);
	o.init_property("background", *getset, *getset);
	getset = new builtin_function(textfield_backgroundColor_getset);
	o.init_property("backgroundColor", *getset, *getset);
	getset = new builtin_function(textfield_border_getset);
	o.init_property("border", *getset, *getset);
	getset = new builtin_function(textfield_borderColor_getset);
	o.init_property("borderColor", *getset, *getset);
	getset = new builtin_function(textfield_textColor_getset);
	o.init_property("textColor", *getset, *getset);
	getset = new builtin_function(textfield_embedFonts_getset);
	o.init_property("embedFonts", *getset, *getset);
	getset = new builtin_function(textfield_autoSize_getset);
	o.init_property("autoSize", *getset, *getset);
	getset = new builtin_function(textfield_type_getset);
	o.init_property("type", *getset, *getset);
	getset = new builtin_function(textfield_wordWrap_getset);
	o.init_property("wordWrap", *getset, *getset);
	getset = new builtin_function(textfield_html_getset);
	o.init_property("html", *getset, *getset);
	getset = new builtin_function(textfield_selectable_getset);
	o.init_property("selectable", *getset, *getset);
	getset = new builtin_function(textfield_length_getset);
	o.init_property("length", *getset, *getset);


	if ( target_version  < 7 ) return;

	// SWF7 or higher
	o.init_member("replaceText", new builtin_function(textfield_replaceText));
	if ( target_version  < 8 ) return;

}

static void
attachTextFieldStaticMembers(as_object& o)
{
	int target_version = o.getVM().getSWFVersion();

	// SWF5 or higher
	if ( target_version  < 6 ) return;

	// SWF6 or higher
	o.init_member("getFontList", new builtin_function(textfield_getFontList));
	if ( target_version  < 7 ) return;

	// SWF7 or higher
	if ( target_version  < 8 ) return;

}

static as_object*
getTextFieldInterface()
{
	static boost::intrusive_ptr<as_object> proto;

	if ( VM::get().getSWFVersion() < 6 ) return NULL;

	if ( proto == NULL )
	{
		proto = new as_object(getObjectInterface());
		VM::get().addStatic(proto.get());

		attachTextFieldInterface(*proto);
		//proto->init_member("constructor", new builtin_function(textfield_ctor)); 
	}
	return proto.get();
}

//
// edit_text_character class
//

edit_text_character::edit_text_character(character* parent,
		edit_text_character_def* def, int id)
	:
	character(parent, id),
	_text(L""),
	_textDefined(def->has_text()),
	m_def(def),
	_underlined(false),
	_leading(m_def->get_leading()),
	_alignment(def->get_alignment()),
	_indent(def->get_indent()), 
	_blockIndent(0),
	_leftMargin(def->get_left_margin()), 
	_rightMargin(def->get_right_margin()), 
	_fontHeight(def->get_font_height()), 
	_font(0),
	m_has_focus(false),
	m_cursor(0u),
	m_xcursor(0.0f),
	m_ycursor(0.0f),
	_text_variable_registered(false),
	_variable_name(m_def->get_variable_name()),
	_drawBackground(m_def->has_border()),
	_backgroundColor(255,255,255,255),
	_drawBorder(m_def->has_border()),
	_borderColor(0,0,0,255),
	_textColor(m_def->get_text_color()),
	_embedFonts(m_def->getUseEmbeddedGlyphs()),
	_wordWrap(m_def->do_word_wrap()),
	_html(m_def->htmlAllowed()),
	_selectable(!m_def->get_no_select()),
	_autoSize(autoSizeNone),
	_type(m_def->get_readonly() ? typeDynamic : typeInput),
	_bounds(m_def->get_bounds().getRange())
{
	assert(parent);
	assert(m_def);

	set_prototype(getTextFieldInterface());

	as_array_object* ar = new as_array_object();
	ar->push(this);
	set_member(NSV::PROP_uLISTENERS, ar);

	// WARNING! remember to set the font *before* setting text value!
	setFont( m_def->get_font() );

	// set default text *before* calling registerTextVariable
	// (if the textvariable already exist and has a value
	//  the text will be replaced with it)

	int version = VM::get().getSWFVersion();
	
	if ( _textDefined ) 
	{
		setTextValue(utf8::decodeCanonicalString(m_def->get_default_text(), version));
	}

	registerTextVariable();

	m_dummy_style.push_back(fill_style());

	reset_bounding_box(0, 0);

}

edit_text_character::~edit_text_character()
{
}

bool
edit_text_character::unload()
{
	// TODO: unregisterTextVariable() ?
	on_event(event_id::KILLFOCUS);

	return character::unload(); 
}

void
edit_text_character::removeTextField()
{
	int depth = get_depth();
	if ( depth < 0 || depth > 1048575 )
	{
		//IF_VERBOSE_ASCODING_ERRORS(
		log_debug(_("CHECKME: removeTextField(%s): TextField depth (%d) out of the "
			"'dynamic' zone [0..1048575], won't remove"),
			getTarget(), depth);
		//);
		return;
	}

	character* parent = get_parent();
	assert(parent); // every TextField must have a parent, right ?

	sprite_instance* parentSprite = parent->to_movie();

	if ( ! parentSprite )
	{
		log_error("FIXME: attempt to remove a TextField being a child of a %s", typeName(*parent));
		return;
	}

	// second argument is arbitrary, see comments above
	// the function declaration in sprite_instance.h
	parentSprite->remove_display_object(depth, 0);
}

void
edit_text_character::show_cursor(const matrix& mat)
{
	boost::uint16_t x = static_cast<boost::uint16_t>(m_xcursor);
	boost::uint16_t y = static_cast<boost::uint16_t>(m_ycursor);
	boost::uint16_t h = getFontHeight();

	boost::int16_t box[4];
	box[0] = x;
	box[1] = y;
	box[2] = x;
	box[3] = y + h;
	
	render::draw_line_strip(box, 2, rgba(0,0,0,255), mat);	// draw line
}

void
edit_text_character::display()
{
	//GNASH_REPORT_FUNCTION;

	registerTextVariable();

	//rect def_bounds = m_def->get_bounds();
	
	bool drawBorder = getDrawBorder();
	bool drawBackground = getDrawBackground();

	matrix	wmat = get_world_matrix();

	if ( (drawBorder || drawBackground) && _bounds.isFinite() )
	{

		point	coords[4];

		float xmin = _bounds.getMinX();
		float xmax = _bounds.getMaxX();
		float ymin = _bounds.getMinY();
		float ymax = _bounds.getMaxY();

		coords[0].setTo(xmin, ymin); 
		coords[1].setTo(xmax, ymin); 
		coords[2].setTo(xmax, ymax); 
		coords[3].setTo(xmin, ymax); 

		rgba borderColor = drawBorder ? getBorderColor() : rgba(0,0,0,0);
		rgba backgroundColor = drawBackground ? getBackgroundColor() : rgba(0,0,0,0);

		cxform	cx = get_world_cxform();
			
		if (drawBorder)
			borderColor = cx.transform(borderColor);
		 
		if (drawBackground)
			backgroundColor = cx.transform(backgroundColor);
		
#ifdef GNASH_DEBUG_TEXTFIELDS
	log_debug("rendering a Pol composed by corners %s", _bounds);
#endif

		render::draw_poly( &coords[0], 4, backgroundColor, borderColor, wmat, true);
		
	}

	// Draw our actual text.
	// Using a matrix to translate to def bounds seems an hack to me.
	// A cleaner implementation is likely correctly setting the
	// m_x_offset and m_y_offset memebers in glyph records.
	// Anyway, see bug #17954 for a testcase.
	matrix m;

	if ( _bounds.isFinite() ) // ! def_bounds.is_null() && ! def_bounds.is_world() 
	{
		m.concatenate_translation(_bounds.getMinX(), _bounds.getMinY()); // def_bounds.get_x_min(), def_bounds.get_y_min()
	}
	
	
	//log_debug("Displaying glyph records for textfield %s", getTarget());
	display_glyph_records(m, this, m_text_glyph_records,
			      m_def->get_root_def(), _embedFonts);

	if (m_has_focus)
	{
		show_cursor(wmat);
	}
	
	clear_invalidated();
}


void
edit_text_character::add_invalidated_bounds(InvalidatedRanges& ranges, 
	bool force)
{
	if (!force && !m_invalidated) return; // no need to redraw
    
	ranges.add(m_old_invalidated_ranges);

	matrix wm = get_world_matrix();

	geometry::Range2d<float> bounds = getBounds();
	bounds.expandTo(m_text_bounding_box); // add text bounds
	wm.transform(bounds);
	ranges.add(bounds);            
}

bool
edit_text_character::on_event(const event_id& id)
{
	if ( isReadOnly() ) 
	{
		return false;
	}

	switch (id.m_id)
	{
		case event_id::SETFOCUS:
		{
			setFocus();
			break;
		}

		case event_id::KILLFOCUS:
		{
			killFocus();
			break;
		}

		case event_id::KEY_PRESS:
		{
			if ( getType() != typeInput ) break; // not an input field
			std::wstring s = _text;

			// id.keyCode is the unique gnash::key::code for a character/key.
			// The maximum value is about 265, including function keys.
			// It seems that typing in characters outside the Latin-1 set
			// (256 character codes, identical to the first 256 of UTF-8)
			// is not supported, though a much greater number UTF-8 codes can be
			// stored and displayed. See utf.h for more information.
			// This is a limit on the number of key codes, not on the
			// capacity of strings.
			gnash::key::code c = id.keyCode;

			// maybe _text is changed in ActionScript
			m_cursor = std::min<size_t>(m_cursor, _text.size());

			switch (c)
			{
				case key::BACKSPACE:
					if (m_cursor > 0)
					{
						s.erase(m_cursor - 1, 1);
						m_cursor--;
						setTextValue(s);
					}
					break;

				case key::DELETEKEY:
					if (s.size() > m_cursor)
					{
						s.erase(m_cursor, 1);
						setTextValue(s);
					}
					break;

				case key::INSERT:		// TODO
					break;

				case key::HOME:
				case key::PGUP:
				case key::UP:
					m_cursor = 0;
					format_text();
					break;

				case key::END:
				case key::PGDN:
				case key::DOWN:
					m_cursor = _text.size();
					format_text();
					break;

				case key::LEFT:
					m_cursor = m_cursor > 0 ? m_cursor - 1 : 0;
					format_text();
					break;

				case key::RIGHT:
					m_cursor = m_cursor < _text.size() ? m_cursor + 1 : _text.size();
					format_text();
					break;

				default:
					wchar_t t = static_cast<wchar_t>(gnash::key::codeMap[c][key::ASCII]);
					if (t != 0)
					{
						// Insert one copy of the character
						// at the cursor position.
				  		s.insert(m_cursor, 1, t);
						m_cursor++;
					}
					setTextValue(s);
					break;
			}
			onChanged();
		}

		default:
			return false;
	}
	return true;
}

character*
edit_text_character::get_topmost_mouse_entity(float x, float y)
{
	//log_debug("get_topmost_mouse_entity called on edit_text_character %p, labeled '%s'", (void*)this, get_text_value());

	if (get_visible() == false)
	{
		return NULL;
	}
	
	// shouldn't this be !can_handle_mouse_event() instead ?
	if (!_selectable)
	{
		// not selectable, so don't catch mouse events!
		return NULL;
	}

	matrix	m = get_matrix();
		
	point	p;
	m.transform_by_inverse(&p, point(x, y));

	//const rect& def_bounds = m_def->get_bounds();
	//if (def_bounds.point_test(p.x, p.y))
	if ( _bounds.contains(p.x, p.y) )
	{
		return this;
	}
	return NULL;
}

void
edit_text_character::updateText(const std::string& str)
{
	int version = VM::get().getSWFVersion();
	const std::wstring& wstr = utf8::decodeCanonicalString(str, version);
	updateText(wstr);
}

void
edit_text_character::updateText(const std::wstring& wstr)
{
	_textDefined=true;

	unsigned int maxLen = m_def->get_max_length();

	std::wstring newText = wstr; // copy needed for eventual resize
	if (maxLen && newText.length() > maxLen )
	{
		newText.resize(maxLen);
	}

	if (_text == newText)
	{
		return;
	}

	set_invalidated();

	_text = newText;

	format_text();

	//log_debug(_("Text set to %s"), new_text);

}

void
edit_text_character::setTextValue(const std::wstring& wstr)
{

	updateText(wstr);

	if ( ! _variable_name.empty() && _text_variable_registered )
	{
		// TODO: notify sprite_instance if we have a variable name !
		VariableRef ref = parseTextVariableRef(_variable_name);
		as_object* tgt = ref.first;
		if ( tgt )
		{
			int version = VM::get().getSWFVersion();
			// we shouldn't truncate, right?
			tgt->set_member(ref.second, utf8::encodeCanonicalString(wstr, version)); 
		}
		else	
		{
			// nothing to do (too early ?)
			log_debug("setTextValue: variable name %s points to an unexisting target, I guess we would not be registered in this was true, or the sprite we've registered our variable name has been unloaded", _variable_name);
		}
	}
}

std::string
edit_text_character::get_text_value() const
{
	// we need the const_cast here because registerTextVariable
	// *might* change our text value, calling the non-const
	// setTextValue().
	// This happens if the TextVariable has not been already registered
	// and during registration comes out to name an existing variable
	// with a pre-existing value.
	const_cast<edit_text_character*>(this)->registerTextVariable();

	int version = VM::get().getSWFVersion();

	return utf8::encodeCanonicalString(_text, version);
}

bool
edit_text_character::set_member(string_table::key name,
		const as_value& val, string_table::key nsname, bool ifFound)
{
	//log_debug("edit_text_character.set_member(%s, %s)", name, val.to_string());

	// FIXME: Turn all standard members into getter/setter properties
	//        of the TextField class. See attachTextFieldInterface()
	// @@ TODO need to inherit basic stuff like _x, _y, _xscale, _yscale etc ?

	switch (name)
	{
	default:
		break;
	case NSV::PROP_TEXT:
		//if (name == "text")
	{
		int version = get_parent()->get_movie_definition()->get_version();
		setTextValue(utf8::decodeCanonicalString(val.to_string_versioned(version), version));
		return true;
	}
	case NSV::PROP_HTML_TEXT:
		//if (name == "htmlText")
	{
		int version = get_parent()->get_movie_definition()->get_version();
		setTextValue(utf8::decodeCanonicalString(val.to_string_versioned(version), version));
		format_text();
		return true;
	}
	case NSV::PROP_uX:
		//else if (name == "_x")
	{
		matrix	m = get_matrix();
		m.m_[0][2] = utility::infinite_to_fzero(PIXELS_TO_TWIPS(val.to_number()));	
		set_matrix(m);

		// m_accept_anim_moves = false;
		
		return true;
	}
	case NSV::PROP_uY:
		//else if (name == "_y")
	{
		matrix	m = get_matrix();
		m.m_[1][2] = utility::infinite_to_fzero(PIXELS_TO_TWIPS(val.to_number()));
		set_matrix(m);

		// m_accept_anim_moves = false;
		
		return true;
	}
	case NSV::PROP_uWIDTH:
	{
		float nw = PIXELS_TO_TWIPS(val.to_number()); // TODO: pass an as_environment !
		if ( ! finite(nw) )
		{
			// might be our fault, see the TODO above (missing to pass as_environment out..)
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Attempt to set TextField._width to %g"), nw);
			);
			return true;
		}

		if ( nw < 0.0f )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Attempt to set TextField._width to a negative number: %g, toggling sign"), nw);
			);
			nw = -nw;
		}

		if ( _bounds.width() == nw )
		{
#ifdef GNASH_DEBUG_TEXTFIELDS
			log_debug("TextField width already == %g, nothing to do to change it", nw);
#endif
			return true; // nothing to do
		}
		if ( ! _bounds.isFinite() )
		{
#ifdef GNASH_DEBUG_TEXTFIELDS
			log_debug("Non-finite TextField bounds : %s", _bounds);
#endif
			return true;
		}

#ifdef GNASH_DEBUG_TEXTFIELDS
		log_debug("Chaging TextField width to %g", nw);
#endif

		set_invalidated();

		// Modify TextField drawing rectangle
		// TODO: check which anchor point we should use !
		float xmin = _bounds.getMinX();
		float ymin = _bounds.getMinY();
		float ymax = _bounds.getMaxY();
		float xmax = xmin + nw;

		assert(xmin <= xmax);

		_bounds.setTo(xmin, ymin, xmax, ymax);
		
		assert( _bounds.width() - nw < 0.001 && _bounds.width() - nw > -0.001);

		// previously truncated text might get visible now
		// TODO: if nested masks were implemented we would 
		// not need to reformat text here
		format_text();

		return true;
	}
	case NSV::PROP_uHEIGHT:
	{
		float nh = PIXELS_TO_TWIPS(val.to_number()); // TODO: pass an as_environment !
		if ( ! finite(nh) )
		{
			// might be our fault, see the TODO above (missing to pass as_environment out..)
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Attempt to set TextField._height to %g"), nh);
			);
			return true;
		}

		if ( nh < 0.0f )
		{
			IF_VERBOSE_ASCODING_ERRORS(
			log_aserror(_("Attempt to set TextField._height to a negative number: %g, toggling sign"), nh);
			);
			nh = -nh;
		}

		if ( _bounds.height() == nh )
		{
#ifdef GNASH_DEBUG_TEXTFIELDS
			log_debug("TextField height already == %g, nothing to do to change it", nh);
#endif // GNASH_DEBUG_TEXTFIELDS
			return true; // nothing to do
		}
		if ( ! _bounds.isFinite() )
		{
#ifdef GNASH_DEBUG_TEXTFIELDS
			log_debug("Non-finite TextField bounds : %s", _bounds);
#endif // GNASH_DEBUG_TEXTFIELDS
			return true;
		}

#ifdef GNASH_DEBUG_TEXTFIELDS
		log_debug("Changing TextField height to %g", nh);
#endif // GNASH_DEBUG_TEXTFIELDS
		set_invalidated();

		// Modify TextField drawing rectangle
		// TODO: check which anchor point we should use !
		float xmin = _bounds.getMinX();
		float xmax = _bounds.getMaxX();
		float ymin = _bounds.getMinY();
		_bounds.setTo(xmin, ymin, xmax, ymin+nh);

		assert(_bounds.height() == nh);

		// previously truncated text might get visible now
		// TODO: if nested masks were implemented we would 
		// not need to reformat text here
		format_text();

		return true;
	}
	case NSV::PROP_uVISIBLE:
		//else if (name == "_visible")
	{
		set_visible(val.to_bool());
		return true;
	}
	case NSV::PROP_uALPHA:
		//else if (name == "_alpha")
	{
		// @@ TODO this should be generic to class character!
		// Arg is in percent.
		cxform	cx = get_cxform();
		cx.aa = (boost::int16_t)(val.to_number() * 2.56);
		set_cxform(cx);
		return true;
	}
	// @@ TODO see TextField members in Flash MX docs
	}	// end switch


	return set_member_default(name, val, nsname, ifFound);
}

bool
edit_text_character::get_member(string_table::key name, as_value* val,
	string_table::key nsname)
{
	//log_debug("edit_text_character.get_member(%s)", name);

	// FIXME: Turn all standard members into getter/setter properties
	//        of the TextField class. See attachTextFieldInterface()

	switch (name)
	{
	default:
		break;
	case NSV::PROP_TEXT:
		//if (name == "text")
	{
		val->set_string(get_text_value());
		return true;
	}
	case NSV::PROP_HTML_TEXT:
		//if (name == "htmlText")
	{
		val->set_string(get_text_value());
		return true;
	}
	case NSV::PROP_uVISIBLE:
		//else if (name == "_visible")
	{
		val->set_bool(get_visible());
		return true;
	}
	case NSV::PROP_uALPHA:
		//else if (name == "_alpha")
	{
		// @@ TODO this should be generic to class character!
		const cxform&	cx = get_cxform();
		val->set_double(cx.aa / 2.56);
		return true;
	}
	case NSV::PROP_uX:
		//else if (name == "_x")
	{
		matrix	m = get_matrix();	// @@ get_world_matrix()???
		val->set_double(TWIPS_TO_PIXELS(m.m_[0][2]));
		return true;
	}
	case NSV::PROP_uY:
		//else if (name == "_y")
	{
		matrix	m = get_matrix();	// @@ get_world_matrix()???
		val->set_double(TWIPS_TO_PIXELS(m.m_[1][2]));
		return true;
	}
	case NSV::PROP_uWIDTH:
	{
		val->set_double(TWIPS_TO_PIXELS(get_width()));
#ifdef GNASH_DEBUG_TEXTFIELDS
		log_debug("Got TextField width == %s", *val);
#endif // GNASH_DEBUG_TEXTFIELDS
		return true;
	}
	case NSV::PROP_uHEIGHT:
	{
		val->set_double(TWIPS_TO_PIXELS(get_height()));
#ifdef GNASH_DEBUG_TEXTFIELDS
		log_debug("Got TextField height == %s", *val);
#endif // GNASH_DEBUG_TEXTFIELDS
		return true;
	}
	}	// end switch

	return get_member_default(name, val, nsname);
	
}
	

float
edit_text_character::align_line(
		edit_text_character_def::alignment align,
		int last_line_start_record, float x)
{
	//GNASH_REPORT_FUNCTION;
	assert(m_def);

	float width = _bounds.width(); // m_def->width()
	float right_margin = getRightMargin();

	float	extra_space = (width - right_margin) - x - PADDING_TWIPS;

	//assert(extra_space >= 0.0f);
	if (extra_space <= 0.0f)
	{
#ifdef GNASH_DEBUG_TEXTFIELDS
		log_debug(_("TextField text doesn't fit in its boundaries: "
			    "width %g, margin %g - nothing to align"),
			    width, right_margin);
#endif
		return 0.0f;
	}

	float	shift_right = 0.0f;

	if (align == edit_text_character_def::ALIGN_LEFT)
	{
		// Nothing to do; already aligned left.
		return 0.0f;
	}
	else if (align == edit_text_character_def::ALIGN_CENTER)
	{
		// Distribute the space evenly on both sides.
		shift_right = extra_space / 2;
	}
	else if (align == edit_text_character_def::ALIGN_RIGHT)
	{
		// Shift all the way to the right.
		shift_right = extra_space;
	}

	// Shift the beginnings of the records on this line.
	for (unsigned int i = last_line_start_record; i < m_text_glyph_records.size(); i++)
	{
		text_glyph_record&	rec = m_text_glyph_records[i];

		if ( rec.m_style.hasXOffset() )
		{
			rec.m_style.shiftXOffset(shift_right); 
		}
	}
	return shift_right;
}

boost::intrusive_ptr<const font>
edit_text_character::setFont(boost::intrusive_ptr<const font> newfont)
{
	if ( newfont == _font ) return _font;

	boost::intrusive_ptr<const font> oldfont = _font;
	set_invalidated();
	_font = newfont; 
	format_text();
	return oldfont;  
}

void
edit_text_character::format_text()
{
	m_text_glyph_records.clear();

	// nothing more to do if text is empty
	if ( _text.empty() ) return;

	Range2d<float> defBounds = m_def->get_bounds().getRange();

	AutoSizeValue autoSize = getAutoSize();
	if ( autoSize != autoSizeNone )
	{
		LOG_ONCE( log_debug(_("TextField.autoSize != 'none' TESTING")) );

		_bounds.setTo(0,0,defBounds.getMaxX(),0); // this is correct for 'true'
		//_bounds.setNull();
	}

	// Should get info from autoSize too maybe ?
	edit_text_character_def::alignment textAlignment = getTextAlignment();

	// nothing more to do if text is empty
	if ( _text.empty() ) return;

	// FIXME: I don't think we should query the definition
	// to find the appropriate font to use, as ActionScript
	// code should be able to change the font of a TextField
	//
	if (_font == NULL)
	{
		log_error(_("No font for edit_text_character! [%s:%d]"),
			__FILE__, __LINE__);
		return;
	}

	boost::uint16_t fontHeight = getFontHeight();
	float scale = fontHeight / (float)_font->unitsPerEM(_embedFonts); 
	float fontDescent = _embedFonts ? (_font->get_descent()*scale) : 0; // TODO: fetch descent from device fonts as well ?
	float fontLeading = _embedFonts ? (_font->get_leading()*scale) : 0;  // TODO: fetch leading from device fonts as well ?
	boost::uint16_t leftMargin = getLeftMargin();
	boost::uint16_t rightMargin = getRightMargin();
	boost::uint16_t indent = getIndent();
	boost::uint16_t blockIndent = getBlockIndent();
	bool underlined = getUnderlined();

	//log_debug("%s: fontDescent:%g, fontLeading:%g, fontHeight:%g, scale:%g", getTarget(), fontDescent, fontLeading, fontHeight, scale);

	text_glyph_record rec;	// one to work on
	rec.m_style.setFont(_font.get());
	rec.m_style.setUnderlined(underlined);
	rec.m_style.m_color = getTextColor(); 
	rec.m_style.setXOffset( PADDING_TWIPS + std::max(0, leftMargin + indent + blockIndent) );
	rec.m_style.setYOffset( PADDING_TWIPS + fontHeight + (fontLeading - fontDescent) );
	rec.m_style.m_text_height = fontHeight;

	float	x = rec.m_style.getXOffset();
	float	y = rec.m_style.getYOffset();

	// Start the bbox at the upper-left corner of the first glyph.
	reset_bounding_box(x, y - fontDescent + fontHeight); 

	float leading = getLeading();
	leading += fontLeading * scale; // not sure this is correct...

	int	last_code = -1; // only used if _embedFonts
	int	last_space_glyph = -1;
	int	last_line_start_record = 0;

	unsigned int character_idx = 0;
	m_xcursor = x;
	m_ycursor = y;

	assert(! _text.empty() );
	
	boost::uint32_t code = 0;
    std::wstring::const_iterator it = _text.begin();
    const std::wstring::const_iterator e = _text.end();

	while (it != e)
	{
	    code = *it++;
	    if (!code) break;

		if ( _embedFonts )
		{
			x += _font->get_kerning_adjustment(last_code, (int) code) * scale;
			last_code = static_cast<int>(code);
		}

		// Expand the bounding-box to the lower-right corner of each glyph as
		// we generate it.
		m_text_bounding_box.expandTo(x, y + fontDescent);

		if (code == 13 || code == 10)
		{
			// newline.

			// Frigging Flash seems to use '\r' (13) as its
			// default newline character.  If we get DOS-style \r\n
			// sequences, it'll show up as double newlines, so maybe we
			// need to detect \r\n and treat it as one newline.

			// Close out this stretch of glyphs.
			m_text_glyph_records.push_back(rec);
			align_line(textAlignment, last_line_start_record, x);

			// Expand bounding box to include last column of text ...
			if ( _autoSize != autoSizeNone ) _bounds.expandTo(x+PADDING_TWIPS, y+PADDING_TWIPS);

			// new paragraphs get the indent.
			x = std::max(0, leftMargin + indent) + PADDING_TWIPS;
			y += fontHeight + leading; 

			// Start a new record on the next line.
			rec.m_glyphs.resize(0);
			rec.m_style.setFont(_font.get()); 
			rec.m_style.setUnderlined(underlined);
			rec.m_style.m_color = getTextColor();
			rec.m_style.setXOffset(x);
			rec.m_style.setYOffset(y);
			rec.m_style.m_text_height = fontHeight; 

			last_space_glyph = -1;
			last_line_start_record = m_text_glyph_records.size();

			continue;
		}

		if (code == 8)
		{
			// backspace (ASCII BS).

			// This is a limited hack to enable overstrike effects.
			// It backs the cursor up by one character and then continues
			// the layout.  E.g. you can use this to display an underline
			// cursor inside a simulated text-entry box.
			//
			// ActionScript understands the '\b' escape sequence
			// for inserting a BS character.
			//
			// ONLY WORKS FOR BACKSPACING OVER ONE CHARACTER, WON'T BS
			// OVER NEWLINES, ETC.

			if (rec.m_glyphs.size() > 0)
			{
				// Peek at the previous glyph, and zero out its advance
				// value, so the next char overwrites it.
				float	advance = rec.m_glyphs.back().m_glyph_advance;
				x -= advance;	// maintain formatting
				rec.m_glyphs.back().m_glyph_advance = 0;	// do the BS effect
			}
			continue;
		}

		if (code == '<' && _html )
		{
			LOG_ONCE(log_debug(_("HTML in a text field is unsupported, "
                                 "gnash will just forget the tags and print their content")));
 
            std::wstring discard;
		    bool complete = parseHTML(discard, it, e);
		    
		    //log_debug("HTML tag: %s", utf8::encodeCanonicalString(discard, 7));
	    
		    // Check incomplete tag (end of string or NULL character in the text).
		    // We should stop parsing and not increment the iterator in this case.
		    if (!complete) break;
		    
		    continue;
		}

		if (code == 9) // tab (ASCII HT)
		{
			int index = _font->get_glyph_index(32, _embedFonts); // ascii SPACE
			if ( index == -1 )
			{
				IF_VERBOSE_MALFORMED_SWF (
				  log_error(_("%s -- missing glyph for space char (needed for TAB). "
					    " Make sure character shapes for font %s are being exported "
					    "into your SWF file."),
					    __PRETTY_FUNCTION__,
					    _font->get_name());
				);
			}
			else
			{
				text_glyph_record::glyph_entry	ge;
				ge.m_glyph_index = index;
				ge.m_glyph_advance = scale * _font->get_advance(index, _embedFonts);

				const int tabstop=8;
				rec.m_glyphs.insert(rec.m_glyphs.end(), tabstop, ge);
				x += ge.m_glyph_advance*tabstop;
			}
			goto after_x_advance;
		}

		// Remember where word breaks occur.
		if (code == 32)
		{
			last_space_glyph = rec.m_glyphs.size();
		}

		{
		// need a sub-scope to avoid the 'goto' in TAB handling to cross
		// initialization of the 'index' variable

		// The font table holds up to 65535 glyphs. Casting from uint32_t
		// would, in the event that the code is higher than 65535, result
		// in the wrong character being chosen. It isn't clear whether this
		// would ever happen, but UTF-8 conversion code can deal with codes
		// up to 2^32; if they are valid, the code table will have to be
		// enlarged.
		int index = _font->get_glyph_index(static_cast<boost::uint16_t>(code), _embedFonts);

		IF_VERBOSE_MALFORMED_SWF (
		    if (index == -1)
		    {
			    // error -- missing glyph!
			    
			    // Log an error, but don't log too many times.
			    static int	s_log_count = 0;
			    if (s_log_count < 10)
			    {
				    s_log_count++;
		
					if ( _embedFonts )
					{
				    log_swferror(_("%s -- missing embedded glyph for char %d. "
						" Make sure character shapes for font %s are being exported "
						"into your SWF file"),
						__PRETTY_FUNCTION__,
						code,
						_font->get_name());
					}
					else
					{
				    log_swferror(_("%s -- missing device glyph for char %d. "
						" Maybe you don't have font '%s' installed in your system?"),
						__PRETTY_FUNCTION__,
						code,
						_font->get_name());
					}
			    }

			    // Drop through and use index == -1; this will display
			    // using the empty-box glyph
		    }
		); // IF_VERBOSE_MALFORMED_SWF

		text_glyph_record::glyph_entry	ge;
		ge.m_glyph_index = index;
		ge.m_glyph_advance = scale * _font->get_advance(index, _embedFonts);

		rec.m_glyphs.push_back(ge);

		x += ge.m_glyph_advance;
		}
		
after_x_advance:

		float width = defBounds.width();
		if (x >= width - rightMargin - PADDING_TWIPS)
		{
#ifdef GNASH_DEBUG_TEXT_FORMATTING
			log_debug("Text in TextField %s exceeds width [ _bounds %s ]", getTarget(), _bounds);
#endif

			// no wrap and no resize --> truncate
			if ( ! doWordWrap() && autoSize == autoSizeNone )
			{
#ifdef GNASH_DEBUG_TEXT_FORMATTING
				log_debug(" wordWrap=false, autoSize=none");
#endif 
				// truncate long line, but keep expanding text box
				bool newlinefound = false;
				while ( it != e )
				{
				    code = *it++;
					if ( _embedFonts )
					{
						x += _font->get_kerning_adjustment(last_code, (int) code) * scale;
						last_code = static_cast<int>(code);
					}
					// Expand the bounding-box to the lower-right corner of each glyph,
					// even if we don't display it 
					m_text_bounding_box.expandTo(x, y + fontDescent);
#ifdef GNASH_DEBUG_TEXT_FORMATTING
					log_debug("Text bbox expanded to %s (width: %f)", m_text_bounding_box, m_text_bounding_box.width());
#endif

					if (code == 13 || code == 10)
					{
						newlinefound = true;
						break;
					}

					int index = _font->get_glyph_index((boost::uint16_t) code, _embedFonts);
					x += scale * _font->get_advance(index, _embedFonts);

				}
				if ( ! newlinefound ) break;
			}
			else if ( doWordWrap() )
			{
#ifdef GNASH_DEBUG_TEXT_FORMATTING
				log_debug(" wordWrap=true");
#endif // DEBUG_MOUSE_ENTITY_FINDING

				// Insert newline if there's space or autosize != none

				// Close out this stretch of glyphs.
				m_text_glyph_records.push_back(rec);

				float	previous_x = x;
				x = leftMargin + blockIndent + PADDING_TWIPS;
				y += fontHeight + leading;

				// Start a new record on the next line.
				rec.m_glyphs.resize(0);
				rec.m_style.setFont(_font.get());
				rec.m_style.setUnderlined(underlined);
				rec.m_style.m_color = getTextColor();
				rec.m_style.setXOffset(x);
				rec.m_style.setYOffset(y);
				rec.m_style.m_text_height = getFontHeight();

				// TODO : what if m_text_glyph_records is empty ? Is it possible ?
				assert(!m_text_glyph_records.empty());
				text_glyph_record&	last_line = m_text_glyph_records.back();
				if (last_space_glyph == -1)
				{
					// Pull the previous glyph down onto the
					// new line.
					if (last_line.m_glyphs.size() > 0)
					{
						rec.m_glyphs.push_back(last_line.m_glyphs.back());
						x += last_line.m_glyphs.back().m_glyph_advance;
						previous_x -= last_line.m_glyphs.back().m_glyph_advance;
						last_line.m_glyphs.resize(last_line.m_glyphs.size() - 1);
					}
				}
				else
				{
					// Move the previous word down onto the next line.

					previous_x -= last_line.m_glyphs[last_space_glyph].m_glyph_advance;

					for (unsigned int i = last_space_glyph + 1; i < last_line.m_glyphs.size(); i++)
					{
						rec.m_glyphs.push_back(last_line.m_glyphs[i]);
						x += last_line.m_glyphs[i].m_glyph_advance;
						previous_x -= last_line.m_glyphs[i].m_glyph_advance;
					}
					last_line.m_glyphs.resize(last_space_glyph);
				}

				align_line(textAlignment, last_line_start_record, previous_x);

				last_space_glyph = -1;
				last_line_start_record = m_text_glyph_records.size();
				
			}
			else
			{
#ifdef GNASH_DEBUG_TEXT_FORMATTING
				log_debug(" wordWrap=%d, autoSize=%d", _wordWrap, _autoSize);
#endif // DEBUG_MOUSE_ENTITY_FINDING
			}
		}


		if ( (y) > (defBounds.height() - PADDING_TWIPS) && autoSize == autoSizeNone )
		{
#ifdef GNASH_DEBUG_TEXT_FORMATTING
			log_debug("Text with wordWrap exceeds height of box");
#endif
			rec.m_glyphs.clear();
			// TODO: should still compute m_text_bounds !
			LOG_ONCE(log_unimpl("Computing text bounds of a TextField containing text that doesn't fit the box vertically"));
			break;
		}


		if (m_cursor > character_idx)
		{
			m_xcursor = x;
			m_ycursor = y;
		}
		character_idx++;

		// TODO: HTML markup
	}

	// Expand bounding box to include the whole text (if autoSize)
	if ( _autoSize != autoSizeNone )
	{
		_bounds.expandTo(x+PADDING_TWIPS, y+PADDING_TWIPS);
	}

	// Add this line to our output.
	if ( ! rec.m_glyphs.empty() ) m_text_glyph_records.push_back(rec);

	float extra_space = align_line(textAlignment, last_line_start_record, x);

	m_xcursor += static_cast<int>(extra_space);
	m_ycursor -= fontHeight + (fontLeading - fontDescent);
}

edit_text_character::VariableRef
edit_text_character::parseTextVariableRef(const std::string& variableName) const
{
	VariableRef ret;
	ret.first = 0;

	std::string var_str = PROPNAME(variableName);

	const char* varname = var_str.c_str();

#ifdef DEBUG_DYNTEXT_VARIABLES
	log_debug(_("VariableName: %s"), var_str);
#endif

	/// Why isn't get_environment const again ?
	as_environment& env = const_cast<edit_text_character*>(this)->get_environment();

	as_object* target = env.get_target();
	assert(target); // is this correct ?

	// If the variable string contains a path, we extract
	// the appropriate target from it and update the variable
	// name
	std::string path, var;
	if ( as_environment::parse_path(varname, path, var) )
	{
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug(_("Variable text Path: %s, Var: %s"), path, var);
#endif
		// find target for the path component
		// we use our parent's environment for this
		target = env.find_object(path);

		// update varname (with path component stripped)
		varname = var.c_str();
	}

	if ( ! target )
	{
		IF_VERBOSE_MALFORMED_SWF(
			log_swferror(_("VariableName associated to text field refer to an unknown target (%s). It is possible that the character will be instantiated later in the SWF stream. Gnash will try to register again on next access."), path);
		);
		return ret;
	}

	ret.first = target;
	ret.second = _vm.getStringTable().find(varname);

	return ret;
}

void
edit_text_character::registerTextVariable() 
{
//#define DEBUG_DYNTEXT_VARIABLES 1

#ifdef DEBUG_DYNTEXT_VARIABLES
	log_debug(_("registerTextVariable() called"));
#endif

	if ( _text_variable_registered )
	{
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug(_("registerTextVariable() no-op call (already registered)"));
#endif
		return;
	}

	if ( _variable_name.empty() )
	{
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug(_("string is empty, consider as registered"));
#endif
		_text_variable_registered=true;
		return;
	}

	VariableRef varRef = parseTextVariableRef(_variable_name);
	as_object* target = varRef.first;
	if ( ! target )
	{
		log_debug(_("VariableName associated to text field (%s) refer to an unknown target. "
				"It is possible that the character will be instantiated later in the SWF stream. "
				"Gnash will try to register again on next access."), _variable_name);
		return;
	}

	string_table::key key = varRef.second;

	// check if the VariableName already has a value,
	// in that case update text value
	as_value val;
	
	int version = VM::get().getSWFVersion();
	
	if (target->get_member(key, &val) )
	{
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug(_("target object (%s @ %p) does have a member named %s"),
			typeName(*target), (void*)target, _vm.getStringTable().value(key));
#endif
		// TODO: pass environment to to_string ?
		// as_environment& env = get_environment();
		setTextValue(utf8::decodeCanonicalString(val.to_string(), version));
	}
	else if ( _textDefined )
	{
		as_value newVal = as_value(utf8::encodeCanonicalString(_text, version));
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug(_("target sprite (%s @ %p) does NOT have a member named %s (no problem, we'll add it with value %s)"),
			typeName(*target), (void*)target, _vm.getStringTable().value(key),
			newVal);
#endif
		target->set_member(key, newVal);
	}
	else
	{
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug(_("target sprite (%s @ %p) does NOT have a member named %s, and we don't have text defined"),
			typeName(*target), (void*)target, _vm.getStringTable().value(key));
#endif
	}

	sprite_instance* sprite = target->to_movie();

	if ( sprite )
	{
		// add the textfield variable to the target sprite
		// TODO: have set_textfield_variable take a string_table::key instead ?
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug("Calling set_textfield_variable(%s) against sprite %s", _vm.getStringTable().value(key), sprite->getTarget());
#endif
		sprite->set_textfield_variable(_vm.getStringTable().value(key), this);

	}
	_text_variable_registered=true;

}

/// Parses an HTML tag (between < and >) and puts
/// the contents into tag. Returns false if the
/// tag was incomplete. The iterator is moved to after
/// the closing tag or the end of the string.
bool
edit_text_character::parseHTML(std::wstring& tag, std::wstring::const_iterator& it,
	                           const std::wstring::const_iterator& e) const
{

    bool complete = false;

    while (it != e)
    {
        if (*it == '>')
        {
            ++it;
            complete = true;
            break;
        }

        // Check for NULL character
        if (*it == 0) break;

        tag.push_back(*it++);
    }
    
#ifdef GNASH_DEBUG_TEXTFIELDS
    log_debug ("HTML tag: %s", utf8::encodeCanonicalString(tag, 7));
#endif
    
    return complete;
}

void
edit_text_character::set_variable_name(const std::string& newname)
{
	if ( newname != _variable_name )
	{
		_variable_name = newname;
		_text_variable_registered = false;
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug("Calling updateText after change of variable name");
#endif
		updateText(m_def->get_default_text());
#ifdef DEBUG_DYNTEXT_VARIABLES
		log_debug("Calling registerTextVariable after change of variable name and updateText call");
#endif
		registerTextVariable();
		//reset_bounding_box(0, 0); // does this make sense ? it's called in the constructor...
	}
}

void
textfield_class_init(as_object& global)
{
	// This is going to be the global TextField "class"/"function"
	static boost::intrusive_ptr<builtin_function> cl=NULL;

	if ( cl == NULL )
	{
		VM& vm = VM::get();

		as_object* iface = getTextFieldInterface();
		cl=new builtin_function(&textfield_ctor, iface);
#ifndef NDEBUG
		int swfVer = vm.getSWFVersion();
		if ( swfVer > 5 )
		{
			assert(iface);
			assert(cl->getOwnProperty(vm.getStringTable().find("prototype")));
		}
		else
		{
			assert(!iface);
			assert(!cl->getOwnProperty(vm.getStringTable().find("prototype")));
		}
#endif

		vm.addStatic(cl.get());

		// replicate all interface to class, to be able to access
		// all methods as static functions
		attachTextFieldStaticMembers(*cl);
		     
	}

	// Register _global.MovieClip
	global.init_member("TextField", cl.get());
}

bool
edit_text_character::pointInShape(float x, float y) const
{
	matrix wm = get_world_matrix();
	point lp(x, y);
	wm.transform_by_inverse(lp);
	return _bounds.contains(lp.x, lp.y);
}

bool
edit_text_character::getDrawBorder() const
{
	return _drawBorder;
}

void
edit_text_character::setDrawBorder(bool val) 
{
	if ( _drawBorder != val )
	{
		set_invalidated();
		_drawBorder = val;
	}
}

rgba
edit_text_character::getBorderColor() const
{
	return _borderColor;
}

void
edit_text_character::setBorderColor(const rgba& col)
{
	if ( _borderColor != col )
	{
		set_invalidated();
		_borderColor = col;
	}
}

bool
edit_text_character::getDrawBackground() const
{
	return _drawBackground;
}

void
edit_text_character::setDrawBackground(bool val) 
{
	if ( _drawBackground != val )
	{
		set_invalidated();
		_drawBackground = val;
	}
}

rgba
edit_text_character::getBackgroundColor() const
{
	return _backgroundColor;
}

void
edit_text_character::setBackgroundColor(const rgba& col)
{
	if ( _backgroundColor != col )
	{
		set_invalidated();
		_backgroundColor = col;
	}
}

void
edit_text_character::setTextColor(const rgba& col)
{
	if ( _textColor != col )
	{
		set_invalidated();

		_textColor = col;

		// Change color of all current glyph records
		for (TextGlyphRecords::iterator i=m_text_glyph_records.begin(),
			e=m_text_glyph_records.end(); i!=e; ++i)
		{
		 	text_glyph_record& rec=*i;
			rec.m_style.m_color = _textColor;
		}

	}
}

void
edit_text_character::setEmbedFonts(bool use)
{
	if ( _embedFonts != use )
	{
		set_invalidated();
		_embedFonts=use;
		format_text();
	}
}

void
edit_text_character::setWordWrap(bool on)
{
	if ( _wordWrap != on )
	{
		set_invalidated();
		_wordWrap=on;
		format_text();
	}
}

cxform	
edit_text_character::get_world_cxform() const
{
  cxform cf = character::get_world_cxform();
  
#ifdef PP_COMPATIBLE_DEVICE_FONT_HANDLING
  if ( ! getEmbedFonts() ) /* if using a device font (PP compatibility) */ 
  {
    // set alpha to default values to make the text field opaque
    cf.m_[3][0] = 1.0f;
    cf.m_[3][1] = 0.0f;
    
    // NOTE: Linux version of Adobe's player also ignores tint
    // transform, so we should (or not) return an identity cxform
    // here. This has to be discussed...
  }
#endif
  
  return cf;
}

void
edit_text_character::setLeading(boost::uint16_t h)
{
	if ( _leading != h )
	{
		set_invalidated();
		_leading = h;
		format_text();
	}
}

void
edit_text_character::setUnderlined(bool v)
{
	if ( _underlined != v )
	{
		set_invalidated();
		_underlined = v;
		format_text();
	}
}

void
edit_text_character::setAlignment(edit_text_character_def::alignment h)
{
	if ( _alignment != h )
	{
		set_invalidated();
		_alignment = h;
		format_text();
	}
}

void
edit_text_character::setIndent(boost::uint16_t h)
{
	if ( _indent != h )
	{
		set_invalidated();
		_indent = h;
		format_text();
	}
}

void
edit_text_character::setBlockIndent(boost::uint16_t h)
{
	if ( _blockIndent != h )
	{
		set_invalidated();
		_blockIndent = h;
		format_text();
	}
}

void
edit_text_character::setRightMargin(boost::uint16_t h)
{
	if ( _rightMargin != h )
	{
		set_invalidated();
		_rightMargin = h;
		format_text();
	}
}

void
edit_text_character::setLeftMargin(boost::uint16_t h)
{
	if (_leftMargin != h)
	{
		set_invalidated();
		_leftMargin = h;
		format_text();
	}
}

void
edit_text_character::setFontHeight(boost::uint16_t h)
{
	if ( _fontHeight != h )
	{
		set_invalidated();
		_fontHeight = h;
		format_text();
	}
}

static as_value
textfield_background_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->getDrawBackground());
	}
	else // setter
	{
		ptr->setDrawBackground(fn.arg(0).to_bool());
	}

	return as_value();
}

static as_value
textfield_border_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->getDrawBorder());
	}
	else // setter
	{
		ptr->setDrawBorder(fn.arg(0).to_bool());
	}

	return as_value();
}

static as_value
textfield_backgroundColor_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->getBackgroundColor().toRGB());
	}
	else // setter
	{
		rgba newColor;
		newColor.parseRGB( fn.arg(0).to_number<boost::uint32_t>() );
		ptr->setBackgroundColor(newColor);
	}

	return as_value();
}

static as_value
textfield_borderColor_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->getBorderColor().toRGB());
	}
	else // setter
	{
		rgba newColor;
		newColor.parseRGB( fn.arg(0).to_number<boost::uint32_t>() );
		ptr->setBorderColor(newColor);
	}

	return as_value();
}

static as_value
textfield_textColor_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->getTextColor().toRGB());
	}
	else // setter
	{
		rgba newColor;
		newColor.parseRGB( fn.arg(0).to_number<boost::uint32_t>() );
		ptr->setTextColor(newColor);
	}

	return as_value();
}

static as_value
textfield_embedFonts_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->getEmbedFonts());
	}
	else // setter
	{
		ptr->setEmbedFonts( fn.arg(0).to_bool() );
	}

	return as_value();
}

static as_value
textfield_wordWrap_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->doWordWrap());
	}
	else // setter
	{
		ptr->setWordWrap( fn.arg(0).to_bool() );
	}

	return as_value();
}

static as_value
textfield_html_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->doHtml());
	}
	else // setter
	{
		ptr->setHtml( fn.arg(0).to_bool() );
	}

	return as_value();
}

static as_value
textfield_selectable_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return as_value(ptr->isSelectable());
	}
	else // setter
	{
		ptr->setSelectable( fn.arg(0).to_bool() );
	}

	return as_value();
}

static as_value
textfield_length_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		const std::string& s = ptr->get_text_value();
		return as_value(s.length()); // TOCHECK: utf-8 ?
	}
	else // setter
	{
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("Attempt to set length property of TextField %s"), ptr->getTarget());
		);
	}

	return as_value();
}

static as_value
textfield_textHeight_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		// Return the height, in pixels, of the text as laid out.
		// (I.e. the actual text content, not our defined
		// bounding box.)
		//
		// In local coords.  Verified against Macromedia Flash.
		return as_value(TWIPS_TO_PIXELS(ptr->getTextBoundingBox().height()));

	}
	else // setter
	{
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("Attempt to set read-only %s property of TextField %s"), "textHeight", ptr->getTarget());
		);
	}

	return as_value();
}

static as_value
textfield_textWidth_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		// Return the width, in pixels, of the text as laid out.
		// (I.e. the actual text content, not our defined
		// bounding box.)
		//
		// In local coords.  Verified against Macromedia Flash.
		return as_value(TWIPS_TO_PIXELS(ptr->getTextBoundingBox().width()));

	}
	else // setter
	{
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("Attempt to set read-only %s property of TextField %s"), "textWidth", ptr->getTarget());
		);
	}

	return as_value();
}

static as_value
textfield_autoSize_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return ptr->autoSizeValueName(ptr->getAutoSize());
	}
	else // setter
	{
		as_value& arg = fn.arg(0);
		if ( arg.is_bool() )
		{
			if ( arg.to_bool() ) // true == left
			{
				ptr->setAutoSize( edit_text_character::autoSizeLeft );
			}
			else
			{
				ptr->setAutoSize( edit_text_character::autoSizeNone );
			}
		}
		else
		{
			std::string strval = arg.to_string();
			edit_text_character::AutoSizeValue val = ptr->parseAutoSizeValue(strval);
			//log_debug("%s => %d", strval, val);
			ptr->setAutoSize( val );
		}
	}

	return as_value();
}

static as_value
textfield_type_getset(const fn_call& fn)
{
	boost::intrusive_ptr<edit_text_character> ptr = ensureType<edit_text_character>(fn.this_ptr);

	if ( fn.nargs == 0 ) // getter
	{
		return ptr->typeValueName(ptr->getType());
	}

	// setter
	as_value& arg = fn.arg(0);
	std::string strval = arg.to_string();
	edit_text_character::TypeValue val = ptr->parseTypeValue(strval);
	//log_debug("setting %s.type : %s (toString->%s) => %d", ptr->getTarget(), arg, strval, val);
	IF_VERBOSE_ASCODING_ERRORS(
	if ( val == edit_text_character::typeInvalid )
	{
		log_aserror(_("Invalid value given to TextField.type: %s"), strval);
	}
	);
	ptr->setType(val);
	return as_value();
}

/* public static */
edit_text_character::AutoSizeValue
edit_text_character::parseAutoSizeValue(const std::string& val)
{
	StringNoCaseEqual cmp;

	if ( cmp(val, "left") )
	{
		return autoSizeLeft;
	}
	if ( cmp(val, "right") )
	{
		return autoSizeRight;
	}
	if ( cmp(val, "center") )
	{
		return autoSizeCenter;
	}
	return autoSizeNone;

}

/* public static */
const char*
edit_text_character::autoSizeValueName(AutoSizeValue val)
{
	switch (val)
	{
		case autoSizeLeft:
			return "left";
		case autoSizeRight:
			return "right";
		case autoSizeCenter:
			return "center";
		case autoSizeNone:
		default:
			return "none";
	}

}

/* public static */
edit_text_character::TypeValue
edit_text_character::parseTypeValue(const std::string& val)
{
	StringNoCaseLessThen cmp;

	if ( ! cmp(val, "input") )
	{
		//log_debug("parsing type value %s: typeInput", val);
		return typeInput;
	}
	if ( ! cmp(val, "dynamic") )
	{
		//log_debug("parsing type value %s: typeDynamic", val);
		return typeDynamic;
	}
	//log_debug("parsing type value %s: typeInvalid", val);
	return typeInvalid;

}

/* public static */
const char*
edit_text_character::typeValueName(TypeValue val)
{
	switch (val)
	{
		case typeInput:
			//log_debug("typeInput returned as 'input'");
			return "input";
		case typeDynamic:
			//log_debug("typeDynamic returned as 'dynamic'");
			return "dynamic";
		default:
			//log_debug("invalid type %d returned as 'invalid'", (int)val);
			return "invalid";
	}

}

void
edit_text_character::setAutoSize(AutoSizeValue val)
{
	if ( val == _autoSize ) return;

	set_invalidated();

	_autoSize = val; 
	format_text();
}

edit_text_character_def::alignment
edit_text_character::getTextAlignment()
{
	// TODO: use a _textAlignment private member to reduce lookups ?
	// The member would be initialized to m_def->get_alignment and then update
	// when _autoSize is updated.
	edit_text_character_def::alignment textAlignment = getAlignment(); 
	if ( _autoSize == autoSizeCenter ) textAlignment = edit_text_character_def::ALIGN_CENTER;
	else if ( _autoSize == autoSizeLeft ) textAlignment = edit_text_character_def::ALIGN_LEFT;
	else if ( _autoSize == autoSizeRight ) textAlignment = edit_text_character_def::ALIGN_RIGHT;
	return textAlignment;
}

void
edit_text_character::onChanged()
{
	as_value met(PROPNAME("onChanged"));
	as_value targetVal(this);
	callMethod(NSV::PROP_BROADCAST_MESSAGE, met, targetVal);
}

void
edit_text_character::onSetFocus()
{
	string_table& st = _vm.getStringTable();
	string_table::key key = st.find(PROPNAME("onSetFocus"));
	callMethod(key);
}

void
edit_text_character::onKillFocus()
{
	string_table& st = _vm.getStringTable();
	string_table::key key = st.find(PROPNAME("onKillFocus"));
	callMethod(key);
}

void
edit_text_character::setFocus()
{
	if ( m_has_focus ) return; // nothing to do

	set_invalidated();

	m_has_focus = true;

	// why should we add to the key listener list every time
	// we call setFocus()???
	_vm.getRoot().add_key_listener(this);

	m_cursor = _text.size();
	format_text();

	onSetFocus();
}

void
edit_text_character::killFocus()
{
	if ( ! m_has_focus ) return; // nothing to do

	set_invalidated();

	m_has_focus = false;

	movie_root& root = _vm.getRoot();
	root.setFocus(NULL);
	root.remove_key_listener(this);
	format_text(); // is this needed ?

	onKillFocus();
}

void
edit_text_character::markReachableResources() const
{
	if ( m_def.get() ) m_def->setReachable();

	if ( _font ) _font->setReachable();

	// recurse to parent...
	markCharacterReachable();
}

} // namespace gnash


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

