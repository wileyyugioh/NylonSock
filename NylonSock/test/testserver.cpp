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
#include <algorithm>
#include <map>
#include <vector>

int main(int argc, const char* argv[])
{
	using namespace NylonSock;

	std::cout << gethostname() << std::endl;
    
    //this is the port num -|
	Server<InClient> serv{3490};

    /*
        We can get away with this being thread safe
        by only accessing the map in onConnect
        since our server's update loop runs in a single thread
    */
    std::map<std::string, std::vector<InClient*> > rooms;

	serv.onConnect([&serv, &rooms](InClient& sock)
	{
        sock.on("usrname", [&serv](SockData data, InClient& sock)
        {
            sock.usrname = data.getRaw();
        });

        sock.on("room", [&serv, &rooms](SockData data, InClient& sock)
        {
            sock.room = data.getRaw();

            rooms[sock.room].push_back(&sock);

            std::cout << sock.usrname + " joined the server at room " + sock.room << std::endl;
            for(auto& it : rooms[sock.room])
            {
                it->emit("msgSend", {sock.usrname + " joined the room."});
            }
        });

        sock.on("msgGet", [&serv, &rooms](SockData data, InClient& sock)
        {
            for(auto& it : rooms[sock.room])
            {
                it->emit("msgSend", {sock.usrname + ": " + data.getRaw()});
            }
        });

        sock.on("disconnect", [&serv, &rooms](InClient& sock)
        {
            if(!sock.usrname.empty())
            {
                auto& vec = rooms[sock.room];
                vec.erase(std::remove(vec.begin(), vec.end(), &sock), vec.end());

                std::cout << sock.usrname + " left the server." << std::endl;
                std::string room = sock.room;
                for(auto& it : rooms[sock.room])
                {
                    it->emit("msgSend", {sock.usrname + " left the server."});
                }
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
