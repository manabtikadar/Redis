#include "lfu.h"

#include <climits>

// ==================================================
// LFU LIST
// ==================================================

LFUList::LFUList()
{
    head =
        new LFUNode("");

    tail =
        new LFUNode("");

    head->next = tail;
    tail->prev = head;

    size = 0;
}

// --------------------------------------------------

LFUList::~LFUList()
{
    delete head;
    delete tail;
}

// --------------------------------------------------

void LFUList::add_front(
    LFUNode* node
)
{
    LFUNode* first =
        head->next;

    head->next = node;

    node->prev = head;
    node->next = first;

    first->prev = node;

    size++;
}

// --------------------------------------------------

void LFUList::remove_node(
    LFUNode* node
)
{
    node->prev->next =
        node->next;

    node->next->prev =
        node->prev;

    size--;
}

// --------------------------------------------------

LFUNode* LFUList::remove_last()
{
    if (size == 0)
    {
        return nullptr;
    }

    LFUNode* node =
        tail->prev;

    remove_node(node);

    return node;
}

// --------------------------------------------------

bool LFUList::empty() const
{
    return size == 0;
}


// ==================================================
// LFU
// ==================================================

LFU::LFU()
{
    min_frequency = 0;
}

// --------------------------------------------------

LFU::~LFU()
{
    for (auto& pair : key_node)
    {
        delete pair.second;
    }

    for (auto& pair : frequency_list)
    {
        delete pair.second;
    }
}

// --------------------------------------------------

void LFU::add(
    const std::string& key
)
{
    // If key already exists
    if (key_node.find(key)
        != key_node.end())
    {
        return;
    }

    LFUNode* node =
        new LFUNode(key);

    key_node[key] =
        node;

    // New keys always have
    // frequency 1

    if (
        frequency_list.find(1)
        == frequency_list.end()
    )
    {
        frequency_list[1] =
            new LFUList();
    }

    frequency_list[1]
        ->add_front(node);

    min_frequency = 1;
}

// --------------------------------------------------

void LFU::touch(
    const std::string& key
)
{
    auto it =
        key_node.find(key);

    if (it == key_node.end())
    {
        return;
    }

    LFUNode* node =
        it->second;

    int old_frequency =
        node->frequency;

    LFUList* old_list =
        frequency_list[
            old_frequency
        ];

    old_list->remove_node(node);

    // If this was the minimum
    // frequency list

    if (
        old_frequency == min_frequency &&
        old_list->empty()
    )
    {
        min_frequency++;
    }

    int new_frequency =
        old_frequency + 1;

    if (
        frequency_list.find(
            new_frequency
        )
        == frequency_list.end()
    )
    {
        frequency_list[
            new_frequency
        ] = new LFUList();
    }

    node->frequency =
        new_frequency;

    frequency_list[
        new_frequency
    ]->add_front(node);
}

// --------------------------------------------------

void LFU::remove(
    const std::string& key
)
{
    auto it =
        key_node.find(key);

    if (it == key_node.end())
    {
        return;
    }

    LFUNode* node =
        it->second;

    int frequency =
        node->frequency;

    LFUList* list =
        frequency_list[frequency];

    list->remove_node(node);

    key_node.erase(it);

    delete node;

    // Update minimum frequency

    if (
        frequency == min_frequency &&
        list->empty()
    )
    {
        min_frequency =
            INT_MAX;

        for (const auto& pair :
             frequency_list)
        {
            if (
                !pair.second->empty() &&
                pair.first < min_frequency
            )
            {
                min_frequency =
                    pair.first;
            }
        }

        if (min_frequency == INT_MAX)
        {
            min_frequency = 0;
        }
    }
}

// --------------------------------------------------

std::string LFU::get_lfu_key()
{
    if (min_frequency == 0)
    {
        return "";
    }

    auto it =
        frequency_list.find(
            min_frequency
        );

    if (
        it == frequency_list.end()
    )
    {
        return "";
    }

    LFUNode* node =
        it->second->tail->prev;

    if (
        node == it->second->head
    )
    {
        return "";
    }

    return node->key;
}

// --------------------------------------------------

bool LFU::contains(
    const std::string& key
) const
{
    return
        key_node.find(key)
        != key_node.end();
}