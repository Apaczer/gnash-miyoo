// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "log.h"
#include "SoundFfmpeg.h"
#include "sound.h" // for sound_sample_impl
#include "movie_definition.h"
#include "sprite_instance.h"
#include "fn_call.h"
#include "GnashException.h"
#include "builtin_function.h"

#include <string>

namespace gnash {

// ffmpeg callback function
int 
SoundFfmpeg::readPacket(void* opaque, uint8_t* buf, int buf_size)
{

	SoundFfmpeg* so = static_cast<SoundFfmpeg*>(opaque);
	NetConnection* nc = so->connection;

	size_t ret = nc->read(static_cast<void*>(buf), buf_size);
	so->inputPos += ret;
	return ret;

}

// ffmpeg callback function
offset_t 
SoundFfmpeg::seekMedia(void *opaque, offset_t offset, int whence){

	SoundFfmpeg* so = static_cast<SoundFfmpeg*>(opaque);
	NetConnection* nc = so->connection;


	// Offset is absolute new position in the file
	if (whence == SEEK_SET) {
		nc->seek(offset);
		so->inputPos = offset;

	// New position is offset + old position
	} else if (whence == SEEK_CUR) {
		nc->seek(so->inputPos + offset);
		so->inputPos = so->inputPos + offset;

	// 	// New position is offset + end of file
	} else if (whence == SEEK_END) {
		// This is (most likely) a streamed file, so we can't seek to the end!
		// Instead we seek to 50.000 bytes... seems to work fine...
		nc->seek(50000);
		so->inputPos = 50000;
		
	}

	return so->inputPos;
}


void
SoundFfmpeg::setupDecoder(SoundFfmpeg* so)
{

	NetConnection* nc = so->connection;
	assert(nc);

	// Pass stuff from/to the NetConnection object.
	assert(so);
	if ( !nc->openConnection(so->externalURL.c_str(), so) ) {
		log_warning("Gnash could not open audio url: %s", so->externalURL.c_str());
		delete so->lock;
		return;
	}

	so->inputPos = 0;

	// This registers all available file formats and codecs 
	// with the library so they will be used automatically when
	// a file with the corresponding format/codec is opened

	av_register_all();

	// Open video file

	// Probe the file to detect the format
	AVProbeData probe_data, *pd = &probe_data;
	pd->filename = "";
	pd->buf = new uint8_t[2048];
	pd->buf_size = 2048;

	if (readPacket(so, pd->buf, pd->buf_size) < 1){
 		log_warning("Gnash could not read from audio url: %s", so->externalURL.c_str());
 		delete[] pd->buf;
 		delete so->lock;
 		return;
	}

	AVInputFormat* inputFmt = av_probe_input_format(pd, 1);

	// After the format probe, reset to the beginning of the file.
	nc->seek(0);

	// Setup the filereader/seeker mechanism. 7th argument (NULL) is the writer function,
	// which isn't needed.
	init_put_byte(&so->ByteIOCxt, new uint8_t[500000], 500000, 0, so, SoundFfmpeg::readPacket, NULL, SoundFfmpeg::seekMedia);
	so->ByteIOCxt.is_streamed = 1;

	so->formatCtx = av_alloc_format_context();

	// Open the stream. the 4th argument is the filename, which we ignore.
	if(av_open_input_stream(&so->formatCtx, &so->ByteIOCxt, "", inputFmt, NULL) < 0){
		log_error("Couldn't open file '%s' for decoding", so->externalURL.c_str());
		delete so->lock;
		return;
	}

	// Next, we need to retrieve information about the streams contained in the file
	// This fills the streams field of the AVFormatContext with valid information
	int ret = av_find_stream_info(so->formatCtx);
	if (ret < 0)
	{
		log_error("Couldn't find stream information from '%s', error code: %d", so->externalURL.c_str(), ret);
		delete so->lock;
		return;
	}

	// Find the first audio stream
	so->audioIndex = -1;

	for (unsigned int i = 0; i < so->formatCtx->nb_streams; i++)
	{
		AVCodecContext* enc = so->formatCtx->streams[i]->codec; 

		switch (enc->codec_type)
		{
			case CODEC_TYPE_AUDIO:
				if (so->audioIndex < 0)
				{
					so->audioIndex = i;
					so->audioStream = so->formatCtx->streams[i];
				}
				break;

			case CODEC_TYPE_VIDEO:
			case CODEC_TYPE_DATA:
			case CODEC_TYPE_SUBTITLE:
			case CODEC_TYPE_UNKNOWN:
				log_warning("Non-audio data found in file %s\n", so->externalURL.c_str());
				break;
		}
	}

	if (so->audioIndex < 0)
	{
		log_error("Didn't find a audio stream from '%s'", so->externalURL.c_str());
		return;
	}

	// Get a pointer to the audio codec context for the video stream
	so->audioCodecCtx = so->formatCtx->streams[so->audioIndex]->codec;

	// Find the decoder for the audio stream
	AVCodec* pACodec = avcodec_find_decoder(so->audioCodecCtx->codec_id);
    if(pACodec == NULL)
	{
		log_error("No available AUDIO decoder to process file: '%s'", so->externalURL.c_str());
		delete so->lock;
		return;
	}
    
	// Open codec
	if (avcodec_open(so->audioCodecCtx, pACodec) < 0)
	{
		log_error("Could not open AUDIO codec");
		delete so->lock;
		return;
	}

	// By deleting this lock we allow start() to start playback
	delete so->lock;
	return;
}

// audio callback is running in sound handler thread
bool SoundFfmpeg::getAudio(void* owner, uint8_t* stream, int len)
{
	SoundFfmpeg* so = static_cast<SoundFfmpeg*>(owner);

	int pos = 0;

	// First use the data left over from last time
	if (so->leftOverSize > 0) {

		// If we have enough "leftover" data to fill the buffer,
		// we don't bother to decode some new.
		if (so->leftOverSize >= len) {
			memcpy(stream, so->leftOverData, len);
			int rest = so->leftOverSize - len;
			if (rest < 1) {
				delete[] so->leftOverData;
				so->leftOverSize = 0;
			} else {
				uint8_t* buf = new uint8_t[rest];
				memcpy(stream, so->leftOverData+len, rest);
				delete[] so->leftOverData;
				so->leftOverData = buf;
				so->leftOverSize -= len;
			}	
			return true;
		} else {
			memcpy(stream, so->leftOverData, so->leftOverSize);
			pos += so->leftOverSize;
			so->leftOverSize = 0;
			delete[] so->leftOverData;
		}
	}

	AVPacket packet;
	int rc;
	bool loop = true;
	uint8_t* ptr = new uint8_t[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	bool ret = true;
	while (loop) {
		// Parse file
		rc = av_read_frame(so->formatCtx, &packet);
		if (rc >= 0)
		{
			if (packet.stream_index == so->audioIndex)
			{
				sound_handler* s = get_sound_handler();
				if (s)
				{
					// Decode audio
					int frame_size;
#ifdef FFMPEG_AUDIO2
					frame_size = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
					if (avcodec_decode_audio2(so->audioCodecCtx, (int16_t*) ptr, &frame_size, packet.data, packet.size) >= 0)
#else
					if (avcodec_decode_audio(so->audioCodecCtx, (int16_t*) ptr, &frame_size, packet.data, packet.size) >= 0)
#endif
					{

						bool stereo = so->audioCodecCtx->channels > 1 ? true : false;
						int samples = stereo ? frame_size >> 2 : frame_size >> 1;
						int newDataSize = 0;
						int16_t* output_data = NULL;

						// Resample if needed
						if (so->audioCodecCtx->channels != 2 || so->audioCodecCtx->sample_rate != 44100) {
							// Resampeling using ffmpegs (libavcodecs) resampler
							if (!so->resampleCtx) {
								// arguments: (output channels, input channels, output rate, input rate)
								so->resampleCtx = audio_resample_init (2, so->audioCodecCtx->channels, 44100, so->audioCodecCtx->sample_rate);
							}
							// The size of this is a guess, we don't know yet... Lets hope it's big enough
							output_data = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE];
							samples = audio_resample (so->resampleCtx, output_data, (int16_t*)ptr, samples);
							newDataSize =  samples * 2 * 2; // 2 for stereo and 2 for samplesize = 2 bytes
						} else {
							output_data = (int16_t*)ptr;
							newDataSize = samples * 2 * 2;
						}

						// Copy the data to buffer
						// If the decoded data isn't enough to fill the buffer, we put the decoded
						// data into the buffer, and continues decoding.
						if (newDataSize <= len-pos) {
							memcpy(stream+pos, (uint8_t*)output_data, newDataSize);
							pos += newDataSize;
						} else {
						// If we can fill the buffer, and still have "leftovers", we save them
						// and use them later.
							int rest = len-pos;
							so->leftOverSize = newDataSize - rest;
							memcpy(stream+pos, (uint8_t*)output_data, rest);
							so->leftOverData = new uint8_t[so->leftOverSize];
							memcpy(so->leftOverData, ((uint8_t*)output_data)+rest, so->leftOverSize);
							loop = false;
							pos += rest;
						}
						delete[] output_data;
					}
				}
			}
		} else {
			// If we should loop we make sure we do.
			if (so->remainingLoops != 0) {
				so->remainingLoops--;

				// Seek to begining of file
				if (av_seek_frame(so->formatCtx, so->audioIndex, 0, 0) < 0) {
					log_warning("seeking to start of file (for looping) failed\n");
					so->remainingLoops = 0;
				}
			} else {
			 // Stops playback by returning false which makes the soundhandler
			 // detach this sound.
			 ret = false;
			 so->isAttached = false;
			 break;
			}
		} 
	} // while
	delete[] ptr;
	return ret;
}

SoundFfmpeg::~SoundFfmpeg() {
	if (externalSound) {
		 if (leftOverData && leftOverSize) delete[] leftOverData;

		if (audioCodecCtx) avcodec_close(audioCodecCtx);
		audioCodecCtx = NULL;

		if (formatCtx) {
			formatCtx->iformat->flags = AVFMT_NOFILE;
			av_close_input_file(formatCtx);
			formatCtx = NULL;
		}

		if (resampleCtx) {
			audio_resample_close (resampleCtx);
		}

		if (isAttached) {
			sound_handler* s = get_sound_handler();
			if (s) {
				s->detach_aux_streamer(this);
			}
		}
	}
}

void
SoundFfmpeg::loadSound(std::string file, bool streaming)
{
	leftOverData = NULL;
	leftOverSize = 0;
	audioIndex = -1;
	resampleCtx = NULL;
	remainingLoops = 0;

	if (connection) {
		log_warning("This sound already has a connection?? (We try to handle this by deleting the old one...)\n");
		delete connection;
	}
	externalURL = file;

	connection = new NetConnection();

	externalSound = true;
	isStreaming = streaming;

	lock = new boost::mutex::scoped_lock(setupMutex);

	// To avoid blocking while connecting, we use a thread.
	setupThread = new boost::thread(boost::bind(SoundFfmpeg::setupDecoder, this));

}

void
SoundFfmpeg::start(int offset, int loops)
{
	boost::mutex::scoped_lock lock(setupMutex);

	if (externalSound) {
		if (offset > 0) {
			// Seek to offset position
			double timebase = (double)formatCtx->streams[audioIndex]->time_base.num / (double)formatCtx->streams[audioIndex]->time_base.den;

			long newpos = (long)(offset / timebase);

			if (av_seek_frame(formatCtx, audioIndex, newpos, 0) < 0) {
				log_warning("seeking to offset failed\n");
			}
		}
		// Save how many loops to do
		if (loops > 0) {
			remainingLoops = loops;
		}
	}

	// Start sound
	sound_handler* s = get_sound_handler();
	if (s) {
		if (externalSound) {
			if (audioIndex >= 0)
			{
				s->attach_aux_streamer(getAudio, (void*) this);
				isAttached = true;
			} 
		} else {
	    	s->play_sound(soundId, loops, offset, 0, NULL);
	    }
	}
}

void
SoundFfmpeg::stop(int si)
{
	// stop the sound
	sound_handler* s = get_sound_handler();
	if (s != NULL)
	{
	    if (si > -1) {
	    	if (externalSound) {
	    		s->detach_aux_streamer(this);
	    	} else {
				s->stop_sound(soundId);
			}
		} else {
			s->stop_sound(si);
		}
	}
}

unsigned int
SoundFfmpeg::getDuration()
{
	// Return the duration of the file in milliseconds
	if (formatCtx && audioIndex) {
		return static_cast<unsigned int>(formatCtx->duration * 1000);
	} else {
		return 0;
	}
}

unsigned int
SoundFfmpeg::getPosition()
{
	// Return the position in the file in milliseconds
	if (formatCtx && audioIndex) {
		double time = (double)formatCtx->streams[audioIndex]->time_base.num / formatCtx->streams[audioIndex]->time_base.den * (double)formatCtx->streams[audioIndex]->cur_dts;
		return static_cast<unsigned int>(time * 1000);
	} else {
		return 0;
	}
}

} // end of gnash namespace

