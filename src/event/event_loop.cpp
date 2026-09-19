#include "event_loop.h"

#include <iostream>
#include <cerrno>
#include <cstring>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>


// --------------------------------------------------
// Make socket non-blocking
// --------------------------------------------------

static void make_non_blocking(int fd)
{
    int flags = fcntl(
        fd,
        F_GETFL,
        0
    );

    if (flags == -1)
    {
        std::cerr
            << "fcntl(F_GETFL) failed: "
            << strerror(errno)
            << std::endl;

        return;
    }

    if (
        fcntl(
            fd,
            F_SETFL,
            flags | O_NONBLOCK
        ) == -1
    )
    {
        std::cerr
            << "fcntl(F_SETFL) failed: "
            << strerror(errno)
            << std::endl;
    }
}


// --------------------------------------------------
// Constructor
// --------------------------------------------------

EventLoop::EventLoop(int server_fd)
    : server_fd(server_fd)
{
    // ----------------------------------------------
    // 1. Create epoll instance
    // ----------------------------------------------

    epoll_fd =
        epoll_create1(0);

    if (epoll_fd == -1)
    {
        throw std::runtime_error(
            "epoll_create1() failed"
        );
    }


    // ----------------------------------------------
    // 2. Make listening socket non-blocking
    // ----------------------------------------------

    make_non_blocking(
        server_fd
    );


    // ----------------------------------------------
    // 3. Register server socket with epoll
    // ----------------------------------------------

    epoll_event event{};

    event.events =
        EPOLLIN;

    event.data.fd =
        server_fd;

    if (
        epoll_ctl(
            epoll_fd,
            EPOLL_CTL_ADD,
            server_fd,
            &event
        ) == -1
    )
    {
        throw std::runtime_error(
            "epoll_ctl() failed for server socket"
        );
    }

    std::cout
        << "[EPOLL] Event loop initialized."
        << std::endl;
}


// --------------------------------------------------
// Destructor
// --------------------------------------------------

EventLoop::~EventLoop()
{
    close(epoll_fd);
}


// --------------------------------------------------
// Event loop
// --------------------------------------------------

void EventLoop::run()
{
    constexpr int MAX_EVENTS = 1024;

    epoll_event events[MAX_EVENTS];


    while (true)
    {
        // ------------------------------------------
        // Wait for events
        // ------------------------------------------

        int event_count =
            epoll_wait(
                epoll_fd,
                events,
                MAX_EVENTS,
                -1
            );


        if (event_count == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            std::cerr
                << "[EPOLL] epoll_wait() failed: "
                << strerror(errno)
                << std::endl;

            break;
        }


        // ------------------------------------------
        // Process returned events
        // ------------------------------------------

        for (
            int i = 0;
            i < event_count;
            i++
        )
        {
            int fd =
                events[i].data.fd;


            // ======================================
            // New client connection
            // ======================================

            if (fd == server_fd)
            {
                while (true)
                {
                    int client_fd =
                        accept(
                            server_fd,
                            nullptr,
                            nullptr
                        );

                    if (client_fd == -1)
                    {
                        if (
                            errno == EAGAIN ||
                            errno == EWOULDBLOCK
                        )
                        {
                            break;
                        }

                        std::cerr
                            << "[EPOLL] accept() failed: "
                            << strerror(errno)
                            << std::endl;

                        break;
                    }


                    // ------------------------------
                    // Make client non-blocking
                    // ------------------------------

                    make_non_blocking(
                        client_fd
                    );


                    // ------------------------------
                    // Register client with epoll
                    // ------------------------------

                    epoll_event client_event{};

                    client_event.events =
                        EPOLLIN;

                    client_event.data.fd =
                        client_fd;

                    if (
                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_ADD,
                            client_fd,
                            &client_event
                        ) == -1
                    )
                    {
                        std::cerr
                            << "[EPOLL] Failed to "
                            << "register client."
                            << std::endl;

                        close(client_fd);

                        continue;
                    }


                    clients[client_fd] =
                        client_fd;

                    std::cout
                        << "[EPOLL] Client connected. "
                        << "fd = "
                        << client_fd
                        << std::endl;
                }
            }


            // ======================================
            // Existing client has data
            // ======================================

            else
            {
                if (
                    events[i].events &
                    EPOLLIN
                )
                {
                    char buffer[4096];

                    ssize_t bytes =
                        recv(
                            fd,
                            buffer,
                            sizeof(buffer),
                            0
                        );


                    // ------------------------------
                    // Client disconnected
                    // ------------------------------

                    if (bytes == 0)
                    {
                        std::cout
                            << "[EPOLL] Client "
                            << fd
                            << " disconnected."
                            << std::endl;

                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_DEL,
                            fd,
                            nullptr
                        );

                        close(fd);

                        clients.erase(fd);

                        continue;
                    }


                    // ------------------------------
                    // Error
                    // ------------------------------

                    if (bytes == -1)
                    {
                        if (
                            errno == EAGAIN ||
                            errno == EWOULDBLOCK
                        )
                        {
                            continue;
                        }

                        std::cerr
                            << "[EPOLL] recv() failed: "
                            << strerror(errno)
                            << std::endl;

                        close(fd);

                        clients.erase(fd);

                        continue;
                    }


                    // ------------------------------
                    // Data received
                    // ------------------------------

                    std::cout
                        << "[EPOLL] Received from fd "
                        << fd
                        << ": ";

                    std::cout.write(
                        buffer,
                        bytes
                    );

                    std::cout
                        << std::endl;
                }
            }
        }
    }
}