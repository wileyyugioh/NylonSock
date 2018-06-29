//
//  testserver.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 12/21/15.
//  Copyright (c) 2015 Wiley Yu. All rights reserved.
//

#include "sharedclient.h"

#include <NylonSock.hpp>

#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, const char* argv[])
{
	using namespace NylonSock;

	std::cout << gethostname() << std::endl;
    
    //this is the port num -|
	Server<InClient> serv{3490};

	serv.onConnect([&serv](InClient& sock)
	{
        sock.on("usrname", [&serv](SockData data, InClient& sock)
        {
            sock.usrname = data.getRaw();
        });

        sock.on("room", [&serv](SockData data, InClient& sock)
        {
            sock.room = data.getRaw();

            std::string room = sock.room;
            std::cout << sock.usrname + " joined the server at room " + room << std::endl;
            serv.emit_if("msgSend", {sock.usrname + " joined the room."}, [room](const InClient& a)
            {
                return room == a.room;
            });
        });

        sock.on("msgGet", [&serv](SockData data, InClient& sock)
        {
            std::string room = sock.room;
            serv.emit_if("msgSend", {sock.usrname + ": " + data.getRaw()}, [room](const InClient& a)
            {
                return room == a.room;
            });
        });

        sock.on("disconnect", [&serv](InClient& sock)
        {
            if(!sock.usrname.empty())
            {
                std::cout << sock.usrname + " left the server." << std::endl;
                std::string room = sock.room;
                serv.emit_if("msgSend", {sock.usrname + " left the server."}, [room](const InClient& a)
                {
                    return room == a.room;
                });
            }
        });
	});
	
    serv.start();

    std::cout << "Entering text sending mode.\nEnter \\q to quit the server." << std::endl;
    while(true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if(msg == "\\q") break;
        msg = "SERVER: " + msg;
        std::cout << msg << std::endl;
        serv.emit("msgSend", msg);
    }

	//It never reaches here, does it...
    serv.stop();

    return 0;
}

