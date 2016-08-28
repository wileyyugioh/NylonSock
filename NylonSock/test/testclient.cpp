//
//  testclient.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/29/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#include "NylonSock.hpp"

#include <iostream>
#include <chrono>
#include <thread>

class InClient : public NylonSock::ClientSocket<InClient>
{
private:
public:
    InClient(NylonSock::Socket sock) : ClientSocket(sock)
    {
    }

    std::string usrname, msg;
};

int main(int argc, const char * argv[])
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
    client.on("msgSend", [](SockData data, InClient& client)
    {
        std::cout << data.getRaw() << std::endl;
    });

    std::cout << "Entering text sending mode.\nPress CTRL-C to quit the client." << std::endl;
    while(true)
    {
        std::getline(std::cin, client.get().msg);

        client.emit("msgGet", {client.get().msg});
    }

    client.stop();
}
