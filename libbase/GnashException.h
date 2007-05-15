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

#ifndef _GNASH_GNASHEXCEPTION__H
#define _GNASH_GNASHEXCEPTION__H 1

#include <exception>
#include <string>

namespace gnash
{

/// Top-level gnash exception
class GnashException: public std::exception
{

public:

	GnashException(const std::string& s)
		:
		_msg(s)
	{}

	GnashException()
		:
		_msg("Generic error")
	{}

	virtual ~GnashException() throw() {}

	const char* what() const throw() { return _msg.c_str(); }

private:

	std::string _msg;
};

/// An SWF parsing exception 
class ParserException: public GnashException
{

public:

	ParserException(const std::string& s)
		:
		GnashException(s)
	{}

	ParserException()
		:
		GnashException("Parser error")
	{}

	virtual ~ParserException() throw() {}

};

/// An ActionScript error exception 
class ActionException: public GnashException
{

public:

	ActionException(const std::string& s)
		:
		GnashException(s)
	{}

	ActionException()
		:
		GnashException("ActionScript error")
	{}

	virtual ~ActionException() throw() {}

};

/// An ActionScript limit exception 
//
/// When this exception is thrown, current execution should
/// be aborted, stacks and registers cleaning included.
///
class ActionLimitException: public GnashException
{

public:

	ActionLimitException(const std::string& s)
		:
		GnashException(s)
	{}

	ActionLimitException()
		:
		GnashException("ActionScript limit hit")
	{}

	virtual ~ActionLimitException() throw() {}

};

} // namespace gnash

#endif // def _GNASH_GNASHEXCEPTION__H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
