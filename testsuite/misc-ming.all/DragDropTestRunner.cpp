/* 
 *   Copyright (C) 2007 Free Software Foundation, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 */ 

#define INPUT_FILENAME "DragDropTest.swf"

#include "MovieTester.h"
#include "sprite_instance.h"
#include "character.h"
#include "dlist.h"
#include "log.h"
#include "Point2d.h"
#include "VM.h"
#include "string_table.h"

#include "check.h"
#include <string>
#include <cassert>
#include <sstream>

using namespace gnash;
using namespace gnash::geometry;
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

	// for variables lookup (consistency checking)
	string_table& st = root->getVM().getStringTable();

	typedef geometry::Point2d<int> IntPoint;

	rgba white(255, 255, 255, 255); // background color
	rgba blue(0, 0, 255, 255);      // blue circles fill color
	rgba green(0, 255, 0, 255);     // green circles fill color
	rgba red(255, 0, 0, 255);       // red circles fill color

	int cs = 20;             // circles size

	IntPoint out(350, 100);  // out of any drawing

	IntPoint rc1(50, 50);    // first red circle
	IntPoint gc1(100, 50);   // first green circle
	IntPoint bc1(70, 100);   // first blue circle

	IntPoint rc2(150, 50);   // second red circle
	IntPoint gc2(200, 50);   // second green circle
	IntPoint bc2(170, 100);  // second blue circle

	IntPoint rc3(250, 50);   // third red circle
	IntPoint gc3(300, 50);   // third green circle
	IntPoint bc3(270, 100);  // third blue circle

	check_equals(root->get_frame_count(), 2);
	check_equals(root->get_current_frame(), 0);

	// first frame is just Dejagnu clip...
	tester.advance();

	check_equals(root->get_current_frame(), 1);

	// Wait for _level50 and loadedTarget to be loaded...
	unsigned long sleepTime = 100000; // microseconds
	unsigned int attempts=10;
	while (1)
	{
		// if _root displaylist contains loadedTarget and loadedTarget
		// contains target100, we've loaded it
		const sprite_instance* loadedTarget = 0;
		//const character* ch = tester.findDisplayItemByName(*root, "loadedTarget");
		const character* ch = tester.findDisplayItemByDepth(*root, 30);
		if ( ch ) loadedTarget = ch->to_movie();
		if ( loadedTarget )
		{
			const character* target100 = tester.findDisplayItemByName(*loadedTarget, "target100");
			if ( target100 ) break;
			else cerr << "target100 not yet found in loadedTarget" << endl;
		}
		else
		{
			cerr << "loadedTarget not yet not found" << endl;
		}

		if ( ! attempts-- )
		{
			check(!"loadTarget was never loaded");
			cerr << "Root display list is: " << endl;
			root->getDisplayList().dump();
			exit(EXIT_FAILURE);
		}
		usleep(sleepTime);
	}


	// 1. Click OUTSIDE of any drawing.
	tester.movePointerTo(out.x, out.y);
	tester.click();

	// 2. Click on the FIRST RED circle.
	tester.movePointerTo(rc1.x, rc1.y);
	tester.click(); 

	// 3. Click on the FIRST GREEN circle.
	tester.movePointerTo(gc1.x, gc1.y);
	tester.click(); 

	// 4. Click on the FIRST BLUE circle.
	tester.movePointerTo(bc1.x, bc1.y);
	tester.click(); 

	// 5. Click on the SECOND RED circle.
	tester.movePointerTo(rc2.x, rc2.y);
	tester.click(); 

	// 6. Click on the SECOND GREEN circle.
	tester.movePointerTo(gc2.x, gc2.y);
	tester.click(); 

	// 7. Click on the SECOND BLUE circle.
	tester.movePointerTo(bc2.x, bc2.y);
	tester.click(); 

	// 8. Click on the THIRD RED circle.
	tester.movePointerTo(rc3.x, rc3.y);
	tester.click(); 

	// 9. Click on the THIRD GREEN circle.
	tester.movePointerTo(gc3.x, gc3.y);
	tester.click(); 

	// 10. Click on the THIRD BLUE circle.
	tester.movePointerTo(bc3.x, bc3.y);
	tester.click(); 

	// 11. Click OUTSIDE of any drawing (this is last thing).
	tester.movePointerTo(out.x, out.y);
	tester.click();

	// Consistency check !!
	as_value eot;
	// It's an swf6, so lowercase 'ENDOFTEST'
	bool endOfTestFound = root->get_member(st.find("endoftest"), &eot);
	check(endOfTestFound);
	if ( endOfTestFound )
	{
		cerr << eot.to_debug_string() << endl;
		check_equals(eot.to_bool(), true);
	}
	else
	{
		cerr << "Didn't find ENDOFTEST... dumping all members" << endl;
		root->dump_members();
	}

}

