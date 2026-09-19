#ifndef CLIENT_H
#define CLIENT_H

#include "database.h"
#include "replication_manager.h"

class Server;

class Client
{
private:

    int fd;

public:

    Client(int fd);

    void handle(
        Database& database,
        ReplicationManager& replication_manager,
        Server& server
    );
};

#endif