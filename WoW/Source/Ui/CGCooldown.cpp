#include "CGCooldown.h"
#include <cmath>
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00C2423C
int32_t CGCooldown::s_objectType;

// STATIC: 0x00AD13CC
int32_t CGCooldown::s_metatable = LUA_NOREF;

// STATIC: 0x00AD1398
const char* CGCooldown::s_objectTypeName = "Cooldown";

// STATIC: 0x00AD139C
const char* CGCooldown::s_starTextureName = "interface\\cooldown\\star4.blp";

// STATIC: 0x00AD13A0
const char* CGCooldown::s_edgeTextureName = "interface\\cooldown\\edge.blp";

// STATIC: 0x00AD13D0
const uint16_t CGCooldown::s_edgeIndices[6] = { 0, 2, 1, 1, 2, 3 };

// STATIC: 0x00AD13DC
const uint16_t CGCooldown::s_flashIndices[6] = { 0, 2, 1, 1, 2, 3 };

// STATIC: 0x00AD13E8
const uint16_t CGCooldown::s_starIndices[24] = { 0, 9, 1, 1, 9, 2, 2, 9, 3, 3, 9, 4, 4, 9, 5, 5, 9, 6, 6, 9, 7, 7, 9, 8 };

// STATIC: 0x00AD1418
float CGCooldown::s_flashScale[7] = { 1.0f, 0.6993007f, 0.5405405f, 0.6289308f, 0.7518797f, 0.625f, 0.8695652f };

// STATIC: 0x00AD1434
float CGCooldown::s_flashAlpha[7] = { 0.0f, 0.5f, 1.0f, 0.75f, 0.5f, 0.25f, 0.0f };

// STATIC: 0x00C24254
float CGCooldown::s_flashRotation[7] = { 0.0f, -0.1303761f, -0.25918141f, -0.39269909f, -0.51836282f, -0.6518805f, -0.78539819f };

// STATIC: 0x00C24240
CDataAllocator CGCooldown::s_cooldownHeap(sizeof(CGCooldown), 100);

// INLINED
void CGCooldown::operator delete(void* ptr) {
    if (ptr) {
        CGCooldown::s_cooldownHeap.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CGCooldown::GetObjectType() {
    if (!CGCooldown::s_objectType) {
        CGCooldown::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CGCooldown::s_objectType;
}

// VERIFY: 61bceefd auto
// OFFSET: 0x0051A380
CSimpleFrame* CGCooldown::Create(CSimpleFrame* parent) {
    void* data = CGCooldown::s_cooldownHeap.GetData(0, STORM_TYPE_NAME(CGCooldown), SLOG_OBJECT);
    return data ? new (data) CGCooldown(parent) : nullptr;
}

// VERIFY: 8c34e1ec auto
// OFFSET: [0x005EB040, 0x005EBE60, 0x005EBEF0]
CGCooldown::~CGCooldown() {
    if (this->m_starTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_starTexture));
    }

    if (this->m_edgeTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_edgeTexture));
    }

    this->m_starTexture = nullptr;
    this->m_edgeTexture = nullptr;
}

// VERIFY: 023e496a auto
// OFFSET: 0x005EB0A0
const char* CGCooldown::GetObjectTypeName() {
    return CGCooldown::s_objectTypeName;
}

// VERIFY: ec45d64e auto
// OFFSET: 0x005EB0B0
int32_t CGCooldown::GetScriptMetaTable() {
    return CGCooldown::s_metatable;
}

// VERIFY: 3469766b manual
// OFFSET: 0x005EB130
void CGCooldown::AnimateOctants01(uint32_t octant, float t) {
    float cx = (this->m_rect.minX + this->m_rect.maxX) * 0.5f;
    float cy = 0.5f * (this->m_rect.maxY + this->m_rect.minY);

    if (this->m_reverse) {
        if (octant > 0) {
            this->m_starVerts[2].y = this->m_rect.maxY - (this->m_rect.maxY - cy) * t;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;

            for (uint32_t i = 3; i < 9; i++) {
                this->m_starVerts[i].x = this->m_rect.maxX;
                this->m_starVerts[i].y = this->m_starVerts[1].y;
            }
        } else {
            float x = cx + (this->m_rect.maxX - cx) * t;
            this->m_starVerts[1].x = x;
            this->m_starVerts[1].y = this->m_rect.maxY;
            this->m_starVerts[2].x = x;
            this->m_starVerts[2].y = this->m_rect.maxY;

            for (uint32_t i = 3; i < 9; i++) {
                this->m_starVerts[i].x = this->m_starVerts[1].x;
                this->m_starVerts[i].y = this->m_rect.maxY;
            }
        }
    } else {
        if (octant > 0) {
            this->m_starVerts[1].y = this->m_rect.maxY - (this->m_rect.maxY - cy) * t;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[0] = this->m_starVerts[1];
        } else {
            this->m_starVerts[0].x = (this->m_rect.maxX - cx) * t + cx;
            this->m_starVerts[0].y = this->m_rect.maxY;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;
        }

        this->m_starVerts[2].x = this->m_rect.maxX;
        this->m_starVerts[2].y = cy;
        this->m_starVerts[3].x = this->m_rect.maxX;
        this->m_starVerts[3].y = this->m_rect.minY;
        this->m_starVerts[4].x = cx;
        this->m_starVerts[4].y = this->m_rect.minY;
        this->m_starVerts[5].x = this->m_rect.minX;
        this->m_starVerts[5].y = this->m_rect.minY;
        this->m_starVerts[6].x = this->m_rect.minX;
        this->m_starVerts[6].y = cy;
        this->m_starVerts[7].x = this->m_rect.minX;
        this->m_starVerts[7].y = this->m_rect.maxY;
    }
}

// VERIFY: 5b558938 manual
// OFFSET: 0x005EB3A0
void CGCooldown::AnimateOctants23(uint32_t octant, float t) {
    float cx = (this->m_rect.minX + this->m_rect.maxX) * 0.5f;
    float cy = 0.5f * (this->m_rect.maxY + this->m_rect.minY);

    if (this->m_reverse) {
        if (octant >= 3) {
            float x = this->m_rect.maxX - (this->m_rect.maxX - cx) * t;
            this->m_starVerts[4].x = x;
            this->m_starVerts[4].y = this->m_rect.minY;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[2].y = cy;
            this->m_starVerts[3].x = this->m_rect.maxX;
            this->m_starVerts[3].y = this->m_rect.minY;
            this->m_starVerts[5].x = x;
            this->m_starVerts[5].y = this->m_rect.minY;

            for (uint32_t i = 6; i < 9; i++) {
                this->m_starVerts[i].x = this->m_starVerts[4].x;
                this->m_starVerts[i].y = this->m_rect.minY;
            }
        } else {
            this->m_starVerts[3].y = cy - (cy - this->m_rect.minY) * t;
            this->m_starVerts[3].x = this->m_rect.maxX;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[2].y = cy;

            for (uint32_t i = 4; i < 9; i++) {
                this->m_starVerts[i].x = this->m_rect.maxX;
                this->m_starVerts[i].y = this->m_starVerts[3].y;
            }
        }
    } else {
        if (octant >= 3) {
            this->m_starVerts[3].x = this->m_rect.maxX - (this->m_rect.maxX - cx) * t;
            this->m_starVerts[3].y = this->m_rect.minY;
            this->m_starVerts[0] = this->m_starVerts[3];
            this->m_starVerts[1] = this->m_starVerts[3];
            this->m_starVerts[2] = this->m_starVerts[3];
        } else {
            this->m_starVerts[2].y = cy - (cy - this->m_rect.minY) * t;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[0] = this->m_starVerts[2];
            this->m_starVerts[1] = this->m_starVerts[2];
            this->m_starVerts[3].x = this->m_rect.maxX;
            this->m_starVerts[3].y = this->m_rect.minY;
        }

        this->m_starVerts[4].x = cx;
        this->m_starVerts[4].y = this->m_rect.minY;
        this->m_starVerts[5].x = this->m_rect.minX;
        this->m_starVerts[5].y = this->m_rect.minY;
        this->m_starVerts[6].x = this->m_rect.minX;
        this->m_starVerts[6].y = cy;
        this->m_starVerts[7].x = this->m_rect.minX;
        this->m_starVerts[7].y = this->m_rect.maxY;
    }
}

// VERIFY: 2445640a auto
// OFFSET: 0x005EB670
void CGCooldown::AnimateOctants45(uint32_t octant, float t) {
    float cx = (this->m_rect.minX + this->m_rect.maxX) * 0.5f;
    float cy = 0.5f * (this->m_rect.maxY + this->m_rect.minY);

    if (this->m_reverse) {
        if (octant >= 5) {
            this->m_starVerts[6].y = (cy - this->m_rect.minY) * t + this->m_rect.minY;
            this->m_starVerts[6].x = this->m_rect.minX;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[2].y = cy;
            this->m_starVerts[3].x = this->m_rect.maxX;
            this->m_starVerts[3].y = this->m_rect.minY;
            this->m_starVerts[4].x = cx;
            this->m_starVerts[4].y = this->m_rect.minY;
            this->m_starVerts[5].x = this->m_rect.minX;
            this->m_starVerts[5].y = this->m_rect.minY;
            this->m_starVerts[7].x = this->m_rect.minX;
            this->m_starVerts[7].y = this->m_starVerts[6].y;
            this->m_starVerts[8].x = this->m_rect.minX;
            this->m_starVerts[8].y = this->m_starVerts[6].y;
        } else {
            float x = cx - (cx - this->m_rect.minX) * t;
            this->m_starVerts[5].x = x;
            this->m_starVerts[5].y = this->m_rect.minY;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[2].y = cy;
            this->m_starVerts[3].x = this->m_rect.maxX;
            this->m_starVerts[3].y = this->m_rect.minY;
            this->m_starVerts[4].x = cx;
            this->m_starVerts[4].y = this->m_rect.minY;
            this->m_starVerts[6].x = x;
            this->m_starVerts[6].y = this->m_rect.minY;
            this->m_starVerts[7].x = this->m_starVerts[5].x;
            this->m_starVerts[7].y = this->m_rect.minY;
            this->m_starVerts[8].x = this->m_starVerts[5].x;
            this->m_starVerts[8].y = this->m_rect.minY;
        }
    } else {
        if (octant >= 5) {
            this->m_starVerts[5].y = (cy - this->m_rect.minY) * t + this->m_rect.minY;
            this->m_starVerts[5].x = this->m_rect.minX;

            for (uint32_t i = 0; i < 5; i++) {
                this->m_starVerts[i] = this->m_starVerts[5];
            }
        } else {
            this->m_starVerts[4].x = cx - (cx - this->m_rect.minX) * t;
            this->m_starVerts[4].y = this->m_rect.minY;

            for (uint32_t i = 0; i < 4; i++) {
                this->m_starVerts[i] = this->m_starVerts[4];
            }

            this->m_starVerts[5].x = this->m_rect.minX;
            this->m_starVerts[5].y = this->m_rect.minY;
        }

        this->m_starVerts[6].x = this->m_rect.minX;
        this->m_starVerts[6].y = cy;
        this->m_starVerts[7].x = this->m_rect.minX;
        this->m_starVerts[7].y = this->m_rect.maxY;
    }
}

// VERIFY: 49957598 auto
// OFFSET: 0x005EB9A0
void CGCooldown::AnimateOctants67(uint32_t octant, float t) {
    float cx = (this->m_rect.minX + this->m_rect.maxX) * 0.5f;
    float cy = 0.5f * (this->m_rect.maxY + this->m_rect.minY);

    if (this->m_reverse) {
        if (octant < 7) {
            this->m_starVerts[7].y = (this->m_rect.maxY - cy) * t + cy;
            this->m_starVerts[7].x = this->m_rect.minX;
            this->m_starVerts[1].x = this->m_rect.maxX;
            this->m_starVerts[1].y = this->m_rect.maxY;
            this->m_starVerts[2].x = this->m_rect.maxX;
            this->m_starVerts[2].y = cy;
            this->m_starVerts[3].x = this->m_rect.maxX;
            this->m_starVerts[3].y = this->m_rect.minY;
            this->m_starVerts[4].x = cx;
            this->m_starVerts[4].y = this->m_rect.minY;
            this->m_starVerts[5].x = this->m_rect.minX;
            this->m_starVerts[5].y = this->m_rect.minY;
            this->m_starVerts[6].x = this->m_rect.minX;
            this->m_starVerts[6].y = cy;
            this->m_starVerts[8].x = this->m_rect.minX;
            this->m_starVerts[8].y = this->m_starVerts[7].y;
            return;
        }

        this->m_starVerts[8].y = this->m_rect.maxY;
        this->m_starVerts[8].x = (cx - this->m_rect.minX) * t + this->m_rect.minX;
        this->m_starVerts[1].x = this->m_rect.maxX;
        this->m_starVerts[1].y = this->m_rect.maxY;
        this->m_starVerts[2].x = this->m_rect.maxX;
        this->m_starVerts[2].y = cy;
        this->m_starVerts[3].x = this->m_rect.maxX;
        this->m_starVerts[3].y = this->m_rect.minY;
        this->m_starVerts[4].x = cx;
        this->m_starVerts[4].y = this->m_rect.minY;
        this->m_starVerts[5].x = this->m_rect.minX;
        this->m_starVerts[5].y = this->m_rect.minY;
        this->m_starVerts[6].x = this->m_rect.minX;
        this->m_starVerts[6].y = cy;
        this->m_starVerts[7].x = this->m_rect.minX;
        this->m_starVerts[7].y = this->m_rect.maxY;
        return;
    }

    if (octant < 7) {
        this->m_starVerts[6].y = cy + (this->m_rect.maxY - cy) * t;
        this->m_starVerts[6].x = this->m_rect.minX;

        for (uint32_t i = 0; i < 6; i++) {
            this->m_starVerts[i] = this->m_starVerts[6];
        }

        this->m_starVerts[7].x = this->m_rect.minX;
        this->m_starVerts[7].y = this->m_rect.maxY;
        return;
    }

    this->m_starVerts[7].x = (cx - this->m_rect.minX) * t + this->m_rect.minX;
    this->m_starVerts[7].y = this->m_rect.maxY;

    for (uint32_t i = 0; i < 7; i++) {
        this->m_starVerts[i] = this->m_starVerts[7];
    }
}

// VERIFY: a9c00058 auto
// OFFSET: 0x005EB0C0
void CGCooldown::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    const char* reverse = node->GetAttributeByName("reverse");

    if (reverse && *reverse) {
        this->m_reverse = StringToBOOL(reverse);
    }

    const char* drawEdge = node->GetAttributeByName("drawEdge");

    if (drawEdge && *drawEdge) {
        this->m_drawEdge = StringToBOOL(drawEdge);
    }
}

// VERIFY: 3aaa4202 auto
// OFFSET: 0x005EBD20
void CGCooldown::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    CSimpleFrame::OnFrameRender(batch, layer);

    CGxShader* shader = CSimpleTexture::GetImageModePixelShader(ImageMode_Normal);

    if (layer == 2) {
        if (this->m_starTexture) {
            batch->Queue(this->m_starTexture, GxBlend_Alpha, 10, this->m_starVerts, this->m_starTexCoords, 0, &this->m_starColor, 24, CGCooldown::s_starIndices, shader);
            batch->Queue(this->m_starTexture, GxBlend_Add, 4, this->m_flashVerts, this->m_flashTexCoords, 0, &this->m_flashColor, 6, CGCooldown::s_flashIndices, shader);
        }
    } else if (layer == 3) {
        if (this->m_edgeTexture && this->m_drawEdge) {
            batch->Queue(this->m_edgeTexture, GxBlend_Opaque, 4, this->m_edgeVerts, this->m_edgeTexCoords, 0, &this->m_edgeColor, 6, CGCooldown::s_edgeIndices, shader);
        }
    }
}

// OFFSET: 0x005EBE70
bool CGCooldown::IsA(int32_t type) {
    return type == CGCooldown::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// OFFSET: 0x005EBEB0
bool CGCooldown::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CGCooldown::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: e5cabff8 manual
// OFFSET: 0x005EBF20
void CGCooldown::ResetVerts() {
    for (uint32_t i = 0; i < 9; i++) {
        this->m_starVerts[i] = C3Vector(0.0f, 0.0f, 1.0f);
    }

    this->m_flashVerts[0] = C3Vector(this->m_rect.minX, this->m_rect.maxY, 1.0f);
    this->m_flashTexCoords[0].x = 0.0f;
    this->m_flashTexCoords[0].y = 0.0f;
    this->m_flashVerts[1] = C3Vector(this->m_rect.maxX, this->m_rect.maxY, 1.0f);
    this->m_flashTexCoords[1].x = 1.0f;
    this->m_flashTexCoords[1].y = 0.0f;
    this->m_flashVerts[2] = C3Vector(this->m_rect.minX, this->m_rect.minY, 1.0f);
    this->m_flashTexCoords[2].y = 1.0f;
    this->m_flashTexCoords[2].x = 0.0f;
    this->m_flashVerts[3] = C3Vector(this->m_rect.maxX, this->m_rect.minY, 1.0f);
    this->m_flashTexCoords[3].x = 1.0f;
    this->m_flashTexCoords[3].y = 1.0f;

    if (this->m_drawEdge) {
        this->m_edgeVerts[0] = C3Vector(this->m_rect.minX, this->m_rect.maxY, 1.0f);
        this->m_edgeTexCoords[0].x = 0.0f;
        this->m_edgeTexCoords[0].y = 0.0f;
        this->m_edgeVerts[1] = C3Vector(this->m_rect.maxX, this->m_rect.maxY, 1.0f);
        this->m_edgeTexCoords[1].x = 1.0f;
        this->m_edgeTexCoords[1].y = 0.0f;
        this->m_edgeVerts[2] = C3Vector(this->m_rect.minX, this->m_rect.minY, 1.0f);
        this->m_edgeTexCoords[2].y = 1.0f;
        this->m_edgeTexCoords[2].x = 0.0f;
        this->m_edgeVerts[3] = C3Vector(this->m_rect.maxX, this->m_rect.minY, 1.0f);
        this->m_edgeTexCoords[3].x = 1.0f;
        this->m_edgeTexCoords[3].y = 1.0f;
    }
}

// VERIFY: a663a649 manual
// OFFSET: 0x005EC1E0
void CGCooldown::AnimateVerts() {
    if (!this->m_flashing) {
        float elapsed = static_cast<float>(this->m_elapsed);
        float duration = static_cast<float>(this->m_duration);
        float octantTime = duration * 0.125f;
        float invOctantTime = 1.0f / octantTime;
        int32_t octant = static_cast<int32_t>(invOctantTime * elapsed);
        float t = invOctantTime * (elapsed - octantTime * static_cast<float>(octant));
        float cx = (this->m_rect.maxX + this->m_rect.minX) * 0.5f;
        float cy = (this->m_rect.minY + this->m_rect.maxY) * 0.5f;

        if (this->m_reverse) {
            this->m_starVerts[0].x = cx;
            this->m_starVerts[0].y = this->m_rect.maxY;
            this->m_starVerts[0].z = 1.0f;
            this->m_starTexCoords[0].x = 0.5f;
            this->m_starTexCoords[0].y = 0.0f;
        } else {
            this->m_starVerts[8].x = cx;
            this->m_starVerts[8].y = this->m_rect.maxY;
            this->m_starVerts[8].z = 1.0f;
            this->m_starTexCoords[8].x = 0.5f;
            this->m_starTexCoords[8].y = 0.0f;
        }

        this->m_starVerts[9].x = cx;
        this->m_starVerts[9].y = cy;
        this->m_starVerts[9].z = 1.0f;
        this->m_starTexCoords[9].x = 0.5f;
        this->m_starTexCoords[9].y = 0.5f;

        if (octant < 4) {
            if (octant < 2) {
                this->AnimateOctants01(octant, t);
            } else {
                this->AnimateOctants23(octant, t);
            }
        } else if (octant < 6) {
            this->AnimateOctants45(octant, t);
        } else {
            this->AnimateOctants67(octant, t);
        }

        if (this->m_drawEdge) {
            float angle = elapsed / duration * -6.2831855f;
            float c = cos(angle);
            float s = sin(angle);
            float scale = this->m_layoutScale;
            float a = c * -0.5f;
            float b = -0.5f * s;
            float d = c * 0.5f;
            float e = s * 0.5f;
            this->m_edgeTexCoords[0].x = (a - b) * scale + 0.5f;
            this->m_edgeTexCoords[0].y = (b + a) * scale + 0.5f;
            this->m_edgeTexCoords[1].x = (d - b) * scale + 0.5f;
            this->m_edgeTexCoords[1].y = (e + a) * scale + 0.5f;
            this->m_edgeTexCoords[2].x = (a - e) * scale + 0.5f;
            this->m_edgeTexCoords[2].y = (b + d) * scale + 0.5f;
            this->m_edgeTexCoords[3].x = (d - e) * scale + 0.5f;
            this->m_edgeTexCoords[3].y = (d + e) * scale + 0.5f;
        }

        return;
    }

    if (this->m_reverse) {
        float cx = (this->m_rect.minX + this->m_rect.maxX) * 0.5f;
        float cy = (this->m_rect.maxY + this->m_rect.minY) * 0.5f;
        this->m_starVerts[0].x = cx;
        this->m_starVerts[0].y = this->m_rect.maxY;
        this->m_starVerts[0].z = 1.0f;
        this->m_starTexCoords[0].x = 0.5f;
        this->m_starTexCoords[0].y = 0.0f;
        this->m_starVerts[1].x = this->m_rect.maxX;
        this->m_starVerts[1].y = this->m_rect.maxY;
        this->m_starVerts[2].x = this->m_rect.maxX;
        this->m_starVerts[2].y = cy;
        this->m_starVerts[3].x = this->m_rect.maxX;
        this->m_starVerts[3].y = this->m_rect.minY;
        this->m_starVerts[4].x = cx;
        this->m_starVerts[4].y = this->m_rect.minY;
        this->m_starVerts[5].x = this->m_rect.minX;
        this->m_starVerts[5].y = this->m_rect.minY;
        this->m_starVerts[6].x = this->m_rect.minX;
        this->m_starVerts[6].y = cy;
        this->m_starVerts[7].x = this->m_rect.minX;
        this->m_starVerts[7].y = this->m_rect.maxY;
        this->m_starVerts[8].x = cx;
        this->m_starVerts[8].y = this->m_rect.maxY;
        this->m_starVerts[9].x = cx;
        this->m_starVerts[9].y = cy;
        this->m_starVerts[9].z = 1.0f;
        this->m_starTexCoords[9].x = 0.5f;
        this->m_starTexCoords[9].y = 0.5f;
        return;
    }

    float elapsed = static_cast<float>(this->m_elapsed);
    int32_t index = static_cast<int32_t>(elapsed * 0.0059999996f);
    float t = (elapsed - static_cast<float>(index) * 166.66667f) * 0.0059999996f;
    float alpha = static_cast<float>(static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255));
    this->m_flashColor.a = static_cast<uint8_t>(static_cast<int32_t>(alpha * ((CGCooldown::s_flashAlpha[index + 1] - CGCooldown::s_flashAlpha[index]) * t + CGCooldown::s_flashAlpha[index])));

    float angle = (CGCooldown::s_flashRotation[index + 1] - CGCooldown::s_flashRotation[index]) * t + CGCooldown::s_flashRotation[index];
    float c = cos(angle);
    float s = sin(angle);
    float a = c * -0.5f;
    float b = -0.5f * s;
    float d = c * 0.5f;
    float e = s * 0.5f;
    this->m_flashTexCoords[0].x = a - b;
    this->m_flashTexCoords[0].y = b + a;
    this->m_flashTexCoords[1].x = d - b;
    this->m_flashTexCoords[1].y = e + a;
    this->m_flashTexCoords[2].x = a - e;
    this->m_flashTexCoords[2].y = b + d;
    this->m_flashTexCoords[3].x = d - e;
    this->m_flashTexCoords[3].y = d + e;

    float scale = (CGCooldown::s_flashScale[index + 1] - CGCooldown::s_flashScale[index]) * t + CGCooldown::s_flashScale[index];

    for (uint32_t i = 0; i < 4; i++) {
        this->m_flashTexCoords[i].x = this->m_flashTexCoords[i].x * scale + 0.5f;
        this->m_flashTexCoords[i].y = this->m_flashTexCoords[i].y * scale + 0.5f;
    }
}

// VERIFY: aa78be12 manual
// OFFSET: 0x005EC790
void CGCooldown::UpdateColors() {
    CImVector starColor;
    CImVector edgeColor;
    CImVector flashColor;

    if (!this->m_flashing) {
        starColor.b = 0;
        starColor.g = 0;
        starColor.r = 0;
        starColor.a = static_cast<uint8_t>(static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255) * 160 / 255);
        this->m_starColor = starColor;

        edgeColor.b = 0xFF;
        edgeColor.g = 0xFF;
        edgeColor.r = 0xFF;
        edgeColor.a = static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255);
        this->m_edgeColor = edgeColor;

        flashColor.b = 0;
        flashColor.g = 0;
        flashColor.r = 0;
        flashColor.a = 0;
        this->m_flashColor = flashColor;
    } else {
        starColor.b = 0;
        starColor.g = 0;
        starColor.r = 0;
        starColor.a = 0;
        this->m_starColor = starColor;

        edgeColor.b = 0;
        edgeColor.g = 0;
        edgeColor.r = 0;
        edgeColor.a = 0;
        this->m_edgeColor = edgeColor;

        flashColor.b = 0xFF;
        flashColor.g = 0xA0;
        flashColor.r = 0x50;
        flashColor.a = static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255);
        this->m_flashColor = flashColor;
    }
}

// VERIFY: 80dc4a8b auto
// OFFSET: 0x005EC8C0
void CGCooldown::UpdateAlpha() {
    CSimpleFrame::UpdateAlpha();
    this->UpdateColors();
}

// VERIFY: 14ee5c0b auto
// OFFSET: 0x005EC8D0
void CGCooldown::OnFrameSizeChanged(const CRect& rect) {
    CSimpleFrame::OnFrameSizeChanged(rect);
    this->ResetVerts();
}

// VERIFY: 9f3e3bf6 manual
// OFFSET: 0x005EC8F0
void CGCooldown::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    this->m_elapsed = OsGetAsyncTimeMs() - this->m_startTime;

    if (this->m_elapsed >= this->m_duration) {
        if (this->m_flashing) {
            this->m_flashing = 0;
            this->Hide();
        } else if (this->m_reverse) {
            this->m_elapsed = this->m_duration - 1;
        } else {
            this->m_startTime = OsGetAsyncTimeMs();
            this->m_flashing = 1;
            this->m_duration = 1000;
            this->m_elapsed = 0;
            this->UpdateColors();
        }
    }

    this->AnimateVerts();
}

// VERIFY: 08514143 manual
// OFFSET: 0x005ECAE0
CGCooldown::CGCooldown(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_starTexture = nullptr;
    this->m_edgeTexture = nullptr;
    this->m_reverse = 0;
    this->m_startTime = 0;
    this->m_duration = 0;
    this->m_elapsed = 0;
    this->m_flashing = 0;
    this->m_drawEdge = 0;
}

// VERIFY: fcbb1306 manual
// OFFSET: 0x005ECD70
void CGCooldown::SetCooldown(uint32_t start, uint32_t duration) {
    this->m_startTime = start;
    this->m_duration = duration;
    this->m_elapsed = 0;
    this->m_flashing = 0;

    if (!this->m_starTexture) {
        CStatus status;
        this->m_starTexture = TextureCreate(CGCooldown::s_starTextureName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);
    }

    if (!this->m_edgeTexture) {
        CStatus status;
        this->m_edgeTexture = TextureCreate(CGCooldown::s_edgeTextureName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);
    }

    this->UpdateColors();
    this->ResetVerts();
    this->Show();
}
