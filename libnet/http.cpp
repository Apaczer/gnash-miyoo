// http.cpp:  HyperText Transport Protocol handler for Cygnal, for Gnash.
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

#include <boost/thread/mutex.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/date_time/time_zone_base.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

#include "amf.h"
#include "http.h"
#include "log.h"
#include "network.h"
#include "handler.h"
#include "utility.h"
#include "buffer.h"

using namespace gnash;
using namespace std;

static boost::mutex stl_mutex;

namespace gnash
{

extern map<int, Handler *> handlers;

// FIXME, this seems too small to me.  --gnu
static const int readsize = 1024;

HTTP::HTTP() 
    : _filetype(amf::AMF::FILETYPE_HTML),
      _filesize(0),
      _port(80),
      _keepalive(true),
      _handler(0),
      _clientid(0),
      _index(0)
{
//    GNASH_REPORT_FUNCTION;
//    struct status_codes *status = new struct status_codes;
    
//    _status_codes(CONTINUE, status);
}

HTTP::HTTP(Handler *hand) 
    : _filetype(amf::AMF::FILETYPE_HTML),
      _filesize(0),
      _port(80),
      _keepalive(false)
{
//    GNASH_REPORT_FUNCTION;
    _handler = hand;
}

HTTP::~HTTP()
{
//    GNASH_REPORT_FUNCTION;
}

bool
HTTP::clearHeader()
{
    _header.str("");
    _body.str("");
    _charset.clear();
    _connections.clear();
    _language.clear();
    _encoding.clear();
    _te.clear();
    _accept.clear();
    _filesize = 0;
    _clientid = 0;
    _index = 0;
    return true;
}

HTTP &
HTTP::operator = (HTTP& /*obj*/)
{
    GNASH_REPORT_FUNCTION;
//    this = obj;
    // TODO: FIXME !
    return *this; 
}

bool
HTTP::waitForGetRequest(Network& /*net*/)
{
    GNASH_REPORT_FUNCTION;
    return false;		// FIXME: this should be finished
}

bool
HTTP::waitForGetRequest()
{
    GNASH_REPORT_FUNCTION;

//     Network::byte_t buffer[readsize+1];
//     const char *ptr = reinterpret_cast<const char *>(buffer);
//     memset(buffer, 0, readsize+1);
    
//    _handler->wait();
    amf::Buffer *buf = _handler->pop();

    if (buf == 0) {
	log_debug("Que empty, net connection dropped for fd #%d", _handler->getFileFd());
	return false;
    }
    
    clearHeader();
    extractCommand(buf);    
    extractAccept(buf);
    extractMethod(buf);
    extractReferer(buf);
    extractHost(buf);
    extractAgent(buf);
    extractLanguage(buf);
    extractCharset(buf);
    extractConnection(buf);
    extractKeepAlive(buf);
    extractEncoding(buf);
    extractTE(buf);
//    dump();

    delete buf;			// we're done with the buffer
    
    _filespec = _url;
    if (!_url.empty()) {
	return true;
    }
    return false;
}

bool
HTTP::formatHeader(http_status_e type)
{
//    GNASH_REPORT_FUNCTION;

    formatHeader(_filesize, type);
    return true;
}


bool
HTTP::formatHeader(int filesize, http_status_e type)
{
//    GNASH_REPORT_FUNCTION;

    _header << "HTTP/1.1 200 OK" << "\r\n";
    formatDate();
    formatServer();
//     if (type == NONE) {
// 	formatConnection("close"); // this is the default for HTTP 1.1
//     }
//     _header << "Accept-Ranges: bytes" << "\r\n";
    formatLastModified();
    formatEtag("24103b9-1c54-ec8632c0"); // FIXME: borrowed from tcpdump
    formatAcceptRanges("bytes");
    formatContentLength(filesize);
    formatKeepAlive("timeout=15, max=100");
    formatConnection("Keep-Alive");
    formatContentType(amf::AMF::FILETYPE_HTML);
    // All HTTP messages are followed by a blank line.
    terminateHeader();
    return true;
}

bool
HTTP::formatErrorResponse(http_status_e code)
{
//    GNASH_REPORT_FUNCTION;

    // First build the message body, so we know how to set Content-Length
    _body << "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" << "\r\n";
    _body << "<html><head>" << "\r\n";
    _body << "<title>" << code << " Not Found</title>" << "\r\n";
    _body << "</head><body>" << "\r\n";
    _body << "<h1>Not Found</h1>" << "\r\n";
    _body << "<p>The requested URL " << _filespec << " was not found on this server.</p>" << "\r\n";
    _body << "<hr>" << "\r\n";
    _body << "<address>Cygnal (GNU/Linux) Server at localhost Port " << _port << " </address>" << "\r\n";
    _body << "</body></html>" << "\r\n";
    _body << "\r\n";

    // First build the header
    _header << "HTTP/1.1 " << code << " Not Found" << "\r\n";
    formatDate();
    formatServer();
    _filesize = _body.str().size();
    formatContentLength(_filesize);
    formatConnection("close");
    formatContentType(amf::AMF::FILETYPE_HTML);
    return true;
}

bool
HTTP::formatDate()
{
//    GNASH_REPORT_FUNCTION;
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    
//    cout <<  now.time_of_day() << "\r\n";
    
    boost::gregorian::date d(now.date());
//     boost::gregorian::date d(boost::gregorian::day_clock::local_day());
//     cout << boost::posix_time::to_simple_string(now) << "\r\n";
//     cout << d.day_of_week() << "\r\n";
//     cout << d.day() << "\r\n";
//     cout << d.year() << "\r\n";
//     cout << d.month() << "\r\n";
    
//    boost::date_time::time_zone_ptr zone(new posix_time_zone("MST"));
//    boost::date_time::time_zone_base b(now "MST");
//    cout << zone.dst_zone_abbrev() << "\r\n";

    _header << "Date: " << d.day_of_week();
    _header << ", " << d.day();
    _header << " "  << d.month();
    _header << " "  << d.year();
    _header << " "  << now.time_of_day();
    _header << " GMT" << "\r\n";
    return true;
}

bool
HTTP::formatServer()
{
//    GNASH_REPORT_FUNCTION;
    _header << "Server: Cygnal (GNU/Linux)" << "\r\n";
    return true;
}

bool
HTTP::formatServer(const string &data)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Server: " << data << "\r\n";
    return true;
}

bool
HTTP::formatMethod(const string &data)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Method: " << data << "\r\n";
    return true;
}

bool
HTTP::formatReferer(const string &refer)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Referer: " << refer << "\r\n";
    return true;
}

bool
HTTP::formatConnection(const string &options)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Connection: " << options << "\r\n";
    return true;
}

bool
HTTP::formatKeepAlive(const string &options)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Keep-Alive: " << options << "\r\n";
    return true;
}

bool
HTTP::formatContentType()
{
    return formatContentType(_filetype);
}

bool
HTTP::formatContentType(amf::AMF::filetype_e filetype)
{
//    GNASH_REPORT_FUNCTION;
    
    switch (filetype) {
      case amf::AMF::FILETYPE_HTML:
	  _header << "Content-Type: text/html" << "\r\n";
//	  _header << "Content-Type: text/html; charset=UTF-8" << "\r\n";
	  break;
      case amf::AMF::FILETYPE_SWF:
	  _header << "Content-Type: application/x-shockwave-flash" << "\r\n";
//	  _header << "Content-Type: application/futuresplash" << "\r\n";
	  break;
      case amf::AMF::FILETYPE_VIDEO:
	  _header << "Content-Type: video/flv" << "\r\n";
	  break;
      case amf::AMF::FILETYPE_MP3:
	  _header << "Content-Type: audio/mpeg" << "\r\n";
	  break;
      case amf::AMF::FILETYPE_FCS:
	  _header << "Content-Type: application/x-fcs" << "\r\n";
	  break;
      default:
	  _header << "Content-Type: text/html" << "\r\n";
//	  _header << "Content-Type: text/html; charset=UTF-8" << "\r\n";
    }
    return true;
}

bool
HTTP::formatContentLength()
{
//    GNASH_REPORT_FUNCTION;
    _header << "Content-Length: " << _filesize << "\r\n";
    return true;
}

bool
HTTP::formatContentLength(int filesize)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Content-Length: " << filesize << "\r\n";
    return true;
}

bool
HTTP::formatHost(const string &host)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Host: " << host << "\r\n";
    return true;
}

bool
HTTP::formatAgent(const string &agent)
{
//    GNASH_REPORT_FUNCTION;
    _header << "User-Agent: " << agent << "\r\n";
    return true;
}

bool
HTTP::formatAcceptRanges(const string &range)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Accept-Ranges: " << range << "\r\n";
    return true;
}

bool
HTTP::formatEtag(const string &tag)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Etag: " << tag << "\r\n";
    return true;
}

bool
HTTP::formatLastModified(const string &date)
{
    _header << "Last-Modified: " << date << "\r\n";
}

bool
HTTP::formatLastModified()
{
//    GNASH_REPORT_FUNCTION;
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    stringstream date;
    
    boost::gregorian::date d(now.date());
    
    date << d.day_of_week();
    date << ", " << d.day();
    date << " "  << d.month();
    date << " "  << d.year();
    date << " "  << now.time_of_day();
    date << " GMT";

    return formatLastModified(date.str());
}

bool
HTTP::formatLanguage(const string &lang)
{
//    GNASH_REPORT_FUNCTION;

    // For some browsers this appears to also be Content-Language
    _header << "Accept-Language: " << lang << "\r\n";
    return true;
}

bool
HTTP::formatCharset(const string &set)
{
//    GNASH_REPORT_FUNCTION;
    // For some browsers this appears to also be Content-Charset
    _header << "Accept-Charset: " << set << "\r\n";
    return true;
}

bool
HTTP::formatEncoding(const string &code)
{
//    GNASH_REPORT_FUNCTION;
    _header << "Accept-Encoding: " << code << "\r\n";
    return true;
}

bool
HTTP::formatTE(const string &te)
{
//    GNASH_REPORT_FUNCTION;
    _header << "TE: " << te << "\r\n";
    return true;
}

bool
HTTP::sendGetReply(http_status_e code)
{
    GNASH_REPORT_FUNCTION;
    
    formatHeader(_filesize, code);
//    int ret = Network::writeNet(_header.str());
    amf::Buffer *buf = new amf::Buffer;
//    Network::byte_t *ptr = (Network::byte_t *)_body.str().c_str();
//     buf->copy(ptr, _body.str().size());
//    _handler->dump();
    if (_header.str().size()) {
	buf->resize(_header.str().size());
	string str = _header.str();
	buf->copy(str);
	_handler->pushout(buf);
	_handler->notifyout();
        log_debug (_("Sent GET Reply"));
	return true; // Default to true
    } else {
	clearHeader();
	log_debug (_("Couldn't send GET Reply, no header data"));
    }
    
    return false;
}

bool
HTTP::sendPostReply(rtmpt_cmd_e code)
{
    GNASH_REPORT_FUNCTION;

    _header << "HTTP/1.1 200 OK" << "\r\n";
    formatDate();
    formatServer();
    formatContentType(amf::AMF::FILETYPE_FCS);
    // All HTTP messages are followed by a blank line.
    terminateHeader();
    return true;

#if 0
    formatHeader(_filesize, code);
    amf::Buffer *buf = new amf::Buffer;
    if (_header.str().size()) {
	buf->resize(_header.str().size());
	string str = _header.str();
	buf->copy(str);
	_handler->pushout(buf);
	_handler->notifyout();
        log_debug (_("Sent GET Reply"));
	return true; // Default to true
    } else {
	clearHeader();
	log_debug (_("Couldn't send POST Reply, no header data"));
    }
#endif
    return false;
}

bool
HTTP::formatRequest(const string &url, http_method_e req)
{
//    GNASH_REPORT_FUNCTION;

    _header.str("");

    _header << req << " " << url << "HTTP/1.1" << "\r\n";
    _header << "User-Agent: Opera/9.01 (X11; Linux i686; U; en)" << "\r\n";
    _header << "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1" << "\r\n";

    _header << "Accept-Language: en" << "\r\n";
    _header << "Accept-Charset: iso-8859-1, utf-8, utf-16, *;q=0.1" << "\r\n";
    
    _header << "Accept-Encoding: deflate, gzip, x-gzip, identity, *;q=0" << "\r\n";
    _header << "Referer: " << url << "\r\n";

    _header << "Connection: Keep-Alive, TE" << "\r\n";
    _header << "TE: deflate, gzip, chunked, identity, trailers" << "\r\n";
    return true;
}
// bool
// HTTP::sendGetReply(Network &net)
// {
//     GNASH_REPORT_FUNCTION;    
// }

// This is what a GET request looks like.
// GET /software/gnash/tests/flvplayer2.swf?file=http://localhost:4080/software/gnash/tests/lulutest.flv HTTP/1.1
// User-Agent: Opera/9.01 (X11; Linux i686; U; en)
// Host: localhost:4080
// Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1
// Accept-Language: en
// Accept-Charset: iso-8859-1, utf-8, utf-16, *;q=0.1
// Accept-Encoding: deflate, gzip, x-gzip, identity, *;q=0
// Referer: http://localhost/software/gnash/tests/
// Connection: Keep-Alive, TE
// TE: deflate, gzip, chunked, identity, trailers
int
HTTP::extractAccept(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos;
    string pattern = "Accept: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//	    return "error";
    }

    length = end-start-pattern.size();
    start = start+pattern.size();
    pos = start;
    while (pos <= end) {
	pos = (body.find(",", start) + 2);
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos > end)) {
	    length = end - start;
	} else {
	    length = pos - start - 2;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_accept.push_back(substr);
	start = pos;
    }

    return _accept.size();
}

/// These methods extract data from an RTMPT message. RTMP is an
/// extension to HTTP that adds commands to manipulate the
/// connection's persistance.
//
/// The URL to be opened has the following form:
/// http://server/<comand>/[<client>/]<index>
/// <command>
///    denotes the RTMPT request type, "OPEN", "SEND", "IDLE", "CLOSE")
/// <client>
///    specifies the id of the client that performs the requests
///    (only sent for established sessions)
/// <index>
///    is a consecutive number that seems to be used to detect missing packages
HTTP::rtmpt_cmd_e
HTTP::extractRTMPT(gnash::Network::byte_t *data)
{
    GNASH_REPORT_FUNCTION;

    string body = reinterpret_cast<const char *>(data);
    string tmp, cid, indx;
    HTTP::rtmpt_cmd_e cmd;

    // force the case to make comparisons easier
    std::transform(body.begin(), body.end(), body.begin(), 
               (int(*)(int)) toupper);
    string::size_type start, end;

    // Extract the command first
    start = body.find("OPEN", 0);
    if (start != string::npos) {
        cmd = HTTP::OPEN;
    }
    start = body.find("SEND", 0);
    if (start != string::npos) {
        cmd = HTTP::SEND;
    }
    start = body.find("IDLE", 0);
    if (start != string::npos) {
        cmd = HTTP::IDLE;
    }
    start = body.find("CLOSE", 0);
    if (start != string::npos) {
        cmd = HTTP::CLOSE;
    }

    // Extract the optional client id
    start = body.find("/", start+1);
    if (start != string::npos) {
	end = body.find("/", start+1);
	if (end != string::npos) {
	    indx = body.substr(end, body.size());
	    cid = body.substr(start, (end-start));
	} else {
	    cid = body.substr(start, body.size());
	}
    }

    _index = strtol(indx.c_str(), NULL, 0);
    _clientid = strtol(cid.c_str(), NULL, 0);
    end =  body.find("\r\n", start);
//     if (end != string::npos) {
//         cmd = HTTP::CLOSE;
//     }

    return cmd;
}

HTTP::http_method_e
HTTP::extractCommand(gnash::Network::byte_t *data)
{
//    GNASH_REPORT_FUNCTION;

    string body = reinterpret_cast<const char *>(data);
    HTTP::http_method_e cmd;

    // force the case to make comparisons easier
//     std::transform(body.begin(), body.end(), body.begin(), 
//                (int(*)(int)) toupper);
    string::size_type start, end;

    // Extract the command
    start = body.find("GET", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_GET;
    }
    start = body.find("POST", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_POST;
    }
    start = body.find("HEAD", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_HEAD;
    }
    start = body.find("CONNECT", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_CONNECT;
    }
    start = body.find("TRACE", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_TRACE;
    }
    start = body.find("OPTIONS", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_OPTIONS;
    }
    start = body.find("PUT", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_PUT;
    }
    start = body.find("DELETE", 0);
    if (start != string::npos) {
        cmd = HTTP::HTTP_DELETE;
    }

    _command = cmd;
    return cmd;
}

string &
HTTP::extractAcceptRanges(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos;
    string pattern = "Accept-Ranges: ";
    start = body.find(pattern, 0);
    if (start == string::npos) {
        _acceptranges = "error";
        return _acceptranges;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
        _acceptranges = "error";
        return _acceptranges;
    }
    
    _acceptranges = body.substr(start+pattern.size(), end-start-1);
    return _acceptranges;    
}

string &
HTTP::extractMethod(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    boost::mutex::scoped_lock lock(stl_mutex);
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end;
    int length;

    length = body.size();
    start = body.find(" ", 0);
    if (start == string::npos) {
        _method = "error";
        return _method;
    }
    _method = body.substr(0, start);
    end = body.find(" ", start+1);
    if (end == string::npos) {
        _method = "error";
        return _method;
    }
    _url = body.substr(start+1, end-start-1);
    _version = body.substr(end+1, length);

    end = _url.find("?", 0);
//    _filespec = _url.substr(start+1, end);
    return _method;
}

string &
HTTP::extractReferer(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end;
    string pattern = "Referer: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
	_referer = "error";
	return _referer;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	_referer = "error";
        return _referer;
    }
    
    _referer = body.substr(start+pattern.size(), end-start-1);
    return _referer;
}

int
HTTP::extractConnection(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos;
    string pattern = "Connection: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//	    return "error";
    }

    length = end-start-pattern.size();
    start = start+pattern.size();
    string _connection = body.substr(start, length);
    pos = start;
    while (pos <= end) {
	pos = (body.find(",", start) + 2);
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos > end)) {
	    length = end - start;
	} else {
	    length = pos - start - 2;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_connections.push_back(substr);
	// Opera uses upper case first letters, Firefox doesn't.
	if ((substr == "Keep-Alive") || (substr == "keep-alive")) {
	    _keepalive = true;
	}
	start = pos;
    }

    return _connections.size();
}

int
HTTP::extractKeepAlive(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos;
    string pattern = "Keep-Alive: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//	    return "error";
    }

    length = end-start-pattern.size();
    start = start+pattern.size();
    string _connection = body.substr(start, length);
    pos = start;
    while (pos <= end) {
	pos = (body.find(",", start) + 2);
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos > end)) {
	    length = end - start;
	} else {
	    length = pos - start - 2;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_kalive.push_back(substr);
	_keepalive = true;	// if we get this header setting, we want to keep alive
	start = pos;
    }

    return _connections.size();
}

string &
HTTP::extractHost(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end;
    string pattern = "Host: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        _host = "error"; 
        return _host;
   }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
        _host = "error"; 
        return _host;
    }
    
    _host = body.substr(start+pattern.size(), end-start-1);
    return _host;
}

string &
HTTP::extractAgent(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end;
    string pattern = "User-Agent: ";
    _agent = "error";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return _agent;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
        return _agent;
    }
    
    _agent = body.substr(start+pattern.size(), end-start-1);
    return _agent;
}

int
HTTP::extractLanguage(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos, terminate;
    // match both Accept-Language and Content-Language
    string pattern = "-Language: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//        return "error";
    }
    length = end-start-pattern.size();
    start = start+pattern.size();
    pos = start;
    terminate = (body.find(";", start));
    if (terminate == string::npos) {
	terminate = end;
    }
    
    while (pos <= end) {
	pos = (body.find(",", start));
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos >= terminate)) {
	    length = terminate - start;
	} else {
	    length = pos - start;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_language.push_back(substr);
	start = pos + 1;
    }
    
//    _language = body.substr(start+pattern.size(), end-start-1);
    return _language.size();
}

int
HTTP::extractCharset(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos, terminate;
// match both Accept-Charset and Content-Charset
    string pattern = "-Charset: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//        return "error";
    }
    
    length = end-start-pattern.size();
    start = start+pattern.size();
    string _connection = body.substr(start, length);
    pos = start;
    terminate = (body.find(";", start));
    if (terminate == string::npos) {
	terminate = end;
    }
    while (pos <= end) {
	pos = (body.find(",", start) + 2);
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos >= terminate)) {
	    length = terminate - start;
	} else {
	    length = pos - start - 2;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_charset.push_back(substr);
	start = pos;
    }
//    _charset = body.substr(start+pattern.size(), end-start-1);
    return _charset.size();
}

int
HTTP::extractEncoding(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos, terminate;
    // match both Accept-Encoding and Content-Encoding
    string pattern = "-Encoding: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end =  body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//        return "error";
    }
    
   length = end-start-pattern.size();
    start = start+pattern.size();
    string _connection = body.substr(start, length);
    pos = start;
    // Drop anything after a ';' character
    terminate = (body.find(";", start));
    if (terminate == string::npos) {
	terminate = end;
    }
    while (pos <= end) {
	pos = (body.find(",", start) + 2);
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos >= terminate)) {
	    length = terminate - start;
	} else {
	    length = pos - start - 2;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_encoding.push_back(substr);
	start = pos;
    }

//    _encoding = body.substr(start+pattern.size(), end-start-1);
    return _encoding.size();
}

int
HTTP::extractTE(Network::byte_t *data) {
//    GNASH_REPORT_FUNCTION;
    
    string body = reinterpret_cast<const char *>(data);
    string::size_type start, end, length, pos;
    string pattern = "TE: ";
    
    start = body.find(pattern, 0);
    if (start == string::npos) {
        return -1;
    }
    end = body.find("\r\n", start);
    if (end == string::npos) {
	end = body.find("\n", start);
//        return "error";
    }
    
    length = end-start-pattern.size();
    start = start+pattern.size();
    pos = start;
    while (pos <= end) {
	pos = (body.find(",", start));
	if (pos <= start) {
	    return _encoding.size();
	}
	if ((pos == string::npos) || (pos >= end)) {
	    length = end - start;
	} else {
	    length = pos - start;
	}
	string substr = body.substr(start, length);
//	printf("FIXME: \"%s\"\n", substr.c_str());
	_te.push_back(substr);
	start = pos + 2;
    }
    return _te.size();
}

// Get the file type, so we know how to set the
// Content-type in the header.
amf::AMF::filetype_e
HTTP::getFileStats(std::string &filespec)
{
//    GNASH_REPORT_FUNCTION;    
    bool try_again = true;
    string actual_filespec = filespec;
    struct stat st;

    while (try_again) {
	try_again = false;
//	cerr << "Trying to open " << actual_filespec << "\r\n";
	if (stat(actual_filespec.c_str(), &st) == 0) {
	    // If it's a directory, then we emulate what apache
	    // does, which is to load the index.html file in that
	    // directry if it exists.
	    if (S_ISDIR(st.st_mode)) {
		log_debug("%s is a directory\n", actual_filespec.c_str());
		if (actual_filespec[actual_filespec.size()-1] != '/') {
		    actual_filespec += '/';
		}
		actual_filespec += "index.html";
		try_again = true;
		continue;
	    } else { 		// not a directory
		log_debug("%s is not a directory\n", actual_filespec.c_str());
		_filespec = actual_filespec;
		string::size_type pos;
		pos = filespec.rfind(".");
		if (pos != string::npos) {
		    string suffix = filespec.substr(pos, filespec.size());
		    if (suffix == "html") {
			_filetype = amf::AMF::FILETYPE_HTML;
			log_debug("HTML content found");
		    }
		    if (suffix == "swf") {
			_filetype = amf::AMF::FILETYPE_SWF;
			log_debug("SWF content found");
		    }
		    if (suffix == "flv") {
			_filetype = amf::AMF::FILETYPE_VIDEO;
			log_debug("FLV content found");
		    }
		    if (suffix == "mp3") {
			_filetype = amf::AMF::FILETYPE_AUDIO;
			log_debug("MP3 content found");
		    }
		}
	    }
	} else {
	    _filetype = amf::AMF::FILETYPE_ERROR;
	} // end of stat()
    } // end of try_waiting

    _filesize = st.st_size;
    return _filetype;
}

void
HTTP::dump() {
//    GNASH_REPORT_FUNCTION;
    
    boost::mutex::scoped_lock lock(stl_mutex);
    vector<string>::iterator it;
    
    log_debug (_("==== The HTTP header breaks down as follows: ===="));
    log_debug (_("Filespec: %s"), _filespec.c_str());
    log_debug (_("URL: %s"), _url.c_str());
    log_debug (_("Version: %s"), _version.c_str());
    for (it = _accept.begin(); it != _accept.end(); it++) {
        log_debug("Accept param: \"%s\"", (*(it)).c_str());
    }
    log_debug (_("Method: %s"), _method.c_str());
    log_debug (_("Referer: %s"), _referer.c_str());
    log_debug (_("Connections:"));
    for (it = _connections.begin(); it != _connections.end(); it++) {
        log_debug("Connection param is: \"%s\"", (*(it)).c_str());
    }
    log_debug (_("Host: %s"), _host.c_str());
    log_debug (_("User Agent: %s"), _agent.c_str());
    for (it = _language.begin(); it != _language.end(); it++) {
        log_debug("Language param: \"%s\"", (*(it)).c_str());
    }
    for (it = _charset.begin(); it != _charset.end(); it++) {
        log_debug("Charset param: \"%s\"", (*(it)).c_str());
    }
    for (it = _encoding.begin(); it != _encoding.end(); it++) {
        log_debug("Encodings param: \"%s\"", (*(it)).c_str());
    }
    for (it = _te.begin(); it != _te.end(); it++) {
        log_debug("TE param: \"%s\"", (*(it)).c_str());
    }

    // Dump the RTMPT fields
    log_debug("RTMPT optional index is: ", _index);
    log_debug("RTMPT optional client ID is: ", _clientid);
    log_debug (_("==== ==== ===="));
}

extern "C" {
void
httphandler(Handler::thread_params_t *args)
{
    GNASH_REPORT_FUNCTION;
    int retries = 10;
//    struct thread_params thread_data;
    string url, filespec, parameters;
    string::size_type pos;
    Handler *hand = reinterpret_cast<Handler *>(args->handle);
    HTTP www;
    www.setHandler(hand);

    log_debug(_("Starting HTTP Handler for fd #%d, tid %ld"),
	      args->netfd, get_thread_id());
    
    string docroot = args->filespec;
    
    while (!hand->timetodie()) {	
 	log_debug(_("Waiting for HTTP GET request on fd #%d..."), args->netfd);
	hand->wait();
	// This thread is the last to wake up when the browser
	// closes the network connection. When browsers do this
	// varies, elinks and lynx are very forgiving to a more
	// flexible HTTP protocol, which Firefox/Mozilla & Opera
	// are much pickier, and will hang or fail to load if
	// you aren't careful.
	if (hand->timetodie()) {
	    log_debug("Not waiting no more, no more for more HTTP data for fd #%d...", args->netfd);
	    map<int, Handler *>::iterator hit = handlers.find(args->netfd);
	    if ((*hit).second) {
		log_debug("Removing handle %x for HTTP on fd #%d", (void *)hand), args->netfd;
		handlers.erase(args->netfd);
		delete (*hit).second;
	    }
	    return;
	}
#ifdef USE_STATISTICS
	struct timespec start;
	clock_gettime (CLOCK_REALTIME, &start);
#endif
	
// 	conndata->statistics->setFileType(NetStats::RTMPT);
// 	conndata->statistics->startClock();
//	args->netfd = www.getFileFd();
	if (!www.waitForGetRequest()) {
	    hand->clearout();	// remove all data from the outgoing que
	    hand->die();	// tell all the threads for this connection to die
	    hand->notifyin();
	    log_debug("Net HTTP done for fd #%d...", args->netfd);
// 	    hand->closeNet(args->netfd);
	    return;
	}
	url = docroot;
	url += www.getURL();
	pos = url.find("?");
	filespec = url.substr(0, pos);
	parameters = url.substr(pos + 1, url.size());
	// Get the file size for the HTTP header
	
	if (www.getFileStats(filespec) == amf::AMF::FILETYPE_ERROR) {
	    www.formatErrorResponse(HTTP::NOT_FOUND);
	}
	www.sendGetReply(HTTP::LIFE_IS_GOOD);
//	strcpy(thread_data.filespec, filespec.c_str());
//	thread_data.statistics = conndata->statistics;
	
	// Keep track of the network statistics
//	conndata->statistics->stopClock();
// 	log_debug (_("Bytes read: %d"), www.getBytesIn());
// 	log_debug (_("Bytes written: %d"), www.getBytesOut());
//	st.setBytes(www.getBytesIn() + www.getBytesOut());
//	conndata->statistics->addStats();
	
	if (filespec[filespec.size()-1] == '/') {
	    filespec += "/index.html";
	}
	if (url != docroot) {
	    log_debug (_("File to load is: %s"), filespec.c_str());
	    log_debug (_("Parameters are: %s"), parameters.c_str());
	    struct stat st;
	    int filefd, ret;
#ifdef USE_STATISTICS
	    struct timespec start;
	    clock_gettime (CLOCK_REALTIME, &start);
#endif
	    if (stat(filespec.c_str(), &st) == 0) {
		filefd = ::open(filespec.c_str(), O_RDONLY);
		log_debug (_("File \"%s\" is %lld bytes in size, disk fd #%d"), filespec,
			   st.st_size, filefd);
		do {
		    amf::Buffer *buf = new amf::Buffer;
		    ret = read(filefd, buf->reference(), buf->size());
		    if (ret == 0) { // the file is done
			delete buf;
			break;
		    }
		    if (ret != buf->size()) {
			buf->resize(ret);
//			log_debug("Got last data block from disk file, size %d", buf->size());
		    }
//		    log_debug("Read %d bytes from %s.", ret, filespec);
#if 1
		    hand->pushout(buf);
		    hand->notifyout();
#else
		    // Don't bother with the outgoing que
		    if (ret > 0) {
			ret = hand->writeNet(buf);
		    }
		    delete buf;
#endif
		} while(ret > 0);
		log_debug("Done transferring %s to net fd #%d",
			  filespec, args->netfd);
		::close(filefd); // close the disk file
		// See if this is a persistant connection
// 		if (!www.keepAlive()) {
// 		    log_debug("Keep-Alive is off", www.keepAlive());
// // 		    hand->closeConnection();
//  		}
#ifdef USE_STATISTICS
		struct timespec end;
		clock_gettime (CLOCK_REALTIME, &end);
		log_debug("Read %d bytes from \"%s\" in %f seconds",
			  st.st_size, filespec,
			  (float)((end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec)/1e9)));
#endif
	    }

// 	    memset(args->filespec, 0, 256);
// 	    memcpy(->filespec, filespec.c_str(), filespec.size());
// 	    boost::thread sendthr(boost::bind(&stream_thread, args));
// 	    sendthr.join();
	}
#ifdef USE_STATISTICS
	struct timespec end;
	clock_gettime (CLOCK_REALTIME, &end);
	log_debug("Processing time for GET request was %f seconds",
		  (float)((end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec)/1e9)));
#endif
//	conndata->statistics->dump();
//    }
    } // end of while retries
    
    log_debug("httphandler all done now finally...");
    
} // end of httphandler
    
} // end of extern C

} // end of gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
