// SoundChannel_as.hx:  ActionScript 3 "SoundChannel" class, for Gnash.
//
// Generated by gen-as3.sh on: 20090503 by "rob". Remove this
// after any hand editing loosing changes.
//
//   Copyright (C) 2009 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

// This test case must be processed by CPP before compiling to include the
//  DejaGnu.hx header file for the testing framework support.

import flash.media.SoundChannel;
import flash.media.SoundTransform;
import flash.display.MovieClip;
import flash.Lib;
import Type;

import DejaGnu;

// Class must be named with the PP prefix, as that's the name the
// file passed to haxe will have after the preprocessing step
class SoundChannel_as {
    static function main() {
        var x1:SoundChannel = new SoundChannel();

        // Make sure we actually get a valid class        
        if (x1 != null) {
            DejaGnu.pass("SoundChannel class exists");
        } else {
            DejaGnu.fail("SoundChannel lass doesn't exist");
        }
// Tests to see if all the properties exist. All these do is test for
// existance of a property, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (x1.leftPeak == 0) {
	    DejaGnu.pass("SoundChannel::leftPeak property exists");
	} else {
	    DejaGnu.fail("SoundChannel::leftPeak property doesn't exist");
	}
	if (x1.position == 0) {
	    DejaGnu.pass("SoundChannel::position property exists");
	} else {
	    DejaGnu.fail("SoundChannel::position property doesn't exist");
	}
	if (x1.rightPeak == 0) {
	    DejaGnu.pass("SoundChannel::rightPeak property exists");
	} else {
	    DejaGnu.fail("SoundChannel::rightPeak property doesn't exist");
	}
	if (x1.soundTransform == SoundTransform) {
	    DejaGnu.pass("SoundChannel::soundTransform property exists");
	} else {
	    DejaGnu.fail("SoundChannel::soundTransform property doesn't exist");
	}

// Tests to see if all the methods exist. All these do is test for
// existance of a method, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (x1.stop == null) {
	    DejaGnu.pass("SoundChannel::stop() method exists");
	} else {
	    DejaGnu.fail("SoundChannel::stop() method doesn't exist");
	}

        // Call this after finishing all tests. It prints out the totals.
        DejaGnu.done();
    }
}
