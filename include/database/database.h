// #ifndef DATABASE_H
// #define DATABASE_H

// #include <bits/stdc++.h>
// #include <string>
// #include <unordered_map>
// #include <mutex>
// #include <chrono>
// #include <utility>

// #include "redis_value.h"
// #include "lfu.h"

// class Database
// {
// private:
//     // ==================================================
//     // Main database
//     // ==================================================

//     std::unordered_map<
//         std::string,
//         RedisValue
//     > data;


//     // ==================================================
//     // TTL / Expiration
//     // ==================================================

//     std::unordered_map<
//         std::string,
//         std::chrono::steady_clock::time_point
//     > expiration;


//     // ==================================================
//     // Thread safety
//     // ==================================================

//     std::mutex db_mutex;


//     // ==================================================
//     // LFU
//     // ==================================================

//     LFU lfu;

//     // -----------------------------
//     // Memory management
//     // -----------------------------

//     enum class EvictionPolicy
//     {
//         NONE,
//         LFU
//     };

//     EvictionPolicy eviction_policy =
//         EvictionPolicy::NONE;

//     size_t max_memory =
//         64 * 1024 * 1024;

//     size_t used_memory = 0;

// public:


//     // ==================================================
//     // Eviction / Memory configuration
//     // ==================================================

//     void set_eviction_policy(
//         EvictionPolicy policy
//     );

//     EvictionPolicy get_eviction_policy() const;

//     void set_max_memory(
//         size_t bytes
//     );

//     size_t get_max_memory() const;

//     size_t get_used_memory() const;

//     // ==================================================
//     // String commands
//     // ==================================================

//     void set(
//         const std::string& key,
//         const std::string& value,
//         long long expiration_seconds = -1
//     );

//     bool get(
//         const std::string& key,
//         std::string& value
//     );

//     bool del(
//         const std::string& key
//     );


//     // ==================================================
//     // List commands
//     // ==================================================

//     int lpush(
//         const std::string& key,
//         const std::string& value
//     );

//     int rpush(
//         const std::string& key,
//         const std::string& value
//     );

//     bool lpop(
//         const std::string& key,
//         std::string& value
//     );

//     bool rpop(
//         const std::string& key,
//         std::string& value
//     );

//     bool lrange(
//         const std::string& key,
//         int start,
//         int stop,
//         std::vector<std::string>& result
//     );


//     // ==================================================
//     // Hash commands
//     // ==================================================

//     bool hset(
//         const std::string& key,
//         const std::string& field,
//         const std::string& value
//     );

//     bool hget(
//         const std::string& key,
//         const std::string& field,
//         std::string& value
//     );

//     int hdel(
//         const std::string& key,
//         const std::string& field
//     );

//     bool hexists(
//         const std::string& key,
//         const std::string& field
//     );

//     bool hgetall(
//         const std::string& key,
//         std::unordered_map<
//             std::string,
//             std::string
//         >& result
//     );


//     // ==================================================
//     // Set commands
//     // ==================================================

//     int sadd(
//         const std::string& key,
//         const std::vector<std::string>& values
//     );

//     int srem(
//         const std::string& key,
//         const std::vector<std::string>& values
//     );

//     int sismember(
//         const std::string& key,
//         const std::string& value
//     );

//     int scard(
//         const std::string& key
//     );

//     bool smembers(
//         const std::string& key,
//         std::vector<std::string>& result
//     );


//     // ==================================================
//     // Sorted Set commands
//     // ==================================================

//     int zadd(
//         const std::string& key,
//         double score,
//         const std::string& member
//     );

//     bool zscore(
//         const std::string& key,
//         const std::string& member,
//         double& score
//     );

//     int zrem(
//         const std::string& key,
//         const std::vector<std::string>& members
//     );

//     int zcard(
//         const std::string& key
//     );

//     bool zrange(
//         const std::string& key,
//         long long start,
//         long long stop,
//         std::vector<
//             std::pair<std::string, double>
//         >& result
//     );


//     // ==================================================
//     // Expiration commands
//     // ==================================================

//     bool expire(
//         const std::string& key,
//         long long seconds
//     );

//     long long ttl(
//         const std::string& key
//     );


//     // ==================================================
//     // Persistence
//     // ==================================================

//     bool save_to_file(
//         const std::string& filename
//     );

//     bool load_from_file(
//         const std::string& filename
//     );


//     // ==================================================
//     // Remove key completely
//     // ==================================================

//     void remove_key(
//         const std::string& key
//     );


// private:

//     // ==================================================
//     // TTL check
//     // ==================================================

//     bool is_expired(
//         const std::string& key
//     );

//     void add_to_lfu(
//         const std::string& key
//     );

//     void touch_lfu(
//         const std::string& key
//     );
// };

// #endif

#ifndef DATABASE_H
#define DATABASE_H

#include <bits/stdc++.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <utility>

#include "redis_value.h"
#include "lfu.h"
#include "lru.h"

class Database
{
private:

    // ==================================================
    // Main database
    // ==================================================

    std::unordered_map<
        std::string,
        RedisValue
    > data;


    // ==================================================
    // TTL / Expiration
    // ==================================================

    std::unordered_map<
        std::string,
        std::chrono::steady_clock::time_point
    > expiration;


    // ==================================================
    // Thread safety
    // ==================================================

    std::mutex db_mutex;


    // ==================================================
    // LFU
    // ==================================================

    LFU lfu;

    // ==================================================
    // LFU
    // ==================================================

    LRU lru;
    // ==================================================
    // Memory management
    // ==================================================

public:

    enum class EvictionPolicy
    {
        NONE,
        LFU,
        LRU
    };

    // Default: no eviction
    EvictionPolicy eviction_policy =
        EvictionPolicy::NONE;

    // Default maximum memory = 64 MB
    size_t max_memory =
       64 * 1024 * 1024;

    // Currently used memory
    size_t used_memory = 0;


public:

    // ==================================================
    // Eviction / Memory configuration
    // ==================================================

    void set_eviction_policy(
        EvictionPolicy policy
    );

    EvictionPolicy get_eviction_policy() const;

    void set_max_memory(
        size_t bytes
    );

    size_t get_max_memory() const;

    size_t get_used_memory() const;


    // ==================================================
    // String commands
    // ==================================================

    void set(
        const std::string& key,
        const std::string& value,
        long long expiration_seconds = -1
    );

    bool get(
        const std::string& key,
        std::string& value
    );

    bool del(
        const std::string& key
    );


    // ==================================================
    // List commands
    // ==================================================

    int lpush(
        const std::string& key,
        const std::string& value
    );

    int rpush(
        const std::string& key,
        const std::string& value
    );

    bool lpop(
        const std::string& key,
        std::string& value
    );

    bool rpop(
        const std::string& key,
        std::string& value
    );

    bool lrange(
        const std::string& key,
        int start,
        int stop,
        std::vector<std::string>& result
    );


    // ==================================================
    // Hash commands
    // ==================================================

    bool hset(
        const std::string& key,
        const std::string& field,
        const std::string& value
    );

    bool hget(
        const std::string& key,
        const std::string& field,
        std::string& value
    );

    int hdel(
        const std::string& key,
        const std::string& field
    );

    bool hexists(
        const std::string& key,
        const std::string& field
    );

    bool hgetall(
        const std::string& key,
        std::unordered_map<
            std::string,
            std::string
        >& result
    );


    // ==================================================
    // Set commands
    // ==================================================

    int sadd(
        const std::string& key,
        const std::vector<std::string>& values
    );

    int srem(
        const std::string& key,
        const std::vector<std::string>& values
    );

    int sismember(
        const std::string& key,
        const std::string& value
    );

    int scard(
        const std::string& key
    );

    bool smembers(
        const std::string& key,
        std::vector<std::string>& result
    );


    // ==================================================
    // Sorted Set commands
    // ==================================================

    int zadd(
        const std::string& key,
        double score,
        const std::string& member
    );

    bool zscore(
        const std::string& key,
        const std::string& member,
        double& score
    );

    int zrem(
        const std::string& key,
        const std::vector<std::string>& members
    );

    int zcard(
        const std::string& key
    );

    bool zrange(
        const std::string& key,
        long long start,
        long long stop,
        std::vector<
            std::pair<std::string, double>
        >& result
    );


    // ==================================================
    // Expiration commands
    // ==================================================

    bool expire(
        const std::string& key,
        long long seconds
    );

    long long ttl(
        const std::string& key
    );


    // ==================================================
    // Persistence
    // ==================================================

    bool save_to_file(
        const std::string& filename
    );

    bool load_from_file(
        const std::string& filename
    );

    std::vector<std::string> get_snapshot_commands();
    // ==================================================
    // Remove key completely
    // ==================================================

    void remove_key(
        const std::string& key
    );
    

private:

    // ==================================================
    // TTL check
    // ==================================================

    bool is_expired(
        const std::string& key
    );

    void add_to_lfu(
        const std::string& key
    );

    void touch_lfu(
        const std::string& key
    );

    size_t calculate_memory();

    void update_memory();

    void evict_if_needed();
};

#endif