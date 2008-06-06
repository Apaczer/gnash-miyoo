// MediaParserFfmpeg.h: FFMPG media parsers, for Gnash
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


#ifndef __MEDIAPARSER_FFMPEG_H__
#define __MEDIAPARSER_FFMPEG_H__

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "MediaParser.h" // for inheritance

#include <vector>
#include <boost/scoped_array.hpp>
#include <memory>

#ifdef HAVE_FFMPEG_AVFORMAT_H
extern "C" {
#include <ffmpeg/avformat.h>
}
#endif

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
extern "C" {
#include <libavformat/avformat.h>
}
#endif

// Forward declaration
class tu_file;

namespace gnash {
namespace media {

/// FFMPEG based MediaParser
///
class MediaParserFfmpeg: public MediaParser
{
public:

	/// Construct a ffmpeg-based media parser for given stream
	//
	/// Can throw a GnashException if input format couldn't be detected
	///
	MediaParserFfmpeg(std::auto_ptr<tu_file> stream);

	~MediaParserFfmpeg();

	// See dox in MediaParser.h
	virtual boost::uint32_t seek(boost::uint32_t);

	// See dox in MediaParser.h
	virtual bool parseNextChunk();

	// See dox in MediaParser.h
	virtual boost::uint64_t getBytesLoaded() const;

private:

	/// Video frame cursor position 
	//
	/// This is the video frame number that will
	/// be referenced by nextVideoFrame and nextVideoFrameTimestamp
	///
	size_t _nextVideoFrame;

	/// Audio frame cursor position 
	//
	/// This is the video frame number that will
	/// be referenced by nextVideoFrame and nextVideoFrameTimestamp
	///
	size_t _nextAudioFrame;

	/// Parse next media frame
	//
	/// @return false on error or eof, true otherwise
	///
	bool parseNextFrame();

	/// Input chunk reader, to be called by ffmpeg parser
	int readPacket(boost::uint8_t* buf, int buf_size);

	/// ffmpeg callback function
	static int readPacketWrapper(void* opaque, boost::uint8_t* buf, int buf_size);

	/// Input stream seeker, to be called by ffmpeg parser
	offset_t seekMedia(offset_t offset, int whence);

	/// ffmpeg callback function
	static offset_t seekMediaWrapper(void *opaque, offset_t offset, int whence);

	/// Read some of the input to figure an AVInputFormat
	AVInputFormat* probeStream();

	AVInputFormat* _inputFmt;

	/// the format (mp3, avi, etc.)
	AVFormatContext *_formatCtx;

	/// Index of the video stream in input
	int _videoStreamIndex;

	/// Video input stream
	AVStream* _videoStream;

	/// Index of the audio stream in input
	int _audioStreamIndex;

	// audio
	AVStream* _audioStream;

	/// ?
	ByteIOContext _byteIOCxt;

	/// Size of the ByteIO context buffer
	//
	/// This seems to be the size of chunks read
	/// by av_read_frame.
	///
	static const size_t byteIOBufferSize = 1024;

	boost::scoped_array<unsigned char> _byteIOBuffer;

	/// The last parsed position, for getBytesLoaded
	boost::uint64_t _lastParsedPosition;

	/// Return sample size from SampleFormat
	//
	/// TODO: move somewhere in ffmpeg utils..
	///
	boost::uint16_t SampleFormatToSampleSize(SampleFormat fmt);

	/// Make an EncodedVideoFrame from an AVPacket and push to buffer
	//
	bool parseVideoFrame(AVPacket& packet);

	/// Make an EncodedAudioFrame from an AVPacket and push to buffer
	bool parseAudioFrame(AVPacket& packet);
};


} // gnash.media namespace 
} // namespace gnash

#endif // __MEDIAPARSER_FFMPEG_H__
