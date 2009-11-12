// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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


/// \page events_handling Handling of user events
///
/// There are two kinds of events:
/// - system generated
/// - user generated
///
/// System generated events are those like load, data recive, unload,
/// enter frame, etc.
/// User generated events are mouse movements and clicks, keyboard activity.
///
/// Events can trigger actions execution, if "handlers" are specified for
/// a specific event with ActionScript code.
/// The actions triggered by user events are executed *immediately*, not
/// at the next frame iteration. Nonetheless, since rendering of the stage
/// usually happens at fixed rate (frame rate) you won't see the effects
/// of actions execution until next iteration... unless...
///
/// Well, *some* events actions always trigger immediate redisplay, while
/// some others require a call to a special function to do so.
///
/// The events actions that trigger immediate redisplay are Button actions.
/// Colin Mook, in his "ActionScript - The Definitive Guide" sais:
/// << Buttons naturally update between frames >>
///
/// Other events, in particular MovieClip events such as mouseDown, mouseUp,
/// mouseMove, keyDown and keyUp don't by default trigger redisplay, unless
/// the attached action code makes a call to the special function named
/// 'updateAfterEvent()'.
///
/// For this purpose, user events notification functions in gnash core 
/// library return a boolean value, which tells wheter any action triggered
/// by the event requires immediate redisplay.
///
/// At the time of writing (2006-10-19) this is not implemented yet and
/// the return code is always TRUE. We shall work on it :)
///
/// The events notification functions that currently support this interface
/// are:
///
/// - bool movie_root::notify_mouse_moved(int x, int y);
/// - bool movie_root::notify_mouse_clicked(bool mouse_pressed, int mask);
/// - bool movie_root::notify_key_event(key::code k, bool down);
/// 
/// 


#ifndef GNASH_MOVIE_ROOT_H
#define GNASH_MOVIE_ROOT_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h" //USE_SWFTREE
#endif

#include "smart_ptr.h" // GNASH_USE_GC
#include "dsodefs.h" // DSOEXPORT
#include "MouseButtonState.h" // for composition
#include "drag_state.h" // for composition
#include "smart_ptr.h" // for memory management
#include "URL.h" // for loadMovie
#include "GnashKey.h" // key::code
#include "Movie.h"
#include "RunResources.h" // for initialization
#include "gnash.h" // Quality
#include "MovieClip.h"
#include "SimpleBuffer.h" // for LoadCallback

#ifdef USE_SWFTREE
# include "tree.hh"
#endif

// GNASH_PARANOIA_LEVEL:
// 0 : (not unimplemented)
// 1 : quick assertions
// 2 : add testInvariant
//
#ifndef GNASH_PARANOIA_LEVEL
# define GNASH_PARANOIA_LEVEL 1
#endif

#include <vector>
#include <list>
#include <set>
#include <bitset>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>

// Forward declarations
namespace gnash {
    class ExecutableCode; // for ActionQueue
    class URL;
    class Timer;
    class MovieClip;
    class VirtualClock;
    class Keyboard_as;
    class IOChannel;
}

namespace gnash
{

struct DepthComparator
{
    typedef MovieClip* LevelMovie;

    bool operator() (const LevelMovie& d1, const LevelMovie& d2)
    {
        return d1->get_depth() < d2->get_depth();
    }
};

/// This class represents the 'Stage' and top-level movie.
//
/// It is a wrapper around the set of loaded levels being played. Each 
/// 'run' of a SWF movie, including all further movies loaded during the
/// run, has exactly one movie_root, which is kept for the entire run.
/// Loading a new top-level movie does not create a new movie_root.
//
/// The 'Stage' part of movie_root is accessible through the ActionScript
/// Stage object, implemented in Stage_as.cpp.
//
/// The movie_root class is responsible for accepting and passing on
/// user events (mouse or key events), for maintaining the heart-beat
/// mechanism, and for advancing all MovieClips on request from the
/// hosting application.
//
/// The _root object is provided by getAsRoot().
class DSOEXPORT movie_root : boost::noncopyable
{

public:

    struct LoadCallback {
        boost::shared_ptr<IOChannel> stream;
        as_object* obj;
        SimpleBuffer buf;
        LoadCallback(boost::shared_ptr<IOChannel> s, as_object* o)
            : stream(s), obj(o) {}
    };
        
    typedef std::list<LoadCallback> LoadCallbacks;

    /// Default constructor
    //
    /// Make sure to call setRootMovie() 
    /// before using any of this class methods !
    ///
    movie_root(const movie_definition& def, VirtualClock& clock,
            const RunResources& runResources);

    ~movie_root();

    /// Initialize movie_root with a parsed movie definition
    //
    /// The definition may be a SWF or Bitmap movie definition.
    // 
    /// The created Movie is returned; it is non-const so may be stored,
    /// queried, and changed by the caller for debugging or manipulation.
    /// Direct use of the pointer may result in unexpected behaviour during
    /// SWF playback, so for normal playback this pointer should not be
    /// used.
    Movie* init(movie_definition* def,
            const MovieClip::MovieVariables& variables);

    /// Return the movie at the given level (0 if unloaded level).
    //
    /// POST CONDITIONS:
    /// - The returned DisplayObject has a depth equal to 'num'
    ///
    Movie* getLevel(unsigned int num) const;

    /// Replace an existing level with a new movie
    //
    /// Depth will be assigned to external_movie by this function.
    /// If the give level number doesn't exist an error is logged
    /// and nothing else happens.
    ///
    /// This method is intended for use by xxx.loadMovie(yyy)
    /// when 'xxx' is a top-level movie.
    ///
    void replaceLevel(unsigned int num, Movie* external_movie);

    /// Swap depth of a level (or two)
    //
    /// Character's depths are updated.
    ///
    /// @param sp
    ///    The level to change depth/level of. A pointer to it is expected
    ///    to be found in the _level# container, or an error will be printed
    ///    and the call would result in a no-op.
    ///
    /// @param depth
    ///    New depth to assign to the DisplayObject. If another level
    ///    exists at the target depth the latter is moved in place of
    ///    the former, with its depth also updated.
    ///
    void swapLevels(MovieClip* sp, int depth);

    /// Drop level at given depth.
    //
    /// @param depth
    ///   Depth of the level to drop. Note that this is 
    ///   -DisplayObject::staticDepthOffset for the root movie. Must be >=0 and
    ///   <= 1048575 or an assertion will fail. Note that if the depth
    ///   evaluates to the original root movie nothing happens (not allowed
    ///   to remove that). It is not tested if it's allowed to remove _level0
    ///   after loading into it.
    void dropLevel(int depth);

    /// Change display viewport coordinates
    //
    /// This currently also change the display scale
    /// but we should instead only do it if rescaling
    /// is allowed.
    ///
    void set_display_viewport(int x0, int y0, int w, int h);

    /// \brief
    /// Return the notional width of the stage, value depending
    /// on scaleMode
    unsigned getStageWidth() const;

    /// \brief
    /// Return the notional height of the stage, actual value depending
    /// on scaleMode
    unsigned getStageHeight() const;

    /// \brief
    /// The host app can use this to tell the movie when
    /// user's mouse pointer has moved.
    //
    /// Coordinates are in Stage Coordinate Space (pseudo-pixels units).
    ///
    /// This function should return TRUE iff any action triggered
    /// by the event requires redraw, see \ref events_handling for
    /// more info.
    ///
    /// TODO: take twips (or float pixels), or we won't be able to
    ///       support sub-pixel accuracy in collision detection.
    ///
    bool notify_mouse_moved(int x, int y);

    /// \brief
    /// The host app can use this to tell the movie when the
    /// user clicked or released the mouse button.
    //
    /// @param mouse_pressed
    /// true if the mouse has been pressed, false if released
    ///
    /// @param mask
    /// ???
    ///
    /// This function should return TRUE iff any action triggered
    /// by the event requires redraw, see \ref events_handling for
    /// more info.
    ///
    bool notify_mouse_clicked(bool mouse_pressed, int mask);

    /// \brief
    /// The host app can use this to tell the movie when
    /// user pressed or released a key.
    //
    /// This function should return TRUE iff any action triggered
    /// by the event requires redraw, see \ref events_handling for
    /// more info.
    ///
    bool notify_key_event(key::code k, bool down);

    /// \brief
    /// Use this to retrieve the last state of the mouse, as set via
    /// notify_mouse_state(). 
    //
    /// Coordinates are in PIXELS, NOT TWIPS.
    ///
    void get_mouse_state(boost::int32_t& x, boost::int32_t& y,
            boost::int32_t& buttons);

    void get_drag_state(drag_state& st);

    void set_drag_state(const drag_state& st);

    /// @return the originating root movie (not necessarely _level0)
    const Movie& getRootMovie() const
    {
        return *_rootMovie;
    }

    /// Creating new MovieClips needs this for now.
    //
    /// TODO: create MovieClips without this and drop. It's deliberately
    /// different from getRootMovie() so it doesn't end up getting used
    /// in the same way.
    Movie* topLevelMovie() const
    {
        return _rootMovie;
    }

    /// Return the current nominal frame rate for the Stage.
    //
    /// This is dependent on the Movie set as root movie.
    float frameRate() const {
        return _rootMovie->frameRate();
    }

    void stop_drag()
    {
        m_drag_state.reset();
    }

    /// Add an interval timer
    //
    /// @param timer
    /// A Timer, ownership will be transferred. Must not be NULL.
    ///
    /// @param internal
    /// If true, this is an internal timer, so will get a negative id.
    ///
    /// @return An integer indentifying the timer
    ///         for subsequent call to clear_interval_timer.
    ///         It will NEVER be zero.
    ///
    unsigned int add_interval_timer(std::auto_ptr<Timer> timer);

    /// Register an object for loading data to.
    //
    /// When complete, the object's onData function is called.
    /// The callback is removed when the load is complete, including failed
    /// loads.
    //
    /// There is no restriction on the type of as_object that can registered.
    //
    /// @param obj      The object to update when data is received.
    /// @param str      The stream to load from.
    //
    /// TODO: this function could be improved, e.g. by handling the
    /// URL checking and stream construction as well.
    //
    /// It may be possible for this function to handle all connections if
    /// it also takes a callback function to call on each advance.
    void addLoadableObject(as_object* obj, std::auto_ptr<IOChannel> str);

    void addAdvanceCallback(ActiveRelay* obj);

    void removeAdvanceCallback(ActiveRelay* obj);

    /// Remove timer identified by given integer
    //
    /// @return true on success, false on error (no such timer)
    ///
    bool clear_interval_timer(unsigned int x);

    /// Return 0-based frame index of originating root movie
    //
    /// TODO: drop this function (currently used by gprocessor)
    ///       or change it to to delegate to _level0 ?
    ///
    size_t get_current_frame() const
    {
        return _rootMovie->get_current_frame();
    }

    void set_background_color(const rgba& color);

    void set_background_alpha(float alpha);

    /// Return the VM used by this movie_root
    VM& getVM() { return _vm; }
 
    /// Main and only callback from hosting application.
    /// Expected to be called at 10ms resolution.
    //
    /// @return true if the heart-beat resulted in actual
    ///         SWF playhead advancement (frame advancement)
    ///
    bool advance();

    /// \brief
    /// Return the number of milliseconds available before
    /// it's time to advance the timeline again.
    //
    /// Return value can be negative if we're late...
    ///
    int timeToNextFrame() const;

    /// Entry point for movie advancement
    //
    /// This function does:
    ///   - Execute all timers
    ///   - Reset the next Random number
    ///   - Advance all advanceable DisplayObjects in reverse-placement order
    ///   - Cleanup key listeners
    ///   - Process all queued actions
    ///   - Remove unloaded DisplayObjects from the advanceable
    ///     DisplayObjects list.
    ///   - Run the GC collector
    void advanceMovie();

    /// 0-based!! delegates to originating root movie
    //
    /// TODO: drop this method. currently used by gprocessor.
    void goto_frame(size_t target_frame_number)
    {
        _rootMovie->goto_frame(target_frame_number);
    }

    void display();

    /// Delegate to originating root movie
    //
    /// TODO: drop ?
    void set_play_state(MovieClip::PlayState s)
    {
        _rootMovie->setPlayState(s);
    }

    /// Get a unique number for unnamed instances.
    size_t nextUnnamedInstance();

    /// Notify still loaded DisplayObject listeners for key events
    DSOEXPORT void notify_key_listeners(key::code k, bool down);

    /// Push a new DisplayObject listener for key events
    void add_key_listener(DisplayObject* listener)
    {
        add_listener(m_key_listeners, listener);
    }

    /// Remove a DisplayObject listener for key events
    void remove_key_listener(DisplayObject* listener)
    {
        remove_listener(m_key_listeners, listener);
    }

    /// Notify still loaded DisplayObject listeners for mouse events
    DSOEXPORT void notify_mouse_listeners(const event_id& event);

    /// Push a new DisplayObject listener for mouse events
    void add_mouse_listener(DisplayObject* listener)
    {
        add_listener(m_mouse_listeners, listener);
    }

    /// Remove a DisplayObject listener for mouse events
    void remove_mouse_listener(DisplayObject* listener)
    {
        remove_listener(m_mouse_listeners, listener);
    }

    /// Get the DisplayObject having focus
    //
    /// The DisplayObject having focus will receive mouse button
    /// and key presses/releases.
    ///
    /// @return the DisplayObject having focus or NULL of none.
    ///
    DisplayObject* getFocus();

    /// Set the DisplayObject having focus
    //
    /// @param to
    /// The DisplayObject to receive focus. NULL to kill focus.
    /// @return true if the focus operation succeeded, false if the passed
    /// DisplayObject cannot receive focus. setFocus(0) is a valid operation, so
    /// returns true (always succeeds).
    bool setFocus(DisplayObject* to);
    
    DSOEXPORT void add_invalidated_bounds(InvalidatedRanges& ranges,
            bool force);
    
    /// Return the topmost active entity under the pointer
    //
    /// This method returns cached info, with cache updated
    /// by notify_mouse_moved (and should be updated also
    /// by movie advancement or actions execution maybe, not
    /// currently implmented).
    ///
    /// @return the topmost active entity under pointer or NULL if none.
    DisplayObject* getActiveEntityUnderPointer() const;

    /// Return the topmost non-dragging entity under the pointer
    //
    /// This method triggers a displaylist scan
    ///
    /// @return the topmost non-dragging entity under pointer or NULL if none
    const DisplayObject* getEntityUnderPointer() const;

    /// Return the DisplayObject currently being dragged, if any
    DisplayObject* getDraggingCharacter() const;

    /// Return true if the mouse pointer is over an active entity
    bool isMouseOverActiveEntity() const;

    bool testInvariant() const;

    /// The possible values of Stage.displayState
    enum DisplayState {
        DISPLAYSTATE_NORMAL,
        DISPLAYSTATE_FULLSCREEN
    };

    /// The possibile values of Stage.scaleMode
    enum ScaleMode {
        showAll,
        noScale,
        exactFit,
        noBorder
    };

    /// The possible horizonal positions of the Stage
    enum StageHorizontalAlign {
        STAGE_H_ALIGN_C,
        STAGE_H_ALIGN_L,
        STAGE_H_ALIGN_R
    };

    /// The possible vertical position of the Stage
    enum StageVerticalAlign {
        STAGE_V_ALIGN_C,
        STAGE_V_ALIGN_T,       
        STAGE_V_ALIGN_B
    };

    /// The possible elements of a Stage.alignMode.
    enum AlignMode {
        STAGE_ALIGN_L,
        STAGE_ALIGN_T,
        STAGE_ALIGN_R,
        STAGE_ALIGN_B
    };

    /// Set the current display quality of the entire SWF.
    void setQuality(Quality q);

    /// Get the current display quality.
    Quality getQuality() const { return _quality; }

    /// Sets movie_root's horizontal and vertical alignment to one
    /// of the three possible positions for each dimension.
    void setStageAlignment(short s);

    typedef std::pair<StageHorizontalAlign, StageVerticalAlign> StageAlign;

    /// Returns the current alignment of the stage (left/right/centre, top/
    /// bottom/centre) as a std::pair
    StageAlign getStageAlignment() const;

	/// Returns the current value of _showMenu which instructs the gui about
	/// how much to display in the context menu
	bool getShowMenuState() const;

	/// Sets the value of _showMenu and calls the fscommand handler for the
	/// current gui
	void setShowMenuState( bool state );

    /// Sets the Stage object's align mode.
    void setStageScaleMode(ScaleMode sm);
    
    /// Returns the Stage object's align mode.
    ScaleMode getStageScaleMode() const { return _scaleMode; }

    // The string representation of the current align mode.
    std::string getStageAlignMode() const;

    /// Returns the Stage object's align mode.
    DisplayState getStageDisplayState() const { return _displayState; }

    // The string representation of the current align mode.
    void setStageDisplayState(const DisplayState ds);

    /// Action priority levels
    enum ActionPriorityLevel {

        /// Init actions, Init event handlers
        apINIT=0,

        /// Construct event handlers
        apCONSTRUCT=1,

        /// EnterFrame event handlers
        apENTERFRAME=2,

        /// Frame actions, load handlers, unload handlers
        apDOACTION=3,

        /// Last element used to easy computation of size...
        apSIZE
        
    };

    /// Push an executable code to the ActionQueue
    void pushAction(std::auto_ptr<ExecutableCode> code, int lvl=apDOACTION);

    /// Push an executable code to the ActionQueue
    void pushAction(const action_buffer& buf, DisplayObject* target,
            int lvl=apDOACTION);

    /// Push a function code to the ActionQueue
    void pushAction(as_function* func, DisplayObject* target,
            int lvl=apDOACTION);

#ifdef GNASH_USE_GC
    /// Mark all reachable resources (for GC)
    //
    /// Resources reachable from movie_root are:
    ///
    /// - All _level# movies (_movies)
    /// - The original root movie (_rootMovie)
    /// - Mouse entities (m_mouse_button_state)
    /// - Timer targets (_intervalTimers)
    /// - Resources reachable by ActionQueue code (_actionQueue)
    /// - Key listeners (m_key_listeners)
    /// - Mouse listeners (m_mouse_listeners)
    /// - Any DisplayObject being dragged 
    ///
    void markReachableResources() const;
#endif // GNASH_USE_GC

    /// \brief
    /// Register a newly born advanceable DisplayObject to the
    /// list of DisplayObjects to be advanced on next ::advance call.
    //
    /// The DisplayObject will only be advanced if not unloaded when
    /// its turn comes. Characters are advanced in reverse-placement
    /// order (first registered is advanced last)
    ///
    void addLiveChar(DisplayObject* ch)
    {
        // Don't register the object in the list twice 
#if GNASH_PARANOIA_LEVEL > 1
        assert(std::find(_liveChars.begin(), _liveChars.end(), ch) ==
            _liveChars.end());
#endif
        _liveChars.push_front(ch);
    }

    /// Cleanup all resources and run the GC collector
    //
    /// This method should be invoked before calling setRootMovie again
    /// for a clean restart.
    ///
    void clear();

    /// Reset stage to its initial state
    void reset();

    /// Call this method for disabling run of actions
    //
    /// NOTE: this will only work for queued actions, not
    ///       for *every* action. Supposedly all actions should
    ///       be queued, but this is not really always the case.
    ///       Notable exceptions are:
    ///         - Actions in callFrame target frame
    ///           but only executed by execution of the callFrame opcode
    ///         - on{,Clip}{Initialize,Construct} event handlers
    ///         - User event handlers (mouse,keyboard)
    ///
    void disableScripts();

    /// Return true if scripts execution is disabled
    bool scriptsDisabled() const { return _disableScripts; };

    /// Process action queues with higher priority then the priority
    /// of the action queue currently being processed.
    //
    /// This is intended to be called at the end of any function call
    /// and at the end of an action block.
    ///
    /// TODO: be aware of infinite loops !
    ///
    void flushHigherPriorityActionQueues();

    DisplayObject* findCharacterByTarget(const std::string& tgtstr) const;

    /// Queue a request for loading a movie
    //
    /// This function constructs the URL and, if required, the postdata
    /// from the arguments. The variables to send should *not* be appended
    /// to @param urlstr before calling this function.
    //
    /// @param urlstr   The url exactly as requested. This may already
    ///                 contain a query string.
    /// @param target   Target for request.
    /// @param data     The variables data to send, URL encoded in
    ///                 key/value pairs
    /// @param method   The VariablesMethod to use for sending the data. If
    ///                 MovieClip::METHOD_NONE, no data will be sent.
    /// @param handler  An object which will be signalled of load
    ///                 events (onLoadStart, onLoadComplete, onLoadInit,
    ///                 onLoadError). Can be null if caller doesn't care.
    ///                 
    void loadMovie(const std::string& url, const std::string& target,
            const std::string& data, MovieClip::VariablesMethod method,
            as_object* handler=0);

    /// Send a request to the hosting application (e.g. browser).
    //
    /// This function constructs the URL and, if required, the postdata
    /// from the arguments. The variables to send should *not* be appended
    /// to @param urlstr before calling this function.
    //
    /// @param urlstr   The url exactly as requested. This may already
    ///                 contain a query string.
    /// @param target   Target for request.
    /// @param data     The variables data to send, URL encoded in
    ///                 key/value pairs
    /// @param method   The VariablesMethod to use for sending the data. If
    ///                 MovieClip::METHOD_NONE, no data will be sent.
    void getURL(const std::string& urlstr, const std::string& target,
            const std::string& data, MovieClip::VariablesMethod method);


    /// Return true if the given string can be interpreted as a _level name
    //
    /// @param name
    ///   The target string.
    ///   Will be considered case-insensitive if VM version is < 7.
    ///
    /// @param levelno
    ///   Output parameter, will be set to the level number, if true is
    ///   returned
    bool isLevelTarget(const std::string& name, unsigned int& levelno);


    /// Set a filedescriptor to use for host application requests
    /// (for browser communication mostly)
    void setHostFD(int fd)
    {
        assert(fd > 0);
        _hostfd = fd;
    }

    /// Get the filedescriptor to use for host application requests
    /// (for browser communication mostly)
    ///
    /// @return -1 if no filedescriptor is provided by host app.
    int getHostFD() const
    {
        return _hostfd;
    }

    /// Abstract base class for FS handlers
    class AbstractFsCallback {
    public:
        virtual void notify(const std::string& cmd, const std::string& arg)=0;
        virtual ~AbstractFsCallback() {}
    };

    /// ActionScript embedded in a movie can use the built-in
    /// fscommand() function to send data back to the host
    /// application.  If you are interested in this data, register
    /// a handler, which will be called when the embedded scripts
    /// call fscommand().
    ///
    /// The handler gets the MovieClip* that the script is
    /// embedded in, and the two string arguments passed by the
    /// script to fscommand().
    DSOEXPORT void registerFSCommandCallback(AbstractFsCallback* handler)
    {
        _fsCommandHandler = handler;
    }

    /// Call this to notify FS commands
    DSOEXPORT void handleFsCommand(const std::string& cmd,
            const std::string& arg) const;

    /// Abstract base class for hosting app handler
    class AbstractIfaceCallback
    {
    public:

        /// Get Gui-related information for the core.
        //
        /// This should be used for occasional AS calls, such as for
        /// Mouse.hide, System.capabilities etc. The return can be
        /// various types, so it is passed as a string.
        virtual std::string call(const std::string& cmd,
                const std::string& arg = std::string()) = 0;

        /// Ask the hosting application for a yes / no answer to
        /// a question.
        virtual bool yesNo(const std::string& cmd) = 0;

        /// Send an error message to the hosting application.
        //
        /// This does not have to be implemented; the default is a no-op.
        virtual void error(const std::string& /*msg*/) {}

        virtual ~AbstractIfaceCallback() {}
    };

    /// A callback to the GUI (or whatever is listening) for sending
    /// events and receiving replies. Used for ActionScript interface
    /// with the gui (Mouse visibility, Stage alignment etc and System
    /// information, for instance).
    ///
    /// See callInterface method
    DSOEXPORT void registerEventCallback(AbstractIfaceCallback* handler)
    {
        _interfaceHandler = handler;
    }

    /// Call into the hosting application
    ///
    /// Will use callback set with registerEventCallback
    DSOEXPORT std::string callInterface(const std::string& cmd,
            const std::string& arg = std::string()) const;

    /// Send an error message to the hosting application.
    //
    /// @param msg  A message to send describing the error.
    //
    /// The hosting app decides what to do with the message, or whether it
    /// wants to do anything at all. It may show a popup box.
    DSOEXPORT void errorInterface(const std::string& msg) const;

    /// Called from the ScriptLimits tag parser to set the
    /// global script limits. It is expected behaviour that
    /// each new loaded movie should override this.
    /// Can be overridden from gnashrc.
    //
    /// @param recursion the maximum number of recursions when
    ///             finding 'super'.
    ///             The default value for this (i.e. when no
    ///             ScriptLimits tag is present) is documented to be
    ///             256, but this may change and appears not to be
    ///             crucial for (backward) compatibility.
    /// @param timeout the timeout in seconds for script execution.
    ///             The default value for this (i.e. when no
    ///             ScriptLimits tag is present) is documented to be
    ///             15 to 20 seconds, depending on platform.
    void setScriptLimits(boost::uint16_t recursion, boost::uint16_t timeout);
    
    /// Get the current global recursion limit for this movie: it can
    /// be changed by loaded movies.
    boost::uint16_t getRecursionLimit() const
    {
        return _recursionLimit;
    }

    /// Get the current global script timeout limit for this movie: it
    /// can be changed by loaded movies.
    boost::uint16_t getTimeoutLimit() const
    {
        return _timeoutLimit;
    }

#ifdef USE_SWFTREE
    typedef std::pair<std::string, std::string> StringPair;
    void getMovieInfo(tree<StringPair>& tr, tree<StringPair>::iterator it);
    void getCharacterTree(tree<StringPair>& tr, tree<StringPair>::iterator it);
#endif

	/// Get URL of the SWF movie used to initialize this VM
	//
	/// This information will be used for security checks
	///
	const std::string& getOriginalURL() const { return _originalURL; }

    const RunResources& runResources() const { return _runResources; }

private:

    /// Set the root movie, replacing the current one if any.
    //
    /// This is needed for the cases in which the top-level movie
    /// is replaced by another movie by effect of a loadMovie call
    /// or similar.
    ///
    /// TODO: inspect what happens about VM version
    ///   (should the *new* movie drive VM operations?
    ///    -- hope not ! )
    ///
    /// Make sure to call this method before using the movie_root,
    /// as most operations are delegated to the associated/wrapped
    /// Movie.
    ///
    /// Note that the display viewport will be updated to match
    /// the size of given movie.
    ///
    /// A call to this method is equivalent to a call to setLevel(0, movie).
    ///
    /// @param movie
    /// The Movie to wrap.
    /// Must have a depth of 0.
    ///
    void setRootMovie(Movie* movie);

    const RunResources& _runResources; 

    /// The URL of the original root movie.
    //
    /// This is a runtime constant because it must not change during a 
    /// run.
    const std::string _originalURL;

    /// This initializes a SharedObjectLibrary, which requires 
    /// _originalURL, so that must be initialized first.
    VM& _vm;

    /// Registered Interface command handler, if any
    AbstractIfaceCallback* _interfaceHandler;

    /// Registered FsCommand handler, if any
    AbstractFsCallback* _fsCommandHandler;

    /// A load movie request
    class LoadMovieRequest : boost::noncopyable {
    public:
        /// @param postdata
        ///   If not null POST method will be used for HTTP.
        ///
        LoadMovieRequest(const URL& u, const std::string& t,
                const std::string* postdata, as_object* handler)
                :
                _target(t),
                _url(u),
                _usePost(false),
                _mdef(0),
                _completed(false),
                _mutex(),
                _handler(handler)
        {
            if ( postdata )
            {
                _postData = *postdata;
                _usePost = true;
            }
        }

        const std::string& getTarget() const { return _target; }
        const URL& getURL() const { return _url; }
        const std::string& getPostData() const { return _postData; }
        bool usePost() const { return _usePost; }
        as_object* getHandler() const { return _handler; }
        void setReachable() const {
            if (_handler) _handler->setReachable();
        }

        /// Get the loaded movie definition, if any
        //
        /// @param md the loaded movie_definition is copied here
        ///           if it was impossible to create one.
        ///
        /// @return true if the request was completed, false otherwise.
        ///
        /// RULE: if return is FALSE param 'md' will be set to 0.
        /// RULE: if return is TRUE  param 'md' may be set to 0 or non 0.
        /// RULE: if parameter 'md' is set to non 0, TRUE must be returned.
        ///
        /// locks _mutex
        ///
        bool getCompleted(boost::intrusive_ptr<movie_definition>& md) const
        {
            boost::mutex::scoped_lock lock(_mutex);
            md = _mdef;
            return _completed;
        }

        /// Complete the request
        //
        /// @param md the loaded movie_definition, or 0 if
        ///           it was impossible to create one.
        ///
        /// locks _mutex
        ///
        void setCompleted(boost::intrusive_ptr<movie_definition> md)
        {
            boost::mutex::scoped_lock lock(_mutex);
            _mdef = md;
            _completed = true;
        }

    private:
        std::string _target;
        URL _url;
        bool _usePost;
        std::string _postData;
        boost::intrusive_ptr<movie_definition> _mdef;
        bool _completed;
        mutable boost::mutex _mutex;
        as_object* _handler;
    };

    /// Load movie requests
    typedef std::list<LoadMovieRequest*> LoadMovieRequests;
    LoadMovieRequests _loadMovieRequests;

    /// Process all load movie requests
    void processLoadMovieRequests();

    /// Process a single load movie request
    void processLoadMovieRequest(LoadMovieRequest& r);

    /// Check a LoadMovieRequest and process if completed.
    //
    /// @return true if the request was completely processed.
    ///
    bool processCompletedLoadMovieRequest(const LoadMovieRequest& r);

    /// Process all completed loadMovie requests.
    void processCompletedLoadMovieRequests();

    /// Listeners container
    typedef std::list<DisplayObject*> CharacterList;

    /// key and mouse listeners container
    typedef CharacterList KeyListeners;
    typedef CharacterList MouseListeners;

    /// Take care of dragging, if needed
    void doMouseDrag();

    /// Delete all elements on the action queue and empty it.
    void clearActionQueue();

    /// Delete all elements on the timers list
    void clearIntervalTimers();

    /// Delete all LoadMovieRequests
    void clearLoadMovieRequests();

    /// Execute expired timers
    void executeAdvanceCallbacks();
    
    /// Execute expired timers
    void executeTimers();

    /// Notify the global Key ActionScript object about a key status change
    Keyboard_as* notify_global_key(key::code k, bool down);

    /// Remove unloaded key and mouselisteners.
    void cleanupUnloadedListeners()
    {
        cleanupUnloadedListeners(m_key_listeners);
        cleanupUnloadedListeners(m_mouse_listeners);
    }

    /// Erase unloaded DisplayObjects from the given listeners list
    static void cleanupUnloadedListeners(CharacterList& ll);

    /// Cleanup references to unloaded DisplayObjects and run the GC.
    void cleanupAndCollect();

    /// Push a DisplayObject listener to the front of given container, if not
    /// already present
    static void add_listener(CharacterList& ll, DisplayObject* elem);

    /// Remove a listener from the list
    static void remove_listener(CharacterList& ll, DisplayObject* elem);

    /// This function should return TRUE iff any action triggered
    /// by the event requires redraw, see \ref events_handling for
    /// more info.
    bool fire_mouse_event();

    /// \brief
    /// Return the topmost entity covering the given point
    /// and enabled to receive mouse events.
    //
    /// Return NULL if no "active" entity is found under the pointer.
    ///
    /// Coordinates of the point are given in world coordinate space.
    /// (twips)
    ///
    /// @param x
    ///     X ordinate of the pointer, in world coordinate space (twips)
    ///
    /// @param y
    ///     Y ordinate of the pointer, in world coordiante space (twips).
    ///
    InteractiveObject* getTopmostMouseEntity(boost::int32_t x,
            boost::int32_t y) const;

    /// Delete DisplayObjects removed from the stage
    /// from the display lists
    void cleanupDisplayList();

    /// Advance a live DisplayObject
    //
    /// @param ch
    ///     The DisplayObject to advance, will NOT be advanced if unloaded
    ///
    static void advanceLiveChar(DisplayObject* ch);

    /// Advance all non-unloaded live chars
    void advanceLiveChars();

    /// Put the given movie at the given level 
    //
    /// @param movie
    /// The Movie to store at the given level.
    /// Its depth will be set to <num>+DisplayObject::staticDepthOffset and
    /// its name to _level<num>
    void setLevel(unsigned int num, Movie* movie);

    /// Boundaries of the Stage are always world boundaries
    /// and are only invalidated by changes in the background
    /// color.
    void setInvalidated() { _invalidated = true; }

    /// Every ::display call clears the invalidated flag
    //
    /// See setInvalidated();
    ///
    void clearInvalidated() { _invalidated = false; }

    /// An invalidated stage will trigger complete redraw
    //
    /// So, this method should return true everytime a complete
    /// redraw is needed. This is tipically only needed when
    /// the background changes.
    ///
    /// See setInvalidated() and clearInvalidated().
    ///
    bool isInvalidated() { return _invalidated; }

    /// Return the priority level of first action queue containing actions.
    //
    /// Scanned in proprity order (lower first)
    ///
    int minPopulatedPriorityQueue() const;

    /// Process all actions in the the given queue, till more actions
    /// are found in lower levels, in which case we have an earlier
    /// return.
    int processActionQueue(int lvl);

    bool processingActions() const
    {
        return (_processingActionLevel < apSIZE);
    }

    const DisplayObject* findDropTarget(boost::int32_t x, boost::int32_t y,
            DisplayObject* dragging) const;

    void handleActionLimitHit(const std::string& ref);
    /// A list of AdvanceableCharacters
    //
    /// This is a list (not a vector) as we want to allow
    /// ::advance of each element to insert new DisplayObjects before
    /// the start w/out invalidating iterators scanning the
    /// list forward for proper movie advancement
    typedef std::list<DisplayObject*> LiveChars;

    /// The list of advanceable DisplayObject, in placement order
    LiveChars _liveChars;

    typedef std::list<ExecutableCode*> ActionQueue;

    ActionQueue _actionQueue[apSIZE];

    /// Process all actions in the queue
    void processActionQueue();

    int m_viewport_x0, m_viewport_y0;

    /// Width and height of viewport, in pixels
    int m_viewport_width, m_viewport_height;

    rgba m_background_color;
    bool m_background_color_set;

    float m_timer;
    int m_mouse_x, m_mouse_y, m_mouse_buttons;

    MouseButtonState  _mouseButtonState;

    /// Objects requesting a callback on every movie_root::advance()
    typedef std::set<ActiveRelay*> ObjectCallbacks;
    ObjectCallbacks _objectCallbacks;

    LoadCallbacks _loadCallbacks;

    typedef std::map<int, Timer*> TimerMap;

    TimerMap _intervalTimers;
    unsigned int _lastTimerId;

    /// Characters for listening key events
    KeyListeners m_key_listeners;

    /// Objects listening for mouse events (down,up,move)
    MouseListeners m_mouse_listeners;

    /// The DisplayObject currently holding focus, or 0 if no focus.
    DisplayObject* _currentFocus;

    /// @todo fold this into m_mouse_button_state?
    drag_state m_drag_state;

    typedef MovieClip* LevelMovie;
    typedef std::map<int, LevelMovie> Levels;

    /// The movie instance wrapped by this movie_root
    //
    /// We keep a pointer to the base MovieClip class
    /// to avoid having to replicate all of the base class
    /// interface to the Movie class definition
    Levels _movies;

    /// The root movie. This is initially the same as getLevel(0) but might
    /// change during the run. It will be used to setup and retrive initial
    /// stage size
    Movie* _rootMovie;

    /// See setInvalidated
    bool _invalidated;

    /// This is set to true if execution of scripts
    /// aborted due to action limit set or whatever else
    bool _disableScripts;

    int _processingActionLevel;
    
    /// filedescriptor to write to for host application requests
    //
    /// -1 if none
    int _hostfd;

    /// The display quality of the entire movie.
    //
    /// This is here, not just in the Renderer, so that AS compatibility
    /// does not rely on the presence of a renderer.
    Quality _quality;

    std::bitset<4u> _alignMode;

	bool _showMenu;
    
    ScaleMode _scaleMode;
    
    DisplayState _displayState;
    
    // The maximum number of recursions e.g. when finding
    // 'super', set in the ScriptLimits tag.
    boost::uint16_t _recursionLimit;

    // The timeout in seconds for script execution, in the
    // ScriptLimits tag.    
    boost::uint16_t _timeoutLimit;

    // delay between movie advancement, in milliseconds
    unsigned int _movieAdvancementDelay;

    // time of last movie advancement, in milliseconds
    unsigned int _lastMovieAdvancement;

    size_t _unnamedInstance;

};

DSOEXPORT short stringToStageAlign(const std::string& s);

} // namespace gnash

#endif // GNASH_MOVIE_ROOT_H

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
