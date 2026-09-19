// #ifndef COMMAND_HANDLER_H
// #define COMMAND_HANDLER_H

// #include <string>

// #include "command.h"
// #include "database.h"

// using namespace std;

// class CommandHandler
// {
// public:
//     CommandHandler(Database& database);

//     string execute(const Command& command);

// private:
//     Database& database;
// };

// #endif

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include <unordered_map>
#include <functional>

#include "command.h"
#include "database.h"

using namespace std;

class Server;

class CommandHandler
{
public:

    using CommandFunction =
        std::function<string(const Command&)>;

    // Shared database
    Database& database;

    Server& server;

    // Command registry
    unordered_map<
        string,
        CommandFunction
    > commands;

    // Register all supported commands
    void register_commands();

    // Individual command handlers
    string handle_ping(
        const Command& command
    );

    string handle_set(
        const Command& command
    );

    string handle_get(
        const Command& command
    );

    string handle_del(
        const Command& command
    );

    std::string handle_lpush(
        const Command& command
    );

    std::string handle_rpush(
        const Command& command
    );

    std::string handle_lpop(
        const Command& command
    );

    std::string handle_rpop(
        const Command& command
    );

    std::string handle_lrange(
        const Command& command
    );

    std::string handle_hset(
        const Command& command
    );

    std::string handle_hget(
        const Command& command
    );

    std::string handle_hdel(
        const Command& command
    );

    std::string handle_hexists(
        const Command& command
    );

    std::string handle_hgetall(
        const Command& command
    );

    std::string handle_expire(
        const Command& command
    );

    std::string handle_ttl(
        const Command& command
    );

    std::string handle_sadd(
        const Command& command
    );

    std::string handle_srem(
        const Command& command
    );

    std::string handle_sismember(
        const Command& command
    );

    std::string handle_scard(
        const Command& command
    );

    std::string handle_smembers(
        const Command& command
    );

    std::string handle_zadd(
        const Command& command
    );

    std::string handle_zscore(
        const Command& command
    );

    std::string handle_zrem(
        const Command& command
    );

    std::string handle_zcard(
        const Command& command
    );

    std::string handle_zrange(
        const Command& command
    );

    std::string handle_save(
        const Command& command
    );

    std::string handle_config(
        const Command& command
    );

    std::string handle_promote(
        const Command& command
    );

public:

    CommandHandler(
        Database& database,
        Server& server
    );

    std::string handle_replicaof(
        const Command& command
    );

    string execute(
        const Command& command
    );

    std::string execute_replication(
        const Command& command
    );

    bool is_write_command(
        const std::string& command
    );
};

#endif