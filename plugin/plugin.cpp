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

/* $Id: plugin.cpp,v 1.66 2006/11/09 16:12:43 strk Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
// The name must be this value to get flash movies that check the
// plugin version to load.
#define PLUGIN_NAME     "Shockwave Flash"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":swf:"PLUGIN_NAME

#define PLUGIN_DESCRIPTION \
  "Shockwave Flash 8.0 - Gnash " VERSION ", the GNU Flash Player. Copyright   \
  &copy; 2006 <a href=\"http://www.fsf.org\">Free Software Foundation</a>,    \
  Inc.<br> Gnash comes with NO WARRANTY, to the extent permitted by law.  You \
  may redistribute copies of Gnash under the terms of the                     \
  <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License \
  </a>, with an additional special exception allowing linking with Mozilla,   \
  or any variant of Mozilla (such as Firefox), so long as the linking is      \
  through its standard plug-in interface.  For more information about Gnash,  \
  see <a href=\"http://www.gnu.org/software/gnash/\">                         \
  http://www.gnu.org/software/gnash</a>."


#include <sys/param.h>
#include "plugin.h" //Fixes Warning on redef of MIN/MAX
#include <csignal>
#include <unistd.h>
#include <cstdio>
#include <cstddef>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#include <string>
#include <vector>
#include <iostream>

// Mozilla SDK headers
#include "prinit.h"
#include "prlock.h"
#include "prcvar.h"
#include "prerr.h"
#include "prerror.h"
#include "prthread.h"

using namespace std;

extern NPNetscapeFuncs NPNFuncs;

NPBool      plugInitialized = FALSE;

static bool  waitforgdb = false;

void
PR_CALLBACK Destructor(void * /* data */)
{
#if 0
    /*
     * We don't actually free the storage since it's actually allocated
     * on the stack. Normally, this would not be the case and this is
     * the opportunity to free whatever.
     */
    PR_Free(data);
#endif
}

/// \brief Return the MIME Type description for this plugin.
char*
NPP_GetMIMEDescription(void)
{
    return MIME_TYPES_DESCRIPTION;
}

//
// general initialization and shutdown
//

/// \brief Initialize the plugin
///
/// This C++ function gets called once when the plugin is loaded,
/// regardless of how many instantiations there is actually playing
/// movies. So this is where all the one time only initialization
/// stuff goes.
NPError
NS_PluginInitialize()
{
    NPError err = NPERR_NO_ERROR;
    PRBool supportsXEmbed = PR_TRUE;
    NPNToolkitType toolkit;

    // Make sure that the browser supports functionality we need
    err = CallNPN_GetValueProc(NPNFuncs.getvalue, NULL,
                               NPNVSupportsXEmbedBool,
                               (void *)&supportsXEmbed);

    if (err != NPERR_NO_ERROR || !supportsXEmbed) {
	cout << "No xEmbed support in this Mozilla version!" << endl;
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    } else {
	cout << "xEmbed supported in this Mozilla version" << endl;
    }

    err = CallNPN_GetValueProc(NPNFuncs.getvalue, NULL,
                               NPNVToolkit,
                               (void *)&toolkit);

    if (err != NPERR_NO_ERROR || toolkit != NPNVGtk2) {
	cout << "No GTK2 support in this Mozilla version! Have "
	     << (int)toolkit << endl;
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    } else {
	cout << "Gtk2+ supported in this Mozilla version" << endl;
    }

    plugInitialized = TRUE;

    return NPERR_NO_ERROR;
}

/// \brief Shutdown the plugin
///
/// This C++ function gets called once when the plugin is being
/// shutdown, regardless of how many instantiations actually are
/// playing movies. So this is where all the one time only
/// shutdown stuff goes.
void
NS_PluginShutdown()
{
    if (!plugInitialized) {
	cout << "Plugin already shut down" << endl;
	return;
    }

    plugInitialized = FALSE;
}



/// \brief Retrieve values from the plugin for the Browser
///
/// This C++ function is called by the browser to get certain
/// information is needs from the plugin. This information is the
/// plugin name, a description, etc...
NPError
NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
    NPError err = NPERR_NO_ERROR;

    switch (aVariable) {
      case NPPVpluginNameString:
          *static_cast<char **> (aValue) = PLUGIN_NAME;
          break;

      // This becomes the description field you see below the opening
      // text when you type about:plugins
      case NPPVpluginDescriptionString:
          *static_cast<char **>(aValue) = PLUGIN_DESCRIPTION;
          break;

      case NPPVpluginNeedsXEmbed:
#ifdef HAVE_GTK2
	  *static_cast<PRBool *>(aValue) = PR_TRUE;
#else
	  *static_cast<PRBool *>(aValue) = PR_FALSE;
#endif
	  break;
      case NPPVpluginTimerInterval:
      case NPPVpluginKeepLibraryInMemory:
      default:
          err = NPERR_INVALID_PARAM;
          break;
    }
    return err;
}

/// \brief construct our plugin instance object
///
/// This instantiates a new object via a C++ function used by the
/// browser.
nsPluginInstanceBase *
NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
    if(!aCreateDataStruct)
      return NULL;

    return new nsPluginInstance(aCreateDataStruct);
}

/// \brief destroy our plugin instance object
///
/// This destroys our instantiated object via a C++ function used by the
/// browser.
void
NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
    delete static_cast<nsPluginInstance *> (aPlugin);
}

//
// nsPluginInstance class implementation
//

/// \brief Constructor
nsPluginInstance::nsPluginInstance(nsPluginCreateData* data)
  : nsPluginInstanceBase(),
    _instance(data->instance),
    _window(0),
    _childpid(0)
{
	for (size_t i=0, n=data->argc; i<n; ++i) {
		string name, val;

		if (data->argn[i]) {
			name = data->argn[i];
		}

		if (data->argv[i]) {
			val = data->argv[i];
		}
		//log_msg("PARAM: %s = %s", name.c_str(), val.c_str());
		_params[name] = val;
	}
}

/// \brief Destructor
nsPluginInstance::~nsPluginInstance()
{
}

/// \brief Initialize an instance of the plugin object
/// 
/// This methods initializes the plugin object, and is called for
/// every movie that gets played. This is where the movie playing
/// specific initialization goes.
NPBool
nsPluginInstance::init(NPWindow* aWindow)
{
    if(!aWindow) {
	cout <<  __PRETTY_FUNCTION__ << " ERROR: Window handle was bogus!" << endl;
        return FALSE;
    } else {
	cout << "X origin: = " << aWindow->x 
	     << ", Y Origin = " << aWindow->y
	     << ", Width = " << aWindow->width
	     << ", Height = " << aWindow->height
             << ", WindowID = " << aWindow->window
             << ", this = " << static_cast<void*>(this) << endl;
    }

#if 0
    // Only for developers. Make the plugin block here so we can
    // attach GDB to it.

    bool gdb = true;
    while (gdb) {
	cout << "Waiting for GDB for pid " << getpid() << endl;
	sleep(5);
    }
#endif
    return TRUE;
}

/// \brief Shutdown an instantiated object
///
/// This shuts down an object, and is called for every movie that gets
/// played. This is where the movie playing specific shutdown code
/// goes.
void
nsPluginInstance::shut()
{
    if (_childpid) {
	// it seems that waiting after a SIGINT hangs firefox
	// IFF not run from the console (see bug#17082).
	// SIGTERM instead solves this problem
	kill(_childpid, SIGTERM);
	int status;
	waitpid(_childpid, &status, 0);
	cout << "Child process exited with status " << status << endl;	
    }

    _childpid = 0;
}

/// \brief Set the window to be used to render in
///
/// This sets up the window the plugin is supposed to render
/// into. This calls passes in various information used by the plugin
/// to setup the window. This may get called multiple times by each
/// instantiated object, so it can't do much but window specific
/// setup here.
NPError
nsPluginInstance::SetWindow(NPWindow* aWindow)
{
    if(!aWindow) {
	cout << __FUNCTION__ << ": ERROR: Window handle was bogus!" << endl;
        return NPERR_INVALID_PARAM;
#if 0
    } else {
 	log_msg("%s: X origin = %d, Y Origin = %d, Width = %d,"
 	       " Height = %d, WindowID = %p, this = %p",
 	       __FUNCTION__,
 	       aWindow->x, aWindow->y, aWindow->width, aWindow->height,
 	       aWindow->window, this);
#endif
    }

    _width = aWindow->width;
    _height = aWindow->height;

    _window = reinterpret_cast<Window> (aWindow->window);

    return NPERR_NO_ERROR;
}


NPError
nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
    return NS_PluginGetValue(aVariable, aValue) ;
}

/// \brief Write a status message
///
/// This writes a status message to the status line at the bottom of
/// the browser window and the console firefox was started from.
NPError
nsPluginInstance::WriteStatus(char *msg) const
{
    NPN_Status(_instance, msg);
    cout << msg << endl;

    return NPERR_NO_ERROR;
}

NPError
nsPluginInstance::WriteStatus(string msg) const
{
  return WriteStatus( const_cast<char*>(msg.c_str()) );
}

/// \brief Open a new data stream
///
/// Opens a new incoming data stream, which is the flash movie we want
/// to play.
/// A URL can be pretty ugly, like in this example:
/// http://www.shockwave.com/swf/navbar/navbar_sw.swf?atomfilms=http%3a//www.atomfilms.com/af/home/&shockwave=http%3a//www.shockwave.com&gameblast=http%3a//gameblast.shockwave.com/gb/gbHome.jsp&known=0
/// ../flash/gui.swf?ip_addr=foobar.com&ip_port=3660&show_cursor=true&path_prefix=../flash/&trapallkeys=true"
///
/// So this is where we parse the URL to get all the options passed in
/// when invoking the plugin.
NPError
nsPluginInstance::NewStream(NPMIMEType /* type */, NPStream * stream,
                            NPBool /* seekable */, uint16_t * /* stype */)
{
    string url = stream->url;
    string opts;
    size_t start, end, eq;
    bool dumpopts = false;

#if 0
    log_msg("%s: this = %p, URL is %s", __FUNCTION__,
      (void *)this, stream->url);
#endif

    // extract the parameters from the URL
#ifdef WRITE_FILE
    string fname;
    end   = url.find(".swf", 0) + 4;
    start = url.rfind("/", end) + 1;
    fname = "/tmp/";
    fname += url.substr(start, end - start);
    cout << "The Flash movie name is: " << fname << endl;
#endif
    end   = url.find(".swf", 0) + 4;
    start = url.find("?", end);
    end   = url.size();
    if (start != string::npos) {
	opts = url.substr(start+1, end);
    }

    string name;
    string value;

    cout << __FUNCTION__ << ": The full URL is " << url << endl;
    while (opts.size() > 0) {
	start = 0; // TODO: An empty name seems useless to me. If this is 
	           //       indeed the case, we should set `start' to one.

	eq = opts.find("=", start);
	if (eq == string::npos) {
	    cout << "INFO: Ignoring URL appendix without name." << endl;
	    goto process;
	} 

 	if (opts[0] == '&') {
	    // A (technically invalid) URL like movie.swf?&option=value.
	    start++;
	}

	end = opts.find("&", start);
 	if (end == string::npos) {
	    // We have only one name=value pair remaining.
	    end = opts.size();
 	}
	
	name = opts.substr(start, eq);
	value = opts.substr(eq+1, end-eq-1);
	
	if (dumpopts) {
	    cout << __FUNCTION__ << "Option " << name << " = "
		       << value << endl;
	}
	// Look for our special debug flags
	if (name == "debug") {
 	    cout << __FUNCTION__ << "Debug flag is " << value << endl;
	    if (value == "waitforgdb") {
		waitforgdb = true;
	    }
	    if (value == "dumpopts") {
		dumpopts = true;
	    }
	} else {
	    _options[name] = value;
	}
	if ((opts.size() > end) && (opts[end] == '&')) {
		end++;
	}
	if (end != string::npos) {
	    opts.erase(start, end);
	}
    }

#if 0
    log_msg("%s: URL is %s", __PRETTY_FUNCTION__, url.c_str());
    log_msg("%s: Open stream for %s, this = %p", __FUNCTION__,
    fname.c_str(), (void *)this);
#endif

process:
    _swf_url = url;

#ifdef WRITE_FILE
    _filefd = open(fname.c_str(), O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    if (_filefd < 0) {
        _filefd = open(fname.c_str(), O_TRUNC | O_WRONLY, S_IRUSR|S_IRGRP|S_IROTH);
    }
#endif
    
    startProc(_window);

    return NPERR_NO_ERROR;
}

/// \brief Destroy the data stream we've been reading.
NPError
nsPluginInstance::DestroyStream(NPStream * /* stream */, NPError /* reason */)
{

#if 0
    nsPluginInstance *arg = (nsPluginInstance *)this;
    log_msg("%s: this = %p, URL is %s", __PRETTY_FUNCTION__,
      (void *)arg, stream->url);
#endif

    if (_streamfd != -1) {
        if (close(_streamfd) == -1) {
            perror(strerror(errno));
        } else {
            _streamfd = -1;
        }
    }

#ifdef WRITE_FILE
    if (_filefd != -1) {
        if (close(_filefd) == -1) {
            perror(strerror(errno));
        } else {
            _filefd = -1;
        }
    }
#endif

    if (waitforgdb) {
	cout << "Attach GDB to PID " << getpid() << " to debug!" << endl;
	cout << "This thread will block until then!..." << endl;
	cout <<"Once blocked here, you can set other breakpoints." << endl;
	cout << "do a \"set variable waitforgdb=false\" to continue" << endl;
	while (waitforgdb) {
	    sleep(1);
	}
    }

    return NPERR_NO_ERROR;
}

/// \brief Return how many bytes we can read into the buffer
int32
nsPluginInstance::WriteReady(NPStream * /* stream */ )
{
#if 0
    cout << "Stream for " << stream->url << " is ready" << endl;
#endif
    return 1024;
}

/// \brief Read the data stream from Mozilla/Firefox
///
/// For now we read the bytes and write them to a disk file.
int32
nsPluginInstance::Write(NPStream * /* stream */, int32 /* offset */, int32 len,
                        void * buffer)
{
#if 0
    log_msg("Reading Stream %s, offset is %d, length = %d",
      stream->url, offset, len);
#endif
#ifdef WRITE_FILE
    write(_filefd, buffer, len);
#endif
    return write(_streamfd, buffer, len);
}

void
nsPluginInstance::startProc(Window win)
{
    string procname;
    char *gnash_env = getenv("GNASH_PLAYER");
    if (!gnash_env) {
      procname = PREFIX;
      procname += "/bin/gnash";
    } else {
      procname = gnash_env;
    }

    struct stat procstats;

    // See if the file actually exists, otherwise we can't spawn it
    if (stat(procname.c_str(), &procstats) == -1) {
	cout << "Invalid filename: " << procname << endl;
      return;
    }

    int pipefd[2]; // 0 For reading, 1 for writing.

    int ret = pipe(pipefd);
    if (ret == -1) {
      cout << "ERROR: pipe() failed: " << strerror(errno) << endl;
    }

    _streamfd = pipefd[1];

    _childpid = fork();
    // childpid is -1, if the fork failed, so print out an error message
    if (_childpid == -1) {
      cout << "ERROR: dup2() failed: " << strerror(errno) << endl;
      return;
    }

    // childpid is a positive integer, if we are the parent, and
    // fork() worked
    if (_childpid > 0) {
      ret = close (pipefd[0]); // we want to write, so close read-fd0
      if (ret == -1) {
        cout << "ERROR: close() failed: " << strerror(errno) << endl;
      }

      cout << "Forked sucessfully, child process PID is " << _childpid << endl;

      return;
    }

    // This is the child scope.

    ret = close (pipefd[1]); // We want to read, so close write-fd1
    if (ret == -1) {
      cout << "ERROR: close() failed: " << strerror(errno) << endl;
    }

    // close standard input and direct read-fd1 to standard input
    ret = dup2 (pipefd[0], fileno(stdin));
    if (ret == -1) {
      cout << "ERROR: dup2() failed: " << strerror(errno) << endl;
    }

    // setup the command line

    const char* pageurl = getCurrentPageURL();
    if ( ! pageurl )
    {
	cout << "Could not get current page URL!" << endl;
	//log_msg("UNIMPLEMENTED: current page url: %s", pageurl);
	// invoke gnash with -U <current_page_url>
    }

    const size_t buf_size = 30;
    char xid[buf_size], width[buf_size], height[buf_size];
    snprintf(xid, buf_size, "%ld", win);
    snprintf(width, buf_size, "%d", _width);
    snprintf(height, buf_size, "%d", _height);

    // Write -P values 
    vector<string> paramvalues;
    paramvalues.reserve(_params.size());
    for ( map<string,string>::const_iterator it=_params.begin(),
		itEnd=_params.end();
		it != itEnd; ++it)
    {
        const string& nam=it->first; 
        const string& val=it->second;

        string param = nam + string("=") + val;
        paramvalues.push_back(param);
    }

    // REMEMBER TO INCREMENT THE maxargc COUNT IF YOU
    // ADD NEW ARGUMENTS 

    const size_t maxargc = 16 + paramvalues.size()*2;
    char **argv = new char *[maxargc];

    size_t argc = 0;
    argv[argc++] = const_cast<char*>( procname.c_str() );
    // don't specify rendering flags, so that the rcfile
    // will control that 
    //argv[argc++] = "-r";
    //argv[argc++] = "3";
    argv[argc++] = "-v";
    argv[argc++] = "-x";
    argv[argc++] = xid;
    argv[argc++] = "-j";
    argv[argc++] = width;
    argv[argc++] = "-k";
    argv[argc++] = height;
    argv[argc++] = "-u";
    argv[argc++] = const_cast<char*>( _swf_url.c_str() );
    if ( pageurl )
    {
    	argv[argc++] = "-U";
    	argv[argc++] = const_cast<char*>( pageurl );
    }

    for ( size_t i=0, n=paramvalues.size(); i<n; ++i)
    {
        argv[argc++] = "-P";
        argv[argc++] = const_cast<char*>( paramvalues[i].c_str() );
    }

    argv[argc++] = "-";
    argv[argc++] = 0;

    assert(argc <= maxargc);

    // Start the desired executable and go away
    cout << "Starting process: ";

    for (int i=0; argv[i] != 0; ++i) {
      cout << argv[i] << " ";
    }
    cout << endl;

    execv(argv[0], argv);
    // if execv returns, an error has occurred.
    perror(strerror(errno));

    delete[] argv;

    exit (-1);
}

const char*
nsPluginInstance::getCurrentPageURL() const
{
	NPP npp = _instance;

        NPIdentifier sDocument = NPN_GetStringIdentifier("document");

        NPObject *window;
        NPN_GetValue(npp, NPNVWindowNPObject, &window);

        NPVariant vDoc;
        NPN_GetProperty(npp, window, sDocument, &vDoc);
        NPN_ReleaseObject(window);
        if (!NPVARIANT_IS_OBJECT(vDoc)) {
	    cout << "Can't get window object" << endl;
	    return NULL;
	}
        NPObject* npDoc = NPVARIANT_TO_OBJECT(vDoc);

        NPIdentifier sLocation = NPN_GetStringIdentifier("location");
        NPVariant vLoc;
        NPN_GetProperty(npp, npDoc, sLocation, &vLoc);
        NPN_ReleaseObject(npDoc);
        if (!NPVARIANT_IS_OBJECT(vLoc))
	{
	    cout <<"Can't get window.location object" << endl;
	    return NULL;
	}
        NPObject* npLoc = NPVARIANT_TO_OBJECT(vLoc);

        NPIdentifier sProperty = NPN_GetStringIdentifier("href");
        NPVariant vProp;
        NPN_GetProperty(npp, npLoc, sProperty, &vProp);
        NPN_ReleaseObject(npLoc);
        if (!NPVARIANT_IS_STRING(vProp))
	{
	    cout << "Can't get window.location.href object" << endl;
	    return NULL;
	}
        const NPString& propValue = NPVARIANT_TO_STRING(vProp);

        return propValue.utf8characters; // const char *
}

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
