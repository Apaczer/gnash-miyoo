// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// 
//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swf/TagLoadersTable.h"
#include "swf.h"

#include <map>
#include <cassert>

namespace gnash {
namespace SWF {

bool
TagLoadersTable::get(SWF::tag_type t, loader_function* lf)
{
	container::iterator it = _tag_loaders.find(t);

	// no loader found for the specified tag
	if ( it == _tag_loaders.end() ) return false;

	// copy loader_function to the given pointer
	*lf = it->second;
	return true;
}

bool
TagLoadersTable::register_loader(SWF::tag_type t, loader_function lf)
{
	assert(lf != NULL);

	// loader for given tag already registered
	if (_tag_loaders.find(t) != _tag_loaders.end()) return false;
    
	_tag_loaders[t] = lf;
	return true;
}

} // namespace gnash::SWF
} // namespace gnash

