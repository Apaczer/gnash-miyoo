// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

// Implementation of the Global ActionScript Object

#ifndef GNASH_GLOBAL_H
#define GNASH_GLOBAL_H

#include "as_object.h" // for inheritance

// Forward declarations
namespace gnash {
	class VM;
	class fn_call;
}

namespace gnash {

class Global: public as_object
{
public:
	Global(VM& vm);
	~Global() {}
private:

#ifdef USE_EXTENSIONS
	Extension et;
#endif
};

} // namespace gnash

#endif // ndef GNASH_GLOBAL_H
