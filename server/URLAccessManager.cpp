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
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "URLAccessManager.h"
#include "URL.h"
#include "log.h"
#include "StringPredicates.h" // for case-insensitive host match

#include "rc.h" // for rcfile
#include <cerrno> // for errno :)

// temporary use of console for confirm load of network urls
#include <iconv.h>
#include <iostream>
#include <algorithm> // for find / find_if

#ifdef WIN32
# include <winsock2.h>
#else
# include <unistd.h>
#endif

#include <cstring> // for strerror
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <cassert>

namespace gnash {
namespace URLAccessManager {

/// Possible access policies for URLs
enum AccessPolicy {	

	/// Forbid access 
	BLOCK,

	/// Allow access
	GRANT
};

const char*
accessPolicyString(AccessPolicy policy)
{
	switch(policy)
	{
		case BLOCK:
			return "BLOCKED";
		case GRANT:
			return "GRANTED";
		default:
			return "UNKNOWN";
	}
}

// The default AccessPolicy when prompting user is not possible
// (this happens when input is not a tty, at the moment)
//static AccessPolicy defaultAccessPolicy = GRANT;

/// A cache of AccessPolicy defined for URLs
typedef std::map< std::string, AccessPolicy > AccessPolicyCache;

/// A global AccessPolicyCache
static AccessPolicyCache policyCache;


#if 0 // @@ this function has been replaced with a wrapper around host_check

// Is access allowed to given url ?
// This function uses the global AccessPolicyCache
// so once a policy is defined for an url it will
// be remembered for the whole run.
//
// Prompts the user on the tty. If inut is not a tty
// uses the global defaultAccessPolicy.
//
static bool
allow(std::string& url)
{
	// Look in cached policy first
	AccessPolicyCache::iterator it = policyCache.find(url);
	if ( it != policyCache.end() )
	{
		log_msg("%s access to %s (cached).\n",
			accessPolicyString(it->second),
			url.c_str());

		return ( it->second == GRANT );
	}

	if ( ! isatty(fileno(stdin)) )
	{
		log_msg("%s access to %s (input is not a terminal).\n",
			accessPolicyString(defaultAccessPolicy),
			url.c_str());

		// If we can't prompt user return default policy
		return ( defaultAccessPolicy == GRANT );
	}

	/// I still don't like this method, typing just
	/// a newline doesn't spit another prompt
	std::string yesno;
	do {
		std::cout << "Attempt to access url " << url << std::endl;
		std::cout << "Block it [yes/no] ? "; 
		std::cin >> yesno;
	} while (yesno != "yes" && yesno != "no");

	AccessPolicy userChoice;

	if ( yesno == "yes" ) {
		userChoice = BLOCK;
	} else {
		userChoice = GRANT;
	}

	// cache for next time
	policyCache[url] = userChoice;
	
	log_msg("%s access to %s (user choice).\n",
		accessPolicyString(userChoice),
		url.c_str());

	return userChoice;

}
#endif

// check host against black/white lists
// return true if we allow load from host, false otherwise
// it is assumed localhost/localdomain was already checked
static bool
host_check_blackwhite_lists(const std::string& host)
{
	using std::vector;
	using std::string;

	RcInitFile& rcfile = RcInitFile::getDefaultInstance();

	vector<string>::iterator it;

	vector<string> whitelist = rcfile.getWhiteList();
	if ( whitelist.size() )
	{
		// TODO: case-insensitive matching ? 
		it = find(whitelist.begin(), whitelist.end(), host);
		if ( it != whitelist.end() ) {
			log_security("Load from host %s granted (whitelisted).",
				host.c_str());
			return true;
		}

		// if there is a whitelist, anything NOT listed is denied
		log_security("Load from host %s forbidden "
			"(not in non-empty whitelist).",
			host.c_str());

		return false;
	}

	vector<string> blacklist = rcfile.getBlackList();
	// TODO: case-insensitive matching ? 
	it = find(blacklist.begin(), blacklist.end(), host);
	if ( it != blacklist.end() )
	{
		log_security("Load from host %s forbidden (blacklisted).",
			host.c_str());
		return false;
	}

	log_security("Load from host %s granted (default).",
		host.c_str());
	return true;
}

/// Return true if we allow load from host, false otherwise.
//
/// This function will check for localhost/localdomain (if requested)
/// and finally call host_check_blackwhitelists
/// 
static bool
host_check(const std::string& host)
{
//    GNASH_REPORT_FUNCTION;

    //log_msg("Checking security of host: %s", host.c_str());
    
    assert( ! host.empty() );

    RcInitFile& rcfile = RcInitFile::getDefaultInstance();
    
    bool check_domain = rcfile.useLocalDomain();
    bool check_localhost = rcfile.useLocalHost();

    // Don't bother gettin hostname if we're not going to need it
    if ( ! ( check_domain  || check_localhost ) )
    {
        return host_check_blackwhite_lists(host);
    }

    //
    // Get hostname
    //

    #define MAXHOSTNAMELEN 200
    char name[MAXHOSTNAMELEN];
    if ( -1 == gethostname(name, MAXHOSTNAMELEN) )
    {
        // FIXME: strerror is NOT thread-safe
        log_error("gethostname failed: %s", strerror(errno)); 
        return host_check_blackwhite_lists(host);
    }
    // From GETHOSTNAME(2): 
    // In case the NUL-terminated hostname does not fit,
    // no  error is returned, but the hostname is truncated. It is unspecified
    // whether the truncated hostname will be NUL-terminated.
    name[MAXHOSTNAMELEN-1] = '\0'; // unlikely, still worth making sure...

    // ok, let's use std::strings... we're a C++ program after all !
    std::string hostname(name); // the hostname
    std::string domainname;     // the domainname

    // Split hostname/domainname or take it all as an hostname if
    // no dot is found
    std::string::size_type dotloc = hostname.find('.', 0);
    if ( dotloc != std::string::npos ) {
        domainname = hostname.substr(dotloc+1);
        hostname.erase(dotloc);
    }

    if ( check_domain && domainname != host ) {
	log_security("Load from host %s forbidden (not in the local domain).",
		host.c_str());
        return false;
	}
    
    if ( check_localhost && hostname != host ) {
	log_security("Load from host %s forbidden (not on the local host).",
		host.c_str());
        return false;
    }

    return host_check_blackwhite_lists(host);

}

bool
allowHost(const std::string& host, short /* port */)
{
	return allowHost(host);
}

bool
allowHost(const std::string& host)
{
	if (host.size() == 0) {
		return true;
	}
	return host_check(host);
}

bool
allow(const URL& url)
{
	// We might reintroduce use of an AccessPolicy cache

	std::string host = url.hostname();

	// always allow from local host
	if (host.size() == 0) {
		return true;
	}
	return host_check(host);
}


} // AccessManager
} // namespace gnash

