#include "replication_manager.h"

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>


// ==================================================
// ADD REPLICA
// ==================================================

void ReplicationManager::add_replica(
    int replica_fd
)
{
    std::lock_guard<std::mutex> lock(
        replication_mutex
    );

    replicas.push_back(
        replica_fd
    );

    std::cout
        << "[REPLICATION] Replica connected. "
        << "fd = "
        << replica_fd
        << std::endl;
}


// ==================================================
// REMOVE REPLICA
// ==================================================

void ReplicationManager::remove_replica(
    int replica_fd
)
{
    std::lock_guard<std::mutex> lock(
        replication_mutex
    );

    auto it =
        std::find(
            replicas.begin(),
            replicas.end(),
            replica_fd
        );

    if (it != replicas.end())
    {
        replicas.erase(it);
    }

    std::cout
        << "[REPLICATION] Replica removed. "
        << "fd = "
        << replica_fd
        << std::endl;
}


// ==================================================
// REPLICATE
// ==================================================

void ReplicationManager::replicate(
    const std::string& command
)
{
    std::lock_guard<std::mutex> lock(
        replication_mutex
    );

    for (auto it = replicas.begin();
         it != replicas.end();)
    {
        int fd = *it;

        ssize_t sent =
            send(
                fd,
                command.c_str(),
                command.size(),
                0
            );

        if (sent <= 0)
        {
            std::cout
                << "[REPLICATION] "
                << "Replica disconnected. fd = "
                << fd
                << std::endl;

            close(fd);

            it =
                replicas.erase(it);

            continue;
        }

        ++it;
    }
}


// ==================================================
// REPLICA COUNT
// ==================================================

size_t ReplicationManager::replica_count() const
{
    std::lock_guard<std::mutex> lock(
        replication_mutex
    );

    return replicas.size();
}

void ReplicationManager::initial_sync(
    int replica_fd,
    Database& database
)
{
    std::vector<std::string> commands =
        database.get_snapshot_commands();

    std::cout
    << "[REPLICATION] Snapshot contains "
    << commands.size()
    << " commands"
    << std::endl;

    for (const auto& command : commands)
    {
        std::cout
            << "[REPLICATION] Snapshot command: "
            << command
            << std::endl;
    }

    const std::string start =
        "SYNC_START\n";

    send(
        replica_fd,
        start.c_str(),
        start.size(),
        0
    );

    for (const std::string& command : commands)
    {
        std::string message =
            command + "\n";

        send(
            replica_fd,
            message.c_str(),
            message.size(),
            0
        );
    }

    const std::string end =
        "SYNC_END\n";

    send(
        replica_fd,
        end.c_str(),
        end.size(),
        0
    );
}