// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

/* $Id: Key.h,v 1.12 2007/03/19 17:11:14 bjacques Exp $ */

#ifndef __KEY_H__
#define __KEY_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tu_config.h"
#include "as_object.h" // for inheritance
#include "fn_call.h"
#include "gnash.h" // for gnash::key namespace

#ifdef WIN32
#	undef _CONTROL
#	undef _SPACE
#	undef _UP
#endif

namespace gnash {
  
class DSOEXPORT Key {
public:
    Key();
    ~Key();
   void addListener();
   void getAscii();
   void getCode();
   void isDown();
   void isToggled();
   void removeListener();
private:
    bool _BACKSPACE;
    bool _CAPSLOCK;
    bool _CONTROL;
    bool _DELETEKEY;
    bool _DOWN;
    bool _END;
    bool _ENTER;
    bool _ESCAPE;
    bool _HOME;
    bool _INSERT;
    bool _LEFT;
    bool _onKeyDown;
    bool _onKeyUp;
    bool _PGDN;
    bool _PGUP;
    bool _RIGHT;
    bool _SHIFT;
    bool _SPACE;
    bool _TAB;
    bool _UP;
};

//class key_as_object : public as_object
//{
//public:
    //Key obj;
//};

as_value key_addlistener(const fn_call& fn);
as_value key_getascii(const fn_call& fn);
as_value key_getcode(const fn_call& fn);
as_value key_isdown(const fn_call& fn);
as_value key_istoggled(const fn_call& fn);
as_value key_removelistener(const fn_call& fn);

/************************************************************************
 *
 * This has been moved from action.cpp, when things are clean
 * everything should have been moved up
 *
 ************************************************************************/

class DSOEXPORT key_as_object : public as_object
{

private:
	uint8_t	m_keymap[key::KEYCOUNT / 8 + 1];	// bit-array
	std::vector<boost::intrusive_ptr<as_object> >	m_listeners;
	int	m_last_key_pressed;

	void notify_listeners(const std::string& funcname);

public:

	key_as_object();

	bool is_key_down(int code);

	void set_key_down(int code);

	void set_key_up(int code);

	void add_listener(as_object* listener);

	void remove_listener(as_object* listener);

	int get_last_key_pressed() const;
};

void key_class_init(as_object& global);

} // end of gnash namespace

// __KEY_H__
#endif

