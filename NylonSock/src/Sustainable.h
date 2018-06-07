//
//  Sustainable.h
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/19/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#ifndef __NylonSock__Sustainable__
#define __NylonSock__Sustainable__

#include "Socket.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>


/*
 How data is sent:
 
 bytes are assumed to be 8 bits
 
 Beginning is uint16_t of content size in bytes
 Next is uint16_t of event name size in bytes
 Next is name
 Rest is data.
 
 */


namespace NylonSock
{
    typedef uint16_t sock_size_type;
    constexpr sock_size_type maximum_sock_val = std::numeric_limits<sock_size_type>::max();

    class FD_Set;
    class Socket;

    class SockData;

    template <class T>
    class ClientSocket;
    
    template<class Self>
    using SockFunc = std::function<void(SockData, Self&)>;

    using NoFunc = std::function<void()>;
    
    class TOO_BIG : public NylonSock::Error
    {
    public:
        TOO_BIG(const std::string& what): Error(what) {}
    };

    class FAILED_CONVERT : public NylonSock::Error
    {
    public:
        FAILED_CONVERT(const std::string& what) : Error(what) {}
    };
    
    class SockData
    {
    private:
        std::string raw_data;

        void initializeByString(const std::string& data)
        {
            raw_data = data;

            if(data.size() > maximum_sock_val)
            {
                //throw error because data is too large
                throw TOO_BIG("The data size of " + std::to_string(data.size()) + " is too big.");
            }
        }
        
    public:
        SockData(const std::string& data)
        {
            initializeByString(data);
        }

        template<typename T>
        SockData(const T& t)
        {
            std::ostringstream oss;
            std::string result;

            oss << t;
            result = oss.str();

            initializeByString(result);
        }

        std::string getRaw() const
        {
            return raw_data;
        }

        template<typename T>
        operator T()
        {
            std::istringstream ss(getRaw() );
            T result;

            if(!(ss >> result) ) throw FAILED_CONVERT("Failed to convert SockData into a primitive type.");

            return result;
        }

        operator std::string()
        {
            return getRaw();
        }
    };

    void emitSend(const std::string& event_name, const SockData& data, Socket& socket)
    {
        //sends data to server/client

        //assumes socket is already binded
        std::string raw_data = data.getRaw();
       
        //size of data
        sock_size_type sizeofstr = htons(raw_data.size() );
        
        //size of event name
        sock_size_type sizeofevent = htons(event_name.size() );
        
        //formatting data: size of data + size of eventname + eventname + data
        auto str_cast = (char*)(&sizeofstr);
        auto event_cast = (char*)(&sizeofevent);

        std::string format_str = std::string{str_cast, str_cast + sizeof(sizeofstr)} + 
            std::string{event_cast, event_cast + 
            sizeof(sizeofevent)} + 
            event_name + 
            raw_data;

        //sending total data
        send(socket, format_str.c_str(), format_str.size(), 0);
    }
    
    //have to use CRTP
    template<class T>
    class ClientInterface
    {
    private:
        T& impl() {return *static_cast<T*>(this);}
    public:
        virtual ~ClientInterface() = default;
        void on(const std::string& event_name, SockFunc<T> func) {impl().on(event_name, func);}
        void on(const std::string& event_name, NoFunc func) {impl().on(event_name, func);}
        void emit(const std::string& event_name, const SockData& data) {impl().emit(event_name, data);}
        bool getDestroy() const {impl().getDestroy();}
        
    };

    template<class T>
    class ClientSocket : public ClientInterface<T>
    {
    private:
        class CLOSE : public std::runtime_error
        {
        public:
            CLOSE(const std::string& err) : std::runtime_error(err) {}
        };

    protected:
        std::unique_ptr<Socket> _client;
        std::unordered_map<std::string, SockFunc<T> > _functions;
        std::unordered_map<std::string, NoFunc> _nofunctions;
        std::unique_ptr<FD_Set> _self_fd;
        T* top_class;
        
        bool _destroy_flag;

        char recvData(Socket& sock)
        {
            auto string_to_hex = [](const std::string& input)
            {
                static const char* const lut = "0123456789ABCDEF";
                size_t len = input.length();

                std::string output;
                output.reserve(2 * len);
                for (size_t i = 0; i < len; ++i)
                {
                    const unsigned char c = input[i];
                    output.push_back(lut[c >> 4]);
                    output.push_back(lut[c & 15]);
                }
                return output;
            };

            auto castback = [](const std::string& str)
            {
                sock_size_type data;

                std::memcpy(&data, str.c_str(), str.size() );

                return ntohs(data);
            };

            //gets the size of data in package that tells data size
            size_t data_size = sizeof(sock_size_type);

            std::vector<char> datalen(data_size), eventlen(data_size);
            
            //receives data from client
            //also assumes socket is non blocking
            char success = recv(sock, datalen.data(), data_size, 0);
            
            //break when there is no info or an error
            if(success <= 0) return success;
            
            //receive event length
            recv(sock, eventlen.data(), data_size, 0);
            
            //allocate buffers!
            //char is not guaranteed 8 bit
            std::vector<uint8_t> event, data;
            std::string eventstr, datastr;
            
            //receive and convert event
            sock_size_type eventlensize = castback({eventlen.begin(), eventlen.end()});

            //only recv if eventname greater than 0
            if(eventlensize > 0)
            {
                event.resize(eventlensize, 0);
                recv(sock, &(event[0]), eventlensize, 0);
                eventstr.assign(event.begin(), event.end() );
            }

            //receive and convert data
            sock_size_type datalensize = castback({datalen.begin(), datalen.end()});

            if(datalensize > 0)
            {
                data.resize(datalensize, 0);
                recv(sock, &(data[0]), datalensize, 0);
                datastr.assign(data.begin(), data.end() );
            }

            eventCall(eventstr, SockData{datastr}, *top_class);

            return NylonSock::SUCCESS;
        }

        void eventCall(const std::string& eventstr, SockData data, T& tclass)
        {
            //if the event is in the functions
            auto efind = _functions.find(eventstr);
            if(efind != _functions.end() )
            {
                //call it
                (efind->second)(data, tclass);
            }
            
            //else, the data gets thrown away
        }

        void eventCall(const std::string& eventstr)
        {
            //if the event is in the functions
            auto efind = _nofunctions.find(eventstr);
            if(efind != _nofunctions.end() )
            {
                //call it
                (efind->second)();
            }
        }

    public:
        ClientSocket(Socket sock) : top_class(static_cast<T*>(this) ), _destroy_flag(false)
        {
            _client = std::make_unique<Socket>(sock);
        }

        void on(const std::string& event_name, SockFunc<T> func)
        {
            _functions[event_name] = func;
        }

        void on(const std::string& event_name, NoFunc func)
        {
            _nofunctions[event_name] = func;
        }

        void emit(const std::string& event_name, const SockData& data)
        {
            //sends data to client
            emitSend(event_name, data, *_client);
        }

        bool getDestroy() const
        {
            return _destroy_flag;
        }

        void update(bool nonblock)
        {
            try
            {
                //lazy initialization
                if(_self_fd == nullptr)
                {
                    _self_fd = std::make_unique<FD_Set>();
                    _self_fd->set(*_client);
                }

                //see if we can recv
                if(nonblock && select(*_self_fd, TimeVal{1000})[0].size() <= 0)
                {
                    return;
                }
                char success = recvData(*_client);
                if(success == NylonSock::SUCCESS) return;
            }
            catch (NylonSock::Error& e) {}

            eventCall("disconnect");

            _client = nullptr;
            _functions.clear();
            _self_fd = nullptr;
            
            _destroy_flag = true;
        }

    };
    
    //dummy
    template<class UsrSock, class Dummy = void>
    class Server;
    
    template <class UsrSock>
    class Server<UsrSock, typename std::enable_if<std::is_base_of<ClientSocket<UsrSock>, UsrSock>::value>::type>
    {
    private:
        using ServClientFunc = std::function<void (UsrSock&)>;
        std::atomic<bool> _stop_thread{true};
        std::unique_ptr<std::thread> _thread;
        std::unique_ptr<FD_Set> _fdset;
        std::unique_ptr<Socket> _server;
        std::vector<std::unique_ptr<UsrSock> > _clients;
        ServClientFunc _func;
        std::mutex _clsz_rw;

        void createServer(const std::string& port)
        {
            addrinfo hints = {0};
            //force server to be ipv6
            //ipv4 and ipv6 addresses can connect
            hints.ai_family = AF_INET6;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = AI_PASSIVE;
            
            _server = std::make_unique<Socket>(nullptr, port.c_str(), &hints);
			
#ifdef PLAT_WIN
			//needed because windows ipv6 doesn't accept ipv4
			const int n = 0;
			setsockopt(*_server, IPPROTO_IPV6, IPV6_V6ONLY, &n, sizeof(n) );
#endif

            fcntl(*_server, O_NONBLOCK);
            
            bind(*_server);
            
            constexpr int backlog = 100;
            
            listen(*_server, backlog);
        }

        void update()
        {
            //this is all accepting new clients
            //sets[0] is an FD_Set of the sockets ready to be accepted
            auto sets = select(*_fdset, TimeVal{100});
                
            if (sets[0].size() > 0)
            {
                auto new_sock = accept(*_server);
                    
                _clsz_rw.lock();
                //it is an actual socket
                _clients.push_back(std::make_unique<UsrSock>(new_sock) );

                _clsz_rw.unlock();
                    
                //call the onConnect func
                 _func(*_clients.back() );

             }

             //update all clients
             auto it = _clients.begin();
             while(it != _clients.end() )
             {
                auto sock = (*it).get();
                if(!sock->getDestroy())
                {
                    sock->update(true);
                    ++it;
                    continue;
                }
                //kill the client
                std::lock_guard<std::mutex> lock{_clsz_rw};
                it = _clients.erase(it);
             }
        }

        void thr_update()
        {
            while(true)
            {
                if(_stop_thread.load() ) break;
                update();
            }
        }

    public:
        Server(const std::string& port)
        {
            createServer(port);

            // create fdset
            _fdset = std::make_unique<FD_Set>();
            _fdset->set(*_server);
        }
        
        Server(int port) : Server(std::to_string(port) ) {}

        ~Server()
        {
            stop();
            _thread->join();
        }
        
        Server(const Server& that) = delete;

        Server& operator=(const Server& that) = delete;

        Server(Server&& that) = delete;

        Server& operator=(Server&& that) = delete;
       
        void onConnect(ServClientFunc func)
        {
            _func = func;
        }
        
        void emit(const std::string& event_name, SockData data)
        {
            std::lock_guard<std::mutex> lock {_clsz_rw};
            for(auto& it : _clients)
            {
                it->emit(event_name, data);
            }
        }

        unsigned long count() 
        {
            std::lock_guard<std::mutex> lock {_clsz_rw};
            return _clients.size();
        }

        void start()
        {
            //Prevents making too many threads
            if(!_stop_thread.load() ) return;

            _stop_thread = false;

            _thread = std::make_unique<std::thread>(&Server::thr_update, this);
        }

        void stop()
        {
            _stop_thread = true;
        }

        bool status() const
        {
            return !_stop_thread.load();
        }

        UsrSock& getUsrSock(unsigned int pos)
        {
            std::lock_guard<std::mutex> lock{_clsz_rw};
            return *_clients.at(pos);
        }

    };
    
    template <class T, class Dummy = void>
    class Client;

    //I really don't want to do any more work
    template <class T>
    class Client<T, typename std::enable_if<std::is_base_of<ClientSocket<T>, T>::value>::type> : public ClientInterface<T>
    {
    private:
        //see top of cpp file to see how data is sent
        std::unique_ptr<Socket> _server;
        
        //client socket has similar interface
        std::unique_ptr<T> _inter;

        std::atomic<bool> _stop_thread{true};
        std::unique_ptr<std::thread> _thread;
        
        void createListener(const std::string& ip, const std::string& port)
        {
            addrinfo hints = {0};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            
            _server = std::make_unique<Socket>(ip, port, &hints, true);
        }

        void update()
        {
            class RAIIMe
            {
            private:
                Client* _c;
            public:
                RAIIMe(Client* c) : _c(c) {};
                ~RAIIMe() {_c->stop();};
            };

            RAIIMe rm{this};
            while(true)
            {
                if(_stop_thread.load() || _inter->getDestroy() ) break;
                
                _inter->update(false);
            }
        }

    public:
        Client(const std::string& ip, const std::string& port)
        {
             createListener(ip, port);
            _inter = std::make_unique<T>(*_server);
        }

        Client(const std::string& ip, int port) : Client(ip, std::to_string(port) ) {}

        ~Client()
        {
            stop();
            _thread->join();
        }

        Client(const Client& that) = delete;

        Client& operator=(const Client& that) = delete;

        Client(Client&& that) = delete;

        Client& operator=(Client&& that) = delete;

        void on(const std::string& event_name, SockFunc<T> func)
        {
            if(!_stop_thread.load() ) _inter->on(event_name, func);
        }

        void on(const std::string& event_name, NoFunc func)
        {
            if(!_stop_thread.load() ) _inter->on(event_name, func);
        }

        void emit(const std::string& event_name, const SockData& data)
        {
            if(!_stop_thread.load() ) _inter->emit(event_name, data);
        }

        bool getDestroy() const
        {
            return _inter->getDestroy();
        }

        void start()
        {
            //Prevents making too many threads
            if(!_stop_thread.load() )
            {
                return;
            }

            _stop_thread = false;

            _thread = std::make_unique<std::thread>(&Client<T>::update, this);
        }

        void stop()
        {
            _stop_thread = true;
        }

        bool status() const
        {
            return !_stop_thread.load();
        }

        T& get()
        {
            return *_inter;
        }

    };
}

#endif /* defined(__NylonSock__Sustainable__) */
