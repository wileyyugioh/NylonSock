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

#include <string>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <cstring>

/*
 How data is sent:
 
 bytes are assumed to be 8 bits
 
 Beginning is uint16_t of size in bytes
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
    using SockFunc = std::function< void (SockData, Self&)>;
    
    //used for getting num of digits in a number
    template <unsigned long long N, size_t base=10>
    struct numberlength
    {
        enum { value = 1 + numberlength<N/base, base>::value };            
    };

    template <size_t base>
    struct numberlength<0, base>
    {
        enum { value = 0 };
    };
    
    class TOOBIG : public std::runtime_error
    {
    public:
        TOOBIG(std::string what);
    };
    
    class SockData
    {
    private:
        std::string raw_data;
        
    public:
        SockData(std::string data);
        std::string getRaw() const;
    };

    void emitSend(std::string event_name, const SockData& data, Socket& socket)
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
        send(socket, format_str.c_str(), format_str.size(), NULL);
    }
    
    //have to use CRTP
    template<class T>
    class ClientInterface
    {
    public:
        virtual ~ClientInterface() = default;
        virtual void on(std::string event_name, SockFunc<T> func) = 0;
        virtual void emit(std::string event_name, const SockData& data) = 0;
        virtual void update() = 0;
        virtual bool getDestroy() const = 0;
        
    };
    
    template<class T>
    class ClientSocket : public ClientInterface<T>
    {
    private:
    	class CLOSE : public std::runtime_error
    	{
    	public:
    		CLOSE(std::string err) : std::runtime_error(err) {};
    	};

    protected:
        std::unique_ptr<Socket> _client;
        std::unordered_map<std::string, SockFunc<T> > _functions;
        std::unique_ptr<FD_Set> _self_fd;
        T* top_class;
        
        bool _destroy_flag = false;

        void recvData(Socket& sock, std::unordered_map<std::string, SockFunc<T> >& _functions)
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

            auto castback = [](std::string str)
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
            char success = recv(sock, datalen.data(), data_size, NULL);
            
            //break when there is no info or an error
            if(success <= 0) return;
            
            //receive event length
            recv(sock, eventlen.data(), data_size, NULL);
            
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
                recv(sock, &(event[0]), eventlensize, NULL);
                eventstr.assign(event.begin(), event.end() );
            }

            //receive and convert data
            sock_size_type datalensize = castback({datalen.begin(), datalen.end()});

            if(datalensize > 0)
            {
                data.resize(datalensize, 0);
                recv(sock, &(data[0]), datalensize, NULL);
                datastr.assign(data.begin(), data.end() );
            }
            
            //if the event is in the functions
            if(_functions.count(eventstr) != 0)
            {
                //call it
                _functions[eventstr](SockData{datastr}, *top_class);
            }
            
            //else, the data gets thrown away
        }

    public:
        ClientSocket(Socket sock) : top_class(static_cast<T*>(this) )
        {
            _client = std::make_unique<Socket>(sock);
        };

        virtual ~ClientSocket() = default;

        void on(std::string event_name, SockFunc<T> func) override
        {
            _functions[event_name] = func;
        };

        void emit(std::string event_name, const SockData& data) override
        {
            //sends data to client
            emitSend(event_name, data, *_client);
        };

        bool getDestroy() const override
        {
            return _destroy_flag;
        };

        void update() override
        {
            try
            {
                //lazy initialization
                if(_self_fd == nullptr)
                {
                    _self_fd = std::make_unique<FD_Set>();
                    _self_fd->set(*_client);
                }
                
                while(true)
                {
                    //see if we can recv
                    if(select(*_self_fd, TimeVal{1000})[0].size() <= 0)
                    {
                        return;
                    }
                    
                    recvData(*_client, _functions);
                }
            }
            catch (SOCK_CLOSED& e)
            {
                _client = nullptr;
                _functions.clear();
                _self_fd = nullptr;
                
                _destroy_flag = true;

                throw e;
            }
        };

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
        std::unique_ptr<FD_Set> _fdset;
        std::unique_ptr<Socket> _server;
        std::vector<std::unique_ptr<UsrSock> > _clients;
        ServClientFunc _func;
        std::mutex _clsz_rw;
        std::mutex _fin;

        void createServer(std::string port)
        {
            addrinfo hints = {0};
            //force server to be ipv6
            //ipv4 and ipv6 addresses can connect
            hints.ai_family = AF_INET6;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;
            
            _server = std::make_unique<Socket>(nullptr, port.c_str(), &hints);
            fcntl(*_server, O_NONBLOCK);
            
            bind(*_server);
            
            constexpr int backlog = 100;
            
            listen(*_server, backlog);
        };
        
        void addToSet()
        {
            //lazy
            if(_fdset == nullptr)
            {
                _fdset = std::make_unique<FD_Set>();
            }
            
            _fdset->set(*_server);
        };

        void update()
		{
            while(true)
            {
                //this is all accepting new clients
            	//sets[0] is an FD_Set of the sockets ready to be accepted
            	auto sets = select(*_fdset, TimeVal{ 1000 });
                    
                if (sets[0].size() == 0) break;

                auto new_sock = accept(*_server);
                    
                _clsz_rw.lock();
                //it is an actual socket
                _clients.push_back(std::make_unique<UsrSock>(new_sock) );

                _clsz_rw.unlock();
                    
                //call the onConnect func
                 _func(*_clients.back() );

                 //put update thread
                 //also takes care of killing and removing client
                 auto client_thr_update = [](UsrSock* sock, std::mutex& clsz, decltype(_clients)& clients, std::atomic<bool>& stop_thr)
                 {
                 	while(!sock->getDestroy() && !stop_thr.load() )
                 	{
                 		try
                 		{
                 			sock->update();
                 		}
                 		catch(SOCK_CLOSED& e)
                 		{
                 			//for now do nothing
                 		}
                 	}

                 	clsz.lock();
                 	clients.erase(std::remove_if(clients.begin(), clients.end(), [&sock](std::unique_ptr<UsrSock>& usrsock)
                 		{
                 			return (usrsock.get() == sock);
                 		}), clients.end() );
                 	clsz.unlock();
                 };

                 std::thread update_thread{client_thr_update, _clients.back().get(), std::ref(_clsz_rw), std::ref(_clients), std::ref(_stop_thread)};
                 update_thread.detach();
            }
	    };

        void thr_update()
        {
            std::lock_guard<std::mutex> end{_fin};
            while(true)
            {
                if(_stop_thread.load() )
                {
                    break;
                }
                update();
            }
        }
    public:
        Server(std::string port)
        {
            createServer(port);
            addToSet();
        };
        
        Server(int port) : Server(std::to_string(port) )
        {
        };

        ~Server()
        {
            stop();
            std::lock_guard<std::mutex> end(_fin);
        }
        
        Server(const Server& that) = delete;

        Server& operator=(const Server& that) = delete;

        Server(Server&& that) = delete;

        Server& operator=(Server&& that) = delete;
       
        void onConnect(ServClientFunc func)
        {
            _func = func;
        };
        
        void emit(std::string event_name, SockData data)
        {
            std::lock_guard<std::mutex> lock {_clsz_rw};
            for(auto& it : _clients)
            {
                it->emit(event_name, data);
            }
        };

        unsigned long count() 
        {
            std::lock_guard<std::mutex> lock {_clsz_rw};
            return _clients.size();
        };

        void start()
        {
            //Prevents making too many threads
            if(!_stop_thread.load() )
            {
                return;
            }

            _stop_thread = false;

            auto accepter = std::thread(&Server::thr_update, this);
            accepter.detach();
        };

        void stop()
        {
            _stop_thread = true;
        };

        bool status() const
        {
            return !_stop_thread.load();
        };

        UsrSock& getUsrSock(unsigned int pos)
        {
            std::lock_guard<std::mutex> lock{_clsz_rw};
            return *_clients.at(pos);
        };

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
        std::mutex _fin;
        
        void createListener(std::string ip, std::string port)
        {
            addrinfo hints = {0};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            
            _server = std::make_unique<Socket>(ip, port, &hints);
            connect(*_server);
        };

        void update() override
        {
            std::lock_guard<std::mutex> end{_fin};
            try
            {
                while(true)
                {
                    if(_stop_thread.load() || _inter->getDestroy() ) break;
                    
                    _inter->update();
                }
            }
            catch(std::exception& e)
            {
                stop();
            }
        };

    public:
        Client(std::string ip, std::string port)
        {
             createListener(ip, port);
            _inter = std::make_unique<T>(*_server);
        };

        Client(std::string ip, int port) : Client(ip, std::to_string(port) ) {};

        ~Client()
        {
            stop();
            std::lock_guard<std::mutex> end{_fin};
        };

        Client(const Client& that) = delete;

        Client& operator=(const Client& that) = delete;

        Client(Client&& that) = delete;

        Client& operator=(Client&& that) = delete;

        void on(std::string event_name, SockFunc<T> func) override
        {
        	if(!_stop_thread.load() ) _inter->on(event_name, func);
        };

        void emit(std::string event_name, const SockData& data) override
        {
            if(!_stop_thread.load() ) _inter->emit(event_name, data);
        };

        bool getDestroy() const override
        {
            return _inter->getDestroy();
        };

        void start()
        {
            //Prevents making too many threads
            if(!_stop_thread.load() )
            {
                return;
            }

            _stop_thread = false;

            auto accepter = std::thread(&Client<T>::update, this);
            accepter.detach();
        };

        void stop()
        {
            _stop_thread = true;
        };

        bool status() const
        {
            return !_stop_thread.load();
        };

        T& get()
        {
            return *_inter;
        }

    };
}

#endif /* defined(__NylonSock__Sustainable__) */
