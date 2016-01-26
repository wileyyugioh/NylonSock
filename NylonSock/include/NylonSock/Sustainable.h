//
//  Sustainable.h
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/19/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#ifndef __NylonSock__Sustainable__
#define __NylonSock__Sustainable__

#include "rapidjson/document.h"

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
    typedef void (*ServClientFunc)(ClientSocket);
    
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
        operator rapidjson::Document() const;
        std::string getRaw() const;
    };
    
    class ClientSocket
    {
    private:
        std::unique_ptr<Socket> _client;
        std::unordered_map<std::string, SockFunc> _functions;
    public:
        void on(std::string event_name, SockFunc func);
        void emit(std::string event_name, const SockData& data);
        void update();
    };
    
    class Server
    {
    private:
        static std::unique_ptr<FD_Set> _fdset;
        std::unique_ptr<Socket> _server;
        std::vector<std::unique_ptr<Socket> > _clients;
        std::unique_ptr<ServClientFunc> _func;
        
        void createServer(std::string port);
        void addToSet();
    public:
        Server(int port);
        Server(std::string port);
        ~Server();
        void onConnect(ServClientFunc func);
        void update();
        unsigned long count() const;
    };
    
    class Client
    {
    private:
        //see top of cpp file to see how data is sent
        std::unique_ptr<Socket> _server;
    public:
        void on(std::string event_name, SockFunc func);
        
        void emit(std::string event_name, const SockData& data) const;
    };
}

#endif /* defined(__NylonSock__Sustainable__) */
