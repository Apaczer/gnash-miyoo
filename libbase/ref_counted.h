// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/* $Id: ref_counted.h,v 1.5 2007/05/28 15:41:02 ann Exp $ */

#ifndef GNASH_REF_COUNTED_H
#define GNASH_REF_COUNTED_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "container.h"
#include "smart_ptr.h"

namespace gnash {

/// \brief
/// For stuff that's tricky to keep track of w/r/t ownership & cleanup.
/// The only use for this class seems to be for putting derived
/// classes in smart_ptr
/// TODO: remove use of this base class in favor of using
/// boost::shared_ptr<> ???? boost::intrusive_ptr(?)
///

class DSOEXPORT ref_counted
{
private:
	mutable int		m_ref_count;
public:
	ref_counted()
	:
	m_ref_count(0)
	{
	}

	virtual ~ref_counted()
	{
	assert(m_ref_count == 0);
	}

	void	add_ref() const
	{
	assert(m_ref_count >= 0);
	m_ref_count++;
	}
	void	drop_ref() const {
	assert(m_ref_count > 0);
	m_ref_count--;
	if (m_ref_count <= 0){
		// Delete me!
		delete this;
		}
	}

	int	get_ref_count() const { return m_ref_count; }
};

} // namespace gnash

#endif // GNASH_REF_COUNTED_H
