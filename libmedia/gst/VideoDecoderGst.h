// VideoDecoderGst.h: Video decoding using Gstreamer.
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

// $Id: VideoDecoderGst.h,v 1.14 2008/02/23 18:12:51 bjacques Exp $

#ifndef __VIDEODECODERGST_H__
#define __VIDEODECODERGST_H__


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "image.h"
#include <gst/gst.h>
#include "log.h"
#include "MediaParser.h"
#include "VideoDecoder.h"


namespace gnash {
namespace media {

// Convenience wrapper for GstBuffer. Intended to be wrapped in an auto_ptr.
class gnashGstBuffer : public image::rgb
{
public:
  gnashGstBuffer(GstBuffer* buf, int width, int height)
  : rgb(NULL, width, height, (width * 3 + 3) & ~3),
    _buffer(buf)
  {}
  
  ~gnashGstBuffer()
  {
    gst_buffer_unref(_buffer);
  }
  
  boost::uint8_t* data()
  {
    return GST_BUFFER_DATA(_buffer);
  }

  std::auto_ptr<image::image_base> clone() const
  {
    return std::auto_ptr<image_base>(new rgb(*this));
  }

private:
  GstBuffer* _buffer;
};


class VideoDecoderGst : public VideoDecoder
{
public:
  VideoDecoderGst(videoCodecType codec_type, int width, int height);
  ~VideoDecoderGst();

  void push(const EncodedVideoFrame& buffer);

  std::auto_ptr<image::rgb> pop();
  
  bool peek();

private:  
  void checkMessages();
  void handleMessage(GstMessage* message);

  VideoDecoderGst();
  VideoDecoderGst(const gnash::media::VideoDecoderGst&);

  GstElement* _pipeline;
  GstElement* _appsrc;
  GstElement* _appsink;
  GstElement* _colorspace;
};


} // namespace media
} // namespace gnash
#endif // __VIDEODECODERGST_H__
