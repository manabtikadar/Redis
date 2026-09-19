#ifndef DISCOVERY_MANAGER_H
#define DISCOVERY_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class Server;

class DiscoveryManager
{
public:

    DiscoveryManager(
        Server& server,
        int server_port
    );

    ~DiscoveryManager();

    void start();

    void stop();

    void discover_nodes();

    std::vector<int> get_discovered_nodes();

private:

    Server& server;

    int server_port;

    int discovery_port;

    int discovery_fd;

    std::atomic<bool> running;

    std::thread listener_thread;

    std::mutex nodes_mutex;

    std::vector<int> discovered_nodes;

    void listen_loop();

    void handle_message(
        const std::string& message
    );

    void add_node(
        int port
    );
};

#endif