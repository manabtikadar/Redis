// #ifndef SERVER_H
// #define SERVER_H

// #include "database.h"
// #include "replication_manager.h"
// #include "election_manager.h"
// #include <thread>
// #include <string>
// #include <mutex>

// enum class ServerRole
// {
//     PRIMARY,
//     REPLICA,
//     CANDIDATE
// };

// class Server
// {
// public:
//     Server(int port);
//     ~Server();

//     void start();

//     bool connect_to_primary(
//         const std::string& host,
//         int port
//     );

//     void promote_to_primary();

//     ServerRole get_role() const;

//     int get_server_id() const;

//     bool handle_vote_request(
//         int term,
//         int candidate_id
//     );

// private:
//     int server_fd;
//     int port;

//     Database database;

//     std::thread replication_thread;

//     void setup_socket();

//     void accept_clients();

//     void replication_loop();

//     void handle_primary_failure();

//     ReplicationManager replication_manager;

//     ServerRole role =
//         ServerRole::PRIMARY;

//     std::string primary_host;

//     int primary_port ;

//     int primary_fd ;

//     // ==============================================
//     // Election state
//     // ==============================================

//     int server_id;

//     int current_term;

//     int voted_for;

//     std::mutex election_state_mutex;

//     ElectionManager election_manager;
// };

// #endif

#ifndef SERVER_H
#define SERVER_H

#include "database.h"
#include "replication_manager.h"
#include "election_manager.h"
#include "discovery_manager.h"

#include <thread>
#include <string>
#include <mutex>
#include <vector>
#include <set>

enum class ServerRole
{
    PRIMARY,
    REPLICA,
    CANDIDATE
};

class Server
{
public:

    Server(int port);
    ~Server();

    // ==============================================
    // Server lifecycle
    // ==============================================

    void start();


    // ==============================================
    // Replication
    // ==============================================

    bool connect_to_primary(
        const std::string& host,
        int port
    );

    void promote_to_primary();

    ServerRole get_role() const;


    // ==============================================
    // Server identity
    // ==============================================

    int get_server_id() const;


    // ==============================================
    // Election
    // ==============================================

    int get_current_term() const;

    std::vector<int> get_peer_ports() const;

    void add_peer(
        int peer_port
    );

    void begin_election();

    void record_vote(
        int voter_id
    );

    bool handle_vote_request(
        int term,
        int candidate_id
    );


private:

    // ==============================================
    // Normal server state
    // ==============================================

    int server_fd;

    int port;

    Database database;

    std::thread replication_thread;


    // ==============================================
    // Normal Redis server
    // ==============================================

    void setup_socket();

    void accept_clients();


    // ==============================================
    // Replication
    // ==============================================

    void replication_loop();

    void handle_primary_failure();

    ReplicationManager replication_manager;

    ServerRole role =
        ServerRole::PRIMARY;

    std::string primary_host;

    int primary_port = -1;

    int primary_fd = -1;


    // ==============================================
    // Election state
    // ==============================================

    int server_id;

    int current_term;

    int voted_for;

    std::set<int> votes_received;

    std::vector<int> peer_ports;

    bool election_in_progress = false;

    mutable std::mutex election_state_mutex;

    ElectionManager election_manager;

    // DiscoveryManager discovery_manager;
};

#endif