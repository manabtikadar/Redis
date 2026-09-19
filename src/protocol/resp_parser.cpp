#include "resp_parser.h"

#include <stdexcept>

ParseResult RespParser::parse(
    const std::string& data
)
{
    ParseResult result;

    result.complete = false;
    result.consumed = 0;

    if (data.empty())
    {
        return result;
    }

    size_t position = 0;

    // --------------------------------------------------
    // RESP Array
    // --------------------------------------------------

    if (data[position] != '*')
    {
        throw std::runtime_error(
            "Expected RESP array"
        );
    }

    position++;

    // Find end of array count line
    size_t line_end =
        data.find("\r\n", position);

    // We don't have the complete line yet
    if (line_end == std::string::npos)
    {
        return result;
    }

    int element_count =
        std::stoi(
            data.substr(
                position,
                line_end - position
            )
        );

    position = line_end + 2;

    // --------------------------------------------------
    // Parse each array element
    // --------------------------------------------------

    for (int i = 0;
         i < element_count;
         i++)
    {
        // We need at least '$'
        if (position >= data.size())
        {
            return result;
        }

        if (data[position] != '$')
        {
            throw std::runtime_error(
                "Expected RESP bulk string"
            );
        }

        position++;

        // Find end of bulk string length
        line_end =
            data.find("\r\n", position);

        if (line_end == std::string::npos)
        {
            return result;
        }

        int string_length =
            std::stoi(
                data.substr(
                    position,
                    line_end - position
                )
            );

        position = line_end + 2;

        // Do we have the complete string?
        if (position + string_length + 2
            > data.size())
        {
            return result;
        }

        std::string value =
            data.substr(
                position,
                string_length
            );

        result.value.elements.push_back(
            value
        );

        position += string_length;

        // Check for \r\n after the value
        if (position + 2 > data.size())
        {
            return result;
        }

        if (data[position] != '\r' ||
            data[position + 1] != '\n')
        {
            throw std::runtime_error(
                "Expected CRLF after bulk string"
            );
        }

        position += 2;
    }

    // --------------------------------------------------
    // Complete RESP message
    // --------------------------------------------------

    result.complete = true;
    result.consumed = position;

    return result;
}