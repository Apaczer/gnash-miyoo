// 
//   Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <string>
#include <deque>
#include <list>
#include <map>

#include "log.h"
#include "network.h"
#include "buffer.h"
#include "utility.h"

#include "rtmp.h"
#include "rtmp_server.h"
#include "http.h"

using namespace gnash;
using namespace std;
using namespace boost;

namespace gnash
{

map<int, Handler *> handlers;

Handler::Handler()
    : _die(false), _netfd(0)
{
//    GNASH_REPORT_FUNCTION;
}

Handler::~Handler()
{
//    GNASH_REPORT_FUNCTION;
    closeConnection();
    _die = true;
    notifyout();
    notifyin();
}

bool
Handler::push(amf::Buffer *data, fifo_e direction)
{
//    GNASH_REPORT_FUNCTION;
    if (direction == Handler::OUTGOING) {
	_outgoing.push(data);
	return true;
    }
    if (direction == Handler::INCOMING) {
	_incoming.push(data);
	return true;
    }
    
    return false;
}

// Push bytes on the outgoing FIFO
bool
Handler::push(gnash::Network::byte_t *data, int nbytes, fifo_e direction)
{
//    GNASH_REPORT_FUNCTION;
    amf::Buffer *ptr = new amf::Buffer;
    ptr->copy(data, nbytes);
    return push(ptr, direction);
}

// Pop the first date element off the FIFO
amf::Buffer *
Handler::pop(fifo_e direction)
{
//    GNASH_REPORT_FUNCTION;
    amf::Buffer *buf;
    
    if (direction == Handler::OUTGOING) {
	if (_outgoing.size()) {
	    buf = _outgoing.pop();
	    return buf;	
	}
    }
    if (direction == Handler::INCOMING) {
	if (_incoming.size()) {
	    buf = _incoming.pop();
	    return buf;	
	}
    }

    return buf;
}

// Peek at the first data element without removing it
amf::Buffer *
Handler::peek(fifo_e direction)
{
//    GNASH_REPORT_FUNCTION;
    if (direction == Handler::OUTGOING) {
	if (_outgoing.size()) {
	    return _outgoing.peek();
	}
    }
    if (direction == Handler::INCOMING) {
	if (_incoming.size()) {
	    return _incoming.peek();
	}
    }    
    return 0;
}

// Return the size of the queues
size_t
Handler::size(fifo_e direction)
{
//    GNASH_REPORT_FUNCTION;
    if (direction == Handler::OUTGOING) {
	return _outgoing.size();
    }
    if (direction == Handler::INCOMING) {
	return _incoming.size();
    }
    
    return 0;			// we should never actually get to here
}

// Return the size of the queues
void
Handler::clear(fifo_e direction)
{
//    GNASH_REPORT_FUNCTION;
    if (direction == Handler::OUTGOING) {
	_outgoing.clear();
    }
    if (direction == Handler::INCOMING) {
	_incoming.clear();
    }    
}

// Dump internal data.
void
Handler::dump()
{
//    GNASH_REPORT_FUNCTION;
    _incoming.dump();
    _outgoing.dump();    
}

// start the two thread handlers for the queues
bool
Handler::start(thread_params_t *args)
{
    GNASH_REPORT_FUNCTION;
//    Handler *hand = reinterpret_cast<Handler *>(args->handle);
    
    _incoming.setName("Incoming");
    _outgoing.setName("Outgoing");
    
    log_debug(_("Starting Handlers for port %d, tid %ld"),
	      args->port, get_thread_id());

    if (args->port == 4080) {			// FIXME: hack alert!
	boost::thread handler(boost::bind(&httphandler, args));
    }
    if (args->port == RTMP_PORT) {
	boost::thread handler(boost::bind(&rtmp_handler, args));
    }
    
    boost::thread outport(boost::bind(&netout_handler, args));
    boost::thread inport(boost::bind(&netin_handler, args));
// We don't want to wait for the threads to complete, we
// want to return to the main program so it can spawn another
// thread for the next incoming connection.    
// 	inport.join();    
// 	outport.join();
// 	handler.join();
//     if (_die) {
// 	log_debug("Handler done...");
//     }
    return true;
}
    
extern "C" {
void
netin_handler(Handler::thread_params_t *args)
{
//    GNASH_REPORT_FUNCTION;

    Handler *hand = reinterpret_cast<Handler *>(args->handle);

    log_debug("Starting to wait for data in net for fd #%d", args->netfd);
    
    do {
	amf::Buffer *buf = new amf::Buffer;
	size_t ret = hand->readNet(args->netfd, buf->reference(), buf->size(), 1);
	// the read timed out as there was no data, but the socket is still open.
 	if (ret == 0) {
	    log_debug("no data yet for fd #%d, continuing...", args->netfd);
 	    continue;
 	}
	// ret is "no position" when the socket is closed from the other end of the connection,
	// so we're done.
	if ((ret == string::npos) || (ret == -1)) {
	    log_debug("socket for fd #%d was closed...", args->netfd);
	    break;
	}
	// We got data. Resize the buffer if necessary.
	if (ret > 0) {
	    if (ret < NETBUFSIZE) {
		buf->resize(ret);
	    }
	    hand->push(buf);
	    hand->notify();
	} else {
	    log_debug("no more data for fd #%d, exiting...", args->netfd);
	    hand->die();
	    break;
	}
    } while (!hand->timetodie());
    // We're done. Notify the other threads the socket is closed, and tell them to die.
    log_debug("Net In handler done for fd #%d...", args->netfd);
    hand->notify();
    hand->closeNet(args->netfd);
//    hand->dump();
}

void
netout_handler(Handler::thread_params_t *args)
{
//    GNASH_REPORT_FUNCTION;
    int ret;
    Handler *hand = reinterpret_cast<Handler *>(args->handle);

    log_debug("Starting to wait for data in que for fd #%d", args->netfd);
    
    do {
	// Don't look for any more packets in the que cause we're done
 	if (hand->timetodie()) {
 	    break;
	}
	hand->waitout();
	while (hand->outsize()) {
	    amf::Buffer *buf = hand->popout();
//	    log_debug("FIXME: got data in Outgoing que");
//	    buf->dump();
//	    ret = hand->writeNet(buf->reference(), buf->size(), 15);
// 	    if (buf->size() != gnash::NETBUFSIZE) {
// 			log_debug("Got smaller packet, size %d", buf->size());		
// 	    }
	    ret = hand->writeNet(args->netfd, buf);
	    delete buf;
	}
    } while (ret > 0);
    hand->die();
    log_debug("Net Out handler done for fd #%d...", args->netfd);
    hand->notifyin();
    hand->closeNet(args->netfd);
#if 0
    map<int, Handler *>::iterator hit = handlers.find(args->netfd);
    if ((*hit).second) {
	log_debug("Removing handle %x for fd #%d: ",
		  (void *)hand), args->netfd;
	handlers.erase(args->netfd);
    }
    delete hand;
#endif
}
    
} // end of extern C

} // end of gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

