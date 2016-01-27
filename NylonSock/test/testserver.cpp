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
    using namespace NylonSock;
    std::cout << gethostname() << std::endl;
    Server serv{3490};
    serv.onConnect([](ClientSocket& sock)
                   {
                       sock.emit("AAA", {"hi"});
                   });
    
    while(serv.count() == 0)
    {
        serv.update();
        sleep(1);
    }
}

