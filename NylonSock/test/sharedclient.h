#ifndef SHAREDCLIENT_H
#define SHAREDCLIENT_H

#include <NylonSock.hpp>

class InClient : public NylonSock::ClientSocket<InClient>
{
public:
    std::string usrname;
    std::string room;

    using NylonSock::ClientSocket<InClient>::ClientSocket;
};

#endif /* SHAREDCLIENT_H */
