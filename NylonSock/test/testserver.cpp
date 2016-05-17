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
	NSInit();

	std::cout << gethostname() << std::endl;
	
	
	Server<TestClientSock> serv{ 3490 };
	serv.onConnect([](TestClientSock& sock)
	{
		sock.emit("DANK", { "ya know it" });
		std::cout << "emiiting" << std::endl;
	});
	
    serv.start();
    while(serv.count() == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100) );
    }
    
    serv.stop();

    NSRelease();
}

