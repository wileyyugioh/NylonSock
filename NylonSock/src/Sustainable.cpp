//
//  Sustainable.cpp
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/19/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#include "Sustainable.h"

namespace NylonSock
{
    TOOBIG::TOOBIG(std::string what) : std::runtime_error(what)
    {
        
    }
    
    SockData::SockData(std::string data) : raw_data(data)
    {
        if(data.size() > maximum32bit)
        {
            //throw error because data is too large
            throw TOOBIG(std::to_string(data.size()) );
        }
    }
    
    std::string SockData::getRaw() const
    {
        return raw_data;
    }

}
