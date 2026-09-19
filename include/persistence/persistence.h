#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <string>

class Database;

class Persistence
{
private:

    std::string filename;

public:

    Persistence(
        const std::string& filename = "dump.rdb"
    );

    bool save(
        Database& database
    );

    bool load(
        Database& database
    );
};

#endif