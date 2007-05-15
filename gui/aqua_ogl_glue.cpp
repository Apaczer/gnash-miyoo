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

// 
//

/* $Id: aqua_ogl_glue.cpp,v 1.6 2007/05/15 22:04:18 nihilus Exp $ */


#include "aqua_ogl_glue.h"
#include <AGL/agl.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include "log.h"

#define OVERSIZE 1.0f

using namespace std;

namespace gnash
{

AquaOglGlue::AquaOglGlue()
{
//    GNASH_REPORT_FUNCTION;
}

AquaOglGlue::~AquaOglGlue()
{
//    GNASH_REPORT_FUNCTION;

}

bool AquaOglGlue::init(int, char***)
{
//    GNASH_REPORT_FUNCTION;

    return true;
}


render_handler* AquaOglGlue::createRenderHandler(int depth)
{
//    GNASH_REPORT_FUNCTION;

    _bpp = depth;
    render_handler* renderer = create_render_handler_ogl();

    return renderer;
}

bool AquaOglGlue::prepDrawingArea(int width, int height)
{
    if (_bpp == 16) {
      // 16-bit color, surface creation is likely to succeed.
      /*SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 15);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);*/
    } else {
      assert(_bpp == 32);

      // 32-bit color etc, for getting dest alpha,
      // for MULTIPASS_ANTIALIASING (see
      // render_handler_ogl.cpp).
      /*SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);*/
    }

    //SDL_SetVideoMode(width, height, _bpp, sdl_flags | SDL_OPENGL);

     // Turn on alpha blending.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                     
    // Turn on line smoothing.  Antialiased lines can be used to
    // smooth the outsides of shapes.
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); // GL_NICEST, GL_FASTEST, GL_DONT_CARE
    glMatrixMode(GL_PROJECTION);


    glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
 
    // We don't need lighting effects
    glDisable(GL_LIGHTING);
    glPushAttrib (GL_ALL_ATTRIB_BITS);         

    return true;
}

void
AquaOglGlue::render()
{
//    GNASH_REPORT_FUNCTION;
    //SDL_GL_SwapBuffers();
}

} // namespace gnash