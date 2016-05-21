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

    std::string rand;
};

void toBeCalled(NylonSock::SockData data, InClient& ps)
{
    ps.emit("okay", {ps.rand});
}
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
    
<<<<<<< Updated upstream
    NylonSock::Client<InClient> client{MYIP, 3490};
    std::cout << "What text do you want to send?" << std::endl;
    std::cin >> client.get().rand;
    client.on("Event", &toBeCalled);
=======
    InClient client{MYIP, 27015};
    client.on("AAA", [](SockData data)
              {
                  std::cout << "A" << std::endl;
                  std::cout << data.getRaw() << std::endl;
              });
    client.on("DANK", [](SockData data)
              {
                  std::cout << data.getRaw() << std::endl;
              });
>>>>>>> Stashed changes
    
    client.start();

    for(int i = 0; i < 50; i++)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100) );
    }

    client.stop();
}
