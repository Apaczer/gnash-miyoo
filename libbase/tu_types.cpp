// tu_types.cpp	-- Ignacio Casta�o, Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Minimal typedefs.  Follows SDL conventions; falls back on SDL.h if
// platform isn't obvious.


#include "tu_types.h"
#include "utility.h"


bool	tu_types_validate()
{
	// Check typedef sizes.
	if (sizeof(uint8_t) != 1
		|| sizeof(uint16_t) != 2
		|| sizeof(uint32_t) != 4
		|| sizeof(uint64) != 8
		|| sizeof(int8_t) != 1
		|| sizeof(int16_t) != 2
		|| sizeof(int32_t) != 4
		|| sizeof(sint64) != 8)
	{
		// No good.
		assert(0);
		return false;
	}

	// Endian checks.
	char* buf = "1234";

#ifdef _TU_LITTLE_ENDIAN_
	if (*(uint32_t*) buf != 0x34333231)
	{
		// No good.
		assert(0);
		return false;
	}
#else	// not _TU_LITTLE_ENDIAN_
	if (*(uint32_t*) buf != 0x31323334)
	{
		// No good.
		assert(0);
		return false;
	}
#endif	// not _TU_LITTLE_ENDIAN_

	// Checks passed.
	return true;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
