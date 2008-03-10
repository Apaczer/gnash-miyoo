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


#ifndef GNASH_STRINGPREDICATES_H
#define GNASH_STRINGPREDICATES_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include <algorithm>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace gnash {

/// A case-insensitive string comparator
class StringNoCaseLessThen {
public:
	bool operator() (const std::string& a, const std::string& b) const
	{
		return std::lexicographical_compare(a.begin(), a.end(),
						    b.begin(), b.end(),
						    boost::is_iless());
	}
};


/// A case-insensitive string equality operator
class StringNoCaseEqual {
public:
	bool operator() (const std::string& a, const std::string& b) const
	{
		if ( a.length() != b.length() ) return false;
		return boost::iequals(a, b);
	}
};

} // namespace gnash

#endif // GNASH_STRINGPREDICATES_H

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
