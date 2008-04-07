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

#include <string>
#include <vector>
#include <deque>

#include "cque.h"
#include "log.h"
#include "gmemory.h"
#include "buffer.h"

using namespace gnash;
using namespace std;
using namespace boost;


namespace gnash
{

CQue::CQue()
{
//    GNASH_REPORT_FUNCTION;
#ifdef USE_STATS_QUEUE
    _stats.totalbytes = 0;
    _stats.totalin = 0;
    _stats.totalout = 0;
    clock_gettime (CLOCK_REALTIME, &_stats.start);
#endif
    _name = "default";
}

CQue::~CQue()
{
//    GNASH_REPORT_FUNCTION;
//    clear();
    Que::iterator it;
    boost::mutex::scoped_lock lock(_mutex);
    for (it = _que.begin(); it != _que.end(); it++) {
	amf::Buffer *ptr = *(it);
	delete ptr;
    }
}

// Wait for a condition variable to trigger
void
CQue::wait()
{
//    GNASH_REPORT_FUNCTION;
    boost::mutex::scoped_lock lk(_cond_mutex);
    _cond.wait(lk);
//    log_debug("wait mutex released for \"%s\"", _name);
}

// Notify a condition variable to trigger
void
CQue::notify()
{
//    GNASH_REPORT_FUNCTION;
    _cond.notify_one();
//    log_debug("wait mutex triggered for \"%s\"", _name);
}

size_t
CQue::size()
{
//    GNASH_REPORT_FUNCTION;
    boost::mutex::scoped_lock lock(_mutex);
    return _que.size();
}

bool
CQue::push(amf::Buffer *data)
{
//    GNASH_REPORT_FUNCTION;
    boost::mutex::scoped_lock lock(_mutex);
    _que.push_back(data);
#ifdef USE_STATS_QUEUE
    _stats.totalbytes += data->size();
    _stats.totalin++;
#endif
    return true;
}

// Push data
bool
CQue::push(gnash::Network::byte_t *data, int nbytes)
{
//    GNASH_REPORT_FUNCTION;
    amf::Buffer *buf = new amf::Buffer;
    std::copy(data, data + nbytes, buf->reference());
    return push(buf);
}


// Pop the first date element off the FIFO
amf::Buffer *
CQue::pop()
{
//    GNASH_REPORT_FUNCTION;
    amf::Buffer *buf = 0;
    boost::mutex::scoped_lock lock(_mutex);
    if (_que.size()) {
        buf = _que.front();
        _que.pop_front();
#ifdef USE_STATS_QUEUE
	_stats.totalout++;
#endif
    }
    return buf;
}

// Peek at the first data element without removing it
amf::Buffer *
CQue::peek()
{
//    GNASH_REPORT_FUNCTION;
    boost::mutex::scoped_lock lock(_mutex);
    if (_que.size()) {
        return _que.front();
    }
    return 0;	
}

// Return the size of the queues
void
CQue::clear()
{
//    GNASH_REPORT_FUNCTION;
    boost::mutex::scoped_lock lock(_mutex);
    _que.clear();
}

// Remove a range of elements
void
CQue::remove(amf::Buffer *begin, amf::Buffer *end)
{
    GNASH_REPORT_FUNCTION;
    deque<amf::Buffer *>::iterator it;
    deque<amf::Buffer *>::iterator start;
    deque<amf::Buffer *>::iterator stop;
    boost::mutex::scoped_lock lock(_mutex);
    amf::Buffer *ptr;
    for (it = _que.begin(); it != _que.end(); it++) {
	ptr = *(it);
	if (ptr->reference() == begin->reference()) {
	    start = it;
	}
	if (ptr->reference() == end->reference()) {
	    stop = it;
	    break;
	}
    }
    _que.erase(start, stop);
}

// Remove an element
void
CQue::remove(amf::Buffer *element)
{
    GNASH_REPORT_FUNCTION;
    deque<amf::Buffer *>::iterator it;
    boost::mutex::scoped_lock lock(_mutex);
    for (it = _que.begin(); it != _que.end(); ) {
	amf::Buffer *ptr = *(it);
	if (ptr->reference() == element->reference()) {
	    it = _que.erase(it);
	} else {
	    it++;
	}
    }
}

// Merge sucessive buffers into one single larger buffer. This is for some
// protocols, than have very long headers.
amf::Buffer *
CQue::merge(amf::Buffer *begin)
{
    GNASH_REPORT_FUNCTION;
    int totalsize = 0;
    Que::iterator it;
    vector<Que::iterator> elements; // iterators to elements to merge
    vector<Que::iterator>::iterator eit;

    boost::mutex::scoped_lock lock(_mutex);
    for (it = _que.begin(); it != _que.end(); it++) {
	amf::Buffer *ptr = *(it);
	if (totalsize > 0) {
	    totalsize += ptr->size();
	    elements.push_back(it); // add this element to the ones to merge
	    if (ptr->size() < gnash::NETBUFSIZE) {// stop merging here
		amf::Buffer *newbuf = new amf::Buffer(totalsize);
		Network::byte_t *tmp = newbuf->reference();
		amf::Buffer *buf;
//		_que.insert(elements.begin(), newbuf);
		for (eit = elements.begin(); eit != elements.end(); eit++) {
 		    deque<amf::Buffer *>::iterator ita = *(eit);
		    buf = *(ita);
		    std::copy(buf->reference(), buf->reference() + buf->size(), tmp);
		    tmp += buf->size();
		    _que.erase(ita);
		}
		_que.push_back(newbuf);
		return newbuf;
	    }
	    continue;
	}
	if (ptr->reference() == begin->reference()) {
	    totalsize = ptr->size();
	    elements.push_back(it);
	}
    }
    return 0;
}

// Dump internal data.
void
CQue::dump()
{
//    GNASH_REPORT_FUNCTION;
    deque<amf::Buffer *>::iterator it;
    boost::mutex::scoped_lock lock(_mutex);
    cerr << endl << "CQue \"" << _name << "\" has "<< _que.size() << " buffers." << endl;
    for (it = _que.begin(); it != _que.end(); it++) {
	amf::Buffer *ptr = *(it);
        ptr->dump();
    }
#ifdef USE_STATS_QUEUE
    struct timespec now;
    clock_gettime (CLOCK_REALTIME, &now);
    cerr << "Que lifespan is " <<
	static_cast<float>((now.tv_sec - _stats.start.tv_sec) + ((now.tv_nsec - _stats.start.tv_nsec)/1e9)) << " seconds" << endl;
    cerr << "Total number of bytes is " << _stats.totalbytes << " bytes" << endl;
    cerr << "Total number of packets pushed to queue is: " << _stats.totalin << endl;
    cerr << "Total number of packets popped from queue is: " << _stats.totalout << endl;
#endif
}

} // end of gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

