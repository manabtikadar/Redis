#ifndef REPLICATION_MANAGER_H
#define REPLICATION_MANAGER_H

#include <string>
#include <vector>
#include <mutex>

#include "database.h"

class ReplicationManager
{
private:

    // TCP socket FDs of connected replicas
    std::vector<int> replicas;

    mutable std::mutex replication_mutex;

public:

    void add_replica(
        int replica_fd
    );

    void remove_replica(
        int replica_fd
    );

    void replicate(
        const std::string& command
    );

    void initial_sync(
        int replica_fd,
        Database& database
    );

    size_t replica_count() const;
};

#endif