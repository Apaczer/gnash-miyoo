// stream.cpp - SWF stream reading class, for Gnash
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


#include "stream.h"

#include "log.h"
#include "types.h"
#include "tu_file.h"
#include "swf.h"
#include "Property.h"
#include <cstring>
#include <climits>
//#include <iostream> // debugging only

//#define USE_TU_FILE_BYTESWAPPING 1

namespace gnash {
	
stream::stream(tu_file* input)
	:
	m_input(input),
	m_current_byte(0),
	m_unused_bits(0)
{
}


stream::~stream()
{
}

unsigned stream::read(char *buf, unsigned count)
{
	align();

	// If we're in a tag, make sure we're not seeking outside the tag.
	if ( ! _tagBoundsStack.empty() )
	{
		TagBoundaries& tb = _tagBoundsStack.back();
		unsigned long end_pos = tb.second;
		unsigned long cur_pos = get_position();
		assert(end_pos >= cur_pos);
		unsigned long left = end_pos - cur_pos;
		if ( left < count ) count = left;
	}

	if ( ! count ) return 0;

	return m_input->read_bytes(buf, count);
}

bool stream::read_bit()
{
	if (!m_unused_bits)
	{
		m_current_byte = m_input->read_byte(); // don't want to align here
		m_unused_bits = 7;
		return (m_current_byte&0x80);
	}
	else
	{
		return ( m_current_byte & (1<<(--m_unused_bits)) );
	}
}

unsigned stream::read_uint(unsigned short bitcount)
{
	// htf_sweet.swf fails when this is set to 24. There seems to
	// be no reason why this should be limited to 32 other than
	// that it is higher than a movie is likely to need.
	if (bitcount > 32)
	{
	    // This might overflow a uint32_t or attempt to read outside
	    // the byte cache (relies on there being only 4 bytes after
        // possible unused bits.)
	    throw ParserException("Unexpectedly long value advertised.");
	}

	// Optimization for multibyte read
	if ( bitcount > m_unused_bits )
	{
		typedef unsigned char byte;

		boost::uint32_t value = 0;

		if (m_unused_bits) // Consume all the unused bits.
		{
			int unusedMask = (1 << m_unused_bits)-1;
			bitcount -= m_unused_bits; 
			value |= ((m_current_byte&unusedMask) << bitcount);
		}

		int bytesToRead = bitcount/8;
		int spareBits = bitcount%8; // additional bits to read

		//std::cerr << "BytesToRead: " << bytesToRead << " spareBits: " << spareBits << " unusedBits: " << (int)m_unused_bits << std::endl;

        assert (bytesToRead <= 4);
		byte cache[4]; // at most 4 bytes in the cache

		if ( spareBits ) m_input->read_bytes(&cache, bytesToRead+1);
		else m_input->read_bytes(&cache, bytesToRead);

		for (int i=0; i<bytesToRead; ++i)
		{
			bitcount -= 8;
			value |= cache[i] << bitcount; 
		}

		//assert(bitcount == spareBits);
		if ( bitcount )
		{
			m_current_byte = cache[bytesToRead];
			m_unused_bits = 8-bitcount;
			value |= m_current_byte >> m_unused_bits;
		}
		else
		{
			m_unused_bits = 0;
		}

		return value;
		
	}

	if (!m_unused_bits)
	{
		m_current_byte = m_input->read_byte();
		m_unused_bits = 8;
	}

	// TODO: optimize unusedMask creation ?
	//       (it's 0xFF if ! m_unused_bits above)
	int unusedMask = (1 << m_unused_bits)-1;

	if (bitcount == m_unused_bits)
	{
		// Consume all the unused bits.
		m_unused_bits = 0;
		return (m_current_byte&unusedMask);
	}
	else
	{
		assert(bitcount < m_unused_bits);
		// Consume some of the unused bits.

		m_unused_bits -= bitcount;
		return ((m_current_byte&unusedMask) >> m_unused_bits);
	}

}


int	stream::read_sint(unsigned short bitcount)
{
	//assert(bitcount <= 32); // already asserted in read_uint

	boost::int32_t	value = boost::int32_t(read_uint(bitcount));

	// Sign extend...
	if (value & (1 << (bitcount - 1))) {
		value |= -1 << bitcount;
	}

//		IF_DEBUG(log_debug("stream::read_sint(%d) == %d\n", bitcount, value));

	return value;
}


float	stream::read_fixed()
{
	// align(); // read_u32 will align 
	return static_cast<float> (
		static_cast<double>(read_s32()) / 65536.0f
	);

}

// float is not large enough to hold a 32 bit value without doing the wrong thing with the sign.
// So we upgrade to double for the calculation and then resize when we know it's small enough.
float	stream::read_ufixed()
{
	// align(); // read_u32 will align 
	return static_cast<float> (
		static_cast<double>(read_u32()) / 65536.0f
	);
}

// Read a short fixed value, unsigned.
float   stream::read_short_ufixed()
{
	// align(); // read_u16 will align 
	return static_cast<float> ( read_u16() / 256.0f );
}

// Read a short fixed value, signed.
float	stream::read_short_sfixed()
{
	// align(); // read_s16 will align 
	return static_cast<float> ( read_s16() / 256.0f );
}

/// Read a 16bit (1:sign 5:exp 10:mantissa) floating point value
float	stream::read_short_float()
{
	// read_s16 will align
	return static_cast<float> ( read_s16() );
}

// Read a little-endian 32-bit float from p
// and return it as a host-endian float.
static float
convert_float_little(const void *p)
{
	// Hairy union for endian detection and munging
	union {
		float	f;
		boost::uint32_t i;
		struct {	// for endian detection
			boost::uint16_t s0;
			boost::uint16_t s1;
		} s;
		struct {	// for byte-swapping
			boost::uint8_t c0;
			boost::uint8_t c1;
			boost::uint8_t c2;
			boost::uint8_t c3;
		} c;
	} u;

	u.f = 1.0;
	switch (u.s.s0) {
	case 0x0000:	// little-endian host
		memcpy(&u.i, p, 4);
		break;
	case 0x3f80:	// big-endian host
	    {
		const boost::uint8_t *cp = (const boost::uint8_t *) p;
		u.c.c0 = cp[3];
		u.c.c1 = cp[2];
		u.c.c2 = cp[1];
		u.c.c3 = cp[0];
	    }
	    break;
	default:
	    log_error(_("Native floating point format not recognised"));
	    abort();
	}
	
	return u.f;
}

/// Read a 32bit (1:sign 8:exp 23:mantissa) floating point value
float	stream::read_long_float()
{
	char data[4]; read(data, 4); // would align
	return convert_float_little(data); 
}

// Read a 64-bit double value
long double stream::read_d64()
{
#ifdef USE_TU_FILE_BYTESWAPPING 
	align();
	return m_input->read_le_double64();
#else
	using boost::uint32_t;

	unsigned char _buf[8]; read((char*)_buf, 8); // would align
	uint64_t low = _buf[0];
		 low |= _buf[1] << 8;
		 low |= _buf[2] << 16;
		 low |= _buf[3] << 24;

	uint64_t hi = _buf[4];
		 hi |= _buf[5] << 8;
		 hi |= _buf[6] << 16;
		 hi |= _buf[7] << 24;

	return static_cast<long double> ( low | (hi<<32) );
#endif
}

boost::uint8_t	stream::read_u8()
{
	align();
	return m_input->read_byte();
}

int8_t	stream::read_s8()
{
	// read_u8 will align
	return read_u8();
}

boost::uint16_t	stream::read_u16()
{
#ifdef USE_TU_FILE_BYTESWAPPING 
	align();
	return m_input->read_le16();
#else
	using boost::uint32_t;

	unsigned char _buf[2]; read((char*)_buf, 2); // would align
	uint32_t result = _buf[0];
		 result |= (_buf[1] << 8);

	return result;
#endif
}

boost::int16_t stream::read_s16()
{
	// read_u16 will align
	return read_u16();
}

boost::uint32_t	stream::read_u32()
{
#ifdef USE_TU_FILE_BYTESWAPPING 
	align();
	return m_input->read_le32();
#else
	using boost::uint32_t;

	unsigned char _buf[4]; read((char*)_buf, 4); // would align
	uint32_t result = _buf[0];
		 result |= _buf[1] << 8;
		 result |= _buf[2] << 16;
		 result |= _buf[3] << 24;

	return result;
#endif
}

boost::int32_t	stream::read_s32()
{
	// read_u32 will align
	return read_u32();
}

void
stream::read_string(std::string& to)
{
	align();

	to.clear();

	do
	{
		ensureBytes(1);
		char c = read_u8();
		if ( c == 0 ) break; // don't store a NULL in the string..
		to += c;
	} while(1);

}

void stream::read_string_with_length(std::string& to)
{
	align();

	ensureBytes(1);
	unsigned int	len = read_u8();
	read_string_with_length(len, to); // will check 'len'
}

void stream::read_string_with_length(unsigned len, std::string& to)
{
	align();

	to.resize(len);

	ensureBytes(len);
	for (unsigned int i = 0; i < len; ++i)
	{
		to[i] = read_u8();
	}

}


unsigned long stream::get_position()
{
	return m_input->get_position();
}


bool	stream::set_position(unsigned long pos)
{
	align();

	// If we're in a tag, make sure we're not seeking outside the tag.
	if ( ! _tagBoundsStack.empty() )
	{
		TagBoundaries& tb = _tagBoundsStack.back();
		unsigned long end_pos = tb.second;
		if ( pos > end_pos )
		{
			log_error("Attempt to seek past the end of an opened tag");
			// abort(); // ?
			// throw ParserException ?
			return false;
		}
		unsigned long start_pos = tb.first;
		if ( pos < start_pos )
		{
			log_error("Attempt to seek before start of an opened tag");
			// abort(); // ?
			// throw ParserException ?
			return false;
		}
	}

	// Do the seek.
	if ( m_input->set_position(pos) == TU_FILE_SEEK_ERROR )
	{
		// TODO: should we throw an exception ?
		//       we might be called from an exception handler
		//       so throwing here might be a double throw...
		log_swferror(_("Unexpected end of stream"));
		return false;
	}

	return true;
}


unsigned long stream::get_tag_end_position()
{
	assert(_tagBoundsStack.size() > 0);

	return _tagBoundsStack.back().second;
}


SWF::tag_type stream::open_tag()
{
	align();

	unsigned long tagStart = get_position();

	int	tagHeader = read_u16();
	int	tagType = tagHeader >> 6;
	int	tagLength = tagHeader & 0x3F;
	assert(m_unused_bits == 0);
		
	if (tagLength == 0x3F)
	{
		tagLength = read_u32();
	}

	if (tagLength < 0)
	{
		throw ParserException("Negative tag length advertised.");
	}

	if ( tagLength > 1024*64 )
	{
		log_debug("Tag %d has a size of %d bytes !!", tagType, tagLength);
	}

	unsigned long tagEnd = get_position() + tagLength;
	
	// Check end position doesn't overflow a signed int - that makes
	// zlib adapter's inflate_seek(int pos, void* appdata) unhappy.
	// The cast stops compiler warnings. We know it's a positive number.
	// TODO: make tu_file take a long instead of an int.
	// TODO: check against stream length.
	if (tagEnd > static_cast<unsigned int>(std::numeric_limits<signed int>::max()))
	{
		std::stringstream ss;
		ss << "Invalid tag end position " << tagEnd << " advertised (tag length "
			<< tagLength << ").";
		throw ParserException(ss.str().c_str());
	}	

	if ( ! _tagBoundsStack.empty() )
	{
		// check that this tag doesn't cross containing tag bounds
		unsigned long containerTagEnd = _tagBoundsStack.back().second;
		if ( tagEnd > containerTagEnd )
		{
			unsigned long containerTagStart = _tagBoundsStack.back().first;
			std::stringstream ss;
			ss << "Tag " << tagType << " starting at offset " << tagStart
			   << " is advertised to end at offset " << tagEnd
			   << " which is after end of previously opened tag starting "
			   << " at offset " << containerTagStart
			   << " and ending at offset " << containerTagEnd << "."
			   << " Making it end where container tag ends.";
			log_swferror("%s", ss.str().c_str());

			// what to do now ?
			tagEnd = containerTagEnd;
			//throw ParserException(ss.str());
		}
	}
		
	// Remember where the end of the tag is, so we can
	// fast-forward past it when we're done reading it.
	_tagBoundsStack.push_back(std::make_pair(tagStart, tagEnd));

	IF_VERBOSE_PARSE (
		log_parse("SWF[%lu]: tag type = %d, tag length = %d, end tag = %lu",
		tagStart, tagType, tagLength, tagEnd);
	);

	return static_cast<SWF::tag_type>(tagType);
}


void	stream::close_tag()
{
	assert(_tagBoundsStack.size() > 0);
	unsigned long end_pos = _tagBoundsStack.back().second;
	_tagBoundsStack.pop_back();

	if ( m_input->set_position(end_pos) == TU_FILE_SEEK_ERROR )
	{
		log_error("Could not seek to end position");
	}

	m_unused_bits = 0;
}

} // end namespace gnash

	
// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
