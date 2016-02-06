//
//  testclient.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/29/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#include "NylonSock.hpp"

#include <iostream>

class InClient : public NylonSock::Client
{
public:
    InClient(std::string ip, int port) : Client(ip, port)
    {
        std::cout << "ay?" << std::endl;
    }
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
    NSInit();
    
    std::cout << gethostname() << std::endl;
    
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
    
    client.update();
    
    NSRelease();
}
