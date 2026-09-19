#ifndef LRU_H
#define LRU_H

#include <string>
#include <unordered_map>

struct LRUNode
{
    std::string key;

    LRUNode* next;
    LRUNode* prev;

    LRUNode(
        const std::string& _key
    )
    {
        key = _key;
        next = nullptr;
        prev = nullptr;
    }
};


class LRU
{
private:

    LRUNode* head;
    LRUNode* tail;

    std::unordered_map<
        std::string,
        LRUNode*
    > keyNode;


private:

    // Insert node immediately after head
    void insertAfterHead(
        LRUNode* node
    );

    // Remove node from linked list
    void deleteNode(
        LRUNode* node
    );


public:

    LRU();

    ~LRU();

    // Add new key
    void add(
        const std::string& key
    );

    // Key was accessed
    void touch(
        const std::string& key
    );

    // Remove key
    void remove(
        const std::string& key
    );

    // Return least recently used key
    std::string get_lru_key();

    // Clear everything
    void clear();
};

#endif