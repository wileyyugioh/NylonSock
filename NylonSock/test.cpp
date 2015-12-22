//
//  main.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 12/21/15.
//  Copyright (c) 2015 Wiley Yu. All rights reserved.
//

#include "Socket.h"

int main(int argc, const char * argv[])
{
    constexpr char port[] = "3490";
    
    addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    using namespace NylonSock;
    Socket sock{nullptr, port, &hints};
    
    bind(sock);
    NylonSock::listen(sock, 100);
    
    std::cout << "Waiting for connections" << std::endl;
    std::cout << "Remote host name is: " << NylonSock::gethostname() << std::endl;
    
    std::cout << "Type into terminal: \"telnet **hostname** " << port << "!" << std::endl;
    
    while(true)
    {
        auto new_sock = accept(sock);
        
        std::cout << inet_ntop(sock) << std::endl;
        send(new_sock, "Hello, world! \n", 13, 0);
        
        break;
    }
}

