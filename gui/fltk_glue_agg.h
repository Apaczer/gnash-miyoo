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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef FLTK_GLUE_AGG_H
#define FLTK_GLUE_AGG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gnash.h"
#include "log.h"
#include "gui.h"

#include "render_handler.h"
#include "render_handler_agg.h"

using namespace std;
using namespace fltk;

namespace gnash {

class FltkAggGlue
{
    public:
      FltkAggGlue();
      ~FltkAggGlue();
     // resize(int width, int height);
      void draw();
      render_handler* createRenderHandler();
      void initBuffer(int width, int height);
      void resize(int width, int height);
      void invalidateRegion(const rect& bounds);

    private:
      int _width;
      int _height;
      int _stride;
      unsigned char* _offscreenbuf;
      render_handler* _renderer;
      //Rectangle _bounds;
      geometry::Range2d<int> _drawbounds;
      geometry::Range2d<int> _validbounds;
};

} // namespace gnash

#endif //FLTK_GLUE_AGG_H
