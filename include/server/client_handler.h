#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "database.h"
#include "replication_manager.h"

// Forward declaration.
// We don't need the full server.h here.
class Server;

void handle_client(
    int client_fd,
    Database& database,
    ReplicationManager& replication_manager,
    Server& server
);

#endif