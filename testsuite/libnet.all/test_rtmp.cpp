// 
//   Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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
#include "gnashconfig.h"
#endif

#ifdef HAVE_DEJAGNU_H

//#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string>

#include "as_object.h"
#include "dejagnu.h"
#include "log.h"
#include "amf.h"
#include "rtmp.h"
#include "rtmp_client.h"
#include "rtmp_server.h"
#include "buffer.h"
#include "network.h"
#include "element.h"
#include "sol.h"
#include "arg_parser.h"
#include "gmemory.h"

using namespace amf;
using namespace gnash;
using namespace std;

static void usage (void);

// Enable the display of memory allocation and timing data
static bool memdebug = false;

// We use the Memory profiling class to check the malloc buffers
// in the kernel to make sure the allocations and frees happen
// the way we expect them too. There is no real other way to tell.
#if defined(HAVE_MALLINFO) && defined(USE_STATS_MEMORY)
Memory *mem = 0;
#endif

static TestState runtest;

static void test_header();
static void test_types();
static void test_results();
static void test_system();
static void test_client();
static void test_split();

LogFile& dbglogfile = LogFile::getDefaultInstance();

// These next two functions are borrowed from Libgloss, part of the GNU binutils,
// of which I am the primary author and copyright holder.
// convert an ascii hex digit to a number.
//      param is hex digit.
//      returns a decimal digit.
Network::byte_t
hex2digit (Network::byte_t digit)
{  
    if (digit == 0)
        return 0;
    
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    if (digit >= 'a' && digit <= 'f')
        return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F')
        return digit - 'A' + 10;
    
    // shouldn't ever get this far
    return -1;
}

// Convert the hex array pointed to by buf into binary to be placed in mem
Buffer *
hex2mem(const char *str)
{
    size_t count = strlen(str);
    Network::byte_t ch = 0;
    Buffer *buf = new Buffer((count/3)+1);
    buf->clear();

    Network::byte_t *ptr = const_cast<Network::byte_t *>(reinterpret_cast<const Network::byte_t *>(str));
    
    for (size_t i=0; i<count; i++) {
        if (*ptr == ' ') {      // skip spaces.
            ptr++;
            continue;
        }
        ch = hex2digit(*ptr++) << 4;
        ch |= hex2digit(*ptr++);
        buf->append(ch);
    }

    return buf;
}

int
main(int argc, char *argv[])
{
    const Arg_parser::Option opts[] =
        {
            { 'h', "help",          Arg_parser::no  },
            { 'v', "verbose",       Arg_parser::no  },
            { 'w', "write",         Arg_parser::no  },
// Unless you have support for memory debugging turned on, and
// you have support for the Linux mallinfo() system call,
// this option is totally useless. This doesn't really matter
// as the memory testing is primarily used only during
// debugging or development.
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
                    log_debug(_("Verbose output turned on"));
                    break;
              case 'm':
                    log_debug(_("Enabling memory statistics"));
                    memdebug = true;
                    break;
	    }
        }
        
        catch (Arg_parser::ArgParserException &e) {
            cerr << _("Error parsing command line options: ") << e.what() << endl;
            cerr << _("This is a Gnash bug.") << endl;
        }
    }

#if defined(HAVE_MALLINFO) && defined(USE_STATS_MEMORY)
    if (memdebug) {
        mem = new Memory;
        mem->startStats();
    }
#endif
    
    test_header();
    test_system();
    test_client();
    test_results();
    test_split();
//    test_types();
}

void
test_split()
{
    GNASH_REPORT_FUNCTION;
    
    RTMPClient client;
    bool notest = false;
    RTMP::rtmp_head_t *rthead = 0;

    Buffer *buf1 = hex2mem("04 00 00 00 00 00 b8 14 01 00 00 00 02 00 08 6f 6e 53 74 61 74 75 73 00 00 00 00 00 00 00 00 00 05 03 00 05 6c 65 76 65 6c 02 00 06 73 74 61 74 75 73 00 04 63 6f 64 65 02 00 14 4e 65 74 53 74 72 65 61 6d 2e 50 6c 61 79 2e 52 65 73 65 74 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 2d 50 6c 61 79 69 6e 67 20 61 6e 64 20 72 65 73 65 74 74 69 6e 67 20 67 61 74 65 30 36 5f 74 61 62 6c 61 6e 5f 62 63 75 65 75 5f 30 31 2e c4 00 07 64 65 74 61 69 6c 73 02 00 16 67 61 74 65 30 36 5f 74 61 62 6c 61 6e 5f 62 63 75 65 75 5f 30 31 00 08 63 6c 69 65 6e 74 69 64 00 41 bf e4 78 30 00 00 00 00 00 09");
    CQue *queues1 = client.split(buf1, 128);
    if (queues1 == 0) {
        notest = true;
    }
    
    if (notest) {
        runtest.untested("RTMP::split(2 packets size)");
    } else {
        if (queues1[4].size() == 2) {
            runtest.pass("RTMP::split(2 packets size)");
        } else {
            runtest.fail("RTMP::split(2 packets size)");
            notest = true;
        }
    }


//     Buffer *tmpbuf = que1.front();
//     que1.pop_front();
    Buffer *tmpbuf = queues1[4].pop();
    if (notest) {
        runtest.untested("RTMP::split(1st packet header) of 2");
    } else {
        if (*tmpbuf->reference() == 0x4) {
            runtest.pass("RTMP::split(1st packet header) of 2");
        } else {
            runtest.fail("RTMP::split(1st packet header) of 2" );
            notest = true;
        }
    }

    tmpbuf = queues1[4].pop();
    if (notest) {
        runtest.untested("RTMP::split(2nd packet header) of 2");
    } else {
        if (*tmpbuf->reference() == 0xc4) {
            runtest.pass("RTMP::split(2nd packet header) of 2");
        } else {
            runtest.fail("RTMP::split(2nd packet header) of 2");
            notest = true;
        }
    }

    Buffer *buf2 = hex2mem("02 00 00 00 00 00 04 01 00 00 00 00 00 00 00 80 02 00 00 00 00 00 06 04 00 00 00 00 00 04 00 00 00 01 04 00 00 00 00 00 b8 14 01 00 00 00 02 00 08 6f 6e 53 74 61 74 75 73 00 00 00 00 00 00 00 00 00 05 03 00 05 6c 65 76 65 6c 02 00 06 73 74 61 74 75 73 00 04 63 6f 64 65 02 00 14 4e 65 74 53 74 72 65 61 6d 2e 50 6c 61 79 2e 52 65 73 65 74 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 2d 50 6c 61 79 69 6e 67 20 61 6e 64 20 72 65 73 65 74 74 69 6e 67 20 67 61 74 65 30 36 5f 74 61 62 6c 61 6e 5f 62 63 75 65 75 5f 30 31 2e 02 00 00 00 00 00 06 04 00 00 00 00 00 00 00 00 00 01 c4 00 07 64 65 74 61 69 6c 73 02 00 16 67 61 74 65 30 36 5f 74 61 62 6c 61 6e 5f 62 63 75 65 75 5f 30 31 00 08 63 6c 69 65 6e 74 69 64 00 41 d8 fb 78 56 00 00 00 00 00 09");
    CQue *queues2 = client.split(buf2, 128);
    if (queues2 == 0) {
        notest = true;
    }    
    if (notest) {
        runtest.fail("RTMP::split(5 packets)");
    } else {
        if ((queues2[2].size() == 3) && (queues2[4].size() == 2)) {
            runtest.pass("RTMP::split(5 packets)");
            notest = false;
        } else {
            runtest.fail("RTMP::split(5 packets)");
        }
    }
    
    if (notest) {
        runtest.untested("RTMP::split(1st packet header of 5)");
    } else {
        tmpbuf = queues2[2].pop();
        if (*tmpbuf->reference() == 0x2) {
            runtest.pass("RTMP::split(1st packet header) of 5");
        } else {
            runtest.fail("RTMP::split(1st packet header) of 5");
        }
    }
    if (notest) {
        runtest.untested("RTMP::split(2nd packet header) of 5");
    } else {
        tmpbuf = queues2[2].pop();
        if (*tmpbuf->reference() == 0x2) {
            runtest.pass("RTMP::split(2nd packet header) of 5");
        } else {
            runtest.fail("RTMP::split(2nd packet header) of 5");
        }
    }

    if (notest) {
        runtest.untested("RTMP::split(3rd packet header) of 5");
    } else {
        tmpbuf = queues2[4].pop();
        if (*tmpbuf->reference() == 0x04) {
            runtest.pass("RTMP::split(3rd packet header) of 5");
        } else {
            runtest.fail("RTMP::split(3rd packet header) of 5");
        }
    }
    
    if (notest) {
        runtest.untested("RTMP::split(4th packet header) of 5");
    } else {
        tmpbuf = queues2[2].pop();
        if (*tmpbuf->reference() == 0x02) {
            runtest.pass("RTMP::split(4th packet header) of 5");
        } else {
            runtest.fail("RTMP::split(4th packet header) of 5");
        }
    }

    if (notest) {
        runtest.untested("RTMP::split(5th packet header) of 5");
    } else {
        tmpbuf = queues2[4].pop();
        if (*tmpbuf->reference() == 0xc4) {
            runtest.pass("RTMP::split(5th packet header) of 5");
        } else {
            runtest.fail("RTMP::split(5th packet header) of 5");
        }
    }
    
    // Try a much more complex packet, similar to the previous one, but with more intermixed packets
    // for other channels.
//    ...............onStatus.............level...status..code...NetStream.Play.Start..description..'Started playing gate06_tablan_bcueu_01...clie......'.......xF....?j.....@....?..O.]...............................;...../..rP.....K.......m......,......%......................B........M.<.$.....`.......i..9..C..J..........%..........G....2Np.".1`@................;.ntid.A..xV.....
    Buffer *buf3 = hex2mem("05 00 00 00 00 00 90 14 01 00 00 00 02 00 08 6f 6e 53 74 61 74 75 73 00 00 00 00 00 00 00 00 00 05 03 00 05 6c 65 76 65 6c 02 00 06 73 74 61 74 75 73 00 04 63 6f 64 65 02 00 14 4e 65 74 53 74 72 65 61 6d 2e 50 6c 61 79 2e 53 74 61 72 74 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 27 53 74 61 72 74 65 64 20 70 6c 61 79 69 6e 67 20 67 61 74 65 30 36 5f 74 61 62 6c 61 6e 5f 62 63 75 65 75 5f 30 31 2e 00 08 63 6c 69 65 07 00 00 00 00 00 27 09 01 00 00 00 14 00 78 46 0f 14 0f 14 3f 6a ff ff 00 08 9f 40 10 9f f8 8b 3f fd b2 4f fb 5d c0 00 00 00 00 00 00 00 00 00 00 00 00 08 00 00 00 00 00 00 08 01 00 00 00 08 00 00 00 00 01 3b 08 01 00 00 00 2f ff fb 72 50 00 00 00 00 00 4b 00 00 00 00 07 e0 09 6d 00 00 00 00 00 01 2c 00 00 00 00 1f 80 25 b4 00 00 00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff fc 0c 87 42 80 ec c8 b0 0e 90 c2 12 4d 90 3c 18 24 16 01 88 03 e1 60 1a 1a a0 1a 09 9c 1a 69 a1 10 39 06 8d 43 02 c3 4a 12 0b 00 c8 1f 0b 00 d8 16 00 25 9f ff ff fe c1 a0 00 00 ff 8a 47 80 80 0e 1e 32 4e 70 f1 22 ed 31 60 40 f8 02 00 00 00 00 00 04 01 00 00 00 00 00 00 01 3b c5 6e 74 69 64 00 41 d8 fb 78 56 00 00 00 00 00 09");
    CQue *queues3 = client.split(buf3, 128);
    if (queues3 == 0) {
        notest = true;
    }    
    if (notest) {
        runtest.fail("RTMP::split(corrupted packets)");
    } else {
        if ((queues3[2].size() == 1) && (queues3[5].size() == 2) && (queues3[8].size() == 2) && (queues3[7].size() == 1)) {
            runtest.pass("RTMP::split(6 complex packets)");
            notest = false;
        } else {
            runtest.fail("RTMP::split(6 complex packets)");
        }
    }

    if (notest) {
        runtest.untested("RTMP::split(1st packet header) of 6");
    } else {
        tmpbuf = queues2[5].pop();
        rthead = client.decodeHeader(tmpbuf);
        if ((*tmpbuf->reference() == 0x05)  && (rthead->type <= RTMP::INVOKE)) {
            runtest.pass("RTMP::split(1st packet header) of 6");
        } else {
            runtest.fail("RTMP::split(1st packet header) of 6");
        }
    }
    
    if (notest) {
        runtest.untested("RTMP::split(2nd packet header) of 6");
    } else {
        tmpbuf = queues2[7].pop();
        rthead = client.decodeHeader(tmpbuf);
        if ((*tmpbuf->reference() == 0x07)  && (rthead->type <= RTMP::VIDEO_DATA)) {
            runtest.pass("RTMP::split(2nd packet header) of 6");
        } else {
            runtest.fail("RTMP::split(2nd packet header) of 6");
        }
    }

    if (notest) {
        runtest.untested("RTMP::split(3rd packet header) of 6");
    } else {
        tmpbuf = queues2[8].pop();
        rthead = client.decodeHeader(tmpbuf);
        if ((*tmpbuf->reference() == 0x08)  && (rthead->type <= RTMP::AUDIO_DATA)) {
            runtest.pass("RTMP::split(3rd packet header) of 6");
        } else {
            runtest.fail("RTMP::split(3rd packet header) of 6");
        }
    }
    
    if (notest) {
        runtest.untested("RTMP::split(4th packet header) of 6");
    } else {
        tmpbuf = queues2[8].pop();
        rthead = client.decodeHeader(tmpbuf);
        if ((*tmpbuf->reference() == 0x08)  && (rthead->type <= RTMP::AUDIO_DATA)) {
            runtest.pass("RTMP::split(4th packet header) of 6");
        } else {
            runtest.fail("RTMP::split(4th packet header) of 6");
        }
    }
    if (notest) {
        runtest.untested("RTMP::split(5th packet header) of 6");
    } else {
        tmpbuf = queues2[2].pop();
        rthead = client.decodeHeader(tmpbuf);
        if ((*tmpbuf->reference() == 0x02)  && (rthead->type <= RTMP::AUDIO_DATA)) {
            runtest.pass("RTMP::split(5th packet header) of 6");
        } else {
            runtest.fail("RTMP::split(5th packet header) of 6");
        }
    }
    if (notest) {
        runtest.untested("RTMP::split(6th packet header) of 6");
    } else {
        tmpbuf = queues2[5].pop();
        rthead = client.decodeHeader(tmpbuf);
        if ((*tmpbuf->reference() == 0xc5) && (rthead->type <= RTMP::CHUNK_SIZE)) {
            runtest.pass("RTMP::split(6th packet header) of 6");
        } else {
            runtest.fail("RTMP::split(6th packet header) of 6");
        }
    }    
    
    delete buf3;
    
    delete buf1;
//    delete que1;

//     delete buf2;
//     delete que2;
}

void
test_system()
{
    GNASH_REPORT_FUNCTION;
    
    RTMPClient client;
    RTMPServer server;
    
    Buffer *buf1 = hex2mem("00 00 00 00 00 00");  // clear buffer message
    Buffer *buf2 = hex2mem("00 06 cf 03 04 c3"); // ping client from server
    Buffer *buf3 = hex2mem("00 07 cf 03 04 c3"); // Pong, reply from client
    Buffer *buf4 = hex2mem("00 00 00 00 00 01"); // clear buffer message
    
    RTMP::rtmp_ping_t *ping1 = client.decodePing(buf1);
    if (ping1->type == RTMP::PING_CLEAR) {
        runtest.pass("Decoded RTMP Ping message");
    } else {
        runtest.fail("Decoded RTMP Ping message");
    }
    
    Buffer *enc1 = server.encodePing(RTMP::PING_CLEAR);
    if ((memcmp(buf1->reference(), enc1->reference(), 6) == 0)) {
        runtest.pass("Encoded RTMP Ping Clear message");
    } else {
        runtest.fail("Encoded RTMP Ping Clear message");
    }

    boost::uint32_t time = *(reinterpret_cast<boost::uint32_t *>(buf2->reference() + 2));
    Buffer *enc2 = server.encodePing(RTMP::PING_CLIENT, htonl(time));
//    cerr << hexify(enc2->begin(), enc2->size(), false) << endl;
    if ((memcmp(buf2->reference(), enc2->reference(), 6) == 0)) {
        runtest.pass("Encoded RTMP Ping Client message");
    } else {
        runtest.fail("Encoded RTMP Ping Client message");
    }

    RTMP::rtmp_ping_t *ping2 = client.decodePing(buf2);
    if ((ping2->type == RTMP::PING_CLIENT)
        && (ping2->target == 0xcf03)
        && (ping2->param1 == 0x4c3)) {
        runtest.pass("Decoded RTMP Ping Client message");
    } else {
        runtest.fail("Decoded RTMP Ping Client message");
    }    

#if 0
    for (double dub=0; dub<=200; dub ++) {
        Element el11;
        el11.makeNumber(dub);
        Buffer *buf11 = el11.getBuffer();
        cerr << "FIXME: " << el11.to_number() << ":     ";
        swapBytes(buf11->begin(), 8);
        cerr << hexify(buf11->begin(), buf11->size(), false) << endl;
    }
#endif
    
    // cleanup
    delete ping1;
    delete ping2;
    delete buf1;
    delete buf2;
    delete buf3;
    delete buf4;
    
    delete enc1;
    delete enc2;
}    

void
test_header()
{
    GNASH_REPORT_FUNCTION;
    RTMPClient client;
    RTMPServer server;
    
    // this is a sample 12 bytes RTMP header
//    const char *x1 = "03 00 00 00 00 01 1f 14 00 00 00 00";
    Buffer *buf1 = hex2mem("03 00 00 00 00 01 1f 14 00 00 00 00");
    Buffer *head1 = server.encodeHeader(0x3, RTMP::HEADER_12, 287,
                                        RTMP::INVOKE, RTMPMsg::FROM_SERVER);
//    cerr << hexify(head1->begin(), RTMP_MAX_HEADER_SIZE, false) << endl;
    
     if ((memcmp(buf1->reference(), head1->reference(), RTMP_MAX_HEADER_SIZE) == 0)) {
         runtest.pass("Encoded RTMP header(Invoke)");
     } else {
         runtest.fail("Encoded RTMP header(Invoke)");
     }

     RTMP::rtmp_head_t *header1 = server.decodeHeader(buf1->reference());
     if ((header1->channel == 0x3) && (header1->head_size == RTMP_MAX_HEADER_SIZE)
         && (header1->bodysize == 287) && (header1->type ==  RTMP::INVOKE)) {
         runtest.pass("Decoded RTMP header(Invoke)");
     } else {
         runtest.fail("Decoded RTMP header(Invoke)");
     }

     Buffer *buf2 = hex2mem("02 00 00 00 00 00 06 04 00 00 00 00");
     Buffer *head2 = server.encodeHeader(0x2, RTMP::HEADER_12, PING_MSG_SIZE,
                                     RTMP::PING, RTMPMsg::FROM_SERVER);
//     cerr << hexify(head2->begin(), RTMP_MAX_HEADER_SIZE, false) << endl;
     if ((memcmp(buf2->reference(), head2->reference(), 8) == 0)) {
         runtest.pass("Encoded RTMP header(Ping 0)");
     } else {
         runtest.fail("Encoded RTMP header(Ping 0)");
     }

     Buffer *buf3 = hex2mem("02 ff e3 6c 00 00 06 04 00 00 00 00");
     Buffer *head3 = server.encodeHeader(0x2, RTMP::HEADER_12, PING_MSG_SIZE,
                                     RTMP::PING, RTMPMsg::FROM_SERVER);
//     cerr << hexify(head3->begin(), RTMP_MAX_HEADER_SIZE, false) << endl;
     if ((memcmp(buf2->reference(), head3->reference(), 8) == 0)) {
         runtest.pass("Encoded RTMP header(Ping 1)");
     } else {
         runtest.fail("Encoded RTMP header(Ping 1)");
     }

     RTMP::rtmp_head_t *header2 = client.decodeHeader(buf3);
     if ((header2->channel == 0x2) && (header2->head_size == RTMP_MAX_HEADER_SIZE)
         && (header2->bodysize == 6) && (header2->type ==  RTMP::PING)) {
         runtest.pass("Decoded RTMP header(Ping)");
     } else {
         runtest.fail("Decoded RTMP header(Ping)");
     }

     Buffer *buf4 = hex2mem("c2");
     Buffer *head4 = server.encodeHeader(0x2, RTMP::HEADER_1);
//     cerr << hexify(head4->begin(), RTMP_MAX_HEADER_SIZE, false) << endl;
     if ((memcmp(buf4->reference(), head4->reference(), 1) == 0)) {
         runtest.pass("Encoded RTMP header(size 1)");
     } else {
         runtest.fail("Encoded RTMP header(size 1)");
     }

// 43 00 00 00 00 00 15 14 02 00 08 6f 6e 42 57 44    onBWDone
// 6f 6e 65 00 40 00 00 00 00 00 00 00 05

     Buffer *buf5 = hex2mem("43 00 00 00 00 00 19 14");
     Buffer *head5 = server.encodeHeader(0x3, RTMP::HEADER_8, 0x19, RTMP::INVOKE,
                                         RTMPMsg::FROM_CLIENT);
//     head5->dump();
//     cerr << hexify(head5->begin(), 8, false) << endl;
     if ((memcmp(buf5->reference(), head5->reference(), 8) == 0)) {
         runtest.pass("Encoded RTMP header(size 8)");
     } else {
         runtest.fail("Encoded RTMP header(size 8)");
     }
     RTMP::rtmp_head_t *header3 = client.decodeHeader(buf5);
     if ((header3->channel == 0x3) && (header3->head_size == 8)
         && (header3->bodysize == 0x19) && (header3->type ==  RTMP::INVOKE)) {
         runtest.pass("Decoded RTMP header(size 8)");
     } else {
         runtest.fail("Decoded RTMP header(size 8)");
     }

     
     // cleanup after ourselves
     delete buf1;
     delete buf2;
     delete buf3;
     delete buf4;
     delete head1;
     delete head2;
     delete head3;
     delete head4;
//     delete header1;
//      delete header2;
}

void
test_results()
{
    GNASH_REPORT_FUNCTION;
    RTMPServer rtmpserv;
//   03 00 00 00 00 00 81 14    00 00 00 00 02 00 07 5f    ..............._
//   72 65 73 75 6c 74 00 3f    f0 00 00 00 00 00 00 05    result.?........
//   03 00 0b 61 70 70 6c 69    63 61 74 69 6f 6e 05 00    ...application..
//   05 6c 65 76 65 6c 02 00    06 73 74 61 74 75 73 00    .level...status.
//   0b 64 65 73 63 72 69 70    74 69 6f 6e 02 00 15 43    .description...C
//   6f 6e 6e 65 63 74 69 6f    6e 20 73 75 63 63 65 65    onnection succee
//   64 65 64 2e 00 04 63 6f    64 65 02 00 1d 4e 65 74    ded...code...Net
//   43 6f 6e 6e 65 63 74 69    6f 6e 2e 43 6f 6e 6e 65    Connection.Conne
//   63 74 2e 53 75 63 63 65    73 73 00 00 c3 09          ct.Success....
    Buffer *hex2 = hex2mem("02 00 07 5f 72 65 73 75 6c 74 00 3f f0 00 00 00 00 00 00 05 03 00 0b 61 70 70 6c 69 63 61 74 69 6f 6e 05 00 05 6c 65 76 65 6c 02 00 06 73 74 61 74 75 73 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 15 43 6f 6e 6e 65 63 74 69 6f 6e 20 73 75 63 63 65 65 64 65 64 2e 00 04 63 6f 64 65 02 00 1d 4e 65 74 43 6f 6e 6e 65 63 74 69 6f 6e 2e 43 6f 6e 6e 65 63 74 2e 53 75 63 63 65 73 73 00 00 09");

    RTMPMsg *msg1 = rtmpserv.decodeMsgBody(hex2);
    if (msg1) {
        std::vector<amf::Element *> hell = msg1->getElements();
        std::vector<amf::Element *> props = hell[0]->getProperties();        
//         printf("FIXME: %d, %d, %s:%s\n", props.size(), msg1->getStatus(),
//                props[3]->getName(), props[3]->to_string());
//         msg1->dump();
//        msg1->dump();
        if ((msg1->getStatus() ==  RTMPMsg::NC_CONNECT_SUCCESS)
            && (msg1->getMethodName() == "_result")
            && (msg1->getStreamID() == 1)
            && (msg1->size() == 1)) { // the msg has one element, which has 4 properties
            runtest.pass("Decoded RTMP Result(NC_CONNECT_SUCCESS) message");
        } else {
            runtest.fail("Decoded RTMP Result(NC_CONNECT_SUCCESS) message");
        }
    } else {
        runtest.untested("Decoded RTMP Result(NC_CONNECT_SUCCESS) message");
    }
    delete msg1;

    Buffer *buf2 = rtmpserv.encodeResult(RTMPMsg::NC_CONNECT_SUCCESS);
//    cerr << hexify(buf2->begin(), 122, true) << endl;
    if ((memcmp(hex2->reference(), buf2->reference(), 122) == 0)) {
        runtest.pass("Encoded RTMP result(NC_CONNECT_SUCCESS)");
    } else {
        runtest.fail("Encoded RTMP result(NC_CONNECT_SUCCESS)");
    }
    delete buf2;
    delete hex2;
    
    Buffer *hex3 = hex2mem("02 00 07 5f 72 65 73 75 6c 74 00 3f f0 00 00 00 00 00 00 05 03 00 0b 61 70 70 6c 69 63 61 74 69 6f 6e 05 00 05 6c 65 76 65 6c 02 00 05 65 72 72 6f 72 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 00 00 04 63 6f 64 65 02 00 1c 4e 65 74 43 6f 6e 6e 65 63 74 69 6f 6e 2e 43 6f 6e 6e 65 63 74 2e 46 61 69 6c 65 64 00 00 09");
    RTMPMsg *msg2 = rtmpserv.decodeMsgBody(hex3);
    std::vector<amf::Element *> hell = msg2->getElements();
    std::vector<amf::Element *> props = hell[0]->getProperties();        
//     printf("FIXME: %d, %d, %s:%s\n", props.size(), msg1->getStatus(),
//            props[3]->getName(), props[3]->to_string());
    if (msg2) {
//        msg2->dump();
        if ((msg2->getStatus() ==  RTMPMsg::NC_CONNECT_FAILED)
            && (msg2->getMethodName() == "_result")
            && (msg2->getStreamID() == 1)
            && (msg2->size() == 1)) {
            runtest.pass("Decoded RTMP result(NC_CONNECT_FAILED(as result)");
        } else {
            runtest.fail("Decoded RTMP result(NC_CONNECT_FAILED(as result)");
        }
    } else {
        runtest.untested("Decoded RTMP result(NC_CONNECT_FAILED(as result)");
    }

    delete msg2;

    delete hex3;
    
//     Buffer hex4 = "43 00 00 00 00 00 48 14 02 00 06 5f 65 72 72 6f 72 00 40 00 00 00 00 00 00 00 05 03 00 04 63 6f 64 65 02 00 19 4e 65 74 43 6f 6e 6e 65 63 74 69 6f 6e 2e 43 61 6c 6c 2e 46 61 69 6c 65 64 00 05 6c 65 76 65 6c 02 00 05 65 72 72 6f 72 00 00 09";
//     if ((memcmp(hex4->reference(), buf4->reference(), hex4->size()) == 0)) {
//         runtest.pass("Encoded RTMP result(NC_CONNECT_FAILED(as result)");
//     } else {
//         runtest.fail("Encoded RTMP result(NC_CONNECT_FAILED(as result)");
//     }
//     delete buf4;

// onStatus
// level
//     status
//     code
//         NetStream.Play.Reset
//     description
//         Playing and resetting PD_English_Low@2001
//     details
//         PD_English_Low@2001
//     clientid
//         dsLgYohb
    Buffer *hex4 = hex2mem("02 00 08 6f 6e 53 74 61 74 75 73 00 00 00 00 00 00 00 00 00 05 03 00 05 6c 65 76 65 6c 02 00 06 73 74 61 74 75 73 00 04 63 6f 64 65 02 00 14 4e 65 74 53 74 72 65 61 6d 2e 50 6c 61 79 2e 52 65 73 65 74 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 2a 50 6c 61 79 69 6e 67 20 61 6e 64 20 72 65 73 65 74 74 69 6e 67 20 50 44 5f 45 6e 67 6c 69 73 68 5f 4c 6f 77 40 32 30 30 31 2e 00 07 64 65 74 61 69 6c 73 02 00 13 50 44 5f 45 6e 67 6c 69 73 68 5f 4c 6f 77 40 32 30 30 31 00 08 63 6c 69 65 6e 74 69 64 02 00 08 64 73 4c 67 59 6f 68 62 00 00 09");
    RTMPMsg *msg4 = rtmpserv.decodeMsgBody(hex4);
//    msg4->dump();
//    std::vector<amf::Element *> hell4 = msg4->getElements();
    if ((msg4->getStatus() ==  RTMPMsg::NS_PLAY_RESET)
        && (msg4->getMethodName() == "onStatus")
        && (msg4->getStreamID() == 0)
        && (msg4->size() == 1)) {
        runtest.pass("Encoded/Decoded RTMP onStatus(Play Reset)");
    } else {
        runtest.fail("Encoded/Decoded RTMP onStatus(Play Reset)");
    }
    delete hex4;
    delete msg4;
    
// onStatus
// code
//     NetStream
// Data.Start
        Buffer *hex5 = hex2mem("02 00 08 6f 6e 53 74 61 74 75 73 03 00 04 63 6f 64 65 02 00 14 4e 65 74 53 74 72 65 61 6d 2e 44 61 74 61 2e 53 74 61 72 74 00 00 09");
    RTMPMsg *msg5 = rtmpserv.decodeMsgBody(hex5);
//    msg5->dump();
//    cout << msg5->getStreamID() << endl;
//    std::vector<amf::Element *> hell4 = msg4->getElements();
    if ((msg5->getStatus() ==  RTMPMsg::NS_DATA_START)
        && (msg5->getMethodName() == "onStatus")
        && (msg5->getStreamID() == -1)
        && (msg5->size() == 1)) {
        runtest.pass("Encoded/Decoded RTMP onStatus(Data Start)");
    } else {
        runtest.fail("Encoded/Decoded RTMP onStatus(Data Start)");
    }
    delete hex5;
    delete msg5;

// onStatus
//     level
//     status
//     code
//         NetStream.Play.Start
//     description
//         Started playing PD_English_Low@2001
//     details
//         PD_English_Low@20..clientid...dsLgYohb.
    Buffer *hex6 = hex2mem("02 00 08 6f 6e 53 74 61 74 75 73 00 00 00 00 00 00 00 00 00 05 03 00 05 6c 65 76 65 6c 02 00 06 73 74 61 74 75 73 00 04 63 6f 64 65 02 00 14 4e 65 74 53 74 72 65 61 6d 2e 50 6c 61 79 2e 53 74 61 72 74 00 0b 64 65 73 63 72 69 70 74 69 6f 6e 02 00 24 53 74 61 72 74 65 64 20 70 6c 61 79 69 6e 67 20 50 44 5f 45 6e 67 6c 69 73 68 5f 4c 6f 77 40 32 30 30 31 2e 00 07 64 65 74 61 69 6c 73 02 00 13 50 44 5f 45 6e 67 6c 69 73 68 5f 4c 6f 77 40 32 30 30 31 00 08 63 6c 69 65 6e 74 69 64 02 00 08 64 73 4c 67 59 6f 68 62 00 00 09");
    RTMPMsg *msg6 = rtmpserv.decodeMsgBody(hex6);
//    msg6->dump();
//    std::vector<amf::Element *> hell4 = msg4->getElements();
    if ((msg6->getStatus() ==  RTMPMsg::NS_PLAY_START)
        && (msg6->getMethodName() == "onStatus")
        && (msg6->getStreamID() == 0)
        && (msg6->size() == 1)) {
        runtest.pass("Encoded/Decoded RTMP onStatus(Play Start)");
    } else {
        runtest.fail("Encoded/Decoded RTMP onStatus(Play Start)");
    }
    delete hex6;
    delete msg6;
    
#if 0

//    const char *x4 = "";
    Buffer *hex4 = hex2mem("");
    Buffer *buf4 = rtmpserv.encodeResult(RTMPMsg::NC_CONNECT_REJECTED);
    if ((memcmp(hex4->reference(), buf4->reference(), buf4->size()) == 0)) {
        runtest.pass("Encoded RTMP result(NC_CONNECT_REJECTED");
    } else {
        runtest.fail("Encoded RTMP result(NC_CONNECT_REJECTED)");
    }
    delete buf4;
    
//    const char *x5 = "";
    Buffer *hex5 = hex2mem("");
    Buffer *buf5 = rtmpserv.encodeResult(RTMPMsg::NC_CONNECT_APPSHUTDOWN);
    if ((memcmp(hex5->reference(), buf5->reference(), buf5->size()) == 0)) {
        runtest.pass("Encoded RTMP result(NC_CONNECT_APPSHUTDOWN)");
    } else {
        runtest.fail("Encoded RTMP result(NC_CONNECT_APPSHUTDOWN)");
    }
    delete buf5;
    
//    const char *x6 = "";
    Buffer *hex6 = hex2mem("");
    Buffer *buf6 = rtmpserv.encodeResult(RTMPMsg::NC_CONNECT_INVALID_APPLICATION);
    if ((memcmp(hex6->reference(), buf6->reference(), buf6->size()) == 0)) {
        runtest.pass("Encoded RTMP result(NC_CONNECT_INVALID_APPLICATION)");
    } else {
        runtest.fail("Encoded RTMP result(NC_CONNECT_INVALID_APPLICATION)");
    }
    delete buf6;
    
//    const char *x7 = "";
    Buffer *hex7 = hex2mem("");
    Buffer *buf7 = rtmpserv.encodeResult(RTMPMsg::NC_CONNECT_CLOSED);
    if ((memcmp(hex7->reference(), buf7->reference(), buf7->size()) == 0)) {
        runtest.pass("Encoded RTMP result(NC_CONNECT_INVALID_CLOSED)");
    } else {
        runtest.fail("Encoded RTMP result(NC_CONNECT_INVALID_CLOSED)");
    }
    delete buf7;
#endif

}

void
test_types()
{
    GNASH_REPORT_FUNCTION;
    RTMP rtmp;

    const char *x = "06 00 d2 04 00 00 00 00";
    Buffer *buf1 = hex2mem(x);

    delete buf1;
}

void
test_client()
{
    GNASH_REPORT_FUNCTION;
    RTMPClient rtmp;

    Buffer *buf1 = hex2mem("02 00 07 63 6f 6e 6e 65 63 74 00 3f f0 00 00 00 00 00 00 03 00 03 61 70 70 02 00 0f 6d 70 33 5f 61 70 70 2f 69 64 33 74 65 73 74 00 08 66 6c 61 73 68 56 65 72 02 00 0c 4c 4e 58 20 39 2c 30 2c 33 31 2c 30 00 06 73 77 66 55 72 6c 02 00 29 68 74 74 70 3a 2f 2f 72 65 6e 61 75 6e 2e 63 6f 6d 2f 66 6c 65 78 32 2f 70 6f 73 74 73 2f 4d 50 33 54 65 73 74 2e 73 77 66 00 05 74 63 55 72 6c 02 00 21 72 74 6d 70 3a 2f 2f 72 65 6e 61 75 6e 2e 63 6f 6d 2f 6d 70 33 5f 61 70 70 2f 69 64 33 74 65 73 74 00 04 66 70 61 64 01 00 00 0b 61 75 64 69 6f 43 6f 64 65 63 73 00 40 83 38 00 00 00 00 00 00 0b 76 69 64 65 6f 43 6f 64 65 63 73 00 40 5f 00 00 00 00 00 00 00 0d 76 69 64 65 6f 46 75 6e 63 74 69 6f 6e 00 3f f0 00 00 00 00 00 00 00 07 70 61 67 65 55 72 6c 02 00 2a 68 74 74 70 3a 2f 2f 72 65 6e 61 75 6e 2e 63 6f 6d 2f 66 6c 65 78 32 2f 70 6f 73 74 73 2f 4d 50 33 54 65 73 74 2e 68 74 6d 6c 00 0e 6f 62 6a 65 63 74 45 6e 63 6f 64 69 6e 67 00 00 00 00 00 00 00 00 00 00 00 09");
    Buffer *buf2 = rtmp.encodeConnect("mp3_app/id3test", "http://renaun.com/flex2/posts/MP3Test.swf", "rtmp://renaun.com/mp3_app/id3test", 615, 124, 1, "http://renaun.com/flex2/posts/MP3Test.html");
//     cerr << hexify(buf1->begin(), buf1->size(), false) << endl;
//     cerr << hexify(buf2->begin(), buf1->size(), false) << endl;
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeConnect()");
    } else {
        runtest.fail("Encoded RTMPClient::encodeConnect()");
    }
    delete buf1;
    delete buf2;
    
    buf1 = hex2mem("02 00 04 70 6c 61 79 00 00 00 00 00 00 00 00 00 05 02 00 16 67 61 74 65 30 36 5f 74 61 62 6c 616e 5f 62 63 75 65 75 5f 30 31");
    buf2 = rtmp.encodeStreamOp(0, RTMP::STREAM_PLAY, false, "gate06_tablan_bcueu_01");
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_PLAY)");
    } else {
        runtest.fail("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_PLAY)");
    }
    delete buf1;
    delete buf2;

    buf1 = hex2mem("02 00 05 70 61 75 73 65 00 00 00 00 00 00 00 00 00 05 01 01 00 00 00 00 00 00 00 00 00");
    buf2 = rtmp.encodeStreamOp(0, RTMP::STREAM_PAUSE, true, 0);
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_PAUSE)");
    } else {
        runtest.fail("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_PAUSE)");
    }
    delete buf1;
    delete buf2;

    buf1 = hex2mem("02 00 04 73 74 6f 70 00 00 00 00 00 00 00 00 00 05 01 00");
    buf2 = rtmp.encodeStreamOp(0, RTMP::STREAM_STOP, false);
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_STOP)");
    } else {
        runtest.fail("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_STOP)");
    }
    delete buf1;
    delete buf2;

#if 0
    buf1 = hex2mem("02 00 07 70 75 62 6c 69 73 68 00 00 00 00 00 00 00 00 00 05 02 00 06 73 74 72 65 61 6d 02 00 04 6c 69 76 65 0d 00 02 ba 00 00 1a 14 02 00 00 00 02 00");
    buf2 = rtmp.encodeStreamOp(0, RTMP::STREAM_PUBLISH, false);
    cerr << hexify(buf1->begin(), buf1->size(), false) << endl;
    cerr << hexify(buf2->begin(), buf1->size(), false) << endl;
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_PUBLISH)");
    } else {
        runtest.fail("Encoded RTMPClient::encodeStreamOp(RTMP::STREAM_PUBLISH)");
    }
    delete buf1;
    delete buf2;
#endif
    
    buf1 = hex2mem("02 00 04 73 65 65 6b 00 00 00 00 00 00 00 00 00 05 00 00 00 00 00 00 00 00 00");
    buf2 = rtmp.encodeStreamOp(0, RTMP::STREAM_SEEK, false);
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeStream(RTMP::SEEK)");
    } else {
        runtest.fail("Encoded RTMPClient::encodeStream(RTMP::SEEK)");
    }
    delete buf1;
    delete buf2;

    buf1 = hex2mem("02 00 04 73 65 65 6b 00 00 00 00 00 00 00 00 00 05 00 40 c7 70 00 00 00 00 00");
    buf2 = rtmp.encodeStreamOp(0, RTMP::STREAM_SEEK, false, 12000);
    if ((memcmp(buf1->reference(), buf2->reference(), buf1->size()) == 0)) {
        runtest.pass("Encoded RTMPClient::encodeStream(RTMP::SEEK, double)");
    } else {
        runtest.fail("Encoded RTMPClient::encodeStream(RTMP::SEEK, double)");
    }
    delete buf1;
    delete buf2;
}

// FLV data header
//
// onMetaData
// duration
// width
// height
// videodatarate
// framerate
// videocodecid
// audiodatarate
// audiodelay
// audiocodecid
// canSeek
// ToEnd
//"02 00 0a 6 6e 4d 65 74 61 44 61 74 61 08 00 00 00 0a 00 08 64 75 72 61 74 69 6f 6e 00 40 ad 04 14 7a e1 47 ae 00 05 77 69 64 74 68 00 40 74 00 00 00 00 00 00 00 06 68 65 69 67 68 74 00 40 6e 00 00 00 00 00 00 0d 76 69 64 65 6f 64 61 74 61 72 61 74 65 00 40 72 c0 00 00 00 00 00 00 09 66 72 61 6d 65 72 61 74 65 00 40 39 00 00 00 00 00 00 00 0c 76 69 64 65 6f 63 6f 64 65 63 69 64 00 40 10 00 00 00 00 00 00 00 0d 61 75 64 69 6f 64 61 74 61 72 61 74 65 00 40 58 00 00 00 00 00 00 00 0a 61 75 64 69 6f 64 65 6c 61 79 00 3f a3 74 bc 6a 7e f9 db 00 0c 61 75 64 69 6f 63 6f 64 65 63 69 64 00 40 00 00 00 00 00 00 00 00 0c 63 61 6e 53 65 65 6b 54 6f 45 6e 64 01 01 00 00 09"

static void
usage (void)
{
    cerr << "This program tests SOL support in the AMF library." << endl;
    cerr << "Usage: test_sol [hv]" << endl;
    cerr << "-h\tHelp" << endl;
    cerr << "-v\tVerbose" << endl;
    exit (-1);
}

#else

int
main(int /*argc*/, char /* *argv[]*/)
{
  // nop
  return 0;  
}

#endif

// 03 00 00 04 00 01 1f 14 00 00 00 00 02 00 07 63   ...............c
// 6f 6e 6e 65 63 74 00 3f f0 00 00 00 00 00 00 03   onnect.?........
// 00 03 61 70 70 02 00 08 6f 66 6c 61 44 65 6d 6f   ..app...oflaDemo
// 00 08 66 6c 61 73 68 56 65 72 02 00 0c 4c 4e 58   ..flashVer...LNX
// 20 39 2c 30 2c 33 31 2c 30 00 06 73 77 66 55 72    9,0,31,0..swfUr
// 6c 02 00 33 68 74 74 70 3a 2f 2f 6c 6f 63 61 6c   l..3http://local
// 68 6f 73 74 2f 73 6f 66 74 77 61 72 65 2f 67 6e   host/software/gn
// 61 73 68 2f 74 65 73 74 73 2f 6f 66 6c 61 5f 64   ash/tests/ofla_d
// 65 6d 6f 2e 73 77 66 00 05 74 63 55 c3 72 6c 02   emo.swf..tcU.rl.
// 00 19 72 74 6d 70 3a 2f 2f 6c 6f 63 61 6c 68 6f   ..rtmp://localho
// 73 74 2f 6f 66 6c 61 44 65 6d 6f 00 04 66 70 61   st/oflaDemo..fpa
// 64 01 00 00 0b 61 75 64 69 6f 43 6f 64 65 63 73   d....audioCodecs
// 00 40 83 38 00 00 00 00 00 00 0b 76 69 64 65 6f   .@.8.......video
// 43 6f 64 65 63 73 00 40 5f 00 00 00 00 00 00 00   Codecs.@_.......
// 0d 76 69 64 65 6f 46 75 6e 63 74 69 6f 6e 00 3f   .videoFunction.?
// f0 00 00 00 00 00 00 00 07 70 61 67 65 55 72 6c   .........pageUrl
// 02 00 26 68 74 74 70 3a 2f 2f 6c 6f 63 c3 61 6c   ..&http://loc.al
// 68 6f 73 74 2f 73 6f 66 74 77 61 72 65 2f 67 6e   host/software/gn
// 61 73 68 2f 74 65 73 74 73 2f 00 00 09

    
