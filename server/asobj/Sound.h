// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

#ifndef __SOUND_H__
#define __SOUND_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "impl.h"
#include "as_object.h" // for inheritance
#include "NetConnection.h"

namespace gnash {

// Forward declarations
class fn_call;
  
class Sound : public as_object {
public:
	Sound();
	~Sound();
	virtual void attachSound(int si, const char* name);
	virtual void getBytesLoaded();
	virtual void getBytesTotal();
	virtual void getPan();
	virtual void getTransform();
	virtual int getVolume();
	virtual void loadSound(std::string file, bool streaming);
	virtual void setPan();
	virtual void setTransform();
	virtual void setVolume(int volume);
	virtual void start(int offset, int loops);
	virtual void stop(int si);
	virtual unsigned int getDuration();
	virtual unsigned int getPosition();

	std::string soundName;

protected:
	bool _duration;
	bool _id3;
	bool _onID3;
	bool _onLoad;
	bool _onComplete;
	bool _position;
	NetConnection* connection;

	int soundId;
	bool externalSound;
	std::string externalURL;
	bool isStreaming;
};

void sound_class_init(as_object& global);

} // end of gnash namespace

// __SOUND_H__
#endif

