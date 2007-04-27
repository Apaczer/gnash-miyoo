// 
//   Copyright (C) 2007 Free Software Foundation, Inc.
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
// $Id: snappingrange.h,v 1.19 2007/04/27 12:42:31 strk Exp $

#ifndef GNASH_SNAPPINGRANGE_H
#define GNASH_SNAPPINGRANGE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <list>
#include <vector>
#include "Range2d.h"

using namespace gnash;

namespace gnash {

namespace geometry {

/// \brief
/// Snapping range class. Can hold a number of 2D ranges and combines 
/// ranges that come very close. This class is used for multiple invalidated
/// bounds calculation.
//
/// Additionally to merge intersecting ranges this class also "snaps" ranges
/// together which are very close to each other. The "snap_distance" property
/// (which *must* be initialized before using the class!) decides below what
/// distance ranges are being snapped. The "distance" is /not/ the closest
/// distance between two edges. Instead, it's the sum of the X and Y minimum
/// distance between any edge. It's similar to the distance between the two
/// nearest /edges/, but it's not the same.
/// This has the advantage that very irregular  ranges (a tight vertical one 
/// and a wide horizontal one) are not combined:
///
///          +---+
///          |   |    
///          |   |    
///          |   |    
///          |   |    
///          |   |
///          +---+
///   +-----------------------------------+
///   |                                   |
///   +-----------------------------------+    
///
/// The above example shows that the combination of the two ranges would
/// cover a much bigger area. It's better to keep two distinct ranges even
/// if they nearly touch each other.
///
template <typename T>
class SnappingRanges2d
{
public:
	typedef geometry::Range2d<T> RangeType;
	typedef std::vector<RangeType> RangeList; // TODO: list might be more appropriate
	typedef typename RangeList::size_type size_type;	

	template <typename U>
	friend std::ostream& operator<< (std::ostream& os, const SnappingRanges2d<U>& r);
	
	/// distance (horizontally *plus* vertically) below ranges are snapped
	/// You should initialize this when single_mode=false! 
	T snap_distance;
	
	/// if set, only a single, outer range is maintained (extended). 
	bool single_mode;	
	
	SnappingRanges2d() 
		:
		snap_distance(0),
		single_mode(false),
		_combine_counter(0)
	{
	}

	/// Templated copy constructor, for casting between range types
	template <typename U>
	SnappingRanges2d(const SnappingRanges2d<U>& from)
	{
		if ( from.isWorld() ) {
			setWorld();
		} else if ( from.isNull() ) {
			setNull();
		} else {
			// TODO: use visitor pattern !
			unsigned rcount = from.size();
			for (unsigned int rno=0; rno<rcount; rno++)
			{
				Range2d<U> r = from.getRange(rno);
				RangeType rc(r);
				add(rc);
			}
		}
	}
	
	/// Add a Range to the set, merging when possible and appropriate
	void add(const RangeType& range) {
		if (range.isWorld()) {
			setWorld();
			return;
		}
		
		if (range.isNull()) return;
		
		if (single_mode) {
		
			// single range mode
		
			if (_ranges.empty()) {
				RangeType temp;
				_ranges.push_back(temp);
			}
			
			_ranges[0].expandTo(range);
		
		} else {	
		
			// multi range mode
		
			for (unsigned int rno=0; rno<_ranges.size(); rno++) {
				if (snaptest(_ranges[rno], range)) {
					_ranges[rno].expandTo(range);
					return;
				}
			}
			
			// reached this point we need a new range 
			_ranges.push_back(range);
			
			combine_ranges_lazy();
		}
	}
	
	
	/// combines two snapping ranges
	void add(SnappingRanges2d<T> other_ranges) {
		for (unsigned int rno=0; rno<other_ranges.size(); rno++)
			add(other_ranges.getRange(rno));
	}
	
	/// Grows all ranges by the specified amount 
	void growBy(T amount) {
	
		if (isWorld() || isNull()) 
			return;
		
		unsigned rcount = _ranges.size();
		
		for (unsigned int rno=0; rno<rcount; rno++)
			_ranges[rno].growBy(amount);
			
		combine_ranges_lazy();
	}

	/// Scale all ranges by the specified factor
	void scale(float factor) {
	
		if (isWorld() || isNull()) 
			return;
		
		unsigned rcount = _ranges.size();
		
		for (unsigned int rno=0; rno<rcount; rno++)
			_ranges[rno].scale(factor);
			
		combine_ranges_lazy();
	}
	
	/// Combines known ranges. Previously merged ranges may have come close
	/// to other ranges. Algorithm could be optimized. 
	void combine_ranges() {
	
		if (single_mode) 	// makes no sense in single mode
			return;
	
		bool restart=true;
		
		_combine_counter=0;
		
		while (restart) {
		
			int rcount = _ranges.size();

			restart=false;
		
			for (int i=0; i<rcount; i++) {
			
				for (int j=i+1; j<rcount; j++) {
				
					if (snaptest(_ranges[i], _ranges[j])) {
						// merge i + j
						_ranges[i].expandTo(_ranges[j]);
						
						_ranges.erase(_ranges.begin() + j);
						
						restart=true; // restart from beginning
						break;
						
					} //if
				
				} //for
				
				if (restart)
					break;
			
			} //for
		
		} //while
	
	}
	
	
	/// Calls combine_ranges() once in a while, but not always. Avoids too many
	/// combine_ranges() checks, which could slow down everything.
	void combine_ranges_lazy() {
		_combine_counter++;
		if (_combine_counter > 5) 
			combine_ranges();
	}
			
	/// returns true, when two ranges should be merged together
	inline bool snaptest(const RangeType& range1, const RangeType& range2) {
	
		// when they intersect anyway, they should of course be merged! 
		if (range1.intersects(range2)) 
			return true;
			
		// simply search for the minimum x or y distances
	
		T xdist = 99999999;
		T ydist = 99999999;
		T xa1 = range1.getMinX();
		T xa2 = range2.getMinX();
		T xb1 = range1.getMaxX();
		T xb2 = range2.getMaxX();
		T ya1 = range1.getMinY();
		T ya2 = range2.getMinY();
		T yb1 = range1.getMaxY();
		T yb2 = range2.getMaxY();
		
		xdist = absmin(xdist, xa1-xa2);
		xdist = absmin(xdist, xa1-xb2);
		xdist = absmin(xdist, xb1-xa2);
		xdist = absmin(xdist, xb1-xb2);

		ydist = absmin(ydist, ya1-ya2);
 		ydist = absmin(ydist, ya1-yb2);
		ydist = absmin(ydist, yb1-ya2);
		ydist = absmin(ydist, yb1-yb2);
		
		return (xdist + ydist) <= snap_distance;

	} 
		
	/// Resets to NULL range
	void setNull() {
		_ranges.clear();
	}
	
	/// Resets to one range with world flags
	void setWorld() {
		if (isWorld()) return;
		_ranges.resize(1);
		_ranges[0].setWorld();
	}
	
	/// Returns true, wenn the ranges equal world range
	bool isWorld() const {
		return ( (size()==1) && (_ranges.front().isWorld()) );
	}
	
	/// Returns true, when there is no range
	bool isNull() const {
		return _ranges.empty();
	}
	
	/// Returns the number of ranges in the list
	size_type size() const {
		finalize();
		return _ranges.size();
	}
	
	/// Returns the range at the specified index
	//
	/// TODO: return by reference ?
	///
	RangeType getRange(unsigned int index) const {
		finalize();
		assert(index<size());
		
		return _ranges[index];
	}
	
	/// Return a range that surrounds *all* added ranges. This is used mainly
	/// for compatibilty issues. 
	RangeType getFullArea() const {
		RangeType range;
		
		range.setNull();
		
		int rcount = _ranges.size();
		
		for (int rno=0; rno<rcount; rno++) 
			range.expandTo(_ranges[rno]);
		
		return range;		
	}
	
	
	/// Returns true if any of the ranges contains the point
	bool contains(T x, T y) const {
	
		finalize();
	
		for (unsigned rno=0, rcount=_ranges.size(); rno<rcount; rno++) 
		if (_ranges[rno].contains(x,y))
			return true;
			
		return false;
	
	}

	/// Returns true if any of the ranges contains the range
	//
	/// Note that a NULL range is not contained in any range and
	/// a WORLD range is onluy contained in another WORLD range.
	///
	bool contains(const RangeType& r) const {
	
		finalize();
	
		for (unsigned rno=0, rcount=_ranges.size(); rno<rcount; rno++) 
		if (_ranges[rno].contains(r))
			return true;
			
		return false;
	
	}

	/// \brief
	/// Returns true if all ranges in the given SnappingRanges2d 
	/// are contained in at least one of the ranges composing this
	/// one.
	///
	/// Note that a NULL range is not contained in any range and
	/// a WORLD range is onluy contained in another WORLD range.
	///
	bool contains(const SnappingRanges2d<T>& o) const
	{
	
		finalize();
		// o.finalize(); // should I finalize the other range too ?

		// Null range set doesn't contain and isn't contained by anything
		if ( isNull() ) return false;
		if ( o.isNull() ) return false;

		// World range contains everything (except null ranges)
		if ( isWorld() ) return true;

		// This snappingrange is neither NULL nor WORLD
		// The other can still be WORLD, but in that case the
		// first iteration would return false
		//
		/// TODO: use a visitor !
		///
		for (unsigned rno=0, rcount=o.size(); rno<rcount; rno++) 
		{
			RangeType r = o.getRange(rno);
			if ( ! contains(r) )
			{
				return false;
			}
		}
			
		return true;
	
	}
	
	
	/// Visit the current Ranges set
	//
	/// Visitor functor will be invoked
	/// for each RangeType in the current set.
	/// 
	/// The visitor functor will 
	/// receive a RangeType reference; must return true if
	/// it wants next item or false to exit the loop.
	///
	template <class V>
	inline void visit(V& visitor) const
	{
		for (typename RangeList::const_iterator
			it = _ranges.begin(), itEnd = _ranges.end();
			it != itEnd;
			++it)
		{
			if ( ! visitor(*it) )
			{
				break;
			}
		}
	}

	/// Visit the current Ranges set
	//
	/// Visitor functor will be invoked inconditionally
	/// for each RangeType in the current set.
	/// 
	/// The visitor functor will receive a RangeType reference.
	///
	template <class V>
	inline void visitAll(V& visitor) const
	{
		for (typename RangeList::const_iterator
			it = _ranges.begin(), itEnd = _ranges.end();
			it != itEnd;
			++it)
		{
			visitor(*it);
		}
	}
	
private:

	inline T absmin(T a, T b) {
		if (b<0) b*=-1;
		return min(a,b);
	}
	
	void finalize() const {
		if (_combine_counter > 0) {
			SnappingRanges2d<T>* me_nonconst = const_cast< SnappingRanges2d<T>* > (this); 
			me_nonconst->combine_ranges();
		}
	} 
	
		
	// The current Ranges list
	RangeList _ranges;
	
	unsigned int _combine_counter;
	
}; //class SnappingRanges2d

template <class T>
std::ostream& operator<< (std::ostream& os, const SnappingRanges2d<T>& r)
{
	if ( r.isNull() ) return os << "NULL";
	if ( r.isWorld() ) return os << "WORLD";

	for (typename SnappingRanges2d<T>::RangeList::const_iterator
		it = r._ranges.begin(), itEnd = r._ranges.end();
		it != itEnd; ++it)
	{
		if ( it != r._ranges.begin() ) os << ", ";
		os << *it;
	}
	return os;
}

} //namespace gnash.geometry

/// Standard snapping 2d ranges type for invalidated bounds calculation  
typedef geometry::SnappingRanges2d<float> InvalidatedRanges;


} //namespace gnash

#endif

