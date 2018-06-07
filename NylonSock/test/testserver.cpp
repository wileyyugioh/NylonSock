//
//  testserver.cpp
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
    std::string usrname;

    TestClientSock(NylonSock::Socket&& sock) : ClientSocket(std::move(sock)) {}
};

int main(int argc, const char * argv[])
{
	using namespace NylonSock;

	std::cout << gethostname() << std::endl;
    
	Server<TestClientSock> serv{ 3490 };

	serv.onConnect([&](TestClientSock& sock)
	{
        sock.on("usrname", [&](SockData data, TestClientSock& sock)
        {
            sock.usrname = data.getRaw();

            std::cout << sock.usrname + " joined the server." << std::endl;
            serv.emit("msgSend", {sock.usrname + " joined the server."});
        });

        sock.on("msgGet", [&](SockData data, TestClientSock& sock)
        {
            std::cout << sock.usrname + ": " + data.getRaw() << std::endl;
            serv.emit("msgSend", {sock.usrname + ": " + data.getRaw()});
        });

        sock.on("disconnect", [&]()
        {
            if(!sock.usrname.empty())
            {
                std::cout << sock.usrname + " left the server." << std::endl;
                serv.emit("msgSend", {sock.usrname + " left the server."});
            }
        });
	});
	
    serv.start();
    while(true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        msg = "SERVER: " + msg;
        std::cout << msg << std::endl;
        serv.emit("msgSend", msg);
    }

	//It never reaches here, does it...
    serv.stop();
}

