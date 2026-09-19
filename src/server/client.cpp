#include "client.h"
#include "client.h"
#include "client_handler.h"

Client::Client(int fd)
{
    this->fd = fd;
}

void Client::handle(
    Database& database,
    ReplicationManager& replication_manager,
    Server& server
)
{
    handle_client(
        this->fd,
        database,
        replication_manager,
        server
    );
}