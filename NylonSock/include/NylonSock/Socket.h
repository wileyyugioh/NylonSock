//
//  Socket.h
//  NylonSock
//
//  Created by Wiley Yu(AN) on 12/16/15.
//  Copyright (c) 2015 Wiley Yu. All rights reserved.
//

#ifndef __NylonSock__Socket__
#define __NylonSock__Socket__

#if defined(unix) || defined(__unix__) || defined(__unix)
#define PLAT_UNIX

#elif defined(__APPLE__)
#define PLAT_APPLE

#elif defined(_WIN32)
#define PLAT_WIN
#endif

#if defined(PLAT_UNIX) || defined(PLAT_APPLE)
#define UNIX_HEADER
#include <sys/socket.h>
#include <netdb.h>
#include <sys/fcntl.h>
#endif

#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cmath>
#include <set>

//Forward Declaration!
struct timeval;


//THE MEATY STUFF
namespace NylonSock
{
    class Error : public std::runtime_error
    {
    public:
        Error(std::string what);
    };
    
    class Socket
    {
    private:
        //socket wrapper that manages socket
        class SocketWrapper;
        class AddrWrapper;
        
        std::shared_ptr<AddrWrapper> _info;
        std::shared_ptr<SocketWrapper> _sw;
    public:
        Socket(const char* node, const char* service, const addrinfo* hints);
        Socket(std::string node, std::string service, const addrinfo* hints);
        Socket(const sockaddr_storage* data, int port);
        Socket() = default;
        ~Socket() = default;
        
        const addrinfo* operator->() const;
        const addrinfo* get() const;
        int port() const;
        size_t size() const;
        bool operator ==(const Socket& that) const;
    };
    
    const Socket NULL_SOCKET{};
    
    void bind(const Socket& sock);
    
    void connect(const Socket& sock);
    
    void listen(const Socket& sock, unsigned char backlog);
    
    Socket accept(const Socket& sock);
    
    size_t send(const Socket& sock, const void* buf, size_t len, int flags);
    
    size_t recv(const Socket& sock, void* buf, size_t len, int flags);
    
    size_t sendto(const Socket& sock, const void* buf, size_t len, unsigned int flags, const Socket& dest);
    
    size_t recvfrom(const Socket& sock, void* buf, size_t len, unsigned int flags, const Socket& dest);
    
    Socket getpeername(const Socket& sock);
    
    std::string gethostname();
    
    void fcntl(const Socket& sock, long args);
    
    class FD_Set
    {
    private:
        class FD_Wrap;
        
        std::unique_ptr<FD_Wrap> _set;
        //sets are sorted. hurrah!
        std::set<int> _sock;
        
    public:
        void set(const Socket& sock);
        void set(fd_set& set);
        void clr(const Socket& sock);
        void zero();
        bool isset(const Socket& sock) const;
        int getMax() const;
        
        //returns size of set
        size_t size() const;
        
        FD_Set();
        ~FD_Set();
        FD_Set(const FD_Set& that);
    };
    
    //not const reference because set.getMax needs to sort the array!
    std::vector<FD_Set> select(FD_Set& set, timeval timeout);
    
    class TimeVal
    {
    private:
        int _tv_sec;
        int _tv_usec;
    public:
        TimeVal(unsigned int milli);
        
        operator timeval() const;
    };
    
    void setsockopt(const Socket& sock, int level, int optname, const void *optval, socklen_t optlen);
    
    std::string inet_ntop(const Socket& sock);
    
}

#endif /* defined(__NylonSock__Socket__) */
