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

/* $Id: aqua_glue.h,v 1.4 2007/05/28 15:40:59 ann Exp $ */

#ifndef AQUA_GLUE_H
#define AQUA_GLUE_H

#include "gnash.h"

namespace gnash
{

class AquaGlue
{
  public:
    virtual ~AquaGlue() { }
    virtual bool init(int argc, char **argv[]) = 0;

    virtual bool prepDrawingArea(int width, int height) = 0;
    virtual render_handler* createRenderHandler(int depth) = 0;
    virtual void render() = 0;
  protected:
    int _bpp;
};

}

#endif /* AQUA_GLUE_H */