// LocalConnection.cpp:  Connect two SWF movies & send objects, for Gnash.
// LocalConnection.cpp:  Connect two SWF movies & send objects, for Gnash.
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

#include "GnashSystemIOHeaders.h"

#include "VM.h"
#include "movie_root.h"
#include "URLAccessManager.h"
#include "URL.h"
#include "log.h"
#include "net/LocalConnection_as.h"
#include "network.h"
#include "fn_call.h"
#include "Global_as.h"
#include "builtin_function.h"
#include "NativeFunction.h"
#include "amf.h"
#include "lcshm.h"
#include "Object.h" // for getObjectInterface
#include "namedStrings.h"
#include "StringPredicates.h"

#include <cerrno>
#include <cstring>
#include <boost/cstdint.hpp> // for boost::?int??_t
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
using namespace amf;

// http://www.osflash.org/localconnection
//
// Listening
// To create a listening LocalConnection, you just have to set a thread to:
//
//    1.register the application as a valid LocalConnection listener,
//    2.require the mutex to have exclusive access to the shared memory,
//    3.access the shared memory and check the recipient,
//    4.if you are the recipient, read the message and mark it read,
//    5.release the shared memory and the mutex,
//    6.repeat indefinitely from step 2.
//
// Sending
// To send a message to a LocalConnection apparently works like that:
//    1. require the mutex to have exclusive access to the shared memory,
//    2. access the shared memory and check that the listener is connected,
//    3. if the recipient is registered, write the message,
//    4. release the shared memory and the mutex.
//
// The main thing you have to care about is the timestamp - simply call GetTickCount()
//  and the message size. If your message is correctly encoded, it should be received
// by the listening LocalConnection 
//
// Some facts:
//     * The header is 16 bytes,
//     * The message can be up to 40k,
//     * The listeners block starts at 40k+16 = 40976 bytes,
//     * To add a listener, simply append its name in the listeners list (null terminated strings)
//
namespace {

gnash::RcInitFile& rcfile = gnash::RcInitFile::getDefaultInstance();    

} // anonymous namespace

namespace gnash {

namespace {
    as_value localconnection_connect(const fn_call& fn);
    as_value localconnection_domain(const fn_call& fn);
    as_value localconnection_send(const fn_call& fn);
    as_value localconnection_new(const fn_call& fn);
    as_value localconnection_close(const fn_call& fn);

    bool validFunctionName(const std::string& func);

    void attachLocalConnectionInterface(as_object& o);
    as_object* getLocalConnectionInterface();
}
  
// \class LocalConnection_as
/// \brief Open a connection between two SWF movies so they can send
/// each other Flash Objects to be executed.
///
/// TODO: don't use multiple inheritance.
class LocalConnection_as : public ActiveRelay, public amf::LcShm
{

public:

    LocalConnection_as(as_object* owner);
    virtual ~LocalConnection_as() {}

    void close();

    const std::string& domain() {
        return _domain;
    }

    const std::string& name() { return _name; };

    // TODO: implement this to check for changes.
    virtual void update() {}

private:
    
    /// Work out the domain.
    //
    /// Called once on construction to set _domain, though it will do
    /// no harm to call it again.
    std::string getDomain();
    
    std::string _name;

    // The immutable domain of this LocalConnection_as, based on the 
    // originating SWF's domain.
    const std::string _domain;
    
};

LocalConnection_as::LocalConnection_as(as_object* owner)
    :
    ActiveRelay(owner),
    _domain(getDomain())
{
    log_debug("The domain for this host is: %s", _domain);
    setconnected(false);
}

/// \brief Closes (disconnects) the LocalConnection object.
void
LocalConnection_as::close()
{
    setconnected(false);
#ifndef NETWORK_CONN
    closeMem();
#endif
}

/// \brief Prepares the LocalConnection object to receive commands from a
/// LocalConnection.send() command.
/// 
/// The name is a symbolic name like "lc_name", that is used by the
/// send() command to signify which local connection to send the
/// object to.
/*void
LocalConnection_as::connect(const std::string& name)
{

    assert(!name.empty());

    _name = name;
    
    // TODO: does this depend on success?
    _connected = true;
    
    log_debug("trying to open shared memory segment: \"%s\"", _name);
    
    if (Shm::attach(_name.c_str(), true) == false) {
        return;
    }

    if (Shm::getAddr() <= 0) {
        log_error("Failed to open shared memory segment: \"%s\"", _name);
        return; 
    }
    
    return;
}
*/

/// \brief Returns a string representing the superdomain of the
/// location of the current SWF file.
//
/// This is set on construction, as it should be constant.
/// The domain is either the "localhost", or the hostname from the
/// network connection. This behaviour changed for SWF v7. Prior to v7
/// only the domain was returned, ie dropping off node names like
/// "www". As of v7, the behaviour is to return the full host
/// name. Gnash supports both behaviours based on the version.
std::string
LocalConnection_as::getDomain()
{
    
    URL url(getRoot(owner()).getOriginalURL());

    if (url.hostname().empty()) {
        return "localhost";
    }

    // Adjust the name based on the swf version. Prior to v7, the nodename part
    // was removed. For v7 or later. the full hostname is returned. The
    // localhost is always just the localhost.
    if (getSWFVersion(owner()) > 6) {
        return url.hostname();
    }

    const std::string& domain = url.hostname();

    std::string::size_type pos;
    pos = domain.rfind('.');

    // If there is no '.', return the whole thing.
    if (pos == std::string::npos) {
        return domain;
    }

    pos = domain.rfind(".", pos - 1);
    
    // If there is no second '.', return the whole thing.
    if (pos == std::string::npos) {
        return domain;
    }

    // Return everything after the second-to-last '.'
    // FIXME: this must be wrong, or it would return 'org.uk' for many
    // UK websites, and not even Adobe is that stupid. I think.
    return domain.substr(pos + 1);

}

void
localconnection_class_init(as_object& where, const ObjectURI& uri)
{
    registerBuiltinClass(where, localconnection_new,
            attachLocalConnectionInterface, 0, uri);
}

void
registerLocalConnectionNative(as_object& global)
{
    VM& vm = getVM(global);
    vm.registerNative(localconnection_connect, 2200, 0);
    vm.registerNative(localconnection_send, 2200, 1);
    vm.registerNative(localconnection_close, 2200, 2);
    vm.registerNative(localconnection_domain, 2200, 3);
}


// Anonymous namespace for module-statics
namespace {

/// Instantiate a new LocalConnection object within a flash movie
as_value
localconnection_new(const fn_call& fn)
{
    // TODO: this doesn't happen on construction.
    as_object* obj = ensureType<as_object>(fn.this_ptr).get();
    obj->setRelay(new LocalConnection_as(obj));
    return as_value();
}

/// The callback for LocalConnection::close()
as_value
localconnection_close(const fn_call& fn)
{
    LocalConnection_as* relay =
        ensureNativeType<LocalConnection_as>(fn.this_ptr);
    
    relay->close();
    return as_value();
}

/// The callback for LocalConnectiono::connect()
as_value
localconnection_connect(const fn_call& fn)
{
    LocalConnection_as* relay =
        ensureNativeType<LocalConnection_as>(fn.this_ptr);

    // If already connected, don't try again until close() is called.
    if (relay->getconnected()) return false;

    if (!fn.nargs) {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("LocalConnection.connect() expects exactly "
                    "1 argument"));
        );
        return as_value(false);
    }

    if (!fn.arg(0).is_string()) {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("LocalConnection.connect(): first argument must "
                    "be a string"));
        );
        return as_value(false);
    }
    
    if (fn.arg(0).to_string().empty()) {
        return as_value(false);
    }

    std::string connection_name = relay->domain();    
    connection_name +=":";
    connection_name += fn.arg(0).to_string();
   
    relay->connect(connection_name);

    // We don't care whether connected or not.
    return as_value(true);
}

/// The callback for LocalConnection::domain()
as_value
localconnection_domain(const fn_call& fn)
{
    LocalConnection_as* relay =
        ensureNativeType<LocalConnection_as>(fn.this_ptr);

    return as_value(relay->domain());
}

/// LocalConnection.send()
//
/// Returns false only if the call was syntactically incorrect.
as_value
localconnection_send(const fn_call& fn)
{
    LocalConnection_as* relay =
        ensureNativeType<LocalConnection_as>(fn.this_ptr);

    // At least 2 args (connection name, function) required.

   log_debug(_("The number of args is %d \n"), fn.nargs) ;
     
    if (fn.nargs < 2) {
        IF_VERBOSE_ASCODING_ERRORS(
            std::ostringstream os;
            fn.dump_args(os);
            log_aserror(_("LocalConnection.send(%s): requires at least 2 "
                    "arguments"), os.str());
        );
        return as_value(false);
        
    }

    // Both the first two arguments must be a string
    if (!fn.arg(0).is_string() || !fn.arg(1).is_string()) {
        IF_VERBOSE_ASCODING_ERRORS(
            std::ostringstream os;
            fn.dump_args(os);
            log_aserror(_("LocalConnection.send(%s): requires at least 2 "
                    "arguments"), os.str());
        );
        return as_value(false);
    }

    const std::string& func = fn.arg(1).to_string();

    if (!validFunctionName(func)) {
        IF_VERBOSE_ASCODING_ERRORS(
            std::ostringstream os;
            fn.dump_args(os);
            log_aserror(_("LocalConnection.send(%s): requires at least 2 "
                    "arguments"), os.str());
        );
        return as_value(false);
    }

    
    const std::string& connectionName = fn.arg(0).to_string();
    const std::string& methodName = fn.arg(1).to_string();
        
    std::vector<amf::Element*> argument_to_send;
    
    const size_t numargs = fn.nargs;
    for (size_t i = 2; i != numargs; ++i) {
        amf::Element* temp_ptr = fn.arg(i).to_element().get();
        argument_to_send.push_back(temp_ptr);
    }
    
    relay->amf::LcShm::send(connectionName, methodName, argument_to_send);

    // Now we have a valid call.

    // It is useful to see what's supposed being sent, so we log
    // this every time until implemented.
    std::ostringstream os;
    fn.dump_args(os);
    log_unimpl(_("LocalConnection.send unimplemented %s"), os.str());

    // We'll return true if the LocalConnection is disabled too, as
    // the return value doesn't indicate success of the connection.
    if (rcfile.getLocalConnection() ) {
        log_security("Attempting to write to disabled LocalConnection!");
        return as_value(true);
    }

    return as_value(true);
}


void
attachLocalConnectionInterface(as_object& o)
{
    VM& vm = getVM(o);
    o.init_member("connect", vm.getNative(2200, 0));
    o.init_member("send", vm.getNative(2200, 1));
    o.init_member("close", vm.getNative(2200, 2));
    o.init_member("domain", vm.getNative(2200, 3));
}

/// These names are invalid as a function name.
bool
validFunctionName(const std::string& func)
{

    if (func.empty()) return false;

    typedef std::vector<std::string> ReservedNames;

    static const ReservedNames reserved = boost::assign::list_of
        ("send")
        ("onStatus")
        ("close")
        ("connect")
        ("domain")
        ("allowDomain");

    const ReservedNames::const_iterator it =
        std::find_if(reserved.begin(), reserved.end(),
                boost::bind(StringNoCaseEqual(), _1, func));
        
    return (it == reserved.end());
}

} // anonymous namespace

} // end of gnash namespace

//Adding for testing commit
