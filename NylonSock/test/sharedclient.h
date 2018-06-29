#ifndef SHAREDCLIENT_H
#define SHAREDCLIENT_H

#include <NylonSock.hpp>

class InClient : public NylonSock::ClientSocket<InClient>
{
public:
    std::string usrname;
    std::string room;

    InClient(NylonSock::Socket&& sock) : ClientSocket(std::move(sock)) {}
};

#endif /* SHAREDCLIENT_H */
