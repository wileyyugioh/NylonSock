//
//  Socket.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/1/16.
//  Copyright (c) 2016 Wiley Yu. All rights reserved.
//

#include "Socket.h"

#include "Definitions.h"

#ifdef PLAT_WIN
constexpr int SHUT_RD = SD_RECEIVE;
constexpr int SHUT_WR = SD_SEND;
constexpr int SHUT_RDWR = SD_BOTH;

constexpr int NSCONNRESET = WSAECONNRESET;
constexpr int NSWOULDBLOCK = WSAEWOULDBLOCK;

#elif defined(UNIX_HEADER)

#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

//make it easier for crossplatform
constexpr char INVALID_SOCKET = -1;
constexpr char SOCKET_ERROR = -1;

typedef int SOCKET;
constexpr int NSCONNRESET = ECONNRESET;
constexpr int NSWOULDBLOCK = EWOULDBLOCK;

#endif

#include <algorithm>
#include <cstring>
#include <mutex>

namespace NylonSock
{
    int NSHelper::_cw = 0;

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

#elif defined(UNIX_HEADER)
        signal(SIGPIPE, SIG_IGN);
#endif
    }
    
    void NSRelease()
    {
#ifdef PLAT_WIN
        WSACleanup();
#endif
    }

    NSHelper::NSHelper()
    {
        //is this thread safe? Who the hell knows?
        if(_cw++ == 0)
        {
            NSInit();
        }
    }

    NSHelper::NSHelper(const NSHelper& that)
    {
        _cw++;
    }

    NSHelper::~NSHelper()
    {
        if(--_cw == 0)
        {
            NSRelease();
        }
    }
#ifdef PLAT_WIN
    class WinStringWrap
    {
    private:
        LPSTR errString;
    public:
        WinStringWrap() = default;

        ~WinStringWrap()
        {
            LocalFree(errString);
        };

        LPSTR& get()
        {
            return errString;
        };
    };

    std::string translateError()
    {
        int errCode = WSAGetLastError();
        WinStringWrap wrap;

        int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM, // use windows internal message table
            0,       // 0 since source is internal message table
            errCode, // this is the error code returned by WSAGetLastError()
                     // Could just as well have been an error code from generic
                     // Windows errors from GetLastError()
            0,       // auto-determine language to use
            (LPSTR)&wrap.get(), // this is WHERE we want FormatMessage
                               // to plunk the error string.  Note the
                               // peculiar pass format:  Even though
                               // errString is already a pointer, we
                               // pass &errString (which is really type LPSTR* now)
                               // and then CAST IT to (LPSTR).  This is a really weird
                               // trip up.. but its how they do it on msdn:
                               // http://msdn.microsoft.com/en-us/library/ms679351(VS.85).aspx
            0,                 // min size for buffer
            0);

        return wrap.get();
    }

    Error::Error(const std::string& what) : std::runtime_error(what + ": " + translateError() ) {}
#elif defined(UNIX_HEADER)
    Error::Error(const std::string& what) : std::runtime_error(what + ": " + strerror(errno) ) {}
#endif
    
    Error::Error(const std::string& what, bool null) : std::runtime_error(what) {}
    
    //needs to be in shared ptr
    class Socket::SocketWrapper
    {
    private:
        SOCKET _sock;

        void closeSocket()
        {
            if (_sock != INVALID_SOCKET)
            {
                shutdown(_sock, SHUT_RDWR);
#ifdef PLAT_WIN
                closesocket(_sock);
#elif defined(UNIX_HEADER)
                close(_sock);
#endif
            }
        }

    public:
        SocketWrapper(addrinfo& res, bool autoconnect)
        {
            //loop through all of the ai until one works
            addrinfo* ptr;
            for(ptr = &res; ptr != nullptr; ptr = ptr->ai_next)
            {
                _sock = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if(_sock == INVALID_SOCKET)
                {
                    continue;
                }

                if(autoconnect && ::connect(_sock, ptr->ai_addr, ptr->ai_addrlen) == SOCKET_ERROR)
                {
                    closeSocket();
                    continue;
                }
				
				break;
            }
            
            if(ptr == nullptr)
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
            closeSocket();
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
    
    Socket::Socket(const char* node, const char* service, const addrinfo* hints, bool autoconnect)
    {
        _info = std::make_unique<AddrWrapper>(node, service, hints);
        
        //socket creation
        _sw = std::make_unique<SocketWrapper>(*_info->get(), autoconnect);
    }
    
    Socket::Socket(const std::string& node, const std::string& service, const addrinfo* hints, bool autoconnect) : 
        Socket(node.c_str(), service.c_str(), hints, autoconnect)
    {
        
    }
    
    Socket::Socket(SOCKET port)
    {
        //used for a socket that got recv.
        //no addr_info because screw you
        
        //sets socket
        _sw = std::make_unique<SocketWrapper>(port);
        
    }
    
    Socket::Socket(SOCKET port, const sockaddr_storage* data)
    {
        //this is for storing data!
        //yay!
        //gotta allocate that memory yo
        _info = std::make_unique<AddrWrapper>();
        
        //sets socket
        _sw = std::make_unique<SocketWrapper>(port);
    }

    Socket::Socket(Socket&& that) : _info(move(that._info)), _sw(move(that._sw)) {}

    Socket::Socket() = default;

    Socket::~Socket() = default;

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
        constexpr int y = 1;
        
        //binds socket to port
        char success = ::bind(sock.port(), sock->ai_addr, sock->ai_addrlen);
        if(success == SOCKET_ERROR)
        {
            //try clearing the port
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y) );
            
            //and rebind
            ::bind(sock.port(), sock->ai_addr, sock->ai_addrlen);
        }
        
        //clears addrinfo
        sock.freeaddrinfo();
    }
    
    void connect(const Socket& sock)
    {
        //connects socket to host
        char success = ::connect(sock.port(), sock->ai_addr, sock->ai_addrlen);

        if (success == SOCKET_ERROR)
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
#ifdef PLAT_WIN
            //needs to be cast to const char* for winsock2
            size += ::send(sock.port(), (const char*)buf, len, flags);
#elif defined(UNIX_HEADER)
            size += ::send(sock.port(), buf, len, flags);
#endif

            if(size == CLOSED)
            {
                throw Error("Failed to send data to socket");
            }
        }
        
        return size;
    }
    
    size_t recv(const Socket& sock, void* buf, size_t len, int flags)
    {
        int NSerrno = 0;
#ifdef PLAT_WIN
        //casting to char* for winsock
        auto size = ::recv(sock.port(), (char*)buf, len, flags);
        if(size == SOCKET_ERROR) NSerrno = WSAGetLastError();
#elif defined(UNIX_HEADER)
        auto size = ::recv(sock.port(), buf, len, flags);
        if(size == SOCKET_ERROR) NSerrno = errno;
#endif
        if(size == SOCKET_ERROR && NSerrno == NSCONNRESET)
        {
            throw PEER_RESET("Connection reset by peer");
        }
        
        if(size == SOCKET_ERROR && NSerrno != NSWOULDBLOCK)
        {
            throw Error(std::string{"Failed to receive data from socket."});
        }
        
        if(size == SOCKET_ERROR && NSerrno == NSWOULDBLOCK)
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
#ifdef PLAT_WIN
        auto size = ::sendto(sock.port(), (const char*)buf, len, flags, dest->ai_addr, dest->ai_addrlen);
#elif defined(UNIX_HEADER)
        auto size = ::sendto(sock.port(), buf, len, flags, dest->ai_addr, dest->ai_addrlen);
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
#ifdef PLAT_WIN
        int rm_const_cast = static_cast<int>(rm_const);
        auto size = ::recvfrom(sock.port(), (char*)buf, len, flags, dest->ai_addr, &rm_const_cast);
#elif defined(UNIX_HEADER)
        auto size = ::recvfrom(sock.port(), buf, len, flags, dest->ai_addr, &rm_const);
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
#ifdef UNIX_HEADER
        constexpr int COMMAND_TYPE = F_SETFL;
#endif
        if(args != O_NONBLOCK && args != O_ASYNC)
        {
            throw Error("Arg to fcntl is not O_NONBLOCK or O_ASYNC");
        }
        
#ifdef PLAT_WIN
        u_long is_true = 1;
        char success = ioctlsocket(sock.port(), args, &is_true);
#elif defined(UNIX_HEADER)
        char success = ::fcntl(sock.port(), COMMAND_TYPE, args);
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
#ifdef UNIX_HEADER
        _sock = that._sock;
#endif
    }
    
    void FD_Set::set(const Socket& sock)
    {
        FD_SET(sock.port(), _set->get() );
#ifdef UNIX_HEADER
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
#ifdef UNIX_HEADER
        _sock.erase(sock.port() );
#endif
    }
    
    void FD_Set::zero()
    {
        FD_ZERO(_set->get() );
#ifdef UNIX_HEADER
        _sock.clear();
#endif
    }
    
    bool FD_Set::isset(const Socket& sock) const
    {
        return FD_ISSET(sock.port(), _set->get() );
    }
    
#ifdef UNIX_HEADER
    int FD_Set::getMax() const
    {
        return *_sock.rbegin();
    }
    
#endif
    
    size_t FD_Set::size() const
    {
#ifdef PLAT_WIN
        return _set->get()->fd_count;
#elif defined(UNIX_HEADER)
        return _sock.size();
#endif
    }
    
    
    fd_set FD_Set::get() const
    {
        return *_set->get();
    }
    
    std::vector<FD_Set> select(const FD_Set& set, timeval timeout)
    {
        if(!set.size()) return {};

        constexpr char NUM_DATA = 3;
        std::vector<fd_set> data;
        data.resize(NUM_DATA, set.get() );
        
#ifdef PLAT_WIN
        char success = ::select(NULL, &data[0], &data[1], &data[2], &timeout);
#elif defined(UNIX_HEADER)
        //                                        read      write     except
        char success = ::select(set.getMax() + 1, &data[0], &data[1], &data[2], &timeout);
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
#ifdef PLAT_WIN
        char success = ::setsockopt(sock.port(), level, optname, (const char*)optval, optlen);
#elif defined(UNIX_HEADER)
        char success = ::setsockopt(sock.port(), level, optname, optval, optlen);
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

    short PollFDs::map_event(const Events& event)
    {
        switch(event)
        {
            case Events::NSPOLLIN: return POLLIN;
            case Events::NSPOLLOUT: return POLLOUT;
            case Events::NSPOLLPRI: return POLLPRI;
            case Events::NSPOLLERR: return POLLERR;
            case Events::NSPOLLHUP: return POLLHUP;
            case Events::NSPOLLINVAL: return POLLNVAL;
            default: throw Error("Invalid PollFDs Event");
        }
    }

    pollfd& PollFDs::get_element(Socket* sock)
    {
        auto port = sock->port();
        auto pfit = std::find_if(_pfs.begin(), _pfs.end(), [&port](const pollfd& obj)
        {
            return obj.fd == port;
        });
        if(pfit == _pfs.end())
        {
            _pfs.push_back({port, 0, 0});
            return _pfs.back();
        }

        return *pfit;
    }

    void PollFDs::add_event(Socket* sock, const PollFDs::Events& event)
    {
        auto& element = get_element(sock);
        element.events = element.events | map_event(event);
    }

    void PollFDs::clear() {_pfs.clear();}

    bool PollFDs::get_event(Socket* sock, const PollFDs::Events& event)
    {
        return (get_element(sock).events & map_event(event) );
    }

    int poll(PollFDs& pollfds, unsigned int timeout)
    {
#ifdef PLAT_WIN
        char success = ::WSAPoll(pollfds.get(), pollfds.size(), timeout);
#elif defined(UNIX_HEADER)
        char success = ::poll(pollfds.get(), pollfds.size(), timeout);
#endif
        if(success == SOCKET_ERROR)
        {
            throw Error("Failed to poll ports");
        }

        return success;
    }
}
