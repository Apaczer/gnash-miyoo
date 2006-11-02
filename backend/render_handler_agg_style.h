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

// 
//

#ifndef BACKEND_RENDER_HANDLER_AGG_STYLE_H
#define BACKEND_RENDER_HANDLER_AGG_STYLE_H

// This include file used only to make render_handler_agg more readable.


// TODO: Instead of re-creating AGG fill styles again and again, they should
// be cached somewhere. 


using namespace gnash;

namespace gnash {

/// Internal style class that represents a fill style. Roughly speaking, AGG 
/// computes the fill areas of a flash composite shape and calls generate_span
/// to generate small horizontal pixel rows. generate_span provides whatever
/// fill pattern for that coordinate. 
class agg_style_base 
{
public:
  // for solid styles:
  bool m_is_solid;
  agg::rgba8 m_color; // defined here for easy access
  
  // for non-solid styles:
  virtual void generate_span(agg::rgba8* span, int x, int y, unsigned len)=0;
};


/// Solid AGG fill style. generate_span is not used in this case as AGG does
/// solid fill styles internally.
class agg_style_solid : public agg_style_base 
{
public:

  agg_style_solid(const agg::rgba8 color) {
    m_is_solid = true;
    m_color = color;
  }

  void generate_span(agg::rgba8* /*span*/, int /*x*/, int /*y*/, unsigned /*len*/)
  {
    assert(0); // never call generate_span for solid fill styles
  }
};


// Simple hack to get rid of that additional parameter for the 
// image_accessor_clip constructor which breaks template usage. 
/*
template <class PixelFormat>
class image_accessor_clip_transp : public agg::image_accessor_clip<PixelFormat>
{
public:
  image_accessor_clip_transp(const PixelFormat& pixf)  
  {
    agg::image_accessor_clip<PixelFormat>::image_accessor_clip(pixf, 
      agg::rgba8_pre(255, 0, 0, 0).premultiply());
  }
};

The image_accessor_clip_transp above does not work correctly. The alpha value
for the background color supplied to image_accessor_clip seems to be ignored
(at least for agg::pixfmt_rgb24). Even if alpha=0 you can see that red color
tone at the borders. So the current workaround is to use image_accessor_clone
which repeats the pixels at the edges. This should be no problem as the clipped
bitmap format is most probably only used for rectangular bitmaps anyway. 
*/
#define image_accessor_clip_transp agg::image_accessor_clone


/// AGG bitmap fill style. There are quite a few combinations possible and so
/// the class types are defined outside. The bitmap can be tiled or clipped.
/// It can have any transformation matrix and color transform. Any pixel format
/// can be used, too. 
template <class PixelFormat, class span_allocator_type, class img_source_type,
  class interpolator_type, class sg_type>
class agg_style_bitmap : public agg_style_base
{
public:
    
  agg_style_bitmap(int width, int height, int rowlen, uint8_t* data, 
    gnash::matrix mat, gnash::cxform cx) :
    
    m_rbuf(data, width, height, rowlen),  
    m_pixf(m_rbuf),
    m_img_src(m_pixf),
    m_tr(),       // initialize later
    m_interpolator(m_tr),
    m_sg(m_img_src, m_interpolator)
  {
  
    m_is_solid = false;
    
    // Convert the transformation matrix to AGG's class. It's basically the
    // same and we could even use gnash::matrix since AGG does not require
    // a real AGG descendant (templates!). However, it's better to use AGG's
    // class as this should be faster (avoid type conversion).
    m_tr=agg::trans_affine(
      mat.m_[0][0], mat.m_[1][0], 
      mat.m_[0][1], mat.m_[1][1], 
      mat.m_[0][2], mat.m_[1][2]);
            
    m_cx = cx;
      
  }
  
  virtual ~agg_style_bitmap() {
  }
    
  void generate_span(agg::rgba8* span, int x, int y, unsigned len)
  {
    m_sg.generate(span, x, y, len);

    // Apply color transform
    // TODO: Check if this can be optimized
    if (!m_cx.is_identity())      
    for (unsigned int i=0; i<len; i++) {
      m_cx.transform(span->r, span->g, span->b, span->a);
      ++span;
    }
  }
    
  
  
private:

  // Color transform
  gnash::cxform m_cx;

  // Pixel access
  agg::rendering_buffer m_rbuf;
  PixelFormat m_pixf;
  
  // Span allocator
  span_allocator_type m_sa;
  
  // Image accessor
  img_source_type m_img_src;
  
  // Transformer
  agg::trans_affine m_tr;
  
  // Interpolator
  interpolator_type m_interpolator;
  
  // Span generator
  sg_type m_sg;  
};


/// AGG gradient fill style. Don't use Gnash texture bitmaps as this is slower
/// and less accurate. Even worse, the bitmap fill would need to be tweaked
/// to have non-repeating gradients (first and last color stops continue 
/// forever on each side). This class can be used for any kind of gradient, so
/// even focal gradients should be possible. 
template <class color_type, class span_allocator_type, class interpolator_type, 
  class gradient_func_type, class gradient_adaptor_type, class color_func_type, 
  class sg_type>
class agg_style_gradient : public agg_style_base {
public:

  agg_style_gradient(const gnash::fill_style& fs, gnash::matrix mat, gnash::cxform cx, int norm_size) :
    m_tr(),
    m_span_interpolator(m_tr),
    m_gradient_func(),
    m_gradient_adaptor(m_gradient_func),
    m_sg(m_span_interpolator, m_gradient_adaptor, m_gradient_lut, 0, norm_size) 
  {
  
    m_is_solid = false;
    
    m_tr=agg::trans_affine(
      mat.m_[0][0], mat.m_[1][0], 
      mat.m_[0][1], mat.m_[1][1], 
      mat.m_[0][2], mat.m_[1][2]);
      
            
    // Built gradient lookup table
    m_gradient_lut.remove_all(); 
    
    for (int i=0; i<fs.get_color_stop_count(); i++) {
    
      const gradient_record gr = fs.get_color_stop(i); 
      
      m_gradient_lut.add_color(gr.m_ratio/255.0, agg::rgba8(gr.m_color.m_r, 
        gr.m_color.m_g, gr.m_color.m_b, gr.m_color.m_a));
        
    } // for
    
    m_gradient_lut.build_lut();
    
    m_cx = cx;

  } // agg_style_gradient constructor


  virtual ~agg_style_gradient() 
  {
  }


  void generate_span(color_type* span, int x, int y, unsigned len) 
  {
    m_sg.generate(span, x, y, len);

    // Apply color transform
    // TODO: Check if this can be optimized
    if (!m_cx.is_identity())      
    for (unsigned int i=0; i<len; i++) {
      m_cx.transform(span->r, span->g, span->b, span->a);
      ++span;
    }
  }
  
private:

  
  // Color transform
  gnash::cxform m_cx;
  
  // Span allocator
  span_allocator_type m_sa;
  
  // Transformer
  agg::trans_affine m_tr;
  
  // Span interpolator
  interpolator_type m_span_interpolator;
  
  gradient_func_type m_gradient_func;
  
  // Gradient adaptor
  gradient_adaptor_type m_gradient_adaptor;  
  
  // Gradient LUT
  color_func_type m_gradient_lut;
  
  // Span generator
  sg_type m_sg;  

}; // agg_style_gradient



// --- AGG HELPER CLASSES ------------------------------------------------------

/// Style handler for AGG's compound rasterizer. This is the class which is
/// called by AGG itself. It provides an interface to the various fill style
/// classes defined above.
class agg_style_handler
{
public:

    agg_style_handler() : 
        m_transparent(0, 0, 0, 0)        
    {}
    
    ~agg_style_handler() {
      int styles_size = m_styles.size(); 
      for (int i=0; i<styles_size; i++)
        delete m_styles[i]; 
    }

    /// Called by AGG to ask if a certain style is a solid color
    bool is_solid(unsigned style) const {
     
      return m_styles[style]->m_is_solid; 
    }
    
    /// Adds a new solid fill color style
    void add_color(const agg::rgba8 color) {
      agg_style_solid *st = new agg_style_solid(color);
      m_styles.push_back(st);
    }
    
    /// Adds a new bitmap fill style
    void add_bitmap(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx, 
      bool repeat, bool smooth) {
      
      if (bi==NULL) {
        // NOTE: Apparently "bi" can be NULL in some cases and this should not
        // be treated as an error.
        log_msg("WARNING: add_bitmap called with bi=NULL");
        add_color(agg::rgba8(0,0,0,0));
        return;
      }

      // Whew! There are 8 bitmap combinations (bpp, smooth, repeat) possible
      // and AGG uses templates, so... 
      // I'd need to pass "span_image_filter_rgba_nn" (because span_image_xxx
      // dependends on the pixel format) without passing the template 
      // parameters, but AFAIK this can't be done. But hey, this is my first 
      // C++ code (the whole AGG backend) and I immediately had to start with 
      // templates. I'm giving up and write eight versions of add_bitmap_xxx. 
      // So, if anyone has a better solution, for heaven's sake, implement it!! 
        
      if (repeat) {
        if (smooth) {
        
          if (bi->get_bpp()==24)
            add_bitmap_repeat_aa_rgb24 (bi, mat, cx);      
          else
          if (bi->get_bpp()==32)
            add_bitmap_repeat_aa_rgba32 (bi, mat, cx);      
          else
            assert(0);
            
        } else {
        
          if (bi->get_bpp()==24)
            add_bitmap_repeat_nn_rgb24 (bi, mat, cx);      
          else
          if (bi->get_bpp()==32)
            add_bitmap_repeat_nn_rgba32 (bi, mat, cx);      
          else
            assert(0);
            
        } // if smooth
      } else {
        if (smooth) {
        
          if (bi->get_bpp()==24)
            add_bitmap_clip_aa_rgb24 (bi, mat, cx);      
          else
          if (bi->get_bpp()==32)
            add_bitmap_clip_aa_rgba32 (bi, mat, cx);      
          else
            assert(0);
            
        } else {
        
          if (bi->get_bpp()==24)
            add_bitmap_clip_nn_rgb24 (bi, mat, cx);      
          else
          if (bi->get_bpp()==32)
            add_bitmap_clip_nn_rgba32 (bi, mat, cx);      
          else
            assert(0);
            
        } // if smooth
      } // if repeat
      
    } // add_bitmap 


    // === RGB24 ===
    

    void add_bitmap_repeat_nn_rgb24(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {
    
      // tiled, nearest neighbor method (faster)   

      typedef agg::pixfmt_rgb24 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef agg::wrap_mode_repeat wrap_type;
      typedef agg::image_accessor_wrap<PixelFormat, wrap_type, wrap_type> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgb_nn<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
        
        
    
    
    void add_bitmap_clip_nn_rgb24(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {

      // clipped, nearest neighbor method (faster)   

      typedef agg::pixfmt_rgb24 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef image_accessor_clip_transp<PixelFormat> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgb_nn<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
    
    
    
    void add_bitmap_repeat_aa_rgb24(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {  

      // tiled, bilinear method (better quality)   

      typedef agg::pixfmt_rgb24 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef agg::wrap_mode_repeat wrap_type;
      typedef agg::image_accessor_wrap<PixelFormat, wrap_type, wrap_type> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgb_bilinear<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
        
    
    void add_bitmap_clip_aa_rgb24(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {

      // clipped, bilinear method (better quality)   

      typedef agg::pixfmt_rgb24 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef image_accessor_clip_transp<PixelFormat> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgb_bilinear<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
    
       
    
    // === RGBA32 ===    

    void add_bitmap_repeat_nn_rgba32(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {
    
      // tiled, nearest neighbor method (faster)   

      typedef agg::pixfmt_rgba32 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef agg::wrap_mode_repeat wrap_type;
      typedef agg::image_accessor_wrap<PixelFormat, wrap_type, wrap_type> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgba_nn<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
        
        
    
    
    void add_bitmap_clip_nn_rgba32(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {

      // clipped, nearest neighbor method (faster)   

      typedef agg::pixfmt_rgba32 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef image_accessor_clip_transp<PixelFormat> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgba_nn<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
    
    
    
    void add_bitmap_repeat_aa_rgba32(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {  

      // tiled, bilinear method (better quality)   

      typedef agg::pixfmt_rgba32 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef agg::wrap_mode_repeat wrap_type;
      typedef agg::image_accessor_wrap<PixelFormat, wrap_type, wrap_type> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgba_bilinear<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
        
    
    void add_bitmap_clip_aa_rgba32(agg_bitmap_info_base* bi, gnash::matrix mat, gnash::cxform cx) {

      // clipped, bilinear method (better quality)   

      typedef agg::pixfmt_rgba32 PixelFormat;
      typedef agg::span_allocator<PixelFormat> span_allocator_type;
      typedef image_accessor_clip_transp<PixelFormat> img_source_type; 
      typedef agg::span_interpolator_linear_subdiv<agg::trans_affine> interpolator_type;
      typedef agg::span_image_filter_rgba_bilinear<img_source_type, interpolator_type> sg_type;
       
      typedef agg_style_bitmap<PixelFormat, span_allocator_type, img_source_type, 
        interpolator_type, sg_type> st_type;
      
      st_type* st = new st_type(bi->get_width(), bi->get_height(),
          bi->get_rowlen(), bi->get_data(), mat, cx);       
        
      m_styles.push_back(st);
    }
    
    
    // === GRADIENT ===

    void add_gradient_linear(const gnash::fill_style& fs, gnash::matrix mat, gnash::cxform cx) {
    
      typedef agg::rgba8 color_type;            
      typedef agg::span_allocator<color_type> span_allocator_type;
      typedef agg::span_interpolator_linear<agg::trans_affine> interpolator_type;
      typedef agg::gradient_x gradient_func_type;
      //typedef agg::gradient_repeat_adaptor<gradient_func_type> gradient_adaptor_type;
      typedef gradient_func_type gradient_adaptor_type;
      typedef agg::gradient_lut<agg::color_interpolator<agg::rgba8>, 256> color_func_type;
      typedef agg::span_gradient<color_type,
                                 interpolator_type,
                                 gradient_adaptor_type,
                                 color_func_type> sg_type;
       
      typedef agg_style_gradient<color_type, span_allocator_type, 
        interpolator_type, gradient_func_type, gradient_adaptor_type, 
        color_func_type, sg_type> st_type;
      
      st_type* st = new st_type(fs, mat, cx, 256);
      
      // NOTE: The value 256 is based on the bitmap texture used by other
      // Gnash renderers which is normally 256x1 pixels for linear gradients.       
        
      m_styles.push_back(st);
    }
    

    void add_gradient_radial(const gnash::fill_style& fs, gnash::matrix mat, gnash::cxform cx) {
    
      typedef agg::rgba8 color_type;            
      typedef agg::span_allocator<color_type> span_allocator_type;
      typedef agg::span_interpolator_linear<agg::trans_affine> interpolator_type;
      typedef agg::gradient_radial gradient_func_type;
      typedef gradient_func_type gradient_adaptor_type;
      typedef agg::gradient_lut<agg::color_interpolator<agg::rgba8>, 256> color_func_type;
      typedef agg::span_gradient<color_type,
                                 interpolator_type,
                                 gradient_adaptor_type,
                                 color_func_type> sg_type;
       
      typedef agg_style_gradient<color_type, span_allocator_type, 
        interpolator_type, gradient_func_type, gradient_adaptor_type, 
        color_func_type, sg_type> st_type;
      
      // move the center of the radial fill to where it should be
      gnash::matrix transl;
      transl.concatenate_translation(-32.0f, -32.0f);
      transl.concatenate(mat);    
      
      st_type* st = new st_type(fs, transl, cx, 64/2);  // div 2 because we need radius, not diameter     
        
      // NOTE: The value 64 is based on the bitmap texture used by other
      // Gnash renderers which is normally 64x64 pixels for linear gradients.       
        
      m_styles.push_back(st);
    }
    

    /// Returns the color of a certain fill style (solid)
    const agg::rgba8& color(unsigned style) const 
    {
        if (style < m_styles.size())
            return m_styles[style]->m_color;

        return m_transparent;
    }

    /// Called by AGG to generate a scanline span for non-solid fills 
    void generate_span(agg::rgba8* span, int x, int y, unsigned len, unsigned style)
    {
      m_styles[style]->generate_span(span,x,y,len);
    }


private:
    std::vector<agg_style_base*> m_styles;
    agg::rgba8          m_transparent;
};  // class agg_style_handler



class agg_mask_style_handler 
{
public:

  agg_mask_style_handler() :
    m_color(255,255)
  {
  }

  bool is_solid(unsigned style) const
  {
    return true;
  }
  
  const agg::gray8& color(unsigned style) const 
  {
    return m_color;
  }
  
  void generate_span(agg::gray8* /*span*/, int /*x*/, int /*y*/, int /*len*/, unsigned /*style*/)
  {
    assert(0); // never call generate_span for solid fill styles
  }

private:
  agg::gray8 m_color;
  
};  // class agg_mask_style_handler


} // namespace gnash

#endif // BACKEND_RENDER_HANDLER_AGG_STYLE_H
