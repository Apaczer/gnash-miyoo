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

#define INPUT_FILENAME "gravity-embedded.swf"

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
	string filename = string(SRCDIR) + string("/") + string(INPUT_FILENAME);
	MovieTester tester(filename);

	// TODO: check why we need this !!
	//       I wouldn't want the first advance to be needed
	tester.advance();

	dbglogfile.setVerbosity(1);

	sprite_instance* root = tester.getRootMovie();
	assert(root);

	//const DisplayList& dl = root->getDisplayList();
	//dl.dump(std::cout);

	check_equals(root->get_frame_count(), 1);

	// give loader time to load the actual gravity.swf movie ?
	//sleep(1);

	// used to get members
	as_value tmp;

	const character* loaded = tester.findDisplayItemByDepth(*root, 8);
	check(loaded);
	check_equals(loaded->get_parent(), root);

	// we need a const_cast as get_member *might* eventually
	// change the character (fetching _x shouldn't change it though)
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(tmp, as_value("50"));

	check_equals(loaded->get_height(), 2056);
	check_equals(loaded->get_width(), 2056);

	const character* text = tester.findDisplayItemByDepth(*root, 7);
	check(text);
	
	check_equals(string(text->get_text_value()), "50");
	check(!tester.isMouseOverMouseEntity());

	// click some on the "smaller" button
	tester.movePointerTo(474, 18);
	check(tester.isMouseOverMouseEntity());
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "50");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "48");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 48);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 48);
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "48");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "46");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 46);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 46);
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "46");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "44");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 44);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 44);

	// click some on the "larger" button
	tester.movePointerTo(580, 18);
	check(tester.isMouseOverMouseEntity());
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "44");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "46");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 46);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 46);
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "46");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "48");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 48);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 48);
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "48");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "50");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 50);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 50);
	tester.pressMouseButton();
	check_equals(string(text->get_text_value()), "50");
	tester.depressMouseButton();
	check_equals(string(text->get_text_value()), "52");
	check(const_cast<character*>(loaded)->get_member("_xscale", &tmp));
	check_equals(round(tmp.to_number()), 52);
	check(const_cast<character*>(loaded)->get_member("_yscale", &tmp));
	check_equals(round(tmp.to_number()), 52);


}

