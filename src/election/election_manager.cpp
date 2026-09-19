#include "election_manager.h"
#include "server.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <random>
#include <chrono>

ElectionManager::ElectionManager(
    Server& server,
    int election_port
)
    : server(server),
      election_server_fd(-1),
      election_port(election_port),
      running(false)
{
}

ElectionManager::~ElectionManager()
{
    stop();
}

void ElectionManager::setup_socket()
{
    election_server_fd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (election_server_fd < 0)
    {
        std::cerr
            << "[ELECTION] socket() failed: "
            << strerror(errno)
            << std::endl;

        return;
    }

    // Allow immediate reuse of the port
    int option = 1;

    setsockopt(
        election_server_fd,
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
        htons(election_port);

    if (
        bind(
            election_server_fd,
            reinterpret_cast<
                sockaddr*
            >(&address),
            sizeof(address)
        ) < 0
    )
    {
        std::cerr
            << "[ELECTION] bind() failed on port "
            << election_port
            << ": "
            << strerror(errno)
            << std::endl;

        close(election_server_fd);

        election_server_fd = -1;

        return;
    }

    if (
        listen(
            election_server_fd,
            10
        ) < 0
    )
    {
        std::cerr
            << "[ELECTION] listen() failed: "
            << strerror(errno)
            << std::endl;

        close(election_server_fd);

        election_server_fd = -1;

        return;
    }

    std::cout
        << "[ELECTION] Listening on port "
        << election_port
        << std::endl;
}

void ElectionManager::start()
{
    setup_socket();

    if (election_server_fd == -1)
    {
        return;
    }

    running = true;

    std::thread(
        &ElectionManager::accept_connections,
        this
    ).detach();

    std::cout
        << "[ELECTION] Election manager started"
        << std::endl;
}

void ElectionManager::accept_connections()
{
    while (running)
    {
        int client_fd =
            accept(
                election_server_fd,
                nullptr,
                nullptr
            );

        if (client_fd < 0)
        {
            if (running)
            {
                std::cerr
                    << "[ELECTION] accept() failed"
                    << std::endl;
            }

            continue;
        }

        std::thread(
            &ElectionManager::handle_connection,
            this,
            client_fd
        ).detach();
    }
}

void ElectionManager::stop()
{
    bool was_running =
        running.exchange(false);

    if (!was_running)
    {
        return;
    }

    if (election_server_fd != -1)
    {
        close(election_server_fd);

        election_server_fd = -1;
    }

    std::cout
        << "[ELECTION] Election manager stopped"
        << std::endl;
}

void ElectionManager::handle_connection(
    int client_fd
)
{
    char buffer[1024];

    ssize_t bytes =
        recv(
            client_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

    if (bytes <= 0)
    {
        close(client_fd);
        return;
    }

    buffer[bytes] = '\0';

    std::string message(buffer);

    std::cout
        << "[ELECTION] Received: "
        << message;

    std::stringstream ss(message);

    std::string command;

    ss >> command;

    // ==========================================
    // VOTE_REQUEST
    // ==========================================

    if (command == "VOTE_REQUEST")
    {
        int term;
        int candidate_id;

        ss
            >> term
            >> candidate_id;

        bool granted =
            server.handle_vote_request(
                term,
                candidate_id
            );

        std::string response =
            "VOTE_RESPONSE "
            + std::to_string(term)
            + " "
            + std::to_string(
                server.get_server_id()
            )
            + " "
            + std::to_string(
                granted ? 1 : 0
            )
            + "\r\n";

        send(
            client_fd,
            response.c_str(),
            response.size(),
            0
        );
    }

    close(client_fd);
}

// void ElectionManager::start_election()
// {
//     std::cout
//         << "[ELECTION] Starting leader election"
//         << std::endl;

//     // ==============================================
//     // 1. Tell Server to become candidate
//     // ==============================================

//     server.begin_election();

//     // ==============================================
//     // 2. Get election information
//     // ==============================================

//     int term =
//         server.get_current_term();

//     int candidate_id =
//         server.get_server_id();

//     std::vector<int> peers =
//         server.get_peer_ports();

//     std::cout
//         << "[ELECTION] Candidate = "
//         << candidate_id
//         << std::endl;

//     std::cout
//         << "[ELECTION] Term = "
//         << term
//         << std::endl;

//     // ==============================================
//     // 3. Ask every other server for a vote
//     // ==============================================

//     for (int peer_port : peers)
//     {
//         // Don't vote-request ourselves
//         if (peer_port == candidate_id)
//         {
//             continue;
//         }

//         std::cout
//             << "[ELECTION] Requesting vote from "
//             << peer_port
//             << std::endl;

//         std::thread(
//             &ElectionManager::request_vote,
//             this,
//             peer_port,
//             term,
//             candidate_id
//         ).detach();
//     }
// }

// void ElectionManager::start_election()
// {
//     std::cout
//         << "[ELECTION] Starting leader election"
//         << std::endl;

//     std::cout
//         << "[ELECTION] BEFORE begin_election()"
//         << std::endl;

//     server.begin_election();

//     std::cout
//         << "[ELECTION] AFTER begin_election()"
//         << std::endl;

//     int term =
//         server.get_current_term();

//     std::cout
//         << "[ELECTION] Current term = "
//         << term
//         << std::endl;

//     int candidate_id =
//         server.get_server_id();

//     std::cout
//         << "[ELECTION] Candidate ID = "
//         << candidate_id
//         << std::endl;

//     std::vector<int> peers =
//         server.get_peer_ports();

//     std::cout
//         << "[ELECTION] Peer count = "
//         << peers.size()
//         << std::endl;

//     for (int peer_port : peers)
//     {
//         std::cout
//             << "[ELECTION] Peer = "
//             << peer_port
//             << std::endl;

//         if (peer_port == candidate_id)
//         {
//             continue;
//         }

//         std::cout
//             << "[ELECTION] Requesting vote from "
//             << peer_port
//             << std::endl;

//         std::thread(
//             &ElectionManager::request_vote,
//             this,
//             peer_port,
//             term,
//             candidate_id
//         ).detach();
//     }
// }

void ElectionManager::start_election()
{
    std::random_device rd;

    std::mt19937 generator(rd());

    std::uniform_int_distribution<int> distribution(
        150,
        500
    );

    int timeout =
        distribution(generator);

    std::cout
        << "[ELECTION] Waiting "
        << timeout
        << " ms before election"
        << std::endl;

    std::this_thread::sleep_for(
        std::chrono::milliseconds(timeout)
    );

    std::cout
        << "[ELECTION] Starting leader election"
        << std::endl;

    server.begin_election();

    int term =
        server.get_current_term();

    int candidate_id =
        server.get_server_id();

    std::vector<int> peers =
        server.get_peer_ports();

    for (int peer_port : peers)
    {
        if (peer_port == candidate_id)
        {
            continue;
        }

        std::thread(
            &ElectionManager::request_vote,
            this,
            peer_port,
            term,
            candidate_id
        ).detach();
    }
}

bool ElectionManager::request_vote(
    int peer_port,
    int term,
    int candidate_id
)
{
    // ==============================================
    // Create TCP socket
    // ==============================================

    int fd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (fd < 0)
    {
        std::cerr
            << "[ELECTION] socket() failed: "
            << strerror(errno)
            << std::endl;

        return false;
    }

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    // Redis port:
    //
    // 6381
    //
    // Election port:
    //
    // 7381

    address.sin_port =
        htons(
            peer_port + 1000
        );

    if (
        inet_pton(
            AF_INET,
            "127.0.0.1",
            &address.sin_addr
        ) <= 0
    )
    {
        std::cerr
            << "[ELECTION] Invalid peer address"
            << std::endl;

        close(fd);

        return false;
    }

    // ==============================================
    // Connect to peer's election socket
    // ==============================================

    std::cout
        << "[ELECTION] Connecting to "
        << peer_port
        << " election port "
        << peer_port + 1000
        << std::endl;

    if (
        connect(
            fd,
            reinterpret_cast<
                sockaddr*
            >(&address),
            sizeof(address)
        ) < 0
    )
    {
        std::cerr
            << "[ELECTION] Could not connect to "
            << peer_port
            << ": "
            << strerror(errno)
            << std::endl;

        close(fd);

        return false;
    }

    // ==============================================
    // Send VOTE_REQUEST
    // ==============================================

    std::string request =
        "VOTE_REQUEST "
        + std::to_string(term)
        + " "
        + std::to_string(candidate_id)
        + "\r\n";

    std::cout
        << "[ELECTION] Sending: "
        << request;

    ssize_t sent =
        send(
            fd,
            request.c_str(),
            request.size(),
            0
        );

    if (sent <= 0)
    {
        std::cerr
            << "[ELECTION] Failed to send vote request"
            << std::endl;

        close(fd);

        return false;
    }

    // ==============================================
    // Wait for VOTE_RESPONSE
    // ==============================================

    char buffer[1024];

    ssize_t bytes =
        recv(
            fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

    if (bytes <= 0)
    {
        std::cerr
            << "[ELECTION] No vote response from "
            << peer_port
            << std::endl;

        close(fd);

        return false;
    }

    buffer[bytes] =
        '\0';

    std::string response(
        buffer
    );

    std::cout
        << "[ELECTION] Received: "
        << response;

    // ==============================================
    // Parse:
    //
    // VOTE_RESPONSE 1 6381 1
    //
    // term       = 1
    // voter_id   = 6381
    // granted    = 1
    // ==============================================

    std::stringstream ss(
        response
    );

    std::string command;

    int response_term;
    int voter_id;
    int granted;

    ss
        >> command
        >> response_term
        >> voter_id
        >> granted;

    close(fd);

    if (
        command != "VOTE_RESPONSE"
    )
    {
        std::cerr
            << "[ELECTION] Invalid vote response"
            << std::endl;

        return false;
    }

    // Ignore response from another election term
    if (
        response_term != term
    )
    {
        std::cerr
            << "[ELECTION] Vote response has "
            << "different term"
            << std::endl;

        return false;
    }

    // ==============================================
    // Vote granted
    // ==============================================

    if (granted == 1)
    {
        std::cout
            << "[ELECTION] Vote granted by "
            << voter_id
            << std::endl;

        server.record_vote(
            voter_id
        );

        return true;
    }

    std::cout
        << "[ELECTION] Vote rejected by "
        << voter_id
        << std::endl;

    return false;
}