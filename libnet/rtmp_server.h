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

#ifndef _RTMP_SERVER_H_
#define _RTMP_SERVER_H_ 1

#include <boost/cstdint.hpp>
#include <string>
#include <map>

#include "rtmp.h"
#include "amf.h"
#include "handler.h"
#include "network.h"
#include "buffer.h"

namespace gnash
{

class DSOEXPORT RTMPServer : public RTMP
{
public:
    RTMPServer();
    ~RTMPServer();
    bool handShakeWait();
    bool handShakeResponse();
    bool serverFinish();
    bool packetSend(amf::Buffer *buf);
    bool packetRead(amf::Buffer *buf);
    
    // These are handlers for the various types
#if 0
    amf::Buffer *encodeChunkSize();
    void decodeChunkSize();
    
    amf::Buffer *encodeBytesRead();
    void decodeBytesRead();
    
    amf::Buffer *encodeServer();
    void decodeServer();
    
    amf::Buffer *encodeClient();
    void decodeClient();
    
    amf::Buffer *encodeAudioData();
    void decodeAudioData();
    
    amf::Buffer *encodeVideoData();
    void decodeVideoData();
    
    amf::Buffer *encodeNotify();
    void decodeNotify();
    
    amf::Buffer *encodeSharedObj();
    void decodeSharedObj();
    
    amf::Buffer *encodeInvoke();
    void decodeInvoke();
#endif
    
    void dump();
  private:
};

// This is the thread for all incoming RTMP connections
void rtmp_handler(Handler::thread_params_t *args);

} // end of gnash namespace
// end of _RTMP_SERVER_H_
#endif

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

