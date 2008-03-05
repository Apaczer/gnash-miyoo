// shape_character_def.cpp:  Quadratic bezier outline shapes, for Gnash.
//
//   Copyright (C) 2006, 2007, 2008 Free Software Foundation, Inc.
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


// Based on the public domain shape.cpp of Thatcher Ulrich <tu@tulrich.com> 2003

// Quadratic bezier outline shapes are the basis for most SWF rendering.

#include "shape_character_def.h"

#include "impl.h"
#include "log.h"
#include "render.h"
#include "stream.h"
#include "sprite_instance.h"

#include "tu_file.h"

#include <cfloat>
#include <algorithm>

#ifdef __sgi
extern double round(double);
#pragma optional round
#endif

// Define the macro below to always compute bounds for shape characters
// and compare them with the bounds encoded in the SWF
//#define GNASH_DEBUG_SHAPE_BOUNDS 1

//#define DEBUG_DISPLAY_SHAPE_PATHS    // won't probably work anymore (Udo)
#ifdef DEBUG_DISPLAY_SHAPE_PATHS
// For debugging only!
bool  gnash_debug_show_paths = false;
#endif                                            // DEBUG_DISPLAY_SHAPE_PATHS

namespace gnash
{

    static float  s_curve_max_pixel_error = 1.0f;

    //
    // helper functions.
    //

    void  set_curve_max_pixel_error(float pixel_error)
    {
        s_curve_max_pixel_error = fclamp(pixel_error, 1e-6f, 1e6f);
    }

    float get_curve_max_pixel_error()
    {
        return s_curve_max_pixel_error;
    }

    // Read fill styles, and push them onto the given style array.
    static void
        read_fill_styles(std::vector<fill_style>& styles, stream* in,
        int tag_type, movie_definition* m)
    {

        // Get the count.
        in->ensureBytes(1);
        boost::uint16_t fill_style_count = in->read_u8();
        if (tag_type > 2)
        {
            if (fill_style_count == 0xFF)
            {
                in->ensureBytes(2);
                fill_style_count = in->read_u16();
            }
        }

        IF_VERBOSE_PARSE (
            log_parse(_("  read_fill_styles: count = %u"), fill_style_count);
            );

        // Read the styles.
        styles.reserve(styles.size()+fill_style_count);
        for (boost::uint16_t i = 0; i < fill_style_count; ++i)
        {
            // TODO: add a fill_style constructor directly
            //       reading from stream
            fill_style fs;
            fs.read(in, tag_type, m);
            // Push a style anyway, so any path referring to
            // it still finds it..
            styles.push_back(fs);
        }

    }

    static void
        read_line_styles(std::vector<line_style>& styles, stream* in, int tag_type,
        movie_definition *md)
    // Read line styles and push them onto the back of the given array.
    {
        // Get the count.
        in->ensureBytes(1);
        int line_style_count = in->read_u8();

        IF_VERBOSE_PARSE
            (
            log_parse(_("  read_line_styles: count = %d"), line_style_count);
            );

        // @@ does the 0xFF flag apply to all tag types?
        // if (tag_type > 2)
        // {
        if (line_style_count == 0xFF)
        {
            in->ensureBytes(2);
            line_style_count = in->read_u16();
            IF_VERBOSE_PARSE
                (
                log_parse(_("  read_line_styles: count2 = %d"), line_style_count);
                );
        }
        // }

        // Read the styles.
        for (int i = 0; i < line_style_count; i++)
        {
            styles.resize(styles.size() + 1);
            //styles[styles.size() - 1].read(in, tag_type);
            styles.back().read(in, tag_type, md);
        }
    }

    shape_character_def::shape_character_def()
        :
    character_def(),
        m_fill_styles(),
        m_line_styles(),
        m_paths(),
        m_bound()
    //,m_cached_meshes()
    {
    }

    shape_character_def::shape_character_def(const shape_character_def& o)
        :
    character_def(o),
        m_fill_styles(o.m_fill_styles),
        m_line_styles(o.m_line_styles),
        m_paths(o.m_paths),
        m_bound(o.m_bound)
    //,m_cached_meshes()
    {
    }

    shape_character_def::~shape_character_def()
    {
        //clear_meshes();
    }

    void
        shape_character_def::read(stream* in, int tag_type, bool with_style,
        movie_definition* m)
    {
        if (with_style)
        {
            m_bound.read(in);

            IF_VERBOSE_PARSE
                (
                std::string b = m_bound.toString();
                log_parse(_("  bound rect: %s"), b.c_str());
                );

            // TODO: Store and use these.  Unfinished.
            if (tag_type == SWF::DEFINESHAPE4 || tag_type == SWF::DEFINESHAPE4_)
            {
                rect tbound;
                tbound.read(in);
                in->ensureBytes(1);
                /*boost::uint8_t scales =*/static_cast<void>(in->read_u8());
                static bool warned = false;
                if ( ! warned )
                {
                    log_unimpl("DEFINESHAPE4 edge boundaries and scales");
                    warned = true;
                }
            }

            read_fill_styles(m_fill_styles, in, tag_type, m);
            read_line_styles(m_line_styles, in, tag_type, m);
        }

        /// Adding a dummy fill style is just needed to make the
        /// parser somewhat more robust. This fill style is not
        /// really used, as text rendering will use style information
        /// from TEXTRECORD tag instead.
        ///
        if ( tag_type == SWF::DEFINEFONT || tag_type == SWF::DEFINEFONT2 )
        {
            assert(!with_style);
            //m_fill_styles.push_back(fill_style());
        }

        //log_debug("Read %u fill styles, %u line styles", m_fill_styles.size(), m_line_styles.size());

        // Use read_u8 to force alignment.
        in->ensureBytes(1);
        boost::uint8_t num_bits = in->read_u8();
        int num_fill_bits = (num_bits & 0xF0) >> 4;
        int num_line_bits = (num_bits & 0x0F);

        IF_VERBOSE_PARSE
            (
            log_parse(_("  shape_character_def read: nfillbits = %d, nlinebits = %d"), num_fill_bits, num_line_bits);
            );

        if ( !num_fill_bits && !num_line_bits )
        {
            /// When reading font glyphs it happens to read 1 byte
            /// past end boundary of a glyph due to fill/line bits being
            /// zero.
            ///
            /// Generally returning here seems to break morphs:
            ///  http://savannah.gnu.org/bugs/?21747
            /// And other normal shapes:
            ///  http://savannah.gnu.org/bugs/?21923
            ///  http://savannah.gnu.org/bugs/?22000
            ///
            /// So for now we only return if NOT reading a morph shape.
            /// Pretty ugly... till next bug report.
            ///
            ///
            if (tag_type == SWF::DEFINEFONT || tag_type == SWF::DEFINEFONT2 || tag_type == SWF::DEFINEFONT3)
            {
                // better log something, so we have an hint
                // if something is broken
                log_debug("Skipping glyph read, being fill and line bits zero. SWF tag is %d.", tag_type);
                return;
            }
        }

        // These are state variables that keep the
        // current position & style of the shape
        // outline, and vary as we read the edge data.
        //
        // At the moment we just store each edge with
        // the full necessary info to render it, which
        // is simple but not optimally efficient.
        int fill_base = 0;
        int line_base = 0;
        int   x = 0, y = 0;
        path  current_path;

#define SHAPE_LOG 0

        // SHAPERECORDS
        for (;;)
        {
            in->ensureBits(1);
            bool isEdgeRecord = in->read_bit();
            if (!isEdgeRecord)
            {
                // Parse the record.
                in->ensureBits(5);
                int flags = in->read_uint(5);
                if (flags == flagEnd)
                {
                    // End of shape records.

                    // Store the current path if any.
                    if (! current_path.is_empty())
                    {
                        m_paths.push_back(current_path);
                        current_path.m_edges.resize(0);
                    }

                    break;
                }
                if (flags & flagMove)
                {
                    // move_to = 1;

                    // Store the current path if any, and prepare a fresh one.
                    if (! current_path.is_empty())
                    {
                        m_paths.push_back(current_path);
                        current_path.m_edges.resize(0);
                    }
                    in->ensureBits(5);
                    int num_move_bits = in->read_uint(5);
                    in->ensureBits(2 * num_move_bits);
                    int move_x = in->read_sint(num_move_bits);
                    int move_y = in->read_sint(num_move_bits);

                    x = move_x;
                    y = move_y;

                    // Set the beginning of the path.
                    current_path.ap.x = x;
                    current_path.ap.y = y;

#if SHAPE_LOG
                    IF_VERBOSE_PARSE
                        (
                        log_parse(_("  shape_character read: moveto %d %d"), x, y);
                        );
#endif
                }
                if ((flags & flagFillStyle0Change) && num_fill_bits > 0)
                {
                    // fill_style_0_change = 1;
                    if (! current_path.is_empty())
                    {
                        m_paths.push_back(current_path);
                        current_path.m_edges.resize(0);
                        current_path.ap.x = x;
                        current_path.ap.y = y;
                    }
                    in->ensureBits(num_fill_bits);
                    unsigned style = in->read_uint(num_fill_bits);
                    if (style > 0)
                    {
                        style += fill_base;
                    }

                    if ( tag_type == SWF::DEFINEFONT || tag_type == SWF::DEFINEFONT2 )
                    {
                        if ( style > 1 )          // 0:hide 1:renderer
                        {
                            IF_VERBOSE_MALFORMED_SWF(
                                log_swferror(_("Invalid fill style %d in fillStyle0Change record for font tag (0 or 1 valid). Set to 0."), style);
                                );
                            style = 0;
                        }
                    }
                    else
                    {
                                                  // 1-based index
                        if ( style > m_fill_styles.size() )
                        {
                            IF_VERBOSE_MALFORMED_SWF(
                                log_swferror(_("Invalid fill style %d in fillStyle0Change record - " SIZET_FMT " defined. Set to 0."), style, m_fill_styles.size());
                                );
                            style = 0;
                        }
                    }

                    current_path.setLeftFill(style);
#if SHAPE_LOG
                    IF_VERBOSE_PARSE(
                        log_parse(_("  shape_character read: fill0 (left) = %d"), current_path.getLeftFill());
                        );
#endif

                }
                if ((flags & flagFillStyle1Change) && num_fill_bits > 0)
                {
                    // fill_style_1_change = 1;
                    if (! current_path.is_empty())
                    {
                        m_paths.push_back(current_path);
                        current_path.m_edges.resize(0);
                        current_path.ap.x = x;
                        current_path.ap.y = y;
                    }
                    in->ensureBits(num_fill_bits);
                    unsigned style = in->read_uint(num_fill_bits);
                    if (style > 0)
                    {
                        style += fill_base;
                    }

                    if ( tag_type == SWF::DEFINEFONT || tag_type == SWF::DEFINEFONT2 )
                    {
                        if ( style > 1 )          // 0:hide 1:renderer
                        {
                            IF_VERBOSE_MALFORMED_SWF(
                                log_swferror(_("Invalid fill style %d in fillStyle1Change record for font tag (0 or 1 valid). Set to 0."), style);
                                );
                            style = 0;
                        }
                    }
                    else
                    {
                                                  // 1-based index
                        if ( style > m_fill_styles.size() )
                        {
                            IF_VERBOSE_MALFORMED_SWF(
                                log_swferror(_("Invalid fill style %d in fillStyle1Change record - " SIZET_FMT " defined. Set to 0."), style, m_fill_styles.size());
                                );
                            style = 0;
                        }
                    }
                                                  // getRightFill() = style;
                    current_path.setRightFill(style);
#if SHAPE_LOG
                    IF_VERBOSE_PARSE (
                        log_parse(_("  shape_character read: fill1 (right) = %d"), current_path.getRightFill());
                        )
    #endif
                }
                if ((flags & flagLineStyleChange) && num_line_bits > 0)
                {
                    // line_style_change = 1;
                    if (! current_path.is_empty())
                    {
                        m_paths.push_back(current_path);
                        current_path.m_edges.resize(0);
                        current_path.ap.x = x;
                        current_path.ap.y = y;
                    }
                    in->ensureBits(num_line_bits);
                    unsigned style = in->read_uint(num_line_bits);
                    if (style > 0)
                    {
                        style += line_base;
                    }
                    if ( tag_type == SWF::DEFINEFONT || tag_type == SWF::DEFINEFONT2 )
                    {
                        if ( style > 1 )          // 0:hide 1:renderer
                        {
                            IF_VERBOSE_MALFORMED_SWF(
                                log_swferror(_("Invalid line style %d in lineStyleChange record for font tag (0 or 1 valid). Set to 0."), style);
                                );
                            style = 0;
                        }
                    }
                    else
                    {
                                                  // 1-based index
                        if ( style > m_line_styles.size() )
                        {
                            IF_VERBOSE_MALFORMED_SWF(
                                log_swferror(_("Invalid fill style %d in lineStyleChange record - " SIZET_FMT " defined. Set to 0."), style, m_line_styles.size());
                                );
                            style = 0;
                        }
                    }
                    current_path.setLineStyle(style);
#if SHAPE_LOG
                    IF_VERBOSE_PARSE (
                        log_parse(_("  shape_character_read: line = %d"), current_path.getLineStyle());
                        )
    #endif
                }
                if (flags & flagHasNewStyles)
                {
                    if (!with_style)
                    {
                        IF_VERBOSE_MALFORMED_SWF(
                            log_swferror("Unexpected HasNewStyle flag in tag %d shape record", tag_type);
                            )
                        // Used to be tag_type += SWF::DEFINESHAPE2, but
                        // I can't belive any such thing could be correct...
                            continue;
                    }

                    IF_VERBOSE_PARSE (
                        log_parse(_("  shape_character read: more fill styles"));
                        );

                    // Store the current path if any.
                    if (! current_path.is_empty())
                    {
                        m_paths.push_back(current_path);
                        current_path.clear();
                    }

                    // Tack on an empty path signalling a new shape.
                    // @@ need better understanding of whether this is correct??!?!!
                    // @@ i.e., we should just start a whole new shape here, right?
                    m_paths.push_back(path());
                    m_paths.back().m_new_shape = true;

                    fill_base = m_fill_styles.size();
                    line_base = m_line_styles.size();
                    read_fill_styles(m_fill_styles, in, tag_type, m);
                    read_line_styles(m_line_styles, in, tag_type, m);

                    in->ensureBits(8);
                    num_fill_bits = in->read_uint(4);
                    num_line_bits = in->read_uint(4);
                }
            }
            else
            {
                // EDGERECORD
                in->ensureBits(1);
                bool edge_flag = in->read_bit();
                if (edge_flag == 0)
                {
                    in->ensureBits(4);
                    int num_bits = 2 + in->read_uint(4);
                    // curved edge
                    in->ensureBits(4 * num_bits);
                    int cx = x + in->read_sint(num_bits);
                    int cy = y + in->read_sint(num_bits);
                    int ax = cx + in->read_sint(num_bits);
                    int ay = cy + in->read_sint(num_bits);

#if SHAPE_LOG
                    IF_VERBOSE_PARSE (
                        log_parse(_("  shape_character read: curved edge   = %d %d - %d %d - %d %d"), x, y, cx, cy, ax, ay);
                        );
#endif

                    current_path.m_edges.push_back(edge(cx, cy, ax, ay));

                    x = ax;
                    y = ay;
                }
                else
                {
                    // straight edge
                    in->ensureBits(5);
                    int num_bits = 2 + in->read_uint(4);
                    bool  line_flag = in->read_bit();
                    int dx = 0, dy = 0;
                    if (line_flag)
                    {
                        // General line.
                        in->ensureBits(2 * num_bits);
                        dx = in->read_sint(num_bits);
                        dy = in->read_sint(num_bits);
                    }
                    else
                    {
                        in->ensureBits(1);
                        bool vert_flag = in->read_bit();
                        if (vert_flag == 0)
                        {
                            // Horizontal line.
                            in->ensureBits(num_bits);
                            dx = in->read_sint(num_bits);
                        }
                        else
                        {
                            // Vertical line.
                            in->ensureBits(num_bits);
                            dy = in->read_sint(num_bits);
                        }
                    }

#if SHAPE_LOG
                    IF_VERBOSE_PARSE (
                        log_parse(_("  shape_character_read: straight edge = %d %d - %d %d"), x, y, x + dx, y + dy);
                        );
#endif

                    current_path.m_edges.push_back(edge(x + dx, y + dy, x + dx, y + dy));

                    x += dx;
                    y += dy;
                }
            }
        }

        if ( ! with_style )
        {
            // TODO: performance would be improved by computing
            //       the bounds as edges are parsed.
            compute_bound(&m_bound);
        }
#ifdef GNASH_DEBUG_SHAPE_BOUNDS
        else
        {
            rect computedBounds;
            compute_bound(&computedBounds);
            if ( computedBounds != m_bounds )
            {
                log_debug("Shape character read for tag %d contained embedded bounds %s, while we computed bounds %s",
                    tag_type, m_bound.toString().c_str(), computedBounds.toString().c_str());
            }
        }
#endif                                    // GNASH_DEBUG_SHAPE_BOUNDS
    }

    void  shape_character_def::display(character* inst)
    // Draw the shape using our own inherent styles.
    {
        //GNASH_REPORT_FUNCTION;

        gnash::render::draw_shape_character(this, inst);

        /*
            matrix  mat = inst->get_world_matrix();
            cxform  cx = inst->get_world_cxform();

            float pixel_scale = inst->get_parent()->get_pixel_scale();
            display(mat, cx, pixel_scale, m_fill_styles, m_line_styles);
            */
    }

#ifdef DEBUG_DISPLAY_SHAPE_PATHS

#include "ogl.h"

    static void point_normalize(point* p)
    {
        float mag2 = p->x * p->x + p->y * p->y;
        if (mag2 < 1e-9f)
        {
            // Very short vector.
            // @@ log error

            // Arbitrary unit vector.
            p->x = 1;
            p->y = 0;
        }

        float inv_mag = 1.0f / sqrtf(mag2);
        p->x *= inv_mag;
        p->y *= inv_mag;
    }

    static void show_fill_number(const point& p, int fill_number)
    {
        // We're inside a glBegin(GL_LINES)

        // Eh, let's do it in binary, least sig four bits...
        float x = p.x;
        float y = p.y;

        int mask = 8;
        while (mask)
        {
            if (mask & fill_number)
            {
                // Vert line --> 1.
                glVertex2f(x, y - 40.0f);
                glVertex2f(x, y + 40.0f);
            }
            else
            {
                // Rectangle --> 0.
                glVertex2f(x - 10.0f, y - 40.0f);
                glVertex2f(x + 10.0f, y - 40.0f);

                glVertex2f(x + 10.0f, y - 40.0f);
                glVertex2f(x + 10.0f, y + 40.0f);

                glVertex2f(x - 10.0f, y + 40.0f);
                glVertex2f(x + 10.0f, y + 40.0f);

                glVertex2f(x - 10.0f, y - 40.0f);
                glVertex2f(x - 10.0f, y + 40.0f);
            }
            x += 40.0f;
            mask >>= 1;
        }
    }

    static void debug_display_shape_paths(
        const matrix& mat,
        float /* object_space_max_error */,
        const std::vector<path>& paths,
        const std::vector<fill_style>& /* fill_styles */,
        const std::vector<line_style>& /* line_styles */)
    {
        for (unsigned int i = 0; i < paths.size(); i++)
        {
            //      if (i > 0) break;//xxxxxxxx
            const path& p = paths[i];

            if (p.getLeftFill() == 0 && p.getRightFill() == 0)
            {
                continue;
            }

            gnash::render::set_matrix(mat);

            // Color the line according to which side has
            // fills.
            if (p.getLeftFill() == 0) glColor4f(1, 0, 0, 0.5);
            else if (p.getRightFill() == 0) glColor4f(0, 1, 0, 0.5);
            else glColor4f(0, 0, 1, 0.5);

            // Offset according to which loop we are.
            float offset_x = (i & 1) * 80.0f;
            float offset_y = ((i & 2) >> 1) * 80.0f;
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glTranslatef(offset_x, offset_y, 0.f);

            point pt;

            glBegin(GL_LINE_STRIP);

            mat.transform(&pt, point(p.ap.x, p.ap.y));
            glVertex2f(pt.x, pt.y);

            for (unsigned int j = 0; j < p.m_edges.size(); j++)
            {
                mat.transform(&pt, point(p.m_edges[j].cp.x, p.m_edges[j].cp.y));
                glVertex2f(pt.x, pt.y);
                mat.transform(&pt, point(p.m_edges[j].ap.x, p.m_edges[j].ap.y));
                glVertex2f(pt.x, pt.y);
            }

            glEnd();

            // Draw arrowheads.
            point dir, right, p0, p1;
            glBegin(GL_LINES);
            {
                for (unsigned int j = 0; j < p.m_edges.size(); j++)
                {
                    mat.transform(&p0, point(p.m_edges[j].cp.x, p.m_edges[j].cp.y));
                    mat.transform(&p1, point(p.m_edges[j].ap.x, p.m_edges[j].ap.y));
                    dir = point(p1.x - p0.x, p1.y - p0.y);
                    point_normalize(&dir);
                    right = point(-dir.y, dir.x); // perpendicular

                    const float ARROW_MAG = 60.f; // TWIPS?
                    if (p.getLeftFill() != 0)
                    {
                        glColor4f(0, 1, 0, 0.5);
                        glVertex2f(p0.x,
                            p0.y);
                        glVertex2f(p0.x - dir.x * ARROW_MAG - right.x * ARROW_MAG,
                            p0.y - dir.y * ARROW_MAG - right.y * ARROW_MAG);

                        show_fill_number(point(p0.x - right.x * ARROW_MAG * 4,
                            p0.y - right.y * ARROW_MAG * 4),
                            p.getLeftFill());
                    }
                    if (p.getRightFill() != 0)
                    {
                        glColor4f(1, 0, 0, 0.5);
                        glVertex2f(p0.x,
                            p0.y);
                        glVertex2f(p0.x - dir.x * ARROW_MAG + right.x * ARROW_MAG,
                            p0.y - dir.y * ARROW_MAG + right.y * ARROW_MAG);

                        show_fill_number(point(p0.x + right.x * ARROW_MAG * 4,
                            p0.y + right.y * ARROW_MAG * 4),
                            p.getRightFill());
                    }
                }
            }
            glEnd();

            glPopMatrix();
        }
    }
#endif                                        // DEBUG_DISPLAY_SHAPE_PATHS

    void  shape_character_def::display(
        const matrix& mat,
        const cxform& cx,
        float pixel_scale,
        const std::vector<fill_style>& fill_styles,
        const std::vector<line_style>& line_styles) const
    {
        shape_character_def* this_non_const = const_cast<shape_character_def*>(this);

        render_handler* renderer = get_render_handler();
        renderer->draw_shape_character(this_non_const, mat, cx, pixel_scale,
            fill_styles, line_styles);
    }

    // TODO: this should be moved to libgeometry or something
    int curve_x_crossings(float x0, float y0, float x1, float y1,
        float cx, float cy, float y, float &cross1, float &cross2)
    // Finds the quadratic bezier curve crossings with the line Y.
    // The function can have zero, one or two solutions (cross1, cross2). The
    // return value of the function is the number of solutions.
    // x0, y0 = start point of the curve
    // x1, y1 = end point of the curve (anchor, aka ax|ay)
    // cx, cy = control point of the curve
    // If there are two crossings, cross1 is the nearest to x0|y0 on the curve.
    {
        int count=0;

        // check if any crossings possible
        if ( ((y0 < y) && (y1 < y) && (cy < y))
            || ((y0 > y) && (y1 > y) && (cy > y)) )
        {
            // all above or below -- no possibility of crossing
            return 0;
        }

        // Quadratic bezier is:
        //
        // p = (1-t)^2 * a0 + 2t(1-t) * c + t^2 * a1
        //
        // We need to solve for x at y.

        // Use the quadratic formula.

        // Numerical Recipes suggests this variation:
        // q = -0.5 [b +sgn(b) sqrt(b^2 - 4ac)]
        // x1 = q/a;  x2 = c/q;

        float A = y1 + y0 - 2 * cy;
        float B = 2 * (cy - y0);
        float C = y0 - y;

        float rad = B * B - 4 * A * C;

        if (rad < 0)
        {
            return 0;
        }
        else
        {
            float q;
            float sqrt_rad = sqrtf(rad);
            if (B < 0)
            {
                q = -0.5f * (B - sqrt_rad);
            }
            else
            {
                q = -0.5f * (B + sqrt_rad);
            }

            // The old-school way.
            // float t0 = (-B + sqrt_rad) / (2 * A);
            // float t1 = (-B - sqrt_rad) / (2 * A);

            if (q != 0)
            {
                float t1 = C / q;
                if (t1 >= 0 && t1 < 1)
                {
                    float x_at_t1 =
                        x0 + 2 * (cx - x0) * t1 + (x1 + x0 - 2 * cx) * t1 * t1;

                    count++;
                    assert(count==1);
                    cross1 = x_at_t1;             // order is important!
                }
            }

            if (A != 0)
            {
                float t0 = q / A;
                if (t0 >= 0 && t0 < 1)
                {
                    float x_at_t0 =
                        x0 + 2 * (cx - x0) * t0 + (x1 + x0 - 2 * cx) * t0 * t0;

                    count++;
                    if (count==2)
                        cross2 = x_at_t0;         // order is important!
                    else
                        cross1 = x_at_t0;
                }
            }

        }

        return count;
    }

    bool  shape_character_def::point_test_local(float x, float y)
    // Return true if the specified point is on the interior of our shape.
    // Incoming coords are local coords.
    {

        //#define DEBUG_POINT_TEST
        //#define DEBUG_POINT_TEST_EXT

#ifdef DEBUG_POINT_TEST
        printf("=== point_test_local ===\n");
        char debug[1024];
#endif

        /*
        Principle:
        For the fill of the shape, we project a ray from the test point to the left
        side of the shape counting all crossings. When a line or curve segment is
        crossed we add 1 if the left fill style is set. Regardless of the left fill
        style we subtract 1 from the counter then the right fill style is set.
        This is true when the line goes in downward direction. If it goes upward,
        the fill styles are reversed.

        The final counter value reveals if the point is inside the shape (and depends
        on filling rule, see below).
        This method should not depend on subshapes and work for some malformed
        shapes situations:
        - wrong fill side (eg. left side set for a clockwise drawen rectangle)
        - intersecting paths
        */

        // Align test coordinates to TWIP coordinate system and shift by a half
        // TWIP to avoid line junction situations which are hard to handle. Oversample
        // everything by 100 to get some degree of accuracy (ie. this won't produce
        // any visible inaccuracy before the shape is scaled more an 2000x). The
        // resulting coordinate is *very* close to the original one and still in the
        // same coordinate system.
        x = (round(x * 2000.0f) + 0.5f) / 2000.0f;
        y = (round(y * 2000.0f) + 0.5f) / 2000.0f;

        point pt(x, y);

        bool even_odd = true;                     // later we will need non-zero for glyphs... (TODO)

        if (m_bound.point_test(x, y) == false)
        {
            // Early out.
            return false;
        }

        unsigned npaths = m_paths.size();
        int counter = 0;

        // browse all paths
        for (unsigned pno=0; pno<npaths; pno++)
        {

            const path& pth = m_paths[pno];
            unsigned nedges = pth.m_edges.size();

            float next_pen_x = pth.ap.x;
            float next_pen_y = pth.ap.y;
            float pen_x, pen_y;

            if (pth.m_new_shape)
            {
                if ( (even_odd && (counter % 2) != 0) ||
                    (!even_odd && (counter != 0)) )
                {
                    // the point is inside the previous subshape, so exit now

#ifdef DEBUG_POINT_TEST
                    printf("  subshape early out. counter=%d\n", counter);
#endif

                    return true;
                }

                counter=0;
            }

#ifdef DEBUG_POINT_TEST_EXT
            printf(" new path, anchor = %.2f / %.2f\n", pth.ap.x, pth.ap.y);
#endif

            if (pth.empty())
                continue;

            // If the path has a line style, check for strokes there
            if (pth.m_line != 0 )
            {

                assert(m_line_styles.size() >= pth.m_line);

                line_style& ls = m_line_styles[pth.m_line-1];

                int thickness = ls.get_width();
                float sqdist;

                if (thickness == 0)
                {
                    // hairline has always a tolerance of a single twip
                    sqdist = 1;
                }
                else
                {
                    float dist = thickness/2;
                    sqdist = dist*dist;
                }

                //cout << "Thickness of line is " << thickness << " squared is " << sqdist << endl;
                if (pth.withinSquareDistance(pt, sqdist))
                    return true;
            }

            // browse all edges of the path
            for (unsigned eno=0; eno<nedges; eno++)
            {

                const edge& edg = pth.m_edges[eno];

                pen_x = next_pen_x;
                pen_y = next_pen_y;

                next_pen_x = edg.ap.x;
                next_pen_y = edg.ap.y;

#ifdef DEBUG_POINT_TEST_EXT
                printf("  to %.2f / %.2f |", edg.ap.x, edg.ap.y);
#endif

                /* 
                printf("EDGE #%d #%d [ %d %d ] : %.2f / %.2f -> %.2f / %.2f\n", pno, eno,
                  pth.m_fill0, pth.m_fill1,
                  pen_x, pen_y,
                  edg.ap.x, edg.ap.y);
                */

                float cross1, cross2;
                int dir1, dir2=0;                 // +1 = downward, -1 = upward
                int crosscount=0;

                if (edg.is_straight())
                {

                    // ==> straight line case

#ifdef DEBUG_POINT_TEST
                    sprintf(debug, "straight");
#endif

                    // ignore horizontal lines
                    if (edg.ap.y == pen_y)        // TODO: better check for small difference?
                    {
#ifdef DEBUG_POINT_TEST_EXT
                        printf("  #%02d, #%02d [%s] horizontal line\n", pno, eno, debug);
#endif
                        continue;
                    }

                    // does this line cross the Y coordinate?
                    if ( ((pen_y <= y) && (edg.ap.y >= y))
                        || ((pen_y >= y) && (edg.ap.y <= y)) )
                    {

                        // calculate X crossing
                        cross1 = pen_x + (edg.ap.x - pen_x) *
                            (y - pen_y) / (edg.ap.y - pen_y);

                        if (pen_y > edg.ap.y)
                            dir1 = -1;            // upward
                        else
                            dir1 = +1;            // downward

                        crosscount=1;

                    }
                    else
                    {

                        // no crossing found
                        crosscount = 0;

                    }

                }
                else
                {

                    // ==> curve case

#ifdef DEBUG_POINT_TEST
                    sprintf(debug, "curve   ");
#endif

                    crosscount = curve_x_crossings(pen_x, pen_y, edg.ap.x, edg.ap.y,
                        edg.cp.x, edg.cp.y, y, cross1, cross2);

                    dir1 = pen_y > y ? -1 : +1;
                    dir2 = dir1 * (-1);           // second crossing always in opposite dir.

                }                                 // curve

                // ==> we have now:
                //  - one (cross1) or two (cross1, cross2) ray crossings (X coordinate)
                //  - dir1/dir2 tells the direction of the crossing
                //    (+1 = downward, -1 = upward)
                //  - crosscount tells the number of crossings

                // need at least one crossing
                if (crosscount==0)
                {
#ifdef DEBUG_POINT_TEST_EXT
                    printf("  #%02d, #%02d [%s] no crossing\n", pno, eno, debug);
#endif
                    continue;
                }

                bool touched=false;

                // check first crossing
                if (cross1 <= x)
                {
                    if (pth.m_fill0 > 0) counter += dir1;
                    if (pth.m_fill1 > 0) counter -= dir1;

                    touched = true;

#ifdef DEBUG_POINT_TEST
                    printf("  #%02d, #%02d [%s] crossing at x=%.2f y=%.2f dir=%d fills=[%d,%d] -> C=%d\n",
                        pno, eno, debug, cross1, y, dir1, pth.m_fill0, pth.m_fill1, counter);
#endif
                }

                // check optional second crossing (only possible with curves)
                if ((crosscount>1) && (cross2 <= x))
                {
                    if (pth.m_fill0 > 0) counter += dir2;
                    if (pth.m_fill1 > 0) counter -= dir2;

                    touched = true;

#ifdef DEBUG_POINT_TEST
                    printf("  #%02d, #%02d [%s] crossing at x=%.2f y=%.2f dir=%d fills=[%d,%d] -> C=%d\n",
                        pno, eno, debug, cross2, y, dir2, pth.m_fill0, pth.m_fill1, counter);
#endif
                }

#ifdef DEBUG_POINT_TEST_EXT
                if (!touched)
                    printf("  #%02d, #%02d [%s] no crossing at left side\n", pno, eno, debug);
#endif

            }                                     // for edge

        }                                         // for path

#ifdef DEBUG_POINT_TEST
        printf("  all paths processed. counter=%d\n", counter);
#endif

        return ( (even_odd && (counter % 2) != 0) ||
            (!even_odd && (counter != 0)) );
    }

    float shape_character_def::get_height_local() const
    {
        return m_bound.height();
    }

    float shape_character_def::get_width_local() const
    {
        return m_bound.width();
    }

    void  shape_character_def::compute_bound(rect* r) const
    // Find the bounds of this shape, and store them in
    // the given rectangle.
    {
        r->set_null();

        for (unsigned int i = 0; i < m_paths.size(); i++)
        {
            const path& p = m_paths[i];

            unsigned thickness = 0;
            if ( p.m_line )
            {
                // For glyph shapes m_line is allowed to be 1
                // while no defined line styles are allowed.
                if ( m_line_styles.empty() )
                {
                    // This is either a Glyph, for which m_line==1 is valid
                    // or a bug in the parser, which we have no way to
                    // check at this time
                    assert(p.m_line == 1);
                }
                else
                {
                    thickness = m_line_styles[p.m_line-1].get_width();
                }
            }
            p.expandBounds(*r, thickness);
        }
    }

#ifdef GNASH_USE_GC
    void
        shape_character_def::markReachableResources() const
    {
        assert(isReachable());
        for (FillStyleVect::const_iterator i=m_fill_styles.begin(), e=m_fill_styles.end();
            i != e; ++i)
        {
            i->markReachableResources();
        }
    }
#endif                                        // GNASH_USE_GC

    size_t
        shape_character_def::numPaths() const
    {
        return m_paths.size();
    }

    size_t
        shape_character_def::numEdges() const
    {
        typedef std::vector<path> PathList;

        size_t count = 0;
        for  (PathList::const_iterator i=m_paths.begin(), ie=m_paths.end(); i!=ie; ++i)
        {
            count += i->size();
        }
        return count;
    }

}                                                 // end namespace gnash

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
