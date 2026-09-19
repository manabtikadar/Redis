#include "redis_value.h"

#include <stdexcept>

// --------------------------------------------------
// String constructor
// --------------------------------------------------

RedisValue::RedisValue(
    const std::string& value
)
{
    type = Type::STRING;
    string_value = value;
}

// --------------------------------------------------
// List constructor
// --------------------------------------------------

RedisValue::RedisValue(
    const std::vector<std::string>& value
)
{
    type = Type::LIST;
    list_value = value;
}

// --------------------------------------------------
// Hash constructor
// --------------------------------------------------
RedisValue::RedisValue(
    const std::unordered_map<std::string, std::string>& value
)
{
    type = Type::HASH;
    hash_value = value;
}

// --------------------------------------------------
// Set constructor
// --------------------------------------------------

RedisValue::RedisValue(
    const std::unordered_set<std::string>& value
)
{
    type = Type::SET;
    set_value = value;
}

//--------------------------------------------------
// Sorted Set constructor
//--------------------------------------------------

RedisValue::RedisValue(
    const std::unordered_map<std::string, double>& value
)
{
    type = Type::SORTED_SET;
    sorted_set_value = value;
}

// --------------------------------------------------
// Get type
// --------------------------------------------------

RedisValue::Type RedisValue::get_type() const
{
    return type;
}

// --------------------------------------------------
// String access
// --------------------------------------------------

std::string& RedisValue::as_string()
{
    if (type != Type::STRING)
    {
        throw std::runtime_error(
            "Redis value is not a string"
        );
    }

    return string_value;
}

const std::string& RedisValue::as_string() const
{
    if (type != Type::STRING)
    {
        throw std::runtime_error(
            "Redis value is not a string"
        );
    }

    return string_value;
}

// --------------------------------------------------
// List access
// --------------------------------------------------

std::vector<std::string>& RedisValue::as_list()
{
    if (type != Type::LIST)
    {
        throw std::runtime_error(
            "Redis value is not a list"
        );
    }

    return list_value;
}

const std::vector<std::string>&
RedisValue::as_list() const
{
    if (type != Type::LIST)
    {
        throw std::runtime_error(
            "Redis value is not a list"
        );
    }

    return list_value;
}

// --------------------------------------------------
// Hash access
// --------------------------------------------------
std::unordered_map<std::string, std::string>& RedisValue::as_hash()
{
    if (type != Type::HASH)
    {
        throw std::runtime_error(
            "Redis value is not a hash"
        );
    }

    return hash_value;
}

const std::unordered_map<std::string, std::string>& RedisValue::as_hash() const
{
    if (type != Type::HASH)
    {
        throw std::runtime_error(
            "Redis value is not a hash"
        );
    }

    return hash_value;
}

// --------------------------------------------------
// Set access
// --------------------------------------------------

std::unordered_set<std::string>& RedisValue::as_set()
{
    if (type != Type::SET)
    {
        throw std::runtime_error(
            "Redis value is not a set"
        );
    }

    return set_value;
}

const std::unordered_set<std::string>& RedisValue::as_set() const
{
    if (type != Type::SET)
    {
        throw std::runtime_error(
            "Redis value is not a set"
        );
    }

    return set_value;
}

// --------------------------------------------------
// Sorted Set access
// --------------------------------------------------
std::unordered_map<std::string, double>& RedisValue::as_sorted_set()
{
    if (type != Type::SORTED_SET)
    {    
        throw std::runtime_error(
            "Redis value is not a sorted set"
        );
    }

    return sorted_set_value;
}

const std::unordered_map<std::string, double>& RedisValue::as_sorted_set() const
{
    if (type != Type::SORTED_SET)
    {
        throw std::runtime_error(
            "Redis value is not a sorted set"
        );
    }

    return sorted_set_value;
}
