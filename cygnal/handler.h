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

#ifndef __HANDLER_H__
#define __HANDLER_H__ 1

#include <boost/cstdint.hpp>
#include <boost/thread/condition.hpp>
#include <string>
#include <deque>

#include "log.h"
#include "network.h"
#include "buffer.h"
#include "cque.h"
#include "network.h"

// _definst_ is the default instance name
namespace cygnal
{


class Handler : public gnash::Network
{
public:
    Handler();
    ~Handler();

    // This is used to pass parameters to a thread using boost::bind
    typedef struct {
	int netfd;
	int port;
	void *handle;
	std::string filespec;
    } thread_params_t ;
    
    // Specify which queue should be used
    typedef enum { INCOMING, OUTGOING } fifo_e;
    
    // Push bytes on the incoming FIFO, which is the default
    bool push(Buffer *data)
	{ return _incoming.push(data); };
    bool push(Buffer *data, fifo_e direction);
    bool push(gnash::Network::byte_t *data, int nbytes, fifo_e direction);
    bool push(gnash::Network::byte_t *data, int nbytes)
	{ return _incoming.push(data, nbytes); };
    bool pushin(gnash::Network::byte_t *data, int nbytes)
	{ return _incoming.push(data, nbytes); };
    bool pushin(Buffer *data)
	{ return _incoming.push(data); };
    
    // Push bytes on the incoming FIFO, which must be specified
    bool pushout(gnash::Network::byte_t *data, int nbytes)
	{ return _outgoing.push(data, nbytes); };
    bool pushout(Buffer *data)
	{ return _outgoing.push(data); };
    
    // Pop the first date element off the incoming FIFO
    Buffer *pop() { return _incoming.pop(); };
    Buffer *pop(fifo_e direction);
    Buffer *popin()
    	{ return _incoming.pop(); };
    // Pop the first date element off the outgoing FIFO
    Buffer *popout()
    	{ return _outgoing.pop(); };
    
    // Peek at the first data element without removing it
    Buffer *peek() { return _incoming.peek(); };
    Buffer *peek(fifo_e direction);
    Buffer *peekin()
    	{ return _incoming.peek(); };
    // Pop the first date element off the outgoing FIFO
    Buffer *peekout()
    	{ return _outgoing.peek(); };    

    // Removes all the buffers from the queues
    void clear() { _incoming.clear(); };
    void clear(fifo_e direction);
    void clearin() { _incoming.clear(); };
    void clearout() { _outgoing.clear(); };
    void clearall() { _outgoing.clear(); _incoming.clear(); };
    
    // Return the size of the queues, default to the incoming queue
    size_t size(fifo_e direction);
    size_t size() { return _incoming.size(); };
    size_t insize() { return _incoming.size(); };
    size_t outsize() { return _outgoing.size(); };

    // Notify the other thread a message is in the que
    void notify() { _incoming.notify(); };
    void notifyin() { _incoming.notify(); };
    void notifyout() { _outgoing.notify(); };

    // Wait for a message from the other thread
    void wait() { _incoming.wait(); };
    void waitin() { _incoming.wait(); };
    void waitout() { _outgoing.wait(); };

    // start the two thread handlers for the queues
    bool start(thread_params_t *args);

    // Take a buffer and write it to the network
    int writeNet(int fd, Buffer *buf)
    	{ Network::writeNet(fd, buf->reference(), buf->size()); };
    
    int writeNet(Buffer *buf)
    	{ Network::writeNet(buf->reference(), buf->size()); };
    
    // Dump internal data.
    void dump();

    void die() { _die = true; _outgoing.notify(); };
    bool timetodie() { return _die; };
    
private:
    bool _die;
    int _netfd;
    CQue _incoming;
    CQue _outgoing;
};

// This is the thread for all incoming network connections, which
// has to be in C.
extern "C" {
    void netin_handler(Handler::thread_params_t *args);
    void netout_handler(Handler::thread_params_t *args);
}

} // end of cygnal namespace

#endif // end of __HANDLER_H__

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
