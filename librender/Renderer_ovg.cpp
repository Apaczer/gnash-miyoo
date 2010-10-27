// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

///
/// Author: Visor <cutevisor@gmail.com>
///

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include <sys/time.h>
#include <cstring>
#include <cmath>
#include <boost/utility.hpp>
#include <boost/bind.hpp>

#include "log.h"
#include "RGBA.h"
#include "smart_ptr.h"
#include "GnashImage.h"
#include "GnashNumeric.h"
#include "FillStyle.h"
#include "log.h"
#include "utility.h"
#include "Range2d.h"
#include "SWFCxForm.h"
#include "Renderer_ovg.h"
#include "Renderer_ovg_bitmap.h"
#include "SWFMatrix.h"
#include "swf/ShapeRecord.h"

#define GNASH_IMAGE_QUALITY     VG_IMAGE_QUALITY_FASTER
#define GNASH_RENDER_QUALITY    VG_RENDERING_QUALITY_FASTER

#define  MAX_POINTS (4096)

/// \file Renderer_ovg.cpp
/// \brief The OpenVG renderer and related code.
///

namespace gnash {

static int tex_size;

typedef std::vector<Path> PathVec;
typedef std::vector<geometry::Range2d<int> > ClipBounds;

namespace renderer {

namespace openvg {

class eglScopeMatrix : public boost::noncopyable
{
public:
    eglScopeMatrix(const SWFMatrix& m)
        {
            vgGetMatrix(orig_mat);

            float mat[9];
            memset(mat, 0, sizeof(mat));
            mat[0] = m.sx  / 65536.0f;
            mat[1] = m.shx / 65536.0f;
            mat[3] = m.shy / 65536.0f;
            mat[4] = m.sy  / 65536.0f;
            mat[6] = m.tx;
            mat[7] = m.ty;
            vgMultMatrix(mat);
        }
  
    ~eglScopeMatrix()
        {
            vgLoadMatrix(orig_mat);
        }
private:
    VGfloat orig_mat[9];
};

#define MAX_SEG  (256)

inline void
startpath(VGPath path, const int x, const int y)
{
    VGubyte     gseg[1];
    VGfloat     gdata[2];
  
    gseg[0] = VG_MOVE_TO;
    gdata[0] = x;
    gdata[1] = y;
    vgAppendPathData (path, 1, gseg, gdata);
}

inline void
closepath(VGPath path)
{
    VGubyte     gseg[1];
    VGfloat     gdata[2];
  
    gseg[0] = VG_CLOSE_PATH;
    vgAppendPathData (path, 1, gseg, gdata);
}

inline void
preparepath(VGPath path, const std::vector<Edge>& edges,
                        const float& anchor_x, const float& anchor_y)
{
    VGubyte     gseg[MAX_SEG];
    VGfloat     gdata[MAX_SEG*3*2];
    int         scount = 0;
    int         dcount = 0; 

    point start(anchor_x, anchor_y);
    point anchor(anchor_x, anchor_y);
  
    for (std::vector<Edge>::const_iterator it = edges.begin(), end = edges.end();
         it != end; ++it) {
        const Edge& the_edge = *it;
      
        point target(the_edge.ap.x, the_edge.ap.y);

        if (the_edge.straight()) {
            gseg[scount++]  = VG_LINE_TO;
            gdata[dcount++] = target.x;
            gdata[dcount++] = target.y;
        } else {
            gseg[scount++]  = VG_QUAD_TO;
            gdata[dcount++] = the_edge.cp.x;
            gdata[dcount++] = the_edge.cp.y;
            gdata[dcount++] = target.x;
            gdata[dcount++] = target.y; 
        }
        if (scount >= MAX_SEG-2) {
            vgAppendPathData(path, scount, gseg, gdata);
            scount = 0;
            dcount = 0;
        }
        anchor = target;
    }
  
    if (scount > 0)
        vgAppendPathData (path, scount, gseg, gdata);
}

// Use the image class copy constructor; it's not important any more
// what kind of image it is.
bitmap_info_ovg::bitmap_info_ovg(gnash::image::GnashImage* img,
                                        VGImageFormat pixelformat, VGPaint paint) 
    :
    _img(img),
    _pixel_format(pixelformat),
    _paint(paint)
{
    _width = _img->width();
    _height = _img->height();
  
    if (_pixel_format == VG_sARGB_8888)
        _image = vgCreateImage(VG_sARGB_8888, _width, _height, GNASH_IMAGE_QUALITY);    
    
    // vgImageSubData(_image, _img->data(), _width * 4, VG_sARGB_8888,
    //                0, 0, _width, _height); FIXME
    
    tex_size += _width * _height * 4;
    log_debug("Add Texture size:%d (%d x %d x %dbpp)", _width * _height * 4, _width, _height, 4);
    log_debug("Current Texture size: %d", tex_size);
}   
    
bitmap_info_ovg::~bitmap_info_ovg()
{
    tex_size -= _width * _height * 4;
    log_debug("Remove Texture size:%d (%d x %d x %dbpp)", _width * _height * 4, _width, _height, 4);
    log_debug("Current Texture size: %d", tex_size);

    vgDestroyImage(_image);
}

void
bitmap_info_ovg::apply(const gnash::SWFMatrix& bitmap_matrix,
                       bitmap_wrap_mode wrap_mode) const
{
    gnash::SWFMatrix mat;
    VGfloat     vmat[9];
    
    mat = bitmap_matrix;

    vgSetParameteri (_paint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
    vgPaintPattern (_paint, _image);
    
    mat.invert();
    memset(vmat, 0, sizeof(vmat));
    vmat[0] = mat.sx  / 65536.0f;
    vmat[1] = mat.shx / 65536.0f;
    vmat[3] = mat.shy / 65536.0f;
    vmat[4] = mat.sy  / 65536.0f;
    vmat[6] = mat.tx;
    vmat[7] = mat.ty;
    
    vgSeti (VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
    vgLoadMatrix (vmat);
    vgSeti (VG_MATRIX_MODE, VG_MATRIX_STROKE_PAINT_TO_USER);
    vgLoadMatrix (vmat);
    vgSeti (VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);

    if (wrap_mode == WRAP_CLAMP) {  
        vgSetParameteri (_paint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_PAD);
    } else {
        vgSetParameteri (_paint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_REPEAT);
    }
}

template<typename C, typename T, typename R, typename A>
void 
for_each(C& container, R (T::*pmf)(const A&),const A& arg)
{
    std::for_each(container.begin(), container.end(),
                  boost::bind(pmf, _1, boost::ref(arg)));
}

Renderer_ovg::Renderer_ovg()
    : m_display_width(0.0),
      m_display_height(0.0),
      _drawing_mask(false)
{
    set_scale(1.0f, 1.0f);
    m_fillpaint = vgCreatePaint();
    m_strokepaint = vgCreatePaint();

    // vgSetPaint (m_fillpaint,   VG_FILL_PATH);
    // vgSetPaint (m_strokepaint, VG_STROKE_PATH);
}

void
Renderer_ovg::init(float x, float y)
{
    GNASH_REPORT_FUNCTION;
#if 0
    static const VGubyte sqrCmds[5] = {VG_MOVE_TO_ABS, VG_HLINE_TO_ABS, VG_VLINE_TO_ABS, VG_HLINE_TO_ABS, VG_CLOSE_PATH};
    static const VGfloat sqrCoords[5]   = {50.0f, 50.0f, 250.0f, 250.0f, 50.0f};
    path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1, 0, 0, 0,
                        VG_PATH_CAPABILITY_APPEND_TO);
    vgAppendPathData(path, 5, sqrCmds, sqrCoords);
    
    fill = vgCreatePaint();
    vgSetParameterfv(fill, VG_PAINT_COLOR, 4, color);
    vgSetPaint(fill, VG_FILL_PATH);
    
    vgSetfv(VG_CLEAR_COLOR, 4, white_color);
    vgSetf(VG_STROKE_LINE_WIDTH, 10);
    vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
    vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_ROUND);
    vgSetf(VG_STROKE_MITER_LIMIT, 4.0f);
#else
    m_display_width = x;
    m_display_height = y;
    
    // Turn on alpha blending.
    vgSeti (VG_BLEND_MODE, VG_BLEND_SRC_OVER);
    
    vgSeti(VG_RENDERING_QUALITY, GNASH_RENDER_QUALITY);
    vgSetf(VG_STROKE_LINE_WIDTH, 20.0f);
    
    VGfloat clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    vgSetfv( VG_CLEAR_COLOR, 4, clearColor );
    
#ifdef OPENVG_VERSION_1_1    
    m_mask = vgCreateMaskLayer(x, y);
#endif
#endif
    log_debug("VG Vendor is %s, VG Version is %s, VG Renderer is %s",
              vgGetString(VG_VENDOR), vgGetString(VG_VERSION),
              vgGetString(VG_RENDERER));
    log_debug("VG Extensions are: ", vgGetString(VG_EXTENSIONS));
    printVGParams();
}

Renderer_ovg::~Renderer_ovg()
{
    vgDestroyPaint(m_fillpaint);
    vgDestroyPaint(m_strokepaint);
#ifdef OPENVG_VERSION_1_1    
    vgDestroyMaskLayer(m_mask);
#endif
}

#if 0
FIXME
    // Given an image, returns a pointer to a bitmap_info class
// that can later be passed to fill_styleX_bitmap(), to set a
// bitmap fill style.
gnash::BitmapInfo *
Renderer_ovg::createBitmapInfo(std::auto_ptr<GnashImage> im)
{
    GNASH_REPORT_FUNCTION;
    
    // OpenVG don't support 24bit RGB, need translate colorspace
    switch (im->type()) {
      case GNASH_IMAGE_RGBA:   
          return new bitmap_info_ovg(im.get(), VG_sARGB_8888, m_fillpaint);
      default:
          std::abort();
    }
}
#endif

// Since we store drawing operations in display lists, we take special care
// to store video frame operations in their own display list, lest they be
// anti-aliased with the rest of the drawing. Since display lists cannot be
// concatenated this means we'll add up with several display lists for normal
// drawing operations.
void
Renderer_ovg::drawVideoFrame(image::GnashImage* /* frame */, const SWFMatrix* /* m */,
                             const SWFRect* /* bounds */, bool /*smooth*/)
{
    log_unimpl("drawVideoFrame");  
}

void
Renderer_ovg::world_to_pixel(int& x, int& y, float world_x, float world_y)
{
    // negative pixels seems ok here... we don't
    // clip to valid range, use world_to_pixel(rect&)
    // and Intersect() against valid range instead.
    point p(world_x, world_y);
    stage_matrix.transform(p);
    x = (int)p.x;
    y = (int)p.y;
}

geometry::Range2d<int>
Renderer_ovg::world_to_pixel(const SWFRect& wb)
{
    using namespace gnash::geometry;
    
    if ( wb.is_null() ) return Range2d<int>(nullRange);
    if ( wb.is_world() ) return Range2d<int>(worldRange);
    
    int xmin, ymin, xmax, ymax;
    
    world_to_pixel(xmin, ymin, wb.get_x_min(), wb.get_y_min());
    world_to_pixel(xmax, ymax, wb.get_x_max(), wb.get_y_max());
    
    return Range2d<int>(xmin, ymin, xmax, ymax);
}

geometry::Range2d<int>
Renderer_ovg::world_to_pixel(const geometry::Range2d<float>& wb)
{
    if (wb.isNull() || wb.isWorld()) return wb;
    
    int xmin, ymin, xmax, ymax;
    
    world_to_pixel(xmin, ymin, wb.getMinX(), wb.getMinY());
    world_to_pixel(xmax, ymax, wb.getMaxX(), wb.getMaxY());
    
    return geometry::Range2d<int>(xmin, ymin, xmax, ymax);
}

point
Renderer_ovg::pixel_to_world(int x, int y)
{
    point p(x, y);
    SWFMatrix mat = stage_matrix;
    mat.invert().transform(p);
    return p;
};

void
Renderer_ovg::begin_display(const rgba& /* bg_color */, int /* viewport_x0 */,
                            int /* viewport_y0 */, int /* viewport_width */,
                            int /* viewport_height */, float x0, float x1,
                            float y0, float y1)
{
    // GNASH_REPORT_FUNCTION;
    
    vgSeti (VG_MASKING, VG_FALSE);
    
    float mat[9];
    memset(mat, 0, sizeof(mat));
    mat[0] = (float)m_display_width / float(x1 - x0);  // scale sx
    mat[1] = 0; // shx
    mat[3] = 0; // shy
    mat[4] = -((float)m_display_height / float(y1 - y0)); // scale sy
    mat[6] = 0;   // shift tx
    mat[7] = m_display_height;   // shift ty
    
    vgSeti (VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadMatrix (mat);
}

void
Renderer_ovg::end_display()
{
    // GNASH_REPORT_FUNCTION;
    
}

/// Draw a line-strip directly, using a thin, solid line. 
//
/// Can be used to draw empty boxes and cursors.
void
Renderer_ovg::drawLine(const std::vector<point>& coords, const rgba& fill,
                       const SWFMatrix& mat)
{
    VGubyte     gseg[MAX_SEG];
    VGfloat     gdata[MAX_SEG*3*2];
    int         scount = 0;
    int         dcount = 0;
    
    if (coords.empty()) return;
    
    eglScopeMatrix scope_mat(mat);
    
    VGfloat color[] = {
        fill.m_r / 255.0f,
        fill.m_g / 255.0f,
        fill.m_b / 255.0f,
        1.0f
    };
    VGPath      vg_path;
    vg_path = vgCreatePath (VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1, 0, 0, 0,
                            VG_PATH_CAPABILITY_ALL);
    vgSetf (VG_FILL_RULE, VG_EVEN_ODD );
    vgSetParameteri (m_strokepaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv (m_strokepaint, VG_PAINT_COLOR, 4, color);
    
    std::vector<point>::const_iterator  it = coords.begin();
    std::vector<point>::const_iterator end = coords.end();
    
    gseg[scount++] = VG_MOVE_TO;
    gdata[dcount++] = (float)(*it).x;
    gdata[dcount++] = (float)(*it).y;
    ++it;            
    
    for (; it != end; ++it) {
        gseg[scount++] = VG_LINE_TO;
        gdata[dcount++] = (float)(*it).x;
        gdata[dcount++] = (float)(*it).y;
        if (scount >= MAX_SEG-1) {
            vgAppendPathData(vg_path, scount, gseg, gdata);
            scount = 0;
            dcount = 0;
        }
    }
    
    if (scount > 0) {
        vgAppendPathData(vg_path, scount, gseg, gdata);
        
    }
    
    vgSetf (VG_STROKE_JOIN_STYLE, VG_JOIN_ROUND);
    vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_ROUND);
    vgSetf (VG_STROKE_LINE_WIDTH, 20.0f);
    
    vgDrawPath (vg_path, VG_STROKE_PATH);
    vgDestroyPath(vg_path);
}

void
Renderer_ovg::drawPoly(const point* corners, size_t corner_count, 
                       const rgba& fill, const rgba& /* outline */,
                       const SWFMatrix& mat, bool /* masked */)
{
    VGubyte     gseg[MAX_SEG];
    VGfloat     gdata[MAX_SEG*3*2];
    int         scount = 0;
    int         dcount = 0;
    
    if (corner_count < 1) {
        return;
    }
    
    unsigned int i;
    
    eglScopeMatrix scope_mat(mat);
    
    VGfloat color[] = {
        fill.m_r / 255.0f,
        fill.m_g / 255.0f,
        fill.m_b / 255.0f,
        fill.m_a / 255.0f
    };
    VGPath      vg_path;
    vg_path = vgCreatePath (VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1, 0, 0, 0,
                            VG_PATH_CAPABILITY_ALL);
    vgSetf (VG_FILL_RULE, VG_NON_ZERO );
    
    vgSetParameteri (m_fillpaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv (m_fillpaint, VG_PAINT_COLOR, 4, color);
    
    const point *ptr = corners;
    gseg[scount++] = VG_MOVE_TO;
    gdata[dcount++] = (float)ptr->x;
    gdata[dcount++] = (float)ptr->y;
    ptr++;
    
    for (i = 1; i < corner_count; i++) {
        gseg[scount++] = VG_LINE_TO;
        gdata[dcount++] = (float)ptr->x;
        gdata[dcount++] = (float)ptr->y;
        ptr++;
        if (scount >= MAX_SEG-2) {
            vgAppendPathData(vg_path, scount, gseg, gdata);
            scount = 0;
            dcount = 0;
        }
    }
    gseg[scount++] = VG_CLOSE_PATH;
    vgAppendPathData (vg_path, scount, gseg, gdata);
    
    vgDrawPath (vg_path, VG_FILL_PATH);
    vgDestroyPath(vg_path);
}

void
Renderer_ovg::set_antialiased(bool /* enable */)
{
    log_unimpl("set_antialiased");
}

void
Renderer_ovg::begin_submit_mask()
{
    PathVec mask;
    _masks.push_back(mask);
    
    _drawing_mask = true;
    
}

void
Renderer_ovg::end_submit_mask()
{
    _drawing_mask = false;
    
    apply_mask();
}

/// Apply the current mask; nesting is supported.
///
/// This method marks the stencil buffer by incrementing every stencil pixel
/// by one every time a solid from one of the current masks is drawn. When
/// all the mask solids are drawn, we change the stencil operation to permit
/// only drawing where all masks have drawn, in other words, where all masks
/// intersect, or in even other words, where the stencil pixel buffer equals
/// the number of masks.
void
Renderer_ovg::apply_mask()
{  
    if (_masks.empty()) {
        return;
    }
    
    float mat[9];
    float omat[9];
    
    vgSeti (VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgGetMatrix (omat);
    
    memset(mat, 0, sizeof(mat));
    mat[0] =  stage_matrix.get_x_scale(); // scale sx
    mat[1] =  0.0f; // shx
    mat[3] =  0.0f; // shy
    mat[4] =  -stage_matrix.get_x_scale(); // scale sy
    mat[6] =  0;    // shift tx
    mat[7] =  m_display_height;   // shift ty
    vgLoadMatrix(mat);
    
#ifdef OPENVG_VERSION_1_1    
    vgMask(m_mask, VG_FILL_MASK, 0, 0, m_display_width, m_display_height); FIXME
#endif
// Call add_paths for each mask.
    std::for_each(_masks.begin(), _masks.end(),
                  boost::bind(&Renderer_ovg::add_paths, this, _1));    
    vgSeti(VG_MASKING, VG_TRUE);      
    vgLoadMatrix (omat);
}

void
Renderer_ovg::add_paths(const PathVec& path_vec)
{
    SWFCxForm dummy_cx;
#if 0
  FIXME:
    
    FillStyle coloring
        coloring.setSolid(rgba(0,255,0,255));
    
    draw_submask(path_vec, SWFMatrix(), dummy_cx, coloring);
#endif
}

void
Renderer_ovg::disable_mask()
{
    _masks.pop_back();
    
    if (_masks.empty()) {
        vgSeti (VG_MASKING, VG_FALSE);
    } else {
        apply_mask();
    }
    
}

Path
Renderer_ovg::reverse_path(const Path& cur_path)
{
    const Edge& cur_end = cur_path.m_edges.back();    
    
    float prev_cx = cur_end.cp.x;
    float prev_cy = cur_end.cp.y;        
    
    Path newpath(cur_end.ap.x, cur_end.ap.y, cur_path.m_fill1, cur_path.m_fill0, cur_path.m_line, cur_path.m_new_shape);
    
    float prev_ax = cur_end.ap.x;
    float prev_ay = cur_end.ap.y; 
    
    for (std::vector<Edge>::const_reverse_iterator it = cur_path.m_edges.rbegin()+1, end = cur_path.m_edges.rend();
         it != end; ++it) {
        const Edge& cur_edge = *it;
        
        if (prev_ax == prev_cx && prev_ay == prev_cy) {
            prev_cx = cur_edge.ap.x;
            prev_cy = cur_edge.ap.y;      
        }
        
        Edge newedge(prev_cx, prev_cy, cur_edge.ap.x, cur_edge.ap.y); 
        
        newpath.m_edges.push_back(newedge);
        
        prev_cx = cur_edge.cp.x;
        prev_cy = cur_edge.cp.y;
        prev_ax = cur_edge.ap.x;
        prev_ay = cur_edge.ap.y;
        
    }
    
    Edge newlastedge(prev_cx, prev_cy, cur_path.ap.x, cur_path.ap.y);    
    newpath.m_edges.push_back(newlastedge);
    
    return newpath;
}

const Path *
Renderer_ovg::find_connecting_path(const Path& to_connect,
                                   std::list<const Path*> path_refs)
{        
    float target_x = to_connect.m_edges.back().ap.x;
    float target_y = to_connect.m_edges.back().ap.y;
    
    if (target_x == to_connect.ap.x &&
        target_y == to_connect.ap.y) {
        return NULL;
    }
    
    for (std::list<const Path*>::const_iterator it = path_refs.begin(),
             end = path_refs.end(); it != end; ++it) {
        const Path* cur_path = *it;
        
        if (cur_path == &to_connect) {
            
            continue;
            
        }
        
        if (cur_path->ap.x == target_x && cur_path->ap.y == target_y) {
            if (cur_path->m_fill1 != to_connect.m_fill1) {
                continue;
            }
            return cur_path;
        }
    }    
    
    return NULL;  
}

PathVec
Renderer_ovg::normalize_paths(const PathVec &paths)
{
    PathVec normalized;
    
    for (PathVec::const_iterator it = paths.begin(), end = paths.end();
         it != end; ++it) {
        const Path& cur_path = *it;
        
        if (cur_path.m_edges.empty()) {
            continue;
            
        } else if (cur_path.m_fill0 && cur_path.m_fill1) {     
            
            // Two fill styles; duplicate and then reverse the left-filled one.
            normalized.push_back(cur_path);
            normalized.back().m_fill0 = 0; 
            
            Path newpath = reverse_path(cur_path);
            newpath.m_fill0 = 0;        
            
            normalized.push_back(newpath);       
            
        } else if (cur_path.m_fill0) {
            // Left fill style.
            Path newpath = reverse_path(cur_path);
            newpath.m_fill0 = 0;
            
            normalized.push_back(newpath);
        } else if (cur_path.m_fill1) {
            // Right fill style.
            normalized.push_back(cur_path);
        } else {
            // No fill styles; copy without modifying.
            normalized.push_back(cur_path);
        }
        
    }
    
    return normalized;
}


/// Analyzes a set of paths to detect real presence of fills and/or outlines
/// TODO: This should be something the character tells us and should be 
/// cached. 
void
Renderer_ovg::analyze_paths(const PathVec &paths, bool& have_shape,
                            bool& have_outline) 
{
    have_shape=false;
    have_outline=false;
    
    int pcount = paths.size();
    
    for (int pno=0; pno<pcount; pno++) {
        
        const Path &the_path = paths[pno];
        
        if ((the_path.m_fill0>0) || (the_path.m_fill1>0)) {
            have_shape=true;
            if (have_outline) return; // have both
        }
        
        if (the_path.m_line>0) {
            have_outline=true;
            if (have_shape) return; // have both
        }
        
    }    
}

void
Renderer_ovg::apply_fill_style(const FillStyle& style, const SWFMatrix& /* mat */,
                               const SWFCxForm& cx)
{
#if 0
    FIXME fill_style changed to FillStyle
        int fill_type = style.get_type();
    
    switch (fill_type) {
        
      case SWF::FILL_LINEAR_GRADIENT:
      case SWF::FILL_RADIAL_GRADIENT:
      case SWF::FILL_FOCAL_GRADIENT:
      {
          const bitmap_info_ovg* binfo = static_cast<const bitmap_info_ovg*>(
              style.need_gradient_bitmap(*this));       
          
          binfo->apply(style.getGradientMatrix(), bitmap_info_ovg::WRAP_CLAMP); 
          vgSetParameteri (m_fillpaint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
          break;
      }        
      case SWF::FILL_TILED_BITMAP_HARD:
      case SWF::FILL_TILED_BITMAP:
      {
          const bitmap_info_ovg* binfo = static_cast<const bitmap_info_ovg*>(
              style.get_bitmap_info(*this));
          
          binfo->apply(style.getBitmapMatrix(), bitmap_info_ovg::WRAP_REPEAT);
          vgSetParameteri (m_fillpaint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
          break;
      }
      
      case SWF::FILL_CLIPPED_BITMAP:
      case SWF::FILL_CLIPPED_BITMAP_HARD:
      {     
          const bitmap_info_ovg* binfo = dynamic_cast<const bitmap_info_ovg*>(
              style.get_bitmap_info(*this));
          
          binfo->apply(style.getBitmapMatrix(), bitmap_info_ovg::WRAP_CLAMP);
          vgSetParameteri (m_fillpaint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
          break;
      } 
      
      case SWF::FILL_SOLID:
      {
          rgba c = cx.transform(style.get_color());
          VGfloat color[] = {
              c.m_r / 255.0f,
              c.m_g / 255.0f,
              c.m_b / 255.0f,
              c.m_a / 255.0f
          };
          
          vgSetParameteri (m_fillpaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
          vgSetParameterfv (m_fillpaint, VG_PAINT_COLOR, 4, color);
      }
      
    } // switch
#endif
}

bool
Renderer_ovg::apply_line_style(const LineStyle& style, const SWFCxForm& cx, const SWFMatrix& mat)
{
    
    bool rv = true;
    
    switch(style.joinStyle()) {
      case JOIN_ROUND:
          vgSetf (VG_STROKE_JOIN_STYLE, VG_JOIN_ROUND);
          break;
      case JOIN_BEVEL:
          vgSetf (VG_STROKE_JOIN_STYLE, VG_JOIN_BEVEL);
          break;
      case JOIN_MITER:
          vgSetf (VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
          break;
      default:
          log_unimpl("join style");
    }
    
    switch(style.startCapStyle()) {
      case CAP_ROUND:
          vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_ROUND);
          break;
      case CAP_NONE:
          vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
          break;
      case CAP_SQUARE:
          vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_SQUARE);
          break;
      default:
          log_unimpl("cap style");
    }
    
    vgSetf (VG_STROKE_MITER_LIMIT, style.miterLimitFactor());
    
    float width = style.getThickness();
    
    if (!width) {
        vgSetf(VG_STROKE_LINE_WIDTH, 20.0f);
        rv = false; // Don't draw rounded lines.
    } else if ( (!style.scaleThicknessVertically()) && (!style.scaleThicknessHorizontally()) ) {
        vgSetf(VG_STROKE_LINE_WIDTH, width);
    } else {
        if ( (!style.scaleThicknessVertically()) || (!style.scaleThicknessHorizontally()) ) {
            LOG_ONCE( log_unimpl(_("Unidirectionally scaled strokes in OGL renderer")) );
        }
        
        float stroke_scale = fabsf(mat.get_x_scale()) + fabsf(mat.get_y_scale());
        stroke_scale /= 2.0f;
        stroke_scale *= (stage_matrix.get_x_scale() + stage_matrix.get_y_scale()) / 2.0f;
        width *= stroke_scale;
        
        if (width < 20.0f) {
            width = 20.0f;
        }
        
        vgSetf(VG_STROKE_LINE_WIDTH, width);
    }
    
    rgba c = cx.transform(style.get_color());
    VGfloat color[] = {
        c.m_r / 255.0f,
        c.m_g / 255.0f,
        c.m_b / 255.0f,
        c.m_a / 255.0f
    };
    
    
    vgSetParameteri (m_strokepaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv (m_strokepaint, VG_PAINT_COLOR, 4, color);
    
    return rv;
}

typedef std::vector<const Path*> PathPtrVec;
  
void
Renderer_ovg::draw_outlines(const PathVec& path_vec, const SWFMatrix& mat,
                            const SWFCxForm& cx, const std::vector<LineStyle>& line_styles)
{
    for (PathVec::const_iterator it = path_vec.begin(), end = path_vec.end();
         it != end; ++it) {
        
        const Path& cur_path = *it;
        
        if (!cur_path.m_line) {
            continue;
        }
        
        VGPath      vpath;
        vpath = vgCreatePath (VG_PATH_FORMAT_STANDARD,
                              VG_PATH_DATATYPE_F,
                              1, 0, 0, 0,
                              VG_PATH_CAPABILITY_ALL);
        vgSetf (VG_FILL_RULE, VG_EVEN_ODD );
        startpath(vpath, cur_path.ap.x, cur_path.ap.y);
        preparepath(vpath, cur_path.m_edges, cur_path.ap.x,
                    cur_path.ap.y);
        
        apply_line_style(line_styles[cur_path.m_line-1], cx, mat);
        vgDrawPath (vpath, VG_STROKE_PATH);
        vgDestroyPath(vpath);
    }
}

std::list<PathPtrVec>
Renderer_ovg::get_contours(const PathPtrVec &paths)
{
    std::list<const Path*> path_refs;
    std::list<PathPtrVec> contours;
    
    for (PathPtrVec::const_iterator it = paths.begin(), end = paths.end();
         it != end; ++it) {
        const Path* cur_path = *it;
        path_refs.push_back(cur_path);
    }
    
    for (std::list<const Path*>::const_iterator it = path_refs.begin(), end = path_refs.end();
         it != end; ++it) {
        const Path* cur_path = *it;
        
        if (cur_path->m_edges.empty()) {
            continue;
        }
        
        if (!cur_path->m_fill0 && !cur_path->m_fill1) {
            continue;
        }
        
        PathPtrVec contour;
        
        contour.push_back(cur_path);
        
        const Path* connector = find_connecting_path(*cur_path, path_refs);
        
        while (connector) {       
            contour.push_back(connector);
            
            const Path* tmp = connector;
            connector = find_connecting_path(*connector, std::list<const Path*>(boost::next(it), end));
            
            // make sure we don't iterate over the connecting path in the for loop.
            path_refs.remove(tmp);
            
        } 
        
        contours.push_back(contour);   
    }
    
    return contours;
}

void
Renderer_ovg::draw_mask(const PathVec& path_vec)
{ 
   
    for (PathVec::const_iterator it = path_vec.begin(), end = path_vec.end();
         it != end; ++it) {
        const Path& cur_path = *it;
        
        if (cur_path.m_fill0 || cur_path.m_fill1) {
            _masks.back().push_back(cur_path);
            _masks.back().back().m_line = 0;    
        }
    }  
    
}

PathPtrVec
Renderer_ovg::paths_by_style(const PathVec& path_vec, unsigned int style)
{
    PathPtrVec paths;
    for (PathVec::const_iterator it = path_vec.begin(), end = path_vec.end();
         it != end; ++it) {
        const Path& cur_path = *it;
        
        if (cur_path.m_fill0 == style) {
            paths.push_back(&cur_path);
        }
        
        if (cur_path.m_fill1 == style) {
            paths.push_back(&cur_path);
        }
        
    }
    return paths;
}


std::vector<PathVec::const_iterator>
Renderer_ovg::find_subshapes(const PathVec& path_vec)
{
    std::vector<PathVec::const_iterator> subshapes;
    
    PathVec::const_iterator it = path_vec.begin(),
        end = path_vec.end();
    
    subshapes.push_back(it);
    ++it;
    
    for (;it != end; ++it) {
        const Path& cur_path = *it;
        
        if (cur_path.m_new_shape) {
            subshapes.push_back(it); 
        } 
    }
    
    if (subshapes.back() != end) {
        subshapes.push_back(end);
    }
    
    return subshapes;
}

/// Takes a path and translates it using the given SWFMatrix.
void
Renderer_ovg::apply_matrix_to_paths(std::vector<Path>& paths, const SWFMatrix& mat)
{  
    std::for_each(paths.begin(), paths.end(),
                  boost::bind(&Path::transform, _1, boost::ref(mat)));
}  

void
Renderer_ovg::draw_subshape(const PathVec& path_vec,
                            const SWFMatrix& mat,
                            const SWFCxForm& cx,
                            const std::vector<FillStyle>& fill_styles,
                            const std::vector<LineStyle>& line_styles)
{
    PathVec normalized = normalize_paths(path_vec);
    
    for (size_t i = 0; i < fill_styles.size(); ++i) {
        PathPtrVec paths = paths_by_style(normalized, i+1);
        
        if (!paths.size()) {
            continue;
        }
        
        std::list<PathPtrVec> contours = get_contours(paths);
        
        apply_fill_style(fill_styles[i], mat, cx);
        
        VGPath      vg_path;
        vg_path = vgCreatePath (VG_PATH_FORMAT_STANDARD,
                                VG_PATH_DATATYPE_F,
                                1, 0, 0, 0,
                                VG_PATH_CAPABILITY_ALL);              
        vgSetf (VG_FILL_RULE, VG_EVEN_ODD );                                  
        
        for (std::list<PathPtrVec>::const_iterator iter = contours.begin(),
                 final = contours.end(); iter != final; ++iter) {
            const PathPtrVec& refs = *iter;
            
            startpath(vg_path, (*(refs[0])).ap.x, (*(refs[0])).ap.y);
            
            for (PathPtrVec::const_iterator it = refs.begin(), end = refs.end();
                 it != end; ++it) {
                const Path& cur_path = *(*it);
                if (!cur_path.m_fill0 && !cur_path.m_fill1)
                    continue;
                preparepath(vg_path, cur_path.m_edges, cur_path.ap.x,
                            cur_path.ap.y);
            }
            closepath(vg_path);
        }
        vgDrawPath (vg_path, VG_FILL_PATH);
        vgDestroyPath(vg_path);    
    }
    
    draw_outlines(normalized, mat, cx, line_styles);
}

void
Renderer_ovg::draw_submask(const PathVec& path_vec,
                           const SWFMatrix& /* mat */,
                           const SWFCxForm& /* cx */,
                           const FillStyle& /* f_style */)
{
    
    PathVec normalized = normalize_paths(path_vec);
    
    PathPtrVec paths = paths_by_style(normalized, 1);
    
    std::list<PathPtrVec> contours = get_contours(paths);
    
    VGfloat color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    vgSetParameteri (m_fillpaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv (m_fillpaint, VG_PAINT_COLOR, 4, color);
    
    
    VGPath      vg_path;
    vg_path = vgCreatePath (VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1, 0, 0, 0,
                            VG_PATH_CAPABILITY_ALL);
    vgSetf (VG_FILL_RULE, VG_EVEN_ODD );
    
    for (std::list<PathPtrVec>::const_iterator iter = contours.begin(),
             final = contours.end(); iter != final; ++iter) {
        const PathPtrVec& refs = *iter;
        startpath(vg_path, (*(refs[0])).ap.x, (*(refs[0])).ap.y);
        for (PathPtrVec::const_iterator it = refs.begin(), end = refs.end();
             it != end; ++it) {
            const Path& cur_path = *(*it);
            preparepath(vg_path, cur_path.m_edges, cur_path.ap.x,
                        cur_path.ap.y);
        }
        closepath(vg_path);
    }
    
#ifdef OPENVG_VERSION_1_1    
    vgRenderToMask(vg_path, VG_FILL_PATH, VG_INTERSECT_MASK); FIXME
#endif
                                                                  vgDestroyPath(vg_path);
}

// Drawing procedure:
// 1. Separate paths by subshape.
// 2. Separate subshapes by fill style.
// 3. For every subshape/fill style combo:
//  a. Separate contours: find closed shapes by connecting ends.
//  b. Apply fill style.
//  c. Feed the contours in the tesselator. (Render.)
//  d. Draw outlines for every path in the subshape with a line style.

void
Renderer_ovg::drawShape(const SWF::ShapeRecord& shape, const SWFCxForm& cx,
                        const SWFMatrix& mat)
{
    const PathVec& path_vec = shape.paths();
    
    if (!path_vec.size()) {
        // No paths. Nothing to draw...
        return;
    }
    if (_drawing_mask) {
        PathVec scaled_path_vec = path_vec;
        apply_matrix_to_paths(scaled_path_vec, mat);
        draw_mask(scaled_path_vec); 
        return;
    }    
    
    bool have_shape, have_outline;
    
    analyze_paths(path_vec, have_shape, have_outline);
    
    if (!have_shape && !have_outline) {
        return;
    }    
    
    eglScopeMatrix scope_mat(mat);
    
    std::vector<PathVec::const_iterator> subshapes = find_subshapes(path_vec);
    
    const std::vector<FillStyle>& fill_styles = shape.fillStyles();
    const std::vector<LineStyle>& line_styles = shape.lineStyles();
    
    for (size_t i = 0; i < subshapes.size()-1; ++i) {
        PathVec subshape_paths;
        
        if (subshapes[i] != subshapes[i+1]) {
            subshape_paths = PathVec(subshapes[i], subshapes[i+1]);
        } else {
            subshape_paths.push_back(*subshapes[i]);
        }
        
        draw_subshape(subshape_paths, mat, cx, fill_styles,
                      line_styles);
    }
}

void
Renderer_ovg::drawGlyph(const SWF::ShapeRecord& rec, const rgba& c,
                        const SWFMatrix& mat)
{
    if (_drawing_mask) abort();
    
    if (rec.getBounds().is_null()) {
        return;
    }
    
    SWFCxForm dummy_cx;
    std::vector<FillStyle> glyph_fs;
    
#if 0
    FIXME fill_style is now FillStyle
        FillStyle coloring;
    coloring.setSolid(c);
    
    glyph_fs.push_back(coloring);
#endif
    std::vector<LineStyle> dummy_ls;
    
    eglScopeMatrix scope_mat(mat);
    
    draw_subshape(rec.paths(), mat, dummy_cx, glyph_fs, dummy_ls);
    
}

void
Renderer_ovg::set_scale(float xscale, float yscale) {
    _xscale = xscale;
    _yscale = yscale;
    stage_matrix.set_identity();
    stage_matrix.set_scale(xscale/20.0f, yscale/20.0f);
}

void
Renderer_ovg::set_invalidated_regions(const InvalidatedRanges& /* ranges */)
{
}
  
DSOEXPORT Renderer *
create_handler(const char */* pixelformat */)
{
    Renderer_ovg *renderer = new Renderer_ovg;
    return renderer;
}


// These methods are only for debugging and development
void
Renderer_ovg::printVGParams()
{
    // vgGetVectorSize();

    std::string str;
    switch(vgGeti(VG_MATRIX_MODE)) {
      case VG_MATRIX_PATH_USER_TO_SURFACE:
          str = "VG_MATRIX_PATH_USER_TO_SURFACE";
          break;
      case VG_MATRIX_IMAGE_USER_TO_SURFACE:
          str = "VG_MATRIX_IMAGE_USER_TO_SURFACE";
          break;
      case VG_MATRIX_FILL_PAINT_TO_USER:
          str = "VG_MATRIX_FILL_PAINT_TO_USER";
          break;
      case VG_MATRIX_STROKE_PAINT_TO_USER:
          str = "VG_MATRIX_STROKE_PAINT_TO_USER";
          break;
      case VG_MATRIX_MODE_FORCE_SIZE:
          str = "VG_MATRIX_MODE_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_MATRIX_MODE!");
    }
    log_debug("VG_MATRIX_MODE is %s", str);
    str.clear();

    switch(vgGeti(VG_FILL_RULE)) {
      case VG_EVEN_ODD:
          str = "VG_EVEN_ODD";
          break;
      case VG_NON_ZERO:
          str = "VG_NON_ZERO";
          break;
      default:
          log_error("unsupported VG_FILL_RULE!");          
    }
    log_debug("VG_FILL_RULE is %s", str);
    str.clear();

    switch(vgGeti(VG_IMAGE_QUALITY)) {
      case VG_IMAGE_QUALITY_NONANTIALIASED:
          str = "VG_IMAGE_QUALITY_NONANTIALIASED";
          break;
      case VG_IMAGE_QUALITY_FASTER:
          str = "VG_IMAGE_QUALITY_FASTER";
          break;
      case VG_IMAGE_QUALITY_BETTER:
          str = "VG_IMAGE_QUALITY_BETTER";
          break;
      case VG_IMAGE_QUALITY_FORCE_SIZE:
          str = "VG_IMAGE_QUALITY_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_IMAGE_QUALITY!");
    }
    log_debug("VG_IMAGE_QUALITY is %s", str);
    str.clear();
    
    switch(vgGeti(VG_RENDERING_QUALITY)) {
      case VG_RENDERING_QUALITY_NONANTIALIASED:
          str = "VG_RENDERING_QUALITY_NONANTIALIASED";
          break;
      case VG_RENDERING_QUALITY_FASTER:
          str = "VG_RENDERING_QUALITY_FASTER";
          break;
      case VG_RENDERING_QUALITY_BETTER:
          str = "VG_RENDERING_QUALITY_BETTER";
          break;
      case VG_RENDERING_QUALITY_FORCE_SIZE:
          str = "VG_RENDERING_QUALITY_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_RENDERING_QUALITY!");
    }
    log_debug("VG_RENDERING_QUALITY is %s", str);
    str.clear();
    
    switch(vgGeti(VG_BLEND_MODE)) {
      case VG_BLEND_SRC:
          str = "VG_BLEND_SRC";
          break;
      case VG_BLEND_SRC_OVER:
          str = "VG_BLEND_SRC_OVER";
          break;
      case VG_BLEND_DST_OVER:
          str = "VG_BLEND_DST_OVER";
          break;
      case VG_BLEND_SRC_IN:
          str = "VG_BLEND_SRC_IN";
          break;
      case VG_BLEND_DST_IN:
          str = "VG_BLEND_DST_IN";
          break;
      case VG_BLEND_MULTIPLY: 
          str = "VG_BLEND_MULTIPLY";
          break;
      case VG_BLEND_SCREEN:
          str = "VG_BLEND_SCREEN";
          break;
      case VG_BLEND_DARKEN:
          str = "VG_BLEND_DARKEN";
          break;
      case VG_BLEND_LIGHTEN:
          str = "VG_BLEND_LIGHTEN";
          break;
      case VG_BLEND_ADDITIVE:
          str = "VG_BLEND_ADDITIVE";
          break;
      default:
          log_error("unsupported VG_BLEND_MODE!");
    }
    log_debug("VG_BLEND_MODE is %s", str);    
    str.clear();
    
    switch(vgGeti(VG_IMAGE_MODE)) {
      case VG_DRAW_IMAGE_NORMAL:
          str = "VG_DRAW_IMAGE_MULTIPLY";
          break;
      case VG_DRAW_IMAGE_MULTIPLY:
          str = "VG_DRAW_IMAGE_MULTIPLY";
          break;
      case VG_DRAW_IMAGE_STENCIL:
          str = "VG_DRAW_IMAGE_STENCIL";
          break;
      case VG_IMAGE_MODE_FORCE_SIZE:
          str = "VG_IMAGE_MODE_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_IMAGE_MODE!");
    }
    log_debug("VG_IMAGE_MODE is %s", str);
    str.clear();
    
    log_debug("VG_STROKE_LINE_WIDTH is %d", vgGeti(VG_STROKE_LINE_WIDTH));    
    str.clear();
    
    switch(vgGeti(VG_STROKE_CAP_STYLE)) {
      case VG_CAP_BUTT:
          str = "VG_CAP_BUTT";
          break;
      case VG_CAP_ROUND:
          str = "VG_CAP_ROUND";
          break;
      case VG_CAP_SQUARE:
          str = "VG_CAP_SQUARE";
          break;
      case VG_CAP_STYLE_FORCE_SIZE:
          str = "VG_CAP_STYLE_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_STROKE_CAP_STYLE!");
    }
    log_debug("VG_STROKE_CAP_STYLE is %s", str);
    str.clear();
    
    switch(vgGeti(VG_STROKE_JOIN_STYLE)) {
      case VG_JOIN_MITER:
          str = "VG_JOIN_MITER";
          break;
      case VG_JOIN_ROUND:
          str = "VG_JOIN_ROUND";
          break;
      case VG_JOIN_BEVEL:
          str = "VG_JOIN_BEVEL";
          break;
      case VG_JOIN_STYLE_FORCE_SIZE:
          str = "VG_JOIN_STYLE_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_STROKE_JOIN_STYLE!");
    }
    log_debug("VG_STROKE_JOIN_STYLE is %s", str);
    str.clear();
    
    log_debug("VG_STROKE_MITER_LIMIT is %d", vgGeti(VG_STROKE_MITER_LIMIT));
    log_debug("VG_MASKING is %d", vgGeti(VG_MASKING));
    log_debug("VG_SCISSORING is %d", vgGeti(VG_SCISSORING));    
    str.clear();
    
    switch(vgGeti(VG_PIXEL_LAYOUT)) {
      case VG_PIXEL_LAYOUT_UNKNOWN:
          str = "VG_PIXEL_LAYOUT_UNKNOWN";
          break;
      case VG_PIXEL_LAYOUT_RGB_VERTICAL:
          str = "VG_PIXEL_LAYOUT_RGB_VERTICAL";
          break;
      case VG_PIXEL_LAYOUT_BGR_VERTICAL:
          str = "VG_PIXEL_LAYOUT_BGR_VERTICAL";
          break;
      case VG_PIXEL_LAYOUT_RGB_HORIZONTAL:
          str = "VG_PIXEL_LAYOUT_RGB_HORIZONTAL";
          break;
      case VG_PIXEL_LAYOUT_BGR_HORIZONTAL:
          str = "VG_PIXEL_LAYOUT_BGR_HORIZONTAL";
          break;
      case VG_PIXEL_LAYOUT_FORCE_SIZE:
          str = "VG_PIXEL_LAYOUT_FORCE_SIZE";
          break;
      default:
          log_error("unsupported VG_PIXEL_LAYOUT!");
    }
    log_debug("VG_PIXEL_LAYOUT is %s", str);
    
    log_debug("VG_STROKE_DASH_PHASE_RESET is %s",
              (vgGeti(VG_STROKE_DASH_PHASE_RESET) == true) ? "true" : "false");
    log_debug("VG_FILTER_FORMAT_LINEAR is %s",
              (vgGeti(VG_FILTER_FORMAT_LINEAR) == true) ? "true" : "false");
    log_debug("VG_FILTER_FORMAT_PREMULTIPLIED is %s",
              (vgGeti(VG_FILTER_FORMAT_PREMULTIPLIED) == true) ? "true" : "false");
    str.clear();
    
    VGint value = vgGeti(VG_FILTER_CHANNEL_MASK);
    if (value & VG_RED) {
        str += " VG_RED";
    }
    if (value & VG_GREEN) {
        str += " VG_GREEN";
    }
    if (value & VG_BLUE) {
        str += " VG_BLUE";
    }
    if (value & VG_ALPHA) {
        str += " VG_ALPHA";
    }
    log_debug("VG_FILTER_CHANNEL_MASK is %s", str);
    
    log_debug("VG_MAX_IMAGE_WIDTH is %d", vgGeti(VG_MAX_IMAGE_WIDTH));
    log_debug("VG_MAX_IMAGE_HEIGHT is %d", vgGeti(VG_MAX_IMAGE_HEIGHT));
    log_debug("VG_MAX_IMAGE_PIXELS is %d", vgGeti(VG_MAX_IMAGE_PIXELS));
    log_debug("VG_MAX_IMAGE_BYTES is %d", vgGeti(VG_MAX_IMAGE_BYTES));
}

bool
Renderer_ovg::initTestBuffer(unsigned int width, unsigned int height)
{
    int size = width * height; // * getBytesPerPixel();
    
    _testBuffer = static_cast<unsigned char *>(realloc(_testBuffer, size));
    memset(_testBuffer, 0, size);
    printf("\tRenderer Test memory at: %p\n", _testBuffer);
    
//    init_buffer(_testBuffer, size, width, height, width * getBytesPerPixel());
//    init(width, height);
    
    return true;
}

Renderer *
Renderer_ovg::startInternalRender(gnash::image::GnashImage&)
{
}

void
Renderer_ovg::endInternalRender()
{
}

void
Renderer_ovg::drawShape(gnash::SWF::ShapeRecord const&, gnash::Transform const&)
{
}
void
Renderer_ovg::begin_display(gnash::rgba const&, int, int, float, float, float, float)
{

}
void
Renderer_ovg::drawVideoFrame(gnash::image::GnashImage*, gnash::Transform const&, gnash::SWFRect const*, bool)
{

}
CachedBitmap *
Renderer_ovg::createCachedBitmap(std::auto_ptr<gnash::image::GnashImage>)
{

}

unsigned int
Renderer_ovg::getBitsPerPixel()
{
    return 0;
}

#if 0
void
Renderer_ovg::printVGHardware()
{
}

void
Renderer_ovg::printVGPath()
{
}

void
Renderer_ovg::set_scale(float xscale, float yscale)
{
}

void
Renderer_ovg::set_translation(float, float)
{  
}

CachedBitmap *
Renderer_ovg::createCachedBitmap(std::auto_ptr<gnash::image::GnashImage>)
{
}

void
Renderer_ovg::drawLine(const std::vector<point>& coords, const rgba& color,
                            const SWFMatrix& mat)
{
}

void
Renderer_ovg::draw_poly(const point* corners, size_t corner_count,
                           const rgba& fill, const rgba& outline,
                           const SWFMatrix& mat, bool masked)
{
}    

void
Renderer_ovg::drawGlyph(const SWF::ShapeRecord& rec, const rgba& color,
                   const SWFMatrix& mat)
{
}

void
Renderer_ovg::drawVideoFrame(gnash::image::GnashImage*, gnash::Transform const&,
                                  gnash::SWFRect const*, bool)
{
}

void
Renderer_ovg::drawShape(gnash::SWF::ShapeRecord const&, gnash::Transform const&)
{
}

void
Renderer_ovg::renderToImage(boost::shared_ptr<IOChannel> io,FileType type)
{
}
void
Renderer_ovg::set_invalidated_regions(const InvalidatedRanges& ranges)
{
}
void
Renderer_ovg::begin_submit_mask()
{
}
void
Renderer_ovg::end_submit_mask()
{
}
void
Renderer_ovg::disable_mask()
{
}
gnash::geometry::Range2d<int>
Renderer_ovg::world_to_pixel(const gnash::SWFRect&)
{
}
gnash::point
Renderer_ovg::pixel_to_world(int, int)
{
}
void
Renderer_ovg::end_display()
{
}

void
Renderer_ovg::begin_display(gnash::rgba const&, int, int, float,
                                 float, float, float)
{
}

Renderer *
Renderer_ovg::startInternalRender(gnash::image::GnashImage&)
{
}

void
Renderer_ovg::endInternalRender()
{
}

unsigned int
Renderer_ovg::getBitsPerPixel()
{
}

bool
Renderer_ovg::getPixel(gnash::rgba&, int, int)
{
}

#endif

} // namespace gnash::renderer::gles1
} // namespace gnash::renderer
} // namespace gnash

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
