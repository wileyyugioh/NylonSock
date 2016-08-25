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
    using SockFunc = void (*)(SockData, Self&);
    
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
        
        //takes in an int, padding length, and a fill character
        //returns a padded string
        auto pad_str = [](int str, int padding, char fill)
        {
            std::stringstream ss;
            ss << std::setw(padding) << std::setfill(fill) << str;
            return ss.str();
        };

        //assumes socket is already binded
        std::string raw_data = data.getRaw();
       
        //size of data
        sock_size_type sizeofstr = raw_data.size();
        
        //size of event name
        sock_size_type sizeofevent = event_name.size();

        //get padding len
        numberlength<maximum_sock_val> nllen;
        auto padding_len = nllen.value; 
        
        //formatting data: size of data + size of eventname + eventname + data
        std::string format_str = pad_str(sizeofstr, padding_len, '0') + pad_str(sizeofevent, padding_len, '0') + event_name + raw_data;

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
            auto stosize_t = [](std::string str)
            {
                size_t data;
                sscanf(str.c_str(), "%zu", &data);

                return data;
            };

            //gets the size of data in package that tells data size
            numberlength<maximum_sock_val> nllen;
            size_t data_size = sizeof(char) * nllen.value;

            char datalen[data_size], eventlen[data_size];
            
            //receives data from client
            //also assumes socket is non blocking
            char success = recv(sock, &datalen, data_size, NULL);
            
            //break when there is no info
            if(success <= 0)
            {
                return;
            }
            
            //receive event length
            success = recv(sock, &eventlen, data_size, NULL);

            auto isdigit = [](std::string s)
            {
            	return std::find_if(s.begin(), s.end(), ::isdigit) != s.end();
            };

            //prevent bad data (everything before should only have numbers)
            if(!isdigit(eventlen) || !isdigit(datalen) )
            {
            	//ignore the data and close connection
            	throw CLOSE{"Bad Header (Letters instead of digits)"};
            }

            
            //allocate buffers!
            //char is not guaranteed 8 bit
            std::vector<uint8_t> event, data;
            
            size_t eventlensize = stosize_t(eventlen);
            size_t datalensize = stosize_t(datalen);

            event.resize(eventlensize, 0);
            data.resize(datalensize, 0);
            
            //receive event data
            success = recv(sock, &(event[0]), eventlensize, NULL);
            
            //receive data data
            success = recv(sock, &(data[0]), datalensize, NULL);
            
            std::string eventstr, datastr;
            
            //copy data into string
            eventstr.assign(event.begin(), event.end() );
            datastr.assign(data.begin(), data.end() );
            
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
            catch(std::runtime_error& e)
            {
                _client = nullptr;
                _functions.clear();
                _self_fd = nullptr;
                
                _destroy_flag = true;
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
        typedef void (*ServClientFunc)(UsrSock&);
        bool _stop_thread = false;
        std::unique_ptr<FD_Set> _fdset;
        std::unique_ptr<Socket> _server;
        std::vector<std::unique_ptr<UsrSock> > _clients;
        size_t _clients_size = 0;
        ServClientFunc _func;
        std::mutex _thr_rw;
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
            	//sets[0] is an FD_Set of the sockets ready to be accept
            	auto sets = select(*_fdset, TimeVal{ 1000 });
                    
                if (sets[0].size() == 0)
                {
                    break;
                }

                auto new_sock = accept(*_server);
                    
                _clsz_rw.lock();
                //it is an actual socket
                _clients.push_back(std::make_unique<UsrSock>(new_sock) );

                _clients_size = _clients.size();
                _clsz_rw.unlock();
                    
                //call the on connect func
                 _func(*_clients.back() );

                 //put update thread
                 //also takes care of killing and removing client
                 auto client_thr_update = [](UsrSock* sock, std::mutex& clsz, decltype(_clients)& clients)
                 {
                 	while(!sock->getDestroy() )
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
                 	clients.erase(std::remove_if(clients.begin(), clients.end(), [=](std::unique_ptr<UsrSock>& usrsock)
                 		{
                 			return (usrsock.get() == sock);
                 		}), clients.end() );
                 	clsz.unlock();
                 };

                 std::thread update_thread{client_thr_update, _clients.back().get(), std::ref(_clsz_rw), std::ref(_clients)};
                 update_thread.detach();
            }

            _clsz_rw.lock();
            _clients_size = _clients.size();
            _clsz_rw.unlock();
	    };

        void thr_update()
        {
            std::lock_guard<std::mutex> end{_fin};
            while(true)
            {
                _thr_rw.lock();
                if(_stop_thread)
                {
                    break;
                }
                _thr_rw.unlock();
                update();
            }
            _thr_rw.unlock();
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
        
        unsigned long count() 
        {
            _clsz_rw.lock();
            auto temp = _clients_size;
            _clsz_rw.unlock();

            return temp;
        };

        void start()
        {
            //Prevents making too many threads
            if(_stop_thread)
            {
                return;
            }

            _thr_rw.lock();
            _stop_thread = false;
            _thr_rw.unlock();
            auto accepter = std::thread(&Server::thr_update, this);
            accepter.detach();
        };

        void stop()
        {
            _thr_rw.lock();
            _stop_thread = true;
            _thr_rw.unlock();
        };

        bool status() const
        {
            return _stop_thread;
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

        std::mutex _thr_rw;
        bool _stop_thread = false;
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
                    _thr_rw.lock();
                    if(_stop_thread)
                    {
                        _thr_rw.unlock();
                        break;
                    }
                    _thr_rw.unlock();
                    _inter->update();
                }
            }
            catch(NylonSock::SOCK_CLOSED& e)
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
            _inter->on(event_name, func);
        };

        void emit(std::string event_name, const SockData& data) override
        {
            _inter->emit(event_name, data);
        };

        bool getDestroy() const override
        {
            return _inter->getDestroy();
        };

        void start()
        {
            //Prevents making too many threads
            if(_stop_thread)
            {
                return;
            }

            _thr_rw.lock();
            _stop_thread = false;
            _thr_rw.unlock();
            auto accepter = std::thread(&Client<T>::update, this);
            accepter.detach();
        };

        void stop()
        {
            _thr_rw.lock();
            _stop_thread = true;
            _thr_rw.unlock();
        };

        bool status() const
        {
            return _stop_thread;
        };

        T& get()
        {
            std::lock_guard<std::mutex> lock(_thr_rw);
            return *_inter;
        }

    };
}

#endif /* defined(__NylonSock__Sustainable__) */
