// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_DEJAGNU_H

#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <log.h>
#include <iostream>
#include <string>

#include "dejagnu.h"


#include "amf.h"

using namespace amf;
using namespace gnash;
using namespace std;

static void usage (void);

static int verbosity;

static TestState runtest;

int
main(int argc, char *argv[])
{
    bool dump = false;
    char buffer[300];
    int c, retries = 3;

    memset(buffer, 0, 300);
    
    while ((c = getopt (argc, argv, "hdvsm:")) != -1) {
        switch (c) {
          case 'h':
            usage ();
            break;
            
          case 'v':
            verbosity++;
            break;
            
          default:
            usage ();
            break;
        }
    }
    
    AMF amf_obj;
    int fd, ret;
    char buf[AMF_NUMBER_SIZE+1];
    amfnum_t value = 0xf03fL;
    amfnum_t *num;
    
    memset(buf, 0, AMF_NUMBER_SIZE+1);
    string filespec = SRCDIR;
    filespec += "/f03f.amf";
    fd = open(filespec.c_str(), O_RDONLY);
    ret = read(fd, buf, 12);
    close(fd);

    num = amf_obj.extractNumber(buf);

//     unsigned char hexint[32];
//     hexify((unsigned char *)hexint, (unsigned char *)num, 8, false);
//     cerr << "AMF number is: 0x" << hexint << endl;
//     hexify((unsigned char *)hexint, (unsigned char *)&value, 8, false);
//     cerr << "AMF value is: 0x" << hexint << endl;

    if (((char *)num)[7] == 0x3f) {
//    if (memcmp(num, &value, AMF_NUMBER_SIZE) == 0) {
        runtest.pass("Extracted Number AMF object");
    } else {
        runtest.fail("Extracted Number AMF object");
    }

    void *out = amf_obj.encodeNumber(*num);
//     hexify((unsigned char *)hexint, (unsigned char *)out, 9, false);
//     cerr << "AMF encoded number is: 0x" << hexint << endl;

//     hexify((unsigned char *)hexint, (unsigned char *)buf, 9, false);
//     cerr << "AMF buff number is: 0x" << hexint << endl;

    if (memcmp(out, buf, 9) == 0) {
        runtest.pass("Encoded AMF Number");
    } else {
        runtest.fail("Encoded AMF Number");
    }

    delete num;
}
static void
usage (void)
{
    cerr << "This program tests Number support in the AMF library." << endl;
    cerr << "Usage: test_number [hv]" << endl;
    cerr << "-h\tHelp" << endl;
    cerr << "-v\tVerbose" << endl;
    exit (-1);
}

#endif
