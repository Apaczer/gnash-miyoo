// 
//   Copyright (C) 2006, 2007, 2008 Free Software Foundation, Inc.
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

#ifndef _RTMP_H_
#define _RTMP_H_

#include <boost/cstdint.hpp>
#include <string>
#include <vector>

#include "amf.h"
#include "element.h"
#include "handler.h"
#include "network.h"
#include "buffer.h"
#include "rtmp_msg.h"

namespace gnash
{

const Network::byte_t RTMP_HANDSHAKE = 0x3;
const int  RTMP_BODY_SIZE = 1536;
const int  MAX_AMF_INDEXES = 64;

const int  RTMP_HEADSIZE_MASK = 0xc0;
const char RTMP_INDEX_MASK = 0x3f;
const int  RTMP_VIDEO_PACKET_SIZE = 128;
const int  RTMP_AUDIO_PACKET_SIZE = 64;
const int  RTMP_MAX_HEADER_SIZE = 12;
const int  PING_MSG_SIZE = 6;

// For terminating sequences, a byte with value 0x09 is used.
const char TERMINATOR = 0x09;

// Each packet consists of the following:
//
// The first byte of the AMF file/stream is believed to be a version
// indicator. So far the only valid value for this field that has been
// found is 0x00. If it is anything other than 0x00 (zero), your
// system should consider the AMF file/stream to be
// 'malformed'd. This can happen in the IDE if AMF calls are put
// on the stack but never executed and the user exits the movie from the
// IDE; the two top bytes will be random and the number of headers will
// be unreliable.

// The third and fourth bytes form an integer value that specifies the
// number of headers.
typedef struct {
    gnash::Network::byte_t version;
    gnash::Network::byte_t source;
    boost::uint32_t  count;
} amfpacket_t;

typedef enum {
    onStatus,
    onResult,
    onDebugEvents
} amfresponse_e;

class DSOEXPORT RTMP : public Network
{
public:
    typedef enum {
	RAW=0x0,
	ADPCM=0x01,
	MP3=0x02,
	NELLYMOSER_8khz=0x05,
	NEYYNOSER=0x6
    } audiocodecs_e;
    typedef enum {
	H263=0x2,
	SCREEN0x3,
	VP6=0x4
    } videocodecs_e;
    // The second byte of the AMF file/stream is appears to be 0x00 if the
    // client is the Flash Player and 0x01 if the client is the FlashCom
    // server.
    typedef enum {
        NONE = 0x0,
        CHUNK_SIZE = 0x1,
        UNKNOWN = 0x2,
        BYTES_READ = 0x3,
        PING = 0x4,
        SERVER = 0x5,
        CLIENT = 0x6,
        UNKNOWN2 = 0x7,
        AUDIO_DATA = 0x8,
        VIDEO_DATA = 0x9,
        UNKNOWN3 = 0xa,
        NOTIFY = 0x12,
        SHARED_OBJ = 0x13,
        INVOKE = 0x14,
	FLV_DATA = 0x16
    } content_types_e;
//     typedef enum {
//         CONNECT = 0x1,
//         DISCONNECT = 0x2,
//         SET_ATTRIBUTE = 0x3,
//         UPDATE_DATA = 0x4,
//         UPDATE_ATTRIBUTE = 0x5,
//         SEND_MESSAGE = 0x6,
//         STATUS = 0x7,
//         CLEAR_DATA = 0x8,
//         DELETE_DATA = 0x9,
//         DELETE_ATTRIBUTE = 0xa,
//         INITIAL_DATA = 0xb
//     } sharedobj_types_e;
    typedef enum {
	PING_CLEAR  = 0x0,	// clear the stream
	PING_PLAY   = 0x1,	// clear the playing buffer
	PING_TIME   = 0x3,	// Buffer time in milliseconds
	PING_RESET  = 0x4,	// Reset stream
	PING_CLIENT = 0x6,	// Ping the client from the server
	PONG_CLIENT = 0x7	// pong reply from client to server
    } rtmp_ping_e;
    typedef enum {
	STREAM_PLAY,		// play the existing stream
	STREAM_PAUSE,		// pause the existing stream
	STREAM_PUBLISH,		// publish the existing stream
	STREAM_STOP,		// stop the existing stream
	STREAM_SEEK		// seek in the existing stream
    } rtmp_op_e;
    typedef struct {
	rtmp_ping_e type;	// the type of the ping message
	boost::uint16_t target; // all Ping message data fields
	boost::uint16_t param1; // are 2 bytes long
	boost::uint16_t param2;
	boost::uint16_t param3;
    } rtmp_ping_t;
    typedef enum {
        RTMP_STATE_HANDSHAKE_SEND,
        RTMP_STATE_HANDSHAKE_RECV,
        RTMP_STATE_HANDSHAKE_ACK,
        RTMP_STATE_CONNECT,
        RTMP_STATE_NETCONNECT,
        RTMP_STATE_NETSTREAM,
        RTMP_STATE_HEADER,
        RTMP_STATE_DONE
    } rtmp_state_t;
//     typedef struct {
// 	rtmp_status_e status;
// 	std::string   method;
// 	double        streamid;
// 	std::vector<amf::Element *> objs;
//     } rtmp_msg_t;
    typedef enum {
        RTMP_ERR_UNDEF=0,
        RTMP_ERR_NOTFOUND,
        RTMP_ERR_PERM,
        RTMP_ERR_DISKFULL,
        RTMP_ERR_ILLEGAL,
        RTMP_ERR_UNKNOWNID,
        RTMP_ERR_EXISTS,
        RTMP_ERR_NOSUCHUSER,
        RTMP_ERR_TIMEOUT,
        RTMP_ERR_NORESPONSE
    } rtmp_error_t;

// Each header consists of the following:
// a single byte that is the index of the RTMP channel,
// then two bits that's a flag to note the size of the header,
// which can be 1, 4, 8, or 12 bytes long.
    
// More info at http://wiki.gnashdev.org/RTMP
    typedef struct {
	int             channel;
	int             head_size;
	int             bodysize;
	RTMPMsg::rtmp_source_e   src_dest;
	content_types_e type;
    } rtmp_head_t;
    typedef enum {
        HEADER_12 = 0x0,
        HEADER_8  = 0x40,
        HEADER_4  = 0x80,
        HEADER_1  = 0xc0
    } rtmp_headersize_e;    
    
// Each body consists of the following:
//
// * UTF String - Target
// * UTF String - Response
// * Long - Body length in bytes
// * Variable - Actual data (including a type code)
//     typedef struct {
//         amf::amfutf8_t target;
//         amf::amfutf8_t response;
// 	boost::uint32_t length;
//         void *data;
//     } rtmp_body_t;
    
    RTMP();
    virtual ~RTMP();

    // Decode
    rtmp_head_t *decodeHeader(gnash::Network::byte_t *header);
    rtmp_head_t *decodeHeader(amf::Buffer *data);
    amf::Buffer *encodeHeader(int amf_index, rtmp_headersize_e head_size,
			      size_t total_size, content_types_e type, RTMPMsg::rtmp_source_e routing);
    amf::Buffer *encodeHeader(int amf_index, rtmp_headersize_e head_size);
    
    bool packetSend(amf::Buffer *buf);
    bool packetRead(amf::Buffer *buf);

    void addProperty(amf::Element *el);
    void addProperty(char *name, amf::Element *el);
    void addProperty(std::string &name, amf::Element *el);
    amf::Element *getProperty(const std::string &name);
    void setHandler(Handler *hand) { _handler = hand; };
    int headerSize(gnash::Network::byte_t header);

    rtmp_head_t *getHeader()    { return &_header; };
    int getHeaderSize()         { return _header.head_size; }; 
    int getTotalSize()          { return _header.bodysize; }; 
    RTMPMsg::rtmp_source_e getRouting()  { return _header.src_dest; };
    int getChannel()            { return _header.channel; };
    int getPacketSize()         { return _packet_size; };
    int getMysteryWord()        { return _mystery_word; };

    // Decode an RTMP message
    RTMPMsg *decodeMsgBody(Network::byte_t *data, size_t size);
    RTMPMsg *decodeMsgBody(amf::Buffer *buf);
    
    virtual rtmp_ping_t *decodePing(Network::byte_t *data);
    rtmp_ping_t *decodePing(amf::Buffer *buf);
    
    // These are handlers for the various types
    virtual amf::Buffer *encodeChunkSize();
    virtual void decodeChunkSize();
    
    virtual amf::Buffer *encodeBytesRead();
    virtual void decodeBytesRead();
    virtual amf::Buffer *encodeServer();
    virtual void decodeServer();
    
    virtual amf::Buffer *encodeClient();
    virtual void decodeClient();
    
    virtual amf::Buffer *encodeAudioData();
    virtual void decodeAudioData();
    
    virtual amf::Buffer *encodeVideoData();
    virtual void decodeVideoData();
    
    virtual amf::Buffer *encodeNotify();
    virtual void decodeNotify();
    
    virtual amf::Buffer *encodeSharedObj();
    virtual void decodeSharedObj();
    
    virtual amf::Buffer *encodeInvoke();
    virtual void decodeInvoke();

    // Receive a message, which is a series of AMF elements, seperated
    // by a one byte header at regular byte intervals. (128 bytes for
    // video data by default). Each message main contain multiple packets.
    amf::Buffer *recvMsg();
    amf::Buffer *recvMsg(int timeout);

    // Send a message, usually a single ActionScript object. This message
    // may be broken down into a series of packets on a regular byte
    // interval. (128 bytes for video data by default). Each message main
    // contain multiple packets.
    bool sendMsg(amf::Buffer *data);
    
    // Send a Msg, and expect a response back of some kind.
    RTMPMsg *sendRecvMsg(int amf_index, rtmp_headersize_e head_size,
			      size_t total_size, content_types_e type,
			      RTMPMsg::rtmp_source_e routing, amf::Buffer *buf);
    // Split a large buffer into multiple smaller ones of the default chunksize
    // of 128 bytes. We read network data in big chunks because it's more efficient,
    // but RTMP uses a weird scheme of a standard header, and then every chunksize
    // bytes another 1 byte RTMP header. The header itself is not part of the byte
    // count.
    Que *split(amf::Buffer *buf);
    Que *split(amf::Buffer *buf, size_t chunksize);

    void dump();
  protected:
    std::map<const char *, amf::Element *> _properties;
    amf::Buffer	*_handshake;
    Handler	*_handler;
    rtmp_head_t	_header;
    int         _packet_size;
    int         _mystery_word;
    size_t	_chunksize;
    int		_timeout;
};

} // end of gnash namespace
// end of _RTMP_H_
#endif

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

