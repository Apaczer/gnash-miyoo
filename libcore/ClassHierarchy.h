// 
//   Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef GNASH_CLASS_HIERARCHY_H
#define GNASH_CLASS_HIERARCHY_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "as_object.h"

#ifdef ENABLE_AVM2
# include "SafeStack.h"
# include "Class.h"
# include "Namespace.h"
# include "BoundValues.h"
# include "asException.h"
# include "Method.h"
#endif

#include <list>
#include <vector>
#include <ostream>

namespace gnash {

class Extension;
class as_object;

/// Register all of the ActionScript classes, with their dependencies.
class ClassHierarchy
{
public:
	struct ExtensionClass
	{
		/// The filename for the library relative to the plugins directory.
		std::string file_name;

		/// Initialization function name
		///
		/// The name of the function which will yield the prototype
		/// object. It should be a function with signature:
		/// void init_name(as_object &obj);
		/// which sets its prototype as the member 'name' in the
		/// object. See extensions/mysql/mysql_db.cpp function
		/// mysql_class_init
		std::string init_name;

        const ObjectURI uri;
        const ObjectURI super;

		/// The version at which this should be added.
		int version;
	};

	struct NativeClass
	{
		
        /// The type of function to use for initialization
		typedef void (*InitFunc)(as_object& obj, const ObjectURI& uri);

        NativeClass(InitFunc init, const ObjectURI& u, const ObjectURI& s,
                int ver)
            :
            initializer(init),
            uri(u),
            super(s),
            version(ver)
        {}

		/// The initialization function
		///
		/// See ExtensionClass.init_name for the necessary function.
		InitFunc initializer;

		/// The name of the class.
		const ObjectURI uri;

		/// The name of the inherited class.
		const ObjectURI super;

		/// The version at which this should be visible.
		int version;
	};
	
    /// \brief
	/// Construct the declaration object. Later set the global and
	/// extension objects using setGlobal and setExtension
	ClassHierarchy(as_object* global, Extension* e)
        :
		mGlobal(global),
        mExtension(e)
#ifdef ENABLE_AVM2
        ,
		mAnonNamespaces(),
        mGlobalNamespace(addNamespace(0)),
		_classMemory(),
        mExceptionMemory(),
		mMethodMemory(),
		mBoundValueMemory(),
        mBoundAccessorMemory()
#endif
	{}

	/// \brief
	/// Delete our private namespaces.
	~ClassHierarchy();

    typedef std::vector<NativeClass> NativeClasses;

	/// Declare an ActionScript class, with information on how to load it.
	///
	/// @param c
	/// The ExtensionClass structure which defines the class.
	///
	/// @return true, unless the class with c.name already existed.
	bool declareClass(ExtensionClass& c);

	/// Declare an ActionScript class and how to instantiate it from the core.
	///
	/// @param c
	/// The NativeClass structure which defines the class.
	///
	/// @return true, unless the class with c.name already existed.
	bool declareClass(const NativeClass& c);

	/// Declare a list of native classes.
	void declareAll(const NativeClasses& classes);

#ifdef ENABLE_AVM2

	/// The global namespace
	///
	/// Get the global namespace.  This is not the Global object -- it only
	/// contains the classes, not any globally available functions or anything
	/// else.
    abc::Namespace* getGlobalNs() { return mGlobalNamespace; }

	/// Find a namespace with the given uri.
	///
	/// @return 
	/// The namespace with the given uri or NULL if it doesn't exist.
    abc::Namespace* findNamespace(string_table::key uri)
	{
		namespacesContainer::iterator i;
		if (mNamespaces.empty())
			return NULL;
		i = mNamespaces.find(uri);
		if (i == mNamespaces.end())
			return NULL;
		return &i->second;
	}

	/// \brief
	/// Obtain a new anonymous namespace. Use this to let the object keep track
	/// of all namespaces, even private ones. Namespaces obtained in this way
	/// can't ever be found. (They must be kept and passed to the appropriate
	/// objects.)
	///
    abc::Namespace* anonNamespace(string_table::key uri)
	{
		mAnonNamespaces.grow(1); 
        abc::Namespace *n = &mAnonNamespaces.top(0); 
		n->setURI(uri); 
		return n; 
	}

	/// \brief
	/// Add a namespace to the set. Don't use to add unnamed namespaces.
	/// Will overwrite existing namespaces 'kind' and 'prefix' values. 
	/// Returns the added space.
    abc::Namespace* addNamespace(string_table::key uri)
	{
        abc::Namespace *n = findNamespace(uri);
		if (n) return n;
		// The set should create it automatically here. TODO: Make sure
		mNamespaces[uri].setURI(uri);
		return &mNamespaces[uri];
	}
	
    /// Create a new abc::Class object for use.
    abc::Class* newClass() {
        _classMemory.grow(1);
        return &_classMemory.top(0);
    }

	asException* newException() {
        mExceptionMemory.grow(1);
        return &mExceptionMemory.top(0);
    }

	/// Create a new Method object for use.
    abc::Method* newMethod() {
        mMethodMemory.grow(1);
        return &mMethodMemory.top(0);
    }

    abc::BoundValue* newBoundValue() {
        mBoundValueMemory.grow(1);
        return &mBoundValueMemory.top(0);
    }

    abc::BoundAccessor* newBoundAccessor() {
        mBoundAccessorMemory.grow(1);
        return &mBoundAccessorMemory.top(0);
    }

#endif

	/// Mark objects for garbage collector.
	void markReachableResources() const;

private:
	as_object* mGlobal;
	Extension* mExtension;

#ifdef ENABLE_AVM2
	typedef std::map<string_table::key, abc::Namespace> namespacesContainer;
	namespacesContainer mNamespaces;
	SafeStack<abc::Namespace> mAnonNamespaces;
    abc::Namespace* mGlobalNamespace;
	SafeStack<abc::Class> _classMemory;
	SafeStack<asException> mExceptionMemory;
	SafeStack<abc::Method> mMethodMemory;
	SafeStack<abc::BoundValue> mBoundValueMemory;
	SafeStack<abc::BoundAccessor> mBoundAccessorMemory;
#endif
};

std::ostream&
operator<< (std::ostream& os, const ClassHierarchy::NativeClass& c);

std::ostream&
operator<< (std::ostream& os, const ClassHierarchy::ExtensionClass& c);

} 
#endif 

