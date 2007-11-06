// FreetypeGlyphsProvider.cpp:  Freetype glyphs manager
// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "FreetypeGlyphsProvider.h"
#include "smart_ptr.h" // for intrusive_ptr
#include "image.h" // for create_alpha
#include "GnashException.h"
#include "render.h"
#include "DynamicShape.h"
#include "log.h"

#ifdef USE_FREETYPE 
# include <ft2build.h>
# include FT_OUTLINE_H
# include FT_BBOX_H
// Methods of FT_Outline_Funcs take a 'const FT_Vector*' in 2.2
// and a non-const one in 2.1, so we use an FT_CONST macro to
// support both
# if FREETYPE_MAJOR == 2 && FREETYPE_MINOR < 2
#  define FT_CONST 
# else
#  define FT_CONST const
# endif
#endif

#ifdef HAVE_FONTCONFIG_FONTCONFIG_H
# define HAVE_FONTCONFIG 1
#endif

#ifdef HAVE_FONTCONFIG
# include <fontconfig/fontconfig.h>
# include <fontconfig/fcfreetype.h>
#endif

#include <cstdio> // for snprintf
#include <string>
#include <memory> // for auto_ptr

// Define the following to make outline decomposition verbose
//#define DEBUG_OUTLINE_DECOMPOSITION 1

// Define the following to make device font handling verbose
//#define GNASH_DEBUG_DEVICEFONTS 1

namespace gnash {

#ifdef USE_FREETYPE 

/// Outline glyph walker/decomposer, for drawing an FT_Outline to DynamicShape
//
/// See  FT_Outline_Decompose function of freetype2 lib
///
class OutlineWalker {

public:

	/// Create an outline walker drawing to the given DynamiShape
	//
	/// @param sh
	///	The DynamiShape to draw to. Externally owned.
	///
	/// @param scale
	///	The scale to apply to coordinates.
	///	This is to match an EM of 1024x1024 when units_per_EM
	///	are of a different value.
	///
	OutlineWalker(DynamicShape& sh, float scale)
		:
		_sh(sh),
		_scale(scale)
	{}

	~OutlineWalker() {}

	/// Callback function for the move_to member of FT_Outline_Funcs
	static int
	walkMoveTo(FT_CONST FT_Vector* to, void* ptr)
	{
		OutlineWalker* walker = static_cast<OutlineWalker*>(ptr);
		return walker->moveTo(to);
	}

	/// Callback function for the line_to member of FT_Outline_Funcs
	static int
	walkLineTo(FT_CONST FT_Vector* to, void* ptr)
	{
		OutlineWalker* walker = static_cast<OutlineWalker*>(ptr);
		return walker->lineTo(to);
	}

	/// Callback function for the conic_to member of FT_Outline_Funcs
	static int
	walkConicTo(FT_CONST FT_Vector* ctrl, FT_CONST FT_Vector* to, void* ptr)
	{
		OutlineWalker* walker = static_cast<OutlineWalker*>(ptr);
		return walker->conicTo(ctrl, to);
	}

	/// Callback function for the cubic_to member of FT_Outline_Funcs
	//
	/// Transform the cubic curve into a quadratic one an interpolated point
	/// falling in the middle of the two control points.
	///
	static int
	walkCubicTo(FT_CONST FT_Vector* ctrl1, FT_CONST FT_Vector* ctrl2, FT_CONST FT_Vector* to, void* ptr)
	{
		OutlineWalker* walker = static_cast<OutlineWalker*>(ptr);
		return walker->cubicTo(ctrl1, ctrl2, to);
	}

private:

	DynamicShape& _sh;

	float _scale;

	int moveTo(const FT_Vector* to)
	{
#ifdef DEBUG_OUTLINE_DECOMPOSITION 
		log_debug("moveTo: %ld,%ld", to->x, to->y);
#endif
		_sh.moveTo(to->x*_scale, -to->y*_scale);
		return 0;
	}

	int
	lineTo(const FT_Vector* to)
	{
#ifdef DEBUG_OUTLINE_DECOMPOSITION 
		log_debug("lineTo: %ld,%ld", to->x, to->y);
#endif
		_sh.lineTo(to->x*_scale, -to->y*_scale);
		return 0;
	}

	int
	conicTo(const FT_Vector* ctrl, const FT_Vector* to)
	{
#ifdef DEBUG_OUTLINE_DECOMPOSITION 
		log_debug("conicTo: %ld,%ld %ld,%ld", ctrl->x, ctrl->y, to->x, to->y);
#endif
		_sh.curveTo(ctrl->x*_scale, -ctrl->y*_scale, to->x*_scale, -to->y*_scale);
		return 0;
	}

	int
	cubicTo(const FT_Vector* ctrl1, const FT_Vector* ctrl2, const FT_Vector* to)
	{
#ifdef DEBUG_OUTLINE_DECOMPOSITION 
		log_debug("cubicTo: %ld,%ld %ld,%ld %ld,%ld", ctrl1->x, ctrl1->y, ctrl2->x, ctrl2->y, to->x, to->y);
#endif

		float x = ctrl1->x + ( (ctrl2->x - ctrl1->x) * 0.5 );
		float y = ctrl1->y + ( (ctrl2->y - ctrl1->y) * 0.5 );

		_sh.curveTo(x*_scale, -y*_scale, to->x*_scale, -to->y*_scale);
		return 0;
	}


};

// static
FT_Library FreetypeGlyphsProvider::m_lib;

// static private
void FreetypeGlyphsProvider::init()
{
	int	error = FT_Init_FreeType(&m_lib);
	if (error)
	{
		fprintf(stderr, "can't init FreeType!  error = %d\n", error);
		exit(1);
	}
}

// static private
void FreetypeGlyphsProvider::close()
{
	int error = FT_Done_FreeType(m_lib);
	if (error)
	{
		fprintf(stderr, "can't close FreeType!  error = %d\n", error);
	}
}

// private
std::auto_ptr<image::alpha>
FreetypeGlyphsProvider::draw_bitmap(const FT_Bitmap& bitmap)
{
	// You must use power-of-two dimensions!!
	int	w = 1; while (w < bitmap.pitch) { w <<= 1; }
	int	h = 1; while (h < bitmap.rows) { h <<= 1; }

	std::auto_ptr<image::alpha> alpha ( image::create_alpha(w, h) );

	// TODO: replace with image_base::clear(byte value)
	memset(alpha->data(), 0, alpha->size());

	// copy image to alpha
	for (int i = 0; i < bitmap.rows; i++)
	{
		uint8_t*	src = bitmap.buffer + bitmap.pitch * i;
		uint8_t*	dst = alpha->scanline(i); 
		int	x = bitmap.width;
		while (x-- > 0)
		{
			*dst++ = *src++;
		}
	}

	return alpha;
}

// private
bool
FreetypeGlyphsProvider::getFontFilename(const std::string &name,
		bool bold, bool italic, std::string& filename)
{

#ifdef HAVE_FONTCONFIG

	if (!FcInit ())
	{

		log_error("Can't init fontconfig library, using hard-coded font filename");
		filename = DEFAULT_FONTFILE;
		return true;
		//return false;
	}
	
	FcResult    result;

	FcPattern* pat = FcNameParse((const FcChar8*)name.c_str());
	
	FcConfigSubstitute (0, pat, FcMatchPattern);

	if (italic) {
		FcPatternAddInteger (pat, FC_SLANT, FC_SLANT_ITALIC);
	}

	if (bold) {
		FcPatternAddInteger (pat, FC_WEIGHT, FC_WEIGHT_BOLD);
	}

	FcDefaultSubstitute (pat);

	FcPattern   *match;
	match = FcFontMatch (0, pat, &result);
	FcPatternDestroy (pat);

	FcFontSet* fs = NULL;
	if (match)
	{
		fs = FcFontSetCreate ();
		FcFontSetAdd (fs, match);
	}

	if ( fs )
	{
#ifdef GNASH_DEBUG_DEVICEFONTS
		log_debug("Found %d fonts matching the family %s (using first)", fs->nfont, name.c_str());
#endif

		for (int j = 0; j < fs->nfont; j++)
		{
			FcChar8 *file;
			if (FcPatternGetString (fs->fonts[j], FC_FILE, 0, &file) != FcResultMatch)
			{
#ifdef GNASH_DEBUG_DEVICEFONTS
		log_debug("Matching font %d has unknown filename, skipping", j);
#endif
		continue;
			}

			filename = (char *)file;
			FcFontSetDestroy(fs);
			return true;

		}

		FcFontSetDestroy(fs);
	}

	log_error("No device font matches the name '%s', using hard-coded font filename", name.c_str());
	filename = DEFAULT_FONTFILE;
	return true;
#else
	log_error("Font filename matching not implemented (no fontconfig support built-in), using hard-coded font filename",
			name.c_str());
	filename = DEFAULT_FONTFILE;
	return true;
#endif
}

#endif // USE_FREETYPE 

#ifdef USE_FREETYPE 
// static
std::auto_ptr<FreetypeGlyphsProvider>
FreetypeGlyphsProvider::createFace(const std::string& name, bool bold, bool italic)
{

	std::auto_ptr<FreetypeGlyphsProvider> ret;

	try { 
		ret.reset( new FreetypeGlyphsProvider(name, bold, italic) );
	} catch (GnashException& ge) {
		log_error(ge.what());
		assert(! ret.get());
	}

	return ret;

}
#else // ndef USE_FREETYPE 
std::auto_ptr<FreetypeGlyphsProvider>
FreetypeGlyphsProvider::createFace(const std::string&, bool, bool)
{
	log_error("Freetype not supported");
	return std::auto_ptr<FreetypeGlyphsProvider>(NULL);
}
#endif // ndef USE_FREETYPE 

#ifdef USE_FREETYPE 
FreetypeGlyphsProvider::FreetypeGlyphsProvider(const std::string& name, bool bold, bool italic)
	:
	m_face(NULL)
{
	const unsigned maxerrlen = 64;
	char buf[maxerrlen];

	if (m_lib == NULL)
	{
		init();
	}

	std::string filename;
	if (getFontFilename(name, bold, italic, filename) == false)
	{
		snprintf(buf, maxerrlen, _("Can't find font file for font '%s'"), name.c_str());
		buf[maxerrlen-1] = '\0';
		throw GnashException(buf);
	}

	int error = FT_New_Face(m_lib, filename.c_str(), 0, &m_face);
	switch (error)
	{
		case 0:
			break;

		case FT_Err_Unknown_File_Format:
			snprintf(buf, maxerrlen, _("Font file '%s' has bad format"), filename.c_str());
			buf[maxerrlen-1] = '\0';
			throw GnashException(buf);
			break;

		default:
			// TODO: return a better error message !
			snprintf(buf, maxerrlen, _("Some error opening font '%s'"), filename.c_str());
			buf[maxerrlen-1] = '\0';
			throw GnashException(buf);
			break;
	}

	// We want an EM of 1024, so if units_per_EM is different
	// we will scale 
	scale = 1024.0f/m_face->units_per_EM;

#ifdef GNASH_DEBUG_DEVICEFONTS
	log_debug("EM square for font '%s' is %d, scale is thus %g", name.c_str(), m_face->units_per_EM, scale);
#endif
}
#else // ndef(USE_FREETYPE)
FreetypeGlyphsProvider::FreetypeGlyphsProvider(const std::string&, bool, bool)
{
	abort(); // should never be called
}
#endif // ndef USE_FREETYPE 

#ifdef USE_FREETYPE
boost::intrusive_ptr<shape_character_def>
FreetypeGlyphsProvider::getGlyph(uint16_t code, float& advance)
{
	boost::intrusive_ptr<DynamicShape> sh;

	FT_Error error = FT_Load_Char(m_face, code, FT_LOAD_NO_BITMAP|FT_LOAD_NO_SCALE);
	if ( error != 0 )
	{
		log_error("Error loading freetype outline glyph for char '%c' (error: %d)", code, error);
		return sh.get();
	}

	// Scale advance by current scale, to match expected output coordinate space
	advance = m_face->glyph->metrics.horiAdvance * scale;
#ifdef GNASH_DEBUG_DEVICEFONTS 
	log_debug("Advance value for glyph '%c' is %g (horiAdvance:%ld, scale:%g)", code, advance, m_face->glyph->metrics.horiAdvance, scale);
#endif

	if ( m_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE )
	{
		unsigned long gf = m_face->glyph->format;
		log_unimpl("FT_Load_Char() returned a glyph format != FT_GLYPH_FORMAT_OUTLINE (%c%c%c%c)",
			static_cast<char>((gf>>24)&0xff),
			static_cast<char>((gf>>16)&0xff),
			static_cast<char>((gf>>8)&0xff),
			static_cast<char>(gf&0xff));
		return 0;
	}
	//assert(m_face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

	FT_Outline* outline = &(m_face->glyph->outline);

	//FT_BBox	glyphBox;
	//FT_Outline_Get_BBox(outline, &glyphBox);
	//rect r(glyphBox.xMin, glyphBox.yMin, glyphBox.xMax, glyphBox.yMax);
	//log_msg("Glyph for character '%c' has computed bounds %s", code, r.toString().c_str());

	sh = new DynamicShape();
	sh->beginFill(rgba(255, 255, 255, 255));

	FT_Outline_Funcs walk;
       	walk.move_to = OutlineWalker::walkMoveTo;
	walk.line_to = OutlineWalker::walkLineTo;
	walk.conic_to = OutlineWalker::walkConicTo;
	walk.cubic_to = OutlineWalker::walkCubicTo;
	walk.shift = 0; // ?
	walk.delta = 0; // ?

#ifdef DEBUG_OUTLINE_DECOMPOSITION 
	log_debug("Decomposing glyph outline for character %u", code);
#endif

	OutlineWalker walker(*sh, scale);

	FT_Outline_Decompose(outline, &walk, &walker);
#ifdef DEBUG_OUTLINE_DECOMPOSITION 
	rect bound; sh->compute_bound(&bound);
	log_msg("Decomposed glyph for character '%c' has bounds %s", code, bound.toString().c_str());
#endif

	return sh.get();
}
#else // ndef(USE_FREETYPE)
boost::intrusive_ptr<shape_character_def>
FreetypeGlyphsProvider::getGlyph(uint16_t, float& advance)
{
	abort(); // should never be called... 
}
#endif // ndef(USE_FREETYPE)

FreetypeGlyphsProvider::~FreetypeGlyphsProvider()
{
#ifdef USE_FREETYPE 
	if ( m_face )
	{
		if ( FT_Done_Face(m_face) != 0 )
		{
			log_error("Could not release FT face resources");
		}
	}
#endif // ndef(USE_FREETYPE)
}

} // namespace gnash

