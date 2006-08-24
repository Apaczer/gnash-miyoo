// text.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code for the text tags.


//#include "utf8.h"
//#include "utility.h"
//#include "impl.h"
//#include "shape.h"
//#include "shape_character_def.h"
#include "stream.h"
#include "log.h"
//#include "font.h"
//#include "fontlib.h"
//#include "render.h"
//#include "textformat.h"
#include "text_character_def.h"
//#include "movie_definition.h"

namespace gnash {

void text_character_def::read(stream* in, int tag_type,
		movie_definition* m)
{
	assert(m != NULL);
	assert(tag_type == 11 || tag_type == 33);

	m_rect.read(in);
	m_matrix.read(in);

	int	glyph_bits = in->read_u8();
	int	advance_bits = in->read_u8();

	IF_VERBOSE_PARSE(
	log_parse("begin text records");
	);

	bool	last_record_was_style_change = false;

	text_style	style;
	for (;;)
	{
		int	first_byte = in->read_u8();
		
		if (first_byte == 0)
		{
			// This is the end of the text records.
			IF_VERBOSE_PARSE(
			log_parse("end text records");
			);
			break;
		}

		// Style changes and glyph records just alternate.
		// (Contrary to what most SWF references say!)
		if (last_record_was_style_change == false)
		{
			// This is a style change.

			last_record_was_style_change = true;

			bool	has_font = (first_byte >> 3) & 1;
			bool	has_color = (first_byte >> 2) & 1;
			bool	has_y_offset = (first_byte >> 1) & 1;
			bool	has_x_offset = (first_byte >> 0) & 1;

			IF_VERBOSE_PARSE(
			log_parse("  text style change");
			);

			if (has_font)
			{
				uint16_t	font_id = in->read_u16();
				style.m_font_id = font_id;
				IF_VERBOSE_PARSE(
				log_parse("  has_font: font id = %d", font_id);
				);
			}
			if (has_color)
			{
				if (tag_type == 11)
				{
					style.m_color.read_rgb(in);
				}
				else
				{
					assert(tag_type == 33);
					style.m_color.read_rgba(in);
				}
				IF_VERBOSE_PARSE(
				log_parse("  has_color");
				);
			}
			if (has_x_offset)
			{
				style.m_has_x_offset = true;
				style.m_x_offset = in->read_s16();
				IF_VERBOSE_PARSE(
				log_parse("  has_x_offset = %g", style.m_x_offset);
				);
			}
			else
			{
				style.m_has_x_offset = false;
				style.m_x_offset = 0.0f;
			}
			if (has_y_offset)
			{
				style.m_has_y_offset = true;
				style.m_y_offset = in->read_s16();
				IF_VERBOSE_PARSE(
				log_parse("  has_y_offset = %g", style.m_y_offset);
				);
			}
			else
			{
				style.m_has_y_offset = false;
				style.m_y_offset = 0.0f;
			}
			if (has_font)
			{
				style.m_text_height = in->read_u16();
				IF_VERBOSE_PARSE(
				log_parse("  text_height = %g", style.m_text_height);
				);
			}
		}
		else
		{
			// Read the glyph record.

			last_record_was_style_change = false;

			int	glyph_count = first_byte;

// 					if (! last_record_was_style_change)
// 					{
// 						glyph_count &= 0x7F;
// 					}
// 					// else { Don't mask the top bit; the first record is allowed to have > 127 glyphs. }

			m_text_glyph_records.resize(m_text_glyph_records.size() + 1);
			m_text_glyph_records.back().m_style = style;
			m_text_glyph_records.back().read(in, glyph_count, glyph_bits, advance_bits);

			IF_VERBOSE_PARSE(
			log_parse("  glyph_records: count = %d", glyph_count);
			);
		}
	}
}

void text_character_def::display(character* inst)
{
//	GNASH_REPORT_FUNCTION;
	display_glyph_records(m_matrix, inst,
		m_text_glyph_records, m_root_def);
}


}	// end namespace gnash


