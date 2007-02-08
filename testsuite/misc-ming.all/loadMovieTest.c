/***********************************************************************
 *
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
 *
 * You should have received a copy of the GNU General Public License
 *
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 ***********************************************************************
 *
 * Test case for the MovieClip.loadMovie actionscript method
 *
 ***********************************************************************/

/*
 * run as ./loadMovieTest <mediadir>
 *
 * mediadir is where lynch.{jpg,swf}, green.{jpg,swf}
 * and offspring.{jpg,swf} are located
 *
 */

#include "ming_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <ming.h>

// In version 7 loadMovie doesn't work !!
#define OUTPUT_VERSION 6
#define OUTPUT_FILENAME "loadMovieTest.swf"

void add_clip(SWFMovie mo, char* file, char* name, char* url, int x, int y);

void
add_clip(SWFMovie mo, char* file, char* name,
		char* url, int x, int y)
{
	FILE *fd;
	SWFJpegBitmap bm;
	SWFShape sh;
	SWFMovieClip mc, mct;
	SWFDisplayItem it;
	SWFAction ac;
	char action[1024];

	printf("Adding %s\n", file);

	fd = fopen(file, "r");
	if ( ! fd ) {
		perror(file);
		exit(1);
	}
	bm = newSWFJpegBitmap(fd);
	sh = newSWFShapeFromBitmap((SWFBitmap)bm, SWFFILL_CLIPPED_BITMAP);
	mc = newSWFMovieClip();
	SWFMovieClip_add(mc, (SWFBlock)sh);
	SWFMovieClip_nextFrame(mc); /* showFrame */
	it = SWFMovie_add(mo, (SWFBlock)mc);
	SWFDisplayItem_setName(it, name);
	SWFDisplayItem_moveTo(it, x, y);

	/* "Click" handler */
	sprintf(action, " \
%s.onPress = function () { \
	coverart.loadMovie('%s'); \
}; \
", name, url, url);

	ac = compileSWFActionCode(action);

	SWFMovie_add(mo, (SWFBlock)ac);
}

int
main(int argc, char** argv)
{
	SWFMovie mo;
	SWFMovieClip dejagnuclip;
	char file_lynch[256];
	char file_green[256];
	char file_offspring[256];
	char url_lynch[256];
	char url_green[256];
	char url_offspring[256];
	const char *srcdir=".";
	SWFShape sh_coverart;
	SWFMovieClip mc_coverart;
	SWFFillStyle fstyle;
	SWFDisplayItem it;
	SWFAction ac;

	/*********************************************
	 *
	 * Initialization
	 *
	 *********************************************/

	if ( argc>1 ) srcdir=argv[1];
	else
	{
		fprintf(stderr, "Usage: %s <mediadir>\n", argv[0]);
		return 1;
	}

	sprintf(file_lynch, "%s/lynch.jpg", srcdir);
	sprintf(file_green, "%s/green.jpg", srcdir);
	sprintf(file_offspring, "%s/offspring.jpg", srcdir);

	/*
	 * Test urls with and w/out 'file://' prefix.
	 * Test both jpeg and swf loading.
	 */
	sprintf(url_lynch, "file://%s/lynch.swf", srcdir);
	sprintf(url_green, "file://%s/green.jpg", srcdir);
	sprintf(url_offspring, "%s/offspring.swf", srcdir);


	puts("Setting things up");

	Ming_init();
        Ming_useSWFVersion (OUTPUT_VERSION);
	Ming_setScale(20.0); /* so we talk pixels */
 
	mo = newSWFMovie();
        SWFMovie_setDimension (mo, 800, 600); 
        SWFMovie_setRate (mo, 12.0); 
        SWFMovie_setBackground (mo, 255, 255, 255); 

	/*****************************************************
	 *
	 * Add Dejagnu clip
	 *
	 *****************************************************/

	dejagnuclip = get_dejagnu_clip((SWFBlock)get_default_font(srcdir), 10, 0, 0, 800, 600);
	//it = SWFMovie_add(mo, (SWFBlock)dejagnuclip);
	//SWFDisplayItem_moveTo(it, 0, 200);
	//SWFMovie_nextFrame(mo); 

	/*****************************************************
	 *
	 * Add the clips
	 *
	 *****************************************************/

	/* Add the LYNCH  clip */
	add_clip(mo, file_lynch, "lynch", url_lynch, 0, 0);

	/* Add the GREEN  clip */
	add_clip(mo, file_green, "green", url_green, 200, 0);

	/* Add the OFFSPRING  clip */
	add_clip(mo, file_offspring, "offspring", url_offspring, 400, 0);

	/*****************************************************
	 *
	 * Add the coverart clip
	 *
	 *****************************************************/

	puts("Adding coverart");

	sh_coverart = newSWFShape();
	fstyle = SWFShape_addSolidFillStyle(sh_coverart, 0,0,0,255);
	SWFShape_setRightFillStyle(sh_coverart, fstyle);
	SWFShape_movePenTo(sh_coverart, 170, 170);
	SWFShape_drawLine(sh_coverart, -170, 0);
	SWFShape_drawLine(sh_coverart, 0, -170);
	SWFShape_drawLine(sh_coverart, 170, 0);
	SWFShape_drawLine(sh_coverart, 0, 170);

	mc_coverart = newSWFMovieClip();
	SWFMovieClip_add(mc_coverart, (SWFBlock)sh_coverart);
	SWFMovieClip_nextFrame(mc_coverart); /* showFrame */
	it = SWFMovie_add(mo, (SWFBlock)mc_coverart);
	SWFDisplayItem_setName(it, "coverart"); 
	SWFDisplayItem_moveTo(it, 500, 200);

	add_actions(mo, "stop();");
	SWFMovie_nextFrame(mo); /* showFrame */

	/*****************************************************
	 *
	 * Output movie
	 *
	 *****************************************************/

	puts("Saving " OUTPUT_FILENAME );

	SWFMovie_save(mo, OUTPUT_FILENAME);

	return 0;
}
