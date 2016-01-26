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

int main(int argc, const char * argv[])
{
    /*
     constexpr char port[] = "3490";
     
     addrinfo hints = {0};
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE;
     
     using namespace NylonSock;
     Socket sock{nullptr, port, &hints};
     fcntl(sock, O_NONBLOCK);
     
     std::cout << O_NONBLOCK << std::endl;
     
     bind(sock);
     NylonSock::listen(sock, 100);
     
     std::cout << "Waiting for connections" << std::endl;
     std::cout << "Remote host name is: " << NylonSock::gethostname() << std::endl;
     
     std::cout << "Type into terminal: telnet " << NylonSock::gethostname() << " " << port << "!" << std::endl;
     
     while(true)
     {
     auto new_sock = accept(sock);
     if(new_sock == NULL_SOCKET)
     {
     continue;
     }
     
     std::cout << inet_ntop(sock) << std::endl;
     send(new_sock, "Hello, world!\n", 14, 0);
     
     break;
     }
     
    */
    /*
    using namespace NylonSock;
    std::cout << gethostname() << std::endl;
    Server serv{3490};
    serv.onConnect([](ClientSocket sock)
                   {
                       std::cout << "hi" << std::endl;
                   });
    
    while(serv.count() == 0)
    {
        serv.update();
        sleep(1);
    }
     */
}

