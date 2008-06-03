// VideoDecoder.h: Video decoding base class.
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


#ifndef __VIDEODECODER_H__
#define __VIDEODECODER_H__

#include "image.h"

#include <boost/noncopyable.hpp>

// Forward declarations
namespace gnash {
	namespace media {
		class EncodedVideoFrame;
	}
}

namespace gnash {
namespace media {

/// Abstract base class for embedded video decoders.
//
/// This very simple design allows, but does not require,
/// the use of threads in an implementation. A user of this class push a frame
/// into the decoder and can subsequently pop a decoded frame. Since the pop()
/// call may block, it is advisable to first call peek() to see if there is a
/// frame ready to be popped.
///
class VideoDecoder : public boost::noncopyable {

public:
  virtual ~VideoDecoder()
  {
  }
  
  /// Push an encoded video frame into the decoder
  //
  /// @param the video frame to decode
  ///
  virtual void push(const EncodedVideoFrame& buffer) = 0;
  
  /// Pop a decoded frame from the decoder. THIS METHOD MAY BLOCK.
  //
  /// @return The decoded video frame, or a NULL-containing auto_ptr if an
  ///         error occurred.
  virtual std::auto_ptr<image::rgb> pop() = 0;
  
  /// \brief
  /// Check whether a decoded frame is ready to be popped.
  //
  /// This method will never block.
  ///
  /// @return true if there is a frame ready to be popped.
  ///
  virtual bool peek() = 0;
};

	
} // gnash.media namespace 
} // gnash namespace

#endif // __VIDEODECODER_H__
