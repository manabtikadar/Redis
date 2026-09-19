#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <unordered_map>

class EventLoop
{
private:

    int epoll_fd; // descriptor representing our epoll instance

    int server_fd; // existing listining socket

    std::unordered_map<int, int> clients;

public:

    EventLoop(int server_fd);

    ~EventLoop();

    void run();
};

#endif
