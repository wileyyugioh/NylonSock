//
//  Sustainable.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/19/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#include "Sustainable.h"

#include "Socket.h"

#include <cstdint>

/*
 How data is sent:
 
 Beginning is uint32_t of size
 Next is uint32_t of event name size
 Next is name
 Rest is data.
 
 */

namespace NylonSock
{
    constexpr size_t sizeint32 = sizeof(uint32_t);
    std::unique_ptr<FD_Set> Server::_fdset(new FD_Set);
    
    TOOBIG::TOOBIG(std::string what) : std::runtime_error(what)
    {
        
    }
    
    SockData::SockData(std::string data) : raw_data(data)
    {
    }
    
    SockData::operator rapidjson::Document() const
    {
        //RVO. no fear!
        rapidjson::Document doc;
        doc.Parse(raw_data.c_str() );
        
        return doc;
    }
    
    std::string SockData::getRaw() const
    {
        return raw_data;
    }
    
    void emit(std::string event_name, const SockData& data, Socket& socket)
    {
        //sends data to server/client
        
        //assumes socket is already binded
        std::string raw_data = data.getRaw();
        
        uint32_t sizeofstr = htonl(raw_data.size() );
        uint32_t sizeofevent = htonl(event_name.size() );
        
        //sending size of data
        send(socket, &sizeofstr, sizeint32, NULL);
        //sending size of event name
        send(socket, &sizeofevent, sizeint32, NULL);
        //sending event name
        send(socket, event_name.c_str(), event_name.size(), NULL);
        
        //sending total data
        send(socket, raw_data.c_str(), raw_data.size(), NULL);
    }
    
    void Client::emit(std::string event_name, const SockData& data) const
    {
        //sends data to server
        NylonSock::emit(event_name, data, *_server);
        
    }
    
    void ClientSocket::emit(std::string event_name, const NylonSock::SockData &data)
    {
        //sends data to client
        NylonSock::emit(event_name, data, *_client);
    }
    
    void ClientSocket::on(std::string event_name, SockFunc func)
    {
        _functions[event_name] = func;
    }
    
    void recvData(Socket& sock, std::unordered_map<std::string, SockFunc>& _functions)
    {
        while(true)
        {
            uint32_t datalen, messagelen;
            
            //recieves data from client
            //also assumes socket is non blocking
            char status = recv(sock, &datalen, sizeint32, NULL);
            
            //check to see if there is any more data to be received
            if(status == -1)
            {
                break;
            }
            
            //receive message length
            recv(sock, &messagelen, sizeint32, NULL);
            
            datalen = ntohl(datalen);
            messagelen = ntohl(messagelen);
            
            //allocate buffers!
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
                _functions[messagestr](SockData{datastr});
            }
            
            //else, the data gets thrown away
        }
    }
    
    void ClientSocket::update()
    {
        recvData(*_client, _functions);
    }
    
    Server::Server(int port) : Server(std::to_string(port) )
    {
    }
    
    Server::Server(std::string port)
    {
        createServer(port);
        addToSet();
    }
    
    Server::~Server() = default;
    
    void Server::createServer(std::string port)
    {
        addrinfo hints = {0};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        _server = std::make_unique<Socket>(nullptr, port.c_str(), &hints);
        fcntl(*_server, O_NONBLOCK);
        
        bind(*_server);
        
        constexpr char backlog = 100;
        
        listen(*_server, backlog);
    }
    
    void Server::addToSet()
    {
        _fdset->set(*_server);
    }
    
    void Server::update()
    {
        auto sets = select(*_fdset, TimeVal{1000});
        //sets[0] is an FD_Set of the sockets ready to be recv
        
        while(true)
        {
            //checks if the socket needs to be recv
            if(!sets[0].isset(*_server) )
            {
                break;
            }
            
            _clients.push_back(std::make_unique<Socket>(accept(*_server) ) );
        }
    }
    
    void Server::onConnect(ServClientFunc func)
    {
        _func = std::make_unique<ServClientFunc>(func);
    }
    
    unsigned long Server::count() const
    {
        return _clients.size();
    }
}