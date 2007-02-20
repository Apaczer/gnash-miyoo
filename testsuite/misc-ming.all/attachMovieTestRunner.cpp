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

#define INPUT_FILENAME "attachMovieTest.swf"

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
	string filename = string(TGTDIR) + string("/") + string(INPUT_FILENAME);
	MovieTester tester(filename);

	gnash::LogFile& dbglogfile = gnash::LogFile::getDefaultInstance();
	dbglogfile.setVerbosity(1);

	sprite_instance* root = tester.getRootMovie();
	assert(root);

	as_value tmp;

	check_equals(root->get_frame_count(), 5);
	check_equals(root->get_play_state(), sprite_instance::PLAY);
	check_equals(root->get_current_frame(), 0);

	check(! tester.findDisplayItemByDepth(*root, 70) );
	check(! tester.findDisplayItemByDepth(*root, 71) );
	check(! tester.findDisplayItemByDepth(*root, 72) );
	check(! tester.findDisplayItemByDepth(*root, 73) );

	tester.movePointerTo(30, 30);
	check(!tester.isMouseOverMouseEntity());

	tester.advance();

	check(tester.findDisplayItemByDepth(*root, 70) );
	check(! tester.findDisplayItemByDepth(*root, 71) );
	check(! tester.findDisplayItemByDepth(*root, 72) );
	check(! tester.findDisplayItemByDepth(*root, 73) );

	tester.movePointerTo(30, 30);
	check(tester.isMouseOverMouseEntity());
	tester.movePointerTo(100, 30);
	check(!tester.isMouseOverMouseEntity());

	root->get_member("mousedown", &tmp);
	check(tmp.is_undefined());
	root->get_member("mouseup", &tmp);
	check(tmp.is_undefined());

	// Note that we are *not* on an active entity !
	tester.pressMouseButton();

	root->get_member("mousedown", &tmp);
	check_equals(tmp.to_number(), 1);
	check ( ! root->get_member("mouseup", &tmp) );

	tester.depressMouseButton();

	root->get_member("mousedown", &tmp);
	check_equals(tmp.to_number(), 1);
	root->get_member("mouseup", &tmp);
	check_equals(tmp.to_number(), 1);

	tester.advance();

	check( tester.findDisplayItemByDepth(*root, 70) );
	check( tester.findDisplayItemByDepth(*root, 71) );
	check(! tester.findDisplayItemByDepth(*root, 72) );
	check(! tester.findDisplayItemByDepth(*root, 73) );

	tester.movePointerTo(100, 30);
	check(tester.isMouseOverMouseEntity());
	tester.movePointerTo(170, 30);
	check(!tester.isMouseOverMouseEntity());

	tester.advance();

	check( tester.findDisplayItemByDepth(*root, 70) );
	check( tester.findDisplayItemByDepth(*root, 71) );
	check( tester.findDisplayItemByDepth(*root, 72) );
	check(! tester.findDisplayItemByDepth(*root, 73) );

	tester.movePointerTo(170, 30);
	check(tester.isMouseOverMouseEntity());
	tester.movePointerTo(240, 30);
	check(!tester.isMouseOverMouseEntity());

	tester.advance();

	check( tester.findDisplayItemByDepth(*root, 70) );
	check( tester.findDisplayItemByDepth(*root, 71) );
	check( tester.findDisplayItemByDepth(*root, 72) );
	check( tester.findDisplayItemByDepth(*root, 73) );

	tester.movePointerTo(240, 30);
	check(tester.isMouseOverMouseEntity());

	tester.movePointerTo(340, 30);
	check(! tester.isMouseOverMouseEntity());

	// Note that we are *not* on an active entity !
	tester.pressMouseButton();

	root->get_member("mousedown", &tmp);
	check_equals(tmp.to_number(), 5);

}

