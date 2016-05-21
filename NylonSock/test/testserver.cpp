//
//  main.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 12/21/15.
//  Copyright (c) 2015 Wiley Yu. All rights reserved.
//

#include "NylonSock.hpp"

#include <iostream>
#include <thread>
#include <chrono>

class TestClientSock : public NylonSock::ClientSocket<TestClientSock>
{
public:
    TestClientSock(NylonSock::Socket sock) : ClientSocket(sock)
    {
    }

    std::string rand_text;
};

int main(int argc, const char * argv[])
{
	using namespace NylonSock;

	std::cout << gethostname() << std::endl;
<<<<<<< Updated upstream
    
	Server<TestClientSock> serv{ 3490 };
=======
	
	doit();

	Server<TestClientSock> serv{ 27015 };
>>>>>>> Stashed changes
	serv.onConnect([](TestClientSock& sock)
	{
		sock.emit("Event", {"This is being sent to all Event text!"});
        sock.on("okay", [](SockData data, TestClientSock& ps)
            {
                std::cout << data.getRaw() << std::endl;
            });
	});
	
    serv.start();
    while(serv.count() < 5)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100) );
    }

    serv.stop();
}

