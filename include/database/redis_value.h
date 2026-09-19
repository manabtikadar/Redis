#ifndef REDIS_VALUE_H
#define REDIS_VALUE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <bits/stdc++.h>

class RedisValue
{
public:

    enum class Type
    {
        STRING,
        LIST,
        HASH,
        SET,
        SORTED_SET
    };

private:

    Type type;

    // Used when type == STRING
    std::string string_value;

    // Used when type == LIST
    std::vector<std::string> list_value;

    // Used when type == HASH
    std::unordered_map<std::string, std::string> hash_value;

    // Used when type == SET
    std::unordered_set<std::string> set_value;

    // Used when type == SORTED_SET
    std::unordered_map<std::string, double> sorted_set_value;

public:

    // String constructor
    RedisValue(const std::string& value);

    // List constructor
    RedisValue(const std::vector<std::string>& value);

    // Hash constructor
    RedisValue(const std::unordered_map<std::string, std::string>& value);

    // Set constructor
    RedisValue(const std::unordered_set<std::string>& value);

    // Sorted Set constructor
    RedisValue(const std::unordered_map<std::string, double>& value);

    Type get_type() const;

    // String access
    std::string& as_string();

    const std::string& as_string() const;

    // List access
    std::vector<std::string>& as_list();

    const std::vector<std::string>& as_list() const;

    // Hash access
    std::unordered_map<std::string, std::string>& as_hash();

    const std::unordered_map<std::string, std::string>& as_hash() const;

    // Set access
    std::unordered_set<std::string>& as_set();

    const std::unordered_set<std::string>& as_set() const;

    // Sorted Set access
    std::unordered_map<std::string, double>& as_sorted_set();

    const std::unordered_map<std::string, double>& as_sorted_set() const;
};

#endif