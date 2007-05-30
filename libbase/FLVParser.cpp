// FLVParser.cpp:  Flash Video file parser, for Gnash.
//
//   Copyright (C) 2007 Free Software Foundation, Inc.
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
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

// $Id: FLVParser.cpp,v 1.17 2007/05/30 09:52:46 strk Exp $

#include "FLVParser.h"
#include "amf.h"
#include "log.h"

#define PADDING_BYTES 8


FLVParser::FLVParser()
	:
	_lt(NULL),
	_lastParsedPosition(0),
	_parsingComplete(false),
	_videoInfo(NULL),
	_audioInfo(NULL),
	_nextAudioFrame(0),
	_nextVideoFrame(0),
	_audio(false),
	_video(false)
{
}

FLVParser::~FLVParser()
{
	_videoFrames.clear();

	_audioFrames.clear();
}


uint32_t FLVParser::getBufferLength()
{
	boost::mutex::scoped_lock lock(_mutex);

	if (_video) {
		size_t size = _videoFrames.size();
		if (size > 1 && size > _nextVideoFrame) {
			return _videoFrames.back()->timestamp - _videoFrames[_nextVideoFrame]->timestamp;
		}
	}
	if (_audio) {
		size_t size = _audioFrames.size();
		if (size > 1 && size > _nextAudioFrame) {
			return _audioFrames.back()->timestamp - _audioFrames[_nextAudioFrame]->timestamp;
		}
	}
	return 0;
}
uint16_t FLVParser::videoFrameRate()
{
	boost::mutex::scoped_lock lock(_mutex);

	// Make sure that there are parsed some frames
	while(_videoFrames.size() < 2 && !_parsingComplete) {
		parseNextFrame();
	}

	if (_videoFrames.size() < 2) return 0;

 	uint32_t framedelay = _videoFrames[1]->timestamp - _videoFrames[0]->timestamp;

	return static_cast<int16_t>(1000 / framedelay);
}


uint32_t FLVParser::videoFrameDelay()
{
	boost::mutex::scoped_lock lock(_mutex);

	// If there are no video in this FLV return 0
	if (!_video && _lastParsedPosition > 0) return 0;

	// Make sure that there are parsed some frames
	while(_videoFrames.size() < 2 && !_parsingComplete) {
		parseNextFrame();
	}

	// If there is no video data return 0
	if (_videoFrames.size() == 0 || !_video || _nextVideoFrame < 2) return 0;

	return _videoFrames[_nextVideoFrame-1]->timestamp - _videoFrames[_nextVideoFrame-2]->timestamp;
}

uint32_t FLVParser::audioFrameDelay()
{
	boost::mutex::scoped_lock lock(_mutex);

	// If there are no audio in this FLV return 0
	if (!_audio && _lastParsedPosition > 0) return 0;

	// Make sure that there are parsed some frames
	while(_audioFrames.size() < 2 && !_parsingComplete) {
		parseNextFrame();
	}

	// If there is no video data return 0
	if (_audioFrames.size() == 0 || !_audio || _nextAudioFrame < 2) return 0;

	return _audioFrames[_nextAudioFrame-1]->timestamp - _audioFrames[_nextAudioFrame-2]->timestamp;
}

FLVFrame* FLVParser::nextMediaFrame()
{
	boost::mutex::scoped_lock lock(_mutex);

	uint32_t video_size = _videoFrames.size();
	uint32_t audio_size = _audioFrames.size();

	if ( ! (audio_size <= _nextAudioFrame && video_size <= _nextVideoFrame) )
	{

		// Parse a media frame if any left or if needed
		while(video_size == _videoFrames.size() && audio_size == _audioFrames.size() && !_parsingComplete) {
			if (!parseNextFrame()) break;
		}
	}

	// Find the next frame in the file
	bool audioReady = _audioFrames.size() > _nextAudioFrame;
	bool videoReady = _videoFrames.size() > _nextVideoFrame;
	bool useAudio = false;

	if (audioReady && videoReady) {
		useAudio = _audioFrames[_nextAudioFrame]->dataPosition < _videoFrames[_nextVideoFrame]->dataPosition;
	} else if (!audioReady && videoReady) {
		useAudio = false;
	} else if (audioReady && !videoReady) {
		useAudio = true;
	} else {
		// If no frames are next we have reached EOF
		return NULL;
	}

	// Find the next frame in the file a return it

	if (useAudio) {

		FLVFrame* frame = new FLVFrame;
		frame->dataSize = _audioFrames[_nextAudioFrame]->dataSize;
		frame->timestamp = _audioFrames[_nextAudioFrame]->timestamp;

		_lt->seek(_audioFrames[_nextAudioFrame]->dataPosition);
		frame->data = new uint8_t[frame->dataSize + PADDING_BYTES];
		size_t bytesread = _lt->read(frame->data, frame->dataSize);
		memset(frame->data + bytesread, 0, PADDING_BYTES);

		frame->tag = 8;
		_nextAudioFrame++;
		return frame;

	} else {
		FLVFrame* frame = new FLVFrame;
		frame->dataSize = _videoFrames[_nextVideoFrame]->dataSize;
		frame->timestamp = _videoFrames[_nextVideoFrame]->timestamp;

		_lt->seek(_videoFrames[_nextVideoFrame]->dataPosition);
		frame->data = new uint8_t[frame->dataSize + PADDING_BYTES];
		size_t bytesread  = _lt->read(frame->data, frame->dataSize);
		memset(frame->data + bytesread, 0, PADDING_BYTES);

		frame->tag = 9;
		_nextVideoFrame++;
		return frame;
	}


}

FLVFrame* FLVParser::nextAudioFrame()
{
	boost::mutex::scoped_lock lock(_mutex);

	// If there are no audio in this FLV return NULL
	if (!_audio && _lastParsedPosition > 0) return NULL;

	// Make sure that there are parsed enough frames to return the need frame
	while(_audioFrames.size() <= _nextAudioFrame && !_parsingComplete) {
		if (!parseNextFrame()) break;
	}

	// If the needed frame can't be parsed (EOF reached) return NULL
	if (_audioFrames.size() <= _nextAudioFrame || _audioFrames.size() == 0) return NULL;

	FLVFrame* frame = new FLVFrame;
	frame->dataSize = _audioFrames[_nextAudioFrame]->dataSize;
	frame->timestamp = _audioFrames[_nextAudioFrame]->timestamp;
	frame->tag = 8;

	_lt->seek(_audioFrames[_nextAudioFrame]->dataPosition);
	frame->data = new uint8_t[_audioFrames[_nextAudioFrame]->dataSize +
				  PADDING_BYTES];
	size_t bytesread = _lt->read(frame->data, 
				_audioFrames[_nextAudioFrame]->dataSize);
	memset(frame->data + bytesread, 0, PADDING_BYTES);

	_nextAudioFrame++;
	return frame;

}

FLVFrame* FLVParser::nextVideoFrame()
{
	boost::mutex::scoped_lock lock(_mutex);

	// If there are no video in this FLV return NULL
	if (!_video && _lastParsedPosition > 0)
	{
		//gnash::log_debug("no video, or lastParserPosition > 0");
		return NULL;
	}

	// Make sure that there are parsed enough frames to return the need frame
	while(_videoFrames.size() <= static_cast<uint32_t>(_nextVideoFrame) && !_parsingComplete)
	{
		if (!parseNextFrame()) break;
	}

	// If the needed frame can't be parsed (EOF reached) return NULL
	if (_videoFrames.size() <= _nextVideoFrame || _videoFrames.size() == 0)
	{
		//gnash::log_debug("The needed frame (%d) can't be parsed (EOF reached)", _lastVideoFrame);
		return NULL;
	}


	FLVFrame* frame = new FLVFrame;
	frame->dataSize = _videoFrames[_nextVideoFrame]->dataSize;
	frame->timestamp = _videoFrames[_nextVideoFrame]->timestamp;
	frame->tag = 9;

	_lt->seek(_videoFrames[_nextVideoFrame]->dataPosition);
	frame->data = new uint8_t[_videoFrames[_nextVideoFrame]->dataSize + 
				  PADDING_BYTES];
	size_t bytesread = _lt->read(frame->data, 
				_videoFrames[_nextVideoFrame]->dataSize);
	memset(frame->data + bytesread, 0, PADDING_BYTES);

	_nextVideoFrame++;
	return frame;
}


uint32_t FLVParser::seekAudio(uint32_t time)
{

	// Make sure that there are parsed some frames
	while(_audioFrames.size() < 1 && !_parsingComplete) {
		parseNextFrame();
	}

	// If there is no audio data return NULL
	if (_audioFrames.size() == 0) return 0;

	// Make sure that there are parsed some enough frames
	// to get the right frame.
	while(_audioFrames.back()->timestamp < time && !_parsingComplete) {
		parseNextFrame();
	}

	// If there are no audio greater than the given time
	// the last audioframe is returned
	FLVAudioFrame* lastFrame = _audioFrames.back();
	if (lastFrame->timestamp < time) {
		_nextAudioFrame = _audioFrames.size() - 1;
		return lastFrame->timestamp;
	}

	// We try to guess where in the vector the audioframe
	// with the correct timestamp is
	size_t numFrames = _audioFrames.size();
	double tpf = lastFrame->timestamp / numFrames; // time per frame
	size_t guess = size_t(time / tpf);

	// Here we test if the guess was ok, and adjust if needed.
	size_t bestFrame = iclamp(guess, 0, _audioFrames.size()-1);

	// Here we test if the guess was ok, and adjust if needed.
	long diff = _audioFrames[bestFrame]->timestamp - time;
	if ( diff > 0 ) // our guess was too long
	{
		while ( bestFrame > 0 && _audioFrames[bestFrame-1]->timestamp > time ) --bestFrame;
	}
	else // our guess was too short
	{
		while ( bestFrame < _audioFrames.size()-1 && _audioFrames[bestFrame+1]->timestamp < time ) ++bestFrame;
	}

	gnash::log_debug("Seek (audio): " SIZET_FMT "/" SIZET_FMT " (%u/%u)", bestFrame, numFrames, _audioFrames[bestFrame]->timestamp, time);
	_nextAudioFrame = bestFrame;
	return _audioFrames[bestFrame]->timestamp;

}


uint32_t FLVParser::seekVideo(uint32_t time)
{
	// Make sure that there are parsed some frames
	while(_videoFrames.size() < 1 && !_parsingComplete) {
		parseNextFrame();
	}

	// If there is no video data return NULL
	if (_videoFrames.size() == 0) return 0;

	// Make sure that there are parsed some enough frames
	// to get the right frame.
	while(_videoFrames.back()->timestamp < time && !_parsingComplete) {
		parseNextFrame();
	}

	// If there are no videoframe greater than the given time
	// the last key videoframe is returned
	FLVVideoFrame* lastFrame = _videoFrames.back();
	size_t numFrames = _videoFrames.size();
	if (lastFrame->timestamp < time)
	{
		size_t lastFrameNum = numFrames -1;
		while (! lastFrame->isKeyFrame() )
		{
			lastFrameNum--;
			lastFrame = _videoFrames[lastFrameNum];
		}

		_nextVideoFrame = lastFrameNum;
		return lastFrame->timestamp;

	}

	// We try to guess where in the vector the videoframe
	// with the correct timestamp is
	double tpf = lastFrame->timestamp / numFrames; // time per frame
	size_t guess = size_t(time / tpf);

	size_t bestFrame = iclamp(guess, 0, _videoFrames.size()-1);

	// Here we test if the guess was ok, and adjust if needed.
	long diff = _videoFrames[bestFrame]->timestamp - time;
	if ( diff > 0 ) // our guess was too long
	{
		while ( bestFrame > 0 && _videoFrames[bestFrame-1]->timestamp > time ) --bestFrame;
	}
	else // our guess was too short
	{
		while ( bestFrame < _videoFrames.size()-1 && _videoFrames[bestFrame+1]->timestamp < time ) ++bestFrame;
	}

#if 0
	uint32_t diff = abs(_videoFrames[bestFrame]->timestamp - time);
	while (true)
	{
		if (bestFrame+1 < numFrames && static_cast<uint32_t>(abs(_videoFrames[bestFrame+1]->timestamp - time)) < diff) {
			diff = abs(_videoFrames[bestFrame+1]->timestamp - time);
			bestFrame = bestFrame + 1;
		} else if (bestFrame > 0 && static_cast<uint32_t>(abs(_videoFrames[bestFrame-1]->timestamp - time)) < diff) {
			diff = abs(_videoFrames[bestFrame-1]->timestamp - time);
			bestFrame = bestFrame - 1;
		} else {
			break;
		}
	}
#endif


	// Find closest backward keyframe  
	size_t rewindKeyframe = bestFrame;
	while ( rewindKeyframe && ! _videoFrames[rewindKeyframe]->isKeyFrame() )
	{
		rewindKeyframe--;
	}

	// Find closest forward keyframe 
	size_t forwardKeyframe = bestFrame;
	size_t size = _videoFrames.size();
	while (size > forwardKeyframe+1 && ! _videoFrames[forwardKeyframe]->isKeyFrame() )
	{
		forwardKeyframe++;
	}

	// We can't ensure we were able to find a key frame *after* the best position
	// in that case we just use any previous keyframe instead..
	if ( ! _videoFrames[forwardKeyframe]->isKeyFrame() )
	{
		bestFrame = rewindKeyframe;
	}
	else
	{
		int32_t forwardDiff = _videoFrames[forwardKeyframe]->timestamp - time;
		int32_t rewindDiff = time - _videoFrames[rewindKeyframe]->timestamp;

		if (forwardDiff < rewindDiff) bestFrame = forwardKeyframe;
		else bestFrame = rewindKeyframe;
	}

	gnash::log_debug("Seek (video): " SIZET_FMT "/" SIZET_FMT " (%u/%u)", bestFrame, numFrames, _videoFrames[bestFrame]->timestamp, time);

	_nextVideoFrame = bestFrame;
	assert( _videoFrames[bestFrame]->isKeyFrame() );
	return _videoFrames[bestFrame]->timestamp;
}



FLVVideoInfo* FLVParser::getVideoInfo()
{
	boost::mutex::scoped_lock lock(_mutex);

	// If there are no video in this FLV return NULL
	if (!_video && _lastParsedPosition > 0) return NULL;

	// Make sure that there are parsed some video frames
	while(_videoInfo == NULL && !_parsingComplete) {
		parseNextFrame();
	}

	// If there are no video data return NULL
	if (_videoInfo == NULL) return NULL;

	FLVVideoInfo* info = new FLVVideoInfo(_videoInfo->codec, _videoInfo->width, _videoInfo->height, _videoInfo->frameRate, _videoInfo->duration);
	return info;

}

FLVAudioInfo* FLVParser::getAudioInfo()
{

	boost::mutex::scoped_lock lock(_mutex);

	// If there are no audio in this FLV return NULL
	if (!_audio && _lastParsedPosition > 0) return NULL;

	// Make sure that there are parsed some audio frames
	while(_audioInfo == NULL && !_parsingComplete) {
		parseNextFrame();
	}

	// If there are no audio data return NULL
	if (_audioInfo == NULL) return NULL;

	FLVAudioInfo* info = new FLVAudioInfo(_audioInfo->codec, _audioInfo->sampleRate, _audioInfo->sampleSize, _audioInfo->stereo, _audioInfo->duration);
	return info;

}

bool FLVParser::isTimeLoaded(uint32_t time)
{
	boost::mutex::scoped_lock lock(_mutex);

	// Parse frames until the need time is found, or EOF
	while (!_parsingComplete) {
		if (!parseNextFrame()) break;
		if ((_videoFrames.size() > 0 && _videoFrames.back()->timestamp >= time)
			|| (_audioFrames.size() > 0 && _audioFrames.back()->timestamp >= time)) {
			return true;
		}
	}

	if (_videoFrames.size() > 0 && _videoFrames.back()->timestamp >= time) {
		return true;
	}

	if (_audioFrames.size() > 0 && _audioFrames.back()->timestamp >= time) {
		return true;
	}

	return false;

}

uint32_t FLVParser::seek(uint32_t time)
{
	boost::mutex::scoped_lock lock(_mutex);

	if (time == 0) {
		if (_video) _nextVideoFrame = 0;
		if (_audio) _nextAudioFrame = 0;
	}

	if (_video)	time = seekVideo(time);
	if (_audio)	time = seekAudio(time);
	return time;
}

bool FLVParser::parseNextFrame()
{
	// Parse the header if not done already. If unsuccesfull return false.
	if (_lastParsedPosition == 0 && !parseHeader()) return false;

	// Check if there is enough data to parse the header of the frame
	if (!_lt->isPositionConfirmed(_lastParsedPosition+14)) return false;

	// Seek to next frame and skip the size of the last tag
	_lt->seek(_lastParsedPosition+4);

	// Read the tag info
	uint8_t tag[12];
	_lt->read(tag, 12);

	// Extract length and timestamp
	uint32_t bodyLength = getUInt24(&tag[1]);
	uint32_t timestamp = getUInt24(&tag[4]);

	// Check if there is enough data to parse the body of the frame
	if (!_lt->isPositionConfirmed(_lastParsedPosition+15+bodyLength)) return false;

	if (tag[0] == AUDIO_TAG) {
		FLVAudioFrame* frame = new FLVAudioFrame;
		frame->dataSize = bodyLength - 1;
		frame->timestamp = timestamp;
		frame->dataPosition = _lt->tell();
		_audioFrames.push_back(frame);

		// If this is the first audioframe no info about the
		// audio format has been noted, so we do that now
		if (_audioInfo == NULL) {
			int samplerate = (tag[11] & 0x0C) >> 2;
			if (samplerate == 0) samplerate = 5500;
			else if (samplerate == 1) samplerate = 11000;
			else if (samplerate == 2) samplerate = 22050;
			else if (samplerate == 3) samplerate = 44100;

			int samplesize = (tag[11] & 0x02) >> 1;
			if (samplesize == 0) samplesize = 1;
			else samplesize = 2;

			_audioInfo = new FLVAudioInfo((tag[11] & 0xf0) >> 4, samplerate, samplesize, (tag[11] & 0x01) >> 0, 0);
		}
		_lastParsedPosition += 15 + bodyLength;

	} else if (tag[0] == VIDEO_TAG) {
		FLVVideoFrame* frame = new FLVVideoFrame;
		frame->dataSize = bodyLength - 1;
		frame->timestamp = timestamp;
		frame->dataPosition = _lt->tell();
		frame->frameType = (tag[11] & 0xf0) >> 4;
		_videoFrames.push_back(frame);

		// If this is the first videoframe no info about the
		// video format has been noted, so we do that now
		if (_videoInfo == NULL) {
			uint16_t codec = (tag[11] & 0x0f) >> 0;
			// Set standard guessed size...
			uint16_t width = 320;
			uint16_t height = 240;

			// Extract the video size from the videodata header
			if (codec == VIDEO_CODEC_H263) {
				_lt->seek(frame->dataPosition);
				uint8_t videohead[12];
				_lt->read(videohead, 12);

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
			_videoInfo = new FLVVideoInfo(codec, width, height, 0 /*frameRate*/, 0 /*duration*/);
		}
		_lastParsedPosition += 15 + bodyLength;

	} else if (tag[0] == META_TAG) {
		// Extract information from the meta tag
		/*_lt->seek(_lastParsedPosition+16);
		char* metaTag = new char[bodyLength];
		_lt->read(metaTag, bodyLength);
		amf::AMF* amfParser = new amf::AMF();
		amfParser->parseAMF(metaTag);*/

		_lastParsedPosition += 15 + bodyLength;
	} else {
		_parsingComplete = true;
		return false;
	}

	return true;
}

bool FLVParser::parseHeader()
{
	// seek to the begining of the file
	_lt->seek(0);

	// Read the header
	uint8_t header[9];
	_lt->read(header, 9);

	// Check if this is really a FLV file
	if (header[0] != 'F' || header[1] != 'L' || header[2] != 'V') return false;

	// Parse the audio+video bitmask
	if (header[4] == 5) {
		_audio = true;
		_video = true;
	} else if (header[4] == 4) {
		_audio = true;
		_video = false;
	} else if (header[4] == 4) {
		_audio = false;
		_video = true;
	} else {
		gnash::log_debug("Weird FLV bit mask\n");
	}

	_lastParsedPosition = 9;
	return true;
}

inline uint32_t FLVParser::getUInt24(uint8_t* in)
{
	// The bits are in big endian order
	return (in[0] << 16) | (in[1] << 8) | in[2];
}

void FLVParser::setLoadThread(LoadThread* lt)
{
	_lt = lt;
}

#undef PADDING_BYTES
