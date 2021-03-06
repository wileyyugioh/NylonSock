//
//  testclient.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/29/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#include "sharedclient.h"

#include <NylonSock.hpp>

#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, const char* argv[])
{
    if(argc == 1)
    {
        std::cout << "First argument is the ip to connect to" << std::endl;
        return -1;
    }
    
    const char* MYIP = argv[1];
    
    using namespace NylonSock;
    
    std::cout << gethostname() << std::endl;

    NylonSock::Client<InClient> client{MYIP, 3490};
    client.start();

    std::cout << "What is your username?" << std::endl;
    std::getline(std::cin, client.get().usrname);
    client.emit("usrname", {client.get().usrname});

    std::cout << "What room do you want to connect to?" << std::endl;
    std::getline(std::cin, client.get().room);
    client.emit("room", {client.get().room});

    client.on("msgSend", [](SockData data, InClient& client)
    {
        std::cout << data.getRaw() << std::endl;
    });

    std::cout << "Entering text sending mode.\nEnter \\q to quit the client." << std::endl;
    while(client.status() )
    {
        std::string msg;
        std::getline(std::cin, msg);
        if(msg == "\\q") break;
        client.emit("msgGet", {msg});
    }
    client.stop();
    std::cout << "Disconnected from server." << std::endl;

    return 0;
}
