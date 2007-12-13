 // 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_DEJAGNU_H

#include <string>

#include <unistd.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#ifndef __GNUC__
extern int optind, getopt(int, char *const *, const char *);
#endif

#include <sys/types.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex.h>

#include "log.h"
#include "http.h"
#include "dejagnu.h"

using namespace cygnal;
using namespace gnash;
using namespace std;

static void usage (void);

static TestState runtest;

LogFile& dbglogfile = LogFile::getDefaultInstance();

int
main(int argc, char *argv[])
{
    int c;
    
    while ((c = getopt (argc, argv, "hdvsm:")) != -1) {
        switch (c) {
          case 'h':
              usage ();
              break;
              
          case 'v':
              dbglogfile.setVerbosity();
              break;
              
          default:
              usage ();
              break;
        }
    }

    HTTP http;

    http.clearHeader();
    http.formatDate();
//    cerr << "FIXME: " << http.getHeader() << endl;

    regex_t regex_pat;

    // Check the Date field
    // The date should look something like this:
    //     Date: Mon, 10 Dec 2007  GMT
    regcomp (&regex_pat, "Date: [A-Z][a-z]*, [0-9]* [A-Z][a-z]* [0-9]* [0-9:]* *GMT$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatDate()");
        cerr << http.getHeader().c_str() << endl;
    } else {
        runtest.pass ("HTTP::formatDate()");
    }
    regfree(&regex_pat);

    // Check the Content-Length field
    http.clearHeader();
    http.formatContentLength(12345);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Content-Length: [0-9]*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatContentLength()");
    } else {
        runtest.pass ("HTTP::formatContentLength()");
    }
    regfree(&regex_pat);


    // Check the Connection field
//     bool formatConnection(const char *data);
    http.clearHeader();
    const char *data = "Keep-Alive";
    http.formatConnection(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Connection: [A-za-z-]*",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatConnection()");
    } else {
        runtest.pass ("HTTP::formatConnection()");
    }
    regfree(&regex_pat);

    // Check the Server field
    http.clearHeader();
    http.formatServer();
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Server: Cygnal (GNU/Linux)$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatServer()");
    } else {
        runtest.pass ("HTTP::formatServer()");
    }
    regfree(&regex_pat);

    // Check the Host field
//     bool formatHost(const char *data);
    http.clearHeader();
    data = "localhost:4080";
    http.formatHost(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Host: [A-za-z-]*:[0-9]*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatHost()");
    } else {
        runtest.pass ("HTTP::formatHost()");
    }
    regfree(&regex_pat);

// Check the Language field
//     bool formatLanguage(const char *data);
    http.clearHeader();
    data = "en-US,en;q=0.9";
    http.formatLanguage(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Accept-Language: en-US,en;q=0.9$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatLanguage()");
    } else {
        runtest.pass ("HTTP::formatLanguage()");
    }
    regfree(&regex_pat);

//     bool formatCharset(const char *data);
// Accept-Charset: iso-8859-1, utf-8, utf-16, *;q=0.1\r
    http.clearHeader();
    data = "iso-8859-1, utf-8, utf-16, *;q=0.1";
    http.formatCharset(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Accept-Charset: iso-8859-1.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatCharset()");
    } else {
        runtest.pass ("HTTP::formatCharset()");
    }
    regfree(&regex_pat);
        
//     bool formatEncoding(const char *data);
    http.clearHeader();
    data = "deflate, gzip, x-gzip, identity, *;q=0";
    http.formatEncoding(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Accept-Encoding: deflate, gzip.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatEncoding()");
    } else {
        runtest.pass ("HTTP::formatEncoding()");
    }
    regfree(&regex_pat);
        
        
//     bool formatTE(const char *data);
    http.clearHeader();
    data = "deflate, gzip, chunked, identity, trailers";
    http.formatTE(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "TE: deflate, gzip,.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatTE()");
    } else {
        runtest.pass ("HTTP::formatTE()");
    }
    regfree(&regex_pat);

//     bool formatAgent(const char *data);
    http.clearHeader();
    data = "Gnash 0.8.1-cvs (X11; Linux i686; U; en)";
    http.formatAgent(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "User-Agent: Gnash 0.8.1-cvs.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatAgent()");
    } else {
        runtest.pass ("HTTP::formatAgent()");
    }
    regfree(&regex_pat);

    // Check the Content Type field. First we check with a
    // specified field, then next to see if the default works.
//     bool formatContentType();
    http.clearHeader();
    http.formatContentType(HTTP::SWF);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Content-Type: application/x-shockwave-flash.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatContentType(type)");
    } else {
        runtest.pass ("HTTP::formatContentType(type)");
    }
    regfree(&regex_pat);

    http.clearHeader();
    http.formatContentType();
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Content-Type: text/html.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatContentType()");
    } else {
        runtest.pass ("HTTP::formatContenType()");
    }
    regfree(&regex_pat);

//     bool formatReferer(const char *data);
    http.clearHeader();
    data = "http://localhost/software/gnash/tests/index.html";
    http.formatReferer(data);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "Referer: http://localhost.*index.html.*$",
             REG_NOSUB|REG_NEWLINE);
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatReferer()");
    } else {
        runtest.pass ("HTTP::formatReferer()");
    }
    regfree(&regex_pat);

    // Check formatHeader()
    http.clearHeader();
    http.formatHeader(RTMP);
//    cerr << "FIXME: " << http.getHeader() << endl;
    regcomp (&regex_pat, "HTTP/1.1 200 OK.*Date:.*Connection:.*-Length.*-Type:.*$",
             REG_NOSUB);        // note that we do want to look for NL
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatHeader(port)");
    } else {
        runtest.pass ("HTTP::formatheader(port)");
    }
    regfree(&regex_pat);

    // Check the Server field
    http.clearHeader();
    http.formatErrorResponse(HTTP::NOT_FOUND);
//    cerr << "FIXME: " << http.getHeader() << endl;
//    cerr << "FIXME: " << http.getBody() << endl;
    regcomp (&regex_pat, "Date:.*Server:.*Content-Length:.*Connection:.*Content-Type:.*$",
             REG_NOSUB);        // note that we do want to look for NL
    if (regexec (&regex_pat, http.getHeader().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatErrorResponse(header)");
    } else {
        runtest.pass ("HTTP::formatErrorResponse(header)");
    }
    regfree(&regex_pat);
    regcomp (&regex_pat, "DOCTYPE.*<title>404 Not Found</title>.*$",
             REG_NOSUB);        // note that we do want to look for NL
    if (regexec (&regex_pat, http.getBody().c_str(), 0, (regmatch_t *)0, 0)) {
        runtest.fail ("HTTP::formatErrorResponse(body)");
    } else {
        runtest.pass ("HTTP::formatErrorResponse(body)");
    }
    regfree(&regex_pat);
    
    //
    // Decoding tests for HTTP
    //
    http.clearHeader();
    const char *buffer = "GET /software/gnash/tests/flvplayer.swf?file=http://localhost/software/gnash/tests/Ouray_Ice_Festival_Climbing_Competition.flv HTTP/1.1\r\n"
"User-Agent: Gnash/0.8.1-cvs (X11; Linux i686; U; en)\r\n"
"Host: localhost:4080\r\n"
"Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1\r\n"
"Accept-Language: en-US,en;q=0.9\r\n"
"Accept-Charset: iso-8859-1, utf-8, utf-16, *;q=0.1\r\n"
"Accept-Encoding: deflate, gzip, x-gzip, identity, *;q=0\r\n"
"If-Modified-Since: Mon, 10 Dec 2007 02:26:31 GMT\r\n"
"If-None-Match: \"4cc434-e266-52ff63c0\"\r\n"
"Connection: Keep-Alive, TE\r\n"
"Referer: http://localhost/software/gnash/tests/index.html\r\n"
"TE: deflate, gzip, chunked, identity, trailers\r\n"
"\r\n";

// GET /software/gnash/tests/ HTTP/1.1
// Host: localhost:4080
// User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.5) Gecko/20070718 Fedora/2.0.0.5-1.fc7 Firefox/2.0.0.5
// Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
// Accept-Language: en-us,en;q=0.5
// Accept-Encoding: gzip,deflate
// Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7
// Keep-Alive: 300
// Connection: keep-alive

// User Agent: Lynx/2.8.6rel.2 libwww-FM/2.14 SSL-MM/1.4.1 OpenSSL/0.9.8b
    
// Some browsers have a different synatax, of course, to keep things
// interesting.
    const char *buffer2 = "GET /software/gnash/tests/flvplayer.swf?file=http://localhost/software/gnash/tests/Ouray_Ice_Festival_Climbing_Competition.flv HTTP/1.1\r\n"
"Content-Language: en-US,en;q=0.9\r\n"
"Content-Charset: iso-8859-1, utf-8, utf-16, *;q=0.1\r\n"
"Content-Encoding: deflate, gzip, x-gzip, identity, *;q=0\r\n";
//    http.extractMethod(buffer);
    string result;
    result = http.extractReferer(buffer);
    if (result == "http://localhost/software/gnash/tests/index.html") {
        runtest.fail ("HTTP::extractReferer()");
    } else {
        runtest.pass ("HTTP::extractReferer()");
    }
    result = http.extractHost(buffer);
    if (result == "localhost:4080") {
        runtest.fail ("HTTP::extractHost()");
    } else {
        runtest.pass ("HTTP::extractHost()");
    }

    result = http.extractAgent(buffer);
    if (result == "Gnash/0.8.1-cvs (X11; Linux i686; U; en)") {
        runtest.fail ("HTTP::extractAgent()");
    } else {
        runtest.pass ("HTTP::extractAgent()");
    }

    int count;
    count = http.extractLanguage(buffer);
    std::vector<std::string> language = http.getLanguage();
    if ((count > 2) &&
        (language[0] == "en-US") &&
        (language[1] == "en")) {
        runtest.fail ("HTTP::extractLanguage(Accept-)");
    } else {
        runtest.pass ("HTTP::extractLanguage(Accept-)");
    }
    count = http.extractLanguage(buffer2);
    language = http.getLanguage();
    if ((count == 2) &&
        (language[0] == "en-US") &&
        (language[1] == "en")) {
        runtest.fail ("HTTP::extractLanguage(Content-)");
    } else {
        runtest.pass ("HTTP::extractLanguage(Content-)");
    }

    result = http.extractCharset(buffer);
    std::vector<std::string> charsets = http.getCharset();
    if ((count == 3) &&
        (charsets[0] == "iso-8859-1") &&
        (charsets[1] == "utf-8") &&
        (charsets[2] == "utf-16")) {
        runtest.fail ("HTTP::extractCharset(Accept-)");
    } else {
        runtest.pass ("HTTP::extractCharset(Accept-)");
    }
    count = http.extractCharset(buffer2);
    charsets = http.getCharset();
    if ((count == 3) &&
        (charsets[0] == "iso-8859-1") &&
        (charsets[1] == "utf-8") &&
        (charsets[2] == "utf-16")) {
        runtest.fail ("HTTP::extractCharset(Content-)");
    } else {
        runtest.pass ("HTTP::extractCharset(Content-)");
    }

    count = http.extractConnection(buffer);
    std::vector<std::string> connections = http.getConnection();
    if ((count == 2) &&
        (connections[0] == "Keep-Alive") &&
        (connections[1] == "TE")) {
        runtest.pass ("HTTP::extractConnection()");
    } else {
        runtest.fail ("HTTP::extractConnection()");
    }

    count = http.extractEncoding(buffer);
    std::vector<std::string> encoding = http.getEncoding();
    if ((count == 4) &&
        (encoding[0] == "deflate") &&
        (encoding[1] == "gzip") &&
        (encoding[2] == "chunked") &&
        (encoding[3] == "identity")) {
        runtest.fail ("HTTP::extractEncoding(Accept-)");
    } else{
        runtest.pass ("HTTP::extractEncoding(Accept-)");
    }

    count = http.extractTE(buffer);
    std::vector<std::string> te = http.getTE();
    if ((count == 5) &&
        (te[0] == "deflate") &&
        (te[1] == "gzip") &&
        (te[2] == "chunked") &&
        (te[3] == "identity") &&
        (te[4] == "trailers")) {
        runtest.pass ("HTTP::extractTE()");
    } else {
        runtest.fail ("HTTP::extractTE()");
    }

//     http.formatHeader(666, RTMP);
//     http.formatRequest("http://localhost:4080", HTTP::GET);
    
//     bool formatMethod(const char *data);
        
        
//     void *out = amf_obj.encodeNumber(*num);

//     if (memcmp(out, buf, 9) == 0) {
//         runtest.pass("Encoded AMF Number");
//     } else {
//         runtest.fail("Encoded AMF Number");
//     }

//    delete num;

    
    if (dbglogfile.getVerbosity() > 0) {
        http.dump();
    }
}
static void
usage (void)
{
    cerr << "This program tests HTTP protocol support." << endl;
    cerr << "Usage: test_http [hv]" << endl;
    cerr << "-h\tHelp" << endl;
    cerr << "-v\tVerbose" << endl;
    exit (-1);
}

#else  // no DejaGnu support

int
main(int /*argc*/, char /* *argv[]*/)
{
  // nop
  return 0;  
}

#endif
