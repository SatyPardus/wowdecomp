#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "Engine/Source/Base/CDataRecycler.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: a2b3cf86 manual
// OFFSET: 0x0095CD20
CDataRecycler::CDataRecycler(uint32_t nodesPerBlock, int32_t maxNodes) {
    this->m_nodeBlockList = nullptr;
    this->m_nodeFullList = nullptr;
    this->m_nodeEmptyList = nullptr;
    this->m_nodesRecyclable = maxNodes <= 1 ? 1 : maxNodes;
    this->m_nodesPerBlock = nodesPerBlock <= 1 ? 1 : nodesPerBlock;
    this->m_nodesPerBlock = this->m_nodesPerBlock >= static_cast<uint32_t>(this->m_nodesRecyclable) ? this->m_nodesRecyclable : this->m_nodesPerBlock;
}

// VERIFY: d99656c0 auto
// OFFSET: [0x0095CD70, 0x0095CE00]
CDataRecycler::~CDataRecycler() {
}

// VERIFY: 6077ec21 auto
// OFFSET: 0x0095CD80
void* CDataRecycler::AllocData(uint32_t allocBytes, uint32_t* bytes, const char* fileName, int32_t lineNumber) {
    void* data = SMemAlloc(allocBytes, fileName, lineNumber, 0);

    if (bytes) {
        *bytes = allocBytes;
    }

    return data;
}

// VERIFY: 5ada6833 auto
// OFFSET: 0x0095CDB0
void* CDataRecycler::ReallocData(void* data, uint32_t allocBytes, uint32_t* bytes, const char* fileName, int32_t lineNumber) {
    void* result = SMemReAlloc(data, allocBytes, fileName, lineNumber, 0);

    if (bytes) {
        *bytes = allocBytes;
    }

    return result;
}

// VERIFY: bbf3e45c auto
// OFFSET: 0x0095CDE0
void CDataRecycler::FreeData(void* data, const char* fileName, int32_t lineNumber) {
    SMemFree(data, fileName, lineNumber, 0);
}

// VERIFY: da650ee0 manual
// OFFSET: 0x0095CE30
void CDataRecycler::Link(Node** list, NodeBlock* nodeBlock) {
    Node* node = nodeBlock->m_nodes;

    for (uint32_t i = 0; i < this->m_nodesPerBlock - 1; i++) {
        node->m_next = node + 1;
        node++;
    }

    Node* last = &nodeBlock->m_nodes[this->m_nodesPerBlock - 1];
    Node* head;

    do {
        head = *list;
        last->m_next = head;
    } while (InterlockedCompareExchangePointer(reinterpret_cast<void**>(list), nodeBlock->m_nodes, head) != head);
}

// VERIFY: 1f959b49 manual
// OFFSET: 0x0095CE90
void CDataRecycler::Link(void** list, void* item) {
    void* head;

    do {
        head = *list;
        *static_cast<void**>(item) = head;
    } while (InterlockedCompareExchangePointer(list, item, head) != head);
}

// INLINED
void* CDataRecycler::Unlink(void** list) {
    void* item;

    for (item = *list; *list; item = *list) {
        if (InterlockedCompareExchangePointer(list, *static_cast<void**>(item), item) == item) {
            break;
        }
    }

    return item;
}

// VERIFY: a256c569 manual
// OFFSET: 0x0095CEC0
void CDataRecycler::Clear() {
    for (Node* node = static_cast<Node*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeFullList))); node; node = static_cast<Node*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeFullList)))) {
        this->FreeData(node->m_data, nullptr, 0);
    }

    this->m_nodeEmptyList = nullptr;

    for (NodeBlock* block = static_cast<NodeBlock*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeBlockList))); block; block = static_cast<NodeBlock*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeBlockList)))) {
        delete block;
    }
}

// VERIFY: 380c0464 manual
// OFFSET: 0x0095CF40
void CDataRecycler::GetData(void*& data, uint32_t& bytes, const char* fileName, int32_t lineNumber) {
    Node* node = static_cast<Node*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeFullList)));

    if (node) {
        InterlockedIncrement(reinterpret_cast<volatile LONG*>(&this->m_nodesRecyclable));
        data = node->m_data;
        bytes = node->m_bytes;
        this->Link(reinterpret_cast<void**>(&this->m_nodeEmptyList), node);
    } else {
        data = nullptr;
        bytes = 0;
    }
}

// VERIFY: 6af524b4 manual
// OFFSET: 0x0095CFB0
void CDataRecycler::PutData(void* data, uint32_t bytes, const char* fileName, int32_t lineNumber) {
    if (InterlockedDecrement(reinterpret_cast<volatile LONG*>(&this->m_nodesRecyclable)) >= 0) {
        Node* node;

        for (node = static_cast<Node*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeEmptyList))); !node; node = static_cast<Node*>(this->Unlink(reinterpret_cast<void**>(&this->m_nodeEmptyList)))) {
            NodeBlock* block = static_cast<NodeBlock*>(SMemAlloc(sizeof(NodeBlock) + sizeof(Node) * (this->m_nodesPerBlock - 1), __FILE__, __LINE__, 0));
            this->Link(reinterpret_cast<void**>(&this->m_nodeBlockList), block);
            this->Link(&this->m_nodeEmptyList, block);
        }

        node->m_data = data;
        node->m_bytes = bytes;
        this->Link(reinterpret_cast<void**>(&this->m_nodeFullList), node);
    } else {
        InterlockedIncrement(reinterpret_cast<volatile LONG*>(&this->m_nodesRecyclable));
        this->FreeData(data, fileName, lineNumber);
    }
}
