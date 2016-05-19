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
    std::cout << "What text do you want to send?" << std::endl;
    std::cin >> client.rand;
    client.on("Event", [](SockData data, InClient& ps)
              {
                  ps.emit("okay", {ps.rand});
              });
    
    client.start();

    for(int i = 0; i < 50; i++)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100) );
    }

    client.stop();
    
    NSRelease();
}
