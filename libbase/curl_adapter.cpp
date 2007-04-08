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

/* $Id: curl_adapter.cpp,v 1.25 2007/04/08 23:06:17 rsavoye Exp $ */

#if defined(_WIN32) || defined(WIN32)
#define snprintf _snprintf
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "curl_adapter.h"
#include "tu_file.h"
#include "utility.h"
#include "GnashException.h"
#include "log.h"

#include <map>
#include <iostream>
#include <string>

using namespace std;

//#define GNASH_CURL_VERBOSE 1

// define this if you want seeks back to be reported (on stderr)
//#define GNASH_CURL_WARN_SEEKSBACK 1

#ifndef USE_CURL

// Stubs, in case client doesn't want to link to zlib.
namespace curl_adapter
{
tu_file* make_stream(const char * /*url */)
	{
		fprintf(stderr, "libcurl is not available, but curl_adapter has been attempted to use\n");
		return NULL; // should assert(0) instead ?
	}
}


#else // def USE_CURL


#include <stdexcept>
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(_WIN32) && !defined(WIN32)
# include <unistd.h>
#endif

#include <curl/curl.h>

namespace curl_adapter
{


/***********************************************************************
 *
 *  CurlStreamFile definition
 * 
 **********************************************************************/

class CurlStreamFile
{

public:

	typedef std::map<std::string, std::string> PostData;

	/// Open a stream from the specified URL
	CurlStreamFile(const std::string& url);

	/// Open a stream from the specified URL posting the specified variables
	//
	/// @param url
	///	The url to post to.
	///
	/// @param vars
	///	The url-encoded post data.
	///
	CurlStreamFile(const std::string& url, const std::string& vars);

	~CurlStreamFile();

	/// Read 'bytes' bytes into the given buffer.
	//
	/// Return number of actually read bytes
	///
	size_t read(void *dst, size_t bytes);

	/// Return true if EOF has been reached
	bool eof();

	/// Report global position within the file
	size_t tell();

	/// Put read pointer at given position
	bool seek(size_t pos);

	/// Put read pointer at eof
	bool seek_to_end();

	/// Returns the size of the stream
	long get_stream_size();

private:

	void init(const std::string& url);

	// Use this file to cache data
	FILE* _cache;

	// _cache file descriptor
	int _cachefd;

	// we keep a copy here to be sure the char*
	// is alive for the whole CurlStreamFile lifetime
	// TODO: don't really do this :)
	std::string _url;

	// the libcurl easy handle
	CURL *_handle;

	// the libcurl multi handle
	CURLM *_mhandle;

	// transfer in progress
	int _running;

	// Post data. Empty if no POST has been requested
	std::string _postdata;

	// Current size of cached data
	long _cached;

	// Attempt at filling the cache up to the given size.
	// Will call libcurl routines to fetch data.
	void fill_cache(off_t size);

	// Append sz bytes to the cache
	size_t cache(void *from, size_t sz);

	void printInfo();

	// Callback for libcurl, will be called
	// by fill_cache() and will call cache() 
	static size_t recv(void *buf, size_t  size, 
		size_t  nmemb, void *userp);


};

/***********************************************************************
 *
 *  CurlStreamFile implementation
 * 
 **********************************************************************/

// Ensure libcurl is initialized
static void ensure_libcurl_initialized()
{
	static bool initialized=0;
	if ( ! initialized ) {
		// TODO: handle an error here
		curl_global_init(CURL_GLOBAL_ALL);
		initialized=1;
	}
}

/*static private*/
size_t
CurlStreamFile::recv(void *buf, size_t  size,  size_t  nmemb, 
	void *userp)
{
#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "curl write callback called for (%d) bytes\n",
		size*nmemb);
#endif
	CurlStreamFile* stream = (CurlStreamFile*)userp;
	return stream->cache(buf, size*nmemb);
}

	
/*private*/
size_t
CurlStreamFile::cache(void *from, size_t sz)
{
	// take note of current position
	long curr_pos = ftell(_cache);

	// seek to the end
	fseek(_cache, 0, SEEK_END);

	size_t wrote = fwrite(from, 1, sz, _cache);
	if ( wrote < 1 )
	{
		char errmsg[256];
	
		snprintf(errmsg, 255,
			"writing to cache file: requested " SIZET_FMT ", wrote " SIZET_FMT " (%s)",
			sz, wrote, strerror(errno));
		fprintf(stderr, "%s\n", errmsg);
		throw gnash::GnashException(errmsg);
	}

	// Set the size of cached data
	_cached = ftell(_cache);

	// reset position for next read
	fseek(_cache, curr_pos, SEEK_SET);

	return wrote;
}


/*private*/
void
CurlStreamFile::fill_cache(off_t size)
{
#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "fill_cache(%d) called\n", size);
#endif

	struct stat statbuf;

	CURLMcode mcode;
	while (_running)
	{
		do
		{
			mcode=curl_multi_perform(_mhandle, &_running);
		} while ( mcode == CURLM_CALL_MULTI_PERFORM );

		if ( mcode != CURLM_OK )
		{
			throw gnash::GnashException(curl_multi_strerror(mcode));
		}

		// we already have that much data
		fstat(_cachefd, &statbuf);
		if ( statbuf.st_size >= size ) 
		{
#ifdef GNASH_CURL_VERBOSE
			fprintf(stderr,
				" big enough (%d), returning\n",
				statbuf.st_size);
#endif
			return;
		}


	}

}

/*private*/
void
CurlStreamFile::printInfo()
{
	fprintf(stderr, "_cache.tell = " SIZET_FMT "\n", tell());
}

/*private*/
void
CurlStreamFile::init(const std::string& url)
{
	ensure_libcurl_initialized();

	_url = url;
	_running = 1;

	_cached = -1;

	_handle = curl_easy_init();
	_mhandle = curl_multi_init();

	/// later on we might want to accept a filename
	/// in the constructor
	_cache = tmpfile();
	if ( ! _cache ) {
		throw gnash::GnashException("Could not create temporary cache file");
	}
	_cachefd = fileno(_cache);

	CURLcode ccode;

	ccode = curl_easy_setopt(_handle, CURLOPT_USERAGENT, "Gnash-" VERSION);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

#ifdef GNASH_CURL_VERBOSE
	// for verbose operations
	ccode = curl_easy_setopt(_handle, CURLOPT_VERBOSE, 1);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}
#endif

/* from libcurl-tutorial(3)
When using multiple threads you should set the CURLOPT_NOSIGNAL  option
to TRUE for all handles. Everything will work fine except that timeouts
are not honored during the DNS lookup - which you can  work  around  by
*/
	ccode = curl_easy_setopt(_handle, CURLOPT_NOSIGNAL, true);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	// set url
	ccode = curl_easy_setopt(_handle, CURLOPT_URL, _url.c_str());
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	//curl_easy_setopt(_handle, CURLOPT_NOPROGRESS, false);


	// set write data and function
	ccode = curl_easy_setopt(_handle, CURLOPT_WRITEDATA, this);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	ccode = curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION,
		CurlStreamFile::recv);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	ccode = curl_easy_setopt(_handle, CURLOPT_FOLLOWLOCATION, 1);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	//fill_cache(32); // pre-cache 32 bytes
	//curl_multi_perform(_mhandle, &_running);
}

/*public*/
CurlStreamFile::CurlStreamFile(const std::string& url)
{
	init(url);

	// CURLMcode ret = 
	CURLMcode mcode = curl_multi_add_handle(_mhandle, _handle);
	if ( mcode != CURLM_OK ) {
		throw gnash::GnashException(curl_multi_strerror(mcode));
	}
}

/*public*/
CurlStreamFile::CurlStreamFile(const std::string& url, const std::string& vars)
{
	init(url);

	_postdata = vars;

	CURLcode ccode;

	ccode = curl_easy_setopt(_handle, CURLOPT_POST, 1);
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	// libcurl needs to access the POSTFIELDS during 'perform' operations,
	// so we must use a string whose lifetime is ensured to be longer then
	// the multihandle itself.
	// The _postdata member should meet this requirement
	ccode = curl_easy_setopt(_handle, CURLOPT_POSTFIELDS, _postdata.c_str());
	if ( ccode != CURLE_OK ) {
		throw gnash::GnashException(curl_easy_strerror(ccode));
	}

	CURLMcode mcode = curl_multi_add_handle(_mhandle, _handle);
	if ( mcode != CURLM_OK ) {
		throw gnash::GnashException(curl_multi_strerror(mcode));
	}

}

/*public*/
CurlStreamFile::~CurlStreamFile()
{
	curl_multi_remove_handle(_mhandle, _handle);
	curl_easy_cleanup(_handle);
	curl_multi_cleanup(_mhandle);
	fclose(_cache);
}

/*public*/
size_t
CurlStreamFile::read(void *dst, size_t bytes)
{
	if ( eof() ) return 0;

#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "read(%d) called\n", bytes);
#endif

	fill_cache(tell()+bytes);

#ifdef GNASH_CURL_VERBOSE
	printInfo();
#endif

	return fread(dst, 1, bytes, _cache);

}

/*public*/
bool
CurlStreamFile::eof()
{
	bool ret = ( ! _running && feof(_cache) );
	
#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "eof() returning %d\n", ret);
#endif
	return ret;

}

/*public*/
size_t
CurlStreamFile::tell()
{
	long ret =  ftell(_cache);

#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "tell() returning %ld\n", ret);
#endif

	return ret;

}

/*public*/
bool
CurlStreamFile::seek(size_t pos)
{
#ifdef GNASH_CURL_WARN_SEEKSBACK
	if ( pos < tell() ) {
		fprintf(stderr,
			"Warning: seek backward requested (%ld from %ld)\n",
			pos, tell());
	}
#endif

	fill_cache(pos);

	if ( fseek(_cache, pos, SEEK_SET) == -1 ) {
		fprintf(stderr, "Warning: fseek failed\n");
		return false;
	} else {
		return true;
	}

}

/*public*/
bool
CurlStreamFile::seek_to_end()
{
	CURLMcode mcode;
	while (_running)
	{
		do
		{
			mcode=curl_multi_perform(_mhandle, &_running);
		} while ( mcode == CURLM_CALL_MULTI_PERFORM );
		
		if ( mcode != CURLM_OK )
		{
			throw gnash::GnashException(curl_multi_strerror(mcode));
		}
	}

	if ( fseek(_cache, 0, SEEK_END) == -1 ) {
		fprintf(stderr, "Warning: fseek to end failed\n");
		return false;
	} else {
		return true;
	}

}

/*public*/
long
CurlStreamFile::get_stream_size()
{
	double size;
	curl_easy_getinfo(_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);

	int ret = static_cast<long>(size);

#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "get_stream_size() returning %ld\n", ret);
#endif

	return ret;

}

/***********************************************************************
 *
 * Adapter calls
 * 
 **********************************************************************/


// Return number of bytes actually read.
static int
read(void* dst, int bytes, void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;
	return stream->read(dst, bytes);
}

static bool
eof(void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;
	return stream->eof();
}

static int
write(const void* /*src*/, int /*bytes*/, void* /*appdata*/)
{
	assert(0); // not supported
	return 0;
}

static int
seek(int pos, void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;
	if ( stream->seek(pos) ) return 0;
	else return TU_FILE_SEEK_ERROR;
}

static int
seek_to_end(void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;
	if ( stream->seek_to_end() ) return 0;
	else return TU_FILE_SEEK_ERROR;
}

static int
tell(void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;
	return stream->tell();
}

static long
get_stream_size(void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;
	return stream->get_stream_size();

}

static int
close(void* appdata)
{
	CurlStreamFile* stream = (CurlStreamFile*) appdata;

	delete stream;

	//return TU_FILE_CLOSE_ERROR;
	return 0;
}

//-------------------------------------------
// Exported interfaces
//-------------------------------------------

tu_file*
make_stream(const char* url)
{
	ensure_libcurl_initialized();

#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "making curl stream for %s\n", url);
#endif

	CurlStreamFile* stream = NULL;

	try {
		stream = new CurlStreamFile(url);
	} catch (const std::exception& ex) {
		fprintf(stderr, "curl stream: %s\n", ex.what());
		delete stream;
		return NULL;
	}

	return new tu_file(
		(void*)stream, // opaque user pointer
		read, // read
		write, // write
		seek, // seek
		seek_to_end, // seek_to_end
		tell, // tell
		eof, // get eof
		get_stream_size, // size of stream 
		close);
}

tu_file*
make_stream(const char* url, const std::string& postdata)
{
	ensure_libcurl_initialized();

#ifdef GNASH_CURL_VERBOSE
	fprintf(stderr, "making curl stream for %s\n", url);
#endif

	CurlStreamFile* stream = NULL;

	try {
		stream = new CurlStreamFile(url, postdata);
	} catch (const std::exception& ex) {
		fprintf(stderr, "curl stream: %s\n", ex.what());
		delete stream;
		return NULL;
	}

	return new tu_file(
		(void*)stream, // opaque user pointer
		read, // read
		write, // write
		seek, // seek
		seek_to_end, // seek_to_end
		tell, // tell
		eof, // get eof
		get_stream_size, // size of stream 
		close);
}

} // namespace curl_adapter

#endif // def USE_CURL


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
