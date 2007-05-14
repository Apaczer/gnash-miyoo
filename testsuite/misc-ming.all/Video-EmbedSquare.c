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
 * Plays video stored internal in the SWF.
 * Should be used with the MovieTester to test if the video decoder works.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ming.h>

#include "ming_utils.h"

#define OUTPUT_VERSION 7
#define OUTPUT_FILENAME "Video-EmbedSquare.swf"


int
main(int argc, char** argv)
{
  SWFMovie mo;

  Ming_init();
  Ming_useSWFVersion (OUTPUT_VERSION);

  int frames;
  SWFVideoStream stream;
  SWFDisplayItem item;
  FILE *flv;

  flv = fopen("../media/square.flv", "rb");
  if (flv == NULL) return -1;
	
  mo = newSWFMovie();
  SWFMovie_setDimension(mo, 128, 96);

  if (mo == NULL) return -1;

  SWFMovie_setRate(mo, 5);

  stream = newSWFVideoStream_fromFile(flv);
  item = SWFMovie_add(mo, (SWFBlock)stream);

  // TODO: dynamic frame rate adjust
  frames = SWFVideoStream_getNumFrames(stream);
  for(; frames > 0; frames--)
    SWFMovie_nextFrame(mo);

  //Output movie
  puts("Saving " OUTPUT_FILENAME );
  SWFMovie_save(mo, OUTPUT_FILENAME);

  return 0;
}
