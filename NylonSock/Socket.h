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
//make sure headers are only included by this header
#if defined(_SYS_SOCKET_H) || defined(_SYS_TYPES_H) || defined(_NETDB_H) || defined(_UNISTD_H)
#error Headers should only be included by NylonSock!
#endif
#define PLAT_UNIX

#elif defined(__APPLE__)
#if defined(_SYS_SOCKET_H_) || defined(_SYS_TYPES_H_) || defined(_NETDB_H_) || defined(_UNISTD_H_)
#error Headers should only be included by NylonSock!
#endif
#define PLAT_APPLE

#elif defined(_WIN32)

#define PLAT_WIN
#endif

#if defined(PLAT_UNIX) || defined(PLAT_APPLE)

#define UNIX_HEADER
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>

//make it easier for crossplatform
constexpr char INVALID_SOCKET = -1;
constexpr char SOCKET_ERROR = -1;

#elif defined(_WIN32)
#define WIN_HEADER
#endif

#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cmath>
//debug
#include <iostream>

namespace NylonSock
{
    class Error : public std::runtime_error
    {
        //general exception class
    public:
        Error(std::string what) : std::runtime_error(what + " " + strerror(errno) )
        {
            std::cerr << this->what() << std::endl;
        }
    };
    
    class Socket
    {
    private:
        //socket wrapper that manages socket
        class SocketWrapper
        {
        private:
            int _sock;
        public:
            SocketWrapper(const addrinfo& res)
            {
                _sock = ::socket(res.ai_family, res.ai_socktype, res.ai_protocol);
                if(_sock == INVALID_SOCKET)
                {
                    throw Error("Failed to create socket");
                }
            }
            
            SocketWrapper(int sock) : _sock(sock)
            {
            };
            
            SocketWrapper()
            {
                _sock = INVALID_SOCKET;
            }
            
            ~SocketWrapper()
            {
#ifdef UNIX_HEADER
                close(_sock);
#elif WIN_HEADER
                closesocket(_sock)
#endif
            }
            
            operator int()
            {
                return get();
            }
            
            int get() const
            {
                return _sock;
            }
        };
        
        class AddrWrapper
        {
        private:
            addrinfo* _info;
        public:
            AddrWrapper(const char* node, const char* service, const addrinfo* hints)
            {
                
                _info = {0};
                char success = ::getaddrinfo(node, service, hints, &_info);
                if(success != 0)
                {
                    throw Error(std::string{"Failed to get addrinfo: "} + gai_strerror(success) );
                }
            }
            
            AddrWrapper()
            {
                //pray this gets deallocated by freeaddrinfo
                _info = new addrinfo;
            }
            
            ~AddrWrapper()
            {
                freeaddrinfo(_info);
            }
            
            operator addrinfo*() const
            {
                return get();
            }
            
            addrinfo* operator->() const
            {
                return get();
            }
            
            addrinfo* get() const
            {
                return _info;
            }
        };
        
        std::shared_ptr<AddrWrapper> _info;
        std::shared_ptr<SocketWrapper> _sw;
    public:
        Socket(const char* node, const char* service, const addrinfo* hints)
        {
            _info = std::make_shared<AddrWrapper>(node, service, hints);
            
            //socket creation
            _sw = std::make_shared<SocketWrapper>(*_info->get() );
        }
        
        Socket(std::string node, std::string service, const addrinfo* hints) : Socket(node.c_str(), service.c_str(), hints)
        {
        }
        
        Socket(const sockaddr_storage* data, int port)
        {
            //gotta allocate that memory yo
            _info = std::make_shared<AddrWrapper>();
            
            //sets socket
            _sw = std::make_shared<SocketWrapper>(port);
            
            _info->get()->ai_family = data->ss_family;
            if(data->ss_family == AF_INET)
            {
                //copies that data
                //I PRAY that freeaddrinfo deletes this malloc!
                //using malloc instead of new because socket is c
                _info->get()->ai_addr = (sockaddr*)malloc(sizeof(sockaddr) );
                std::memcpy(_info->get()->ai_addr, (sockaddr*)data, sizeof(*_info->get()->ai_addr) );
                _info->get()->ai_addrlen = sizeof(sockaddr_in);
                       
            }
            else
            {
                //ipv6
                //copies that data
                //I PRAY that freeaddrinfo deletes this malloc!
                //using malloc instead of new because sockets is c libraries
                _info->get()->ai_addr = (sockaddr*)malloc(sizeof(sockaddr) );
                std::memcpy(_info->get()->ai_addr, (sockaddr*)data, sizeof(*_info->get()->ai_addr) );
                _info->get()->ai_addrlen = sizeof(sockaddr_in6);
            }
        }
        
        ~Socket() = default;
        
        const addrinfo* operator->() const
        {
            //returns socket info
            return get();
        }
        
        const addrinfo* get() const
        {
            return *_info.get();
        }
        
        operator int() const
        {
            return port();
        }
        
        int port() const
        {
            return *_sw.get();
        }
        
        size_t size() const
        {
            //returns size of addrinfo
            return sizeof(_info);
        }
        
    };
    
    
    void bind(const Socket& sock)
    {
        const int y = 1;
        for(char i = 0; i < 1; i++)
        {
            //binds socket to port
            char success = ::bind(sock, sock->ai_addr, sock->ai_addrlen);
            if(success == SOCKET_ERROR)
            {
                //try clearning the port
                if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y) ) == SOCKET_ERROR)
                {
                    //there is an error
                    throw Error("Failed to bind socket");
                }
            }
        }
    }
    
    void connect(const Socket& sock)
    {
        //connects socket to host
        char success = ::connect(sock, sock->ai_addr, sock->ai_addrlen);
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to connect to socket");
        }
    }
    
    void listen(const Socket& sock, unsigned char backlog)
    {
        char success = ::listen(sock, static_cast<int>(backlog) );
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to listen to socket");
        }
    }
    
    Socket accept(const Socket& sock)
    {
        //0 initialized again!
        //we also want that ipv6
        sockaddr_storage t_data = {0};
        socklen_t t_size = sizeof(t_data);
        //                         I really don't like c casts...
        char port = ::accept(sock, (sockaddr*)(&t_data), &t_size);
        if(port == INVALID_SOCKET)
        {
            throw Error("Failed to accept socket");
        }
        
        return {&t_data, port};
    }
    
    size_t send(const Socket& sock, const void* buf, size_t len, int flags)
    {
        //returns amount of data sent
        //may not equal total amount of data!
        auto size = ::send(sock, buf, len, flags);
        if(size == SOCKET_ERROR)
        {
            throw Error("Failed to send data to socket");
        }
        
        return size;
    }
    
    size_t recv(const Socket& sock, void* buf, size_t len, int flags)
    {
        auto size = ::recv(sock, buf, len, flags);
        if(size == SOCKET_ERROR)
        {
            throw Error("Failed to receive data from socket");
        }
        else if(size == 0)
        {
            throw Error("Recieve has failed due to socket being closed");
        }
        
        return size;
    }
    
    size_t sendto(const Socket& sock, const void* buf, size_t len, unsigned int flags, const Socket& dest)
    {
        auto size = ::sendto(sock, buf, len, flags, dest->ai_addr, dest->ai_addrlen);
        if(size == SOCKET_ERROR)
        {
            throw Error("Failed to sendto data to socket");
        }
        
        return size;
    }
    
    size_t recvfrom(const Socket& sock, void* buf, size_t len, unsigned int flags, const Socket& dest)
    {
        auto rm_const = dest->ai_addrlen;
        auto size = ::recvfrom(sock, buf, len, flags, dest->ai_addr, &rm_const);
        if(size == SOCKET_ERROR)
        {
            throw Error("Failed to recvfrom data from destination");
        }
        
        return size;
    }
    
    Socket getpeername(const Socket& sock)
    {
        //0 initialized again!
        //we also want that ipv6
        sockaddr_storage t_data = {0};
        socklen_t t_size = sizeof(t_data);
        //                         I really don't like c casts...
        char port = ::getpeername(sock, (sockaddr*)(&t_data), &t_size);
        if(port == SOCKET_ERROR)
        {
            throw Error("Failed to get peername");
        }
        
        return {&t_data, port};
    }
    
    std::string gethostname()
    {
        constexpr size_t BUFFER_SIZE = 256;
        char name[BUFFER_SIZE];
        
        char success = ::gethostname(name, sizeof(name) );
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to get hostname");
        }
        
        return {name};
    }
    
    void fcntl(const Socket& sock, long args)
    {
        constexpr int SOCKET_TYPE = F_SETFL;
        if(args != O_NONBLOCK && args != O_ASYNC)
        {
            throw Error("Arg to fcntl is not O_NONBLOCK or O_ASYNC");
        }
        
        char success = ::fcntl(sock, SOCKET_TYPE, args);
        
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to fcntl socket");
        }
        
    }
    
    class FD_Set
    {
    private:
        fd_set _set;
        std::vector<int> _sock;
        static bool cpy_exist;
    public:
        void set(const Socket& sock)
        {
            FD_SET(sock, &_set);
            _sock.push_back(sock);
            
            //sorts it automatically
            std::sort(_sock.begin(), _sock.end() );
        }
        
        void clr(const Socket& sock)
        {
            FD_CLR(sock, &_set);
            _sock.erase(std::remove(_sock.begin(), _sock.end(), sock), _sock.end() );
        }
        
        void zero()
        {
            FD_ZERO(&_set);
            _sock.clear();
        }
        
        bool isset(const Socket& sock) const
        {
            return FD_ISSET(sock, &_set);
        }
        
        FD_Set()
        {
            //only one copy of FD_Set exists at a time
            if(cpy_exist)
            {
                throw Error("Only one instance of FD_Set should exist at a time");
            }
            
            cpy_exist = true;
        }
        
        ~FD_Set()
        {
            zero();
            cpy_exist = false;
        }
        
        int getMax() const
        {
            return _sock.back();
        }
    };
    
    std::vector<fd_set> select(const FD_Set& set, timeval timeout)
    {
        constexpr char NUM_DATA = 3;
        std::vector<fd_set> data;
        data.resize(NUM_DATA);
        
        //                                        read      write     except
        char success = ::select(set.getMax() + 1, &data[0], &data[1], &data[2], &timeout);
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to select ports");
        }
        
        return data;
    }
    
    class TimeVal
    {
    private:
        int _tv_sec;
        int _tv_usec;
    public:
        TimeVal(unsigned int milli)
        {
            //find seconds
            _tv_sec = floor(milli / 1000);
            _tv_usec = (milli % 1000) * 1000;
        }
        
        operator timeval() const
        {
            return {_tv_sec, _tv_usec};
        }
    };
    
    void setsockopt(const Socket& sock, int level, int optname,
                    const void *optval, socklen_t optlen)
    {
        char success = ::setsockopt(sock, level, optname, optval, optlen);
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to setsockopt");
        }
    }
    
    std::string inet_ntop(const Socket& sock)
    {
        constexpr size_t DATA_SIZE = 256;
        char data[DATA_SIZE];
        
        auto success = ::inet_ntop(sock->ai_family, sock->ai_addr , data, sizeof(data) );
        if(success == nullptr)
        {
            throw Error("Failed to inet_ntop");
        }
        
        return {data};
    }
    
}

#endif /* defined(__NylonSock__Socket__) */
