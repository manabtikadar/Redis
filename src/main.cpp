// #include <iostream>
// #include <cstring>
// #include <thread>

// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <unistd.h>
// #include <cerrno>

// using namespace std;

// void handle_client(int client_fd)
// {
//     cout << "Client connected!" << endl;

//     char buffer[1024];

//     while (true)
//     {
//         int bytes_received = recv(
//             client_fd,
//             buffer,
//             sizeof(buffer) - 1,
//             0
//         );

//         if (bytes_received == 0)
//         {
//             cout << "Client disconnected." << endl;
//             break;
//         }

//         if (bytes_received == -1)
//         {
//             cerr << "recv() failed" << endl;
//             break;
//         }

//         buffer[bytes_received] = '\0';

//         cout << "Received: " << buffer << endl;

//         const char* response = "+OK\r\n";

//         send(
//             client_fd,
//             response,
//             strlen(response),
//             0
//         );
//     }

//     close(client_fd);

//     cout << "Client handler finished." << endl;
// }

// int main()
// {
//     cout << "RediForge server starting..." << endl;

//     // --------------------------------------------------
//     // 1. Prepare the address information
//     // --------------------------------------------------

//     // struct addrinfo {
//     //     int              ai_flags;      // options/flags
//     //     int              ai_family;     // IPv4, IPv6, or either
//     //     int              ai_socktype;   // TCP or UDP
//     //     int              ai_protocol;   // protocol
//     //     size_t           ai_addrlen;    // size of address
//     //     struct sockaddr *ai_addr;       // actual IP + port
//     //     char            *ai_canonname;  // canonical hostname
//     //     struct addrinfo *ai_next;       // next result
//     // };

//     struct addrinfo hints{};
//     struct addrinfo* serverInfo;

//     hints.ai_family = AF_INET;        // IPv4
//     hints.ai_socktype = SOCK_STREAM;  // TCP
//     hints.ai_flags = AI_PASSIVE;      // For wildcard IP address

//     int status = getaddrinfo(
//         nullptr,        // hostname (nullptr for localhost)
//         "6379",         // redis port
//         &hints,
//         &serverInfo
//     );

//     if (status != 0) {
//         cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
//         return 1;
//     }

//     // --------------------------------------------------
//     // 2. Create the socket
//     // --------------------------------------------------

//     int server_fd = socket(
//         serverInfo->ai_family,
//         serverInfo->ai_socktype,
//         serverInfo->ai_protocol
//     );

//     if (server_fd == -1) {
//         cerr << "socket error: " << strerror(errno) << endl;
//         freeaddrinfo(serverInfo);
//         return 1;
//     }

//     // --------------------------------------------------
//     // 3. Bind socket to port 6379
//     // --------------------------------------------------

//     if (bind(
//         server_fd,
//         serverInfo->ai_addr,
//         serverInfo->ai_addrlen
//     ) == -1 )
//     {
//         cerr << "bind() failed" << endl;
//         close(server_fd);
//         freeaddrinfo(serverInfo);
//         return 1;
//     }
    
//     // We don't need the address information anymore
//     freeaddrinfo(serverInfo);

//     // --------------------------------------------------
//     // 4. Start listening
//     // --------------------------------------------------

//     if (listen(server_fd, 10) == -1)
//     {
//         cerr << "listen() failed" << endl;

//         close(server_fd);

//         return 1;
//     }

//     cout << "RediForge is listening on port 6379..." << endl;

//     // --------------------------------------------------
//     // 5. Keep the server running
//     // --------------------------------------------------

//     while (true)
//     {
//         cout << "Waiting for client..." << endl;

//         int client_fd = accept(server_fd, nullptr, nullptr);

//         if(client_fd == -1)
//         {
//             cerr << "accept() failed" << endl;
//             continue;
//         }

//         thread client_thread(
//             handle_client, 
//             client_fd
//         );

//         client_thread.detach();
//     }

//     close(server_fd);

//     return 0;
// }

// #include "server.h"

// #include <iostream>

// int main(int argc, char* argv[])
// {
//     int port = 6379;

//     // If a port is provided:
//     // ./rediforge 6380
//     if (argc >= 2)
//     {
//         try
//         {
//             port = std::stoi(argv[1]);
//         }
//         catch (...)
//         {
//             std::cerr
//                 << "Invalid port: "
//                 << argv[1]
//                 << std::endl;

//             return 1;
//         }
//     }

//     std::cout
//         << "Starting RediForge on port "
//         << port
//         << std::endl;

//     Server server(port);

//     // ==============================================
//     // Configure cluster peers
//     // ==============================================

//     if (port == 6379)
//     {
//         server.add_peer(6380);
//         server.add_peer(6381);
//     }
//     else if (port == 6380)
//     {
//         server.add_peer(6379);
//         server.add_peer(6381);
//     }
//     else if (port == 6381)
//     {
//         server.add_peer(6379);
//         server.add_peer(6380);
//     }

//     server.start();

//     return 0;
// }

#include "server.h"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr
            << "Usage: ./rediforge <port> [peer_port ...]"
            << std::endl;

        return 1;
    }

    int port;

    try
    {
        port = std::stoi(argv[1]);
    }
    catch (...)
    {
        std::cerr
            << "Invalid port: "
            << argv[1]
            << std::endl;

        return 1;
    }

    std::cout
        << "Starting RediForge on port "
        << port
        << std::endl;

    Server server(port);

    // ==============================================
    // Add peers from command line
    // ==============================================

    for (int i = 2; i < argc; i++)
    {
        try
        {
            int peer_port =
                std::stoi(argv[i]);

            server.add_peer(peer_port);
        }
        catch (...)
        {
            std::cerr
                << "Invalid peer port: "
                << argv[i]
                << std::endl;

            return 1;
        }
    }

    server.start();

    return 0;
}