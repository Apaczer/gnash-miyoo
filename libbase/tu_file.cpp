// tu_file.cpp	-- Ignacio Casta�o, Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A file class that can be customized with callbacks.

#include "tu_file.h"
#include "log.h"

#include <boost/format.hpp>
#include <cerrno>
#include <cstdio>
#include "GnashFileUtilities.h"

//
// tu_file functions using FILE
//

namespace gnash {


std::streamsize
tu_file::read(void* dst, std::streamsize bytes) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
//    GNASH_REPORT_FUNCTION;
    
    assert(dst);
    return fread( dst, 1, bytes, static_cast<FILE*>(m_data) );
}

std::streamsize
tu_file::write(const void* src, std::streamsize bytes)
// Return the number of bytes actually written.
{
    assert(src);
    return std::fwrite(src, 1, bytes, static_cast<FILE*>(m_data));
}

bool
tu_file::seek(std::streampos pos)
{

    // TODO: optimize this by caching total stream size ?
    if (static_cast<size_t>(pos) > size()) return false;

    FILE* file = static_cast<FILE*>(m_data);

    std::clearerr(file); // make sure EOF flag is cleared.
    int	result = std::fseek(file, pos, SEEK_SET);
    if (result == EOF) {
        // @@ TODO should set m_error to something relevant based on errno.
        return false;
    }

    assert (std::ftell(file) == pos);

    return true;
}

void
tu_file::go_to_end()
{
    int err = std::fseek(static_cast<FILE*>(m_data), 0, SEEK_END);
    if (-1 == err ) {
        boost::format fmt = boost::format(
                    _("Error while seeking to end: %1%")
                ) % strerror(errno);
        throw IOException(fmt.str());
    }
}

std::streampos
tu_file::tell() const
{
    FILE* f = static_cast<FILE*>(m_data);

    std::streampos ret = std::ftell(f);
    if (ret < 0) throw IOException("Error getting stream position");

    assert(static_cast<size_t>(ret) <= size());
    return ret;
}

bool
tu_file::eof() const
{
    return std::feof(static_cast<FILE*>(m_data));
}

bool
tu_file::bad() const
{
    if (!m_data) return true;
    return std::ferror(static_cast<FILE*>(m_data));
}

size_t
tu_file::size() const
{
    assert(m_data);

    FILE* f = static_cast<FILE*>(m_data);

    struct stat statbuf;
    if (fstat(fileno(f), &statbuf) < 0)
    {
	    log_error("Could not fstat file");
	    return static_cast<size_t>(-1);
    }
    return statbuf.st_size;
}


void
tu_file::close()
// Return 0 on success, or TU_FILE_CLOSE_ERROR on failure.
{
    assert(m_data);
    int	result = std::fclose(static_cast<FILE*>(m_data));
    if (result == EOF) {
	    // @@ TODO should set m_error to something relevant based on errno.
    }
}


//// Create a file from a standard file pointer.
tu_file::tu_file(FILE* fp, bool autoclose=false) :
    m_data(fp),
    _autoclose(autoclose)
{
    //GNASH_REPORT_FUNCTION;
}


tu_file::~tu_file()
// Close this file when destroyed.
{
    if (_autoclose) close();
}


} // end namespace gnash

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
