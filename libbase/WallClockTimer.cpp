// WallClockTimer.cpp:  Wall clock timer, for Gnash.
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

/* $Id: WallClockTimer.cpp,v 1.3 2008/01/21 20:55:44 rsavoye Exp $ */

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "WallClockTimer.h"

#include "tu_timer.h" // for "portable" get_ticks (contains implementation for win32)

namespace gnash {

WallClockTimer::WallClockTimer()
	:
	startTimer(tu_timer::get_ticks())
{
}

void
WallClockTimer::restart()
{
	startTimer = tu_timer::get_ticks();
}

boost::uint32_t
WallClockTimer::elapsed() const
{
	boost::uint64_t currTime = tu_timer::get_ticks();
	
	// be aware of time glitches
	if ( currTime <= startTimer ) return 0;

	return currTime-startTimer;

}

} // namespace gnash
