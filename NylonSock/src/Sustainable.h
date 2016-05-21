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
#include <stdexcept>
#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <cstdint>


/*
 How data is sent:
 
 bytes are assumed to be 8 bits
 
 Beginning is uint32_t of size in bytes
 Next is uint32_t of event name size in bytes
 Next is name
 Rest is data.
 
 */


namespace NylonSock
{
    typedef uint32_t sock_size_type;
    constexpr size_t sizeint32 = sizeof(sock_size_type);
    constexpr sock_size_type maximum32bit = std::numeric_limits<sock_size_type>::max();

    class FD_Set;
    class Socket;

    class SockData;

    template <class T>
    class ClientSocket;
    
    template<class Self>
    using SockFunc = void (*)(SockData, Self&);
    
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
        
        sock_size_type sizeofstr = htonl(raw_data.size() );
        sock_size_type sizeofevent = htonl(event_name.size() );
        
        //sending size of data
        send(socket, &sizeofstr, sizeint32, NULL);
        //sending size of event name
        send(socket, &sizeofevent, sizeint32, NULL);
        //sending event name
        send(socket, event_name.c_str(), event_name.size(), NULL);
        
        //sending total data
        send(socket, raw_data.c_str(), raw_data.size(), NULL);
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
    protected:
        std::unique_ptr<Socket> _client;
        std::unordered_map<std::string, SockFunc<T> > _functions;
        std::unique_ptr<FD_Set> _self_fd;
        T* top_class;
        
        bool _destroy_flag = false;

        void recvData(Socket& sock, std::unordered_map<std::string, SockFunc<T> >& _functions)
        {
            sock_size_type datalen, messagelen;
            
            //recieves data from client
            //also assumes socket is non blocking
            char success = recv(sock, &datalen, sizeint32, NULL);
            
            //break when there is no info
            if(success <= 0)
            {
                return;
            }
            
            //receive message length
            recv(sock, &messagelen, sizeint32, NULL);
            
            //convert to host language
            datalen = ntohl(datalen);
            messagelen = ntohl(messagelen);
            
            //allocate buffers!
            //char is not guaranteed 8 bit
            std::vector<uint8_t> message, data;
            
            message.resize(messagelen, 0);
            data.resize(datalen, 0);
            
            //receive message data
            recv(sock, &(message[0]), messagelen, NULL);
            
            //receive data data
            recv(sock, &(data[0]), datalen, NULL);
            
            std::string messagestr, datastr;
            
            //copy data into string
            messagestr.assign(message.begin(), message.end() );
            datastr.assign(data.begin(), data.end() );
            
            //if the event is in the functions
            if(_functions.count(messagestr) != 0)
            {
                //call it
                _functions[messagestr](SockData{datastr}, *top_class);
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
            catch(PEER_RESET& e)
            {
                _client = nullptr;
                _functions.clear();
                _self_fd = nullptr;
                
                _destroy_flag = true;
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
            hints.ai_family = AF_UNSPEC;
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
            }
	                
                //updating clients
            auto it = std::begin(_clients);
            while(it != _clients.end() )
            {
                if((*it)->getDestroy() == true)
                {
                    _clsz_rw.lock();
                    it = _clients.erase(it);
                    _clsz_rw.unlock();
                }
                else
                {
                    try
                    {
                        (*it)->update();
                        it++;
                    }
                    catch(SOCK_CLOSED& e)
                    {
                        //ignore it for now
                    }
                }
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

        Server(const Server&& that) = delete;

        Server& operator=(const Server&& that) = delete;
       
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

        Client(const Client&& that) = delete;

        Client& operator=(const Client&& that) = delete;

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
