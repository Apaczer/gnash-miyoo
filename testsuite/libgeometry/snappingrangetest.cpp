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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "check.h"
#include "Range2d.h"
#include "snappingrange.h"
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;
using namespace gnash;
using namespace gnash::geometry;

int
main(int /*argc*/, char** /*argv*/)
{

	//
	// Test NULL range construction
	//

	SnappingRanges2d<float> nullSnap1;
	check(   nullSnap1.isNull() );
	check( ! nullSnap1.isWorld() );

	//
	// Test WORLD range construction
	//

	Range2d<float> worldRange1(worldRange);
	SnappingRanges2d<float> worldSnap1;
	worldSnap1.add(worldRange);
	check( ! worldSnap1.isNull() );
	check(   worldSnap1.isWorld() );
	
	
	//
	// Test world range no-op additions
	//
	Range2d<float> someRange(10.1,20.2,30.3,40.4);
	worldSnap1.add(someRange);
	check( ! worldSnap1.isNull() );
	check(   worldSnap1.isWorld() );
	
	//
	// Test overlapping ranges snapping
	//
	
	Range2d<float> upperleft (10, 10, 30, 30); 
	Range2d<float> lowerright (20, 20, 40, 40);
	SnappingRanges2d<float> overlapSnap;
	overlapSnap.add(upperleft);
	overlapSnap.add(lowerright);
	check( ! overlapSnap.isNull() );
	check( ! overlapSnap.isWorld() );
	check( ! overlapSnap.contains(5,  5) );
	check(   overlapSnap.contains(10, 10) );
	check(   overlapSnap.contains(15, 15) );
	check(   overlapSnap.contains(20, 20) );
	check(   overlapSnap.contains(25, 25) );
	check(   overlapSnap.contains(30, 30) );
	check(   overlapSnap.contains(35, 35) );
	check(   overlapSnap.contains(40, 40) );
	check( ! overlapSnap.contains(45, 45) );
	
	// 
	// Test touching ranges snapping
	//
	
	Range2d<float> left1 (10, 10, 50, 50); 
	Range2d<float> right1 (50, 10, 100, 50);
	SnappingRanges2d<float> touchSnap;
	touchSnap.snap_distance = 20;
	touchSnap.add(left1);
	touchSnap.add(right1);
	check( ! touchSnap.isNull() );
	check( ! touchSnap.isWorld() );
	check( ! touchSnap.contains( 5, 30) );
	check(   touchSnap.contains(10, 30) );
	check(   touchSnap.contains(15, 30) );
	check(   touchSnap.contains(45, 30) );
	check(   touchSnap.contains(50, 30) );
	check(   touchSnap.contains(55, 30) );
	check(   touchSnap.contains(95, 30) );
	check(   touchSnap.contains(100, 30) );
	check( ! touchSnap.contains(105, 30) );

	// 
	// Test near ranges snapping
	//
	
	Range2d<float> left2 (10, 10, 50, 50); 
	Range2d<float> right2 (60, 10, 100, 50);
	SnappingRanges2d<float> nearSnap;
	nearSnap.snap_distance = 20;
	nearSnap.add(left2);
	nearSnap.add(right2);
	check( ! nearSnap.isNull() );
	check( ! nearSnap.isWorld() );
	check( ! nearSnap.contains( 5, 30) );
	check(   nearSnap.contains(10, 30) );
	check(   nearSnap.contains(15, 30) );
	check(   nearSnap.contains(45, 30) );
	check(   nearSnap.contains(50, 30) );
	check(   nearSnap.contains(55, 30) );
	check(   nearSnap.contains(95, 30) );
	check(   nearSnap.contains(100, 30) );
	check( ! nearSnap.contains(105, 30) );
	
	
	//
	// Test irregular ranges (should not snap)
	//
	Range2d<float> horiz (10, 10, 500, 20); 
	Range2d<float> vert (290, 22, 300, 500);
	SnappingRanges2d<float> irrSnap;
	irrSnap.snap_distance = 20;
	irrSnap.add(horiz);
	irrSnap.add(vert);
	check( ! irrSnap.isNull() );
	check( ! irrSnap.isWorld() );
	check(   irrSnap.contains( 15,  15) );
	check(   irrSnap.contains(495,  15) );
	check(   irrSnap.contains(295,  10) );
	check(   irrSnap.contains(295,  15) );
	check(   irrSnap.contains(295,  20) );
	check(   irrSnap.contains(295,  22) );
	check(   irrSnap.contains(295,  25) );
	check(   irrSnap.contains(295,  30) );
	check(   irrSnap.contains(295, 495) );
	check( ! irrSnap.contains( 15,  25) );
	check( ! irrSnap.contains(495,  25) );
	check( ! irrSnap.contains( 15, 495) );
	check( ! irrSnap.contains(495, 495) );
	
	 
}

