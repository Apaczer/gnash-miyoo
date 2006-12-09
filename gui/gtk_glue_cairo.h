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

#include "gtk_glue.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

# include <cairo.h>

namespace gnash
{

class GtkCairoGlue : public GtkGlue
{
  public:
    GtkCairoGlue();
    ~GtkCairoGlue();

    bool init(int argc, char ***argv);
    void prepDrawingArea(GtkWidget *drawing_area);
    render_handler* createRenderHandler();
    void render();
    void render(int minx, int miny, int maxx, int maxy) { render(); };
    void configure(GtkWidget *const widget, GdkEventConfigure *const event);
  private:
    cairo_t     *_cairo_handle;
    cairo_t     *_cairo_offscreen;
};

} // namespace gnash
