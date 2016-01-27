//
//  main.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 12/21/15.
//  Copyright (c) 2015 Wiley Yu. All rights reserved.
//

#include "NylonSock.hpp"

#include <iostream>

//for sleep
#include <unistd.h>

constexpr const char MYIP[] = "192.168.29.114";

class InClient : public NylonSock::Client
{
public:
    InClient(std::string ip, int port) : Client(ip, port)
    {
        std::cout << "ay?" << std::endl;
    }
};

class TestClientSock : public NylonSock::ClientSocket
{
public:
    TestClientSock(NylonSock::Socket sock) : ClientSocket(sock)
    {
        std::cout << "ayylmao" << std::endl;
    }
};

int main(int argc, const char * argv[])
{
    using namespace NylonSock;
    
    std::cout << gethostname() << std::endl;
    Server<TestClientSock> serv{3490};
    serv.onConnect([](TestClientSock& sock)
                   {
                       sock.emit("DANK", {"ya know it"});
                       std::cout << "emiiting" << std::endl;
                   });
    
    //test client?
    InClient client{MYIP, 3490};
    client.on("AAA", [](SockData data)
              {
                  std::cout << "A" << std::endl;
                  std::cout << data.getRaw() << std::endl;
              });
    client.on("DANK", [](SockData data)
              {
                  std::cout << data.getRaw() << std::endl;
              });
    
    serv.update();
    client.update();
}

