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
    
    class ClientSocket
    {
    private:
        std::unique_ptr<Socket> _client;
        std::unordered_map<std::string, SockFunc> _functions;
        std::unique_ptr<FD_Set> _self_fd;
    public:
        ClientSocket(Socket sock);
        virtual ~ClientSocket() = default;
        virtual void on(std::string event_name, SockFunc func);
        virtual void emit(std::string event_name, const SockData& data);
        virtual void update();
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
            auto sets = select(*_fdset, TimeVal{1000});
            //sets[0] is an FD_Set of the sockets ready to be recv
            
            while(true)
            {
                auto new_sock = accept(*_server);
                if(new_sock == NULL_SOCKET)
                {
                    break;
                }
                
                //it is an actual socket
                _clients.push_back(std::make_unique<UsrSock>(new_sock) );
                
                //call the on connect func
                _func(*_clients.back() );
            }
            
            for(auto& it : _clients)
            {
                it->update();
            }
        };
        
        unsigned long count() const
        {
            return _clients.size();
        };
    };
    
    class Client
    {
    private:
        //see top of cpp file to see how data is sent
        std::unique_ptr<Socket> _server;
    public:
        Client(std::string port);
        Client(int port);
        
        void on(std::string event_name, SockFunc func);
        
        void emit(std::string event_name, const SockData& data) const;
    };
}

#endif /* defined(__NylonSock__Sustainable__) */
