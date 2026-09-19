#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <string>

#include "resp_value.h"

using namespace std;

struct ParseResult
{
    RespValue value;

    // Number of bytes belonging to this RESP message
    size_t consumed;

    // false when we don't have a complete message yet
    bool complete;
};

class RespParser
{
public:
    ParseResult parse(const string& data);

};

#endif