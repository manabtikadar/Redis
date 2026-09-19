#include "database.h"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>

// --------------------------------------------------
// SET
// --------------------------------------------------

// void Database::set(
//     const std::string& key,
//     const std::string& value,
//     long long expiration_seconds
// )
// {
//     std::lock_guard<std::mutex> lock(db_mutex);

//     // Remove old value
//     data.erase(key);

//     // Remove old expiration
//     expiration.erase(key);

//     // Store new value
//     data.emplace(
//         key,
//         RedisValue(value)
//     );

//     // Set expiration if requested
//     if (expiration_seconds >= 0)
//     {
//         expiration[key] =
//             std::chrono::steady_clock::now()
//             + std::chrono::seconds(
//                 expiration_seconds
//             );
//     }
// }

void Database::set(
    const std::string& key,
    const std::string& value,
    long long expiration_seconds
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    // If key already exists
    if (data.find(key) != data.end())
    {
        data.erase(key);

        // Remove old LFU entry
        lfu.remove(key);
        lru.remove(key);
    }

    // Insert value
    data.emplace(
        key,
        RedisValue(value)
    );

    // Add key to LFU
    lfu.add(key);
    lru.add(key);

    // TTL
    if (expiration_seconds >= 0)
    {
        expiration[key] =
            std::chrono::steady_clock::now()
            + std::chrono::seconds(
                expiration_seconds
            );
    }
    else
    {
        expiration.erase(key);
    }

    // Memory management
    update_memory();

    evict_if_needed();
}

// --------------------------------------------------
// GET
// --------------------------------------------------

// bool Database::get(
//     const std::string& key,
//     std::string& value
// )
// {
//     std::lock_guard<std::mutex> lock(db_mutex);

//     auto it = data.find(key);

//     if (it == data.end())
//     {
//         return false;
//     }

//     if (
//         it->second.get_type()
//         != RedisValue::Type::STRING
//     )
//     {
//         return false;
//     }

//     value = it->second.as_string();

//     return true;
// }

// bool Database::get(
//     const std::string& key,
//     std::string& value
// )
// {
//     std::lock_guard<std::mutex> lock(db_mutex);

//     auto it = data.find(key);

//     if (it == data.end())
//     {
//         return false;
//     }

//     // Check expiration
//     if (is_expired(key))
//     {
//         return false;
//     }

//     if (
//         it->second.get_type()
//         != RedisValue::Type::STRING
//     )
//     {
//         return false;
//     }

//     value = it->second.as_string();

//     return true;
// }

bool Database::get(
    const std::string& key,
    std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    auto it =
        data.find(key);

    if (it == data.end())
    {
        return false;
    }

    if (is_expired(key))
    {
        return false;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::STRING
    )
    {
        return false;
    }

    value =
        it->second.as_string();

    // Key was accessed
    lfu.touch(key);
    lru.touch(key);

    return true;
}

// --------------------------------------------------
// DEL
// --------------------------------------------------

// bool Database::del(
//     const std::string& key
// )
// {
//     std::lock_guard<std::mutex> lock(db_mutex);

//     return data.erase(key) > 0;
// }

// bool Database::del(
//     const std::string& key
// )
// {
//     std::lock_guard<std::mutex> lock(db_mutex);

//     bool deleted =
//         data.erase(key) > 0;

//     expiration.erase(key);

//     return deleted;
// }

bool Database::del(
    const std::string& key
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    auto it =
        data.find(key);

    if (it == data.end())
    {
        return false;
    }

    data.erase(it);

    expiration.erase(key);

    // Remove from LFU
    lfu.remove(key);
    lru.remove(key);

    // Memory management
    update_memory();

    return true;
}

// ==================================================
// LIST COMMANDS
// ==================================================

// --------------------------------------------------
// LPUSH
// --------------------------------------------------

int Database::lpush(
    const std::string& key,
    const std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    // --------------------------------
    // Key doesn't exist
    // --------------------------------
    if (it == data.end())
    {
        std::vector<std::string> list;

        list.push_back(value);

        data.emplace(
            key,
            RedisValue(list)
        );

        // New key → add to LFU
        lfu.add(key);
        lru.add(key);

        return 1;
    }

    // --------------------------------
    // Wrong type
    // --------------------------------
    if (
        it->second.get_type()
        != RedisValue::Type::LIST
    )
    {
        return -1;
    }

    // --------------------------------
    // Existing list
    // --------------------------------
    std::vector<std::string>& list =
        it->second.as_list();

    list.insert(
        list.begin(),
        value
    );

    // Existing key was accessed
    lfu.touch(key);
    lru.touch(key);

    update_memory();

    evict_if_needed();

    return list.size();
}

// --------------------------------------------------
// RPUSH
// --------------------------------------------------

int Database::rpush(
    const std::string& key,
    const std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if(is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        std::vector<std::string> list;

        list.push_back(value);

        data.emplace(
            key,
            RedisValue(list)
        );

        // New key → add to LFU
        lfu.add(key);
        lru.add(key);

        return 1;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::LIST
    )
    {
        return -1;
    }

    std::vector<std::string>& list =
        it->second.as_list();

    list.push_back(value);

    // Existing key was accessed
    lfu.touch(key);
    lru.touch(key);

    update_memory();

    evict_if_needed();

    return list.size();
}

// --------------------------------------------------
// LPOP
// --------------------------------------------------

bool Database::lpop(
    const std::string& key,
    std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if(is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return false;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::LIST
    )
    {
        return false;
    }

    std::vector<std::string>& list =
        it->second.as_list();

    // Empty list
    if (list.empty())
    {
        return false;
    }

    // Get first element
    value = list.front();

    // Remove first element
    list.erase(list.begin());

    // If list becomes empty,
    // remove the key completely.
    // List becomes empty
    if (list.empty())
    {
        data.erase(it);

        // Remove from LFU
        lfu.remove(key);
        lru.remove(key);

        // TTL is no longer needed
        expiration.erase(key);
    }
    else
    {
        // Existing key was accessed
        lfu.touch(key);
        lru.touch(key);
    }

    update_memory();

    return true;
}

// --------------------------------------------------
// RPOP
// --------------------------------------------------

bool Database::rpop(
    const std::string& key,
    std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if(is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return false;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::LIST
    )
    {
        return false;
    }

    std::vector<std::string>& list =
        it->second.as_list();

    if (list.empty())
    {
        return false;
    }

    // Get last element
    value = list.back();

    // Remove last element
    list.pop_back();

    // List becomes empty
    if (list.empty())
    {
        data.erase(it);

        // Remove from LFU
        lfu.remove(key);
        lru.remove(key);

        // TTL is no longer needed
        expiration.erase(key);
    }
    else
    {
        // Existing key was accessed
        lfu.touch(key);
        lru.touch(key);
    }

    update_memory();

    return true;
}

// --------------------------------------------------
// LRANGE
// --------------------------------------------------

bool Database::lrange(
    const std::string& key,
    int start,
    int stop,
    std::vector<std::string>& result
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if(is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return true;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::LIST
    )
    {
        return false;
    }

    const std::vector<std::string>& list =
        it->second.as_list();

    int size = list.size();

    // Redis supports negative indexes
    if (start < 0)
    {
        start = size + start;
    }

    if (stop < 0)
    {
        stop = size + stop;
    }

    // Clamp start
    if (start < 0)
    {
        start = 0;
    }

    // Clamp stop
    if (stop >= size)
    {
        stop = size - 1;
    }

    // Invalid range
    if (start > stop || start >= size)
    {
        return true;
    }

    for (int i = start; i <= stop; i++)
    {
        result.push_back(list[i]);
    }

    // Key was successfully accessed
    lfu.touch(key);
    lru.touch(key);

    return true;
}

// ==================================================
// HASH COMMANDS
// ==================================================

//--------------------------------------------------
// HSET
//--------------------------------------------------
bool Database::hset(
    const std::string& key,
    const std::string& field,
    const std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    // --------------------------------
    // Key doesn't exist
    // --------------------------------

    if (it == data.end())
    {
        std::unordered_map<
            std::string,
            std::string
        > hash;

        hash[field] = value;

        data.emplace(
            key,
            RedisValue(hash)
        );

        // New key
        lfu.add(key);
        lru.add(key);

        return true;
    }

    // --------------------------------
    // Wrong type
    // --------------------------------

    if (
        it->second.get_type()
        != RedisValue::Type::HASH
    )
    {
        return false;
    }

    // --------------------------------
    // Existing hash
    // --------------------------------

    auto& hash =
        it->second.as_hash();

    hash[field] = value;

    // Existing key was modified
    lfu.touch(key);
    lru.touch(key);

    update_memory();

    evict_if_needed();

    return true;
}

bool Database::hget(
    const std::string& key,
    const std::string& field,
    std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return false;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::HASH
    )
    {
        return false;
    }

    auto& hash =
        it->second.as_hash();

    auto field_it =
        hash.find(field);

    // Field doesn't exist
    if (field_it == hash.end())
    {
        return false;
    }

    value =
        field_it->second;

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return true;
}

int Database::hdel(
    const std::string& key,
    const std::string& field
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return 0;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::HASH
    )
    {
        return -1;
    }

    auto& hash =
        it->second.as_hash();

    int deleted =
        hash.erase(field);

    // Field didn't exist
    if (deleted == 0)
    {
        return 0;
    }

    // Hash became empty
    if (hash.empty())
    {
        data.erase(it);

        // Key completely removed
        lfu.remove(key);
        lru.remove(key);

        expiration.erase(key);
    }
    else
    {
        // Existing key was modified
        lfu.touch(key);
        lru.touch(key);
    }

    update_memory();

    return deleted;
}

bool Database::hexists(
    const std::string& key,
    const std::string& field
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return false;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::HASH
    )
    {
        return false;
    }

    auto& hash =
        it->second.as_hash();

    auto field_it =
        hash.find(field);

    if (field_it == hash.end())
    {
        return false;
    }

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return true;
}

bool Database::hgetall(
    const std::string& key,
    std::unordered_map<std::string, std::string>& result
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if(is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return true;
    }

    // Key exists but is not a HASH
    if (
        it->second.get_type()
        != RedisValue::Type::HASH
    )
    {
        return false;
    }

    const auto& hash =
        it->second.as_hash();

    result = hash;

    lfu.touch(key);
    lru.touch(key);

    return true;
}

// --------------------------------------------------
// EXPIRATION COMMANDS
// --------------------------------------------------

// bool Database::is_expired(
//     const std::string& key
// )
// {
//     auto it = expiration.find(key);

//     // No expiration
//     if (it == expiration.end())
//     {
//         return false;
//     }

//     // Check expiration
//     if (
//         std::chrono::steady_clock::now()
//         >= it->second
//     )
//     {
//         // Delete the actual data
//         data.erase(key);

//         // Delete expiration information
//         expiration.erase(it);

//         return true;
//     }

//     return false;
// }

bool Database::is_expired(
    const std::string& key
)
{
    auto it =
        expiration.find(key);

    if (it == expiration.end())
    {
        return false;
    }

    if (
        std::chrono::steady_clock::now()
        >= it->second
    )
    {
        remove_key(key);

        return true;
    }

    return false;
}

bool Database::expire(
    const std::string& key,
    long long seconds
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    // Check whether key exists
    auto it = data.find(key);

    if (it == data.end())
    {
        return false;
    }

    // Already expired
    if (is_expired(key))
    {
        return false;
    }

    expiration[key] =
        std::chrono::steady_clock::now()
        + std::chrono::seconds(seconds);

    return true;
}

long long Database::ttl(
    const std::string& key
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    // Key doesn't exist
    auto data_it = data.find(key);

    if (data_it == data.end())
    {
        return -2;
    }

    // Check whether it expired
    if (is_expired(key))
    {
        return -2;
    }

    auto expiration_it =
        expiration.find(key);

    // Key exists but has no expiration
    if (expiration_it == expiration.end())
    {
        return -1;
    }

    auto remaining =
        std::chrono::duration_cast<
            std::chrono::seconds
        >(
            expiration_it->second
            - std::chrono::steady_clock::now()
        ).count();

    // It may have expired between checks
    if (remaining < 0)
    {
        data.erase(key);
        expiration.erase(expiration_it);

        return -2;
    }

    return remaining;
}

// ==================================================
// SET COMMANDS
// ==================================================
int Database::sadd(
    const std::string& key,
    const std::vector<std::string>& values
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        std::unordered_set<std::string> set;

        for (const auto& value : values)
        {
            set.insert(value);
        }

        data.emplace(
            key,
            RedisValue(set)
        );

        // New key
        lfu.add(key);
        lru.add(key);

        return set.size();
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::SET
    )
    {
        return -1;
    }

    auto& set =
        it->second.as_set();

    int added_count = 0;

    for (const auto& value : values)
    {
        auto result =
            set.insert(value);

        if (result.second)
        {
            added_count++;
        }
    }

    // Existing key was accessed/modified
    lfu.touch(key);
    lru.touch(key);

    update_memory();

    evict_if_needed();

    return added_count;
}

int Database::srem(
    const std::string& key,
    const std::vector<std::string>& values
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return 0;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::SET
    )
    {
        return -1;
    }

    auto& set =
        it->second.as_set();

    int removed_count = 0;

    for (const auto& value : values)
    {
        removed_count +=
            set.erase(value);
    }

    if (removed_count == 0)
    {
        return 0;
    }

    // Set became empty
    if (set.empty())
    {
        data.erase(it);

        // Key completely removed
        lfu.remove(key);
        lru.remove(key);

        expiration.erase(key);
    }
    else
    {
        // Existing key was modified
        lfu.touch(key);
        lru.touch(key);
    }

    update_memory();

    evict_if_needed();

    return removed_count;
}

int Database::sismember(
    const std::string& key,
    const std::string& value
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return 0;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::SET
    )
    {
        return -1;
    }

    auto& set =
        it->second.as_set();

    if (set.count(value) == 0)
    {
        return 0;
    }

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return 1;
}

int Database::scard(
    const std::string& key
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return -1;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return 0;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::SET
    )
    {
        return -1;
    }

    auto& set =
        it->second.as_set();

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return set.size();
}

bool Database::smembers(
    const std::string& key,
    std::vector<std::string>& result
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return false;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::SET
    )
    {
        return false;
    }

    auto& set =
        it->second.as_set();

    result.assign(
        set.begin(),
        set.end()
    );

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return true;
}

// ==================================================
// SORTED SET COMMANDS
// ==================================================
int Database::zadd(
    const std::string& key,
    double score,
    const std::string& member
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    // Check expiration
    if (is_expired(key))
    {
        // Key was removed.
        // Continue and create a new sorted set.
    }

    auto it = data.find(key);

    // --------------------------------------------------
    // Key doesn't exist
    // --------------------------------------------------

    if (it == data.end())
    {
        std::unordered_map<
            std::string,
            double
        > sorted_set;

        sorted_set[member] = score;

        data.emplace(
            key,
            RedisValue(sorted_set)
        );

        // New key
        lfu.add(key);
        lru.add(key);

        return 1;
    }

    // --------------------------------------------------
    // Wrong type
    // --------------------------------------------------

    if (
        it->second.get_type()
        != RedisValue::Type::SORTED_SET
    )
    {
        return -1;
    }

    auto& sorted_set =
        it->second.as_sorted_set();

    auto member_it =
        sorted_set.find(member);

    // --------------------------------------------------
    // New member
    // --------------------------------------------------

    if (member_it == sorted_set.end())
    {
        sorted_set[member] = score;

        // Existing key modified
        lfu.touch(key);
        lru.touch(key);

        return 1;
    }

    // --------------------------------------------------
    // Existing member → update score
    // --------------------------------------------------

    member_it->second = score;

    // Existing key modified
    lfu.touch(key);
    lru.touch(key);

    update_memory();

    evict_if_needed();

    return 0;
}

bool Database::zscore(
    const std::string& key,
    const std::string& member,
    double& score
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return false;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return false;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::SORTED_SET
    )
    {
        return false;
    }

    auto& sorted_set =
        it->second.as_sorted_set();

    auto member_it =
        sorted_set.find(member);

    if (member_it == sorted_set.end())
    {
        return false;
    }

    score =
        member_it->second;

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return true;
}

int Database::zrem(
    const std::string& key,
    const std::vector<std::string>& members
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return 0;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return 0;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::SORTED_SET
    )
    {
        return -1;
    }

    auto& sorted_set =
        it->second.as_sorted_set();

    int removed = 0;

    for (const std::string& member : members)
    {
        removed +=
            sorted_set.erase(member);
    }

    // Nothing was removed
    if (removed == 0)
    {
        return 0;
    }

    // Sorted set became empty
    if (sorted_set.empty())
    {
        data.erase(it);

        expiration.erase(key);

        // Remove completely from LFU
        lfu.remove(key);
        lru.remove(key);
    }
    else
    {
        // Existing key was modified
        lfu.touch(key);
        lru.touch(key);
    }

    update_memory();

    return removed;
}

int Database::zcard(
    const std::string& key
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_expired(key))
    {
        return 0;
    }

    auto it = data.find(key);

    if (it == data.end())
    {
        return 0;
    }

    if (
        it->second.get_type()
        != RedisValue::Type::SORTED_SET
    )
    {
        return -1;
    }

    auto& sorted_set =
        it->second.as_sorted_set();

    // Successful access
    lfu.touch(key);
    lru.touch(key);

    return static_cast<int>(
        sorted_set.size()
    );
}

bool Database::zrange(
    const std::string& key,
    long long start,
    long long stop,
    std::vector<
        std::pair<std::string, double>
    >& result
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    // Check expiration
    if (is_expired(key))
    {
        return true;
    }

    auto it = data.find(key);

    // Key doesn't exist
    if (it == data.end())
    {
        return true;
    }

    // Wrong type
    if (
        it->second.get_type()
        != RedisValue::Type::SORTED_SET
    )
    {
        return false;
    }

    auto& sorted_set =
        it->second.as_sorted_set();

    // --------------------------------------------------
    // Convert unordered_map into sortable vector
    // --------------------------------------------------

    std::vector<
        std::pair<std::string, double>
    > sorted_members;

    for (const auto& entry : sorted_set)
    {
        sorted_members.push_back({
            entry.first,   // member
            entry.second   // score
        });
    }

    // --------------------------------------------------
    // Sort by score
    // If scores are equal, sort by member
    // --------------------------------------------------

    std::sort(
        sorted_members.begin(),
        sorted_members.end(),
        [](const auto& a, const auto& b)
        {
            // Compare scores first
            if (a.second != b.second)
            {
                return a.second < b.second;
            }

            // Same score → compare member names
            return a.first < b.first;
        }
    );

    // --------------------------------------------------
    // Handle negative indexes
    // --------------------------------------------------

    long long size =
        static_cast<long long>(
            sorted_members.size()
        );

    if (start < 0)
    {
        start = size + start;
    }

    if (stop < 0)
    {
        stop = size + stop;
    }

    // Clamp start
    if (start < 0)
    {
        start = 0;
    }

    // Clamp stop
    if (stop >= size)
    {
        stop = size - 1;
    }

    // Invalid range
    if (
        start > stop ||
        start >= size
    )
    {
        return true;
    }

    // --------------------------------------------------
    // Copy selected elements
    // --------------------------------------------------

    for (
        long long i = start;
        i <= stop;
        i++
    )
    {
        result.push_back(
            sorted_members[i]
        );
    }

    lfu.touch(key);
    lru.touch(key);

    return true;
}

static void write_string(
    std::ofstream& file,
    const std::string& value
)
{
    uint64_t size =
        value.size();

    file.write(
        reinterpret_cast<const char*>(&size),
        sizeof(size)
    );

    file.write(
        value.data(),
        size
    );
}

static bool read_string(
    std::ifstream& file,
    std::string& value
)
{
    uint64_t size;

    if (!file.read(
        reinterpret_cast<char*>(&size),
        sizeof(size)
    ))
    {
        return false;
    }

    value.resize(size);

    if (!file.read(
        &value[0],
        size
    ))
    {
        return false;
    }

    return true;
}

enum class PersistType : uint8_t
{
    STRING = 1,
    LIST = 2,
    HASH = 3,
    SET = 4,
    SORTED_SET = 5
};

bool Database::save_to_file(
    const std::string& filename
)
{
    // std::lock_guard<std::mutex> lock(db_mutex);

    // std::ofstream file(
    //     filename,
    //     std::ios::binary |
    //     std::ios::trunc
    // );

    // if (!file.is_open())
    // {
    //     return false;
    // }

    std::cout
        << "[SAVE] filename = "
        << filename
        << std::endl;

    std::cout
        << "[SAVE] opening file..."
        << std::endl;

    std::lock_guard<std::mutex> lock(db_mutex);

    std::ofstream file(
        filename,
        std::ios::binary |
        std::ios::trunc
    );

    if (!file.is_open())
    {
        std::cerr
            << "[SAVE] FAILED TO OPEN FILE"
            << std::endl;

        return false;
    }

    std::cout
        << "[SAVE] file opened"
        << std::endl;

    // --------------------------------------------------
    // MAGIC HEADER
    // --------------------------------------------------

    const char magic[] = "REDIFORGE1";

    file.write(
        magic,
        10
    );

    // --------------------------------------------------
    // Number of keys
    // --------------------------------------------------

    uint64_t key_count =
        data.size();

    file.write(
        reinterpret_cast<const char*>(&key_count),
        sizeof(key_count)
    );

    // --------------------------------------------------
    // Save each key
    // --------------------------------------------------

    for (auto& entry : data)
    {
        const std::string& key =
            entry.first;

        RedisValue& value =
            entry.second;

        // Key
        write_string(
            file,
            key
        );

        // Type
        PersistType type;

        switch (value.get_type())
        {
            case RedisValue::Type::STRING:
                type = PersistType::STRING;
                break;

            case RedisValue::Type::LIST:
                type = PersistType::LIST;
                break;

            case RedisValue::Type::HASH:
                type = PersistType::HASH;
                break;

            case RedisValue::Type::SET:
                type = PersistType::SET;
                break;

            case RedisValue::Type::SORTED_SET:
                type = PersistType::SORTED_SET;
                break;
        }

        file.write(
            reinterpret_cast<const char*>(&type),
            sizeof(type)
        );

        // --------------------------------------------------
        // STRING
        // --------------------------------------------------

        if (
            value.get_type()
            == RedisValue::Type::STRING
        )
        {
            write_string(
                file,
                value.as_string()
            );
        }

        // --------------------------------------------------
        // LIST
        // --------------------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::LIST
        )
        {
            auto& list =
                value.as_list();

            uint64_t size =
                list.size();

            file.write(
                reinterpret_cast<const char*>(&size),
                sizeof(size)
            );

            for (const auto& item : list)
            {
                write_string(
                    file,
                    item
                );
            }
        }

        // --------------------------------------------------
        // HASH
        // --------------------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::HASH
        )
        {
            auto& hash =
                value.as_hash();

            uint64_t size =
                hash.size();

            file.write(
                reinterpret_cast<const char*>(&size),
                sizeof(size)
            );

            for (const auto& item : hash)
            {
                write_string(
                    file,
                    item.first
                );

                write_string(
                    file,
                    item.second
                );
            }
        }

        // --------------------------------------------------
        // SET
        // --------------------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::SET
        )
        {
            auto& set =
                value.as_set();

            uint64_t size =
                set.size();

            file.write(
                reinterpret_cast<const char*>(&size),
                sizeof(size)
            );

            for (const auto& item : set)
            {
                write_string(
                    file,
                    item
                );
            }
        }

        // --------------------------------------------------
        // SORTED SET
        // --------------------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::SORTED_SET
        )
        {
            auto& sorted_set =
                value.as_sorted_set();

            uint64_t size =
                sorted_set.size();

            file.write(
                reinterpret_cast<const char*>(&size),
                sizeof(size)
            );

            for (const auto& item : sorted_set)
            {
                // Member
                write_string(
                    file,
                    item.first
                );

                // Score
                double score =
                    item.second;

                file.write(
                    reinterpret_cast<const char*>(&score),
                    sizeof(score)
                );
            }
        }

        // --------------------------------------------------
        // TTL
        // --------------------------------------------------

        auto expiration_it =
            expiration.find(key);

        bool has_expiration =
            expiration_it != expiration.end();

        file.write(
            reinterpret_cast<const char*>(
                &has_expiration
            ),
            sizeof(has_expiration)
        );

        if (has_expiration)
        {
            auto now =
                std::chrono::steady_clock::now();

            auto remaining =
                std::chrono::duration_cast<
                    std::chrono::seconds
                >(
                    expiration_it->second - now
                ).count();

            if (remaining < 0)
            {
                remaining = 0;
            }

            file.write(
                reinterpret_cast<const char*>(&remaining),
                sizeof(remaining)
            );
        }
    }

    file.close();

    return true;
}

bool Database::load_from_file(
    const std::string& filename
)
{
    std::cout
        << "Loading file: "
        << filename
        << std::endl;

    std::lock_guard<std::mutex> lock(db_mutex);

    std::ifstream file(
        filename,
        std::ios::binary
    );

    if (!file.is_open())
    {
        std::cerr
            << "ERROR: Cannot open file"
            << std::endl;

        return false;
    }

    std::cout
        << "File opened successfully"
        << std::endl;

    // --------------------------------------------------
    // MAGIC
    // --------------------------------------------------

    char magic[10];

    if (!file.read(magic, 10))
    {
        std::cerr
            << "ERROR: Cannot read magic"
            << std::endl;

        return false;
    }

    std::cout
        << "Magic = ["
        << std::string(magic, 10)
        << "]"
        << std::endl;

    if (
        std::string(magic, 10)
        != "REDIFORGE1"
    )
    {
        std::cerr
            << "ERROR: Invalid magic"
            << std::endl;

        return false;
    }

    std::cout
        << "Magic OK"
        << std::endl;

    // --------------------------------------------------
    // KEY COUNT
    // --------------------------------------------------

    uint64_t key_count;

    if (!file.read(
        reinterpret_cast<char*>(&key_count),
        sizeof(key_count)
    ))
    {
        std::cerr
            << "ERROR: Cannot read key count"
            << std::endl;

        return false;
    }

    std::cout
        << "Key count = "
        << key_count
        << std::endl;
    // std::lock_guard<std::mutex> lock(db_mutex);

    // std::ifstream file(
    //     filename,
    //     std::ios::binary
    // );

    // if (!file.is_open())
    // {
    //     return false;
    // }

    // // --------------------------------------------------
    // // Check MAGIC
    // // --------------------------------------------------

    // char magic[11];

    // if (!file.read(
    //     magic,
    //     sizeof(magic)
    // ))
    // {
    //     return false;
    // }

    // const std::string expected =
    //     "REDIFORGE1";

    // if (
    //     std::string(
    //         magic,
    //         sizeof(magic)
    //     ) != expected
    // )
    // {
    //     return false;
    // }

    // --------------------------------------------------
    // Number of keys
    // --------------------------------------------------

    // uint64_t key_count;

    // if (!file.read(
    //     reinterpret_cast<char*>(&key_count),
    //     sizeof(key_count)
    // ))
    // {
    //     return false;
    // }

    // data.clear();
    // expiration.clear();

    // --------------------------------------------------
    // Load keys
    // --------------------------------------------------

    for (
        uint64_t i = 0;
        i < key_count;
        i++
    )
    {
        std::string key;

        if (!read_string(file, key))
        {
            return false;
        }

        PersistType type;

        if (!file.read(
            reinterpret_cast<char*>(&type),
            sizeof(type)
        ))
        {
            return false;
        }

        // --------------------------------------------------
        // STRING
        // --------------------------------------------------

        if (type == PersistType::STRING)
        {
            std::string value;

            if (!read_string(file, value))
            {
                return false;
            }

            data.emplace(
                key,
                RedisValue(value)
            );
        }

        // --------------------------------------------------
        // LIST
        // --------------------------------------------------

        else if (type == PersistType::LIST)
        {
            uint64_t size;

            file.read(
                reinterpret_cast<char*>(&size),
                sizeof(size)
            );

            std::vector<std::string> list;

            for (
                uint64_t j = 0;
                j < size;
                j++
            )
            {
                std::string item;

                if (!read_string(file, item))
                {
                    return false;
                }

                list.push_back(item);
            }

            data.emplace(
                key,
                RedisValue(list)
            );
        }

        // --------------------------------------------------
        // HASH
        // --------------------------------------------------

        else if (type == PersistType::HASH)
        {
            uint64_t size;

            file.read(
                reinterpret_cast<char*>(&size),
                sizeof(size)
            );

            std::unordered_map<
                std::string,
                std::string
            > hash;

            for (
                uint64_t j = 0;
                j < size;
                j++
            )
            {
                std::string field;
                std::string value;

                if (!read_string(file, field))
                {
                    return false;
                }

                if (!read_string(file, value))
                {
                    return false;
                }

                hash[field] = value;
            }

            data.emplace(
                key,
                RedisValue(hash)
            );
        }

        // --------------------------------------------------
        // SET
        // --------------------------------------------------

        else if (type == PersistType::SET)
        {
            uint64_t size;

            file.read(
                reinterpret_cast<char*>(&size),
                sizeof(size)
            );

            std::unordered_set<
                std::string
            > set;

            for (
                uint64_t j = 0;
                j < size;
                j++
            )
            {
                std::string item;

                if (!read_string(file, item))
                {
                    return false;
                }

                set.insert(item);
            }

            data.emplace(
                key,
                RedisValue(set)
            );
        }

        // --------------------------------------------------
        // SORTED SET
        // --------------------------------------------------

        else if (
            type == PersistType::SORTED_SET
        )
        {
            uint64_t size;

            file.read(
                reinterpret_cast<char*>(&size),
                sizeof(size)
            );

            std::unordered_map<
                std::string,
                double
            > sorted_set;

            for (
                uint64_t j = 0;
                j < size;
                j++
            )
            {
                std::string member;

                if (!read_string(file, member))
                {
                    return false;
                }

                double score;

                if (!file.read(
                    reinterpret_cast<char*>(&score),
                    sizeof(score)
                ))
                {
                    return false;
                }

                sorted_set[member] =
                    score;
            }

            data.emplace(
                key,
                RedisValue(sorted_set)
            );
        }

        // --------------------------------------------------
        // TTL
        // --------------------------------------------------

        bool has_expiration;

        if (!file.read(
            reinterpret_cast<char*>(&has_expiration),
            sizeof(has_expiration)
        ))
        {
            return false;
        }

        if (has_expiration)
        {
            long long remaining;

            if (!file.read(
                reinterpret_cast<char*>(&remaining),
                sizeof(remaining)
            ))
            {
                return false;
            }

            if (remaining > 0)
            {
                expiration[key] =
                    std::chrono::steady_clock::now()
                    + std::chrono::seconds(
                        remaining
                    );
            }
            else
            {
                // Already expired
                data.erase(key);
            }
        }
    }

    file.close();

    return true;
}

void Database::add_to_lfu(
    const std::string& key
)
{
    lfu.add(key);
}

void Database::touch_lfu(
    const std::string& key
)
{
    lfu.touch(key);
}

void Database::remove_key(
    const std::string& key
)
{
    data.erase(key);

    expiration.erase(key);

    // Remove from LFU
    lfu.remove(key);
    lru.remove(key);
}

// ==================================================
// Eviction Policy Configuration
// ==================================================

void Database::set_eviction_policy(
    EvictionPolicy policy
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    eviction_policy = policy;
}


// ==================================================
// Get Eviction Policy
// ==================================================

Database::EvictionPolicy
Database::get_eviction_policy() const
{
    // We don't lock here because this function
    // only reads the configuration value.
    return eviction_policy;
}


// ==================================================
// Set Maximum Memory
// ==================================================

void Database::set_max_memory(
    size_t bytes
)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    max_memory = bytes;
}


// ==================================================
// Get Maximum Memory
// ==================================================

size_t Database::get_max_memory() const
{
    return max_memory;
}


// ==================================================
// Get Currently Used Memory
// ==================================================

size_t Database::get_used_memory() const
{
    return used_memory;
}

size_t Database::calculate_memory()
{
    size_t memory = 0;

    for (const auto& entry : data)
    {
        const std::string& key =
            entry.first;

        const RedisValue& value =
            entry.second;

        // Key memory
        memory += key.capacity();

        // -----------------------------------------
        // STRING
        // -----------------------------------------

        if (
            value.get_type()
            == RedisValue::Type::STRING
        )
        {
            memory +=
                value.as_string().capacity();
        }

        // -----------------------------------------
        // LIST
        // -----------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::LIST
        )
        {
            const auto& list =
                value.as_list();

            for (const auto& item : list)
            {
                memory += item.capacity();
            }

            memory +=
                list.capacity()
                * sizeof(std::string);
        }

        // -----------------------------------------
        // HASH
        // -----------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::HASH
        )
        {
            const auto& hash =
                value.as_hash();

            for (const auto& item : hash)
            {
                memory +=
                    item.first.capacity();

                memory +=
                    item.second.capacity();

                memory +=
                    sizeof(item);
            }
        }

        // -----------------------------------------
        // SET
        // -----------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::SET
        )
        {
            const auto& set =
                value.as_set();

            for (const auto& item : set)
            {
                memory += item.capacity();
            }
        }

        // -----------------------------------------
        // SORTED SET
        // -----------------------------------------

        else if (
            value.get_type()
            == RedisValue::Type::SORTED_SET
        )
        {
            const auto& sorted_set =
                value.as_sorted_set();

            for (const auto& item : sorted_set)
            {
                memory +=
                    item.first.capacity();

                memory +=
                    sizeof(double);
            }
        }
    }

    return memory;
}

void Database::update_memory()
{
    used_memory =
        calculate_memory();
}

void Database::evict_if_needed()
{
    // ==========================================
    // Eviction disabled
    // ==========================================

    if (
        eviction_policy
        == EvictionPolicy::NONE
    )
    {
        return;
    }

    // ==========================================
    // Memory is within limit
    // ==========================================

    if (used_memory <= max_memory)
    {
        return;
    }

    // ==========================================
    // LFU eviction
    // ==========================================

    if (
        eviction_policy
        == EvictionPolicy::LFU
    )
    {
        while (used_memory > max_memory)
        {
            std::string key =
                lfu.get_lfu_key();

            // No key available
            if (key.empty())
            {
                break;
            }

            std::cout
                << "[EVICTION] Removing key: "
                << key
                << std::endl;

            // Remove from database,
            // expiration and LFU
            remove_key(key);

            // Recalculate memory
            update_memory();
        }
    }

    // ==================================================
    // LRU
    // ==================================================

    else if (
        eviction_policy
        == EvictionPolicy::LRU
    )
    {
        while (used_memory > max_memory)
        {
            std::string key =
                lru.get_lru_key();

            if (key.empty())
            {
                break;
            }

            std::cout
                << "[EVICTION] LRU removing key: "
                << key
                << std::endl;

            remove_key(key);

            update_memory();
        }
    }
}

std::vector<std::string>
Database::get_snapshot_commands()
{
    std::lock_guard<std::mutex> lock(db_mutex);

    std::vector<std::string> commands;

    // ==================================================
    // Iterate through every key
    // ==================================================

    for (const auto& [key, value] : data)
    {
        RedisValue::Type type =
            value.get_type();


        // ==================================================
        // STRING
        // ==================================================

        if (type == RedisValue::Type::STRING)
        {
            commands.push_back(
                "SET " +
                key +
                " " +
                value.as_string()
            );
        }


        // ==================================================
        // LIST
        // ==================================================

        else if (type == RedisValue::Type::LIST)
        {
            const auto& list =
                value.as_list();

            for (const auto& element : list)
            {
                commands.push_back(
                    "RPUSH " +
                    key +
                    " " +
                    element
                );
            }
        }


        // ==================================================
        // HASH
        // ==================================================

        else if (type == RedisValue::Type::HASH)
        {
            const auto& hash =
                value.as_hash();

            for (const auto& [field, field_value] : hash)
            {
                commands.push_back(
                    "HSET " +
                    key +
                    " " +
                    field +
                    " " +
                    field_value
                );
            }
        }


        // ==================================================
        // SET
        // ==================================================

        else if (type == RedisValue::Type::SET)
        {
            const auto& set =
                value.as_set();

            for (const auto& member : set)
            {
                commands.push_back(
                    "SADD " +
                    key +
                    " " +
                    member
                );
            }
        }


        // ==================================================
        // SORTED SET
        // ==================================================

        else if (type == RedisValue::Type::SORTED_SET)
        {
            const auto& sorted_set =
                value.as_sorted_set();

            for (const auto& [member, score] : sorted_set)
            {
                commands.push_back(
                    "ZADD " +
                    key +
                    " " +
                    std::to_string(score) +
                    " " +
                    member
                );
            }
        }
    }


    // ==================================================
    // TTL / EXPIRATION
    // ==================================================

    for (const auto& [key, expiry_time] : expiration)
    {
        // Key may have expired / been deleted
        if (data.find(key) == data.end())
        {
            continue;
        }

        auto now =
            std::chrono::steady_clock::now();

        auto remaining =
            std::chrono::duration_cast<
                std::chrono::seconds
            >(
                expiry_time - now
            ).count();

        if (remaining <= 0)
        {
            continue;
        }

        commands.push_back(
            "EXPIRE " +
            key +
            " " +
            std::to_string(remaining)
        );
    }

    return commands;
}