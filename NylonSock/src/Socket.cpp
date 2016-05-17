//
//  Socket.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/1/16.
//  Copyright (c) 2016 Wiley Yu. All rights reserved.
//

#include "Definitions.h"

#include "Socket.h"

#if defined(UNIX_HEADER)

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdlib.h>

//make it easier for crossplatform
constexpr char INVALID_SOCKET = -1;
constexpr char SOCKET_ERROR = -1;

typedef int SOCKET;

#elif defined(PLAT_WIN)
constexpr int SHUT_RD = SD_RECEIVE;
constexpr int SHUT_WR = SD_SEND;
constexpr int SHUT_RDWR = SD_BOTH;
#endif

#include <cstring>

namespace NylonSock
{
    void NSInit()
    {
#ifdef PLAT_WIN
        constexpr char major = 2;
        constexpr char minor = 2;
        WSADATA wsaData;
        
        char success = WSAStartup(MAKEWORD(major, minor), &wsaData);
        if (success != 0)
        {
            throw Error("Failed to start Winsock2.2");
        }
        
#endif
    }
    
    void NSRelease()
    {
#ifdef PLAT_WIN
        WSACleanup();
#endif
    }
    
    Error::Error(std::string what) : std::runtime_error(what + " " + strerror(errno) )
    {
    }
    
    Error::Error(std::string what, bool null) : std::runtime_error(what)
    {
    }
    
    SOCK_CLOSED::SOCK_CLOSED(std::string what) : Error(what) {};
    
    PEER_RESET::PEER_RESET(std::string what) : Error(what) {};
    
    //needs to be in shared ptr
    class Socket::SocketWrapper
    {
    private:
        SOCKET _sock;
        
    public:
        SocketWrapper(addrinfo& res)
        {
            //loop through all of the ai until one works
            addrinfo* ptr;
            for(ptr = &res; ptr != nullptr; ptr = res.ai_next)
            {
                _sock = ::socket(res.ai_family, res.ai_socktype, res.ai_protocol);
                if(_sock != INVALID_SOCKET)
                {
                    break;
                }
            }
            
            if(_sock == INVALID_SOCKET)
            {
                throw Error("Failed to create socket");
            }
            
            res = *ptr;
        }
        
        SocketWrapper(int sock) : _sock(sock)
        {
            
        }
        
        SocketWrapper()
        {
            _sock = INVALID_SOCKET;
        }
        
        SocketWrapper(const SocketWrapper& that) = delete;
        SocketWrapper(SocketWrapper&& that) = delete;
        SocketWrapper& operator=(SocketWrapper that) = delete;
        SocketWrapper& operator=(SocketWrapper&& that) = delete;
        
        
        ~SocketWrapper()
        {
            if (_sock != INVALID_SOCKET)
            {
                shutdown(_sock, SHUT_RDWR);
#ifdef UNIX_HEADER
                close(_sock);
#elif defined(PLAT_WIN)
                closesocket(_sock);
#endif
            }
        }
        
        operator SOCKET()
        {
            return get();
        }
        
        SOCKET get() const
        {
            return _sock;
        }
        
    };
    
    //needs to be in shared ptr
    class Socket::AddrWrapper
    {
    private:
        addrinfo* _orig;
        addrinfo* _info;
        char _man;
    public:
        AddrWrapper(const char* node, const char* service, const addrinfo* hints)
        {
            
            _info = {0};
            int success = ::getaddrinfo(node, service, hints, &_info);
            if(success != 0)
            {
                throw Error(std::string{"Failed to get addrinfo: "} + gai_strerror(success) );
            }
            
            _orig = _info;
            _man = false;
        }
        
        AddrWrapper()
        {
            //pray this gets freed
            //have to use malloc because this is a c library?
            _info = (addrinfo*)(malloc(sizeof(addrinfo) ) );
            _orig = _info;
            _man = true;
        }
        
        ~AddrWrapper()
        {
            if(_man == false)
            {
                ::freeaddrinfo(_orig);
                _info = nullptr;
                _orig = nullptr;
            }
            else
            {
                free(_orig);
                _info = nullptr;
                _orig = nullptr;
            }
        }
        
        AddrWrapper(const AddrWrapper& that) = delete;
        AddrWrapper(AddrWrapper&& that) = delete;
        AddrWrapper& operator=(AddrWrapper that) = delete;
        AddrWrapper& operator=(AddrWrapper&& that) = delete;
        
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
        
        addrinfo* next()
        {
            _info = _info->ai_next;
            return _info;
        }
    };
    
    Socket::Socket(const char* node, const char* service, const addrinfo* hints)
    {
        _info = std::make_shared<AddrWrapper>(node, service, hints);
        
        //socket creation
        _sw = std::make_shared<SocketWrapper>(*_info->get() );
    }
    
    Socket::Socket(std::string node, std::string service, const addrinfo* hints) : Socket(node.c_str(), service.c_str(), hints)
    {
        
    }
    
    Socket::Socket(SOCKET port)
    {
        //used for a socket that got recv.
        //no addr_info because screw you
        
        //sets socket
        _sw = std::make_shared<SocketWrapper>(port);
        
    }
    
    Socket::Socket(SOCKET port, const sockaddr_storage* data)
    {
        //this is for storing data!
        //yay!
        //gotta allocate that memory yo
        _info = std::make_shared<AddrWrapper>();
        
        //sets socket
        _sw = std::make_shared<SocketWrapper>(port);
        
        _info->get()->ai_family = data->ss_family;
        
        //copies that data
        //I PRAY that freeaddrinfo deletes this malloc!
        //using malloc instead of new because socket is c
        _info->get()->ai_addr = (sockaddr*)malloc(sizeof(sockaddr) );
        *_info->get()->ai_addr = *(sockaddr*)data;
        
        //not needed?
        //_info->get()->ai_addrlen = sizeof(data->ss_len);
    }
    
    const addrinfo* Socket::operator->() const
    {
        //returns socket info
        return get();
    }
    
    const addrinfo* Socket::get() const
    {
        //purposely throws if Socket is a recv one
        if (_info == nullptr)
        {
            throw Error("Addrinfo hasn't been copied or has already been freed by bind call!");
        }
        return *_info.get();
    }
    
    SOCKET Socket::port() const
    {
        return _sw.get()->get();
    }
    
    size_t Socket::size() const
    {
        //returns size of addrinfo
        return sizeof(_info);
    }
    
    bool Socket::operator==(const Socket& that) const
    {
        return (_info == that._info && _sw == that._sw);
    }
    
    void Socket::freeaddrinfo()
    {
        _info = nullptr;
    }
    
    void bind(Socket& sock)
    {
        
        //true
        const int y = 1;
        
        //binds socket to port
        char success = ::bind(sock.port(), sock->ai_addr, sock->ai_addrlen);
        if(success == SOCKET_ERROR)
        {
            //try clearing the port
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y) );
        }
        
        //clears addrinfo
        sock.freeaddrinfo();
    }
    
    void connect(const Socket& sock)
    {
        //connects socket to host
        char success = ::connect(sock.port(), sock->ai_addr, sock->ai_addrlen);
        
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to connect to socket");
        }
    }
    
    void listen(const Socket& sock, unsigned char backlog)
    {
        char success = ::listen(sock.port(), static_cast<int>(backlog) );
        
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
        SOCKET port = ::accept(sock.port(), (sockaddr*)(&t_data), &t_size);
        
        if(port == INVALID_SOCKET)
        {
            //requires select to test before
            throw Error("Failed to accept socket");
        }
        
        return {port, &t_data};
    }
    
    size_t send(const Socket& sock, const void* buf, size_t len, int flags)
    {
        //returns amount of data sent
        //may not equal total amount of data!
        size_t size = 0;
        while(size != len)
        {
#ifndef PLAT_WIN
            size += ::send(sock.port(), buf, len, flags);
#else
            //needs to be cast to const char* for winsock2
            size += ::send(sock.port(), (const char*)buf, len, flags);
#endif
            
            if(size == SOCKET_ERROR)
            {
                throw Error("Failed to send data to socket");
            }
        }
        
        return size;
    }
    
    size_t recv(const Socket& sock, void* buf, size_t len, int flags)
    {
#ifndef PLAT_WIN
        auto size = ::recv(sock.port(), buf, len, flags);
#else
        //casting to char* for winsock
        auto size = ::recv(sock.port(), (char*)buf, len, flags);
#endif
        
        if(size == SOCKET_ERROR && errno == ECONNRESET)
        {
            throw PEER_RESET("Connection reset by peer");
        }
        
        if(size == SOCKET_ERROR && errno != EWOULDBLOCK)
        {
            throw Error(std::string{"Failed to receive data from socket."});
        }
        
        if(size == SOCKET_ERROR && errno == EWOULDBLOCK)
        {
            return 0;
        }
        
        if(size == 0)
        {
            throw SOCK_CLOSED("Receive has failed due to socket being closed");
        }
        
        return size;
    }
    
    size_t sendto(const Socket& sock, const void* buf, size_t len, unsigned int flags, const Socket& dest)
    {
#ifndef PLAT_WIN
        auto size = ::sendto(sock.port(), buf, len, flags, dest->ai_addr, dest->ai_addrlen);
#else
        auto size = ::sendto(sock.port(), (const char*)buf, len, flags, dest->ai_addr, dest->ai_addrlen);
#endif
        if(size == SOCKET_ERROR)
        {
            throw Error("Failed to sendto data to socket");
        }
        
        return size;
    }
    
    size_t recvfrom(const Socket& sock, void* buf, size_t len, unsigned int flags, const Socket& dest)
    {
        auto rm_const = dest->ai_addrlen;
#ifndef PLAT_WIN
        auto size = ::recvfrom(sock.port(), buf, len, flags, dest->ai_addr, &rm_const);
#else
        int rm_const_cast = static_cast<int>(rm_const);
        auto size = ::recvfrom(sock.port(), (char*)buf, len, flags, dest->ai_addr, &rm_const_cast);
#endif
        if(size == SOCKET_ERROR)
        {
            throw Error("Failed to recvfrom data from destination");
        }
        
        return size;
    }
    
    sockaddr_storage getpeername(const Socket& sock)
    {
        //0 initialized again!
        //we also want that ipv6
        sockaddr_storage t_data = {0};
        socklen_t t_size = sizeof(t_data);
        //                         I really don't like c casts...
        int port = ::getpeername(sock.port(), (sockaddr*)(&t_data), &t_size);
        if(port == SOCKET_ERROR)
        {
            throw Error("Failed to get peername");
        }
        
        
        return t_data;
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
#ifndef PLAT_WIN
        constexpr int SOCKET_TYPE = F_SETFL;
#endif
        if(args != O_NONBLOCK && args != O_ASYNC)
        {
            throw Error("Arg to fcntl is not O_NONBLOCK or O_ASYNC");
        }
        
#ifndef PLAT_WIN
        char success = ::fcntl(sock.port(), SOCKET_TYPE, args);
#else
        u_long is_true = 1;
        char success = ioctlsocket(sock.port(), args, &is_true);
#endif
        
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to fcntl socket");
        }
        
    }
    
    class FD_Set::FD_Wrap
    {
    private:
        std::unique_ptr<fd_set> _set;
        
        static void swap(FD_Wrap& that, FD_Wrap& thus)
        {
            std::swap(that._set, thus._set);
        }
    public:
        FD_Wrap()
        {
            _set = std::make_unique<fd_set>();
        };
        
        FD_Wrap(const FD_Wrap& that)
        {
            *_set = *that._set;
        }
        
        FD_Wrap(FD_Wrap&& that) noexcept : FD_Wrap()
        {
            swap(*this, that);
        }
        
        FD_Wrap& operator=(FD_Wrap that)
        {
            swap(*this, that);
            
            return *this;
        }
        
        ~FD_Wrap()
        {
            FD_ZERO(_set.get() );
        }
        
        void set(const fd_set& set)
        {
            *_set = set;
        }
        
        fd_set* get()
        {
            return _set.get();
        }
        
        operator fd_set*()
        {
            return get();
        }
    };
    
    FD_Set::FD_Set()
    {
        _set = std::make_unique<FD_Wrap>();
        
    };
    FD_Set::~FD_Set() = default;
    
    FD_Set::FD_Set(const FD_Set& that)
    {
        *_set = *that._set;
#ifndef PLAT_WIN
        _sock = that._sock;
#endif
    }
    
    void FD_Set::set(const Socket& sock)
    {
        FD_SET(sock.port(), _set->get() );
#ifndef PLAT_WIN
        _sock.insert(sock.port() );
#endif
    }
    
    void FD_Set::set(fd_set& set)
    {
        _set->set(set);
        //watch out! fd_set structure is platform dependent
#if defined(PLAT_APPLE)
        for(auto& it : set.fds_bits)
        {
            if(it != 0)
            {
                _sock.insert(it);
                
            }
        }
#endif
    }
    
    void FD_Set::clr(const Socket& sock)
    {
        FD_CLR(sock.port(), _set->get() );
#ifndef PLAT_WIN
        _sock.erase(sock.port() );
#endif
    }
    
    void FD_Set::zero()
    {
        FD_ZERO(_set->get() );
#ifndef PLAT_WIN
        _sock.clear();
#endif
    }
    
    bool FD_Set::isset(const Socket& sock) const
    {
        return FD_ISSET(sock.port(), _set->get() );
    }
    
#ifndef PLAT_WIN
    int FD_Set::getMax() const
    {
        return *_sock.rbegin();
    }
    
#endif
    
    size_t FD_Set::size() const
    {
#ifndef PLAT_WIN
        return _sock.size();
#else
        return _set->get()->fd_count;
#endif
    }
    
    
    fd_set FD_Set::get() const
    {
        return *_set->get();
    }
    
    std::vector<FD_Set> select(const FD_Set& set, timeval timeout)
    {
        constexpr char NUM_DATA = 3;
        std::vector<fd_set> data;
        data.resize(NUM_DATA, set.get() );
        
#ifndef PLAT_WIN
        //                                        read      write     except
        char success = ::select(set.getMax() + 1, &data[0], &data[1], &data[2], &timeout);
#else
        char success = ::select(NULL, &data[0], &data[1], &data[2], &timeout);
#endif
        
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to select ports");
        }
        
        std::vector<FD_Set> fds_data;
        fds_data.resize(NUM_DATA);
        
        for(char i = 0; i < NUM_DATA; i++)
        {
            fds_data[i].set(data[i]);
        }
        
        return fds_data;
    }
    
    std::vector<FD_Set> select(const FD_Set& set, TimeVal timeout)
    {
        return select(set, timeout.get() );
    }
    
    
    
    TimeVal::TimeVal(unsigned int milli)
    {
        //find seconds
        _tv_sec = floor(milli / 1000);
        _tv_usec = (milli % 1000) * 1000;
    }
    
    timeval TimeVal::get() const
    {
        return {_tv_sec, _tv_usec};
    }
    
    TimeVal::operator timeval() const
    {
        return get();
    }
    
    void setsockopt(const Socket& sock, int level, int optname, const void *optval, socklen_t optlen)
    {
#ifndef PLAT_WIN
        char success = ::setsockopt(sock.port(), level, optname, optval, optlen);
#else
        char success = ::setsockopt(sock.port(), level, optname, (const char*)optval, optlen);
#endif
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
