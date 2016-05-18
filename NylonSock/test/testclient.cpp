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

class InClient : public NylonSock::Client<InClient>
{
private:
public:
    InClient(std::string ip, int port) : Client(ip, port)
    {
        std::cout << "ay?" << std::endl;
    }

    std::string rand;
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
    client.rand = "hello";
    client.on("DANK", [](SockData data, InClient& ps)
              {
                  std::cout << ps.rand << std::endl;
                  std::cout << data.getRaw() << std::endl;
              });
    
    client.start();

    for(int i = 0; i < 5; i++)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000) );
      std::cout << i << std::endl;
    }

    client.stop();
    
    NSRelease();
}
