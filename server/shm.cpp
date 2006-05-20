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

// Linking Gnash statically or dynamically with other modules is making a
// combined work based on Gnash. Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Gnash give you
// permission to combine Gnash with free software programs or libraries
// that are released under the GNU LGPL and with code included in any
// release of Talkback distributed by the Mozilla Foundation. You may
// copy and distribute such a system following the terms of the GNU GPL
// for all but the LGPL-covered parts and Talkback, and following the
// LGPL for the LGPL-covered parts.
//
// Note that people who make modified versions of Gnash are not obligated
// to grant this special exception for their modified versions; it is their
// choice whether to do so. The GNU General Public License gives permission
// to release a modified version without this exception; this exception
// also makes it possible to release a modified version which carries
// forward this exception.
// 
//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#ifndef HAVE_WINSOCK_H
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#else
#include <windows.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#endif
#include <fcntl.h>
#include <string>
#include <vector>
#include <errno.h>

#include "log.h"
#include "shm.h"
#include "fn_call.h"

using namespace std;

namespace gnash {

const int DEFAULT_SHM_SIZE = 10240;
const int MAX_FILESPEC_SIZE = 20;

#ifdef darwin
# ifndef MAP_INHERIT
# define MAP_INHERIT 0
#endif
#ifndef PROT_EXEC
# define PROT_EXEC
# endif
#endif

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 8
#endif

#define FLAT_ADDR_SPACE 1

  Shm::Shm() :_addr(0), _alloced(0), _size(0), _shmkey(0), _shmfd(0)
{
}

Shm::~Shm()
{
}
    
/// \brief Initialize the shared memory segment
///
/// This creates or attaches to an existing shared memory segment.
bool
Shm::attach(char const *filespec, bool nuke)
{
    bool exists = false;
    long addr;
    // #ifdef FLAT_ADDR_SPACE
    //     off_t off;
    // #endif
    Shm *sc;
    string absfilespec;

    _size = DEFAULT_SHM_SIZE;

#ifdef darwin
    absfilespec = "/tmp";
#else
    absfilespec = "/";
#endif
    absfilespec += filespec;
    _filespec = absfilespec;
    filespec = absfilespec.c_str();
    
    
    //     log_msg("%s: Initializing %d bytes of memory for \"%s\"\n",
    //             __PRETTY_FUNCTION__, DEFAULT_SHM_SIZE, absfilespec.c_str());

    // Adjust the allocated amount of memory to be on a page boundary.
    // We can only do this on POSIX systems, so for braindead win32,
    // don't readjust the size.
#ifdef HAVE_SYSCONF
    long pageSize = sysconf(_SC_PAGESIZE);
    if (_size % pageSize) {
	_size += pageSize - _size % pageSize;
    }
#endif
    
    errno = 0;
#ifdef HAVE_SHM_OPEN
    // Create the shared memory segment
    _shmfd = shm_open(filespec, O_RDWR|O_CREAT|O_EXCL|O_TRUNC,
		      S_IRUSR|S_IWUSR);
#else
# ifdef HAVE_SHMGET
    const int shmflg = 0660 | IPC_CREAT | IPC_EXCL;
    shmid_ds shmInfo;
    _shmkey = 1234567;		// FIXME:
    filespec = "1234567";
    _shmfd = shmget(_shmkey, _size, shmflg);
    if (_shmfd < 0 && errno == EEXIST)
# else
	_shmhandle = CreateFileMapping ((HANDLE) 0xFFFFFFFF, NULL,
					PAGE_READWRITE, 0,
					_size, filespec);
    if (_shmhandle <= 0)
# endif
#endif
	{
    // If it already exists, then just attach to it.
	exists = true;
	log_msg("Shared Memory segment \"%s\" already exists\n",
		filespec);
#ifdef HAVE_SHM_OPEN
	_shmfd = shm_open(filespec, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
#else
# ifdef HAVE_SHMGET
	// Get the shared memory id for this segment
	_shmfd = shmget(_shmkey, _size, 0);
# else
	_shmhandle = CreateFileMapping ((HANDLE) 0xFFFFFFFF, NULL,
					PAGE_READWRITE, 0,
					_size, filespec);
# endif
#endif
    }
    
    // MacOSX returns this when you use O_EXCL for shm_open() instead
    // of EEXIST
#if defined(HAVE_SHMGET) ||  defined(HAVE_SHM_OPEN)
    if (_shmfd < 0 && errno == EINVAL)
#else
    if (_shmhandle <= 0 && errno == EINVAL)
#endif
	{
	exists = true;
	log_msg(
#ifdef HAVE_SHM_OPEN
	    "WARNING: shm_open() failed, retrying: %s\n",
#else
# ifdef HAVE_SHMGET
	    "WARNING: shmget() failed, retrying: %s\n",
# else
	    "WARNING: CreateFileMapping() failed, retrying: %s\n",
# endif
#endif
	    strerror(errno));
//        fd = shm_open(filespec, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	return false;
    }
    
    // We got the file descriptor, now map it into our process.
#if defined(HAVE_SHMGET) ||  defined(HAVE_SHM_OPEN)
    if (_shmfd >= 0)
#else
    if (_shmhandle >= 0)
#endif
    {
#ifdef HAVE_SHM_OPEN
	if (!exists) {
	    // Set the size so we can write to new segment
	    ftruncate(_shmfd, _size);
	}
	_addr = static_cast<char *>(mmap(0, _size,
				 PROT_READ|PROT_WRITE|PROT_EXEC,
				 MAP_SHARED|MAP_INHERIT|MAP_HASSEMAPHORE,
				 _shmfd, 0));
	if (_addr == MAP_FAILED) {
	    log_msg("WARNING: mmap() failed: %s\n", strerror(errno));
	    return false;
	}
#else  // else of HAVE_SHM_OPEN
# ifdef HAVE_SHMAT
	_addr = (char *)shmat(_shmfd, 0, 0);
	if (_addr <= 0) {
	    log_msg("WARNING: shmat() failed: %s\n", strerror(errno));
	    return false;
	}
# else
	_addr = (char *)MapViewOfFile (_shmhandle, FILE_MAP_ALL_ACCESS,
				       0, 0, _size);
# endif
#endif
        if (exists && !nuke) {
	    // If there is an existing memory segment that we don't
	    // want to trash, we just want to attach to it. We know
	    // that a ShmControl data class has been instantiated in
	    // the base of memory, and the first field is the address
	    // used for the previous mmap(), so we grab that value,
	    // unmap the old address, and map the original address
	    // into this process. This is done so that memory
	    // allocations between processes all have the same
	    // addresses. Otherwise, one can't do globally shared data
	    // among processes that requires any dynamic memory
	    // allocation. All of this is so our custom memory
	    // allocator for STL containers will work.
	    addr = *(reinterpret_cast<long *>(_addr));
	    if (addr == 0) {
		log_msg("WARNING: No address found in memory segment!\n");
		nuke = true;
	    } else {
#ifdef FLAT_ADDR_SPACE
	    log_msg("Adjusting address to 0x%lx\n", addr);
#ifdef HAVE_SHM_OPEN
	    munmap(_addr, _size);
	    log_msg("Unmapped address %p\n", _addr);
	    _addr = static_cast<char *>(mmap(reinterpret_cast<char *>(addr),
					     _size, PROT_READ|PROT_WRITE,
					     MAP_SHARED|MAP_FIXED|MAP_INHERIT|MAP_HASSEMAPHORE,
					     _shmfd, static_cast<off_t>(0)));
	    //                 off = (off_t)((long)addr - (long)_addr);
	    _addr = static_cast<char *>(mmap((char *)addr,
					     _size, PROT_READ|PROT_WRITE|PROT_EXEC,
					     MAP_FIXED|MAP_SHARED, _shmfd, 0));
	    
	    if (_addr == MAP_FAILED) {
		log_msg("WARNING: MMAP failed: %s\n", strerror(errno));
		return static_cast<Shm *>(0);
	    }
        }
#else
#ifdef HAVE_SHMAT	    
	shmdt(_addr);
	_addr = (char *)shmat(_shmfd, (void *)addr, 0);
#else
	CloseHandle(_shmhandle);	
	_addr = (char *)MapViewOfFile (_shmhandle, FILE_MAP_ALL_ACCESS,
			       0, 0, _size);
#endif // end of HAVE_SHMAT
	}
#endif // end of HAVE_SHM_OPEN
#endif // end of FLAT_ADDR_SPACE
    
	log_msg("Opened Shared Memory segment \"%s\": %zd bytes at %p.\n",
		filespec, _size, _addr);
	}
    
	if (nuke) {
	    //            log_msg("Zeroing %d bytes at %p.\n", _size, _addr);
	    // Nuke all the segment, so we don't have any problems
	    // with leftover data.
	    memset(_addr, 0, _size);
	    sc = cloneSelf();
	} else {
	    sc = reinterpret_cast<Shm *>(_addr);
	}
    } else {
	log_msg("ERROR: Couldn't open the Shared Memory segment \"%s\"! %s\n",
		filespec, strerror(errno));
	return false;
    }
    
#ifdef HAVE_SHM_OPEN
// don't close it on an error
    if (_shmfd) {
	::close(_shmfd);
    }
#endif
      
    return true; 
}

Shm *
Shm::cloneSelf(void)
{

    if (_addr > 0) {
//         log_msg("Cloning ShmControl, %d bytes to %p\n",
//                 sizeof(Shm), _addr);
        // set the allocated bytes before we copy so the value is
        // correct in both instantiations of this object.
        _alloced = sizeof(Shm);
        memcpy(_addr, this, sizeof(Shm));
        return reinterpret_cast<Shm *>(_addr);
    }
    
    log_msg("WARNING: Can't clone Self, address 0x0\n");

    return static_cast<Shm *>(0);
}

// Resize the allocated memory segment
bool
Shm::resize()
{
    // Increase the size by 10 %
    return resize(DEFAULT_SHM_SIZE + (DEFAULT_SHM_SIZE/10)); 
}

bool
Shm::resize(int bytes)
    
{
# ifdef HAVE_MREMAP
    _addr = mremap(_shmAddr, _shmSize, _shmSize + bytes, MREMAP_MAYMOVE);
    if (_addr != 0) {
        return true;
    }
#else
    // FIXME: alloc a whole new segment, and copy this one
    // into it. Yeuch...
#endif
    return false;
}

// Allocate a memory from the shared memory segment
void *
Shm::brk(int bytes)
{
    int wordsize = sizeof(long);
    
    // Adjust the allocated amount of memory to be on a word boundary.
    if (bytes % wordsize) {
        int wordsize = sizeof(long);
        
        // Adjust the allocated amount of memory to be on a word boundary.
        if (bytes % wordsize) {
            bytes += wordsize - bytes % wordsize;
        }
        
        void *addr = (static_cast<char *>(_addr)) + _alloced;
        
        log_msg("%s: Allocating %d bytes at %p\n",
                __PRETTY_FUNCTION__, bytes, addr);
        // Zero out the block before returning it
        memset(addr, 0, bytes);
        
        // Increment the counter
        _alloced += bytes;
        
        // Return a pointer to the beginning of the block
        return addr;
        bytes += wordsize - bytes % wordsize;
    }
    
    void *addr = (static_cast<char *>(_addr)) + _alloced;
    
    log_msg("%s: Allocating %d bytes at %p\n",
            __PRETTY_FUNCTION__, bytes, addr);

    // Zero out the block before returning it
    memset(addr, 0, bytes);
    
    
    // Increment the counter
    _alloced += bytes;
    
    // Return a pointer to the beginning of the block
    return addr; 
}

// Close the memory segment. This removes it from the system.
bool
Shm::closeMem()
{
    // Only nuke the shared memory segement if we're the last one.
#ifdef HAVE_SHM_OPEN
    if (_filespec.size() != 0) {
        shm_unlink(_filespec.c_str());
    }
    
     // flush the shared memory to disk
     if (_addr > 0) {
         // detach memory
         munmap(_addr, _size);
     }
#else
#ifdef HAVE_SHMGET
     shmctl(_shmfd, IPC_RMID, 0);
#else
     CloseHandle(_shmhandle);
#endif
#endif
    
    _addr = 0;
    _alloced = 0;

    return true;    
}

#ifdef ENABLE_TESTING
bool
Shm::exists()
{
    struct stat           stats;
    struct dirent         *entry;
    vector<const char *>  dirlist;
    string                realname;
    DIR                   *library_dir = NULL;

    // Solaris stores shared memory segments in /var/tmp/.SHMD and
    // /tmp/.SHMD. Linux stores them in /dev/shm.
    dirlist.push_back("/dev/shm");
    dirlist.push_back("/var/tmp/.SHMD");
    dirlist.push_back("/tmp/.SHMD");

    // Open the directory where the raw POSIX shared memory files are
    for (unsigned int i=0; i<dirlist.size(); i++)
    {
        library_dir = opendir (dirlist[i]);
        if (library_dir != NULL) {
            realname = dirlist[i];
            
            // By convention, the first two entries in each directory
            // are for . and .. (``dot'' and ``dot dot''), so we
            // ignore those. The next directory read will get a real
            // file, if any exists.
            entry = readdir(library_dir);
            entry = readdir(library_dir);
            break;
        }
    }
    
    realname += _filespec;
    
    if (stat(realname.c_str(), &stats) == 0) {
        return true;
    }
    return false;
}

// These are the callbacks used to define custom methods for our AS
// classes. This way we can examine the private data after calling a
// method to see if it worked correctly.
void shm_getname(const fn_call& fn)
{
    shm_as_object *ptr = (shm_as_object*)fn.this_ptr;
    assert(ptr);
    fn.result->set_tu_string(ptr->obj.getName().c_str());
}
void shm_getsize(const fn_call& fn)
{
    shm_as_object *ptr = (shm_as_object*)fn.this_ptr;
    assert(ptr);
    fn.result->set_int(ptr->obj.getSize());
}
void shm_getallocated(const fn_call& fn)
{
    shm_as_object *ptr = (shm_as_object*)fn.this_ptr;
    assert(ptr);
    fn.result->set_int(ptr->obj.getAllocated());
}
void shm_exists(const fn_call& fn)
{
    shm_as_object *ptr = (shm_as_object*)fn.this_ptr;
    assert(ptr);
    fn.result->set_bool(ptr->obj.exists());
}
#endif

} // end of gnash namespace

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
