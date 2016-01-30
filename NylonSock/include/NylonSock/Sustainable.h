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

namespace NylonSock
{
    class FD_Set;
    class Socket;
    class SockData;
    class ClientSocket;
    
    typedef void (*SockFunc)(SockData);
    
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
    
    class ClientInterface
    {
    public:
        virtual ~ClientInterface() = default;
        virtual void on(std::string event_name, SockFunc func) = 0;
        virtual void emit(std::string event_name, const SockData& data) = 0;
        virtual void update() = 0;
        virtual bool getDestroy() const = 0;
        
    };
    
    class ClientSocket : public ClientInterface
    {
    private:
        std::unique_ptr<Socket> _client;
        std::unordered_map<std::string, SockFunc> _functions;
        std::unique_ptr<FD_Set> _self_fd;
        
        bool _destroy_flag = false;
    public:
        ClientSocket(Socket sock);
        virtual ~ClientSocket() = default;
        void on(std::string event_name, SockFunc func) override;
        void emit(std::string event_name, const SockData& data) override;
        void update() override;
        bool getDestroy() const override;
    };
    
    //dummy
    template<class UsrSock, class Dummy = void>
    class Server;
    
    template <class UsrSock>
    class Server<UsrSock, typename std::enable_if<std::is_base_of<ClientSocket, UsrSock>::value>::type>
    {
    private:
        typedef void (*ServClientFunc)(UsrSock&);
        std::unique_ptr<FD_Set> _fdset;
        std::unique_ptr<Socket> _server;
        std::vector<std::unique_ptr<UsrSock> > _clients;
        ServClientFunc _func;
        
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
    public:
        Server(std::string port)
        {
            createServer(port);
            addToSet();
        };
        
        Server(int port) : Server(std::to_string(port) )
        {
        };
        
        ~Server() = default;
        
        void onConnect(ServClientFunc func)
        {
            _func = func;
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
                
                //it is an actual socket
                _clients.push_back(std::make_unique<UsrSock>(new_sock) );
                
                //call the on connect func
                _func(*_clients.back() );
            }
            
            //updating clients
            auto it = std::begin(_clients);
            while(it != _clients.end() )
            {
                if((*it)->getDestroy() == true)
                {
                    it = _clients.erase(it);
                }
                else
                {
                    (*it)->update();
                    it++;
                }
            }
		   };
        
        unsigned long count() const
        {
            return _clients.size();
        };
    };
    
    //I really don't want to do any more work
    class Client : public ClientInterface
    {
    private:
        //see top of cpp file to see how data is sent
        std::unique_ptr<Socket> _server;
        
        //client socket has similar interface
        std::unique_ptr<ClientSocket> _inter;
        
        void createListener(std::string ip, std::string port);
    public:
        Client(std::string ip, std::string port);
        Client(std::string ip, int port);
        ~Client();
        
        void on(std::string event_name, SockFunc func) override;
        void emit(std::string event_name, const SockData& data) override;
        void update() override;
        bool getDestroy() const override;

    };
}

#endif /* defined(__NylonSock__Sustainable__) */
