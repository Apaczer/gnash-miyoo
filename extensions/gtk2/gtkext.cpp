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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <map>
#include <iostream>
#include <string>
#include <cstdio>
#include <boost/algorithm/string/case_conv.hpp>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include "VM.h"
#include "fn_call.h"
#include "log.h"
#include "fn_call.h"
#include "as_object.h"
#include "as_function.h"
#include "builtin_function.h" // need builtin_function
#include "debugger.h"
#include "gtkext.h"

using namespace std;

namespace gnash
{

static LogFile& dbglogfile = LogFile::getDefaultInstance();
#ifdef USE_DEBUGGER
static Debugger& debugger = Debugger::getDefaultInstance();
#endif

// prototypes for the callbacks required by Gnash
void gtkext_window_new(const fn_call& fn);
void gtkext_signal_connect(const fn_call& fn);
void gtkext_container_set_border_width(const fn_call& fn);
void gtkext_button_new_with_label(const fn_call& fn);
void gtkext_signal_connect_swapped(const fn_call& fn);
void gtkext_container_add(const fn_call& fn);
void gtkext_widget_show(const fn_call& fn);
void gtkext_main(const fn_call& fn);

// Sigh... We can't store the callbacks for the events in the GtkExt
// class object because that data is inaccessible to a C symbol based
// callback. 
static map<string, as_value> callbacks;

void dump_callbacks(map<string, as_value> &calls)
{
//    GNASH_REPORT_FUNCTION;
    map<string, as_value>::const_iterator it;
    dbglogfile << "# of callbacks is: " << calls.size() << endl;
    for (it=calls.begin(); it != calls.end(); it++) {
	string name = (*it).first;
	as_value as = (*it).second;
	dbglogfile << "Event \"" << name.c_str() << "\" has AS function" << as.to_string() << endl;
    }
	
}

static void
generic_callback(GtkWidget *widget, gpointer data)
{
//    GNASH_REPORT_FUNCTION;
//    g_print ("Hello World - %d\n", *(int *)data;
    const char *event = (const char *)data;

    as_value handler = callbacks[event];
    as_function *as_func = handler.to_as_function();

//	start the debugger when this callback is activated.
// 	debugger.enabled(true);
// 	debugger.console();

    // FIXME: Delete events don't seem to pass in data in a form we
    // can access it. So for now we just hack in a quit, since we know
    // we're done, we hope...
    if (*event == NULL) {
	gtk_main_quit();
	return;
    } else {
	cerr << "event is: \"" << event << "\"" << endl;
    }
    
    as_value	val;
    as_environment env;
    env.push_val(handler.to_object());
    env.push(event);
    env.push(handler.to_string());

    // Call the AS function defined in the source file using this extension
    (*as_func)(fn_call(&val, handler.to_object(), &env, 2, 2));
}

static void
attachInterface(as_object *obj)
{
//    GNASH_REPORT_FUNCTION;

    obj->set_member("window_new", &gtkext_window_new);
    obj->set_member("signal_connect", &gtkext_signal_connect);
    obj->set_member("container_set_border_width", &gtkext_container_set_border_width);
    obj->set_member("button_new_with_label", &gtkext_button_new_with_label);
    obj->set_member("signal_connect_swapped", &gtkext_signal_connect_swapped);
    obj->set_member("container_add", &gtkext_container_add);
    obj->set_member("widget_show", &gtkext_widget_show);
    obj->set_member("main", &gtkext_main);
}

static as_object*
getInterface()
{
//    GNASH_REPORT_FUNCTION;
    static boost::intrusive_ptr<as_object> o;
    if (o == NULL) {
	o = new as_object();
    }
    return o.get();
}

static void
gtkext_ctor(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *obj = new GtkExt();

    attachInterface(obj);
    fn.result->set_as_object(obj); // will keep alive
}


GtkExt::GtkExt()
{
//    GNASH_REPORT_FUNCTION;
    int argc = 0;
    char **argv;
    gtk_init (&argc, &argv);
}

GtkExt::~GtkExt()
{
//    GNASH_REPORT_FUNCTION;
}

void
GtkExt::window_new()
{
    GNASH_REPORT_FUNCTION;
    _window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
}

// void
// GtkExt::signal_connect()
// {
//     GNASH_REPORT_FUNCTION;
// }

// void gtk_container_set_border_width (GtkContainer *container, guint border_width);
void
GtkExt::container_set_border_width(int width)
{
//    GNASH_REPORT_FUNCTION;
    if (_window) {
	gtk_container_set_border_width (GTK_CONTAINER (_window), width);
    }
}

// GtkWidget *gtk_button_new_with_label (const gchar *label);
GtkWidget *
GtkExt::button_new_with_label(const char *label)
{
//    GNASH_REPORT_FUNCTION;
    _window = gtk_button_new_with_label (label);
    return _window;
}

void
GtkExt::main()
{
//    GNASH_REPORT_FUNCTION;
}

// this callback takes no arguments
void gtkext_window_new(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);

    GtkExt *obj = new GtkExt;
    obj->window_new();
    fn.result->set_as_object(obj);
}

// this callback takes 4 arguments, we only need two of them
// g_signal_connect (instance, detailed_signal, c_handler, data)
void gtkext_signal_connect(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);

    if (fn.nargs > 0) {
	GtkExt *window = dynamic_cast<GtkExt *>(fn.arg(0).to_object());
	string name = fn.arg(1).to_std_string();
	as_value func = fn.arg(2).to_as_function();
	int data = fn.arg(3).to_number();

	dbglogfile << "Adding callback " << func.to_string()
		   << " for event \"" << name << "\"" << endl;
 	callbacks[name] = func;
 	g_signal_connect (G_OBJECT (window->getWindow()), name.c_str(),
			  G_CALLBACK (generic_callback), (void *)name.c_str());
    }
}

// this callback takes 2 arguments
// void gtk_container_set_border_width (GtkContainer *container, guint border_width);
void gtkext_container_set_border_width(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);
    
    if (fn.nargs > 0) {
	GtkExt *window = dynamic_cast<GtkExt *>(fn.arg(0).to_object());
	int width = fn.arg(1).to_number();
	window->container_set_border_width(width);
	dbglogfile << "set container border width to " << width << " !" << endl;
    }
}

// Creates a new button with the label "Hello World".
// GtkWidget *gtk_button_new_with_label (const gchar *label);
void gtkext_button_new_with_label(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);
    
    if (fn.nargs > 0) {
	const char *label = fn.arg(0).to_string();
	GtkExt *obj = new GtkExt;
	obj->button_new_with_label(label);
	fn.result->set_as_object(obj);
    }
}

// g_signal_connect_swapped(instance, detailed_signal, c_handler, data)
//
// Connects a GCallback function to a signal for a particular object.
//
// The instance on which the signal is emitted and data will be swapped when calling the handler.
// instance : 	the instance to connect to.
// detailed_signal : 	a string of the form "signal-name::detail".
// c_handler : 	the GCallback to connect.
// data : 	data to pass to c_handler calls.
// Returns : 	the handler id
void gtkext_signal_connect_swapped(const fn_call& fn)
{
    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);
    

    if (fn.nargs > 0) {
	GtkExt *parent = dynamic_cast<GtkExt *>(fn.arg(0).to_object());
	string name = (fn.arg(1).to_string());
	GtkExt *child = dynamic_cast<GtkExt *>(fn.arg(3).to_object());
	as_value *callback = dynamic_cast<as_value *>(fn.arg(2).to_object());

	// FIXME: This seems to cause an Gobject warning
	g_signal_connect_swapped (G_OBJECT (child->getWindow()), name.c_str(),
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (parent->getWindow()));
    }
}

// this takes two arguments
void gtkext_container_add(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);
    
    if (fn.nargs > 0) {
	GtkExt *parent = dynamic_cast<GtkExt *>(fn.arg(0).to_object());
	GtkExt *child = dynamic_cast<GtkExt *>(fn.arg(1).to_object());
	gtk_container_add (GTK_CONTAINER (parent->getWindow()), child->getWindow());
	fn.result->set_bool(true);
    }
}

void gtkext_widget_show(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);
     
    if (fn.nargs > 0) {
	GtkExt *window = dynamic_cast<GtkExt *>(fn.arg(0).to_object());
	gtk_widget_show(window->getWindow());
    }
}

// gtk_main taks no arguments.
void gtkext_main(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    GtkExt *ptr = dynamic_cast<GtkExt *>(fn.this_ptr);
    assert(ptr);

    gtk_main();
}

std::auto_ptr<as_object>
init_gtkext_instance()
{
    return std::auto_ptr<as_object>(new GtkExt());
}

extern "C" {
    void
    gtkext_class_init(as_object &obj)
    {
//	GNASH_REPORT_FUNCTION;
	// This is going to be the global "class"/"function"
	static boost::intrusive_ptr<builtin_function> cl;
	if (cl == NULL) {
	    cl = new builtin_function(&gtkext_ctor, getInterface());
// 	    // replicate all interface to class, to be able to access
// 	    // all methods as static functions
 	    attachInterface(cl.get());
	}
	
	VM& vm = VM::get(); // cache this ?
	std::string name = "GtkExt";
	if ( vm.getSWFVersion() < 7 ) {
	    boost::to_lower(name, vm.getLocale());
	}
	obj.init_member(name, cl.get());
    }
} // end of extern C


} // end of gnash namespace

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
