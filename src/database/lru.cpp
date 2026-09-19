#include "lru.h"


// ==================================================
// Constructor
// ==================================================

LRU::LRU()
{
    head =
        new LRUNode("");

    tail =
        new LRUNode("");

    head->next = tail;
    tail->prev = head;
}


// ==================================================
// Destructor
// ==================================================

LRU::~LRU()
{
    clear();

    delete head;
    delete tail;
}


// ==================================================
// Insert After Head
// ==================================================

void LRU::insertAfterHead(
    LRUNode* node
)
{
    LRUNode* currAfterHead =
        head->next;

    node->next =
        currAfterHead;

    currAfterHead->prev =
        node;

    head->next =
        node;

    node->prev =
        head;
}


// ==================================================
// Delete Node
// ==================================================

void LRU::deleteNode(
    LRUNode* node
)
{
    LRUNode* prevNode =
        node->prev;

    LRUNode* nextNode =
        node->next;

    prevNode->next =
        nextNode;

    nextNode->prev =
        prevNode;
}


// ==================================================
// ADD
// ==================================================

void LRU::add(
    const std::string& key
)
{
    // If key already exists,
    // remove old node first.
    if (keyNode.find(key)
        != keyNode.end())
    {
        remove(key);
    }

    LRUNode* node =
        new LRUNode(key);

    keyNode[key] =
        node;

    insertAfterHead(node);
}


// ==================================================
// TOUCH
// ==================================================

void LRU::touch(
    const std::string& key
)
{
    auto it =
        keyNode.find(key);

    if (it == keyNode.end())
    {
        return;
    }

    LRUNode* node =
        it->second;

    // Remove from current position
    deleteNode(node);

    // Move to front
    insertAfterHead(node);
}


// ==================================================
// REMOVE
// ==================================================

void LRU::remove(
    const std::string& key
)
{
    auto it =
        keyNode.find(key);

    if (it == keyNode.end())
    {
        return;
    }

    LRUNode* node =
        it->second;

    deleteNode(node);

    keyNode.erase(it);

    delete node;
}


// ==================================================
// GET LRU KEY
// ==================================================

std::string LRU::get_lru_key()
{
    // No keys
    if (head->next == tail)
    {
        return "";
    }

    // Least recently used
    // is just before tail.
    LRUNode* node =
        tail->prev;

    return node->key;
}


// ==================================================
// CLEAR
// ==================================================

void LRU::clear()
{
    LRUNode* current =
        head->next;

    while (current != tail)
    {
        LRUNode* next =
            current->next;

        delete current;

        current = next;
    }

    keyNode.clear();

    head->next = tail;
    tail->prev = head;
}