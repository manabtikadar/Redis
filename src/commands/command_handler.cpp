#include "command_handler.h"
#include "server.h"

#include <algorithm>
#include <cctype>
#include <vector>

// --------------------------------------------------
// Constructor
// --------------------------------------------------

CommandHandler::CommandHandler(
    Database& database,
    Server& server
)
    : database(database),
      server(server)
{
    register_commands();
}

// --------------------------------------------------
// Register Commands
// --------------------------------------------------

void CommandHandler::register_commands()
{
    commands["PING"] =
        [this](const Command& command)
        {
            return handle_ping(command);
        };

    commands["SET"] =
        [this](const Command& command)
        {
            return handle_set(command);
        };

    commands["GET"] =
        [this](const Command& command)
        {
            return handle_get(command);
        };

    commands["DEL"] =
        [this](const Command& command)
        {
            return handle_del(command);
        };
    
    commands["LPUSH"] =
    [this](const Command& command)
    {
        return handle_lpush(command);
    };

    commands["RPUSH"] =
    [this](const Command& command)
    {
        return handle_rpush(command);
    };

    commands["LPOP"] =
        [this](const Command& command)
        {
            return handle_lpop(command);
        };

    commands["RPOP"] =
        [this](const Command& command)
        {
            return handle_rpop(command);
        };

    commands["LRANGE"] =
        [this](const Command& command)
        {
            return handle_lrange(command);
        };

    commands["HSET"] =
        [this](const Command& command)
        {
            return handle_hset(command);
        };
    
    commands["HGET"] =
        [this](const Command& command)
        { 
            return handle_hget(command);
        };  
    
    commands["HDEL"] =
        [this](const Command& command)
        {
            return handle_hdel(command);
        };  
    
    commands["HGETALL"] =
        [this](const Command& command)
        {
            return handle_hgetall(command);
        };

    commands["HEXISTS"] =
        [this](const Command& command)
        {
            return handle_hexists(command);
        };

    commands["EXPIRE"] =
        [this](const Command& command)
        {
            return handle_expire(command);
        };

    commands["TTL"] =
        [this](const Command& command)
        {
            return handle_ttl(command);
        };

    commands["SADD"] =
        [this](const Command& command)
        {
            return handle_sadd(command);
        };

    commands["SREM"] =
        [this](const Command& command)
        {
            return handle_srem(command);
        };

    commands["SISMEMBER"] =
        [this](const Command& command)
        {
            return handle_sismember(command);
        };

    commands["SCARD"] =
        [this](const Command& command)
        {
            return handle_scard(command);
        };

    commands["SMEMBERS"] =
        [this](const Command& command)
        {
            return handle_smembers(command);
        };

    commands["ZADD"] =
        [this](const Command& command)
        {
            return handle_zadd(command);
        };

    commands["ZSCORE"] =
        [this](const Command& command)
        {
            return handle_zscore(command);
        };

    commands["ZREM"] =
        [this](const Command& command)
        {
            return handle_zrem(command);
        };

    commands["ZCARD"] =
        [this](const Command& command)
        {
            return handle_zcard(command);
        };

    commands["ZRANGE"] =
        [this](const Command& command)
        {
            return handle_zrange(command);
        };

    commands["SAVE"] =
        [this](const Command& command)
        {
            return handle_save(command);
        };
    
    commands["CONFIG"] =
        [this](const Command& commands)
        {
            return handle_config(commands);
        };
    
    commands["REPLICAOF"] =
        [this](const Command& command)
        {
            return handle_replicaof(command);
        };

    commands["PROMOTE"] =
        [this](const Command& command)
        {
            return handle_promote(command);
        };
}

bool CommandHandler::is_write_command(
    const std::string& command
)
{
    return
        command == "SET"    ||
        command == "DEL"    ||

        command == "LPUSH"  ||
        command == "RPUSH"  ||
        command == "LPOP"   ||
        command == "RPOP"   ||

        command == "HSET"   ||
        command == "HDEL"   ||

        command == "SADD"   ||
        command == "SREM"   ||

        command == "ZADD"   ||
        command == "ZREM"   ||

        command == "EXPIRE";
}

// --------------------------------------------------
// Execute Command
// --------------------------------------------------

std::string CommandHandler::execute(
    const Command& command
)
{
    if (
        server.get_role()
        == ServerRole::REPLICA
    )
    {
        if (is_write_command(command.name))
        {
            return
                "-READONLY You can't write "
                "to a replica\r\n";
        }
    }

    std::string command_name = command.name;

    // Convert command name to uppercase
    std::transform(
        command_name.begin(),
        command_name.end(),
        command_name.begin(),
        [](unsigned char c)
        {
            return std::toupper(c);
        }
    );

    // Find command in registry
    auto it = commands.find(command_name);

    // Command doesn't exist
    if (it == commands.end())
    {
        return "-ERR unknown command\r\n";
    }

    // Execute registered command
    return it->second(command);
}

std::string CommandHandler::execute_replication(
    const Command& command
)
{
    std::string command_name = command.name;

    // Convert command name to uppercase
    std::transform(
        command_name.begin(),
        command_name.end(),
        command_name.begin(),
        [](unsigned char c)
        {
            return std::toupper(c);
        }
    );

    // Find command in registry
    auto it = commands.find(command_name);

    // Command doesn't exist
    if (it == commands.end())
    {
        return "-ERR unknown command\r\n";
    }

    // Execute registered command
    return it->second(command);
}

// --------------------------------------------------
// PING
// --------------------------------------------------

std::string CommandHandler::handle_ping(
    const Command& command
)
{
    if (!command.arguments.empty())
    {
        return "-ERR wrong number of arguments for PING\r\n";
    }

    return "+PONG\r\n";
}

// --------------------------------------------------
// SET
// --------------------------------------------------

// std::string CommandHandler::handle_set(
//     const Command& command
// )
// {
//     if (command.arguments.size() != 2)
//     {
//         return "-ERR wrong number of arguments for SET\r\n";
//     }

//     const std::string& key =
//         command.arguments[0];

//     const std::string& value =
//         command.arguments[1];

//     database.set(key, value);

//     return "+OK\r\n";
// }

std::string CommandHandler::handle_set(
    const Command& command
)
{
    // Minimum:
    // SET key value
    if (command.arguments.size() != 2 &&
        command.arguments.size() != 4)
    {
        return "-ERR wrong number of arguments for SET\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& value =
        command.arguments[1];

    long long expiration_seconds = -1;

    // ---------------------------------------------
    // SET key value EX seconds
    // ---------------------------------------------

    if (command.arguments.size() == 4)
    {
        std::string option =
            command.arguments[2];

        // Convert EX to uppercase
        std::transform(
            option.begin(),
            option.end(),
            option.begin(),
            [](unsigned char c)
            {
                return std::toupper(c);
            }
        );

        if (option != "EX")
        {
            return "-ERR syntax error\r\n";
        }

        try
        {
            expiration_seconds =
                std::stoll(
                    command.arguments[3]
                );
        }
        catch (...)
        {
            return "-ERR invalid expire time\r\n";
        }

        if (expiration_seconds < 0)
        {
            return "-ERR invalid expire time\r\n";
        }
    }

    database.set(
        key,
        value,
        expiration_seconds
    );

    return "+OK\r\n";
}

// --------------------------------------------------
// GET
// --------------------------------------------------

std::string CommandHandler::handle_get(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for GET\r\n";
    }

    const std::string& key =
        command.arguments[0];

    std::string value;

    bool found =
        database.get(key, value);

    // Key doesn't exist
    if (!found)
    {
        return "$-1\r\n";
    }

    // RESP Bulk String
    return "$" +
           std::to_string(value.size()) +
           "\r\n" +
           value +
           "\r\n";
}

// --------------------------------------------------
// DEL
// --------------------------------------------------

std::string CommandHandler::handle_del(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for DEL\r\n";
    }

    bool deleted =
        database.del(command.arguments[0]);

    if (deleted)
    {
        return ":1\r\n";
    }

    return ":0\r\n";
}

std::string CommandHandler::handle_lpush(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for LPUSH\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& value =
        command.arguments[1];

    int length =
        database.lpush(key, value);

    if (length == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(length) +
           "\r\n";
}

std::string CommandHandler::handle_rpush(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for RPUSH\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& value =
        command.arguments[1];

    int length =
        database.rpush(key, value);

    if (length == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(length) +
           "\r\n";
}

std::string CommandHandler::handle_lpop(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for LPOP\r\n";
    }

    std::string value;

    bool success =
        database.lpop(
            command.arguments[0],
            value
        );

    if (!success)
    {
        return "$-1\r\n";
    }

    return "$" +
           std::to_string(value.size()) +
           "\r\n" +
           value +
           "\r\n";
}

std::string CommandHandler::handle_rpop(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for RPOP\r\n";
    }

    std::string value;

    bool success =
        database.rpop(
            command.arguments[0],
            value
        );

    if (!success)
    {
        return "$-1\r\n";
    }

    return "$" +
           std::to_string(value.size()) +
           "\r\n" +
           value +
           "\r\n";
}

std::string CommandHandler::handle_lrange(
    const Command& command
)
{
    if (command.arguments.size() != 3)
    {
        return "-ERR wrong number of arguments for LRANGE\r\n";
    }

    const std::string& key =
        command.arguments[0];

    int start;
    int stop;

    try
    {
        start = std::stoi(
            command.arguments[1]
        );

        stop = std::stoi(
            command.arguments[2]
        );
    }
    catch (...)
    {
        return "-ERR value is not an integer\r\n";
    }

    std::vector<std::string> values;

    bool success =
        database.lrange(
            key,
            start,
            stop,
            values
        );

    if (!success)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    // --------------------------------------------------
    // RESP Array
    // --------------------------------------------------

    std::string response =
        "*" +
        std::to_string(values.size()) +
        "\r\n";

    for (const std::string& value : values)
    {
        response +=
            "$" +
            std::to_string(value.size()) +
            "\r\n" +
            value +
            "\r\n";
    }

    return response;
}

// --------------------------------------------------
// HSET
// --------------------------------------------------
std::string CommandHandler::handle_hset(
    const Command& command
)
{
    if (command.arguments.size() != 3)
    {
        return "-ERR wrong number of arguments for HSET\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& field =
        command.arguments[1];

    const std::string& value =
        command.arguments[2];

    bool success =
        database.hset(
            key,
            field,
            value
        );

    if (!success)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":1\r\n";
}

std::string CommandHandler::handle_hget(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for HGET\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& field =
        command.arguments[1];

    std::string value;

    bool success =
        database.hget(
            key,
            field,
            value
        );

    if (!success)
    {
        return "$-1\r\n";
    }

    return "$" +
           std::to_string(value.size()) +
           "\r\n" +
           value +
           "\r\n";
}

std::string CommandHandler::handle_hdel(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for HDEL\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& field =
        command.arguments[1];

    bool success =
        database.hdel(
            key,
            field
        );

    if (!success)
    {
        return ":0\r\n";
    }

    return ":1\r\n";
}

std::string CommandHandler::handle_hexists(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for HEXISTS\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& field =
        command.arguments[1];

    bool exists =
        database.hexists(
            key,
            field
        );

    if (exists)
    {
        return ":1\r\n";
    }

    return ":0\r\n";
}

std::string CommandHandler::handle_hgetall(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for HGETALL\r\n";
    }

    const std::string& key =
        command.arguments[0];

    std::unordered_map<std::string, std::string> result;

    bool success =
        database.hgetall(
            key,
            result
        );

    if (!success)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    // --------------------------------------------------
    // RESP Array
    // --------------------------------------------------

    std::string response =
        "*" +
        std::to_string(result.size() * 2) +
        "\r\n";

    for (const auto& pair : result)
    {
        const std::string& field = pair.first;
        const std::string& value = pair.second;

        response +=
            "$" +
            std::to_string(field.size()) +
            "\r\n" +
            field +
            "\r\n";

        response +=
            "$" +
            std::to_string(value.size()) +
            "\r\n" +
            value +
            "\r\n";
    }

    return response;
}   

// --------------------------------------------------
// EXPIRE
// --------------------------------------------------

std::string CommandHandler::handle_expire(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for EXPIRE\r\n";
    }

    const std::string& key =
        command.arguments[0];

    long long seconds;

    try
    {
        seconds =
            std::stoll(
                command.arguments[1]
            );
    }
    catch (...)
    {
        return "-ERR value is not an integer\r\n";
    }

    if (seconds < 0)
    {
        return "-ERR invalid expire time\r\n";
    }

    bool success =
        database.expire(
            key,
            seconds
        );

    if (success)
    {
        return ":1\r\n";
    }

    return ":0\r\n";
}

std::string CommandHandler::handle_ttl(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for TTL\r\n";
    }

    long long seconds =
        database.ttl(
            command.arguments[0]
        );

    return ":" +
           std::to_string(seconds) +
           "\r\n";
}

// --------------------------------------------------
// SET Commands
// --------------------------------------------------
std::string CommandHandler::handle_sadd(
    const Command& command
)
{
    if (command.arguments.size() < 2)
    {
        return "-ERR wrong number of arguments for SADD\r\n";
    }

    const std::string& key =
        command.arguments[0];

    std::vector<std::string> values(
        command.arguments.begin() + 1,
        command.arguments.end()
    );

    int added_count =
        database.sadd(
            key,
            values
        );

    if (added_count == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(added_count) +
           "\r\n";
}

std::string CommandHandler::handle_srem(
    const Command& command
)
{
    if (command.arguments.size() < 2)
    {
        return "-ERR wrong number of arguments for SREM\r\n";
    }

    const std::string& key =
        command.arguments[0];

    std::vector<std::string> values(
        command.arguments.begin() + 1,
        command.arguments.end()
    );

    int removed_count =
        database.srem(
            key,
            values
        );

    if (removed_count == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(removed_count) +
           "\r\n";
}

std::string CommandHandler::handle_sismember(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for SISMEMBER\r\n";
    }

    const std::string& key =
        command.arguments[0];

    const std::string& value =
        command.arguments[1];

    int is_member =
        database.sismember(
            key,
            value
        );

    if (is_member == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(is_member) +
           "\r\n";
}

std::string CommandHandler::handle_scard(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for SCARD\r\n";
    }

    const std::string& key =
        command.arguments[0];

    int cardinality =
        database.scard(key);

    if (cardinality == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(cardinality) +
           "\r\n";
}

std::string CommandHandler::handle_smembers(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for SMEMBERS\r\n";
    }

    const std::string& key =
        command.arguments[0];

    std::vector<std::string> members;

    bool success =
        database.smembers(
            key,
            members
        );

    if (!success)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    // --------------------------------------------------
    // RESP Array
    // --------------------------------------------------

    std::string response =
        "*" +
        std::to_string(members.size()) +
        "\r\n";

    for (const std::string& member : members)
    {
        response +=
            "$" +
            std::to_string(member.size()) +
            "\r\n" +
            member +
            "\r\n";
    }

    return response;
}

// --------------------------------------------------
// SORTED SET Commands
// --------------------------------------------------
std::string CommandHandler::handle_zadd(
    const Command& command
)
{
    if (command.arguments.size() != 3)
    {
        return "-ERR wrong number of arguments for ZADD\r\n";
    }

    const std::string& key =
        command.arguments[0];

    double score;

    try
    {
        score =
            std::stod(
                command.arguments[1]
            );
    }
    catch (...)
    {
        return "-ERR value is not a valid float\r\n";
    }

    const std::string& member =
        command.arguments[2];

    int result =
        database.zadd(
            key,
            score,
            member
        );

    if (result == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(result) +
           "\r\n";
}

std::string CommandHandler::handle_zscore(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return "-ERR wrong number of arguments for ZSCORE\r\n";
    }

    double score;

    bool found =
        database.zscore(
            command.arguments[0],
            command.arguments[1],
            score
        );

    if (!found)
    {
        return "$-1\r\n";
    }

    std::string score_string =
        std::to_string(score);

    return "$" +
           std::to_string(score_string.size()) +
           "\r\n" +
           score_string +
           "\r\n";
}

std::string CommandHandler::handle_zrem(
    const Command& command
)
{
    if (command.arguments.size() < 2)
    {
        return "-ERR wrong number of arguments for ZREM\r\n";
    }

    std::vector<std::string> members;

    for (size_t i = 1;
         i < command.arguments.size();
         i++)
    {
        members.push_back(
            command.arguments[i]
        );
    }

    int result =
        database.zrem(
            command.arguments[0],
            members
        );

    if (result == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(result) +
           "\r\n";
}

std::string CommandHandler::handle_zcard(
    const Command& command
)
{
    if (command.arguments.size() != 1)
    {
        return "-ERR wrong number of arguments for ZCARD\r\n";
    }

    int result =
        database.zcard(
            command.arguments[0]
        );

    if (result == -1)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    return ":" +
           std::to_string(result) +
           "\r\n";
}

std::string CommandHandler::handle_zrange(
    const Command& command
)
{
    // --------------------------------------------------
    // ZRANGE key start stop
    // ZRANGE key start stop WITHSCORES
    // --------------------------------------------------

    if (
        command.arguments.size() != 3 &&
        command.arguments.size() != 4
    )
    {
        return "-ERR wrong number of arguments for ZRANGE\r\n";
    }

    long long start;
    long long stop;

    // --------------------------------------------------
    // Parse start
    // --------------------------------------------------

    try
    {
        start =
            std::stoll(
                command.arguments[1]
            );

        stop =
            std::stoll(
                command.arguments[2]
            );
    }
    catch (...)
    {
        return "-ERR value is not an integer\r\n";
    }

    // --------------------------------------------------
    // Check WITHSCORES
    // --------------------------------------------------

    bool with_scores = false;

    if (command.arguments.size() == 4)
    {
        std::string option =
            command.arguments[3];

        std::transform(
            option.begin(),
            option.end(),
            option.begin(),
            [](unsigned char c)
            {
                return std::toupper(c);
            }
        );

        if (option != "WITHSCORES")
        {
            return "-ERR syntax error\r\n";
        }

        with_scores = true;
    }

    // --------------------------------------------------
    // Get data from Database
    // --------------------------------------------------

    std::vector<
        std::pair<std::string, double>
    > values;

    bool success =
        database.zrange(
            command.arguments[0],
            start,
            stop,
            values
        );

    // Wrong type
    if (!success)
    {
        return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
    }

    // --------------------------------------------------
    // RESP Array
    // --------------------------------------------------

    if (!with_scores)
    {
        // ----------------------------------------------
        // ZRANGE without WITHSCORES
        //
        // *3
        // $5
        // Rahul
        // $4
        // Alex
        // $5
        // Manab
        // ----------------------------------------------

        std::string response =
            "*" +
            std::to_string(values.size()) +
            "\r\n";

        for (const auto& entry : values)
        {
            const std::string& member =
                entry.first;

            response +=
                "$" +
                std::to_string(member.size()) +
                "\r\n" +
                member +
                "\r\n";
        }

        return response;
    }

    // --------------------------------------------------
    // ZRANGE WITHSCORES
    // --------------------------------------------------

    std::string response =
        "*" +
        std::to_string(values.size() * 2) +
        "\r\n";

    for (const auto& entry : values)
    {
        const std::string& member =
            entry.first;

        double score =
            entry.second;

        std::string score_string =
            std::to_string(score);

        // Member
        response +=
            "$" +
            std::to_string(member.size()) +
            "\r\n" +
            member +
            "\r\n";

        // Score
        response +=
            "$" +
            std::to_string(score_string.size()) +
            "\r\n" +
            score_string +
            "\r\n";
    }

    return response;
}

std::string CommandHandler::handle_save(
    const Command& command
)
{
    if (!command.arguments.empty())
    {
        return "-ERR wrong number of arguments for SAVE\r\n";
    }

    bool success =
        database.save_to_file(
            "dump.rdb"
        );

    if (!success)
    {
        return "-ERR failed to save database\r\n";
    }

    return "+OK\r\n";
}

std::string CommandHandler::handle_config(
    const Command& command
)
{
    // CONFIG requires at least:
    // CONFIG SET ...
    // CONFIG GET ...

    if (command.arguments.empty())
    {
        return "-ERR wrong number of arguments for CONFIG\r\n";
    }

    const std::string& subcommand =
        command.arguments[0];

    // ==================================================
    // CONFIG SET
    // ==================================================

    if (subcommand == "SET")
    {
        if (command.arguments.size() != 3)
        {
            return "-ERR wrong number of arguments for CONFIG SET\r\n";
        }

        const std::string& option =
            command.arguments[1];

        const std::string& value =
            command.arguments[2];

        // ----------------------------------------------
        // maxmemory
        // ----------------------------------------------

        if (option == "maxmemory")
        {
            size_t bytes;

            try
            {
                bytes =
                    std::stoull(value);
            }
            catch (...)
            {
                return "-ERR invalid maxmemory value\r\n";
            }

            database.set_max_memory(bytes);

            return "+OK\r\n";
        }

        // ----------------------------------------------
        // maxmemory-policy
        // ----------------------------------------------

        if (option == "maxmemory-policy")
        {
            // NONE
            if (value == "none")
            {
                database.set_eviction_policy(
                    Database::EvictionPolicy::NONE
                );

                return "+OK\r\n";
            }

            // LFU
            if (value == "lfu")
            {
                database.set_eviction_policy(
                    Database::EvictionPolicy::LFU
                );

                return "+OK\r\n";
            }

            // LRU
            if (value == "lru")
            {
                database.set_eviction_policy(
                    Database::EvictionPolicy::LRU
                );

                return "+OK\r\n";
            }

            return "-ERR unsupported eviction policy\r\n";
        }

        return "-ERR unsupported CONFIG option\r\n";
    }

    // ==================================================
    // CONFIG GET
    // ==================================================

    if (subcommand == "GET")
    {
        if (command.arguments.size() != 2)
        {
            return "-ERR wrong number of arguments for CONFIG GET\r\n";
        }

        const std::string& option =
            command.arguments[1];

        // ----------------------------------------------
        // GET maxmemory
        // ----------------------------------------------

        if (option == "maxmemory")
        {
            std::string value =
                std::to_string(
                    database.get_max_memory()
                );

            return "*2\r\n"
                   "$9\r\n"
                   "maxmemory\r\n"
                   "$" +
                   std::to_string(value.size()) +
                   "\r\n" +
                   value +
                   "\r\n";
        }

        // ----------------------------------------------
        // GET maxmemory-policy
        // ----------------------------------------------

        if (option == "maxmemory-policy")
        {
            std::string policy;

            if (
                database.get_eviction_policy()
                == Database::EvictionPolicy::LFU
            )
            {
                policy = "lfu";
            }
            else if (
                database.get_eviction_policy()
                == Database::EvictionPolicy::LRU
            )
            {
                policy = "lru";
            }
            else
            {
                policy = "none";
            }

            return "*2\r\n"
                   "$16\r\n"
                   "maxmemory-policy\r\n"
                   "$" +
                   std::to_string(policy.size()) +
                   "\r\n" +
                   policy +
                   "\r\n";
        }

        return "-ERR unsupported CONFIG option\r\n";
    }

    return "-ERR unsupported CONFIG subcommand\r\n";
}

std::string CommandHandler::handle_replicaof(
    const Command& command
)
{
    if (command.arguments.size() != 2)
    {
        return
            "-ERR wrong number of arguments "
            "for REPLICAOF\r\n";
    }

    const std::string& host =
        command.arguments[0];

    int port;

    try
    {
        port =
            std::stoi(
                command.arguments[1]
            );
    }
    catch (...)
    {
        return
            "-ERR invalid port\r\n";
    }

    if (
        port <= 0 ||
        port > 65535
    )
    {
        return
            "-ERR invalid port\r\n";
    }

    bool success =
        server.connect_to_primary(
            host,
            port
        );

    if (!success)
    {
        return
            "-ERR failed to connect "
            "to primary\r\n";
    }

    return "+OK\r\n";
}

std::string CommandHandler::handle_promote(
    const Command& command
)
{
    if (!command.arguments.empty())
    {
        return
            "-ERR wrong number of arguments "
            "for PROMOTE\r\n";
    }

    if (
        server.get_role()
        == ServerRole::PRIMARY
    )
    {
        return
            "-ERR server is already primary\r\n";
    }

    server.promote_to_primary();

    return "+OK\r\n";
}