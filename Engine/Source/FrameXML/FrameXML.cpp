#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <typeinfo>
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleCheckbox.h"
#include "Engine/Source/Frame/CSimpleColorSelect.h"
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Frame/CSimpleFont.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleHTML.h"
#include "Engine/Source/Frame/CSimpleMessageFrame.h"
#include "Engine/Source/Frame/CSimpleMessageScrollFrame.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/Frame/CSimpleMovieFrame.h"
#include "Engine/Source/Frame/CSimpleScrollFrame.h"
#include "Engine/Source/Frame/CSimpleSlider.h"
#include "Engine/Source/Frame/CSimpleStatusBar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

// STATIC: 0x00AF5718
static CFrameXMLStatus s_frameXMLStatus;

// STATIC: 0x00D3F6AC
static uint32_t s_progressTotal;

// STATIC: 0x00D3F6B0
static uint32_t s_progressFiles;

// STATIC: 0x00D3F6B4
static void* s_progressParam;

// STATIC: 0x00D3F6B8
static void (*s_progressCallback)(float progress, void* param);

// STATIC: 0x00D3F6BC
static int32_t s_debugLevel;

// STATIC: 0x00D3F6C0
static TSHashTable<HashedNode, HASHKEY_STRI> s_virtualNodes;

// STATIC: 0x00D3F6E8
static TSHashTable<FrameFactoryNode, HASHKEY_STRI> s_factories;

// VERIFY: 3a773c6f auto
// OFFSET: 0x00812530
XMLTree* FrameXML_LoadXML(const char* filename, MD5_CTX* md5, CStatus* status) {
    void* data;
    uint32_t size;

    if (!SFile::ReadFile(nullptr, filename, &data, &size, 0, 1, nullptr)) {
        status->Add(STATUS_ERROR, "Couldn't open %s", filename);
        return nullptr;
    }

    MD5Update(md5, static_cast<const uint8_t*>(data), size);

    XMLTree* tree = XMLTree_Load(static_cast<const char*>(data), size);

    if (!tree) {
        status->Add(STATUS_ERROR, "Couldn't parse XML in %s", filename);
    }

    SFile::Unload(data);

    return tree;
}

// VERIFY: c503afd8 auto
// OFFSET: 0x008125C0
void FrameXML_SetDebugLevel(int32_t level) {
    s_debugLevel = level;
}

// VERIFY: b84c8d55 auto
// OFFSET: 0x008125D0
int32_t FrameXML_GetDebugLevel() {
    return s_debugLevel;
}

// VERIFY: 4231d5bb auto
// OFFSET: 0x008125E0
int32_t FrameXML_GuessNumFiles(const char* toc) {
    int32_t count = 0;
    const char* current = toc;

    while (*current) {
        while (*current == '\r' || *current == '\n') {
            current++;
        }

        char c = *current;

        if (!c) {
            break;
        }

        if (c != '#') {
            count++;
        }

        while (c != '\r' && c != '\n') {
            c = *++current;

            if (!c) {
                return count;
            }
        }
    }

    return count;
}

// VERIFY: c88573f9 auto
// OFFSET: 0x00812630
CSimpleFrame* Create_SimpleCheckButton(CSimpleFrame* parent) {
    void* data = CSimpleCheckbox::s_allocator.GetData(0, typeid(CSimpleCheckbox).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleCheckbox(parent) : nullptr;
}

// VERIFY: e0d76346 auto
// OFFSET: 0x00812660
CSimpleFrame* Create_SimpleEditBox(CSimpleFrame* parent) {
    void* data = CSimpleEditBox::s_allocator.GetData(0, typeid(CSimpleEditBox).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleEditBox(parent) : nullptr;
}

// VERIFY: 2aa2d739 auto
// OFFSET: 0x00812690
CSimpleFrame* Create_SimpleMessageFrame(CSimpleFrame* parent) {
    void* data = CSimpleMessageFrame::s_allocator.GetData(0, typeid(CSimpleMessageFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleMessageFrame(parent) : nullptr;
}

// VERIFY: 9ec2bd5d auto
// OFFSET: 0x008126C0
CSimpleFrame* Create_SimpleModel(CSimpleFrame* parent) {
    void* data = CSimpleModel::s_allocator.GetData(0, typeid(CSimpleModel).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleModel(parent) : nullptr;
}

// VERIFY: d93ae8f1 auto
// OFFSET: 0x008126F0
CSimpleFrame* Create_SimpleScrollFrame(CSimpleFrame* parent) {
    void* data = CSimpleScrollFrame::s_allocator.GetData(0, typeid(CSimpleScrollFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleScrollFrame(parent) : nullptr;
}

// VERIFY: 2ba95665 auto
// OFFSET: 0x00812720
CSimpleFrame* Create_SimpleScrollingMessageFrame(CSimpleFrame* parent) {
    void* data = CSimpleMessageScrollFrame::s_allocator.GetData(0, typeid(CSimpleMessageScrollFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleMessageScrollFrame(parent, 8) : nullptr;
}

// VERIFY: 1a3fde32 auto
// OFFSET: 0x00812750
CSimpleFrame* Create_SimpleSlider(CSimpleFrame* parent) {
    void* data = CSimpleSlider::s_allocator.GetData(0, typeid(CSimpleSlider).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleSlider(parent) : nullptr;
}

// VERIFY: 4bbcca61 auto
// OFFSET: 0x00812780
CSimpleFrame* Create_SimpleHTML(CSimpleFrame* parent) {
    void* data = CSimpleHTML::s_allocator.GetData(0, typeid(CSimpleHTML).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleHTML(parent) : nullptr;
}

// VERIFY: 7fb244de auto
// OFFSET: 0x008127B0
CSimpleFrame* Create_SimpleStatusBar(CSimpleFrame* parent) {
    void* data = CSimpleStatusBar::s_allocator.GetData(0, typeid(CSimpleStatusBar).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleStatusBar(parent) : nullptr;
}

// VERIFY: dd0cf87c auto
// OFFSET: 0x008127E0
CSimpleFrame* Create_SimpleColorSelect(CSimpleFrame* parent) {
    void* data = CSimpleColorSelect::s_allocator.GetData(0, typeid(CSimpleColorSelect).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleColorSelect(parent) : nullptr;
}

// VERIFY: b85fa974 auto
// OFFSET: 0x00812810
CSimpleFrame* Create_SimpleMovieFrame(CSimpleFrame* parent) {
    void* data = CSimpleMovieFrame::s_allocator.GetData(0, typeid(CSimpleMovieFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleMovieFrame(parent) : nullptr;
}

// VERIFY: 1562cfb0 auto
// OFFSET: 0x00812840
void FrameXML_RegisterLoadProgressCallback(void (*callback)(float progress, void* param), void* param, uint32_t total) {
    s_progressCallback = callback;
    s_progressParam = param;
    s_progressFiles = 0;
    s_progressTotal = total;
}

// VERIFY: 36e2fb88 auto
// OFFSET: 0x00812870
CSimpleFrame* Create_SimpleButton(CSimpleFrame* parent) {
    void* data = CSimpleButton::s_allocator.GetData(0, typeid(CSimpleButton).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleButton(parent) : nullptr;
}

// VERIFY: 8993fe13 auto
// OFFSET: 0x008128A0
CSimpleFrame* Create_SimpleFrame(CSimpleFrame* parent) {
    void* data = CSimpleFrame::s_allocator.GetData(0, typeid(CSimpleFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CSimpleFrame(parent) : nullptr;
}

// VERIFY: f5a4d98a auto
// OFFSET: 0x00812B90
void FrameXML_FreeHashNodes() {
    s_virtualNodes.InternalClear(0);
}

// VERIFY: b51e4fcc auto
// OFFSET: 0x00812BA0
void FrameXML_ClearFactories() {
    s_factories.InternalClear(0);
    XMLTree_Cleanup();
}

// VERIFY: d21a2d6d manual
// OFFSET: 0x00812CE0
XMLNode* FrameXML_AcquireHashNode(const char* name, const char*& taint, bool& locked) {
    HashedNode* hashed = s_virtualNodes.Ptr(name);

    if (!hashed) {
        return nullptr;
    }

    locked = hashed->m_locked;
    taint = hashed->m_taint;
    hashed->m_locked = true;

    return hashed->m_node;
}

// VERIFY: e53052f6 auto
// OFFSET: 0x00812D20
void FrameXML_ReleaseHashNode(const char* name) {
    s_virtualNodes.Ptr(name)->m_locked = false;
}

// VERIFY: 743ec1d8 auto
// OFFSET: 0x00812F60
void CFrameXMLStatus::Add(STATUS_TYPE severity, const char* format, ...) {
    char buffer[512];

    va_list args;
    va_start(args, format);
    SStrVPrintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DebugLog(buffer);
}

// VERIFY: e9a4b97a manual
// OFFSET: 0x00812FA0
CSimpleFrame* FrameXML_CreateFrame(XMLNode* node, CSimpleFrame* parent, CStatus* status) {
    if (s_debugLevel > 0) {
        const char* frameName = node->GetAttributeByName("name");

        if (frameName && *frameName) {
            status->Add(STATUS_INFO, "-- Creating %s named %s", node->m_name.m_str, frameName);
        } else {
            status->Add(STATUS_INFO, "-- Creating unnamed %s", node->m_name.m_str);
        }
    }

    FrameFactoryNode* factory = s_factories.Ptr(node->m_name.m_str);

    if (!factory) {
        // Not in the original: the element type is in no factory table.
        sExtensionMgr->OnUnknownFrameType(node->m_name.m_str, node);

        status->Add(STATUS_WARNING, "Unknown frame type: %s", node->m_name.m_str);
        return nullptr;
    }

    const char* parentName = node->GetAttributeByName("parent");

    if (!parentName) {
        XMLNode* current = node;

        do {
            const char* inherits = current->GetAttributeByName("inherits");

            if (!inherits || !*inherits) {
                break;
            }

            char inheritName[1024];

            do {
                SStrTokenize(&inherits, inheritName, sizeof(inheritName), " ,", nullptr);

                if (!inheritName[0]) {
                    break;
                }

                HashedNode* hashed = s_virtualNodes.Ptr(inheritName);

                if (hashed) {
                    current = hashed->m_node;
                    bool locked = hashed->m_locked;
                    hashed->m_locked = true;

                    if (current && !locked) {
                        parentName = current->GetAttributeByName("parent");
                        s_virtualNodes.Ptr(inheritName)->m_locked = false;
                    }
                } else {
                    current = nullptr;
                }
            } while (inheritName[0]);

            if (parentName) {
                break;
            }
        } while (current);
    }

    if (parentName && *parentName) {
        if (!CSimpleFrame::s_objectType) {
            CSimpleFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
        }

        parent = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(parentName, CSimpleFrame::s_objectType));

        if (!parent) {
            status->Add(STATUS_WARNING, "Couldn't find frame parent: %s", parentName);
        }
    }

    CSimpleFrame* frame = factory->m_factory(parent);

    if (!frame) {
        status->Add(STATUS_WARNING, "Unable to create frame type: %s", node->m_name.m_str);
        return nullptr;
    }

    if (factory->m_unique) {
        s_factories.Delete(factory);
    }

    frame->PreLoadXML(node, status);
    frame->LoadXML(node, status);
    frame->PostLoadXML(node, status);

    // Not in the original: the frame is built from its element.
    sExtensionMgr->OnFrameCreated(frame, node);

    return frame;
}

// VERIFY: 50a4650d manual
// OFFSET: 0x00813BE0
void FrameXML_StoreHashNode(XMLNode* node, const char* name, const char* taint, CStatus* status) {
    HashedNode* hashed = s_virtualNodes.Ptr(name);

    if (hashed) {
        if (!hashed->m_taint || taint) {
            status->Add(STATUS_WARNING, "Virtual object named %s already exists", name);
            return;
        }

        if (hashed->m_node) {
            delete hashed->m_node;
        }
    } else {
        hashed = s_virtualNodes.New(name, 0, 0);
    }

    hashed->m_node = node;
    hashed->m_taint = taint;
    node->Detach();

    if (s_debugLevel > 0) {
        status->Add(STATUS_INFO, "-- Added virtual frame %s", name);
    }
}

// VERIFY: 56caf1af manual
// OFFSET: 0x00813D20
int32_t FrameXML_RegisterFactory(const char* name, CSimpleFrame* (*factory)(CSimpleFrame* parent), bool unique) {
    if (s_factories.Ptr(name)) {
        return 0;
    }

    FrameFactoryNode* node = s_factories.New(name, 0, 0);
    node->m_factory = factory;
    node->m_unique = unique;

    return 1;
}

// VERIFY: 99b277ec auto
// OFFSET: 0x00813DF0
int32_t FrameXML_RegisterDefault() {
    FrameXML_RegisterFactory("Button", Create_SimpleButton, false);
    FrameXML_RegisterFactory("CheckButton", Create_SimpleCheckButton, false);
    FrameXML_RegisterFactory("EditBox", Create_SimpleEditBox, false);
    FrameXML_RegisterFactory("Frame", Create_SimpleFrame, false);
    FrameXML_RegisterFactory("MessageFrame", Create_SimpleMessageFrame, false);
    FrameXML_RegisterFactory("Model", Create_SimpleModel, false);
    FrameXML_RegisterFactory("ScrollFrame", Create_SimpleScrollFrame, false);
    FrameXML_RegisterFactory("ScrollingMessageFrame", Create_SimpleScrollingMessageFrame, false);
    FrameXML_RegisterFactory("Slider", Create_SimpleSlider, false);
    FrameXML_RegisterFactory("SimpleHTML", Create_SimpleHTML, false);
    FrameXML_RegisterFactory("StatusBar", Create_SimpleStatusBar, false);
    FrameXML_RegisterFactory("ColorSelect", Create_SimpleColorSelect, false);
    FrameXML_RegisterFactory("MovieFrame", Create_SimpleMovieFrame, false);

    return 1;
}

// VERIFY: 0ad0cd1b manual
// OFFSET: 0x00813EE0
int32_t FrameXML_ProcessFile(const char* filename, const char* wrapper, MD5_CTX* md5, CStatus* status) {
    char cleanPath[260];

    if (SStrStr(filename, "..")) {
        SStrCopy(cleanPath, filename, sizeof(cleanPath));

        for (char* dots = SStrStr(cleanPath, ".."); dots; dots = SStrStr(cleanPath, "..")) {
            char before = dots[-1];

            if (before != '\\' && before != '/') {
                break;
            }

            char after = dots[2];

            if (after != '\\' && after != '/') {
                break;
            }

            char* dir = dots - 2;

            while (dir >= cleanPath && *dir != '\\' && *dir != '/') {
                dir--;
            }

            SStrCopy(dir + 1, dots + 3, STORM_MAX_STR);
        }

        filename = cleanPath;
    }

    const char* extension = SStrChrR(const_cast<char*>(filename), '.');

    if (extension && !SStrCmpI(extension, ".lua", STORM_MAX_STR)) {
        return FrameScript_ExecuteFile(filename, wrapper, md5, status);
    }

    XMLTree* tree = FrameXML_LoadXML(filename, md5, status);

    if (!tree) {
        return 0;
    }

    CStatus fileStatus;
    XMLNode* child = XMLTree_GetRoot(tree)->m_child;

    if (child) {
        XMLNode* next;

        do {
            next = child->m_next;

            if (!SStrCmpI(child->m_name.m_str, "Include", STORM_MAX_STR)) {
                const char* file = child->GetAttributeByName("file");

                if (file) {
                    char includePath[260];
                    const char* slash = SStrChrR(const_cast<char*>(filename), '\\');

                    if (slash) {
                        size_t length = slash - filename + 1;

                        if (length < sizeof(includePath)) {
                            SStrCopy(includePath, filename, sizeof(includePath));
                            includePath[length] = '\0';
                            SStrPack(includePath, file, sizeof(includePath));
                        }
                    } else {
                        SStrCopy(includePath, file, sizeof(includePath));
                    }

                    FrameXML_ProcessFile(includePath, wrapper, md5, &fileStatus);
                } else {
                    fileStatus.Add(STATUS_ERROR, "Element 'Include' without file attribute");
                }
            } else if (!SStrCmpI(child->m_name.m_str, "Script", STORM_MAX_STR)) {
                const char* file = child->GetAttributeByName("file");

                if (file) {
                    char scriptPath[260];
                    const char* slash = SStrChrR(const_cast<char*>(filename), '\\');

                    if (slash) {
                        size_t length = slash - filename + 1;

                        if (length < sizeof(scriptPath)) {
                            SStrCopy(scriptPath, filename, sizeof(scriptPath));
                            scriptPath[length] = '\0';
                            SStrPack(scriptPath, file, sizeof(scriptPath));
                        }
                    } else {
                        SStrCopy(scriptPath, file, sizeof(scriptPath));
                    }

                    FrameScript_ExecuteFile(scriptPath, wrapper, md5, &fileStatus);
                }

                const char* body = child->m_body;

                if (body && *body) {
                    char scriptName[271];
                    SStrPrintf(scriptName, sizeof(scriptName), "%s:<Scripts>", filename);
                    FrameScript_Execute(body, scriptName, lua_tainted);
                }
            } else if (!SStrCmpI(child->m_name.m_str, "Font", STORM_MAX_STR)) {
                const char* fontName = child->GetAttributeByName("name");

                if (fontName && *fontName) {
                    CSimpleFont* font = CSimpleFont::GetFont(fontName, 1);
                    font->LoadXML(child, status);
                } else {
                    fileStatus.Add(STATUS_WARNING, "Unnamed font node at top level");
                }
            } else {
                const char* isVirtual = child->GetAttributeByName("virtual");

                if (isVirtual && !SStrCmpI(isVirtual, "true", STORM_MAX_STR)) {
                    const char* virtualName = child->GetAttributeByName("name");

                    if (virtualName && *virtualName) {
                        FrameXML_StoreHashNode(child, virtualName, lua_tainted, &fileStatus);
                    } else {
                        fileStatus.Add(STATUS_WARNING, "Unnamed virtual node at top level");
                    }
                } else {
                    FrameXML_CreateFrame(child, nullptr, &fileStatus);
                    CLayoutFrame::ResizePending();
                }
            }

            child = next;
        } while (next);
    }

    XMLTree_Free(tree);

    if (s_debugLevel > 0 || fileStatus.m_highestSeverity > STATUS_INFO) {
        fileStatus.Prepend(STATUS_INFO, "++ Loading file %s", filename);
    }

    status->Add(fileStatus);

    return 1;
}

// VERIFY: a8106274 manual
// OFFSET: 0x00814340
int32_t FrameXML_CreateFrames(const char* tocFile, const char* wrapper, MD5_CTX* md5, CStatus* status) {
    if (!status) {
        status = &s_frameXMLStatus;
    }

    char tocPath[260];
    memset(tocPath, 0, sizeof(tocPath));

    const char* slash = SStrChrR(const_cast<char*>(tocFile), '\\');

    if (slash) {
        size_t length = slash - tocFile + 1;

        if (length < sizeof(tocPath) - 1) {
            SStrCopy(tocPath, tocFile, length + 1);
            tocPath[length] = '\0';
        }
    }

    void* data;
    uint32_t size;

    if (!SFile::ReadFile(nullptr, tocFile, &data, &size, 1, 1, nullptr)) {
        status->Add(STATUS_ERROR, "Couldn't open %s", tocFile);
        return 0;
    }

    MD5Update(md5, static_cast<const uint8_t*>(data), size);

    const char* text = static_cast<const char*>(data);

    if (static_cast<uint8_t>(text[0]) == 0xEF && static_cast<uint8_t>(text[1]) == 0xBB && static_cast<uint8_t>(text[2]) == 0xBF) {
        text += 3;
    }

    CStatus tocStatus;
    char line[1024];

    do {
        SStrTokenize(&text, line, sizeof(line), "\r\n", nullptr);

        if (!line[0]) {
            break;
        }

        if (line[0] != '#') {
            char path[260];
            SStrCopy(path, tocPath, sizeof(path));
            SStrPack(path, line, sizeof(path));

            size_t length = SStrLen(path);
            char* end = &path[length];

            if (path[length - 1] == ' ') {
                do {
                    if (end <= path) {
                        break;
                    }

                    end--;
                } while (end[-1] == ' ');
            }

            *end = '\0';

            FrameXML_ProcessFile(path, wrapper, md5, &tocStatus);

            if (s_progressCallback && s_progressFiles < s_progressTotal) {
                s_progressFiles++;
                s_progressCallback(static_cast<float>(s_progressFiles) / static_cast<float>(s_progressTotal), s_progressParam);
            }
        }
    } while (line[0]);

    XMLTree_Cleanup();
    SFile::Unload(data);

    if (s_debugLevel > 0 || tocStatus.m_highestSeverity > STATUS_INFO) {
        tocStatus.Prepend(STATUS_INFO, "** Loading table of contents %s", tocFile);
    }

    status->Add(tocStatus);

    return 1;
}
