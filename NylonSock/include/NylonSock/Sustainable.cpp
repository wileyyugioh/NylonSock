//
//  Sustainable.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/19/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#include "Sustainable.h"

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
    constexpr size_t sizeint32 = sizeof(uint32_t);
    constexpr uint32_t maximum32bit = std::numeric_limits<uint32_t>::max();
    
    TOOBIG::TOOBIG(std::string what) : std::runtime_error(what)
    {
        
    }
    
    SockData::SockData(std::string data) : raw_data(data)
    {
        if(data.size() > maximum32bit)
        {
            //throw error because data is too large
            throw TOOBIG(std::to_string(data.size()) );
        }
    }
    
    std::string SockData::getRaw() const
    {
        return raw_data;
    }
    
    void emitSend(std::string event_name, const SockData& data, Socket& socket)
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
    
    Client::Client(std::string port) : Client(std::stoi(port) )
    {
        
    }
    
    Client::Client(int port)
    {
        
    }
    
    void Client::emit(std::string event_name, const SockData& data) const
    {
        //sends data to server
        emitSend(event_name, data, *_server);
        
    }
    
    void ClientSocket::emit(std::string event_name, const NylonSock::SockData &data)
    {
        //sends data to client
        emitSend(event_name, data, *_client);
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
            char success = recv(sock, &datalen, sizeint32, NULL);
            
            //break in case of errors
            if(success == -1)
            {
                break;
            }
            
            //receive message length
            recv(sock, &messagelen, sizeint32, NULL);
            
            //convert to host language
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
        //lazy initialization
        if(_self_fd == nullptr)
        {
            _self_fd = std::make_unique<FD_Set>();
            _self_fd->set(*_client);
        }
        
        //see if there is any need to recv
        if(select(*_self_fd, TimeVal{1000})[0].size() == 0)
        {
            return;
        }
        
        recvData(*_client, _functions);
    }
    
    ClientSocket::ClientSocket(Socket sock)
    {
        _client = std::make_unique<Socket>(sock);
    }
}