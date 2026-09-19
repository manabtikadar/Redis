#include "server.h"
#include "client_handler.h"
#include "client.h"
#include "database.h"
#include "command_handler.h"
#include "command.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <cerrno>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>

using namespace std;

Server::Server(int port)
    : server_fd(-1),
      port(port),
      role(ServerRole::PRIMARY),
      primary_port(-1),
      primary_fd(-1),
      server_id(port),
      current_term(0),
      voted_for(-1),
      election_in_progress(false),
      election_manager(
          *this,
          port + 1000
      )
{
}

Server::~Server()
{
    if (server_fd != -1)
    {
        close(server_fd);
    }
}

ServerRole Server::get_role() const
{
    return role;
}

void Server::setup_socket()
{
    // --------------------------------------------------
    // 1. Prepare the address information
    // --------------------------------------------------

    // struct addrinfo {
    //     int              ai_flags;      // options/flags
    //     int              ai_family;     // IPv4, IPv6, or either
    //     int              ai_socktype;   // TCP or UDP
    //     int              ai_protocol;   // protocol
    //     size_t           ai_addrlen;    // size of address
    //     struct sockaddr *ai_addr;       // actual IP + port
    //     char            *ai_canonname;  // canonical hostname
    //     struct addrinfo *ai_next;       // next result
    // };

    struct addrinfo hints{};
    struct addrinfo* serverInfo;

    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_PASSIVE;      // For wildcard IP address

    int status = getaddrinfo(
        nullptr,                 // hostname (nullptr for localhost)
        to_string(port).c_str(), // redis port
        &hints,
        &serverInfo
    );

    if (status != 0)
    {
        cerr << "getaddrinfo error: "
             << gai_strerror(status)
             << endl;

        exit(1);
    }

    // --------------------------------------------------
    // 2. Create the socket
    // --------------------------------------------------

    server_fd = socket(
        serverInfo->ai_family,
        serverInfo->ai_socktype,
        serverInfo->ai_protocol
    );

    if (server_fd == -1)
    {
        cerr << "socket error: "
             << strerror(errno)
             << endl;

        freeaddrinfo(serverInfo);
        exit(1);
    }

    // --------------------------------------------------
    // 3. Bind socket to port 6379
    // --------------------------------------------------

    if (bind(
        server_fd,
        serverInfo->ai_addr,
        serverInfo->ai_addrlen
    ) == -1)
    {
        cerr << "bind() failed: "
             << strerror(errno)
             << endl;

        close(server_fd);
        freeaddrinfo(serverInfo);

        exit(1);
    }
    
    // We don't need the address information anymore
    freeaddrinfo(serverInfo);

    // --------------------------------------------------
    // 4. Start listening
    // --------------------------------------------------
    if (listen(server_fd, 10) == -1)
    {
        cerr << "listen() failed: "
             << strerror(errno)
             << endl;

        close(server_fd);
        exit(1);
    }

    cout << "RediForge listening on port "
         << port << endl;
}

void Server::accept_clients()
{
    // --------------------------------------------------
    // 5. Keep the server running
    // --------------------------------------------------

    while (true)
    {
        cout << "Waiting for client..." << endl;

        int client_fd = accept(
            server_fd,
            nullptr,
            nullptr
        );

        if (client_fd == -1)
        {
            cerr << "accept() failed: "
                 << strerror(errno)
                 << endl;

            continue;
        }

        thread client_thread(
            [this,client_fd]()
            {
                Client client(client_fd);

                client.handle(
                    database,
                    replication_manager,
                    *this
                );
            }
        );

        client_thread.detach();
    }
}

void Server::start()
{
    // Load previous database
    if (database.load_from_file("dump.rdb"))
    {
        std::cout
            << "Database loaded from dump.rdb"
            << std::endl;
    }
    else
    {
        std::cout
            << "No existing database found"
            << std::endl;
    }

    election_manager.start();

    // Start automatic discovery
    // discovery_manager.start();

    // // Announce ourselves
    // discovery_manager.discover_nodes();

    setup_socket();

    accept_clients();
}

bool Server::connect_to_primary(
    const std::string& host,
    int port
)
{
    std::cout
        << "[REPLICATION] Connecting to primary "
        << host
        << ":"
        << port
        << std::endl;

    int fd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (fd < 0)
    {
        std::cerr
            << "[REPLICATION] "
            << "Failed to create socket"
            << std::endl;

        return false;
    }

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    address.sin_port =
        htons(port);

    if (
        inet_pton(
            AF_INET,
            host.c_str(),
            &address.sin_addr
        ) <= 0
    )
    {
        std::cerr
            << "[REPLICATION] "
            << "Invalid primary address"
            << std::endl;

        close(fd);

        return false;
    }

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
            << "[REPLICATION] "
            << "Failed to connect to primary"
            << std::endl;

        close(fd);

        return false;
    }

    // ==============================================
    // Connection successful
    // ==============================================

    primary_fd =
        fd;

    primary_host =
        host;

    primary_port =
        port;

    role =
        ServerRole::REPLICA;

    // ==============================================
    // Send replication handshake
    // ==============================================

    const std::string handshake =
        "REPLICA_HANDSHAKE\r\n";

    ssize_t sent =
        send(
            primary_fd,
            handshake.c_str(),
            handshake.size(),
            0
        );

    if (sent <= 0)
    {
        std::cerr
            << "[REPLICATION] "
            << "Failed to send handshake"
            << std::endl;

        close(primary_fd);

        primary_fd = -1;

        role =
            ServerRole::PRIMARY;

        return false;
    }

    std::cout
        << "[REPLICATION] "
        << "Handshake sent"
        << std::endl;

    // ==============================================
    // 6. Start receiving replication data
    // ==============================================

    std::cout
        << "[REPLICATION] Starting replication thread..."
        << std::endl;

    replication_thread =
        std::thread(
            &Server::replication_loop,
            this
        );

    replication_thread.detach();

    return true;

    std::cout
        << "[REPLICATION] Replication thread started."
        << std::endl;

    return true;
}

// void Server::replication_loop()
// {
//     char buffer[4096];
//     std::string input_buffer;

//     while (true)
//     {
//         ssize_t bytes_received = recv(
//             primary_fd,
//             buffer,
//             sizeof(buffer),
//             0
//         );

//         if (bytes_received == 0)
//         {
//             std::cout
//                 << "[REPLICATION] Primary disconnected."
//                 << std::endl;
//             break;
//         }

//         if (bytes_received < 0)
//         {
//             std::cerr
//                 << "[REPLICATION] recv() failed."
//                 << std::endl;
//             break;
//         }

//         input_buffer.append(
//             buffer,
//             bytes_received
//         );

//         // TCP can split/combine messages.
//         // Process complete lines only.
//         while (true)
//         {
//             size_t pos =
//                 input_buffer.find("\r\n");

//             if (pos == std::string::npos)
//             {
//                 break;
//             }

//             std::string command =
//                 input_buffer.substr(0, pos);

//             input_buffer.erase(
//                 0,
//                 pos + 2
//             );

//             std::cout
//                 << "[REPLICATION] Applying: "
//                 << command
//                 << std::endl;


//             // ------------------------------------------
//             // Synchronization markers
//             // ------------------------------------------

//             if (command == "SYNC_START")
//             {
//                 continue;
//             }

//             if (command == "SYNC_END")
//             {
//                 continue;
//             }


//             // ------------------------------------------
//             // Execute replication command
//             // ------------------------------------------

//             std::stringstream ss(command);

//             std::string name;
//             ss >> name;

//             if (name == "SET")
//             {
//                 std::string key;
//                 std::string value;

//                 ss >> key;

//                 std::getline(
//                     ss,
//                     value
//                 );

//                 if (!value.empty() &&
//                     value[0] == ' ')
//                 {
//                     value.erase(0, 1);
//                 }

//                 database.set(
//                     key,
//                     value
//                 );
//             }
//         }
//     }
// }

// void Server::replication_loop()
// {
//     std::cout
//         << "[REPLICATION] replication_loop STARTED"
//         << std::endl;

//     char buffer[4096];
//     std::string input_buffer;

//     // Use the same Database owned by this Server
//     CommandHandler command_handler(
//         database,
//         *this
//     );

//     while (true)
//     {
//         ssize_t bytes_received = recv(
//             primary_fd,
//             buffer,
//             sizeof(buffer),
//             0
//         );

//         if (bytes_received == 0)
//         {
//             std::cout
//                 << "[REPLICATION] Primary disconnected."
//                 << std::endl;

//             // handle_primary_failure();

//             break;
//         }

//         if (bytes_received < 0)
//         {
//             std::cerr
//                 << "[REPLICATION] recv() failed."
//                 << std::endl;

//             //handle_primary_failure();

//             break;
//         }

//         input_buffer.append(
//             buffer,
//             bytes_received
//         );

//         // ----------------------------------------------
//         // Extract complete replication messages
//         // ----------------------------------------------

//         while (true)
//         {
//             size_t pos =
//                 input_buffer.find('\n');

//             if (pos == std::string::npos)
//             {
//                 break;
//             }

//             std::string command_text =
//                 input_buffer.substr(
//                     0,
//                     pos
//                 );

//             input_buffer.erase(
//                 0,
//                 pos + 1
//             );

//             // Remove \r if sender uses \r\n
//             if (
//                 !command_text.empty() &&
//                 command_text.back() == '\r'
//             )
//             {
//                 command_text.pop_back();
//             }

//             std::cout
//                 << "[REPLICATION] Received: "
//                 << command_text
//                 << std::endl;


//             // ----------------------------------------------
//             // Synchronization markers
//             // ----------------------------------------------

//             if (command_text == "SYNC_START")
//             {
//                 std::cout
//                     << "[REPLICATION] Sync started"
//                     << std::endl;

//                 continue;
//             }

//             if (command_text == "SYNC_END")
//             {
//                 std::cout
//                     << "[REPLICATION] Sync completed"
//                     << std::endl;

//                 continue;
//             }


//             // ----------------------------------------------
//             // Convert:
//             //
//             // SET name Manab
//             //
//             // into Command
//             // ----------------------------------------------

//             std::stringstream ss(command_text);

//             Command command;

//             ss >> command.name;

//             std::string argument;

//             while (ss >> argument)
//             {
//                 command.arguments.push_back(
//                     argument
//                 );
//             }


//             // ----------------------------------------------
//             // Execute using existing CommandHandler
//             // ----------------------------------------------

//             try
//             {
//                 std::string response =
//                     command_handler.execute(
//                         command
//                     );

//                 std::cout
//                     << "[REPLICATION] Applied: "
//                     << command_text
//                     << std::endl;
//             }
//             catch (const std::exception& e)
//             {
//                 std::cerr
//                     << "[REPLICATION] Failed to apply "
//                     << command_text
//                     << ": "
//                     << e.what()
//                     << std::endl;
//             }
//         }
//     }
// }

void Server::replication_loop()
{
    std::cout
        << "[REPLICATION] replication_loop STARTED"
        << std::endl;

    char buffer[4096];
    std::string input_buffer;

    // This Server's database
    CommandHandler command_handler(
        database,
        *this
    );

    // Continue receiving replication data
    // only while this server is a REPLICA.
    while (role == ServerRole::REPLICA)
    {
        // ==================================================
        // 1. Receive data from primary
        // ==================================================

        ssize_t bytes_received =
            recv(
                primary_fd,
                buffer,
                sizeof(buffer),
                0
            );

        // ==================================================
        // 2. Primary disconnected
        // ==================================================

        if (bytes_received == 0)
        {
            std::cout
                << "[REPLICATION] "
                << "Primary disconnected."
                << std::endl;

            handle_primary_failure();

            return;
        }

        // ==================================================
        // 3. recv() error
        // ==================================================

        if (bytes_received < 0)
        {
            std::cerr
                << "[REPLICATION] "
                << "recv() failed."
                << std::endl;

            handle_primary_failure();

            return;
        }

        // ==================================================
        // 4. Append received TCP data
        // ==================================================

        input_buffer.append(
            buffer,
            bytes_received
        );

        // ==================================================
        // 5. Process complete commands
        // ==================================================

        while (true)
        {
            size_t pos =
                input_buffer.find('\n');

            // We don't have a complete command yet.
            if (pos == std::string::npos)
            {
                break;
            }

            // ==================================================
            // Extract one command
            // ==================================================

            std::string command_text =
                input_buffer.substr(
                    0,
                    pos
                );

            // Remove processed command
            input_buffer.erase(
                0,
                pos + 1
            );

            // Remove '\r' from "\r\n"
            if (
                !command_text.empty() &&
                command_text.back() == '\r'
            )
            {
                command_text.pop_back();
            }

            std::cout
                << "[REPLICATION] Received: "
                << command_text
                << std::endl;

            // ==================================================
            // 6. Synchronization markers
            // ==================================================

            if (command_text == "SYNC_START")
            {
                std::cout
                    << "[REPLICATION] "
                    << "Sync started"
                    << std::endl;

                continue;
            }

            if (command_text == "SYNC_END")
            {
                std::cout
                    << "[REPLICATION] "
                    << "Sync completed"
                    << std::endl;

                continue;
            }

            // ==================================================
            // 7. Convert command text into Command
            // ==================================================

            std::stringstream ss(
                command_text
            );

            Command command;

            // First word = command name
            //
            // SET name Manab
            // ^
            // command.name

            ss >> command.name;

            // Remaining words = arguments

            std::string argument;

            while (ss >> argument)
            {
                command.arguments.push_back(
                    argument
                );
            }

            // ==================================================
            // 8. Apply command to replica database
            // ==================================================

            try
            {
                std::string response =
                    command_handler.execute_replication(
                        command
                    );

                std::cout
                    << "[REPLICATION] Applied: "
                    << command_text
                    << std::endl;
            }
            catch (
                const std::exception& e
            )
            {
                std::cerr
                    << "[REPLICATION] "
                    << "Failed to apply "
                    << command_text
                    << ": "
                    << e.what()
                    << std::endl;
            }
        }
    }

    std::cout
        << "[REPLICATION] "
        << "Replication loop stopped."
        << std::endl;
}

void Server::handle_primary_failure()
{
    {
        std::lock_guard<std::mutex> lock(
            election_state_mutex
        );

        if (election_in_progress)
        {
            std::cout
                << "[ELECTION] Election already in progress"
                << std::endl;

            return;
        }

        election_in_progress = true;
    }

    // The mutex is RELEASED here.

    std::cout
        << "[FAILOVER] Primary failure detected"
        << std::endl;

    if (primary_fd != -1)
    {
        close(primary_fd);
        primary_fd = -1;
    }

    primary_host.clear();
    primary_port = -1;

    std::cout
        << "[FAILOVER] Starting election manager"
        << std::endl;

    election_manager.start_election();

    std::cout
        << "[FAILOVER] Election manager returned"
        << std::endl;
}

void Server::promote_to_primary()
{
    if (role == ServerRole::PRIMARY)
    {
        return;
    }

    std::cout
        << "[FAILOVER] Promoting replica "
        << "to PRIMARY"
        << std::endl;

    // ------------------------------------------
    // Stop using old primary connection
    // ------------------------------------------

    if (primary_fd != -1)
    {
        close(primary_fd);
        primary_fd = -1;
    }

    primary_host.clear();
    primary_port = -1;

    // ------------------------------------------
    // Change role
    // ------------------------------------------

    role = ServerRole::PRIMARY;

    std::cout
        << "[FAILOVER] Server is now PRIMARY"
        << std::endl;
}

bool Server::handle_vote_request(
    int term,
    int candidate_id
)
{
    std::lock_guard<std::mutex> lock(
        election_state_mutex
    );

    std::cout
        << "[ELECTION] Vote request received"
        << " | term = "
        << term
        << " | candidate = "
        << candidate_id
        << std::endl;

    // ==================================================
    // 1. Candidate is using an old election term
    // ==================================================

    if (term < current_term)
    {
        std::cout
            << "[ELECTION] Rejecting vote: "
            << "old term"
            << std::endl;

        return false;
    }

    // ==================================================
    // 2. Candidate has a newer term
    // ==================================================

    if (term > current_term)
    {
        current_term = term;

        // We haven't voted in this new term yet.
        voted_for = -1;

        // A newer election term means we should
        // follow the new election.
        role = ServerRole::REPLICA;

        std::cout
            << "[ELECTION] Updated term to "
            << current_term
            << std::endl;
    }

    // ==================================================
    // 3. Already voted for another candidate
    // ==================================================

    if (
        voted_for != -1 &&
        voted_for != candidate_id
    )
    {
        std::cout
            << "[ELECTION] Rejecting vote: "
            << "already voted for "
            << voted_for
            << std::endl;

        return false;
    }

    // ==================================================
    // 4. Grant vote
    // ==================================================

    voted_for = candidate_id;

    std::cout
        << "[ELECTION] Vote GRANTED to "
        << candidate_id
        << " for term "
        << current_term
        << std::endl;

    return true;
}

int Server::get_server_id() const
{
    return server_id;
}

int Server::get_current_term() const
{
    std::lock_guard<std::mutex> lock(
        election_state_mutex
    );

    return current_term;
}

std::vector<int> Server::get_peer_ports() const
{
    // std::lock_guard<std::mutex> lock(
    //     election_state_mutex
    // );

    std::cout
        << "[ELECTION] get_peer_ports(): "
        << peer_ports.size()
        << std::endl;

    for (int port : peer_ports)
    {
        std::cout
            << "[ELECTION] Stored peer: "
            << port
            << std::endl;
    }

    return peer_ports;
}

void Server::add_peer(int peer_port)
{
    if (peer_port == server_id)
    {
        return;
    }

    peer_ports.push_back(peer_port);

    std::cout
        << "[ELECTION] Peer added: "
        << peer_port
        << std::endl;

    std::cout
        << "[ELECTION] Total peers: "
        << peer_ports.size()
        << std::endl;
}

void Server::begin_election()
{
    std::lock_guard<std::mutex> lock(
        election_state_mutex
    );

    role =
        ServerRole::CANDIDATE;

    current_term++;

    voted_for =
        server_id;

    votes_received.clear();

    // Vote for ourselves
    votes_received.insert(
        server_id
    );

    std::cout
        << "[ELECTION] Server "
        << server_id
        << " became CANDIDATE"
        << std::endl;

    std::cout
        << "[ELECTION] Term = "
        << current_term
        << std::endl;

    std::cout
        << "[ELECTION] Self vote recorded"
        << std::endl;
}

void Server::record_vote(
    int voter_id
)
{
    bool should_promote = false;

    {
        std::lock_guard<std::mutex> lock(
            election_state_mutex
        );

        // We only count votes while we are a candidate
        if (role != ServerRole::CANDIDATE)
        {
            std::cout
                << "[ELECTION] Ignoring vote because "
                << "server is not a candidate"
                << std::endl;

            return;
        }

        // Insert voter.
        // std::set automatically prevents duplicate votes.
        votes_received.insert(
            voter_id
        );

        // ==============================================
        // Calculate majority
        // ==============================================

        int total_servers =
            static_cast<int>(
                peer_ports.size()
            ) + 1;

        int majority =
            total_servers / 2 + 1;

        std::cout
            << "[ELECTION] Vote received from "
            << voter_id
            << std::endl;

        std::cout
            << "[ELECTION] Votes: "
            << votes_received.size()
            << "/"
            << majority
            << std::endl;

        // ==============================================
        // Did we get majority?
        // ==============================================

        if (
            votes_received.size()
            >= static_cast<size_t>(majority)
        )
        {
            should_promote = true;
        }
    }

    // ==============================================
    // Promote outside the mutex
    // ==============================================

    if (should_promote)
    {
        std::cout
            << "[ELECTION] MAJORITY ACHIEVED"
            << std::endl;

        std::cout
            << "[ELECTION] Server "
            << server_id
            << " won the election"
            << std::endl;

        promote_to_primary();

        {
            std::lock_guard<std::mutex> lock(
                election_state_mutex
            );

            election_in_progress = false;
        }
    }
}