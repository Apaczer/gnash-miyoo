// GnashImage.h: Base class for reading image data in Gnash.
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
//

#ifndef GNASH_GNASHIMAGE_H
#define GNASH_GNASHIMAGE_H

#include <boost/shared_ptr.hpp> 

#include "dsodefs.h"

// Forward declarations
namespace gnash { class IOChannel; }

namespace gnash {

class ImageInput {

public:

	ImageInput(boost::shared_ptr<IOChannel> in) :
	    _inStream(in)
	{}

	virtual ~ImageInput() {}


	/// \brief
	/// Create and return a jpeg-input object that will read from the
	/// given input stream.
	//
	/// The created input reads the jpeg header
	///
	/// @param in
	///	The stream to read from. Ownership specified by last arg.
	///
	/// @param takeOwnership
	///	If false, ownership of the stream 
	///	is left to caller, otherwise we take it.
	///	NOTE: In case the caller retains ownership, it must
	///	make sure the stream is alive and not modified
	///	for the whole lifetime of the returned instance.
	///
	/// @return NULL on error
	///

    virtual void read() = 0;

	virtual size_t getHeight() const = 0;
	virtual size_t getWidth() const = 0;
	virtual void readScanline(unsigned char* rgb_data) = 0;

protected:

    boost::shared_ptr<IOChannel> _inStream;

};

class ImageOutput
{

public:

    ImageOutput(size_t width, size_t height) :
        _width(width),
        _height(height)
        {}

    virtual ~ImageOutput() {}

    virtual void writeScanline(unsigned char* rgbData) = 0;

protected:

    size_t _width;

    size_t _height;

};

} // namespace gnash



#endif
