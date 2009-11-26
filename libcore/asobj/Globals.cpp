// Global.cpp:  Global ActionScript class setup, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "as_object.h"
#include "movie_root.h"
#include "PropFlags.h"
#include "as_value.h"
#include "as_function.h" // for function_class_init
#include "NativeFunction.h" 
#include "AsBroadcaster.h"
#include "Boolean_as.h"
#include "Color_as.h"
#include "CustomActions.h"
#include "Date_as.h" 
#include "Array_as.h" 
#include "Error_as.h"
#include "String_as.h"
#include "Selection_as.h"
#include "Number_as.h"
#include "Math_as.h"
#include "flash/accessibility/Accessibility_as.h"
#include "flash/ui/ContextMenu_as.h"
#include "flash/ui/ContextMenuItem_as.h"
#include "flash/ui/Keyboard_as.h"
#include "flash/ui/Mouse_as.h"
#include "flash/media/Microphone_as.h"
#include "flash/media/Sound_as.h"
#include "flash/media/Camera_as.h"
#include "flash/display/InteractiveObject_as.h"
#include "flash/display/DisplayObjectContainer_as.h"
#include "flash/display/DisplayObject_as.h"
#include "flash/display/Stage_as.h"
#include "flash/display/MovieClip_as.h"
#include "flash/display/Shape_as.h"
#include "flash/display/Sprite_as.h"
#include "flash/display/Bitmap_as.h"
#include "flash/events/Event_as.h"
#include "flash/events/EventDispatcher_as.h"
#include "flash/filters/BitmapFilter_as.h"
#include "flash/geom/ColorTransform_as.h"
#include "flash/net/LocalConnection_as.h"
#include "flash/net/XMLSocket_as.h"
#include "flash/net/SharedObject_as.h"
#include "flash/net/NetConnection_as.h"
#include "flash/net/NetStream_as.h"
#include "flash/system/System_as.h"
#include "flash/text/TextSnapshot_as.h"
#include "flash/text/TextFieldAutoSize_as.h"
#include "flash/text/Font_as.h"
#include "flash/text/FontStyle_as.h"
#include "flash/text/AntiAliasType_as.h"
#include "flash/text/CSMSettings_as.h"
#include "flash/text/GridFitType_as.h"
#include "flash/text/StaticText_as.h"
#include "flash/text/StyleSheet_as.h"
#include "flash/text/TextColorType_as.h"
#include "flash/text/TextDisplayMode_as.h"
#include "flash/text/TextFieldType_as.h"
#include "flash/text/TextFormatAlign_as.h"
#include "flash/text/TextFormat_as.h"
#include "flash/text/TextLineMetrics_as.h"
#include "flash/text/TextRenderer_as.h"
#include "flash/xml/XMLDocument_as.h"
#include "flash/xml/XMLNode_as.h"
#include "MovieClipLoader.h"
#include "movie_definition.h"
#include "Video.h"
#include "extension.h"
#include "VM.h"
#include "Timers.h"
#include "URL.h" 
#include "builtin_function.h"
#include "TextField.h"
#include "rc.h"
#include "ClassHierarchy.h"
#include "namedStrings.h"
#include "GnashNumeric.h" // for isfinite replacement
#include "flash_pkg.h"
#include "fn_call.h"
#include "Button.h"
#include "Globals.h"
#include "int_as.h"
#include "LoadVars_as.h"
#include "Namespace_as.h"
#include "QName_as.h"
#include "Object.h"

#include <limits> 
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>

// Common code to warn and return if a required single arg is not present
// and to warn if there are extra args.
#define ASSERT_FN_ARGS_IS_1                        \
    if (fn.nargs < 1) {                            \
    IF_VERBOSE_ASCODING_ERRORS(                    \
            log_aserror(_("%s needs one argument"), __FUNCTION__);        \
            )                                \
         return as_value();                            \
    }                                    \
    IF_VERBOSE_ASCODING_ERRORS(                        \
    if (fn.nargs > 1)                        \
            log_aserror(_("%s has more than one argument"), __FUNCTION__);    \
    )

namespace gnash {

namespace {

    const ClassHierarchy::NativeClasses& avm1Classes();
    const ClassHierarchy::NativeClasses& avm2Classes(string_table& st);

    as_value global_trace(const fn_call& fn);
    as_value global_isNaN(const fn_call& fn);
    as_value global_isfinite(const fn_call& fn);
    as_value global_unescape(const fn_call& fn);
    as_value global_escape(const fn_call& fn);
    as_value global_parsefloat(const fn_call& fn);
    as_value global_parseint(const fn_call& fn);
    as_value global_assetpropflags(const fn_call& fn);
    as_value global_assetuperror(const fn_call& fn);
    as_value global_asnative(const fn_call& fn);
    as_value global_asnew(const fn_call& fn);
    as_value global_assetnative(const fn_call& fn);
    as_value global_assetnativeaccessor(const fn_call& fn);
    as_value global_asconstructor(const fn_call& fn);
    as_value global_updateAfterEvent(const fn_call& fn);
    as_value global_setTimeout(const fn_call& fn);
    as_value global_clearTimeout(const fn_call& fn);
    as_value global_clearInterval(const fn_call& fn);
    as_value global_setInterval(const fn_call& fn);
    
    // These are present in the standalone, not sure about the plugin.
    as_value global_enableDebugConsole(const fn_call& fn);
    as_value global_showRedrawRegions(const fn_call& fn);

    // This is a help function for the silly AsSetupError function.
    as_value local_errorConstructor(const fn_call& fn);
    
    void registerNatives(as_object& global);
    template<typename T> as_object* constructObject(Global_as& gl, const T& arg,
            string_table::key className);
}

AVM2Global::AVM2Global(abc::Machine& /*machine*/, VM& vm)
    :
    _classes(this, 0),
    _vm(vm)
{
}

void
AVM2Global::registerClasses()
{
   
    const string_table::key NS_GLOBAL(0);
    
    object_class_init(*this, ObjectURI(NSV::CLASS_OBJECT, NS_GLOBAL)); 
    function_class_init(*this, ObjectURI(NSV::CLASS_FUNCTION, NS_GLOBAL));
    string_class_init(*this, ObjectURI(NSV::CLASS_STRING, NS_GLOBAL)); 
    array_class_init(*this, ObjectURI(NSV::CLASS_ARRAY, NS_GLOBAL)); 
    
    init_member("trace", createFunction(global_trace));
    init_member("escape", createFunction(global_escape));

    _classes.declareAll(avm2Classes(_vm.getStringTable()));
    _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_FUNCTION);
    _classes.getGlobalNs()->getClass(NSV::CLASS_FUNCTION)->setDeclared();
    _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_OBJECT);
    _classes.getGlobalNs()->getClass(NSV::CLASS_OBJECT)->setDeclared();
    _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_ARRAY);
    _classes.getGlobalNs()->getClass(NSV::CLASS_ARRAY)->setDeclared();
    _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_STRING);
    _classes.getGlobalNs()->getClass(NSV::CLASS_STRING)->setDeclared();        

}
    
as_object*
AVM1Global::createObject()
{
    as_object* obj = new as_object(*this);
    obj->set_prototype(getObjectInterface());
    return obj;
}

builtin_function*
AVM1Global::createFunction(Global_as::ASFunction function)
{
    as_object* proto = createObject();
    builtin_function* f = new builtin_function(*this, function);
    
    proto->init_member(NSV::PROP_CONSTRUCTOR, f); 
    
    f->init_member(NSV::PROP_PROTOTYPE, proto);
    f->init_member(NSV::PROP_CONSTRUCTOR,
            as_function::getFunctionConstructor());
    return f;
}

as_object*
AVM1Global::createClass(Global_as::ASFunction ctor, as_object* prototype)
{
    as_object* cl = new builtin_function(*this, ctor);
    
    if (prototype) {
        prototype->init_member(NSV::PROP_CONSTRUCTOR, cl); 
        cl->init_member(NSV::PROP_PROTOTYPE, prototype);
    }
    cl->init_member(NSV::PROP_CONSTRUCTOR,
            as_function::getFunctionConstructor());
    return cl;
}

as_object*
AVM1Global::createString(const std::string& s)
{
    // This is not really correct. If there is no String class, to_object()
    // returns an undefined value, not a null object. The behaviour is the
    // same for versions 5 to 8.
    return constructObject(*this, s, NSV::CLASS_STRING);
}

as_object*
AVM1Global::createNumber(double d)
{
    // This is not really correct. If there is no String class, to_object()
    // returns an undefined value, not a null object. The behaviour is the
    // same for versions 5 to 8.
    return constructObject(*this, d, NSV::CLASS_NUMBER);

}

/// Construct an Array.
//
/// This uses the _global Array class to initialize the "constructor" and
/// "__proto__" properties. If Array.prototype is undefined, those properties
/// are not added.
as_object*
AVM1Global::createArray()
{
    as_object* array = new as_object(*this);

    as_value ctor = getMember(NSV::CLASS_ARRAY);
    as_object* obj = ctor.to_object(*this);
    if (obj) {
        as_value proto;
        if (obj->get_member(NSV::PROP_PROTOTYPE, &proto)) {
            array->init_member(NSV::PROP_CONSTRUCTOR, ctor);
            array->set_prototype(obj->getMember(NSV::PROP_PROTOTYPE));
        }
    }

    array->init_member(NSV::PROP_LENGTH, 0.0);
    array->setArray();
    return array;
}

as_object*
AVM1Global::createBoolean(bool b)
{
    return constructObject(*this, b, NSV::CLASS_BOOLEAN);
}
    
as_object*
AVM2Global::createObject()
{
    as_object* obj = new as_object(*this);
    obj->set_prototype(getObjectInterface());
    return obj;
}

builtin_function*
AVM2Global::createFunction(Global_as::ASFunction function)
{
    builtin_function* f = new builtin_function(*this, function);
    f->init_member(NSV::PROP_CONSTRUCTOR,
            as_function::getFunctionConstructor());
    return f;
}

as_object*
AVM2Global::createClass(Global_as::ASFunction ctor, as_object* prototype)
{
    // TODO: this should attach the function to the prototype as its
    as_object* cl = new builtin_function(*this, ctor);
    
    if (prototype) {
        prototype->init_member(NSV::PROP_CONSTRUCTOR, cl); 
        cl->init_member(NSV::PROP_PROTOTYPE, prototype);
    }
    return cl;
}

as_object*
AVM2Global::createString(const std::string& s)
{
    // What AVM2 does for createString is untested, so we do the same
    // as AVM1 for now.
    return constructObject(*this, s, NSV::CLASS_STRING);
}

as_object*
AVM2Global::createNumber(double d)
{
    return constructObject(*this, d, NSV::CLASS_NUMBER);
}

as_object*
AVM2Global::createBoolean(bool b)
{
    return constructObject(*this, b, NSV::CLASS_BOOLEAN);
}

/// This serves the purpose of hiding the Array_as type from the
/// implementation, which at least enforces good behaviour from users.
as_object*
AVM2Global::createArray()
{
    as_object* array = new as_object(*this);
    array->setArray();
    array->init_member(NSV::PROP_CONSTRUCTOR, getMember(NSV::CLASS_ARRAY));
    return array;
}

void 
AVM1Global::markReachableResources() const
{
    _classes.markReachableResources();
    markAsObjectReachable();
}

AVM1Global::AVM1Global(VM& vm)
    :
    _classes(this, &_et),
    _vm(vm)
{
}

void
AVM1Global::registerClasses()
{
    registerNatives(*this);

    const string_table::key NS_GLOBAL(0);

    object_class_init(*this, ObjectURI(NSV::CLASS_OBJECT, NS_GLOBAL)); 
    function_class_init(*this, ObjectURI(NSV::CLASS_FUNCTION, NS_GLOBAL));
    string_class_init(*this, ObjectURI(NSV::CLASS_STRING, NS_GLOBAL)); 
    array_class_init(*this, ObjectURI(NSV::CLASS_ARRAY, NS_GLOBAL)); 

    // No idea why, but it seems there's a NULL _global.o 
    // defined at player startup...
    // Probably due to the AS-based initialization 
    // Not enumerable but overridable and deletable.
    //
    as_value nullVal; nullVal.set_null();
    init_member("o", nullVal, PropFlags::dontEnum);

    // _global functions.            
    // These functions are only available in SWF6+, but this is just
    // because SWF5 or lower did not have a "_global"
    // reference at all.
    init_member("ASnative", createFunction(global_asnative));
    init_member("ASconstructor", createFunction(global_asconstructor));
    init_member("ASSetPropFlags", _vm.getNative(1, 0));
    init_member("ASSetNative", _vm.getNative(4, 0));
    init_member("ASSetNativeAccessor", _vm.getNative(4, 1));
    init_member("AsSetupError", createFunction(global_assetuperror));
    init_member("updateAfterEvent", _vm.getNative(9, 0));
    init_member("trace", _vm.getNative(100, 4));

    init_member("setInterval", _vm.getNative(250, 0));
    init_member("clearInterval", _vm.getNative(250, 1));
    init_member("setTimeout", _vm.getNative(250, 2));
 
    // This is an odd function with no properties. There ought to be
    // a better way of implementing this. See also TextFormat.getTextExtent.
    as_function* edc = createFunction(global_enableDebugConsole);
    edc->clearProperties();
    init_member("enableDebugConsole", edc);
    init_member("showRedrawRegions", _vm.getNative(1021, 1));
    
    string_table& st = getStringTable(*this);
    init_member("clearTimeout", getMember(st.find("clearInterval")));

    _classes.declareAll(avm1Classes());

    // SWF8 visibility:
    const string_table::key NS_FLASH = st.find("flash");
    flash_package_init(*this, ObjectURI(NS_FLASH, NS_GLOBAL)); 

    const int version = _vm.getSWFVersion();

    switch (version)
    {
        default:
            // Version 10 or above reported
        case 9:
        case 8:

        case 7:
        case 6:

            _classes.getGlobalNs()->stubPrototype(_classes, 
                    NSV::CLASS_FUNCTION);
            
            _classes.getGlobalNs()->getClass(
                    NSV::CLASS_FUNCTION)->setDeclared();

        case 5:
        
            _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_OBJECT);
            _classes.getGlobalNs()->getClass(NSV::CLASS_OBJECT)->setDeclared();
            _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_ARRAY);
            _classes.getGlobalNs()->getClass(NSV::CLASS_ARRAY)->setDeclared();
            _classes.getGlobalNs()->stubPrototype(_classes, NSV::CLASS_STRING);
            _classes.getGlobalNs()->getClass(NSV::CLASS_STRING)->setDeclared();        
            // This is surely not correct, but they are not available
            // in SWF4
            init_member("escape", _vm.getNative(100, 0));
            init_member("unescape", _vm.getNative(100, 1));
            init_member("parseInt", _vm.getNative(100, 2));
            init_member("parseFloat", _vm.getNative(100, 3));
            init_member("isNaN", _vm.getNative(200, 18));
            init_member("isFinite", _vm.getNative(200, 19));

            init_member("NaN", as_value(NaN));
            init_member("Infinity", as_value(
                        std::numeric_limits<double>::infinity()));
        
        case 4:
        case 3:
        case 2:
        case 1:
            break;
    }

    loadExtensions();
}

//-----------------------
// Extensions
//-----------------------
// Scan the plugin directories for all plugins, and load them now.
// FIXME: this should actually be done dynamically, and only
// if a plugin defines a class that a movie actually wants to
// use.
void
AVM1Global::loadExtensions()
{

    if ( RcInitFile::getDefaultInstance().enableExtensions() )
    {
        log_security(_("Extensions enabled, scanning plugin dir for load"));
        _et.scanAndLoad(*this);
    }
    else
    {
        log_security(_("Extensions disabled"));
    }

}

namespace {

const ClassHierarchy::NativeClasses&
avm1Classes()
{

    const string_table::key NS_GLOBAL = 0;

    typedef ClassHierarchy::NativeClass N;

    // Since we maintain separate lists for AVM1 and AVM2, these are all
    // considered to be in the 'Global' namespace (AVM1 has no namespaces).
    static const ClassHierarchy::NativeClasses s = boost::assign::list_of

        (N(system_class_init, NSV::CLASS_SYSTEM, 0, NS_GLOBAL, 1))
        (N(stage_class_init, NSV::CLASS_STAGE, 0, NS_GLOBAL, 1))
        (N(movieclip_class_init, NSV::CLASS_MOVIE_CLIP, 0, NS_GLOBAL, 3))
        (N(textfield_class_init, NSV::CLASS_TEXT_FIELD, 0, NS_GLOBAL, 3))
        (N(math_class_init, NSV::CLASS_MATH, 0, NS_GLOBAL, 4))
        (N(boolean_class_init, NSV::CLASS_BOOLEAN, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(button_class_init, NSV::CLASS_BUTTON, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(color_class_init, NSV::CLASS_COLOR, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(selection_class_init, NSV::CLASS_SELECTION, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(sound_class_init, NSV::CLASS_SOUND, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(xmlsocket_class_init, NSV::CLASS_XMLSOCKET, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(date_class_init, NSV::CLASS_DATE, NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(XMLNode_as::init, NSV::CLASS_XMLNODE, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(XMLDocument_as::init, NSV::CLASS_XML, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(mouse_class_init, NSV::CLASS_MOUSE, NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(number_class_init, NSV::CLASS_NUMBER, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(textformat_class_init, NSV::CLASS_TEXT_FORMAT, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(keyboard_class_init, NSV::CLASS_KEY, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(AsBroadcaster::init, NSV::CLASS_AS_BROADCASTER, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(textsnapshot_class_init, NSV::CLASS_TEXT_SNAPSHOT, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(video_class_init, NSV::CLASS_VIDEO, NSV::CLASS_OBJECT, NS_GLOBAL, 6))
        (N(camera_class_init, NSV::CLASS_CAMERA, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(microphone_class_init, NSV::CLASS_MICROPHONE, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(sharedobject_class_init, NSV::CLASS_SHARED_OBJECT,
           NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(loadvars_class_init, NSV::CLASS_LOAD_VARS, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(localconnection_class_init, NSV::CLASS_LOCALCONNECTION,
           NSV::CLASS_OBJECT, NS_GLOBAL, 6))
        (N(customactions_class_init, NSV::CLASS_CUSTOM_ACTIONS,
           NSV::CLASS_OBJECT, NS_GLOBAL, 6))
        (N(netconnection_class_init, NSV::CLASS_NET_CONNECTION,
           NSV::CLASS_OBJECT, NS_GLOBAL, 6))
        (N(netstream_class_init, NSV::CLASS_NET_STREAM, NSV::CLASS_OBJECT,
           NS_GLOBAL, 6))
        (N(contextmenu_class_init, NSV::CLASS_CONTEXTMENU, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(contextmenuitem_class_init, NSV::CLASS_CONTEXTMENUITEM,
           NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(moviecliploader_class_init, NSV::CLASS_MOVIE_CLIP_LOADER,
           NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(Error_class_init, NSV::CLASS_ERROR, NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(accessibility_class_init, NSV::CLASS_ACCESSIBILITY,
           NSV::CLASS_OBJECT, NS_GLOBAL, 5));

    return s;

}

const ClassHierarchy::NativeClasses&
avm2Classes(string_table& st)
{

    const string_table::key NS_GLOBAL = 0;

    typedef ClassHierarchy::NativeClass N;

    static const ClassHierarchy::NativeClasses s = boost::assign::list_of

        // Global classes
        (N(math_class_init, NSV::CLASS_MATH, 0, NS_GLOBAL, 4))
        (N(boolean_class_init, NSV::CLASS_BOOLEAN, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(number_class_init, NSV::CLASS_NUMBER, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(int_class_init, NSV::CLASS_INT, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(namespace_class_init, NSV::CLASS_NAMESPACE, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(qname_class_init, NSV::CLASS_QNAME, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))
        (N(date_class_init, NSV::CLASS_DATE, NSV::CLASS_OBJECT, NS_GLOBAL, 5))
        (N(Error_class_init, NSV::CLASS_ERROR, NSV::CLASS_OBJECT,
           NS_GLOBAL, 5))

        // System classes
        (N(system_class_init, NSV::CLASS_SYSTEM, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_SYSTEM, 1))

        // Display classes
        (N(shape_class_init, st.find("Shape"), NSV::CLASS_DISPLAYOBJECT,
           NSV::NS_FLASH_DISPLAY, 3))
        (N(displayobject_class_init, NSV::CLASS_DISPLAYOBJECT,
           NSV::CLASS_EVENTDISPATCHER, NSV::NS_FLASH_DISPLAY, 3))
        (N(interactiveobject_class_init, NSV::CLASS_INTERACTIVEOBJECT,
           NSV::CLASS_DISPLAYOBJECT, NSV::NS_FLASH_DISPLAY, 3))
        (N(displayobjectcontainer_class_init, NSV::CLASS_DISPLAYOBJECTCONTAINER,
           NSV::CLASS_INTERACTIVEOBJECT, NSV::NS_FLASH_DISPLAY, 3))
        (N(sprite_class_init, NSV::CLASS_SPRITE,
           NSV::CLASS_DISPLAYOBJECTCONTAINER, NSV::NS_FLASH_DISPLAY, 3))
        (N(bitmap_class_init, NSV::CLASS_BITMAP, NSV::CLASS_DISPLAYOBJECT,
           NSV::NS_FLASH_DISPLAY, 3))
        (N(movieclip_class_init, NSV::CLASS_MOVIE_CLIP, NSV::CLASS_SPRITE,
           NSV::NS_FLASH_DISPLAY, 3))
        (N(stage_class_init, NSV::CLASS_STAGE, NSV::CLASS_MOVIE_CLIP,
           NSV::NS_FLASH_DISPLAY, 1))
        (N(button_class_init, st.find("SimpleButton"),
           NSV::CLASS_INTERACTIVEOBJECT, NSV::NS_FLASH_DISPLAY, 5))

        // Text classes
        (N(textfield_class_init, NSV::CLASS_TEXT_FIELD,
           NSV::CLASS_INTERACTIVEOBJECT, NSV::NS_FLASH_TEXT, 3))
        (N(textformat_class_init, NSV::CLASS_TEXT_FORMAT, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_TEXT, 5))
        (N(textsnapshot_class_init, NSV::CLASS_TEXT_SNAPSHOT, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_TEXT, 5))
        (N(textfieldautosize_class_init, NSV::CLASS_TEXTFIELDAUTOSIZE,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(font_class_init, NSV::CLASS_FONT, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_TEXT, 5))
        (N(fontstyle_class_init, NSV::CLASS_FONTSTYLE, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_TEXT, 5))
        (N(antialiastype_class_init, NSV::CLASS_ANTIALIASTYPE,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(csmsettings_class_init, NSV::CLASS_CSMTEXTSETTINGS,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(gridfittype_class_init, NSV::CLASS_GRIDFITTYPE,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(statictext_class_init, NSV::CLASS_STATICTEXT,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(stylesheet_class_init, NSV::CLASS_STYLESHEET,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
#if 0
        // This one isn't stubbed for some reason.
        (N(textcolor_class_init, NSV::CLASS_TEXTCOLOR,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
#endif
        (N(textcolortype_class_init, NSV::CLASS_TEXTCOLORTYPE,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(textdisplaymode_class_init, NSV::CLASS_TEXTDISPLAYMODE,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(textfieldtype_class_init, NSV::CLASS_TEXTFIELDTYPE,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(textformatalign_class_init, NSV::CLASS_TEXTFORMATALIGN,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(textlinemetrics_class_init, NSV::CLASS_TEXTLINEMETRICS,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))
        (N(textrenderer_class_init, NSV::CLASS_TEXTRENDERER,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_TEXT, 5))

        // Media classes
        (N(sound_class_init, NSV::CLASS_SOUND, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_MEDIA, 5))
        (N(video_class_init, NSV::CLASS_VIDEO, NSV::CLASS_DISPLAYOBJECT,
           NSV::NS_FLASH_MEDIA, 6))
        (N(camera_class_init, NSV::CLASS_CAMERA, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_MEDIA, 6))
        (N(microphone_class_init, NSV::CLASS_MICROPHONE, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_MEDIA, 6))

        // Net classes
        (N(xmlsocket_class_init, NSV::CLASS_XMLSOCKET, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_NET, 5))
        (N(sharedobject_class_init, NSV::CLASS_SHARED_OBJECT,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_NET, 5))
        (N(localconnection_class_init, NSV::CLASS_LOCALCONNECTION,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_NET, 6))
        (N(netconnection_class_init, NSV::CLASS_NET_CONNECTION,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_NET, 6))
        (N(netstream_class_init, NSV::CLASS_NET_STREAM, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_NET, 6))
        
        // Error classes
        
        // XML classes
        (N(XMLNode_as::init, NSV::CLASS_XMLNODE, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_XML, 5))
        (N(XMLDocument_as::init, st.find("XMLDocument"), NSV::CLASS_OBJECT,
           NSV::NS_FLASH_XML, 5))

        // UI classes
        (N(mouse_class_init, NSV::CLASS_MOUSE, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_UI, 5))
        (N(keyboard_class_init, st.find("Keyboard"), NSV::CLASS_OBJECT,
           NSV::NS_FLASH_UI, 5))
        (N(contextmenu_class_init, NSV::CLASS_CONTEXTMENU, NSV::CLASS_OBJECT,
           NSV::NS_FLASH_UI, 7))
        (N(contextmenuitem_class_init, NSV::CLASS_CONTEXTMENUITEM,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_UI, 5))
        
        // Accessibility classes
        (N(accessibility_class_init, NSV::CLASS_ACCESSIBILITY,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_ACCESSIBILITY, 5))
       
        // Event classes
        (N(event_class_init, NSV::CLASS_EVENT,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_EVENTS, 5))
        (N(eventdispatcher_class_init, NSV::CLASS_EVENTDISPATCHER,
           NSV::CLASS_OBJECT, NSV::NS_FLASH_EVENTS, 5));
        

    return s;
}

as_value
global_trace(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    // Log our argument.
    //
    // @@ what if we get extra args?
    //
    // @@ Nothing needs special treatment,
    //    as_value::to_string() will take care of everything
    log_trace("%s", fn.arg(0).to_string());

    return as_value();
}


as_value
global_isNaN(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    return as_value(static_cast<bool>(isNaN(fn.arg(0).to_number())));
}


as_value
global_isfinite(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    return as_value(static_cast<bool>(isFinite(fn.arg(0).to_number())));
}

/// \brief Encode a string to URL-encoded format
/// converting all dodgy DisplayObjects to %AB hex sequences
//
/// Characters that need escaping are:
/// - ASCII control DisplayObjects: 0-31 and 127
/// - Non-ASCII chars: 128-255
/// - URL syntax DisplayObjects: $ & + , / : ; = ? @
/// - Unsafe DisplayObjects: SPACE " < > # % { } | \ ^ ~ [ ] `
/// Encoding is a % followed by two hexadecimal DisplayObjects, case insensitive.
/// See RFC1738 http://www.rfc-editor.org/rfc/rfc1738.txt,
/// Section 2.2 "URL Character Encoding Issues"
as_value
global_escape(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    std::string input = fn.arg(0).to_string();
    URL::encode(input);
    return as_value(input);
}

/// \brief Decode a string from URL-encoded format
/// converting all hexadecimal sequences to ASCII DisplayObjects.
//
/// A sequence to convert is % followed by two case-independent hexadecimal
/// digits, which is replaced by the equivalent ASCII DisplayObject.
/// See RFC1738 http://www.rfc-editor.org/rfc/rfc1738.txt,
/// Section 2.2 "URL Character Encoding Issues"
as_value
global_unescape(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1

    std::string input = fn.arg(0).to_string();
    URL::decode(input);
    return as_value(input);
}

// parseFloat (string)
as_value
global_parsefloat(const fn_call& fn)
{
    ASSERT_FN_ARGS_IS_1
    
    std::istringstream s(fn.arg(0).to_string());
    double result;
    
    if (!(s >> result)) {
        return as_value(NaN);   
    }    

    return as_value(result);
}

// parseInt(string[, base])
// The second argument, if supplied, is the base.
// If none is supplied, we have to work out the 
// base from the string. Decimal, octal and hexadecimal are
// possible, according to the following rules:
// 1. If the string starts with 0x or 0X, the number is hex.
// 2. The 0x or 0X may be *followed* by '-' or '+' to indicate sign. A number
//    with no sign is positive.
// 3. If the string starts with 0, -0 or +0 and contains only the DisplayObjects
//    0-7.
// 4. If the string starts with *any* other sequence of DisplayObjects, including
//    whitespace, it is decimal.
as_value
global_parseint(const fn_call& fn)
{
    if (!fn.nargs) {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("%s needs at least one argument"), __FUNCTION__);
        )
        return as_value();
    }

    IF_VERBOSE_ASCODING_ERRORS(
        if (fn.nargs > 2) {
            log_aserror(_("%s has more than two arguments"), __FUNCTION__);
        }
    )

    const std::string& expr = fn.arg(0).to_string();

    // A second argument specifies the base.
    // Parsing still starts after any positive/negative 
    // sign or hex identifier (parseInt("0x123", 8) gives
    // 83, not 0; parseInt(" 0x123", 8) is 0), which is
    // why we do this here.
    size_t base;
    if (fn.nargs > 1)
    {
        base = fn.arg(1).to_int();
    
        // Bases from 2 to 36 are valid, otherwise return NaN
        if (base < 2 || base > 36) return as_value(NaN);
    }
    else
    {
        /// No radix specified, so try parsing as octal or hexadecimal
        try {
            double d;
            if (as_value::parseNonDecimalInt(expr, d, false)) return d;
        }
        catch (boost::bad_lexical_cast&) {
            return as_value(NaN);
        }

        /// The number is not hex or octal, so we'll assume it's base-10.
        base = 10;

    }

    std::string::const_iterator it = expr.begin();

    // Check for expectional case "-0x" or "+0x", which
    // return NaN
    if ((expr.length() > 2) && (*it == '-' || *it == '+') &&
            *(it + 1) == '0' && std::toupper(*(it + 2)) == 'X') {
        return as_value(NaN);
    }
    
    // Try hexadecimal first
    if (expr.substr(0, 2) == "0x" || expr.substr(0, 2) == "0X") it += 2;
    else {
        // Skip leading whitespace
        while(*it == ' ' || *it == '\n' || *it == '\t' || *it == '\r') {
            ++it;
        }
        if (it == expr.end()) return as_value(NaN);
    }    

    bool negative = false;
    if (*it == '-' || *it == '+')
    {
        if (*it == '-') negative = true;
        
        it++;
        if (it == expr.end()) return as_value(NaN);
    }
    
    // Now we have the base, parse the digits. The iterator should
    // be pointing at the first digit.
    
    const std::string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    // Check to see if the first digit is valid, otherwise 
    // return NaN.
    std::string::size_type digit = digits.find(toupper(*it));

    if (digit >= base || digit == std::string::npos) return as_value(NaN);

    // The first digit was valid, so continue from the present position
    // until we reach the end of the string or an invalid DisplayObject,
    // adding valid DisplayObjects to our result.
    // Which DisplayObjects are invalid depends on the base. 
    double result = digit;
    ++it;
    
    while (it != expr.end() && (digit = digits.find(toupper(*it))) < base
            && digit != std::string::npos) {
        result = result * base + digit;
        ++it;
    }

    // Now return the parsed string as an integer.
    return negative ? as_value(-result) : as_value(result);
}

// ASSetPropFlags function
as_value
global_assetpropflags(const fn_call& fn)
{

    if (fn.nargs < 3) {
        IF_VERBOSE_ASCODING_ERRORS(    
            log_aserror(_("%s needs at least three arguments"), __FUNCTION__);
        )
        return as_value();
    }
    
    IF_VERBOSE_ASCODING_ERRORS(
        if (fn.nargs > 4) {
            log_aserror(_("%s has more than four arguments"), "AsSetPropFlags");
        }
    );
    
    // object
    boost::intrusive_ptr<as_object> obj = fn.arg(0).to_object(getGlobal(fn));
    if ( ! obj ) {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Invalid call to ASSetPropFlags: "
            "first argument is not an object: %s"),
            fn.arg(0));
        );
        return as_value();
    }

    // list of child names

    const as_value& props = fn.arg(1);

    const int flagsMask = PropFlags::dontEnum |
                          PropFlags::dontDelete |
                          PropFlags::readOnly |
                          PropFlags::onlySWF6Up |
                          PropFlags::ignoreSWF6 |
                          PropFlags::onlySWF7Up |
                          PropFlags::onlySWF8Up |
                          PropFlags::onlySWF9Up;

    // a number which represents three bitwise flags which
    // are used to determine whether the list of child names should be hidden,
    // un-hidden, protected from over-write, un-protected from over-write,
    // protected from deletion and un-protected from deletion
    const int setTrue = int(fn.arg(2).to_number()) & flagsMask;

    // Is another integer bitmask that works like set_true,
    // except it sets the attributes to false. The
    // set_false bitmask is applied before set_true is applied

    // ASSetPropFlags was exposed in Flash 5, however the fourth argument
    // 'set_false' was not required as it always defaulted to the value '~0'. 
    const int setFalse = (fn.nargs < 4 ? 0 : fn.arg(3).to_int()) &
        flagsMask;

    obj->setPropFlags(props, setFalse, setTrue);

    return as_value();
}

// ASconstructor function
// See: http://osflash.org/flashcoders/undocumented/asnative?s=asnative
as_value
global_asconstructor(const fn_call& fn)
{

    if (fn.nargs < 2)
    {
        IF_VERBOSE_ASCODING_ERRORS(    
        log_aserror(_("ASNative(%s): needs at least two arguments"),
            fn.dump_args());
        )
        return as_value();
    }

    const int sx = fn.arg(0).to_int();
    const int sy = fn.arg(1).to_int();

    if (sx < 0 || sy < 0) {
        IF_VERBOSE_ASCODING_ERRORS(    
            log_aserror(_("ASconstructor(%s): args must be 0 or above"),
                fn.dump_args());
        )
        return as_value();
    }

    const unsigned int x = static_cast<unsigned int>(sx);
    const unsigned int y = static_cast<unsigned int>(sy);

    VM& vm = getVM(fn);
    as_function* fun = vm.getNative(x, y);
    if (!fun) {
        log_debug(_("No ASnative(%d, %d) registered with the VM"), x, y);
        return as_value();
    }

    Global_as& gl = getGlobal(fn);
    fun->init_member(NSV::PROP_PROTOTYPE, gl.createObject());

    return as_value(fun);
        
}

// ASNative function
// See: http://osflash.org/flashcoders/undocumented/asnative?s=asnative
as_value
global_asnative(const fn_call& fn)
{

    as_value ret;

    if (fn.nargs < 2)
    {
        IF_VERBOSE_ASCODING_ERRORS(    
        log_aserror(_("ASNative(%s): needs at least two arguments"),
            fn.dump_args());
        )
        return ret;
    }

    const int sx = fn.arg(0).to_int();
    const int sy = fn.arg(1).to_int();

    if (sx < 0 || sy < 0) {
        IF_VERBOSE_ASCODING_ERRORS(    
            log_aserror(_("ASnative(%s): args must be 0 or above"),
                fn.dump_args());
        )
        return as_value();
    }

    const unsigned int x = static_cast<unsigned int>(sx);
    const unsigned int y = static_cast<unsigned int>(sy);

    VM& vm = getVM(fn);
    as_function* fun = vm.getNative(x, y);
    if ( ! fun ) {
        log_debug(_("No ASnative(%d, %d) registered with the VM"), x, y);
        return ret;
    }
    ret.set_as_function(fun);
    return ret;
        
}

// Obsolete ASnew function (exists only as ASnative(2, 0))
as_value
global_asnew(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("ASNative (2, 0) - old ASnew"));
    return as_value();
}


/// ASSetNative(targetObject, major, properties, minor)
//
/// Sets a series of properties on targetObject using the native table.
/// The third argument is generally documented to be an array, but in fact
/// it is always converted to a string and parsed.
as_value
global_assetnative(const fn_call& fn)
{

    if (fn.nargs < 3) {
        return as_value();
    }

    Global_as& gl = getGlobal(fn);

    as_object* targetObject = fn.arg(0).to_object(gl);
    if (!targetObject) {
        return as_value();
    }

    const int major = fn.arg(1).to_int();
    if (major < 0) return as_value();

    const std::string& props = fn.arg(2).to_string();
    const int minor =
        fn.nargs > 3 ? std::max<boost::int32_t>(fn.arg(3).to_int(), 0) : 0;

    std::string::const_iterator pos = props.begin();

    VM& vm = getVM(fn);

    size_t i = 0;

    // pos is always the position after the last located property.
    while (pos != props.end()) {

        // If there are no further commas, find the end of the string.
        std::string::const_iterator comma = std::find(pos, props.end(), ',');

        const char num = *pos;
        
        int flag;

        switch (num) {
            case '6':
                flag = PropFlags::onlySWF6Up;
                ++pos;
                break;
            case '7':
                flag = PropFlags::onlySWF7Up;
                ++pos;
                break;
            case '8':
                flag = PropFlags::onlySWF8Up;
                ++pos;
                break;
            case '9':
                flag = PropFlags::onlySWF9Up;
                ++pos;
                break;
            default:
                flag = 0;

        }
        const std::string& property = std::string(pos, comma);
        if (!property.empty()) {
            targetObject->init_member(property,
                    vm.getNative(major, minor + i), flag);
        }
        if (comma == props.end()) break;
        pos = comma + 1;
        ++i;
    }
    return as_value();
}

// This is like ASSetNative, but attaches getter/setters.
as_value
global_assetnativeaccessor(const fn_call& fn)
{
    if (fn.nargs < 3) {
        return as_value();
    }

    Global_as& gl = getGlobal(fn);

    as_object* targetObject = fn.arg(0).to_object(gl);
    if (!targetObject) {
        return as_value();
    }

    const int major = fn.arg(1).to_int();
    if (major < 0) return as_value();

    const std::string& props = fn.arg(2).to_string();
    const int minor =
        fn.nargs > 3 ? std::max<boost::int32_t>(fn.arg(3).to_int(), 0) : 0;

    std::string::const_iterator pos = props.begin();

    VM& vm = getVM(fn);

    size_t i = 0;

    // pos is always the position after the last located property.
    while (pos != props.end()) {

        // If there are no further commas, find the end of the string.
        std::string::const_iterator comma = std::find(pos, props.end(), ',');

        const char num = *pos;
        
        int flag;

        switch (num) {
            case '6':
                flag = PropFlags::onlySWF6Up;
                ++pos;
                break;
            case '7':
                flag = PropFlags::onlySWF7Up;
                ++pos;
                break;
            case '8':
                flag = PropFlags::onlySWF8Up;
                ++pos;
                break;
            case '9':
                flag = PropFlags::onlySWF9Up;
                ++pos;
                break;
            default:
                flag = 0;

        }
        const std::string& property = std::string(pos, comma);
        if (!property.empty()) {
            NativeFunction* getset = vm.getNative(major, minor + i);
            targetObject->init_property(property, *getset, *getset, flag);
        }
        if (comma == props.end()) break;
        pos = comma + 1;
        ++i;
    }
    return as_value();
    LOG_ONCE(log_unimpl("ASSetNativeAccessor"));
    return as_value();
}


// updateAfterEvent function
as_value
global_updateAfterEvent(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("updateAfterEvent()"));
    return as_value();
}

as_value
local_errorConstructor(const fn_call& fn)
{
    as_object* obj = ensure<ValidThis>(fn);
    const as_value& arg = fn.nargs ? fn.arg(0) : as_value();
    string_table& st = getStringTable(fn);
    obj->set_member(st.find("message"), arg);
    return as_value();
}


/// Sets a range of Error subclasses.
as_value
global_assetuperror(const fn_call& fn)
{
    if (!fn.nargs) return as_value();

    // This should actually call String.split, but since our Array is
    // wrong we may as well do it like this for now.
    const std::string& errors = fn.arg(0).to_string();

    std::string::const_iterator pos = errors.begin();

    Global_as& gl = getGlobal(fn);

    // pos is always the position after the last located error.
    for (;;) {

        // If there are no further commas, find the end of the string.
        std::string::const_iterator comma = std::find(pos, errors.end(), ',');

        const std::string& err = std::string(pos, comma);

        string_table& st = getStringTable(fn);

        as_function* ctor = gl.getMember(NSV::CLASS_ERROR).to_function();
        if (ctor) {
            fn_call::Args args;
            as_object* proto = ctor->constructInstance(fn.env(), args);

            // Not really sure what the point of this is.
            gl.createClass(local_errorConstructor, proto);
            proto->set_member(st.find("name"), err);
            proto->set_member(st.find("message"), err);
        }
        
        if (comma == errors.end()) break;
        pos = comma + 1;
    }
    return as_value();
}

as_value
global_setInterval(const fn_call& fn)
{
    
	if (fn.nargs < 2) {
		IF_VERBOSE_ASCODING_ERRORS(
			std::stringstream ss; fn.dump_args(ss);
			log_aserror("Invalid call to setInterval(%s) "
				"- need at least 2 arguments",
				ss.str());
		);
		return as_value();
	}

	unsigned timer_arg = 1;

	as_object* obj = fn.arg(0).to_object(getGlobal(fn));
	if (!obj) {

		IF_VERBOSE_ASCODING_ERRORS(
			std::stringstream ss; fn.dump_args(ss);
			log_aserror("Invalid call to setInterval(%s) "
				"- first argument is not an object or function",
				ss.str());
		);
		return as_value();
	}

    string_table::key methodName(0);

	// Get interval function
	as_function* as_func = obj->to_function(); 
	if (!as_func) {
		methodName = getStringTable(fn).find(fn.arg(1).to_string());
		timer_arg = 2;
	}


	if (fn.nargs < timer_arg + 1) {
		IF_VERBOSE_ASCODING_ERRORS(
			std::stringstream ss; fn.dump_args(ss);
			log_aserror("Invalid call to setInterval(%s) "
				"- missing timeout argument",
				ss.str());
        );
		return as_value();
	}

	// Get interval time
	unsigned long ms =
        static_cast<unsigned long>(fn.arg(timer_arg).to_number());
	// TODO: check validity of interval time number ?

	// Parse arguments 
    fn_call::Args args;
	for (unsigned i = timer_arg + 1; i < fn.nargs; ++i) {
		args += fn.arg(i);
	}

	std::auto_ptr<Timer> timer;
	if (as_func) {
		timer.reset(new Timer(*as_func, ms, fn.this_ptr, args));
	}
	else {
		timer.reset(new Timer(obj, methodName, ms, args));
	}
    
	movie_root& root = getRoot(fn);
	int id = root.add_interval_timer(timer);
	return as_value(id);
}

as_value
global_setTimeout(const fn_call& fn)
{
    
	if (fn.nargs < 2) {
		IF_VERBOSE_ASCODING_ERRORS(
			std::stringstream ss; fn.dump_args(ss);
			log_aserror("Invalid call to setTimeout(%s) "
			"- need at least 2 arguments",
			ss.str());
		);
		return as_value();
	}

	unsigned timer_arg = 1;

	as_object* obj = fn.arg(0).to_object(getGlobal(fn));
	if (!obj) {
		IF_VERBOSE_ASCODING_ERRORS(
			std::stringstream ss; fn.dump_args(ss);
			log_aserror("Invalid call to setInterval(%s) "
				"- first argument is not an object or function",
				ss.str());
		);
		return as_value();
	}

    string_table::key methodName(0);

	// Get interval function
	as_function* as_func = obj->to_function(); 
	if (!as_func) {
		methodName = getStringTable(fn).find(fn.arg(1).to_string());
		timer_arg = 2;
	}


	if (fn.nargs < timer_arg + 1) {
		IF_VERBOSE_ASCODING_ERRORS(
			std::stringstream ss; fn.dump_args(ss);
			log_aserror("Invalid call to setTimeout(%s): missing "
                "timeout argument", ss.str());
		);
		return as_value();
	}

	// Get interval time
	unsigned long ms =
        static_cast<unsigned long>(fn.arg(timer_arg).to_number());

	// Parse arguments 
    fn_call::Args args;
	for (unsigned i = timer_arg + 1; i < fn.nargs; ++i) {
		args += fn.arg(i);
	}

	std::auto_ptr<Timer> timer;
	if (as_func) {
		timer.reset(new Timer(*as_func, ms, fn.this_ptr, args, true));
	}
	else {
		timer.reset(new Timer(obj, methodName, ms, args, true));
	}
    
	movie_root& root = getRoot(fn);

	int id = root.add_interval_timer(timer);
	return as_value(id);
}
  
as_value
global_clearInterval(const fn_call& fn)
{
    if (!fn.nargs) {
        IF_VERBOSE_ASCODING_ERRORS(
                log_aserror("clearInterval requires one argument, got none");
        );
        return as_value();
    }

	int id = int(fn.arg(0).to_number());

	movie_root& root = getRoot(fn);
	bool ret = root.clear_interval_timer(id);
	return as_value(ret);
}

as_value
global_showRedrawRegions(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("_global.showRedrawRegions"));
    return as_value();
}

as_value
global_enableDebugConsole(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("_global.enableDebugConsole"));
    return as_value();
}
/// Construct an instance of the specified global class.
//
/// If the class is not present or is not a constructor function, this
/// function throws an ActionTypeError.
//
/// TODO: consider whether ActionTypeError is an appropriate exception.
/// TODO: test the other failure cases.
template<typename T>
as_object*
constructObject(Global_as& gl, const T& arg, string_table::key className)
{
    as_value clval;

    // This is tested in actionscript.all/Object.as to return an 
    // undefined value. We throw the exception back to the VM, which pushes
    // an undefined value onto the stack.
    if (!gl.get_member(className, &clval) ) {
        throw ActionTypeError();
    }
    
    // This is not properly tested.
    if (!clval.is_function()) {
        throw ActionTypeError();
    }
    
    as_function* ctor = clval.to_function();

    // This is also not properly tested.
    if (!ctor) throw ActionTypeError();

    fn_call::Args args;
    args += arg;

    as_environment env(getVM(gl));
    as_object* ret = ctor->constructInstance(env, args);

    return ret;

}

void
registerNatives(as_object& global)
{
    
    VM& vm = getVM(global);

    // ASNew was dropped as an API function but exists
    // as ASnative.
    vm.registerNative(global_assetpropflags, 1, 0);
    vm.registerNative(global_asnew, 2, 0);    
    vm.registerNative(global_assetnative, 4, 0);    
    vm.registerNative(global_assetnativeaccessor, 4, 1);
    vm.registerNative(global_updateAfterEvent, 9, 0);    
    vm.registerNative(global_escape, 100, 0);
    vm.registerNative(global_unescape, 100, 1);
    vm.registerNative(global_parseint, 100, 2);
    vm.registerNative(global_parsefloat, 100, 3);
    vm.registerNative(global_trace, 100, 4);
    vm.registerNative(global_isNaN, 200, 18);
    vm.registerNative(global_isfinite, 200, 19);
    vm.registerNative(global_setInterval, 250, 0);
    vm.registerNative(global_clearInterval, 250, 1);
    vm.registerNative(global_setTimeout, 250, 2);
    
    vm.registerNative(global_showRedrawRegions, 1021, 1);

    registerObjectNative(global);
    registerFunctionNative(global);
    registerStringNative(global);
    registerArrayNative(global);
    registerNumberNative(global);
    registerBooleanNative(global);
    registerMovieClipNative(global);
    registerSelectionNative(global);
    registerColorNative(global);
    registerMathNative(global);
    registerSystemNative(global);
    registerAccessibilityNative(global);
    registerStageNative(global);
    registerTextFieldNative(global);
    registerButtonNative(global);
    registerVideoNative(global);
    registerMovieClipLoaderNative(global);
    registerXMLSocketNative(global);
    registerSharedObjectNative(global);
    registerKeyboardNative(global);
    registerNetStreamNative(global);
    registerCameraNative(global);
    registerMicrophoneNative(global);
    registerTextSnapshotNative(global);
    registerSoundNative(global);
    registerLocalConnectionNative(global);
    registerBitmapFilterNative(global);
    registerColorTransformNative(global);

    AsBroadcaster::registerNative(global);
    registerTextFormatNative(global);
    registerDateNative(global);
    Mouse_as::registerNative(global);

    // LoadableObject has natives shared between LoadVars and XML, so 
    // should be registered first.
    registerLoadableNative(global);
    XMLDocument_as::registerNative(global);
    XMLNode_as::registerNative(global);

}

} // anonymous namespace
} // namespace gnash

