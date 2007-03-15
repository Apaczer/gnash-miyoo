// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef GNASH_VM_H
#define GNASH_VM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smart_ptr.h" // for boost::intrusive_ptr
#include "movie_root.h" // for composition

#include <memory> // for auto_ptr
#include <locale>

// Forward declarations
namespace gnash {
	class movie_definition;
	class as_object;
}

namespace gnash {

/// The virtual machine
//
/// This is the machine that executes all actions in the 
/// main movie, including the actions in movies loaded by
/// it.
///
/// Note that the target SWF version of the "main" movie
/// (the first movie loaded, the 'root' movie) drives
/// the operation, as depending on that version the Virtual
/// Machine acts differently, for backward compatibility.
/// 
/// The VM is initialized once for each *main* movie.
/// Gnash currently only supports a *single* VM as it uses
/// gloabls a lot. Definition of this class is aimed at
/// grouping the globals into a specific VM instance that
/// we might pass around in the future to allow multiple
/// movies runs.
/// For the moment, it will be a singleton, providing one-time
/// initialization.
///
class VM {

	/// Use VM::get() to access the singleton
	VM(movie_definition& movie);

	/// Don't copy
	VM(const VM&);

	/// Don't assign
	VM& operator=(const VM&);

	~VM();

	// We use an auto_ptr here to allow constructing
	// the singleton when the init() function is called.
	friend class std::auto_ptr<VM>;
	static std::auto_ptr<VM> _singleton;

	/// \brief
	/// Root movie, will be instantiated from the definition
	/// given to the init() function.
	///
	std::auto_ptr<movie_root> _root_movie;

	/// The _global ActionScript object
	boost::intrusive_ptr<as_object> _global;

	/// Target SWF version
	int _swfversion;

	/// Set the current Root movie.
	//
	/// Will be called by the init() function
	///
	void setRoot(movie_instance*);

	/// Set the _global Object for actions run by Virtual Machine
	//
	/// Will be called by the init() function
	/// 
	void setGlobal(as_object*);

public:

	/// \brief
	/// Initialize the virtual machine singleton with the given
	/// movie definition and return a reference to it.
	//
	/// An instance of the given movie will become the absolute
	/// root of the application (ActionScript's _level0)
	///
	/// Don't call this function twice, and make sure you have
	/// called this *before* you call VM::get()
	///
	/// @param movie
	///	The definition for the root movie.
	///	It is required that the given definition's
	///	create_movie_instance() method returns
	///	not NULL, or an assertion will fail.
	///	See movie_definition::create_root_instance()
	///	for more info.
	///
	static VM& init(movie_definition& movie);

	/// Return true if the singleton VM has been initialized
	static bool isInitialized();

	/// Get the singleton instance of the virtual machine
	//
	/// Make sure you called VM::init() before trying to
	/// get the singleton (an assertion would fail otherwise)
	///
	/// Use isInitialized() if you're unsure.
	///
	static VM& get();

	/// Get version of the SWF movie used to initialize this VM
	//
	/// This information will drive operations of the virtual machine
	///
	int getSWFVersion() const;

	/// Get version of the player, in a compatible representation
	//
	/// This information will be used for the System.capabilities.version
	/// and $version ActionScript variables.
	///
	const std::string& getPlayerVersion() const;

	/// Get a pointer to this VM's Root movie 
	movie_root& getRoot() const;

	/// Get a pointer to this VM's _global Object
	as_object* getGlobal() const;

	/// Get the SWF locale to use 
	std::locale& getLocale() const;

};

} // namespace gnash

#endif // GNASH_VM_H

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
