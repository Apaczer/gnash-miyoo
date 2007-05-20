/* 
 *   Copyright (C) 2007 Free Software Foundation, Inc.
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
 */ 

/*
 * Zou Lunkai, zoulunkai@gmail.com
 *
 * Test "Jumping backward with dynamic instances in static depth zone removed"
 *
 * run as ./displaylist_depths_test9
 *
 * Timeline:
 * 
 *   Frame  | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
 *  --------+---+---+---+---+---+---+---+
 *   Event  |   |PPp| p | p | * | p | J |
 * 
 *  P = place (by PlaceObject2)
 *  p = place (by ActionScript)
 *  J = jump
 *  * = jump target
 * 
 * Description:
 * 
 *  frame2: place a static character at depth -16381 at position (10,200);
 *          place a static character at depth -16380 at position (100,200);
 *          replace the character at depth -16380 with a dynamic character;
 *  frame3: create a script character at depth -10;
 *  frame4: create a script character at depth -20;
 *  frame6: create a script character at depth -30;
 *  frame7: jump back to frame 5 and stop
 * 
 * Expected behaviour:
 * 
 *  Before the jump we have 5 instances.
 *  After the jump only two timeline instances keep alive, all dynamic instances get removed;
 *  Seven instances have been constructed in total.
 */

#include "ming_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <ming.h>

#define OUTPUT_VERSION 6
#define OUTPUT_FILENAME "displaylist_depths_test9.swf"

SWFDisplayItem add_static_mc(SWFMovie mo, const char* name, int depth, int x, int y, int width, int height);

SWFDisplayItem
add_static_mc(SWFMovie mo, const char* name, int depth, int x, int y, int width, int height)
{
  SWFShape sh;
  SWFMovieClip mc, mc2;
  SWFDisplayItem it;

  sh = make_fill_square (-(width/2), -(height/2), width, height, 255, 0, 0, 255, 0, 0);
  mc = newSWFMovieClip();
  SWFMovieClip_add(mc, (SWFBlock)sh);

  SWFMovieClip_nextFrame(mc);

  it = SWFMovie_add(mo, (SWFBlock)mc);
  SWFDisplayItem_setDepth(it, depth); 
  SWFDisplayItem_moveTo(it, x, y); 
  SWFDisplayItem_setName(it, name);

  return it;
}


int
main(int argc, char** argv)
{
  SWFMovie mo;
  SWFMovieClip dejagnuclip;
  int i;
  SWFDisplayItem it1, it2;


  const char *srcdir=".";
  if ( argc>1 ) 
    srcdir=argv[1];
  else
  {
      //fprintf(stderr, "Usage: %s <mediadir>\n", argv[0]);
      //return 1;
  }

  Ming_init();
  mo = newSWFMovieWithVersion(OUTPUT_VERSION);
  SWFMovie_setDimension(mo, 800, 600);
  SWFMovie_setRate (mo, 2);

  dejagnuclip = get_dejagnu_clip((SWFBlock)get_default_font(srcdir), 10, 0, 0, 800, 600);
  SWFMovie_add(mo, (SWFBlock)dejagnuclip);
  SWFMovie_nextFrame(mo); 

  // Frame 2: 
  //   Add a "static3" at depth 3 with origin at 10,200.
  //   Add a "static4" at depth 4 with origin at 100,200.
  //   Replace "static4" with a dynamic movieClip "dup0". 
  it1 = add_static_mc(mo, "static3", 3, 10, 200, 20, 20);
  check_equals(mo, "static3.getDepth()", "-16381");
  it2 = add_static_mc(mo, "static4", 4, 100, 200, 20, 20);
  check_equals(mo, "static4.getDepth()", "-16380");
  add_actions(mo, "duplicateMovieClip('/static4', 'dup0', -16380);");
  check_equals(mo, "dup0.getDepth()", "-16380");
  SWFMovie_nextFrame(mo); 

  // Frame 3: create a script instance at depth -10.
  check_equals(mo, "dup0.getDepth()", "-16380");
  add_actions(mo,  "duplicateMovieClip('static3', 'dup1', -10);");
  check_equals(mo, "dup1.getDepth()", "-10");
  SWFMovie_nextFrame(mo); 
 
  // Frame 4: create a script instance at depth -20.
  add_actions(mo, 
    "duplicateMovieClip('/:static3', 'dup2', -20);"
    "check_equals(dup2.getDepth(), -20);"); 
  SWFMovie_nextFrame(mo); 

  // Frame 5: nothing new
  SWFMovie_nextFrame(mo); 
  
  // Frame 6: create a script instance at depth -30.
  add_actions(mo, 
    "duplicateMovieClip('_root.static3', 'dup3', -30);"
    "check_equals(dup3.getDepth(), -30);");
  SWFMovie_nextFrame(mo); 

  // Frame 7: go to frame 5 and checks
  add_actions(mo,  "gotoAndStop(5);");
  check_equals(mo, "typeof(static3)", "'movieclip'");
  check_equals(mo, "typeof(static4)", "'movieclip'");
  check_equals(mo, "typeof(dup0)", "'undefined'");
  check_equals(mo, "typeof(dup1)", "'undefined'");
  check_equals(mo, "typeof(dup2)", "'undefined'");
  check_equals(mo, "typeof(dup3)", "'undefined'");
  add_actions(mo,  "totals();");
  SWFMovie_nextFrame(mo); 

  //Output movie
  puts("Saving " OUTPUT_FILENAME );
  SWFMovie_save(mo, OUTPUT_FILENAME);

  return 0;
}
