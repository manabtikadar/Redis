#include "discovery_manager.h"
#include "server.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// DiscoveryManager::DiscoveryManager(
//     Server& server,
//     int server_port
// )
//     : server(server),
//       server_port(server_port),
//       discovery_port(16000 + server_port),
//       discovery_fd(-1),
//       running(false)
// {
// }

DiscoveryManager::DiscoveryManager(
    Server& server,
    int server_port
)
    : server(server),
      server_port(server_port),
      discovery_port(2300),
      discovery_fd(-1),
      running(false)
{
}

DiscoveryManager::~DiscoveryManager()
{
    stop();
}

void DiscoveryManager::start()
{
    discovery_fd =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0
        );

    if (discovery_fd < 0)
    {
        std::cerr
            << "[DISCOVERY] socket() failed: "
            << strerror(errno)
            << std::endl;

        return;
    }

    int option = 1;

    setsockopt(
        discovery_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &option,
        sizeof(option)
    );

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    address.sin_addr.s_addr =
        INADDR_ANY;

    address.sin_port =
        htons(discovery_port);

    if (
        bind(
            discovery_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) < 0
    )
    {
        std::cerr
            << "[DISCOVERY] bind() failed on port "
            << discovery_port
            << ": "
            << strerror(errno)
            << std::endl;

        close(discovery_fd);

        discovery_fd = -1;

        return;
    }

    running = true;

    listener_thread =
        std::thread(
            &DiscoveryManager::listen_loop,
            this
        );

    std::cout
        << "[DISCOVERY] Listening on UDP port "
        << discovery_port
        << std::endl;
}

void DiscoveryManager::listen_loop()
{
    char buffer[1024];

    while (running)
    {
        sockaddr_in sender{};

        socklen_t sender_len =
            sizeof(sender);

        ssize_t bytes =
            recvfrom(
                discovery_fd,
                buffer,
                sizeof(buffer) - 1,
                0,
                reinterpret_cast<sockaddr*>(&sender),
                &sender_len
            );

        if (bytes < 0)
        {
            if (running)
            {
                std::cerr
                    << "[DISCOVERY] recvfrom() failed: "
                    << strerror(errno)
                    << std::endl;
            }

            continue;
        }

        buffer[bytes] = '\0';

        std::string message(buffer);

        std::cout
            << "[DISCOVERY] Received: "
            << message
            << std::endl;

        handle_message(message);
    }
}

void DiscoveryManager::handle_message(
    const std::string& message
)
{
    std::stringstream ss(message);

    std::string command;

    int port;

    ss >> command;

    // ==============================================
    // DISCOVER
    // ==============================================

    if (command == "DISCOVER")
    {
        ss >> port;

        if (port == server_port)
        {
            return;
        }

        add_node(port);

        // ----------------------------------------------
        // Tell the new node that we exist
        // ----------------------------------------------

        std::string response =
            "NODE "
            + std::to_string(server_port);

        sockaddr_in destination{};

        destination.sin_family =
            AF_INET;

        destination.sin_port =
            htons(
                16000 + port
            );

        inet_pton(
            AF_INET,
            "127.0.0.1",
            &destination.sin_addr
        );

        sendto(
            discovery_fd,
            response.c_str(),
            response.size(),
            0,
            reinterpret_cast<sockaddr*>(&destination),
            sizeof(destination)
        );

        return;
    }

    // ==============================================
    // NODE
    // ==============================================

    if (command == "NODE")
    {
        ss >> port;

        if (port == server_port)
        {
            return;
        }

        add_node(port);

        return;
    }
}

void DiscoveryManager::add_node(
    int port
)
{
    std::lock_guard<std::mutex> lock(
        nodes_mutex
    );

    for (int existing : discovered_nodes)
    {
        if (existing == port)
        {
            return;
        }
    }

    discovered_nodes.push_back(port);

    std::cout
        << "[DISCOVERY] Discovered RediForge node: "
        << port
        << std::endl;

    server.add_peer(port);
}

std::vector<int>
DiscoveryManager::get_discovered_nodes()
{
    std::lock_guard<std::mutex> lock(
        nodes_mutex
    );

    return discovered_nodes;
}

void DiscoveryManager::stop()
{
    running = false;

    if (discovery_fd != -1)
    {
        close(discovery_fd);

        discovery_fd = -1;
    }

    if (
        listener_thread.joinable()
    )
    {
        listener_thread.join();
    }

    std::cout
        << "[DISCOVERY] Discovery manager stopped"
        << std::endl;
}

void DiscoveryManager::discover_nodes()
{
    int fd =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0
        );

    if (fd < 0)
    {
        std::cerr
            << "[DISCOVERY] Failed to create "
            << "broadcast socket"
            << std::endl;

        return;
    }

    int broadcast = 1;

    setsockopt(
        fd,
        SOL_SOCKET,
        SO_BROADCAST,
        &broadcast,
        sizeof(broadcast)
    );

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    address.sin_port =
        htons(16000);

    address.sin_addr.s_addr =
        INADDR_BROADCAST;

    std::string message =
        "DISCOVER "
        + std::to_string(server_port);

    sendto(
        fd,
        message.c_str(),
        message.size(),
        0,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    );

    std::cout
        << "[DISCOVERY] Broadcast sent: "
        << message
        << std::endl;

    close(fd);
}