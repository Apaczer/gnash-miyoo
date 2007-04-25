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

/* $Id: gtk_glue_gtkglext.h,v 1.8 2007/04/25 12:36:44 martinwguy Exp $ */

#include "gtk_glue.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gtk/gtkgl.h>

#include "tu_opengl_includes.h"




using namespace std;


namespace gnash
{

class GtkGlExtGlue : public GtkGlue
{
  public:
    GtkGlExtGlue();
    ~GtkGlExtGlue();

    bool init(int argc, char **argv[]);
    void prepDrawingArea(GtkWidget *drawing_area);
    render_handler* createRenderHandler();
    void render();
    void render(int /*minx*/, int /*miny*/, int /*maxx*/, int /*maxy*/) { render(); };
    void configure(GtkWidget *const widget, GdkEventConfigure *const event);
  private:
    GdkGLConfig *_glconfig;
#ifdef FIX_I810_LOD_BIAS
    float _tex_lod_bias;
#endif
};

}
