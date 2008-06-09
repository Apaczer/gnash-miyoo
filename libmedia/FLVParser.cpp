// FLVParser.cpp:  Flash Video file parser, for Gnash.
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
//


#include "FLVParser.h"
#include "amf.h"
#include "log.h"
#include "utility.h"
#include "GnashException.h"
#include "IOChannel.h"

#include <string>
#include <iosfwd>

using namespace std;

#define PADDING_BYTES 64
#define READ_CHUNKS 64

// Define the following macro the have seek() operations printed
//#define GNASH_DEBUG_SEEK 1

namespace gnash {
namespace media {

FLVParser::FLVParser(std::auto_ptr<IOChannel> lt)
	:
	MediaParser(lt),
	_lastParsedPosition(0),
	_nextAudioFrame(0),
	_nextVideoFrame(0),
	_audio(false),
	_video(false)
{
	if ( ! parseHeader() ) 
		throw GnashException("FLVParser couldn't parse header from input");
}

FLVParser::~FLVParser()
{
	// nothing to do here, all done in base class now
}


boost::uint32_t
FLVParser::seek(boost::uint32_t /*time*/)
{
	LOG_ONCE( log_unimpl("%s", __PRETTY_FUNCTION__) );

	// In particular, what to do if there's no frames in queue ?
	// just seek to the the later available first timestamp
	return 0;
}

bool
FLVParser::parseNextChunk()
{
	static const int tagsPerChunk=10;
	for (int i=0; i<tagsPerChunk; ++i)
	{
		if ( ! parseNextTag() ) return false;
	}
	return true;
}

bool FLVParser::parseNextTag()
{
	if ( _parsingComplete ) return false;

	// Seek to next frame and skip the size of the last tag
	if ( _stream->seek(_lastParsedPosition+4) )
	{
		log_error("FLVParser::parseNextTag: can't seek to %d", _lastParsedPosition+4);
		_parsingComplete=true;
		return false;
	}

	// Read the tag info
	boost::uint8_t tag[12];
	int actuallyRead = _stream->read(tag, 12);
	if ( actuallyRead < 12 )
	{
		if ( actuallyRead )
			log_error("FLVParser::parseNextTag: can't read tag info (needed 12 bytes, only got %d)", actuallyRead);
		// else { assert(_stream->eof(); } ?
		_parsingComplete=true;
		return false;
	}

	// Extract length and timestamp
	boost::uint32_t bodyLength = getUInt24(&tag[1]);
	boost::uint32_t timestamp = getUInt24(&tag[4]);

	_lastParsedPosition += 15 + bodyLength;

	// check for empty tag
	if (bodyLength == 0) return true;

	enum tagType
	{
		AUDIO_TAG = 0x08,
		VIDEO_TAG = 0x09,
		META_TAG = 0x12
	};

	if (tag[0] == AUDIO_TAG)
	{
		std::auto_ptr<EncodedAudioFrame> frame = readAudioFrame(bodyLength-1, timestamp);
		if ( ! frame.get() ) { log_error("could not read audio frame?"); }
		else _audioFrames.push_back(frame.release());

		// If this is the first audioframe no info about the
		// audio format has been noted, so we do that now
		if ( !_audioInfo.get() )
		{
			int samplerate = (tag[11] & 0x0C) >> 2;
			if (samplerate == 0) samplerate = 5500;
			else if (samplerate == 1) samplerate = 11000;
			else if (samplerate == 2) samplerate = 22050;
			else if (samplerate == 3) samplerate = 44100;

			int samplesize = (tag[11] & 0x02) >> 1;
			if (samplesize == 0) samplesize = 1;
			else samplesize = 2;

			_audioInfo.reset( new AudioInfo((tag[11] & 0xf0) >> 4, samplerate, samplesize, (tag[11] & 0x01) >> 0, 0, FLASH) );
		}


	}
	else if (tag[0] == VIDEO_TAG)
	{
		bool isKeyFrame = (tag[11] & 0xf0) >> 4;
		UNUSED(isKeyFrame); // may be used for building seekable indexes...

		size_t dataPosition = _stream->tell();

		std::auto_ptr<EncodedVideoFrame> frame = readVideoFrame(bodyLength-1, timestamp);
		if ( ! frame.get() ) { log_error("could not read video frame?"); }
		else _videoFrames.push_back(frame.release());

		// If this is the first videoframe no info about the
		// video format has been noted, so we do that now
		if ( ! _videoInfo.get() )
		{
			boost::uint16_t codec = (tag[11] & 0x0f) >> 0;
			// Set standard guessed size...
			boost::uint16_t width = 320;
			boost::uint16_t height = 240;

			// Extract the video size from the videodata header
			if (codec == VIDEO_CODEC_H263) {

				// We're going to re-read some data here
				// (can likely avoid with a better cleanup)

				size_t bkpos = _stream->tell();
				if ( _stream->seek(dataPosition) ) {
					log_error(" Couldn't seek to VideoTag data position");
					_parsingComplete=true;
					return false;
				}
				boost::uint8_t videohead[12];

				int actuallyRead = _stream->read(videohead, 12);
				_stream->seek(bkpos); // rewind

				if ( actuallyRead < 12 )
				{
		log_error("FLVParser::parseNextTag: can't read H263 video header (needed 12 bytes, only got %d)", actuallyRead);
		_parsingComplete=true;
		return false;
				}

				bool sizebit1 = (videohead[3] & 0x02);
				bool sizebit2 = (videohead[3] & 0x01);
				bool sizebit3 = (videohead[4] & 0x80);

				// First some predefined sizes
				if (!sizebit1 && sizebit2 && !sizebit3 ) {
					width = 352;
					height = 288;
				} else if (!sizebit1 && sizebit2 && sizebit3 ) {
					width = 176;
					height = 144;
				} else if (sizebit1 && !sizebit2 && !sizebit3 ) {
					width = 128;
					height = 96;
				} else if (sizebit1 && !sizebit2 && sizebit3 ) {
					width = 320;
					height = 240;
				} else if (sizebit1 && sizebit2 && !sizebit3 ) {
					width = 160;
					height = 120;

				// Then the custom sizes (1 byte - untested and ugly)
				} else if (!sizebit1 && !sizebit2 && !sizebit3 ) {
					width = (videohead[4] & 0x40) | (videohead[4] & 0x20) | (videohead[4] & 0x20) | (videohead[4] & 0x08) | (videohead[4] & 0x04) | (videohead[4] & 0x02) | (videohead[4] & 0x01) | (videohead[5] & 0x80);

					height = (videohead[5] & 0x40) | (videohead[5] & 0x20) | (videohead[5] & 0x20) | (videohead[5] & 0x08) | (videohead[5] & 0x04) | (videohead[5] & 0x02) | (videohead[5] & 0x01) | (videohead[6] & 0x80);

				// Then the custom sizes (2 byte - untested and ugly)
				} else if (!sizebit1 && !sizebit2 && sizebit3 ) {
					width = (videohead[4] & 0x40) | (videohead[4] & 0x20) | (videohead[4] & 0x20) | (videohead[4] & 0x08) | (videohead[4] & 0x04) | (videohead[4] & 0x02) | (videohead[4] & 0x01) | (videohead[5] & 0x80) | (videohead[5] & 0x40) | (videohead[5] & 0x20) | (videohead[5] & 0x20) | (videohead[5] & 0x08) | (videohead[5] & 0x04) | (videohead[5] & 0x02) | (videohead[5] & 0x01) | (videohead[6] & 0x80);

					height = (videohead[6] & 0x40) | (videohead[6] & 0x20) | (videohead[6] & 0x20) | (videohead[6] & 0x08) | (videohead[6] & 0x04) | (videohead[6] & 0x02) | (videohead[6] & 0x01) | (videohead[7] & 0x80) | (videohead[7] & 0x40) | (videohead[7] & 0x20) | (videohead[7] & 0x20) | (videohead[7] & 0x08) | (videohead[7] & 0x04) | (videohead[7] & 0x02) | (videohead[7] & 0x01) | (videohead[8] & 0x80);
				}

			}

			// Create the videoinfo
			_videoInfo.reset( new VideoInfo(codec, width, height, 0 /*frameRate*/, 0 /*duration*/, FLASH /*codec type*/) );
		}

	} else if (tag[0] == META_TAG) {
		LOG_ONCE( log_unimpl("FLV MetaTag parser") );
		// Extract information from the meta tag
		/*_stream->seek(_lastParsedPosition+16);
		char* metaTag = new char[bodyLength];
		size_t actuallyRead = _stream->read(metaTag, bodyLength);
		if ( actuallyRead < bodyLength )
		{
			log_error("FLVParser::parseNextTag: can't read metaTag (%d) body (needed %d bytes, only got %d)",
				META_TAG, bodyLength, actuallyRead);
			_parsingComplete=true;
			return false;
		}
		amf::AMF* amfParser = new amf::AMF();
		amfParser->parseAMF(metaTag);*/

	} else {
		log_error("Unknown FLV tag type %d", tag[0]);
		//_parsingComplete = true;
		//return false;
	}

	return true;
}

bool FLVParser::parseHeader()
{
	// seek to the begining of the file
	_stream->seek(0); // seek back ? really ?

	// Read the header
	boost::uint8_t header[9];
	if ( _stream->read(header, 9) != 9 )
	{
		log_error("FLVParser::parseHeader: couldn't read 9 bytes of header");
		return false;
	}
	_lastParsedPosition = 9;

	// Check if this is really a FLV file
	if (header[0] != 'F' || header[1] != 'L' || header[2] != 'V') return false;

	int version = header[3];

	// Parse the audio+video bitmask
	_audio = header[4]&(1<<2);
	_video = header[4]&(1<<0);

	log_debug("Parsing FLV version %d, audio:%d, video:%d", version, _audio, _video);

	// Make sure we initialize audio/video info (if any)
	while ( !_parsingComplete && (_video && !_videoInfo.get()) || (_audio && !_audioInfo.get()) )
	{
		parseNextTag();
	}

	if ( _video && !_videoInfo.get() )
		log_error(" couldn't find any video frame in an FLV advertising video in header");

	if ( _audio && !_audioInfo.get() )
		log_error(" couldn't find any audio frame in an FLV advertising audio in header");

	return true;
}

inline boost::uint32_t FLVParser::getUInt24(boost::uint8_t* in)
{
	// The bits are in big endian order
	return (in[0] << 16) | (in[1] << 8) | in[2];
}

boost::uint64_t
FLVParser::getBytesLoaded() const
{
	return _lastParsedPosition;
}

/*private*/
std::auto_ptr<EncodedAudioFrame>
FLVParser::readAudioFrame(boost::uint32_t dataSize, boost::uint32_t timestamp)
{
	IOChannel& in = *_stream;

	//log_debug("Reading the %dth audio frame, with data size %d, from position %d", _audioFrames.size()+1, dataSize, in.tell());

	std::auto_ptr<EncodedAudioFrame> frame ( new EncodedAudioFrame );
	frame->dataSize = dataSize;
	frame->timestamp = timestamp;

	unsigned long int chunkSize = smallestMultipleContaining(READ_CHUNKS, dataSize+PADDING_BYTES);

	frame->data.reset( new boost::uint8_t[chunkSize] );
	size_t bytesread = in.read(frame->data.get(), dataSize);
	if ( bytesread < dataSize )
	{
		log_error("FLVParser::readAudioFrame: could only read %d/%d bytes", bytesread, dataSize);
	}

	unsigned long int padding = chunkSize-dataSize;
	assert(padding);
	memset(frame->data.get() + bytesread, 0, padding);

	return frame;
}

/*private*/
std::auto_ptr<EncodedVideoFrame>
FLVParser::readVideoFrame(boost::uint32_t dataSize, boost::uint32_t timestamp)
{
	IOChannel& in = *_stream;

	std::auto_ptr<EncodedVideoFrame> frame;

	unsigned long int chunkSize = smallestMultipleContaining(READ_CHUNKS, dataSize+PADDING_BYTES);

	boost::uint8_t* data = new boost::uint8_t[chunkSize];
	size_t bytesread = in.read(data, dataSize);

	unsigned long int padding = chunkSize-dataSize;
	assert(padding);
	memset(data + bytesread, 0, padding);

	// We won't need frameNum, so will set to zero...
	// TODO: fix this ?
	// NOTE: ownership of 'data' is transferred here
	frame.reset( new EncodedVideoFrame(data, dataSize, 0, timestamp) );
	return frame;
}

} // end of gnash::media namespace
} // end of gnash namespace

#undef PADDING_BYTES
#undef READ_CHUNKS 
