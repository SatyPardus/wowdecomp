#include <cstddef>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <expat.h>
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00D3F710
static XML_Parser s_parser;

// STATIC: 0x00D3F714
static XMLNode* s_freeNodes;

// STATIC: 0x00D3F718
CDataAllocator XMLNode::s_allocator(sizeof(XMLNode), 1170);

// VERIFY: 5d4cbc21 auto
// OFFSET: 0x008145B0
static void end_element(void* userData, const char* name) {
    XMLTree* tree = static_cast<XMLTree*>(userData);
    tree->m_current = tree->m_current->m_parent;
}

// VERIFY: 74b0d33e manual
// OFFSET: 0x008145D0
static void handle_body(void* userData, const char* s, int len) {
    XMLNode* node = static_cast<XMLTree*>(userData)->m_current;

    if (!len) {
        return;
    }

    if (!node->m_body) {
        int32_t i = 0;

        while (i < len) {
            char c = s[i];

            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                break;
            }

            i++;
        }

        if (i >= len) {
            return;
        }
    }

    node->m_body = static_cast<char*>(SMemReAlloc(node->m_body, node->m_bodyLen + len + 1, __FILE__, __LINE__, 0));
    memcpy(&node->m_body[node->m_bodyLen], s, len);
    node->m_bodyLen += len;
    node->m_body[node->m_bodyLen] = '\0';
}

// VERIFY: 39eb725a auto
// OFFSET: 0x00814650
void XMLNode::Detach() {
    if (!this->m_parent) {
        return;
    }

    XMLNode* child = this->m_parent->m_child;

    if (child == this) {
        this->m_parent->m_child = this->m_next;
    } else {
        while (child->m_next != this) {
            child = child->m_next;
        }

        child->m_next = this->m_next;
    }

    this->m_next = nullptr;
    this->m_parent = nullptr;
}

// VERIFY: 5934cd10 auto
// OFFSET: 0x008146A0
void XMLNode::Init(XMLNode* parent, const char* name) {
    this->m_parent = parent;
    this->m_child = nullptr;
    this->m_name.Copy(name);
    this->m_body = nullptr;
    this->m_bodyLen = 0;

    if (this->m_parent && this->m_parent->m_body) {
        this->m_userData = nullptr;
        this->m_next = nullptr;
        this->m_offset = this->m_parent->m_bodyLen;
    } else {
        this->m_offset = 0;
        this->m_userData = nullptr;
        this->m_next = nullptr;
    }
}

// VERIFY: 626eada9 auto
// OFFSET: 0x008146F0
XMLNode* XMLNode::GetChildByName(const char* name) {
    for (XMLNode* child = this->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, name, STORM_MAX_STR)) {
            return child;
        }
    }

    return nullptr;
}

// VERIFY: 106137c0 manual
// OFFSET: 0x00814730
const char* XMLNode::GetAttributeByName(const char* name) {
    uint32_t count = this->m_attributes.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (!SStrCmpI(this->m_attributes[i].m_name.m_str, name, STORM_MAX_STR)) {
            return this->m_attributes[i].m_value.m_str;
        }
    }

    return nullptr;
}

// VERIFY: dac86679 manual
// OFFSET: 0x008147A0
void XMLTree_Cleanup() {
    if (s_parser) {
        XML_ParserFree(s_parser);
        s_parser = nullptr;
    }

    while (s_freeNodes) {
        XMLNode* node = s_freeNodes;
        s_freeNodes = node->m_next;
        node->m_next = nullptr;
        delete node;
    }
}

// VERIFY: e6755991 manual
// OFFSET: 0x00814960
XMLNode::~XMLNode() {
    if (this->m_userData) {
        delete this->m_userData;
    }

    if (this->m_next) {
        delete this->m_next;
    }

    if (this->m_child) {
        delete this->m_child;
    }

    if (this->m_body) {
        SMemFree(this->m_body, __FILE__, __LINE__, 0);
    }
}

// INLINED
void* XMLNode::operator new(size_t bytes) noexcept {
    return XMLNode::s_allocator.GetData(0, StormTypeName(typeid(XMLNode)), SLOG_OBJECT);
}

// INLINED
void XMLNode::operator delete(void* ptr) {
    if (ptr) {
        XMLNode::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: c8b1abf3 manual
// OFFSET: 0x00814AD0
XMLNode::XMLNode(XMLNode* parent, const char* name) {
    this->Init(parent, name);
}

// VERIFY: ee81cbfc manual
// OFFSET: 0x00814B50
static void begin_element(void* userData, const char* name, const char** atts) {
    XMLTree* tree = static_cast<XMLTree*>(userData);
    XMLNode* node = s_freeNodes;

    if (node) {
        s_freeNodes = node->m_next;
        node->Init(tree->m_current, name);
    } else {
        node = new XMLNode(tree->m_current, name);
    }

    XMLNode* parent = tree->m_current;

    if (parent) {
        XMLNode* child = parent->m_child;

        if (child) {
            while (child->m_next) {
                child = child->m_next;
            }

            child->m_next = node;
        } else {
            parent->m_child = node;
        }
    } else {
        tree->m_root = node;
    }

    tree->m_current = node;

    uint32_t count = 0;

    for (const char** att = atts; *att; att += 2) {
        count++;
    }

    node->m_attributes.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        node->m_attributes[i].m_name.Copy(atts[2 * i]);
        node->m_attributes[i].m_value.Copy(atts[2 * i + 1]);
    }
}

// VERIFY: 8b011be3 manual
// OFFSET: 0x00814C40
void XMLNode::SetAttribute(const char* name, const char* value) {
    uint32_t count = this->m_attributes.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (!SStrCmpI(name, this->m_attributes[i].m_name.m_str, STORM_MAX_STR)) {
            this->m_attributes[i].m_value.Copy(value);
            return;
        }
    }

    this->m_attributes.SetCount(count + 1);
    this->m_attributes[count].m_name.Copy(name);
    this->m_attributes[count].m_value.Copy(value);
}

// VERIFY: ec65dbaa manual
// OFFSET: 0x00814CE0
void XMLNode::Recycle() {
    if (this->m_userData) {
        this->m_userData->Release();
        this->m_userData = nullptr;
    }

    if (this->m_next) {
        this->m_next->Recycle();
        this->m_next = nullptr;
    }

    if (this->m_child) {
        this->m_child->Recycle();
        this->m_child = nullptr;
    }

    if (this->m_body) {
        SMemFree(this->m_body, __FILE__, __LINE__, 0);
        this->m_body = nullptr;
    }

    this->m_attributes.Clear();
    this->m_next = s_freeNodes;
    s_freeNodes = this;
}

// VERIFY: 6710e971 manual
// OFFSET: 0x00814D60
void XMLTree_Free(XMLTree* tree) {
    if (tree->m_root) {
        tree->m_root->Recycle();
    }

    SMemFree(tree, __FILE__, __LINE__, 0);
}

// VERIFY: 5504e9d7 manual
// OFFSET: 0x00814D90
XMLTree* XMLTree_Load(const char* data, uint32_t size) {
    XMLTree* tree = static_cast<XMLTree*>(SMemAlloc(sizeof(XMLTree), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY));

    if (s_parser) {
        XML_ParserReset(s_parser, nullptr);
    } else {
        s_parser = XML_ParserCreate(nullptr);
    }

    XML_SetElementHandler(s_parser, begin_element, end_element);
    XML_SetCharacterDataHandler(s_parser, handle_body);
    XML_SetUserData(s_parser, tree);

    if (XML_Parse(s_parser, data, size, 1)) {
        return tree;
    }

    XMLTree_Free(tree);

    return nullptr;
}

// VERIFY: 072e861b auto
// OFFSET: 0x008BE770
XMLNode* XMLTree_GetRoot(XMLTree* tree) {
    return tree->m_root;
}

// VERIFY: d2051799 manual
// OFFSET: 0x009C1820
XMLNodeData::~XMLNodeData() {
}

// VERIFY: 7981ca12 manual
// OFFSET: 0x005EEB70
void XMLNodeData::Release() {
}
