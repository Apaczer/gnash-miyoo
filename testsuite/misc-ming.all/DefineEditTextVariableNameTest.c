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

/*
 * Test DefineEditText tag with VariableName
 *
 * Uses "embedded" font with chars: "Hello world"
 *
 * Then, every second it toggles the text between "Hello",
 * "World" and "" (the empty string) by setting the VariableName.
 * 
 * After every variable set it also traces the value of the
 * VariableName, of the textfield and of it's 'text' member.
 * Note that the traces are both "visual" and normal ("visual"
 * traces use the drawing API).
 *
 * The EditText character is stored inside a MovieClip, and
 * it's variable is set on the root. Note that the ActionScript
 * code also tries to *move* the character trough the variable
 * (incdement varname._x).
 * The correct behaviour is for the character to NOT move
 *
 *
 * run as ./DefineEditTextVariableNameTest
 */

#include <stdlib.h>
#include <stdio.h>
#include <ming.h>

#include "ming_utils.h"

#define OUTPUT_VERSION 6
#define OUTPUT_FILENAME "DefineEditTextVariableNameTest.swf"

void add_text_field(SWFMovieClip mo, SWFBlock font, const char* varname, const char* text);
void set_text(SWFMovie mo, const char* txt, const char* mcname, const char* varname);
void shift_horizontally(SWFMovie mo, const char* what, int howmuch);
void set_x(SWFMovie mo, const char* what, int x);

void
shift_horizontally(SWFMovie mo, const char* what, int howmuch)
{ 
	static const size_t BUFLEN = 256;

	char buf[BUFLEN];
	SWFAction ac;

	snprintf(buf, BUFLEN, "%s._x += %d;", what, howmuch);
	buf[BUFLEN-1] = '\0';

	ac = compileSWFActionCode(buf);
	SWFMovie_add(mo, (SWFBlock)ac);
}

void
set_x(SWFMovie mo, const char* what, int x)
{ 
	static const size_t BUFLEN = 256;

	char buf[BUFLEN];
	SWFAction ac;

	snprintf(buf, BUFLEN, "%s._x = %d;", what, x);
	buf[BUFLEN-1] = '\0';

	ac = compileSWFActionCode(buf);
	SWFMovie_add(mo, (SWFBlock)ac);
}


void
set_text(SWFMovie mo, const char* txt, const char* mcname, const char* varname)
{
	static const size_t BUFLEN = 512;

	char buf[BUFLEN];
	SWFAction ac;
	snprintf(buf, BUFLEN, "%s = \"%s\"; ",
		varname, txt);
	buf[BUFLEN-1] = '\0';
	ac = compileSWFActionCode(buf);
	SWFMovie_add(mo, (SWFBlock)ac);
}

void
add_text_field(SWFMovieClip mo, SWFBlock font, const char* varname,
		const char* text)
{
	SWFTextField tf;
	SWFDisplayItem it;

	tf = newSWFTextField();

	SWFTextField_setFlags(tf, SWFTEXTFIELD_DRAWBOX);


	SWFTextField_setFont(tf, font);
	SWFTextField_addChars(tf, text);
	SWFTextField_addString(tf, text);

	/* Give the textField a variablename*/
	SWFTextField_setVariableName(tf, varname);

	it = SWFMovieClip_add(mo, (SWFBlock)tf);
	SWFDisplayItem_setName(it, "textfield");

	SWFMovieClip_nextFrame(mo);

}

int
main(int argc, char** argv)
{
	SWFMovie mo;
	SWFMovieClip mc1, mc2, mc3;
	const char *srcdir=".";
	char fdbfont[256];
	/* The variable name for textfield */
	char* varName1 = "_root.testName";
	char* varName2 = "_root.mc3.testName";
	FILE *font_file;
	/*SWFBrowserFont bfont; */
	SWFFont bfont; 


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

	sprintf(fdbfont, "%s/Bitstream Vera Sans.fdb", srcdir);

	puts("Setting things up");

	Ming_init();
        Ming_useSWFVersion (OUTPUT_VERSION);
	Ming_setScale(20.0); /* let's talk pixels */
 
	mo = newSWFMovie();
	SWFMovie_setRate(mo, 1);
	SWFMovie_setDimension(mo, 628, 451);

	font_file = fopen(fdbfont, "r");
	if ( font_file == NULL )
	{
		perror(fdbfont);
		exit(1);
	}
	/*SWFBrowserFont bfont = newSWFBrowserFont("_sans");*/
	bfont = loadSWFFontFromFile(font_file);


	/***************************************************
	 *
	 * Add MovieClips mc1 and mc2 with their textfield
	 *
	 ***************************************************/

	{
		SWFDisplayItem it;
		mc1 = newSWFMovieClip();
		add_text_field(mc1, (SWFBlock)bfont, varName1, "Hello World");
		it = SWFMovie_add(mo, (SWFBlock)mc1);
		SWFDisplayItem_setName(it, "mc1");
		set_x(mo, "mc1.textfield", 0);
	}

	{
		SWFDisplayItem it;
		mc2 = newSWFMovieClip();
		add_text_field(mc2, (SWFBlock)bfont, varName2, "Hi There");
		it = SWFMovie_add(mo, (SWFBlock)mc2);
		SWFDisplayItem_setName(it, "mc2");
		set_x(mo, "mc2.textfield", 150);
	}

	/*********************************************
	 *
	 * Now add mc3, which is referenced by
	 * mc2.textfield variableName (after it's placement)
	 *
	 *********************************************/

	{
		SWFDisplayItem it;
		mc3 = newSWFMovieClip();
		it = SWFMovie_add(mo, (SWFBlock)mc3);
		SWFDisplayItem_setName(it, "mc3");
		/*SWFMovie_nextFrame(mo);*/
	}



	/*********************************************
	 *
	 * Add xtrace code
	 *
	 *********************************************/

	/*add_xtrace_function(mo, 3000, 0, 50, 400, 800);*/
	add_dejagnu_functions(mo, bfont, 3000, 0, 50, 400, 800);

	/*********************************************
	 *
	 * Set and get value of the textfields using
	 * their variableName
	 *
	 *********************************************/

	set_text(mo, "Hello", "mc1", varName1);
	shift_horizontally(mo, varName1, 10);
	check_equals(mo, "mc1.textfield.text", "'Hello'", 0);
	check_equals(mo, varName1, "'Hello'", 0);
	check_equals(mo, "mc1.textfield._x", "0", 0);
	check_equals(mo, "mc1._height", "16", 0);
	check_equals(mo, "mc1._width", "136", 0);
	check_equals(mo, "mc2._height", "16", 0);
	check_equals(mo, "mc2._width", "100", 0);

	set_text(mo, "Hi", "mc2", varName2);
	shift_horizontally(mo, varName2, 10);
	check_equals(mo, "mc2.textfield.text", "'Hi'", 0); 
	check_equals(mo, varName2, "'Hi'", 0);
	check_equals(mo, "mc2.textfield._x", "150", 0);

	SWFMovie_nextFrame(mo); /* showFrame */

	set_text(mo, "World", "mc1", varName1);
	shift_horizontally(mo, varName1, 10);
	check_equals(mo, "mc1.textfield.text", "'World'", 0);
	check_equals(mo, varName1, "'World'", 0);
	check_equals(mo, "mc1.textfield._x", "0", 0);

	set_text(mo, "There", "mc2", varName2);
	shift_horizontally(mo, varName2, 10);
	check_equals(mo, "mc2.textfield.text", "'There'", 0);
	check_equals(mo, varName2, "'There'", 0);
	check_equals(mo, "mc2.textfield._x", "150", 0);

	SWFMovie_nextFrame(mo); /* showFrame */

	set_text(mo, "", "mc1", varName1);
	shift_horizontally(mo, varName1, 10);
	check_equals(mo, "mc1.textfield.text", "''", 0);
	check_equals(mo, varName1, "''", 0);
	check_equals(mo, "mc1.textfield._x", "0", 0);

	set_text(mo, "", "mc2", varName2);
	shift_horizontally(mo, varName2, 10);
	check_equals(mo, "mc2.textfield.text", "''", 0);
	check_equals(mo, varName2, "''", 0);
	check_equals(mo, "mc2.textfield._x", "150", 0);

	print_tests_summary(mo);

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
