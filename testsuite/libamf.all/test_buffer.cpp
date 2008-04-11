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

#include <regex.h>
#include <cstdio>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <boost/cstdint.hpp>

#ifdef HAVE_DEJAGNU_H
#include "dejagnu.h"
#else
#include "check.h"
#endif

#include "log.h"
#include "rc.h"
#include "network.h"
#include "gmemory.h"
#include "buffer.h"
#include "arg_parser.h"

using namespace std;
using namespace amf;
using namespace gnash;
using namespace boost;

static void usage();

// Prototypes for test cases
static void test_construct();
static void test_copy();
static void test_find();
static void test_append();
static void test_remove();
static void test_destruct();
static void test_operators();

// Enable the display of memory allocation and timing data
static bool memdebug = false;

TestState runtest;
LogFile& dbglogfile = LogFile::getDefaultInstance();
RcInitFile& rcfile = RcInitFile::getDefaultInstance();

int
main (int argc, char** argv)
{
    const Arg_parser::Option opts[] =
        {
            { 'h', "help",          Arg_parser::no  },
            { 'v', "verbose",       Arg_parser::no  },
            { 'w', "write",         Arg_parser::no  },
            { 'm', "memstats",      Arg_parser::no  },
            { 'd', "dump",          Arg_parser::no  },
        };
    
    Arg_parser parser(argc, argv, opts);
    if( ! parser.error().empty() ) {
        cout << parser.error() << endl;
        exit(EXIT_FAILURE);
    }
    
    for( int i = 0; i < parser.arguments(); ++i ) {
        const int code = parser.code(i);
        try {
            switch( code ) {
              case 'h':
                  usage ();
                  exit(EXIT_SUCCESS);
              case 'v':
                    dbglogfile.setVerbosity();
                    // This happens once per 'v' flag 
                    log_debug(_("Verbose output turned on"));
                    break;
              case 'm':
                    // This happens once per 'v' flag 
                    log_debug(_("Enabling memory statistics"));
                    memdebug = true;
                    break;
              case 'w':
                  rcfile.useWriteLog(true); // dbglogfile.setWriteDisk(true);
                  log_debug(_("Logging to disk enabled"));
                  break;
                  
	    }
        }
        
        catch (Arg_parser::ArgParserException &e) {
            cerr << _("Error parsing command line options: ") << e.what() << endl;
            cerr << _("This is a Gnash bug.") << endl;
        }
    }
    
    // We use the Memory profiling class to check the malloc buffers
    // in the kernel to make sure the allocations and frees happen
    // the way we expect them too. There is no real other way to tell.
    Memory *mem = 0;
    if (memdebug) {
        mem = new Memory;
        mem->startStats();
    }
    
    Buffer buf;
    if (memdebug) {
        mem->addStats(__LINE__);             // take a sample
    }
    
    if (buf.size() == gnash::NETBUFSIZE) {
         runtest.pass ("Buffer::size()");
     } else {
         runtest.fail ("Buffer::size()");
    }

    if (memdebug) {
        mem->addStats(__LINE__);             // take a sample
    }
    buf.resize(112);
    if (memdebug) {
        mem->addStats(__LINE__);             // take a sample
    }

    if (buf.size() == 112) {
         runtest.pass ("Buffer::resize()");
     } else {
         runtest.fail ("Buffer::resize()");
    }
    if (memdebug) {
        mem->addStats(__LINE__);             // take a sample
    }

    // test creating Buffers
    test_construct();
    // test destroying Buffers
    test_destruct();

    test_copy();
    test_find();
    test_append();
    test_remove();
    test_operators();

// amf::Buffer::resize(unsigned int)
    
    if (memdebug) {
        mem->analyze();
    }

    // cleanup
    if (mem) {
        delete mem;
    }
}

void
test_copy()
{
    // Make some data for the buffers
    Network::byte_t *data = 0;
    data = new Network::byte_t[10];
    memset(data, 0, 10);
    for (size_t i=1; i<10; i++) {
        *(data + i) = i + '0';
    }

    Buffer buf1;
    Network::byte_t *ptr1 = 0;
    ptr1 = buf1.reference();

    buf1.copy(data, 10);
    if (memcmp(ptr1, data, 10) == 0) {
         runtest.pass ("Buffer::copy(Network::byte_t *, size_t)");
    } else {
         runtest.fail ("Buffer::copy(Network::byte_t *, size_t)");
    }

    const char *str = "I'm bored";
    string str1 = str;
    buf1.copy(str1);
    if (memcmp(ptr1, str, 9) == 0) {
         runtest.pass ("Buffer::copy(std::string &)");
    } else {
         runtest.fail ("Buffer::copy(std::string &)");
    }

    Buffer buf2;
    buf2.copy(str);
    Network::byte_t *ptr2 = buf2.reference();
    if (memcmp(ptr2, str, 9) == 0) {
         runtest.pass ("Buffer::copy(const char *)");
    } else {
         runtest.fail ("Buffer::copy(const char)");
    }

    boost::uint16_t length = 12;
    Buffer buf3;
    buf3.copy(length);
    Network::byte_t *ptr3 = buf3.reference();
    boost::uint16_t newlen = *(reinterpret_cast<boost::uint16_t *>(ptr3));
    if (length == newlen) {
         runtest.pass ("Buffer::copy(boost::uint16_t)");
    } else {
         runtest.fail ("Buffer::copy(boost::uint16_t)");
    }
}

void
test_find()
{
    // Make some data for the buffers
    Network::byte_t *data = new Network::byte_t[10];
    for (size_t i=0; i<10; i++) {
        data[i] = i + 'a';
    }

    Buffer buf1, buf2, buf3;
    Network::byte_t *ptr1 = buf1.reference();

    // populate the buffer
    buf1.copy(data, 10);

    // See if we can find a character
    Network::byte_t *fptr = buf1.find('c');
    if (fptr == (ptr1 + 2)) {
         runtest.pass ("Buffer::find(Network::byte_t)");
    } else {
         runtest.fail ("Buffer::find(Network::byte_t)");
    }

    char *sub = "fgh";
    Network::byte_t *ptr2 = reinterpret_cast<Network::byte_t *>(sub);
    fptr = buf1.find(ptr2, 3);
    if (fptr == (ptr1 + 5)) {
         runtest.pass ("Buffer::find(Network::byte_t *, size_t)");
    } else {
         runtest.fail ("Buffer::find(Network::byte_t *, size_t)");
    }

// amf::Buffer::init(unsigned int)
}

void
test_append()
{
    Buffer buf1;
    buf1.clear();
    Network::byte_t *ptr1 = buf1.reference();

    Network::byte_t *data1 = new Network::byte_t[10];
    memset(data1, 0, 10);
    for (size_t i=0; i< 10; i++) {
        data1[i] = i + 'a';
    }
    Network::byte_t *data2 = new Network::byte_t[10];
    memset(data2, 0, 10);
    for (size_t i=0; i< 10; i++) {
        data2[i] = i + 'A';
    }

    // append a string of bytes
    Network::byte_t *data3 = new Network::byte_t[20];
    memcpy(data3, data1, 10);
    memcpy(data3+10, data2, 10);
    buf1.copy(data1, 10);
    buf1.append(data2, 10);
    if (memcmp(data3, buf1.reference(), 20) == 0) {
         runtest.pass ("Buffer::append(Network::byte_t *, size_t)");
    } else {
         runtest.fail ("Buffer::append(Network::byte_t *, size_t)");
    }

    // append an unsigned byte
    Buffer buf2(30);
    buf2.clear();
    buf2.copy(data1, 10);
    Network::byte_t byte = '@';
    buf2.append(byte);
    memset(data3, 0, 20);
    memcpy(data3, data1, 10);
    *(data3 + 10) = '@';
    if (memcmp(data3, buf2.reference(), 11) == 0) {
         runtest.pass ("Buffer::append(Network::byte_t)");
    } else {
         runtest.fail ("Buffer::append(Network::byte_t)");
    }

    double num = 1.2345;
    Buffer buf3;
    buf3.clear();
    buf3.copy(data1, 10);
    buf3.append(num);
    
    memset(data3, 0, 20);
    memcpy(data3, data1, 10);
    memcpy(data3 + 10, &num, sizeof(double));
    if (memcmp(data3, buf3.reference(), 10+sizeof(double)) == 0) {
         runtest.pass ("Buffer::append(double)");
    } else {
         runtest.fail ("Buffer::append(double)");
    }
    
// amf::Buffer::append(amf::Element::amf_type_e)
// amf::Buffer::append(amf::Buffer*)
// amf::Buffer::append(std::string const&)
// amf::Buffer::append(amf::Buffer&)
// amf::Buffer::append(bool)
// amf::Buffer::append(unsigned int)
// amf::Buffer::append(unsigned short)
}

void
test_remove()
{
    Network::byte_t *data1 = new Network::byte_t[10];
    memset(data1, 0, 10);
    Network::byte_t *data2 = new Network::byte_t[10];
    memset(data2, 0, 10);
    for (size_t i=0; i< 10; i++) {
        data1[i] = i + 'a';
    }

    // Build identical buffer nissing one character
    memcpy(data2, data1, 6);
    memcpy(data2 + 6, data1 + 7, 5);

    // Remove a single byte
    Network::byte_t byte = 'g';
    Buffer buf1(10);
    buf1.clear();
    buf1.copy(data1, 10);
    buf1.remove(byte);
    if (memcmp(data2, buf1.reference(), 9) == 0) {
         runtest.pass ("Buffer::remove(Network::byte_t)");
    } else {
         runtest.fail ("Buffer::remove(Network::byte_t)");
    }
    
    Buffer buf2(10);
    buf2.clear();
    buf2.copy(data1, 10);
    buf2.remove(6);
    if (memcmp(data2, buf2.reference(), 9) == 0) {
         runtest.pass ("Buffer::remove(int)");
    } else {
         runtest.fail ("Buffer::remove(int)");
    }

    // Remove a range of bytes
    Network::byte_t *data3 = new Network::byte_t[10];
    memset(data3, 0, 10);
    memcpy(data3, data1, 6);
    memcpy(data3 + 6, data1 + 9, 5);
    
    Buffer buf3(10);
    buf3.clear();
    buf3.copy(data1, 10);
    buf3.remove(6, 8);
    if (memcmp(data3, buf3.reference(), 7) == 0) {
         runtest.pass ("Buffer::remove(int, int)");
    } else {
         runtest.fail ("Buffer::remove(int, int)");
    }    
}

void
test_construct()
{
    bool valgrind = false;
    
    Memory mem(5);
    mem.addStats(__LINE__);             // take a sample
    Buffer buf1;
    mem.addStats(__LINE__);             // take a sample
    size_t diff = mem.diffStats() - sizeof(buf1);
    if (diff > NETBUFSIZE) {
        valgrind = true;
        log_debug("Running this test case under valgrind screws up mallinfo(), so the results get skewed");
    }

    // Different systems allocate memory slightly differently, so about all we can do to see
    // if it worked is check to make sure it's within a tight range of possible values.
    if ((buf1.size() == NETBUFSIZE) && (diff >= (NETBUFSIZE - (sizeof(long *) * 4)) && diff <= (NETBUFSIZE + sizeof(long *)*4))) {
        runtest.pass ("Buffer::Buffer()");
    } else {
        if (valgrind) {
            runtest.unresolved("Buffer::Buffer()) under valgrind");
        } else {
            runtest.fail("Buffer::Buffer()");
        }
    }
    
    mem.addStats(__LINE__);             // take a sample
    Buffer buf2(124);
    mem.addStats(__LINE__);             // take a sample
    diff = mem.diffStats() - sizeof(long *);
    if ((buf2.size() == 124) && (124 - (sizeof(long *) * 4)) && diff <= (124 + sizeof(long *)*4)) {
        runtest.pass ("Buffer::Buffer(size_t)");
    } else {
        if (valgrind) {
            runtest.unresolved("Buffer::Buffer(size_t) under valgrind");
        } else {
            runtest.fail("Buffer::Buffer(size_t)");
        }
    }
}

// make sure when we delete a Buffer, *all* the allocated
// memory goes away. As the only way to do this is to examine
// the malloc buffers in the kernel, this will only work on
// POSIX conforming systems, and probabably only Linux & BSD.
void
test_destruct()
{
    Memory mem(5);
    mem.addStats(__LINE__);             // take a sample
    Buffer *buf1, *buf2;

    mem.startCheckpoint();
    buf1 = new Buffer(NETBUFSIZE);
    delete buf1;
    
    if (mem.endCheckpoint()) {
        runtest.pass ("Buffer::~Buffer()");
    } else {
        runtest.fail ("Buffer::~Buffer()");
    }

    mem.startCheckpoint();
    buf2 = new Buffer(124);
    delete buf2;
    
    if (mem.endCheckpoint()) {
        runtest.pass ("Buffer::~Buffer(size_t)");
    } else {
        runtest.fail ("Buffer::~Buffer(size_t)");
    }
    
}

void
test_operators()
{
    // Make some data for the buffers
    Buffer buf1, buf2;
    // valgrind gets pissed unless we zero the memory. Constructing
    // a buffer doesn't clear the memory to save speed, and it's
    // also unnecessary normally, but makes debugging easier.
    buf1.clear();
    buf2.clear();

    boost::uint8_t *ptr1 = buf1.reference();
    for (size_t i=1; i< buf1.size(); i++) {
        ptr1[i] = i;
    }

    // buf1 has data, but buf2 doesn't, so if the
    // equivalance test fails, then this passed.
    buf2.clear();
    if (buf2 == buf1) {
         runtest.fail ("Buffer::operator==(Buffer &)");
     } else {
         runtest.pass ("Buffer::operator==(Buffer &)");
    }

    // This makes the new buffer be identical to the
    // the source buffer, including copying all the data.
    buf2 = buf1;
    if (buf1 == buf2) {
         runtest.pass ("Buffer::operator=(Buffer &)");
     } else {
         runtest.fail ("Buffer::operator=(Buffer &)");
    }

    Buffer *buf3, *buf4;
    buf3 = new Buffer;
    buf4 = new Buffer;
    boost::uint8_t *ptr2 = buf3->reference();
    for (size_t i=1; i< buf3->size(); i++) {
        ptr2[i] = i + 'a';
    }
    if (buf3 == buf4) {
         runtest.fail ("Buffer::operator==(Buffer *)");
     } else {
         runtest.pass ("Buffer::operator==(Buffer *)");
    }

    // This makes the new buffer be identical to the
    // the source buffer, including copying all the data.
    buf4 = buf3;
    if (buf3 == buf4) {
         runtest.pass ("Buffer::operator=(Buffer *)");
    } else {
         runtest.fail ("Buffer::operator=(Buffer *)");
    }

    Buffer buf5(10);
    boost::uint8_t *ptr3 = buf5.reference();
    buf5 += 'a';
    buf5 += 'b';
    buf5 += 'c';
    if (memcmp(ptr3, "abc", 3) == 0) {
         runtest.pass ("Buffer::operator+=(char)");
    } else {
         runtest.fail ("Buffer::operator+=(char)");
    }

    Buffer buf6(10);
    buf6 += 'D';
    buf6 += 'E';
    buf6 += 'F';
    buf5 += buf6;
    ptr3 = buf5.reference();    // refresh the pointer, as it changes
                                // on a resize()
    // The size should now be the default 10, plus the 3 characters
    // already added.
    if ((memcmp(ptr3, "abcDEF", 6) == 0) && (buf5.size() == 13)) {
         runtest.pass ("Buffer::operator+=(Buffer &)");
    } else {
         runtest.fail ("Buffer::operator+=(Buffer &)");
    }
}

static void
usage()
{
    cout << _("test_buffer - test Buffer class") << endl
         << endl
         << _("Usage: cygnal [options...]") << endl
         << _("  -h,  --help          Print this help and exit") << endl
         << _("  -v,  --verbose       Output verbose debug info") << endl
         << _("  -m,  --memdebug      Output memory statistics") << endl
         << endl;
}

