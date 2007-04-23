/* 
 *   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 */ 

// TODO: fix invalidated bounds, which are clearly bogus !

#define INPUT_FILENAME "simple_loop_test.swf"

#include "MovieTester.h"
#include "sprite_instance.h"
#include "character.h"
#include "dlist.h"
#include "container.h"
#include "log.h"

#include "check.h"
#include <string>
#include <cassert>

using namespace gnash;
using namespace std;

int
main(int /*argc*/, char** /*argv*/)
{
	typedef gnash::geometry::Range2d<int> Bounds;

	string filename = string(TGTDIR) + string("/") + string(INPUT_FILENAME);
	MovieTester tester(filename);

	gnash::LogFile& dbglogfile = gnash::LogFile::getDefaultInstance();
	dbglogfile.setVerbosity(1);

	Bounds invalidated;
	sprite_instance* root = tester.getRootMovie();
	assert(root);

	// FRAME 1/4 (start)

	check_equals(root->get_frame_count(), 4);
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 0);
	check_equals(root->getDisplayList().size(), 0); // no chars
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.isNull() );

	tester.advance(); // FRAME 2/4
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 1);
	check_equals(root->getDisplayList().size(), 1);
	check( tester.findDisplayItemByDepth(*root, 2+character::staticDepthOffset) );
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(0, 0, 60, 60)) );

	tester.advance(); // FRAME 3/4
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 2);
	check_equals(root->getDisplayList().size(), 2);
	check( tester.findDisplayItemByDepth(*root, 2+character::staticDepthOffset) );
	check( tester.findDisplayItemByDepth(*root, 3+character::staticDepthOffset) );
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(60, 0, 120, 60)) );

	tester.advance(); // FRAME 4/4
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 3);
	check_equals(root->getDisplayList().size(), 3);
	check( tester.findDisplayItemByDepth(*root, 2+character::staticDepthOffset) );
	check( tester.findDisplayItemByDepth(*root, 3+character::staticDepthOffset) );
	check( tester.findDisplayItemByDepth(*root, 4+character::staticDepthOffset) );
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(120, 0, 180, 60)) );

	tester.advance(); // FRAME 1/4 (loop back)
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 0);
	check_equals(root->getDisplayList().size(), 0);
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(0, 0, 180, 60)) );

	tester.advance(); // FRAME 2/4
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 1);
	check_equals(root->getDisplayList().size(), 1);
	check( tester.findDisplayItemByDepth(*root, 2+character::staticDepthOffset) );
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(0, 0, 60, 60)) );

	tester.advance(); // FRAME 3/4
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 2);
	check_equals(root->getDisplayList().size(), 2);
	check( tester.findDisplayItemByDepth(*root, 2+character::staticDepthOffset) );
	check( tester.findDisplayItemByDepth(*root, 3+character::staticDepthOffset) );
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(60, 0, 120, 60)) );

	tester.advance(); // FRAME 4/4
	
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 3);
	check_equals(root->getDisplayList().size(), 3);
	check( tester.findDisplayItemByDepth(*root, 2+character::staticDepthOffset) );
	check( tester.findDisplayItemByDepth(*root, 3+character::staticDepthOffset) );
	check( tester.findDisplayItemByDepth(*root, 4+character::staticDepthOffset) );
	invalidated = tester.getInvalidatedBounds();
	check( invalidated.contains(Bounds(120, 0, 180, 60)) );

}

