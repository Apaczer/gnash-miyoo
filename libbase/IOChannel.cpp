// IOChannel.cpp - a virtual IO channel, for Gnash
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


#include "IOChannel.h"

namespace gnash
{

boost::uint32_t
IOChannel::read_le32() 
{
	// read_byte() is boost::uint8_t, so no masks with 0xff are required.
	boost::uint32_t result = static_cast<boost::uint32_t>(read_byte());
	result |= static_cast<boost::uint32_t>(read_byte()) << 8;
	result |= static_cast<boost::uint32_t>(read_byte()) << 16;
	result |= static_cast<boost::uint32_t>(read_byte()) << 24;
	return(result);
}

long double
IOChannel::read_le_double64() 
{
	return static_cast<long double> (
		static_cast<boost::int64_t> (read_le32()) |
		static_cast<boost::int64_t> (read_le32()) << 32
	);
}

boost::uint16_t
IOChannel::read_le16()
{
	boost::uint16_t result = static_cast<boost::uint16_t>(read_byte());
	result |= static_cast<boost::uint16_t>(read_byte()) << 8;
	return(result);
}

void
IOChannel::write_le32(boost::uint32_t u)
{
        write_byte(static_cast<boost::int8_t>(u));
        write_byte(static_cast<boost::int8_t>(u>>8));
        write_byte(static_cast<boost::int8_t>(u>>16));
        write_byte(static_cast<boost::int8_t>(u>>24));
}

void
IOChannel::write_le16(boost::uint16_t u)
{
	write_byte(static_cast<boost::int8_t>(u));
	write_byte(static_cast<boost::int8_t>(u>>8));
}

void
IOChannel::write_string(const char* src)
{
	for (;;)
	{
		write_byte(*src);
		if (*src == 0) break;
		src++;
    	}
}

int
IOChannel::read_string(char* dst, int max_length) 
{
	int i=0;
	while (i<max_length)
	{
		dst[i] = read_byte();
		if (dst[i]=='\0') return i;
		i++;
	}
    
	dst[max_length - 1] = '\0';	// force termination.
    
	return -1;
}

void
IOChannel::write_float32(float value)
{
    union alias {
        float	f;
        boost::uint32_t	i;
    } u;

    compiler_assert(sizeof(alias) == sizeof(boost::uint32_t));
    
    u.f = value;
    write_le32(u.i);
}

float
IOChannel::read_float32()
{
    union {
        float	f;
        boost::uint32_t	i;
    } u;

    compiler_assert(sizeof(u) == sizeof(u.i));
    
    u.i = read_le32();
    return u.f;
}


} // namespace gnash
