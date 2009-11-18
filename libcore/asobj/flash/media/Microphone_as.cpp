// Microphone_as.cpp:  ActionScript "Microphone" class, for Gnash.
//
//   Copyright (C) 2009 Free Software Foundation, Inc.
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

#include "flash/media/Microphone_as.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "Global_as.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h"
#include "NativeFunction.h"
#include "Object.h"
#include <cmath>

#ifdef USE_GST
#include "gst/AudioInputGst.h"
#endif

#include "AudioInput.h"

namespace gnash {

as_value microphone_ctor(const fn_call& fn);
as_value microphone_get(const fn_call& fn);
as_value microphone_getMicrophone(const fn_call& fn);
as_value microphone_setgain(const fn_call& fn);
as_value microphone_setrate(const fn_call& fn);
as_value microphone_setsilencelevel(const fn_call& fn);
as_value microphone_setuseechosuppression(const fn_call& fn);
as_value microphone_activityLevel(const fn_call& fn);
as_value microphone_gain(const fn_call& fn);
as_value microphone_index(const fn_call& fn);
as_value microphone_muted(const fn_call& fn);
as_value microphone_name(const fn_call& fn);
as_value microphone_names(const fn_call& fn);
as_value microphone_rate(const fn_call& fn);
as_value microphone_silenceLevel(const fn_call& fn);
as_value microphone_silenceTimeout(const fn_call& fn);
as_value microphone_useEchoSuppression(const fn_call& fn);


// get() and names are static properties in AS2.
void
attachMicrophoneStaticInterface(as_object& o)
{
    Global_as& gl = getGlobal(o);

    const int flags = 0;

	o.init_member("get", gl.createFunction(microphone_get), flags);
 
    VM& vm = getVM(o);   
    NativeFunction* getset = vm.getNative(2102, 201);
    o.init_property("names", *getset, *getset);
}

void
attachMicrophoneAS3StaticInterface(as_object& o)
{
    Global_as& gl = getGlobal(o);

    o.init_member("getMicrophone",
            gl.createFunction(microphone_getMicrophone));
}

// These are added to the AS2 prototype when get() is called.
void
attachMicrophoneProperties(as_object& o)
{
    Global_as& gl = getGlobal(o);

    boost::intrusive_ptr<builtin_function> getset;
    getset = gl.createFunction(microphone_activityLevel);
    o.init_property("activityLevel", *getset, *getset);
    getset = gl.createFunction(microphone_gain);
    o.init_property("gain", *getset, *getset);
    getset = gl.createFunction(microphone_index);
    o.init_property("index", *getset, *getset);
    getset = gl.createFunction(microphone_muted);
    o.init_property("muted", *getset, *getset);
    getset = gl.createFunction(microphone_name);
    o.init_property("name", *getset, *getset);
    getset = gl.createFunction(microphone_rate);
    o.init_property("rate", *getset, *getset);
    getset = gl.createFunction(microphone_silenceLevel);
    o.init_property("silenceLevel", *getset, *getset);
    getset = gl.createFunction(microphone_silenceTimeout);
    o.init_property("silenceTimeout", *getset, *getset);
    getset = gl.createFunction(microphone_useEchoSuppression);
    o.init_property("useEchoSuppression", *getset, *getset);
}

static void
attachMicrophoneInterface(as_object& o)
{

    VM& vm = getVM(o);

    const int flags = as_object::DefaultFlags | PropFlags::onlySWF6Up;

	o.init_member("setSilenceLevel", vm.getNative(2104, 0), flags);
	o.init_member("setRate", vm.getNative(2104, 1), flags);
	o.init_member("setGain", vm.getNative(2104, 2), flags);
	o.init_member("setUseEchoSuppression", vm.getNative(2104, 3), flags);
    
}

static as_object*
getMicrophoneInterface()
{
	static boost::intrusive_ptr<as_object> o;
	if ( ! o )
	{
		o = new as_object(getObjectInterface());
		attachMicrophoneInterface(*o);
	}
	return o.get();
}

#ifdef USE_GST
class Microphone_as: public as_object, public media::gst::AudioInputGst
{

public:

	Microphone_as()
		:
		as_object(getMicrophoneInterface())
	{
        attachMicrophoneProperties(*get_prototype());
    }

};
#endif

// FIXME: this should be USE_FFMPEG, but Microphone has no ffmpeg
// support yet.
#ifndef USE_GST
class Microphone_as: public as_object, public media::AudioInput
{

public:

	Microphone_as()
		:
		as_object(getMicrophoneInterface())
	{
        attachMicrophoneProperties(*get_prototype());
    }

};
#endif

// There is a constructor for Microphone that returns an object with
// the correct properties, but it is not usable.
as_value
microphone_ctor(const fn_call& /*fn*/)
{
    return as_value();
}

// AS2 static accessor.
as_value
microphone_get(const fn_call& /*fn*/)
{
    static size_t newcount = 0;
    static boost::intrusive_ptr<Microphone_as> permaMicPtr;
    boost::intrusive_ptr<Microphone_as> ptr;
    if (newcount == 0) {
        log_debug("creating a new microphone_as object");
        ptr = new Microphone_as;
        newcount++;
        permaMicPtr = ptr;
        return as_value(ptr);
    } else {
        return as_value(permaMicPtr);
    }
}

// AS3 static accessor.
as_value
microphone_getMicrophone(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    int numargs = fn.nargs;
    if (numargs > 0) {
        log_debug("%s: the mic is automatically chosen from gnashrc", __FUNCTION__);
    }
    return as_value(ptr); 
}


as_value 
microphone_setgain(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    int numargs = fn.nargs;
    if (numargs != 1) {
        log_error("%s: wrong number of parameters passed", __FUNCTION__);
    } else {
        const int32_t argument = fn.arg(0).to_int();
        if (argument >= 0 && argument <= 100) { 
#ifdef USE_GST
            // gstreamer's gain values can be between -60 and 60,
            // whereas actionscript uses values between 0 and 100.
            // this conversion is made here and the proper
            // value is passed to gstreamer. so, plug the argument
            // into this equation
            // and then send the new value for use with gstreamer
            ptr->set_gain((argument - 50) * 1.2);
            ptr->audioChangeSourceBin(ptr->getGlobalAudio());
#endif
#ifdef USE_FFMPEG
            // haven't yet implemented FFMPEG support for this, so we
            // might need to do a conversion similar to the one above
            // for Gstreamer
            ptr->set_gain(argument);
#endif
        } else {
            //set to highest or lowest gain if bad value was passed
#ifdef USE_GST
            if (argument < 0) {
                ptr->set_gain(-60);
            } else if (argument > 100) {
                ptr->set_gain(60);
            }
#endif
#ifdef USE_FFMPEG
            if (argument < 0) {
                ptr->set_gain(0);
            } else if (argument > 100) {
                ptr->set_gain(100);
            }
#endif
        }
    }
    return as_value();
}


as_value
microphone_setrate(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    int numargs = fn.nargs;
    const int32_t argument = fn.arg(0).to_int();
    
    if (numargs != 1) {
        log_error("%s: wrong number of parameters passed", __FUNCTION__);
    } else if ((argument != 5) && (argument != 8) && (argument != 11) &&
        (argument != 16) && (argument != 22) && (argument != 44)) {
        log_error("%s: invalid rate argument (%d) passed", __FUNCTION__,
            argument);
        //choose the next supported rate
        if (argument > 44) {
            ptr->set_rate(44000);
        } else {
            int supported[] = {5, 8, 11, 16, 22, 44};
            for (size_t i = 0; i < 6; ++i) {
                if (argument < supported[i]) {
                    ptr->set_rate(supported[i]*1000);
                    break;
                } else {
                    continue;
                }
            }
        }
#ifdef USE_GST
        ptr->audioChangeSourceBin(ptr->getGlobalAudio());
#endif
    } else {
        int32_t gstarg = argument * 1000;
        ptr->set_rate(gstarg);
#ifdef USE_GST
        ptr->audioChangeSourceBin(ptr->getGlobalAudio());
#endif
    }
    return as_value();
}

as_value
microphone_activityLevel(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
        
    if ( fn.nargs == 0 ) // getter
    {
        log_unimpl("Microphone::activityLevel only has default value (-1)");
        return as_value(ptr->get_activityLevel());
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set activity property of Microphone"));
        );
    }

    return as_value();
}

as_value
microphone_gain(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
        
    if ( fn.nargs == 0 ) // getter
    {
#ifdef USE_GST
    double gain;
    if (ptr->get_gain() == 0) {
        return as_value(50.0);
    } else {
        gain = ((ptr->get_gain())*(0.8333333333333)) + 50;
        gain = round(gain);
        return as_value(gain);
    }
#else
    UNUSED(ptr);
#endif

        log_unimpl("FFMPEG not implemented. Returning a number");
        return as_value(50.0);
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set gain property of Microphone, use setGain()"));
        );
    }

    return as_value();
}

as_value
microphone_index(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    if ( fn.nargs == 0 ) // getter
    {
        return as_value(ptr->get_index());
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set index property of Microphone"));
        );
    }

    return as_value();
}

as_value
microphone_muted(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    if ( fn.nargs == 0 ) // getter
    {
        log_unimpl("Microphone::muted is always false (always allows access)");
        return as_value(ptr->get_muted());
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set muted property of Microphone"));
        );
    }

    return as_value();
}

as_value
microphone_name(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
        
    if ( fn.nargs == 0 ) // getter
    {
        return as_value(ptr->get_name());
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set name property of Microphone"));
        );
    }

    return as_value();
}


as_value
microphone_names(const fn_call& fn)
{
    // TODO: populate from MediaHandler like Camera.names.
    std::vector<std::string> vect;
    
    size_t size = vect.size();
    
    Global_as& gl = getGlobal(fn);
    as_object* data = gl.createArray();
    
    for (size_t i=0; i < size; ++i) {
        callMethod(data, NSV::PROP_PUSH, vect[i]);
    }
        
    if ( fn.nargs == 0 ) // getter
    {
        return as_value(data);
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set names property of Microphone"));
        );
    }

    return as_value();
} 


as_value
microphone_rate(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    if ( fn.nargs == 0 ) // getter
    {
#ifdef USE_GST
        return as_value(ptr->get_rate()/1000);
#else
        UNUSED(ptr);
        log_unimpl("FFMPEG is unsupported, returning default val");
        return as_value(8);
#endif
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set rate property of Microphone"));
        );
    }

    return as_value();
}

as_value
microphone_silenceLevel(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);

    if ( fn.nargs == 0 ) // getter
    {
        log_unimpl("Microphone::silenceLevel can be set, but is unimplemented");
        return as_value(ptr->get_silenceLevel());
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set silenceLevel property of Microphone, use setSilenceLevel"));
        );
    }

    return as_value();
}

as_value
microphone_silenceTimeout(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
        
    if ( fn.nargs == 0 ) // getter
    {
        log_unimpl("Microphone::silenceTimeout can be set, but is unimplemented");
        return as_value(ptr->get_silenceTimeout());
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set silenceTimeout property of Microphone"));
        );
    }

    return as_value();
}

as_value
microphone_useEchoSuppression(const fn_call& fn)
{
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    if ( fn.nargs == 0 ) // getter
    {
        log_unimpl("Microphone::useEchoSuppression can be set, but is "
                "unimplemented");
        return as_value(static_cast<double>(ptr->get_useEchoSuppression()));
    }
    else // setter
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set useEchoSuppression property of Microphone"));
        );
    }

    return as_value();
}


as_value
microphone_setsilencelevel(const fn_call& fn)
{
    log_unimpl ("Microphone::setSilenceLevel can be set, but it's not "
            "implemented");

    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    int numargs = fn.nargs;
    if (numargs > 2) {
        log_error("%s: Too many arguments", __FUNCTION__);
    } else {
        if (numargs == 2) {
            double argument = fn.arg(0).to_number();
            if ((argument >= 0) && (argument <=100)) {
                //then the arg is valid
                ptr->set_silenceLevel(argument);
            } else {
                log_error("%s: argument 1 out of acceptable range", __FUNCTION__);
                if (argument < 0) {
                    ptr->set_silenceLevel(0);
                } else if (argument > 100) {
                    ptr->set_silenceLevel(100);
                }
            }
            int argument2 = fn.arg(1).to_int();
            if (argument2 >= 0) {
                ptr->set_silenceTimeout(argument2);
            } else {
                log_error("%s: argument 2 out of acceptable range", __FUNCTION__);
                ptr->set_silenceTimeout(0);
            }
        } else {
            double argument = fn.arg(0).to_number();
            if ((argument >= 0) && (argument <=100)) {
                //then the arg is valid
                ptr->set_silenceLevel(argument);
            } else {
                log_error("%s: argument 1 out of acceptable range", __FUNCTION__);
                if (argument < 0) {
                    ptr->set_silenceLevel(0);
                } else if (argument > 100) {
                    ptr->set_silenceLevel(100);
                }
            }
        }
    }
    return as_value();
}

as_value 
microphone_setuseechosuppression(const fn_call& fn)
{
    log_unimpl ("Microphone::setUseEchoSuppression can be set, but it's not "
            "implemented");
    Microphone_as* ptr = ensure<ThisIs<Microphone_as> >(fn);
    
    int numargs = fn.nargs;
    if (numargs > 1) {
        log_error("%s: Too many arguments", __FUNCTION__);
    } else {
        ptr->set_useEchoSuppression(fn.arg(0).to_bool());
    }
    return as_value();
}

// extern (used by Global.cpp)
void
microphone_class_init(as_object& where, const ObjectURI& uri)
{

    Global_as& gl = getGlobal(where);
    
    as_object* proto = getMicrophoneInterface();
    as_object* cl;

    if (isAS3(getVM(where))) {
        cl = gl.createClass(microphone_ctor, proto);
        attachMicrophoneAS3StaticInterface(*cl);
    } else {
        cl = gl.createClass(microphone_ctor, proto);
        attachMicrophoneStaticInterface(*cl);
    }
        
	// Register _global.Microphone
	where.init_member(getName(uri), cl, as_object::DefaultFlags,
            getNamespace(uri));

}

void
registerMicrophoneNative(as_object& global)
{
    VM& vm = getVM(global);
    vm.registerNative(microphone_names, 2104, 201);
    vm.registerNative(microphone_setsilencelevel, 2104, 0);
    vm.registerNative(microphone_setrate, 2104, 1);
    vm.registerNative(microphone_setgain, 2104, 2);
    vm.registerNative(microphone_setuseechosuppression, 2104, 3);
}


} // end of gnash namespace
