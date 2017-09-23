//
//  Socket.h
//  NylonSock
//
//  Created by Wiley Yu(AN) on 12/16/15.
//  Copyright (c) 2015 Wiley Yu. All rights reserved.
//

#ifndef __NylonSock__Socket__
#define __NylonSock__Socket__

#include "Definitions.h"

#ifdef UNIX_HEADER
#include <sys/socket.h>
#include <netdb.h>
#include <sys/fcntl.h>

typedef int SOCKET;
#elif defined(PLAT_WIN)

//windows is trippy
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define NOMINMAX

#include <ws2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

//if not built with cmake
#pragma comment(lib, "ws2_32.lib")

enum PortBlockers
{
	O_NONBLOCK = FIONBIO,
	O_ASYNC = FIONBIO
};
#endif

#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cmath>
#include <set>
#include <mutex>

//Forward Declaration!
struct timeval;

//THE MEATY STUFF
namespace NylonSock
{
	void NSInit();
	void NSRelease();

    class NSHelper
    {
    private:
        static int _cw;

    public:
        NSHelper();
        NSHelper(const NSHelper& that);
        NSHelper(NSHelper&& that) = delete;
        ~NSHelper();
    };

    class Error : public std::runtime_error
    {
    public:
        Error(std::string what);
        Error(std::string what, bool null);
    };

	class SOCK_CLOSED : public Error
	{
	public:
		SOCK_CLOSED(std::string what);
	};
    
    class PEER_RESET : public Error
    {
    public:
        PEER_RESET(std::string what);
    };
    
    class Socket
    {
    private:
        //socket wrapper that manages socket
        class SocketWrapper;
        class AddrWrapper;

        std::shared_ptr<AddrWrapper> _info;
        std::shared_ptr<SocketWrapper> _sw;

        NSHelper _the_help{};
    public:
        Socket(const char* node, const char* service, const addrinfo* hints, bool autoconnect = false);
        Socket(std::string node, std::string service, const addrinfo* hints, bool autoconnect = false);
        Socket(SOCKET port);
        Socket(SOCKET port, const sockaddr_storage* data);
        Socket() = default;
        ~Socket() = default;
        
        const addrinfo* operator->() const;
        const addrinfo* get() const;

        SOCKET port() const;

        size_t size() const;
        bool operator ==(const Socket& that) const;

		void freeaddrinfo();
    };
    
    const Socket NULL_SOCKET{};
    
    void bind(Socket& sock);
    
    void connect(const Socket& sock);
    
    void listen(const Socket& sock, unsigned char backlog);
    
    Socket accept(const Socket& sock);
    
    size_t send(const Socket& sock, const void* buf, size_t len, int flags);
    
    size_t recv(const Socket& sock, void* buf, size_t len, int flags);
    
    size_t sendto(const Socket& sock, const void* buf, size_t len, unsigned int flags, const Socket& dest);
    
    size_t recvfrom(const Socket& sock, void* buf, size_t len, unsigned int flags, const Socket& dest);
    
    sockaddr_storage getpeername(const Socket& sock);
    
    std::string gethostname();
    
    void fcntl(const Socket& sock, long args);
    
    class FD_Set
    {
    private:
        class FD_Wrap;
        
        std::unique_ptr<FD_Wrap> _set;
        //sets are sorted. hurrah!
        //we don't need to worry about sets and ports in windows...
#ifndef PLAT_WIN
        std::set<int> _sock;
#endif
        
    public:
        void set(const Socket& sock);
        void set(fd_set& set);
        void clr(const Socket& sock);
        void zero();
        bool isset(const Socket& sock) const;
        
        fd_set get() const;
        
        size_t size() const;
#ifndef PLAT_WIN
		//returns size of set
        int getMax() const;
#endif
        
        FD_Set();
        ~FD_Set();
        FD_Set(const FD_Set& that);
    };
    
    //select
    //0 = read
    //1 = write
    //2 = except
    std::vector<FD_Set> select(const FD_Set& set, timeval timeout);
    
    class TimeVal;
    
    std::vector<FD_Set> select(const FD_Set& set, TimeVal timeout);
    
    class TimeVal
    {
    private:
        int _tv_sec;
        int _tv_usec;
    public:
        TimeVal(unsigned int milli);
        
        timeval get() const;
        operator timeval() const;
    };
    
    void setsockopt(const Socket& sock, int level, int optname, const void *optval, socklen_t optlen);
    
    std::string inet_ntop(const Socket& sock);
    
}

#endif /* defined(__NylonSock__Socket__) */
