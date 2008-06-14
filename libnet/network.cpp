//
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "utility.h"
#include "log.h"
#include "network.h"

#include <sys/types.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#if defined(HAVE_WINSOCK_H) && !defined(__OS2__)
# include <winsock2.h>
# include <windows.h>
# include <sys/stat.h>
# include <io.h>
# include <ws2tcpip.h>
#else
# include <sys/time.h>
# include <unistd.h>
# include <sys/select.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <netdb.h>
# include <sys/param.h>
# include <sys/select.h>
#endif

#include "buffer.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

using namespace std;

namespace gnash {

static const char *DEFAULTPROTO = "tcp";
static const short DEFAULTPORT  = RTMP_PORT;

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff
#endif

Network::Network()
	:
	_ipaddr(INADDR_ANY),
	_sockfd(0),
	_listenfd(0),
	_port(0),
	_connected(false),
	_debug(false),
	_timeout(0)
{
//    GNASH_REPORT_FUNCTION;
#if defined(HAVE_WINSOCK_H) && !defined(__OS2__)
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(1, 1);		// Windows Sockets 1.1
    if (WSAStartup( wVersionRequested, &wsaData ) != 0) {
        log_error(_("Could not find a usable WinSock DLL"));
        exit(1);
    }
#endif

}

Network::~Network()
{
//    GNASH_REPORT_FUNCTION;
#if defined(HAVE_WINSOCK_H) && !defined(__OS2__)
    WSACleanup();
#else
    closeNet();
#endif
}

// Description: Create a tcp/ip network server. This creates a server
//              that listens for incoming socket connections. This
//              supports IP aliasing on the host, and will sequntially
//              look for IP address to bind this port to.
int
Network::createServer(void)
{
//    GNASH_REPORT_FUNCTION;

    return createServer(DEFAULTPORT);
}

// FIXME: Should also support IPv6 (AF_INET6)
int
Network::createServer(short port)
{
//    GNASH_REPORT_FUNCTION;

    struct protoent *ppe;
    struct sockaddr_in sock_in;
    int             on, type;
    int             retries = 0;
    in_addr_t       nodeaddr;

#if 0
    if (port < 1024) {
	log_error(_("Can't connect to privileged port #%d"), port);
	return -1;
    }
#endif
    
    if (_listenfd >= 2) {
	log_debug("already connected to port %hd", port);
	return _listenfd;
    }
    
    const struct hostent *host = gethostbyname("localhost");
    struct in_addr *thisaddr = reinterpret_cast<struct in_addr *>(host->h_addr_list[0]);
    _ipaddr = thisaddr->s_addr;
    memset(&sock_in, 0, sizeof(sock_in));

#if 0
    // Accept incoming connections only on our IP number
    sock_in.sin_addr.s_addr = thisaddr->s_addr;
#else
    // Accept incoming connections on any IP number
    sock_in.sin_addr.s_addr = INADDR_ANY;
#endif

    _ipaddr = sock_in.sin_addr.s_addr;
    sock_in.sin_family = AF_INET;
    sock_in.sin_port = htons(port);

    if ((ppe = getprotobyname(DEFAULTPROTO)) == 0) {
        log_error(_("unable to get protocol entry for %s"),
                DEFAULTPROTO);
        return -1;
    }

    // set protocol type
    if ( strcmp(DEFAULTPROTO, "udp") == 0) {
        type = SOCK_DGRAM;
    } else {
        type = SOCK_STREAM;
    }

    // Get a file descriptor for this socket connection
    _listenfd = socket(PF_INET, type, ppe->p_proto);

    // error, wasn't able to create a socket
    if (_listenfd < 0) {
        log_error(_("unable to create socket: %s"), strerror(errno));
        return -1;
    }

    on = 1;
    if (setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&on, sizeof(on)) < 0) {
        log_error(_("setsockopt SO_REUSEADDR failed"));
        return -1;
    }

    retries = 0;

    nodeaddr = inet_lnaof(*thisaddr);
    while (retries < 5) {
        if (bind(_listenfd, reinterpret_cast<struct sockaddr *>(&sock_in),
                 sizeof(sock_in)) == -1) {
            log_error(_("unable to bind to port %hd: %s"),
                    port, strerror(errno));
//                    inet_ntoa(sock_in.sin_addr), strerror(errno));
            retries++;
        }

	if (_debug) {
//		char  ascip[INET_ADDRSTRLEN];
//		inet_ntop(sock_in.sin_family, &_ipaddr, ascip, INET_ADDRSTRLEN);
		char *ascip = ::inet_ntoa(sock_in.sin_addr);
		log_debug(_("Server bound to service on %s, port %hd, using fd #%d"),
		    ascip, ntohs(sock_in.sin_port),
		    _listenfd);
	}

        if (type == SOCK_STREAM && listen(_listenfd, 5) < 0) {
            log_error(_("unable to listen on port: %hd: %s "),
                port, strerror(errno));
            return -1;
        }

	// We have a socket created
        _port = port;
        return _listenfd;
    }
    return -1;
}

// Description: Accept a new network connection for the port we have
//              created a server for.
// The default is to block.
int
Network::newConnection(void)
{
//    GNASH_REPORT_FUNCTION;

    return newConnection(true, _listenfd);
}

int
Network::newConnection(int fd)
{
//    GNASH_REPORT_FUNCTION;

    return newConnection(true, fd);
}

int
Network::newConnection(bool block)
{
//    GNASH_REPORT_FUNCTION;

    return newConnection(block, _listenfd);
}

int
Network::newConnection(bool block, int fd)
{
    GNASH_REPORT_FUNCTION;

    struct sockaddr	newfsin;
    socklen_t		alen;
    int			ret;
    struct timeval        tval;
    fd_set                fdset;
    int                   retries = 3;

    alen = sizeof(struct sockaddr_in);

    if (fd <= 2) {
        return -1;
    }
    if (_debug) {
	log_debug(_("Trying to accept net traffic on fd #%d for port %d"), fd, _port);
    }

    while (retries--) {
        // We use select to wait for the read file descriptor to be
        // active, which means there is a client waiting to connect.
        FD_ZERO(&fdset);
        // also return on any input from stdin
//         if (_console) {
//             FD_SET(fileno(stdin), &fdset);
//         }
        FD_SET(fd, &fdset);

        // Reset the timeout value, since select modifies it on return. To
        // block, set the timeout to zero.
        tval.tv_sec = 1;
        tval.tv_usec = 0;

        if (block) {
            ret = select(fd+1, &fdset, NULL, NULL, NULL);
        } else {
            ret = select(fd+1, &fdset, NULL, NULL, &tval);
        }

        if (FD_ISSET(0, &fdset)) {
	    if (_debug) {
		log_debug(_("There is data at the console for stdin"));
	    }
            return 1;
        }

        // If interupted by a system call, try again
        if (ret == -1 && errno == EINTR) {
            log_debug(_("The accept() socket for fd #%d was interupted by a system call"), fd);
        }

        if (ret == -1) {
            log_debug(_("The accept() socket for fd #%d never was available for writing"), fd);
            return -1;
        }

        if (ret == 0) {
            if (_debug) {
                log_debug(_("The accept() socket for fd #%d timed out waiting to write"), fd);
            }
        }
    }

#ifndef HAVE_WINSOCK_H
    fcntl(_listenfd, F_SETFL, O_NONBLOCK); // Don't let accept() block
#endif
    _sockfd = accept(fd, &newfsin, &alen);

    if (_sockfd < 0) {
        log_error(_("unable to accept: %s"), strerror(errno));
        return -1;
    }

    if (_debug) {
	log_debug(_("Accepting tcp/ip connection on fd #%d for port %d"), _sockfd, _port);
    }

    return _sockfd;
}

#ifdef _WIN32
/* from sys/socket.h */
typedef unsigned short      sa_family_t;

/* from sys/un.h */
#define UNIX_PATH_MAX   108

struct sockaddr_un {
    sa_family_t sun_family; /* AF_UNIX */
    char sun_path[UNIX_PATH_MAX];   /* pathname */
};

#endif /* _WIN32 */

// Connect to a named pipe
bool
Network::connectSocket(const string &sockname)
{
//    GNASH_REPORT_FUNCTION;

    struct sockaddr_un  addr;
    fd_set              fdset;
    struct timeval      tval;
    int                 ret;
    int                 retries;

    addr.sun_family = AF_UNIX;
    // socket names must be 108 bytes or less as specifiec in sys/un.h.
    strncpy(addr.sun_path, sockname.c_str(), 100);

    _sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (_sockfd < 0)
        {
            log_error(_("unable to create socket: %s"), strerror(errno));
            _sockfd = -1;
            return false;
        }

    retries = 2;
    while (retries-- > 0) {
        // We use select to wait for the read file descriptor to be
        // active, which means there is a client waiting to connect.
        FD_ZERO(&fdset);
        FD_SET(_sockfd, &fdset);

        // Reset the timeout value, since select modifies it on return. To
        // block, set the timeout to zero.
        tval.tv_sec = 5;
        tval.tv_usec = 0;

        ret = ::select(_sockfd+1, &fdset, NULL, NULL, &tval);

        // If interupted by a system call, try again
        if (ret == -1 && errno == EINTR)
            {
                log_debug(_("The connect() socket for fd %d was interupted by a system call"),
                        _sockfd);
                continue;
            }

        if (ret == -1)
            {
                log_debug(_("The connect() socket for fd %d never was available for writing"),
                        _sockfd);
#ifdef HAVE_WINSOCK_H
                ::shutdown(_sockfd, 0); // FIXME: was SHUT_BOTH
#else
                ::shutdown(_sockfd, SHUT_RDWR);
#endif
                _sockfd = -1;
                return false;
            }
        if (ret == 0) {
            log_error(_("The connect() socket for fd %d timed out waiting to write"),
                      _sockfd);
            continue;
        }

        if (ret > 0) {
            ret = ::connect(_sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
            if (ret == 0) {
                log_debug(_("\tsocket name %s for fd %d"), sockname, _sockfd);
                _connected = true;
                assert(_sockfd > 0);
                return true;
            }
            if (ret == -1) {
                log_error(_("The connect() socket for fd %d never was available for writing"),
                        _sockfd);
                _sockfd = -1;
                assert(!_connected);
                return false;
            }
        }
    }
    

#ifndef HAVE_WINSOCK_H
    fcntl(_sockfd, F_SETFL, O_NONBLOCK);
#endif

    _connected = true;
    assert(_sockfd > 0);
    return true;    
}

// Create a client connection to a tcp/ip based service
bool
Network::createClient(void)
{
//    GNASH_REPORT_FUNCTION;

    return createClient("localhost", RTMP_PORT);
}
bool
Network::createClient(short /* port */)
{
//    GNASH_REPORT_FUNCTION;

    return false;
}

bool
Network::createClient(const string &hostname)
{
//    GNASH_REPORT_FUNCTION;

    return createClient(hostname, RTMP_PORT);
}

bool
Network::createClient(const string &hostname, short port)
{
    GNASH_REPORT_FUNCTION;

    struct sockaddr_in  sock_in;
    fd_set              fdset;
    struct timeval      tval;
    int                 ret;
    int                 retries;
    char                thishostname[MAXHOSTNAMELEN];
    struct protoent     *proto;

    assert( ! connected() );

    if (port < 1024) {
        log_error(_("Can't connect to privileged port %hd"), port);
        _connected = false;
        return false;
    }

    _port = port;    
    log_debug(_("%s: to host %s at port %d"), __FUNCTION__, hostname, port);

    memset(&sock_in, 0, sizeof(struct sockaddr_in));
    memset(&thishostname, 0, MAXHOSTNAMELEN);
    if (hostname.size() == 0) {
        if (gethostname(thishostname, MAXHOSTNAMELEN) == 0) {
            log_debug(_("The hostname for this machine is %s"), thishostname);
        } else {
            log_debug(_("Couldn't get the hostname for this machine"));
            return false;
        }
    }
    const struct hostent *hent = ::gethostbyname(hostname.c_str());
    if (hent > 0) {
        ::memcpy(&sock_in.sin_addr, hent->h_addr, hent->h_length);
    }
    sock_in.sin_family = AF_INET;
    sock_in.sin_port = ntohs(static_cast<short>(port));

#if 0
    char ascip[INET_ADDRSTRLEN];
    inet_ntop(sock_in.sin_family, &sock_in.sin_addr.s_addr, ascip, INET_ADDRSTRLEN);
    log_debug(_("The IP address for this client socket is %s"), ascip);
#endif

    proto = ::getprotobyname("TCP");

    _sockfd = ::socket(PF_INET, SOCK_STREAM, proto->p_proto);
    if (_sockfd < 0)
        {
            log_error(_("unable to create socket: %s"), strerror(errno));
            _sockfd = -1;
            return false;
        }

    retries = 2;
    while (retries-- > 0) {
        // We use select to wait for the read file descriptor to be
        // active, which means there is a client waiting to connect.
        FD_ZERO(&fdset);
        FD_SET(_sockfd, &fdset);

        // Reset the timeout value, since select modifies it on return. To
        // block, set the timeout to zero.
        tval.tv_sec = 5;
        tval.tv_usec = 0;

        ret = ::select(_sockfd+1, &fdset, NULL, NULL, &tval);

        // If interupted by a system call, try again
        if (ret == -1 && errno == EINTR)
            {
                log_debug(_("The connect() socket for fd %d was interupted by a system call"),
                        _sockfd);
                continue;
            }

        if (ret == -1)
            {
                log_debug(_("The connect() socket for fd %d never was available for writing"),
                        _sockfd);
#ifdef HAVE_WINSOCK_H
                ::shutdown(_sockfd, 0); // FIXME: was SHUT_BOTH
#else
                ::shutdown(_sockfd, SHUT_RDWR);
#endif
                _sockfd = -1;
                return false;
            }
        if (ret == 0) {
            log_error(_("The connect() socket for fd %d timed out waiting to write"),
                      _sockfd);
            continue;
        }

        if (ret > 0) {
            ret = ::connect(_sockfd, reinterpret_cast<struct sockaddr *>(&sock_in), sizeof(sock_in));
            if (ret == 0) {
		char *ascip = ::inet_ntoa(sock_in.sin_addr);
// 		char ascip[INET_ADDRSTRLEN];
// 		inet_ntop(sock_in.sin_family, &sock_in.sin_addr.s_addr, ascip, INET_ADDRSTRLEN);
                log_debug(_("\tport %d at IP %s for fd %d"), port,
                        ascip, _sockfd);
                _connected = true;
                assert(_sockfd > 0);
                return true;
            }
            if (ret == -1) {
                log_error(_("The connect() socket for fd %d never was available for writing"),
                        _sockfd);
                _sockfd = -1;
                assert(!_connected);
                return false;
            }
        }
    }
    //  ::close(_sockfd);
    //  return false;

    printf("\tConnected at port %d on IP %s for fd #%d", port,
           ::inet_ntoa(sock_in.sin_addr), _sockfd);

#ifndef HAVE_WINSOCK_H
    fcntl(_sockfd, F_SETFL, O_NONBLOCK);
#endif

    _connected = true;
    _port = port;
    assert(_sockfd > 0);
    return true;
}

bool
Network::closeNet()
{
    GNASH_REPORT_FUNCTION;

    if ((_sockfd > 0) && (_connected)) {
        closeNet(_sockfd);
        _sockfd = 0;
        _connected = false;
    }

    return false;
}

bool
Network::closeNet(int sockfd)
{
    GNASH_REPORT_FUNCTION;

    int retries = 0;

    // If we can't close the socket, other processes must be
    // locked on it, so we wait a second, and try again. After a
    // few tries, we give up, cause there must be something
    // wrong.

    if (sockfd <= 0) {
        return true;
    }

    while (retries < 3) {
        if (sockfd) {
            // Shutdown the socket connection
#if 0
            if (shutdown(sockfd, SHUT_RDWR) < 0) {
                if (errno != ENOTCONN) {
                    cerr << "WARNING: Unable to shutdown socket for fd #"
                         << sockfd << strerror(errno) << endl;
                } else {
                    cerr << "The socket using fd #" << sockfd
                         << " has been shut down successfully." << endl;
                    return true;
                }
            }
#endif
            if (::close(sockfd) < 0) {
		// If we have a bad file descriptor, it's because
		// this got closed already, usually by another
		// thread being paranoid.
		if (errno != EBADF) {
		    log_error(_("Unable to close the socket for fd #%d: %s"),
			      sockfd, strerror(errno));
		}
#ifndef HAVE_WINSOCK_H
                sleep(1);
#endif
                retries++;
            } else {
		log_debug(_("Closed the socket on fd #%d for port %d"), sockfd, _port);
                return true;
            }
        }
    }
    return false;
}
// Description: Close an open socket connection.
bool
Network::closeConnection(void)
{
//    GNASH_REPORT_FUNCTION;

    closeConnection(_sockfd);
    _sockfd = 0;
    closeConnection(_listenfd);
    _listenfd = 0;
    _connected = false;

    return false;
}

bool
Network::closeConnection(int fd)
{
    GNASH_REPORT_FUNCTION;

    if (fd > 0) {
        ::close(fd);
	log_debug("%s: Closed fd #%d", __FUNCTION__, fd);
//        closeNet(fd);
    }

    return false;
}

// Read from the connection
int
Network::readNet(amf::Buffer *buffer)
{
    return readNet(_sockfd, buffer->reference(), buffer->size(), _timeout);
}

int
Network::readNet(byte_t *buffer, int nbytes)
{
    return readNet(_sockfd, buffer, nbytes, _timeout);
}

int
Network::readNet(byte_t *buffer, int nbytes, int timeout)
{
    return readNet(_sockfd, buffer, nbytes, timeout);
}

int
Network::readNet(int fd, byte_t *buffer, int nbytes)
{
    return readNet(fd, buffer, nbytes, _timeout);
}

int
Network::readNet(int fd, byte_t *buffer, int nbytes, int timeout)
{
    fd_set              fdset;
    int                 ret = -1;
    struct timeval      tval;

#ifdef NET_TIMING
    if (_timing_debug)
    {
        gettimeofday(&tp, NULL);
        read_start_time = static_cast<double>(tp.tv_sec)
            + static_cast<double>(tp.tv_usec*1e-6);
    }
#endif
    if (fd > 2) {
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);

        if (timeout == 0) {
	    ret = select(fd+1, &fdset, NULL, NULL, NULL);
	} else {	
	    tval.tv_sec = timeout;
	    tval.tv_usec = 0;
	    ret = select(fd+1, &fdset, NULL, NULL, &tval);
	}

        // If interupted by a system call, try again
        if (ret == -1 && errno == EINTR) {
            log_error (_("The socket for fd %d was interupted by a system call"), fd);
        }

        if (ret == -1) {
            log_error (_("The socket for fd %d was never available for reading"), fd);
            return -1;
        }

        if (ret == 0) {
	    if (_debug) {
		log_debug (_("The socket for fd %d timed out waiting to read"), fd);
	    }
            return 0;
        }

        ret = read(fd, buffer, nbytes);
	// If we read zero bytes, the network is closed, as we returned from the select()
	if (ret == 0) {
	    return -1;
	}
	
	if (_debug) {
	    log_debug (_("read %d bytes from fd %d from port %d"), ret, fd, _port);
	}
#if 0
	if (ret) {
	    log_debug (_("%s: Read packet data from fd %d (%d bytes): \n%s"),
		       __FUNCTION__, fd, ret, hexify(buffer, ret, true));
	}
#endif    
    }

    return ret;

}

// Write to the connection
int
Network::writeNet(amf::Buffer *buffer)
{
    return writeNet(buffer->reference(), buffer->size());
};

int
Network::writeNet(const std::string& buffer)
{
    return writeNet(reinterpret_cast<const byte_t *>(buffer.c_str()), buffer.size());
}

int
Network::writeNet(const byte_t *buffer, int nbytes)
{
    return writeNet(_sockfd, buffer, nbytes, _timeout);
}

// int
// Network::writeNet(const byte_t *buffer, int nbytes)
// {
//     return writeNet(_sockfd, buffer, nbytes, _timeout);
// }

// int
// Network::writeNet(int fd, const byte_t *buffer)
// {
//     return writeNet(fd, buffer, strlen(buffer), _timeout);
// }

int
Network::writeNet(int fd, const byte_t *buffer, int nbytes)
{
    return writeNet(fd, buffer, nbytes, _timeout);
}

int
Network::writeNet(int fd, const byte_t *buffer, int nbytes, int timeout)
{
    fd_set              fdset;
    int                 ret = -1;
    struct timeval      tval;

    // We need a writable, and not const point for byte arithmetic.
    byte_t *bufptr = const_cast<byte_t *>(buffer);

#ifdef NET_TIMING
    // If we are debugging the tcp/ip timings, get the initial time.
    if (_timing_debug)
    {
        gettimeofday(&starttime, 0);
    }
#endif
    if (fd > 2) {
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);

        // Reset the timeout value, since select modifies it on return
        if (timeout <= 0) {
            timeout = 5;
        }
        tval.tv_sec = timeout;
        tval.tv_usec = 0;
        ret = select(fd+1, NULL, &fdset, NULL, &tval);

        // If interupted by a system call, try again
        if (ret == -1 && errno == EINTR) {
            log_error (_("The socket for fd %d was interupted by a system call"), fd);
        }

        if (ret == -1) {
            log_error (_("The socket for fd %d was never available for writing"), fd);
        }

        if (ret == 0) {
            log_debug (_("The socket for fd %d timed out waiting to write"), fd);
	    return 0;
        }

        ret = write(fd, bufptr, nbytes);

        if (ret == 0) {
            log_error (_("Wrote zero out of %d bytes to fd %d: %s"), 
		nbytes, fd, strerror(errno));
            return ret;
        }
        if (ret < 0) {
            log_error (_("Couldn't write %d bytes to fd %d: %s"), 
		nbytes, fd, strerror(errno));
            return ret;
        }
        if (ret > 0) {
            bufptr += ret;
            if (ret != nbytes) {
		if (_debug) {
		    log_debug (_("wrote %d bytes to fd %d, expected %d"),
			       ret, fd, nbytes);
		}
            } else {
		if (_debug) {
		    log_debug (_("wrote %d bytes to fd %d for port %d"),
			       ret, fd, _port);
		}
//                return ret;
            }
        }
#if 0
	if (ret) {
	    log_debug (_("%s: Wrote packet data to fd %d: \n%s"),
		       __FUNCTION__, fd, hexify(buffer, ret, true));
	}
#endif    
    }

#ifdef NET_TIMING
    if (_timing_debug)
    {
        gettimeofday(&endtime, 0);

        if ((endtime.tv_sec - starttime.tv_sec) &&
            endtime.tv_usec - starttime.tv_usec)
        {
            log_debug (_("took %d usec to write (%d bytes)"),
		endtime.tv_usec - starttime.tv_usec, bytes_written);
        }
    }
#endif


    return ret;
}

void
Network::toggleDebug(bool val)
{
    // Turn on our own debugging
    _debug = val;

    // Turn on debugging for the utility methods
		// recursive on all control paths,
		// function will cause runtime stack overflow

		// toggleDebug(true);
}


} // end of gnash namespace

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
