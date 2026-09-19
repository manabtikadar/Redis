#include "client_handler.h"

#include "resp_parser.h"
#include "command_handler.h"
#include "database.h"
#include "replication_manager.h"
#include "server.h"

#include <iostream>
#include <string>

#include <sys/socket.h>
#include <unistd.h>

using namespace std;


void handle_client(
    int client_fd,
    Database& database,
    ReplicationManager& replication_manager,
    Server& server
)
{
    cout << "Client connected!" << endl;

    char buffer[1024];

    string input_buffer;

    RespParser parser;

    CommandHandler command_handler(
        database,
        server
    );

    // ==================================================
    // Track whether this connection became a replica
    // ==================================================

    bool is_replica = false;

    while (true)
    {
        // ==================================================
        // 1. Receive data
        // ==================================================

        int bytes_received =
            recv(
                client_fd,
                buffer,
                sizeof(buffer),
                0
            );

        // --------------------------------------------------
        // Client disconnected
        // --------------------------------------------------

        if (bytes_received == 0)
        {
            cout
                << "Client disconnected."
                << endl;

            break;
        }

        // --------------------------------------------------
        // recv() error
        // --------------------------------------------------

        if (bytes_received == -1)
        {
            cerr
                << "recv() failed"
                << endl;

            break;
        }

        // ==================================================
        // 2. Add received data to input buffer
        // ==================================================

        input_buffer.append(
            buffer,
            bytes_received
        );


        // ==================================================
        // 3. Check for replication handshake
        // ==================================================

        const string handshake =
            "REPLICA_HANDSHAKE\r\n";

        if (
            input_buffer.find(handshake) == 0
        )
        {
            cout
                << "[REPLICATION] "
                << "Replica handshake received."
                << endl;


            // --------------------------------------------------
            // Mark this connection as a replica
            // --------------------------------------------------

            is_replica = true;


            // --------------------------------------------------
            // Add replica socket to ReplicationManager
            // --------------------------------------------------

            replication_manager.add_replica(
                client_fd
            );


            // --------------------------------------------------
            // Send complete database to replica
            // --------------------------------------------------

            replication_manager.initial_sync(
                client_fd,
                database
            );


            // --------------------------------------------------
            // Remove handshake from input buffer
            // --------------------------------------------------

            input_buffer.erase(
                0,
                handshake.size()
            );


            cout
                << "[REPLICATION] "
                << "Initial synchronization completed."
                << endl;


            // --------------------------------------------------
            // IMPORTANT
            //
            // Do NOT process this connection as a
            // normal Redis client.
            //
            // The replication manager now owns this socket.
            // --------------------------------------------------

            return;
        }


        // ==================================================
        // 4. Normal Redis client processing
        // ==================================================

        while (!input_buffer.empty())
        {
            try
            {
                // --------------------------------------------------
                // Parse RESP
                // --------------------------------------------------

                ParseResult result =
                    parser.parse(
                        input_buffer
                    );


                // --------------------------------------------------
                // Incomplete command
                // --------------------------------------------------

                if (!result.complete)
                {
                    break;
                }


                // --------------------------------------------------
                // Empty command
                // --------------------------------------------------

                if (
                    result.value.elements.empty()
                )
                {
                    break;
                }


                // ==================================================
                // 5. RESP → Command
                // ==================================================

                Command command;

                command.name =
                    result.value.elements[0];

                for (
                    size_t i = 1;
                    i < result.value.elements.size();
                    i++
                )
                {
                    command.arguments.push_back(
                        result.value.elements[i]
                    );
                }


                // ==================================================
                // 6. Execute command
                // ==================================================

                string response =
                    command_handler.execute(
                        command
                    );


                // ==================================================
                // 7. Send response to client
                // ==================================================

                ssize_t bytes_sent =
                    send(
                        client_fd,
                        response.c_str(),
                        response.size(),
                        0
                    );

                if (bytes_sent == -1)
                {
                    cerr
                        << "send() failed"
                        << endl;

                    break;
                }


                // ==================================================
                // 8. Remove processed command
                // ==================================================

                input_buffer.erase(
                    0,
                    result.consumed
                );
            }
            catch (
                const exception& e
            )
            {
                cerr
                    << "Parser error: "
                    << e.what()
                    << endl;


                string response =
                    "-ERR invalid request\r\n";


                send(
                    client_fd,
                    response.c_str(),
                    response.size(),
                    0
                );


                input_buffer.clear();

                break;
            }
        }
    }


    // ==================================================
    // 9. Connection cleanup
    // ==================================================

    // IMPORTANT:
    //
    // If this was a replica connection, DO NOT close()
    // the socket here.
    //
    // ReplicationManager is keeping this socket open
    // for future replication.
    //
    // Normal clients still need to be closed.
    // ==================================================

    if (!is_replica)
    {
        close(client_fd);

        cout
            << "Client handler finished."
            << endl;
    }
    else
    {
        cout
            << "[REPLICATION] "
            << "Replication socket transferred to "
            << "ReplicationManager."
            << endl;
    }
}