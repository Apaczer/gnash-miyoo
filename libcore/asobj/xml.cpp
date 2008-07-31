// xml.cpp:  XML markup language support, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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

#include "log.h"
#include "as_function.h" // for as_function
#include "fn_call.h"
#include "action.h" // for call_method
#include "utf8.h" // for BOM stripping

#include "xmlattrs.h"
#include "xmlnode.h"
#include "xml.h"
#include "builtin_function.h"
#include "debugger.h"
#include "StreamProvider.h"
#include "URLAccessManager.h"
#include "IOChannel.h"
#include "URL.h"
#include "VM.h"
#include "namedStrings.h"
#include "timers.h" // for setting up timers to check loads

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <string>
#include <sstream>
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>
#include <memory>

namespace gnash {
  
//#define DEBUG_MEMORY_ALLOCATION 1

// Define this to enable verbosity of XML loads
//#define DEBUG_XML_LOADS 1

// Define this to enable verbosity of XML parsing
//#define DEBUG_XML_PARSE 1

static as_object* getXMLInterface();
static void attachXMLInterface(as_object& o);
static void attachXMLProperties(as_object& o);

DSOEXPORT as_value xml_new(const fn_call& fn);
static as_value xml_load(const fn_call& fn);
static as_value xml_addrequestheader(const fn_call& fn);
static as_value xml_createelement(const fn_call& fn);
static as_value xml_createtextnode(const fn_call& fn);
static as_value xml_getbytesloaded(const fn_call& fn);
static as_value xml_getbytestotal(const fn_call& fn);
static as_value xml_parsexml(const fn_call& fn);
static as_value xml_send(const fn_call& fn);
static as_value xml_sendandload(const fn_call& fn);
static as_value xml_ondata(const fn_call& fn);

#ifdef USE_DEBUGGER
static Debugger& debugger = Debugger::getDefaultInstance();
#endif

XML::XML() 
    :
    XMLNode(getXMLInterface()),
    //_doc(0),
    //_firstChild(0),
    _loaded(-1), 
    _status(sOK),
    _loadThreads(),
    _loadCheckerTimer(0),
    _bytesTotal(-1),
    _bytesLoaded(-1)
{
    //GNASH_REPORT_FUNCTION;
#ifdef DEBUG_MEMORY_ALLOCATION
    log_debug(_("Creating XML data at %p"), this);
#endif
    //log_debug("%s: %p", __FUNCTION__, this);
    attachXMLProperties(*this);
}


// Parse the ASCII XML string into memory
XML::XML(const std::string& xml_in)
    :
    XMLNode(getXMLInterface()),
    //_doc(0),
    //_firstChild(0),
    _loaded(-1), 
    _status(sOK),
    _loadThreads(),
    _loadCheckerTimer(0),
    _bytesTotal(-1),
    _bytesLoaded(-1)
{
    //GNASH_REPORT_FUNCTION;
#ifdef DEBUG_MEMORY_ALLOCATION
    log_debug(_("Creating XML data at %p"), this);
#endif
    parseXML(xml_in);
}

bool
XML::get_member(string_table::key name, as_value *val, string_table::key nsname)
{
        if (name == NSV::PROP_STATUS) 
        {
                val->set_int(_status);
                return true;
        }
        else if (name == NSV::PROP_LOADED)
        {
                if ( _loaded < 0 ) val->set_undefined();
                else val->set_bool(_loaded);
                return true;
        }

        return get_member_default(name, val, nsname);
}

bool
XML::set_member(string_table::key name, const as_value& val, 
	string_table::key nsname, bool ifFound)
{
        if (name == NSV::PROP_STATUS)
	{
		// TODO: this should really be a proper property (see XML.as)
		if ( ! val.is_number() )
		{
			_status = static_cast<XML::Status>(std::numeric_limits<boost::int32_t>::min());
		}
		else
		{
			unsigned int statusNumber = static_cast<int>(val.to_number());
			_status = XML::Status( static_cast<XML::Status>(statusNumber) );
		}
		return true;
	}
        else if (name == NSV::PROP_LOADED)
        {
		// TODO: this should really be a proper property
                bool b = val.to_bool();
		//log_debug(_("set_member 'loaded' (%s) became boolean %d"), val, b);
                if ( b ) _loaded = 1;
                else _loaded = 0;
                return true;
        }

        return set_member_default(name, val, nsname, ifFound);
}

XML::~XML()
{
    //GNASH_REPORT_FUNCTION;

    for (LoadThreadList::iterator it = _loadThreads.begin(),
		    e = _loadThreads.end(); it != e; ++it)
    {
        delete *it; // supposedly joins the thread
    }

    if ( _loadCheckerTimer )
    {
        VM& vm = getVM();
        vm.getRoot().clear_interval_timer(_loadCheckerTimer);
    }
    
#ifdef DEBUG_MEMORY_ALLOCATION
    log_debug(_("\tDeleting XML top level node at %p"), this);
#endif
  
}

void
XML::onLoadEvent(bool success, as_environment& env)
{
    // Do the events that (appear to) happen as the movie
    // loads.  frame1 tags and actions are executed (even
    // before advance() is called).  Then the onLoad event
    // is triggered.

    as_value	method;
    if (!get_member(NSV::PROP_ON_LOAD, &method) ) return;
    if ( method.is_undefined() ) return;
    if ( ! method.is_function() ) return;

#ifndef NDEBUG
    size_t prevStackSize = env.stack_size();
#endif
    env.push(as_value(success));
    call_method(method, &env, this, 1, env.stack_size()-1);
    env.drop(1);
#ifndef NDEBUG
    assert( prevStackSize == env.stack_size());
#endif
}

void
XML::onCloseEvent(as_environment& env)
{
    // Do the events that (appear to) happen as the movie
    // loads.  frame1 tags and actions are executed (even
    // before advance() is called).  Then the onLoad event
    // is triggered.

    as_value	method;
    if (! get_member(NSV::PROP_ON_CLOSE, &method) ) return;
    if ( method.is_undefined() ) return;
    if ( ! method.is_function() ) return;

    call_method(method, &env, this, 0, 0);
}

bool
XML::extractNode(XMLNode& element, xmlNodePtr node, bool mem)
{
    xmlAttrPtr attr;
    xmlChar *ptr = NULL;
    boost::intrusive_ptr<XMLNode> child;

#ifdef DEBUG_XML_PARSE
    log_debug(_("%s: extracting node %s"), __FUNCTION__, node->name);
#endif

    // See if we have any Attributes (properties)
    attr = node->properties;
    while (attr != NULL)
    {
#ifdef DEBUG_XML_PARSE
        log_debug(_("extractNode %s has property %s, value is %s"),
                  node->name, attr->name, attr->children->content);
#endif
        
        std::ostringstream name, content;

        name << attr->name;
        content << attr->children->content;
        
        XMLAttr attrib(name.str(), content.str());

#ifdef DEBUG_XML_PARSE
        log_debug(_("\tPushing attribute %s for element %s has value %s, next attribute is %p"),
                attr->name, node->name, attr->children->content, attr->next);
#endif

        element._attributes.push_back(attrib);
        attr = attr->next;
    }
    if (node->type == XML_COMMENT_NODE)
    {
    	// Comments apparently not handled until AS3
    	// Comments in a text node are a *sibling* of the text node
    	// for libxml2.
    	return false;
    }
    else if (node->type == XML_ELEMENT_NODE)
    {
            element.nodeTypeSet(tElement);

            std::ostringstream name;
            name << node->name;
            element.nodeNameSet(name.str());
    }
    else if ( node->type == XML_TEXT_NODE )
    {
            element.nodeTypeSet(tText);

            ptr = xmlNodeGetContent(node);
            if (ptr == NULL) return false;
	    if (node->content)
	    {
		std::ostringstream in;
		in << ptr;
		// XML_PARSE_NOBLANKS seems not to be working, so here's
		// a custom implementation of it.
		if ( ignoreWhite() )
		{
			if ( in.str().find_first_not_of(" \n\t\r") == std::string::npos )
			{
#ifdef DEBUG_XML_PARSE
				log_debug("Text node value consists in blanks only, discarding");
#endif
				xmlFree(ptr);
				return false;
			}
		}
		element.nodeValueSet(in.str());
	    }
            xmlFree(ptr);
    }

    // See if we have any data (content)
    xmlNodePtr childnode = node->children;

    while (childnode)
    {
        child = new XMLNode();
        child->setParent(&element);
        if ( extractNode(*child, childnode, mem) )
	{
		element._children.push_back(child);
	}
        childnode = childnode->next;
    }

    return true;
}

/*private*/
bool
XML::parseDoc(xmlNodePtr cur, bool mem)
{
    GNASH_REPORT_FUNCTION;  

    while (cur)
    {
        boost::intrusive_ptr<XMLNode> child = new XMLNode();
        child->setParent(this);
#ifdef DEBUG_XML_PARSE
        log_debug("\tParsing top-level node %s", cur->name);
#endif
        if ( extractNode(*child, cur, mem) ) 
	{
        	_children.push_back(child);
	}
	cur = cur->next;
    }  

    return true;
}

// This reads in an XML file from disk and parses into into a memory resident
// tree which can be walked through later.
bool
XML::parseXML(const std::string& xml_in)
{
    //GNASH_REPORT_FUNCTION;

    //log_debug(_("Parse XML from memory: %s"), xml_in.c_str());

    if (xml_in.empty()) {
        log_error(_("XML data is empty"));
        return false;
    }

    // Clear current data
    clear(); 
    
    initParser();

    xmlNodePtr firstNode; 

    xmlDocPtr doc = xmlReadMemory(xml_in.c_str(), xml_in.size(), NULL, NULL, getXMLOptions()); // do NOT recover here !
    if ( doc )
    {
        firstNode = doc->children; // xmlDocGetRootElement(doc);
    }
    else
    {
        log_debug(_("malformed XML, trying to recover"));
        int ret = xmlParseBalancedChunkMemoryRecover(NULL, NULL, NULL, 0, (const xmlChar*)xml_in.c_str(), &firstNode, 1);
        log_debug("xmlParseBalancedChunkMemoryRecover returned %d", ret);
        if ( ! firstNode )
        {
            log_error(_("unrecoverable malformed XML (xmlParseBalancedChunkMemoryRecover returned %d)."), ret);
            return false;
        }
        else
        {
            log_error(_("recovered malformed XML."));
        }
    }



    bool ret = parseDoc(firstNode, true);

    xmlCleanupParser();
    if ( doc ) xmlFreeDoc(doc); // TOCHECK: can it be freed before ?
    else if ( firstNode ) xmlFreeNodeList(firstNode);
    xmlMemoryDump();

    return ret;
  
}

void
XML::queueLoad(std::auto_ptr<IOChannel> str)
{
    //GNASH_REPORT_FUNCTION;

    // Set the "loaded" parameter to false
    VM& vm = getVM();
    string_table& st = vm.getStringTable();
    string_table::key loadedKey = st.find("loaded");
    set_member(loadedKey, as_value(false));

    bool startTimer = _loadThreads.empty();

    std::auto_ptr<LoadThread> lt ( new LoadThread(str) );

    // we push on the front to avoid invalidating
    // iterators when queueLoad is called as effect
    // of onData invocation.
    // Doing so also avoids processing queued load
    // request immediately
    // 
    _loadThreads.push_front(lt.get());
#ifdef DEBUG_XML_LOADS
    log_debug("Pushed thread %p to _loadThreads, number of XML load threads now: %d", (void*)lt.get(),  _loadThreads.size());
#endif
    lt.release();


    if ( startTimer )
    {
        boost::intrusive_ptr<builtin_function> loadsChecker = \
            new builtin_function(&XML::checkLoads_wrapper);
        std::auto_ptr<Timer> timer(new Timer);
        timer->setInterval(*loadsChecker, 50, this);
        _loadCheckerTimer = getVM().getRoot().add_interval_timer(timer, true);
#ifdef DEBUG_XML_LOADS
        log_debug("Registered XML loads interval %d", _loadCheckerTimer);
#endif
    }

    _bytesLoaded = 0;
    _bytesTotal = -1;

}

long int
XML::getBytesLoaded() const
{
    return _bytesLoaded;
}

long int
XML::getBytesTotal() const
{
    return _bytesTotal;
}

/* private */
void
XML::checkLoads()
{
#ifdef DEBUG_XML_LOADS
    static int call=0;
    log_debug("XML %p checkLoads call %d, _loadThreads: %d", (void *)this, _loadThreads.size(), ++call);
#endif

    if ( _loadThreads.empty() ) return; // nothing to do

    string_table::key onDataKey = NSV::PROP_ON_DATA;

    for (LoadThreadList::iterator it=_loadThreads.begin();
            it != _loadThreads.end(); )
    {
        LoadThread* lt = *it;

        // TODO: notify progress 

	_bytesLoaded = lt->getBytesLoaded();
        _bytesTotal = lt->getBytesTotal();

#ifdef DEBUG_XML_LOADS
        log_debug("XML loads thread %p got %ld/%ld bytes", (void*)lt, lt->getBytesLoaded(), lt->getBytesTotal() );
#endif
        if ( lt->completed() )
        {
            size_t xmlsize = lt->getBytesLoaded();
            boost::scoped_array<char> buf(new char[xmlsize+1]);
            size_t actuallyRead = lt->read(buf.get(), xmlsize);
            if ( actuallyRead != xmlsize )
			{
				// This would be either a bug of LoadThread or an expected
				// possibility which lacks documentation (thus a bug in documentation)
				//
#ifdef DEBUG_XML_LOADS
				log_debug("LoadThread::getBytesLoaded() returned %d but ::read(%d) returned %d",
					xmlsize, xmlsize, actuallyRead);
#endif
			}
            buf[actuallyRead] = '\0';
            // Strip BOM, if any.
            // See http://savannah.gnu.org/bugs/?19915
            utf8::TextEncoding encoding;
            // NOTE: the call below will possibly change 'xmlsize' parameter
            char* bufptr = utf8::stripBOM(buf.get(), xmlsize, encoding);
            if ( encoding != utf8::encUTF8 && encoding != utf8::encUNSPECIFIED )
            {
                log_unimpl("%s to utf8 conversion in XML input parsing", utf8::textEncodingName(encoding));
            }
            as_value dataVal(bufptr); // memory copy here (optimize?)

            it = _loadThreads.erase(it);
            delete lt; // supposedly joins the thread...

            // might push_front on the list..
            callMethod(onDataKey, dataVal);

#ifdef DEBUG_XML_LOADS
            log_debug("Completed load, _loadThreads have now %d elements", _loadThreads.size());
#endif
        }
        else
        {
            ++it;
        }
    }

    if ( _loadThreads.empty() ) 
    {
#ifdef DEBUG_XML_LOADS
        log_debug("Clearing XML load checker interval timer");
#endif
    	VM& vm = getVM();
        vm.getRoot().clear_interval_timer(_loadCheckerTimer);
        _loadCheckerTimer=0;
    }
}

/* private static */
as_value
XML::checkLoads_wrapper(const fn_call& fn)
{
#ifdef DEBUG_XML_LOADS
    log_debug("checkLoads_wrapper called");
#endif

	boost::intrusive_ptr<XML> ptr = ensureType<XML>(fn.this_ptr);
	ptr->checkLoads();
	return as_value();
}

// This reads in an XML file from disk and parses into into a memory resident
// tree which can be walked through later.
bool
XML::load(const URL& url)
{
    GNASH_REPORT_FUNCTION;
  
    //log_debug(_("%s: mem is %d"), __FUNCTION__, mem);

    std::auto_ptr<IOChannel> str ( StreamProvider::getDefaultInstance().getStream(url) );
    if ( ! str.get() ) 
    {
        log_error(_("Can't load XML file: %s (security?)"), url.str().c_str());
        return false;
        // TODO: this is still not correct.. we should still send onData later...
        //as_value nullValue; nullValue.set_null();
        //callMethod(NSV::PROP_ON_DATA, nullValue);
    }

    log_security(_("Loading XML file from url: '%s'"), url.str().c_str());
    queueLoad(str);

    return true;
}


bool
XML::onLoad()
{
    log_debug(_("%s: FIXME: onLoad Default event handler"), __FUNCTION__);

    return(_loaded);
}

void
XML::cleanupStackFrames(XMLNode * /* xml */)
{
    GNASH_REPORT_FUNCTION;
}

/// \brief add or change the HTTP Request header
///
/// Method; adds or changes HTTP request headers (such as Content-Type
/// or SOAPAction) sent with POST actions. In the first usage, you pass
/// two strings to the method: headerName and headerValue. In the
/// second usage, you pass an array of strings, alternating header
/// names and header values.
///
/// If multiple calls are made to set the same header name, each
/// successive value replaces the value set in the previous call.
void
XML::addRequestHeader(const char * /* name */, const char * /* value */)
{
    log_unimpl (__FUNCTION__);
}


void
XML::send()
{
    log_unimpl (__FUNCTION__);
}

bool
XML::sendAndLoad(const URL& url, XML& target)
{
    //GNASH_REPORT_FUNCTION;

    std::stringstream ss;
    toString(ss);
    const std::string& data = ss.str();

    VM& vm = getVM();
    string_table& st = vm.getStringTable();
    string_table::key ctypeKey = st.find("contentType");
    as_value ctypeVal;
    if ( get_member(ctypeKey, &ctypeVal) )
    {
       log_unimpl ("Custom ContentType (%s) in XML.sendAndLoad", ctypeVal);
    }
  
    //log_debug(_("%s: mem is %d"), __FUNCTION__, mem);

    std::auto_ptr<IOChannel> str ( StreamProvider::getDefaultInstance().getStream(url, data) );
    if ( ! str.get() ) 
    {
        log_error(_("Can't load XML file: %s (security?)"), url.str().c_str());
        return false;
        // TODO: this is still not correct.. we should still send onData later...
        //as_value nullValue; nullValue.set_null();
        //callMethod(NSV::PROP_ON_DATA, nullValue);
    }

    log_security(_("Loading XML file from url: '%s'"), url.str().c_str());
    target.queueLoad(str);

    return true;
}


//
// Callbacks. These are the wrappers for the C++ functions so they'll work as
// callbacks from within gnash.
//
as_value
xml_load(const fn_call& fn)
{
    as_value	method;
    as_value	val;
    as_value	rv = false;
    bool          ret;

    //GNASH_REPORT_FUNCTION;
  
    boost::intrusive_ptr<XML> xml_obj = ensureType<XML>(fn.this_ptr);
  
    if ( ! fn.nargs )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("XML.load(): missing argument"));
        );
        return rv;
    }

    const std::string& filespec = fn.arg(0).to_string();

    URL url(filespec, get_base_url());

    // Set the argument to the function event handler based on whether the load
    // was successful or failed.
    ret = xml_obj->load(url);
    rv = ret;

    if (ret == false) {
        return rv;
    }
    
    rv = true;
    return rv;
}

static void
attachXMLProperties(as_object& /*o*/)
{
    // if we use a proper member here hasOwnProperty() would return true
    // but we want it to return false instead. See XML.as
    //o.init_member("status", as_value(XML::sOK));
}

static void
attachXMLInterface(as_object& o)
{
    o.init_member("addRequestHeader", new builtin_function(xml_addrequestheader));
    o.init_member("createElement", new builtin_function(xml_createelement));
    o.init_member("createTextNode", new builtin_function(xml_createtextnode));
    o.init_member("getBytesLoaded", new builtin_function(xml_getbytesloaded));
    o.init_member("getBytesTotal", new builtin_function(xml_getbytestotal));
    o.init_member("load", new builtin_function(xml_load));
    o.init_member("parseXML", new builtin_function(xml_parsexml));
    o.init_member("send", new builtin_function(xml_send));
    o.init_member("sendAndLoad", new builtin_function(xml_sendandload));
    o.init_member("onData", new builtin_function(xml_ondata));

}

static as_object*
getXMLInterface()
{
    static boost::intrusive_ptr<as_object> o;
    if ( o == NULL )
    {
        o = new as_object(getXMLNodeInterface());
        attachXMLInterface(*o);
    }
    return o.get();
}

as_value
xml_new(const fn_call& fn)
{
    as_value      inum;
    boost::intrusive_ptr<XML> xml_obj;
    //const char    *data;
  
    // log_debug(_("%s: nargs=%d"), __FUNCTION__, fn.nargs);
  
    if ( fn.nargs > 0 )
    {
        if ( fn.arg(0).is_object() )
        {
            boost::intrusive_ptr<as_object> obj = fn.arg(0).to_object();
            xml_obj = boost::dynamic_pointer_cast<XML>(obj);
            if ( xml_obj )
            {
                log_debug(_("\tCloned the XML object at %p"), (void *)xml_obj.get());
                return as_value(xml_obj->cloneNode(true).get());
            }
        }

        const std::string& xml_in = fn.arg(0).to_string();
        if ( xml_in.empty() )
        {
            IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("First arg given to XML constructor (%s) evaluates to the empty string"),
                    fn.arg(0));
            );
        }
        else
        {
            xml_obj = new XML(xml_in);
            return as_value(xml_obj.get());
        }
    }

    xml_obj = new XML;
    //log_debug(_("\tCreated New XML object at %p"), xml_obj);

    return as_value(xml_obj.get());
}

//
// SWF Property of this class. These are "accessors" into the private data
// of the class.
//

as_value xml_addrequestheader(const fn_call& fn)
{
    GNASH_REPORT_FUNCTION;
    log_debug(_("%s: %d args"), __PRETTY_FUNCTION__, fn.nargs);
    
//    return as_value(ptr->getAllocated());
//    ptr->addRequestHeader();
    log_unimpl (__FUNCTION__);
    return as_value();
}

/// \brief create a new XML element
///
/// Method; creates a new XML element with the name specified in the
/// parameter. The new element initially has no parent, no children,
/// and no siblings. The method returns a reference to the newly
/// created XML object that represents the element. This method and
/// the XML.createTextNode() method are the constructor methods for
/// creating nodes for an XML object. 

static as_value
xml_createelement(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;
    
    if (fn.nargs > 0) {
        const std::string& text = fn.arg(0).to_string();
	XMLNode *xml_obj = new XMLNode();
	xml_obj->nodeNameSet(text);
	xml_obj->nodeTypeSet(XMLNode::tText);
	// no return code from this method
	return as_value(xml_obj);
	
    } else {
        log_error(_("no text for element creation"));
    }
    return as_value();
}

/// \brief Create a new XML node
/// 
/// Method; creates a new XML text node with the specified text. The
/// new node initially has no parent, and text nodes cannot have
/// children or siblings. This method returns a reference to the XML
/// object that represents the new text node. This method and the
/// XML.createElement() method are the constructor methods for
/// creating nodes for an XML object.

as_value
xml_createtextnode(const fn_call& fn)
{
//    GNASH_REPORT_FUNCTION;

    XMLNode *xml_obj;

    if (fn.nargs > 0) {
	const std::string& text = fn.arg(0).to_string();
	xml_obj = new XMLNode;
	xml_obj->nodeValueSet(text);
	xml_obj->nodeTypeSet(XMLNode::tText);
	return as_value(xml_obj);
//	log_debug(_("%s: xml obj is %p"), __PRETTY_FUNCTION__, xml_obj);
    } else {
	log_error(_("no text for text node creation"));
    }
    return as_value();
}

as_value xml_getbytesloaded(const fn_call& fn)
{
	boost::intrusive_ptr<XML> ptr = ensureType<XML>(fn.this_ptr);
	long int ret = ptr->getBytesLoaded();
	if ( ret < 0 ) return as_value();
	else return as_value(ret);
}

as_value xml_getbytestotal(const fn_call& fn)
{
	boost::intrusive_ptr<XML> ptr = ensureType<XML>(fn.this_ptr);
	long int ret = ptr->getBytesTotal();
	if ( ret < 0 ) return as_value();
	else return as_value(ret);
}

as_value xml_parsexml(const fn_call& fn)
{
    //GNASH_REPORT_FUNCTION;
    as_value	method;
    as_value	val;    
    boost::intrusive_ptr<XML> ptr = ensureType<XML>(fn.this_ptr);

    if (fn.nargs < 1)
    {
        IF_VERBOSE_ASCODING_ERRORS(
        log_aserror("XML.parseXML() needs one argument");
        );
        return as_value();
    }

    const std::string& text = fn.arg(0).to_string();
    ptr->parseXML(text);
    
    return as_value();
}

/// \brief removes the specified XML object from its parent. Also
/// deletes all descendants of the node.
    
as_value xml_send(const fn_call& fn)
{
    GNASH_REPORT_FUNCTION;
    boost::intrusive_ptr<XML> ptr = ensureType<XML>(fn.this_ptr);
    
//    return as_value(ptr->getAllocated());
    ptr->send();
    return as_value();
}

static as_value
xml_sendandload(const fn_call& fn)
{
    GNASH_REPORT_FUNCTION;
    boost::intrusive_ptr<XML> ptr = ensureType<XML>(fn.this_ptr);
    
    if ( fn.nargs < 2 )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        std::stringstream ss;
        fn.dump_args(ss);
        log_aserror(_("XML.sendAndLoad(%s): missing arguments"),
		ss.str().c_str());
        );
        return as_value(false);
    }

    const std::string& filespec = fn.arg(0).to_string();

    boost::intrusive_ptr<as_object> targetObj = fn.arg(1).to_object();
    if ( ! targetObj )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        std::stringstream ss;
        fn.dump_args(ss);
        log_aserror(_("XML.sendAndLoad(%s): second argument doesn't cast to an object"),
		ss.str().c_str());
        );
        return as_value(false);
    }
    XML* target = dynamic_cast<XML*>(targetObj.get());
    if ( ! target )
    {
        IF_VERBOSE_ASCODING_ERRORS(
        std::stringstream ss;
        fn.dump_args(ss);
        log_aserror(_("XML.sendAndLoad(%s): second argument is not an XML object"),
		ss.str().c_str());
        );
        return as_value(false);
    }

    URL url(filespec, get_base_url());

//    return as_value(ptr->getAllocated());
    bool ret = ptr->sendAndLoad(url, *target);

    return ret; // TODO: check expected return values
}

static as_value
xml_ondata(const fn_call& fn)
{
    GNASH_REPORT_FUNCTION;

    string_table::key onLoadKey = NSV::PROP_ON_LOAD;
    string_table::key loadedKey = NSV::PROP_LOADED; 

    as_object* thisPtr = fn.this_ptr.get();
    assert(thisPtr);

    // See http://gitweb.freedesktop.org/?p=swfdec/swfdec.git;a=blob;f=libswfdec/swfdec_initialize.as

    as_value src; src.set_null();
    if ( fn.nargs ) src = fn.arg(0);

    if ( ! src.is_null() )
    {
        string_table::key parseXMLKey = NSV::PROP_PARSE_XML;
        as_value tmp(true);
        thisPtr->set_member(loadedKey, tmp);
        thisPtr->callMethod(parseXMLKey, src);
        thisPtr->callMethod(onLoadKey, tmp);
    }
    else
    {
        as_value tmp(true);
        thisPtr->set_member(loadedKey, tmp);
        thisPtr->callMethod(onLoadKey, tmp);
    }

    return as_value();
}

int
memadjust(int x)
{
    return (x + (4 - x % 4));
}

// extern (used by Global.cpp)
void xml_class_init(as_object& global)
{
//    GNASH_REPORT_FUNCTION;
    // This is going to be the global XML "class"/"function"
    static boost::intrusive_ptr<builtin_function> cl;

    if ( cl == NULL )
    {
        cl=new builtin_function(&xml_new, getXMLInterface());
    }
    
    // Register _global.String
    global.init_member("XML", cl.get());

}

#if 0 // not time for this (yet)
static
void _xmlErrorHandler(void* ctx, const char* fmt, ...)
{
    va_list ap;
    static const unsigned long BUFFER_SIZE = 128;
    char tmp[BUFFER_SIZE];

    va_start (ap, fmt);
    vsnprintf (tmp, BUFFER_SIZE, fmt, ap);
    tmp[BUFFER_SIZE-1] = '\0';

    log_error(_("XML parser: %s"), tmp);
    
    va_end (ap);    
}
#endif // disabled

void
XML::initParser()
{
    static bool initialized = false;
    if ( ! initialized )
    {
        xmlInitParser();
        //xmlGenericErrorFunc func = _xmlErrorHandler;
        //initGenericErrorDefaultFunc(&func);
        initialized = true;
    }
}

void
XML::clear()
{
	// TODO: should set childs's parent to NULL ?
	_children.clear();

	_attributes.clear();
}

/*private*/
bool
XML::ignoreWhite() const
{

	string_table::key propnamekey = VM::get().getStringTable().find("ignoreWhite");
	as_value val;
	if (!const_cast<XML*>(this)->get_member(propnamekey, &val) ) return false;
	return val.to_bool();
}

/*private*/
int
XML::getXMLOptions() const
{
    int options = XML_PARSE_NOENT
		//| XML_PARSE_RECOVER -- don't recover now, we'll call xmlParseBalancedChunkRecover later
		//| XML_PARSE_NOWARNING
    		//| XML_PARSE_NOERROR
		| XML_PARSE_NOCDATA;
    // Using libxml2 to convert CDATA nodes to text seems to be what is
    // required.
    
    if ( ignoreWhite() )
    {
	    // This doesn't seem to work, so the blanks skipping
	    // is actually implemented in XML::extractNode instead.
            //log_debug("Adding XML_PARSE_NOBLANKS to options");
            options |= XML_PARSE_NOBLANKS;
    }

    return options;
}

} // end of gnash namespace

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
