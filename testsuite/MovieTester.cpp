/* 
 *   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
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

#include "MovieTester.h"
#include "GnashException.h"
#include "URL.h"
#include "noseek_fd_adapter.h"
#include "movie_definition.h"
#include "movie_instance.h"
#include "movie_root.h"
#include "sprite_instance.h"
#include "as_environment.h"
#include "gnash.h" // for create_movie and create_library_movie and for gnash::key namespace
#include "VM.h" // for initialization
#include "sound_handler.h" // for creating the "test" sound handlers
#include "render.h" // for get_render_handler
#include "types.h" // for rgba class
#include "FuzzyPixel.h"
#include "render.h"
#include "render_handler.h"
#include "render_handler_agg.h"
#include "SystemClock.h"
#ifdef RENDERER_CAIRO
#include "render_handler_cairo.h"
#endif

#include <cstdio>
#include <string>
#include <memory> // for auto_ptr
#include <cmath> // for ceil and exp2

#define SHOW_INVALIDATED_BOUNDS_ON_ADVANCE 1

#ifdef SHOW_INVALIDATED_BOUNDS_ON_ADVANCE
#include <sstream>
#endif


namespace gnash {

MovieTester::MovieTester(const std::string& url)
	:
	_forceRedraw(true)
{

	// Initialize gnash code lib
	gnashInit();

	// Set base url *before* calling create_movie
	// TODO: use PWD if url == '-'
	set_base_url(url);

	if ( url == "-" )
	{
		std::auto_ptr<tu_file> in (
				noseek_fd_adapter::make_stream(fileno(stdin))
				);
		_movie_def = gnash::create_movie(in, url, false);
	}
	else
	{
		URL urlObj(url);
		if ( urlObj.protocol() == "file" )
		{
			RcInitFile& rcfile = RcInitFile::getDefaultInstance();
			const std::string& path = urlObj.path();
#if 1 // add the *directory* the movie was loaded from to the local sandbox path
			size_t lastSlash = path.find_last_of('/');
			std::string dir = path.substr(0, lastSlash+1);
			rcfile.addLocalSandboxPath(dir);
			log_debug(_("%s appended to local sandboxes"), dir.c_str());
#else // add the *file* to be loaded to the local sandbox path
			rcfile.addLocalSandboxPath(path);
			log_debug(_("%s appended to local sandboxes"), path.c_str());
#endif
		}
		// _url should be always set at this point...
		_movie_def = gnash::create_library_movie(urlObj, NULL, false);
	}

	if ( ! _movie_def )
	{
		throw GnashException("Could not load movie from "+url);
	}

	// Initialize the sound handler(s)
	initTestingSoundHandlers();

	_movie_root = &(VM::init(*_movie_def, _clock).getRoot());

	// Initialize viewport size with the one advertised in the header
	_width = unsigned(_movie_def->get_width_pixels());
	_height = unsigned(_movie_def->get_height_pixels());

	// Initialize the testing renderers
	initTestingRenderers();

	// Now complete load of the movie
	_movie_def->completeLoad();
	_movie_def->ensure_frame_loaded(_movie_def->get_frame_count());

	// Activate verbosity so that self-contained testcases are
	// also used 
	gnash::LogFile& dbglogfile = gnash::LogFile::getDefaultInstance();
	dbglogfile.setVerbosity(1);

	
	std::auto_ptr<movie_instance> mi ( _movie_def->create_movie_instance() );

	// Set _movie before calling ::render
	_movie = mi.get();

	// Finally, place the root movie on the stage ...
        _movie_root->setRootMovie( mi.release() );

	// ... and render it
	render();
}

void
MovieTester::render(render_handler& h, InvalidatedRanges& invalidated_regions) 
{
	assert(_movie);

	set_render_handler(&h);

	h.set_invalidated_regions(invalidated_regions);

	// We call display here to simulate effect of a real run.
	//
	// What we're particularly interested about is 
	// proper computation of invalidated bounds, which
	// needs clear_invalidated() to be called.
	// display() will call clear_invalidated() on characters
	// actually modified so we're fine with that.
	//
	// Directly calling _movie->clear_invalidated() here
	// also work currently, as invalidating the topmost
	// movie will force recomputation of all invalidated
	// bounds. Still, possible future changes might 
	// introduce differences, so better to reproduce
	// real runs as close as possible, by calling display().
	//
	_movie_root->display();
}

void
MovieTester::redraw()
{
	_forceRedraw=true;
	render();
}

void
MovieTester::render() 
{
	// Get invalidated ranges and cache them
	_invalidatedBounds.setNull();

	_movie_root->add_invalidated_bounds(_invalidatedBounds, false);

#ifdef SHOW_INVALIDATED_BOUNDS_ON_ADVANCE
	std::cout << "frame " << _movie->get_current_frame() << ") Invalidated bounds " << _invalidatedBounds << std::endl;
#endif

	// Force full redraw by using a WORLD invalidated ranges
	InvalidatedRanges ranges = _invalidatedBounds; 
	if ( _forceRedraw )
	{
		ranges.setWorld(); // set to world if asked a full redraw
		_forceRedraw = false; // reset to no forced redraw
	}

	for (TRenderers::const_iterator it=_testingRenderers.begin(), itE=_testingRenderers.end();
				it != itE; ++it)
	{
		TestingRenderer& rend = *(*it);
		render(rend.getRenderer(), ranges);
	}
	
	if ( _testingRenderers.empty() )
	{
		// Make sure display is called in any case 
		//
		// What we're particularly interested about is 
		// proper computation of invalidated bounds, which
		// needs clear_invalidated() to be called.
		// display() will call clear_invalidated() on characters
		// actually modified so we're fine with that.
		//
		// Directly calling _movie->clear_invalidated() here
		// also work currently, as invalidating the topmost
		// movie will force recomputation of all invalidated
		// bounds. Still, possible future changes might 
		// introduce differences, so better to reproduce
		// real runs as close as possible, by calling display().
		//
		_movie_root->display();
	}
}

void
MovieTester::advanceClock(unsigned long ms)
{
	_clock.advance(ms);
}

void
MovieTester::advance(bool updateClock)
{
	if ( updateClock )
	{
		// TODO: cache 'clockAdvance' 
		float fps = _movie_def->get_frame_rate();
		unsigned long clockAdvance = long(1000/fps);
		advanceClock(clockAdvance);
	}

	_movie_root->advance();

	render();

}

const character*
MovieTester::findDisplayItemByName(const sprite_instance& mc,
		const std::string& name) 
{
	const DisplayList& dlist = mc.getDisplayList();
	return dlist.get_character_by_name(name);
}

const character*
MovieTester::findDisplayItemByDepth(const sprite_instance& mc,
		int depth)
{
	const DisplayList& dlist = mc.getDisplayList();
	return dlist.get_character_at_depth(depth);
}

void
MovieTester::movePointerTo(int x, int y)
{
	_x = x;
	_y = y;
	if ( _movie_root->notify_mouse_moved(x, y) ) render();
}

void
MovieTester::checkPixel(int x, int y, unsigned radius, const rgba& color,
		short unsigned tolerance, const std::string& label, bool expectFailure) const
{
	if ( ! canTestRendering() )
	{
		std::stringstream ss;
	        ss << "exp:" << color.toShortString() << " ";
		log_msg("UNTESTED: NORENDERER: pix:%d,%d %s %s", x,  y, ss.str().c_str(), label.c_str());
	}

	FuzzyPixel exp(color, tolerance);
	const char* X="";
	if ( expectFailure ) X="X";

	//std::cout <<"chekPixel(" << color << ") called" << std::endl;

	for (TRenderers::const_iterator it=_testingRenderers.begin(), itE=_testingRenderers.end();
				it != itE; ++it)
	{
		const TestingRenderer& rend = *(*it);

		std::stringstream ss;
		ss << rend.getName() <<" ";
		ss << "pix:" << x << "," << y <<" ";

		rgba obt_col;

		render_handler& handler = rend.getRenderer();

	        if ( ! handler.getAveragePixel(obt_col, x, y, radius) )
		{
			ss << " is out of rendering buffer";
			log_msg("%sFAILED: %s (%s)", X,
					ss.str().c_str(),
					label.c_str()
					);
			continue;
		}

		// Find minimum tolerance as a function of BPP

		unsigned short minRendererTolerance = 1;
		unsigned int bpp = handler.getBitsPerPixel();
		if ( bpp ) 
		{
			// UdoG: check_pixel should *always* tolerate at least 2 ^ (8 - bpp/3)
			minRendererTolerance = int(ceil(exp2(8 - bpp/3)));
		}

		//unsigned short tol = std::max(tolerance, minRendererTolerance);
		unsigned short tol = tolerance*minRendererTolerance; 

	        ss << "exp:" << color.toShortString() << " ";
	        ss << "obt:" << obt_col.toShortString() << " ";
	        ss << "tol:" << tol;

		FuzzyPixel obt(obt_col, tol);
		if (exp ==  obt) // equality operator would use tolerance of most tolerating FuzzyPixel
		{
			log_msg("%sPASSED: %s %s", X,
					ss.str().c_str(),
					label.c_str()
					);
		}
		else
		{
			log_msg("%sFAILED: %s %s", X,
					ss.str().c_str(),
					label.c_str()
					);
		}
	}
}

void
MovieTester::pressMouseButton()
{
	if ( _movie_root->notify_mouse_clicked(true, 1) )
	{
		render();
	}
}

void
MovieTester::depressMouseButton()
{
	if ( _movie_root->notify_mouse_clicked(false, 1) )
	{
		render();
	}
}

void
MovieTester::click()
{
	int wantRedraw = 0;
	if ( _movie_root->notify_mouse_clicked(true, 1) ) ++wantRedraw;
	if ( _movie_root->notify_mouse_clicked(false, 1) ) ++wantRedraw;

	if ( wantRedraw ) render();
}

void
MovieTester::pressKey(key::code code)
{
	if ( _movie_root->notify_key_event(code, true) )
	{
		render();
	}
}

void
MovieTester::releaseKey(key::code code)
{
	if ( _movie_root->notify_key_event(code, false) )
	{
		render();
	}
}

bool
MovieTester::isMouseOverMouseEntity()
{
	return _movie_root->isMouseOverActiveEntity();
}

geometry::SnappingRanges2d<int>
MovieTester::getInvalidatedRanges() const
{
	using namespace gnash::geometry;

	SnappingRanges2d<float> ranges = _invalidatedBounds;

	// scale by 1/20 (twips to pixels)
	ranges.scale(1.0/20);

	// Convert to integer range.
	SnappingRanges2d<int> pixranges(ranges);

	return pixranges;
	
}

int
MovieTester::soundsStarted()
{
	if ( ! _sound_handler.get() ) return 0;
	return _sound_handler->numSoundsStarted();
}

int
MovieTester::soundsStopped()
{
	if ( ! _sound_handler.get() ) return 0;
	return _sound_handler->numSoundsStopped();
}

void
MovieTester::initTestingRenderers()
{
	std::auto_ptr<render_handler> handler;

	// TODO: add support for testing multiple renderers
	// This is tricky as requires changes in the core lib

#ifdef RENDERER_AGG
	// Initialize AGG
	static const char* aggPixelFormats[] = {
		"RGB555", "RGB565", "RGBA16",
		"RGB24", "BGR24", "RGBA32", "BGRA32",
		"ARGB32", "ABGR32"
	};

	for (unsigned i=0; i<sizeof(aggPixelFormats)/sizeof(*aggPixelFormats); ++i)
	{
		const char* pixelFormat = aggPixelFormats[i];
		std::string name = "AGG_" + std::string(pixelFormat);

		handler.reset( create_render_handler_agg(pixelFormat) );
		if ( handler.get() )
		{
			//log_msg("Renderer %s initialized", name.c_str());
			std::cout << "Renderer " << name << " initialized" << std::endl;
			addTestingRenderer(handler, name); 
		}
		else
		{
			std::cout << "Renderer " << name << " not supported" << std::endl;
		}
	}
#endif // RENDERER_AGG

#ifdef RENDERER_CAIRO
	// Initialize Cairo
	handler.reset(renderer::cairo::create_handler());

        addTestingRenderer(handler, "Cairo");
#endif

#ifdef RENDERER_OPENGL
	// Initialize opengl renderer
	handler.reset(create_render_handler_ogl(false));
	addTestingRenderer(handler, "OpenGL");
#endif
}

void
MovieTester::addTestingRenderer(std::auto_ptr<render_handler> h, const std::string& name)
{
	if ( ! h->initTestBuffer(_width, _height) )
	{
		std::cout << "UNTESTED: render handler " << name
			<< " doesn't support in-memory rendering "
			<< std::endl;
		return;
	}

	// TODO: make the core lib support this
	if ( ! _testingRenderers.empty() )
	{
		std::cout << "UNTESTED: can't test render handler " << name
			<< " because gnash core lib is unable to support testing of "
			<< "multiple renderers from a single process "
			<< "and we're already testing render handler "
			<< _testingRenderers.front()->getName()
			<< std::endl;
		return;
	}

	_testingRenderers.push_back(TestingRendererPtr(new TestingRenderer(h, name)));

	// this will be needed till we allow run-time swapping of renderers,
	// see above UNTESTED message...
	set_render_handler(&(_testingRenderers.back()->getRenderer()));
}

bool
MovieTester::canTestVideo() const
{
	if ( ! canTestSound() ) return false;

#ifdef USE_MAD
	// mad doesn't support video !
	return false;
#endif

	return true;
}

void
MovieTester::initTestingSoundHandlers()
{

#ifdef SOUND_SDL
	std::cout << "Creating SDL sound handler" << std::endl;
        _sound_handler.reset( gnash::media::create_sound_handler_sdl() );
#elif defined(SOUND_GST)
	std::cout << "Creating GST sound handler" << std::endl;
        _sound_handler.reset( gnash::media::create_sound_handler_gst() );
#else
	std::cerr << "Neigher SOUND_SDL nor SOUND_GST defined" << std::endl;
	return;
	//exit(1);
#endif

	gnash::set_sound_handler(_sound_handler.get());
}

void
MovieTester::restart() 
{
	_movie_root->clear(); // restart();
	_movie = _movie_def->create_movie_instance();
	_movie_root->setRootMovie(_movie);

	// Set _movie before calling ::render
	render();
}

} // namespace gnash
