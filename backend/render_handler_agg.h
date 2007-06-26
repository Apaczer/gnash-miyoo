// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

// 
//

/* $Id: render_handler_agg.h,v 1.7 2006/10/15 10:10:23 nihilus Exp $ */

#ifndef BACKEND_RENDER_HANDLER_AGG_H
#define BACKEND_RENDER_HANDLER_AGG_H

namespace gnash {

// Base class to shield GUIs from AGG's pixelformat classes 
class render_handler_agg_base : public render_handler
{
private:

  unsigned char *_testBuffer; // buffer used by initTestBuffer() only
  
public:
  
  render_handler_agg_base() : _testBuffer(0) { }  

  // virtual classes should have virtual destructors
  virtual ~render_handler_agg_base() {}
  
  // these methods need to be accessed from outside:
  virtual void init_buffer(unsigned char *mem, int size, int x, int y)=0;

  virtual unsigned int getBytesPerPixel() const=0;

  unsigned int getBitsPerPixel() const { return getBytesPerPixel()*8; }
  
  virtual bool initTestBuffer(unsigned width, unsigned height) {
    int size = width * height * getBytesPerPixel();
    
    _testBuffer	= static_cast<unsigned char *>( realloc(_testBuffer, size) );
    
    init_buffer(_testBuffer, size, width, height);
    
    return true;
  }
  
  
};


/// Create a render handler 
//
/// If the given pixelformat is unsupported, or any other error
/// occurs, NULL is returned.
///
DSOEXPORT render_handler_agg_base*
  create_render_handler_agg(const char *pixelformat);
  
/// Detect pixel format based on bit mask. If the pixel format is unknown,
/// NULL is returned. Note that a successfully detected pixel format does
/// not necessarily mean that the pixel format is available (compiled in).
DSOEXPORT char* agg_detect_pixel_format(unsigned int rofs, unsigned int rsize,
  unsigned int gofs, unsigned int gsize,
  unsigned int bofs, unsigned int bsize,
  unsigned int bpp);
  

} // namespace gnash


#endif // BACKEND_RENDER_HANDLER_CAIRO_H
