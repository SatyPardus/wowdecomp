#include <cmath>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gxu/GxuFontMiscClasses.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/CGxVertexPCT.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Gxu/GxuFontUtil.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"

static const double ONEOVERTEXSIZE = (1.0 / 256.0);

// STATIC: 0x00AD9BCC
TSList<TEXTLINETEXTURE, TSGetLink<TEXTLINETEXTURE> > TEXTLINETEXTURE::s_freeTextLineTextures;

// STATIC: 0x00C7D2D8
int32_t TEXTLINETEXTURE::s_recycledBytes;

// STATIC: 0x00C7D2DC
CGxPool* BATCHEDRENDERFONTDESC::s_indexPool;

// STATIC: 0x00C7D2E0
CGxBuf* BATCHEDRENDERFONTDESC::s_indexBuf;

// STATIC: 0x00C7D2E4
uint8_t BATCHEDRENDERFONTDESC::s_billboarded;

// VERIFY: 9faecdeb manual
// OFFSET: 0x006C2C40
CHARCODEDESC::CHARCODEDESC() {
    this->m_textureNumber = 0xFFFFFFFF;
    this->m_rowNumber = 0xFFFFFFFF;
    this->m_glyphStartPixel = 0xFFFFFFFF;
    this->m_glyphEndPixel = 0;
}

// VERIFY: 600ff1ab manual
// OFFSET: 0x006C4190
void GLYPHBITMAPDATA::CopyFrom(GLYPHBITMAPDATA* source) {
    if (this == source) {
        return;
    }

    if (this->m_data) {
        SMemFree(this->m_data, __FILE__, __LINE__, 0);
    }

    this->m_data = nullptr;
    *this = *source;
    source->m_data = nullptr;
}

// VERIFY: 1a45e3fa auto
// OFFSET: 0x006C4340
GLYPHBITMAPDATA::GLYPHBITMAPDATA() {
    this->m_data = nullptr;
    this->m_dataSize = 0;
    this->m_textureCoords.minY = 0.0f;
    this->m_textureCoords.minX = 0.0f;
    this->m_textureCoords.maxY = 0.0f;
    this->m_textureCoords.maxX = 0.0f;
}

// VERIFY: f210f239 manual
// OFFSET: 0x006C4360
GLYPHBITMAPDATA::~GLYPHBITMAPDATA() {
    if (this->m_data) {
        SMemFree(this->m_data, __FILE__, __LINE__, 0);
    }

    this->m_data = nullptr;
}

// VERIFY: d966ef03 manual
// OFFSET: 0x006C4380
void CHARCODEDESC::GenerateTextureCoords(uint32_t rowNumber, uint32_t cellHeight) {
    int32_t top = rowNumber * cellHeight;
    float left = this->m_glyphStartPixel;

    CRect coords;
    coords.minY = top * ONEOVERTEXSIZE;
    coords.minX = left * ONEOVERTEXSIZE;
    coords.maxY = (top + cellHeight) * ONEOVERTEXSIZE;

    float right = left + this->m_bitmapData.m_glyphCellWidth;
    coords.maxX = ONEOVERTEXSIZE * right;

    this->m_bitmapData.m_textureCoords = coords;
}

// VERIFY: 132485d8 auto
// OFFSET: 0x006C4420
uint32_t CHARCODEDESC::GapToPrevious() {
    CHARCODEDESC* previous = this->m_textureRowLink.Prev();

    if (!previous) {
        return this->m_glyphStartPixel;
    }

    return this->m_glyphStartPixel - previous->m_glyphEndPixel - 1;
}

// VERIFY: 6fcf1d8c manual
// OFFSET: 0x006C4440
void TEXTLINETEXTURE::WriteGeometry(CGxVertexPCT* buf, const CImVector& fontColor, const C2Vector& shadowOffset, const CImVector& shadowColor, const C3Vector& viewTranslation, bool hasShadow, bool hasGradient, int32_t vertIndex, int32_t numVerts) const {
    if (!numVerts) {
        return;
    }

    int32_t vertCount = this->m_verts.m_count;

    if (!vertCount) {
        return;
    }

    int32_t useColors = this->m_colors.m_count;

    if (useColors && useColors != vertCount) {
        useColors = 0;
    }

    if (hasShadow) {
        if (vertIndex < vertCount) {
            int32_t count = numVerts < vertCount - vertIndex ? numVerts : vertCount - vertIndex;

            float offsetY = floor(ScreenToPixelHeight(0, shadowOffset.y));
            float offsetX = floor(ScreenToPixelWidth(0, shadowOffset.x));

            C3Vector offset;
            offset.x = viewTranslation.x + offsetX;
            offset.y = viewTranslation.y + offsetY;
            offset.z = viewTranslation.z;

            const CImVector* color = &fontColor;

            if (useColors) {
                color = &this->m_colors.m_data[vertIndex];
            }

            const VERT* vert = &this->m_verts.m_data[vertIndex];

            for (int32_t i = 0; i < count; i++) {
                buf->p = vert->m_vc + offset;
                buf->tc = vert->m_tc;

                CImVector vertColor = shadowColor;

                if (hasGradient && useColors) {
                    vertColor.a = static_cast<int32_t>((shadowColor.a * color->a) * (1.0 / 65536.0));
                }

                GxFormatColor(vertColor);
                buf->c = vertColor;

                vert++;
                buf++;
            }

            numVerts -= count;

            if (vertIndex + count >= vertCount) {
                vertIndex = 0;
            }
        } else {
            vertIndex -= vertCount;
        }
    }

    if (!numVerts || vertIndex >= vertCount) {
        return;
    }

    if (numVerts >= vertCount - vertIndex) {
        numVerts = vertCount - vertIndex;
    }

    const CImVector* color = &fontColor;
    int32_t colorStride = 0;

    if (useColors) {
        color = &this->m_colors.m_data[vertIndex];
        colorStride = 1;
    }

    C44Matrix billboard;

    if (BATCHEDRENDERFONTDESC::s_billboarded) {
        C44Matrix view;
        GxXformView(view);
        billboard.Translate(viewTranslation * view);
        billboard *= view.Inverse();
    }

    const VERT* vert = &this->m_verts.m_data[vertIndex];

    for (int32_t i = numVerts; i > 0; i--) {
        if (BATCHEDRENDERFONTDESC::s_billboarded) {
            buf->p = vert->m_vc * billboard;
        } else {
            buf->p = viewTranslation + vert->m_vc;
        }

        buf->tc = vert->m_tc;

        CImVector vertColor = *color;
        color += colorStride;
        GxFormatColor(vertColor);
        buf->c = vertColor;

        vert++;
        buf++;
    }
}

// VERIFY: 4249ae07 auto
// OFFSET: 0x006C47B0
void BATCHEDRENDERFONTDESC::Initialize() {
    BATCHEDRENDERFONTDESC::s_indexPool = g_theGxDevicePtr->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, 6144, GxPoolHintBit_Unk0, "BATCHEDRENDERFONTDESC_idx");
    BATCHEDRENDERFONTDESC::s_indexBuf = g_theGxDevicePtr->BufCreate(BATCHEDRENDERFONTDESC::s_indexPool, sizeof(uint16_t), 3072, 0);
}

// VERIFY: 416d5eb2 manual
// OFFSET: 0x006C47F0
void BATCHEDRENDERFONTDESC::InitializeIndexBuff() {
    uint16_t* indices = reinterpret_cast<uint16_t*>(GxBufLock(BATCHEDRENDERFONTDESC::s_indexBuf));

    uint16_t vertex = 0;

    for (uint32_t i = 0; i < 512; i++) {
        indices[0] = vertex;
        indices[1] = vertex + 2;
        indices[2] = vertex + 1;
        indices[3] = vertex + 2;
        indices[4] = vertex + 3;
        indices[5] = vertex + 1;
        indices += 6;
        vertex += 4;
    }

    GxBufUnlock(BATCHEDRENDERFONTDESC::s_indexBuf, 0);
}

// VERIFY: 2e596cb8 manual
// OFFSET: 0x006C4860
void BATCHEDRENDERFONTDESC::Shutdown() {
    if (BATCHEDRENDERFONTDESC::s_indexBuf) {
        GxBufDestroy(BATCHEDRENDERFONTDESC::s_indexBuf);
        BATCHEDRENDERFONTDESC::s_indexBuf = nullptr;
    }

    if (BATCHEDRENDERFONTDESC::s_indexPool) {
        g_theGxDevicePtr->PoolDestroy(BATCHEDRENDERFONTDESC::s_indexPool);
        BATCHEDRENDERFONTDESC::s_indexPool = nullptr;
    }
}

// VERIFY: 36c33614 manual
// OFFSET: 0x006C48D0
CGxVertexPCT* BATCHEDRENDERFONTDESC::UnlockVertexPtrAndRender(CGxBuf*& buf, int32_t count) {
    GxBufUnlock(buf, sizeof(CGxVertexPCT) * count);

    if (!BATCHEDRENDERFONTDESC::s_indexBuf->m_hasData || !BATCHEDRENDERFONTDESC::s_indexBuf->m_valid) {
        BATCHEDRENDERFONTDESC::InitializeIndexBuff();
    }

    if (BATCHEDRENDERFONTDESC::s_indexBuf->m_hasData && BATCHEDRENDERFONTDESC::s_indexBuf->m_valid) {
        GxPrimVertexPtr(buf, GxVBF_PCT);
        g_theGxDevicePtr->PrimIndexPtr(BATCHEDRENDERFONTDESC::s_indexBuf);

        CGxBatch batch;
        batch.m_primType = GxPrim_Triangles;
        batch.m_start = 0;
        batch.m_count = 6 * (count / 4);
        batch.m_minIndex = 0;
        batch.m_maxIndex = count - 1;

        GxDraw(&batch, 1);
    }

    return reinterpret_cast<CGxVertexPCT*>(GxBufLock(buf));
}

// VERIFY: 7e6d9f88 manual
// OFFSET: 0x006C4AD0
void BATCHEDRENDERFONTDESC::RenderBatch() {
    if (!BATCHEDRENDERFONTDESC::s_indexPool) {
        return;
    }

    for (CGxString* string = this->m_strings.Head(); string; string = this->m_strings.Next(string)) {
        string->CheckGeometry();
    }

    for (uint32_t i = 0; i < 8; i++) {
        this->m_face->m_textureCache[i].UpdateDirty();
    }

    CGxBuf* buf = GxBufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), 2048);
    CGxVertexPCT* verts = reinterpret_cast<CGxVertexPCT*>(GxBufLock(buf));
    int32_t space = 2048;

    for (int32_t texturePage = 0; texturePage < 8; texturePage++) {
        CTexture* texture = this->m_face->m_textureCache[texturePage].m_texture;

        if (!texture) {
            continue;
        }

        CGxTex* gxTex = TextureGetGxTex(texture, 1, nullptr);

        if (!gxTex) {
            continue;
        }

        g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);

        for (CGxString* string = this->m_strings.Head(); string; string = this->m_strings.Next(string)) {
            if (!string->m_textLines[texturePage]) {
                continue;
            }

            int32_t vertsNeeded = string->CalculateVertsNeeded(texturePage);
            int32_t vertIndex = 0;

            while (vertsNeeded) {
                int32_t count = space < vertsNeeded ? space : vertsNeeded;

                string->WriteGeometry(verts, texturePage, vertIndex, count);

                vertsNeeded -= count;
                vertIndex += count;
                space -= count;
                verts += count;

                if (!space) {
                    verts = BATCHEDRENDERFONTDESC::UnlockVertexPtrAndRender(buf, 2048);
                    space = 2048;
                }
            }
        }

        if (space != 2048) {
            verts = BATCHEDRENDERFONTDESC::UnlockVertexPtrAndRender(buf, 2048 - space);
            space = 2048;
        }
    }

    GxBufUnlock(buf, 0);
}

// VERIFY: cb7c9536 manual
// OFFSET: 0x006C4CD0
TEXTLINETEXTURE::~TEXTLINETEXTURE() {
    this->m_verts.Clear();
    this->m_colors.Clear();
}

// VERIFY: c3da7340 manual
// OFFSET: 0x006C4D80
TEXTLINETEXTURE* TEXTLINETEXTURE::NewTextLineTexture() {
    TEXTLINETEXTURE* texture = TEXTLINETEXTURE::s_freeTextLineTextures.Head();

    if (!texture) {
        return new (__FILE__, __LINE__) TEXTLINETEXTURE;
    }

    TEXTLINETEXTURE::s_recycledBytes -= sizeof(VERT) * texture->m_verts.m_alloc + sizeof(CImVector) * texture->m_colors.m_alloc;
    texture->m_link.Unlink();

    return texture;
}

// VERIFY: be22d63b manual
// OFFSET: 0x006C4F80
void TEXTURECACHEROW::EvictGlyph(CHARCODEDESC* desc) {
    uint32_t needed = desc->m_bitmapData.m_glyphCellWidth;

    CHARCODEDESC* glyph = this->m_glyphlist.Head();

    while (glyph && glyph != desc) {
        glyph = this->m_glyphlist.Next(glyph);
    }

    uint32_t freed = 0;

    while (freed < needed) {
        CHARCODEDESC* next = this->m_glyphlist.Next(desc);
        uint32_t gap = next ? next->m_glyphStartPixel - desc->m_glyphEndPixel - 1 : 255 - desc->m_glyphEndPixel;
        freed += desc->m_glyphEndPixel - desc->m_glyphStartPixel + 1 + gap;

        desc = this->m_glyphlist.DeleteNode(desc);

        if (!desc) {
            for (CHARCODEDESC* last = this->m_glyphlist.Tail(); last && freed < needed; ) {
                freed += last->m_glyphEndPixel - last->m_glyphStartPixel + 1 + last->GapToPrevious();
                last = this->m_glyphlist.DeleteNode(last);
            }

            break;
        }
    }

    CHARCODEDESC* head = this->m_glyphlist.Head();
    this->m_widestFreeSlot = head ? head->GapToPrevious() : 0;

    for (glyph = this->m_glyphlist.Head(); glyph; glyph = this->m_glyphlist.Next(glyph)) {
        CHARCODEDESC* next = this->m_glyphlist.Next(glyph);
        uint32_t gap = next ? next->m_glyphStartPixel - glyph->m_glyphEndPixel - 1 : 255 - glyph->m_glyphEndPixel;

        if (gap > this->m_widestFreeSlot) {
            this->m_widestFreeSlot = gap;
        }
    }
}

// VERIFY: 2ccd62da manual
// OFFSET: 0x006C5120
CHARCODEDESC* TEXTURECACHEROW::CreateNewDesc(GLYPHBITMAPDATA* data, uint32_t rowNumber, uint32_t cellHeight) {
    uint32_t width = data->m_glyphCellWidth;

    if (this->m_widestFreeSlot < width) {
        return nullptr;
    }

    CHARCODEDESC* head = this->m_glyphlist.Head();

    if (!head) {
        CHARCODEDESC* desc = new (__FILE__, __LINE__) CHARCODEDESC;
        this->m_glyphlist.LinkToTail(desc);
        this->m_widestFreeSlot -= width;
        desc->m_glyphStartPixel = 0;
        desc->m_glyphEndPixel = width - 1;
        desc->m_rowNumber = rowNumber;
        desc->m_bitmapData.CopyFrom(data);
        desc->GenerateTextureCoords(rowNumber, cellHeight);

        return desc;
    }

    this->m_widestFreeSlot = head->GapToPrevious();

    if (this->m_widestFreeSlot >= width) {
        CHARCODEDESC* desc = new (__FILE__, __LINE__) CHARCODEDESC;
        this->m_glyphlist.LinkNode(desc, 2, head);
        desc->m_glyphEndPixel = head->m_glyphStartPixel - 1;
        desc->m_glyphStartPixel = desc->m_glyphEndPixel - width + 1;
        desc->m_rowNumber = rowNumber;
        desc->m_bitmapData.CopyFrom(data);
        desc->GenerateTextureCoords(rowNumber, cellHeight);

        CHARCODEDESC* first = this->m_glyphlist.Head();
        this->m_widestFreeSlot = first ? first->GapToPrevious() : 0;

        for (CHARCODEDESC* glyph = this->m_glyphlist.Head(); glyph; glyph = this->m_glyphlist.Next(glyph)) {
            CHARCODEDESC* next = this->m_glyphlist.Next(glyph);
            uint32_t gap = next ? next->m_glyphStartPixel - glyph->m_glyphEndPixel - 1 : 255 - glyph->m_glyphEndPixel;

            if (gap > this->m_widestFreeSlot) {
                this->m_widestFreeSlot = gap;
            }
        }

        return desc;
    }

    CHARCODEDESC* desc = nullptr;
    int32_t inserted = 0;

    for (CHARCODEDESC* glyph = head; glyph; ) {
        CHARCODEDESC* next = this->m_glyphlist.Next(glyph);
        uint32_t gap = next ? next->m_glyphStartPixel - glyph->m_glyphEndPixel - 1 : 255 - glyph->m_glyphEndPixel;

        if (inserted) {
            if (gap > this->m_widestFreeSlot) {
                this->m_widestFreeSlot = gap;
            }
        } else if (gap >= width) {
            desc = new (__FILE__, __LINE__) CHARCODEDESC;
            this->m_glyphlist.LinkNode(desc, 1, glyph);
            desc->m_glyphStartPixel = glyph->m_glyphEndPixel + 1;
            desc->m_glyphEndPixel = glyph->m_glyphEndPixel + width;
            desc->m_rowNumber = rowNumber;
            desc->m_bitmapData.CopyFrom(data);
            desc->GenerateTextureCoords(rowNumber, cellHeight);
            inserted = 1;
            next = desc;
        } else if (!next) {
            return nullptr;
        }

        glyph = next;
    }

    return desc;
}

// VERIFY: 1d5b1e44 manual
// OFFSET: 0x006C53A0
void CGxStringBatch::RenderBatch() {
    if (g_fontStatsEnabled) {
        return;
    }

    C44Matrix projection;
    C44Matrix view;
    GxXformProjection(projection);
    GxXformView(view);

    g_theGxDevicePtr->RsPush();

    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);

    CGxShader* vertexShader;

    if (this->m_flags & 1) {
        BATCHEDRENDERFONTDESC::s_billboarded = 1;
        GxRsSet(GxRs_DepthTest, 1);
        GxRsSet(GxRs_DepthWrite, 1);
        GxRsSet(GxRs_AlphaRef, 1);
        vertexShader = g_fontVertexShader[0];
    } else {
        C44Matrix identity;
        GxXformSetView(identity);

        bool projectionSet = SetProjection();

        GxRsSet(GxRs_DepthTest, 0);
        GxRsSet(GxRs_DepthWrite, 0);

        vertexShader = g_theGxDevicePtr->StereoEnabled() ? g_fontVertexShader[1] : g_fontVertexShader[0];

        if (!projectionSet) {
            BATCHEDRENDERFONTDESC::s_billboarded = 0;
            g_theGxDevicePtr->RsPop();
            GxXformSetView(view);
            GxXformSetProjection(projection);
            return;
        }
    }

    if (vertexShader && vertexShader->Valid() && g_fontPixelShader && g_fontPixelShader->Valid()) {
        C44Matrix viewProj;
        GxRsSet(GxRs_VertexShader, vertexShader);
        GxRsSet(GxRs_PixelShader, g_fontPixelShader);
        GxXformViewProjNativeTranspose(viewProj);
        GxShaderConstantsSet(GxSh_Vertex, 0, reinterpret_cast<const float*>(&viewProj), 4);
    } else {
        GxRsSet(GxRs_Lighting, 0);
        GxRsSet(GxRs_ColorOp0, 0);
        GxRsSet(GxRs_AlphaOp0, 0);
    }

    for (BATCHEDRENDERFONTDESC* desc = this->m_fontBatch.m_fulllist.Head(); desc; ) {
        if (!desc->m_strings.Head()) {
            desc = this->m_fontBatch.DeleteNode(desc);
            continue;
        }

        desc->RenderBatch();

        if (this->m_flags & 2) {
            desc->m_strings.UnlinkAll();
        }

        desc = this->m_fontBatch.m_fulllist.Next(desc);
    }

    BATCHEDRENDERFONTDESC::s_billboarded = 0;
    g_theGxDevicePtr->RsPop();
    GxXformSetView(view);
    GxXformSetProjection(projection);
}

// VERIFY: 53b7b02e manual
// OFFSET: 0x006C5890
BATCHEDRENDERFONTDESC::~BATCHEDRENDERFONTDESC() {
    this->m_strings.UnlinkAll();
}

// VERIFY: c7622af2 manual
// OFFSET: 0x006C5930
void TEXTLINETEXTURE::Recycle(TEXTLINETEXTURE* ptr) {
    if (TEXTLINETEXTURE::s_recycledBytes > 0x80000) {
        delete ptr;
        return;
    }

    ptr->m_verts.SetCount(0);
    ptr->m_colors.SetCount(0);
    TEXTLINETEXTURE::s_recycledBytes += sizeof(VERT) * ptr->m_verts.m_alloc + sizeof(CImVector) * ptr->m_colors.m_alloc;
    TEXTLINETEXTURE::s_freeTextLineTextures.LinkToTail(ptr);
}

// VERIFY: d6f05cb6 manual
// OFFSET: 0x006C59B0
void TEXTLINETEXTURE::Shutdown() {
    TEXTLINETEXTURE::s_recycledBytes = 0;
    TEXTLINETEXTURE::s_freeTextLineTextures.Clear();
}

// INLINED
CGxStringBatch::CGxStringBatch() : m_flags(0) {
}

// VERIFY: a85538e9 manual
// OFFSET: 0x006C5E10
void CGxStringBatch::AddString(CGxString* string) {
    CGxFont* face = string->m_currentFace;
    uint32_t hashval = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(face));

    HASHKEY_PTR key;
    key.m_key = face;

    BATCHEDRENDERFONTDESC* desc = this->m_fontBatch.Ptr(hashval, key);

    if (!desc) {
        desc = this->m_fontBatch.New(hashval, key, 0, 0);
        desc->m_face = face;
    }

    desc->m_strings.LinkToTail(string);
}

// INLINED
TEXTLINETEXTURE::TEXTLINETEXTURE() {
    this->m_verts.m_chunk = 128;
    this->m_colors.m_chunk = 128;
}

// INLINED
TEXTURECACHEROW::TEXTURECACHEROW() {
    this->m_widestFreeSlot = 0;
}
