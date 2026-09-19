#ifndef LFU_H
#define LFU_H

#include <string>
#include <unordered_map>

// ==================================================
// LFU NODE
// ==================================================

struct LFUNode
{
    std::string key;

    int frequency;

    LFUNode* prev;
    LFUNode* next;

    LFUNode(
        const std::string& key
    )
        : key(key),
          frequency(1),
          prev(nullptr),
          next(nullptr)
    {
    }
};

// ==================================================
// LFU LIST
// ==================================================

class LFUList
{
public:

    LFUNode* head;
    LFUNode* tail;

    int size;

    LFUList();

    ~LFUList();

    void add_front(
        LFUNode* node
    );

    void remove_node(
        LFUNode* node
    );

    LFUNode* remove_last();

    bool empty() const;
};

// ==================================================
// LFU MANAGER
// ==================================================

class LFU
{
private:

    // key → LFU node
    std::unordered_map<
        std::string,
        LFUNode*
    > key_node;

    // frequency → list
    std::unordered_map<
        int,
        LFUList*
    > frequency_list;

    // Minimum frequency
    int min_frequency;

public:

    LFU();

    ~LFU();

    // Add new key
    void add(
        const std::string& key
    );

    // Key accessed
    void touch(
        const std::string& key
    );

    // Remove key
    void remove(
        const std::string& key
    );

    // Get least frequently used key
    std::string get_lfu_key();

    // Check key
    bool contains(
        const std::string& key
    ) const;
};

#endif