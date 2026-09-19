// #ifndef ELECTION_MANAGER_H
// #define ELECTION_MANAGER_H

// #include <string>
// #include <vector>
// #include <mutex>

// class Server;

// class ElectionManager
// {
// private:

//     Server& server;

//     int election_server_fd;

//     int election_port;

//     std::mutex election_mutex;

//     bool running;

// public:

//     ElectionManager(
//         Server& server,
//         int election_port
//     );

//     ~ElectionManager();

//     // Start internal election server
//     void start();

//     // Stop internal election server
//     void stop();

//     // Start leader election
//     void start_election();

// private:

//     // Create election socket
//     void setup_socket();

//     // Accept incoming election connections
//     void accept_connections();

//     // Handle one election connection
//     void handle_connection(
//         int client_fd
//     );

//     // Send vote request
//     bool request_vote(
//         int peer_port,
//         int term,
//         int candidate_id
//     );
// };

// #endif

#ifndef ELECTION_MANAGER_H
#define ELECTION_MANAGER_H

#include <string>
#include <thread>
#include <atomic>

class Server;

class ElectionManager
{
public:

    ElectionManager(
        Server& server,
        int election_port
    );

    ~ElectionManager();

    // Start election server socket
    void start();

    // Stop election server
    void stop();

    // Automatically start a leader election
    void start_election();


private:

    Server& server;

    int election_server_fd;

    int election_port;

    std::atomic<bool> running;


    // ==============================================
    // Election server
    // ==============================================

    void setup_socket();

    void accept_connections();

    void handle_connection(
        int client_fd
    );


    // ==============================================
    // Candidate -> Replica
    // ==============================================

    bool request_vote(
        int peer_port,
        int term,
        int candidate_id
    );
};

#endif