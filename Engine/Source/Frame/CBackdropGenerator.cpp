#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CBackdropGenerator.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/CRect.h"

static const float DEFAULT_CORNER_SIZE = 0.025f;

// VERIFY: 2d4ced50 auto
// OFFSET: 0x004A25F0
void CBackdropGenerator::SetVertexColor(const CImVector& color) {
    this->m_backgroundColor.a = color.a;
    this->m_backgroundColor.r = color.r;
    this->m_backgroundColor.g = color.g;
    this->m_backgroundColor.b = color.b;

    if (this->m_backgroundTexture) {
        this->m_backgroundTexture->SetVertexColor(color);
    }
}

// VERIFY: 606f01de auto
// OFFSET: 0x004A2630
void CBackdropGenerator::SetBorderVertexColor(const CImVector& color) {
    this->m_borderColor.a = color.a;
    this->m_borderColor.r = color.r;
    this->m_borderColor.g = color.g;
    this->m_borderColor.b = color.b;

    if (this->m_leftTexture) {
        this->m_leftTexture->SetVertexColor(color);
    }

    if (this->m_rightTexture) {
        this->m_rightTexture->SetVertexColor(color);
    }

    if (this->m_topTexture) {
        this->m_topTexture->SetVertexColor(color);
    }

    if (this->m_bottomTexture) {
        this->m_bottomTexture->SetVertexColor(color);
    }

    if (this->m_topLeftTexture) {
        this->m_topLeftTexture->SetVertexColor(color);
    }

    if (this->m_topRightTexture) {
        this->m_topRightTexture->SetVertexColor(color);
    }

    if (this->m_bottomLeftTexture) {
        this->m_bottomLeftTexture->SetVertexColor(color);
    }

    if (this->m_bottomRightTexture) {
        this->m_bottomRightTexture->SetVertexColor(color);
    }
}

// VERIFY: 6abbecf6 manual
// OFFSET: 0x004A26D0
void CBackdropGenerator::DestroyTextures() {
    if (this->m_backgroundTexture) {
        delete this->m_backgroundTexture;
        this->m_backgroundTexture = nullptr;
    }

    if (this->m_leftTexture) {
        delete this->m_leftTexture;
        this->m_leftTexture = nullptr;
    }

    if (this->m_rightTexture) {
        delete this->m_rightTexture;
        this->m_rightTexture = nullptr;
    }

    if (this->m_topTexture) {
        delete this->m_topTexture;
        this->m_topTexture = nullptr;
    }

    if (this->m_bottomTexture) {
        delete this->m_bottomTexture;
        this->m_bottomTexture = nullptr;
    }

    if (this->m_topLeftTexture) {
        delete this->m_topLeftTexture;
        this->m_topLeftTexture = nullptr;
    }

    if (this->m_topRightTexture) {
        delete this->m_topRightTexture;
        this->m_topRightTexture = nullptr;
    }

    if (this->m_bottomLeftTexture) {
        delete this->m_bottomLeftTexture;
        this->m_bottomLeftTexture = nullptr;
    }

    if (this->m_bottomRightTexture) {
        delete this->m_bottomRightTexture;
        this->m_bottomRightTexture = nullptr;
    }
}

// VERIFY: 2a96c7b0 auto
// OFFSET: 0x004A2780
void CBackdropGenerator::LoadXML(XMLNode* node, CStatus* status) {
    const char* bgFile = node->GetAttributeByName("bgFile");
    const char* edgeFile = node->GetAttributeByName("edgeFile");
    const char* tile = node->GetAttributeByName("tile");
    const char* alphaMode = node->GetAttributeByName("alphaMode");

    EGxBlend blend = GxBlend_Alpha;

    if (alphaMode && *alphaMode) {
        StringToBlendMode(alphaMode, blend);
    }

    int32_t tiled = 0;

    if (tile && *tile) {
        tiled = StringToBOOL(tile);
    }

    uint32_t pieces = edgeFile && *edgeFile ? THEWORKS : BACKDROPONLY;

    this->m_bgFile.Copy(bgFile);
    this->m_tile = tiled;
    this->m_pieces = pieces;
    this->m_edgeFile.Copy(edgeFile);
    this->m_alphaMode = blend;

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "TileSize", STORM_MAX_STR)) {
            float value;

            if (LoadXML_Value(child, value, status)) {
                this->m_tileSize = value;
            }
        } else if (!SStrCmpI(child->m_name.m_str, "EdgeSize", STORM_MAX_STR)) {
            float value;

            if (LoadXML_Value(child, value, status)) {
                this->m_edgeSize = value;
            }
        } else if (!SStrCmpI(child->m_name.m_str, "BackgroundInsets", STORM_MAX_STR)) {
            float left;
            float right;
            float top;
            float bottom;

            if (LoadXML_Insets(child, left, right, top, bottom, status)) {
                this->m_insetLeft = left;
                this->m_insetRight = right;
                this->m_insetTop = top;
                this->m_insetBottom = bottom;
            }
        } else if (!SStrCmpI(child->m_name.m_str, "Color", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->SetVertexColor(color);
        } else if (!SStrCmpI(child->m_name.m_str, "BorderColor", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->SetBorderVertexColor(color);
        } else {
            status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->m_name.m_str, child->m_name.m_str);
        }
    }
}

// VERIFY: d70fe96e manual
// OFFSET: 0x004A29D0
void CBackdropGenerator::Generate(const CRect* rect) {
    if (!rect) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    C2Vector texCoords[4];

    float width = rect->maxX - rect->minX;
    float scale = 1.0f / this->m_edgeSize;
    float tilesX = scale * width - 2.0f;
    float height = rect->maxY - rect->minY;
    float tilesY = scale * height - 2.0f;

    if (tilesX < 0.0f) {
        tilesX = 0.0f;
    }

    if (tilesY < 0.0f) {
        tilesY = 0.0f;
    }

    if (this->m_bgFile.m_str && *this->m_bgFile.m_str && this->m_tile) {
        float u = width / (this->m_tileSize != 0.0f ? this->m_tileSize : this->m_edgeSize);
        float v = height / (this->m_tileSize != 0.0f ? this->m_tileSize : this->m_edgeSize);

        texCoords[0].x = 0.0f;
        texCoords[0].y = 0.0f;
        texCoords[1].x = 0.0f;
        texCoords[1].y = v;
        texCoords[2].x = u;
        texCoords[2].y = 0.0f;
        texCoords[3].x = u;
        texCoords[3].y = v;
        this->m_backgroundTexture->SetTexCoord(texCoords);
    }

    if (this->m_pieces & LEFTSIDE) {
        texCoords[0].x = 0.0078125f;
        texCoords[0].y = 0.0625f;
        texCoords[1].x = 0.0078125f;
        texCoords[1].y = tilesY - 0.0625f;
        texCoords[2].x = 0.1171875f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.1171875f;
        texCoords[3].y = tilesY - 0.0625f;
        this->m_leftTexture->SetTexCoord(texCoords);
    }

    if (this->m_pieces & RIGHTSIDE) {
        texCoords[0].x = 0.1328125f;
        texCoords[0].y = 0.0625f;
        texCoords[1].x = 0.1328125f;
        texCoords[1].y = tilesY - 0.0625f;
        texCoords[2].x = 0.2421875f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.2421875f;
        texCoords[3].y = tilesY - 0.0625f;
        this->m_rightTexture->SetTexCoord(texCoords);
    }

    if (this->m_pieces & TOPSIDE) {
        texCoords[0].x = 0.2578125f;
        texCoords[0].y = tilesX - 0.0625f;
        texCoords[1].x = 0.3671875f;
        texCoords[1].y = tilesX - 0.0625f;
        texCoords[2].x = 0.2578125f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.3671875f;
        texCoords[3].y = 0.0625f;
        this->m_topTexture->SetTexCoord(texCoords);
    }

    if (this->m_pieces & BOTTOMSIDE) {
        texCoords[0].x = 0.3828125f;
        texCoords[0].y = tilesX - 0.0625f;
        texCoords[1].x = 0.4921875f;
        texCoords[1].y = tilesX - 0.0625f;
        texCoords[2].x = 0.3828125f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.4921875f;
        texCoords[3].y = 0.0625f;
        this->m_bottomTexture->SetTexCoord(texCoords);
    }

    this->SetVertexColor(this->m_backgroundColor);
    this->SetBorderVertexColor(this->m_borderColor);
}

// VERIFY: 67f40144 manual
// OFFSET: 0x004A2D40
CBackdropGenerator::CBackdropGenerator() {
    this->m_backgroundTexture = nullptr;
    this->m_leftTexture = nullptr;
    this->m_rightTexture = nullptr;
    this->m_topTexture = nullptr;
    this->m_bottomTexture = nullptr;
    this->m_topLeftTexture = nullptr;
    this->m_topRightTexture = nullptr;
    this->m_bottomLeftTexture = nullptr;
    this->m_bottomRightTexture = nullptr;
    this->m_edgeSize = DEFAULT_CORNER_SIZE;
    this->m_tileSize = 0.0f;
    this->m_pieces = BACKDROPONLY;
    this->m_insetTop = 0.0f;
    this->m_tile = 0;
    this->m_insetBottom = 0.0f;
    this->m_insetLeft = 0.0f;
    this->m_insetRight = 0.0f;
    this->m_backgroundColor.Set(1.0f, 1.0f, 1.0f, 1.0f);
    this->m_borderColor.Set(1.0f, 1.0f, 1.0f, 1.0f);
}

// VERIFY: 37f5488d manual
// OFFSET: 0x004A2E00
void CBackdropGenerator::SetOutput(CSimpleFrame* frame) {
    if (!frame) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    C2Vector texCoords[4];

    if (this->m_bgFile.m_str && *this->m_bgFile.m_str) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 0, 1) : nullptr;
        this->m_backgroundTexture = texture;

        texture->SetPoint(FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, this->m_insetLeft, -this->m_insetTop, 0);
        texture->SetPoint(FRAMEPOINT_TOPRIGHT, frame, FRAMEPOINT_TOPRIGHT, -this->m_insetRight, -this->m_insetTop, 0);
        texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, frame, FRAMEPOINT_BOTTOMLEFT, this->m_insetLeft, this->m_insetBottom, 0);
        texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, frame, FRAMEPOINT_BOTTOMRIGHT, -this->m_insetRight, this->m_insetTop, 0);
        texture->Resize(0);
        texture->SetTexture(this->m_bgFile.m_str ? this->m_bgFile.m_str : "", this->m_tile, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
    }

    if (this->m_pieces & LEFTSIDE) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_leftTexture = texture;

        texture->SetWidth(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, 0.0f, -this->m_edgeSize, 0);
        texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, frame, FRAMEPOINT_BOTTOMLEFT, 0.0f, this->m_edgeSize, 0);
        texture->Resize(0);
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 1, 1, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & RIGHTSIDE) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_rightTexture = texture;

        texture->SetWidth(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_TOPRIGHT, frame, FRAMEPOINT_TOPRIGHT, 0.0f, -this->m_edgeSize, 0);
        texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, frame, FRAMEPOINT_BOTTOMRIGHT, 0.0f, this->m_edgeSize, 0);
        texture->Resize(0);
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 1, 1, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & TOPSIDE) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_topTexture = texture;

        texture->SetHeight(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, this->m_edgeSize, 0.0f, 0);
        texture->SetPoint(FRAMEPOINT_TOPRIGHT, frame, FRAMEPOINT_TOPRIGHT, -this->m_edgeSize, 0.0f, 0);
        texture->Resize(0);
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 1, 1, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & BOTTOMSIDE) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_bottomTexture = texture;

        texture->SetHeight(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, frame, FRAMEPOINT_BOTTOMLEFT, this->m_edgeSize, 0.0f, 0);
        texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, frame, FRAMEPOINT_BOTTOMRIGHT, -this->m_edgeSize, 0.0f, 0);
        texture->Resize(0);
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 1, 1, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & TOPLEFTCORNER) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_topLeftTexture = texture;

        texture->SetWidth(this->m_edgeSize);
        texture->SetHeight(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
        texCoords[0].x = 0.5078125f;
        texCoords[0].y = 0.0625f;
        texCoords[1].x = 0.5078125f;
        texCoords[1].y = 0.9375f;
        texCoords[2].x = 0.6171875f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.6171875f;
        texCoords[3].y = 0.9375f;
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetTexCoord(texCoords);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & TOPRIGHTCORNER) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_topRightTexture = texture;

        texture->SetWidth(this->m_edgeSize);
        texture->SetHeight(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_TOPRIGHT, frame, FRAMEPOINT_TOPRIGHT, 0.0f, 0.0f, 1);
        texCoords[0].x = 0.6328125f;
        texCoords[0].y = 0.0625f;
        texCoords[1].x = 0.6328125f;
        texCoords[1].y = 0.9375f;
        texCoords[2].x = 0.7421875f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.7421875f;
        texCoords[3].y = 0.9375f;
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetTexCoord(texCoords);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & BOTTOMLEFTCORNER) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_bottomLeftTexture = texture;

        texture->SetWidth(this->m_edgeSize);
        texture->SetHeight(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, frame, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
        texCoords[0].x = 0.7578125f;
        texCoords[0].y = 0.0625f;
        texCoords[1].x = 0.7578125f;
        texCoords[1].y = 0.9375f;
        texCoords[2].x = 0.8671875f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.8671875f;
        texCoords[3].y = 0.9375f;
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetTexCoord(texCoords);
        texture->SetBlendMode(this->m_alphaMode);
    }

    if (this->m_pieces & BOTTOMRIGHTCORNER) {
        void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, 1, 1) : nullptr;
        this->m_bottomRightTexture = texture;

        texture->SetWidth(this->m_edgeSize);
        texture->SetHeight(this->m_edgeSize);
        texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, frame, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
        texCoords[0].x = 0.8828125f;
        texCoords[0].y = 0.0625f;
        texCoords[1].x = 0.8828125f;
        texCoords[1].y = 0.9375f;
        texCoords[2].x = 0.9921875f;
        texCoords[2].y = 0.0625f;
        texCoords[3].x = 0.9921875f;
        texCoords[3].y = 0.9375f;
        texture->SetTexture(this->m_edgeFile.m_str ? this->m_edgeFile.m_str : "", 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetTexCoord(texCoords);
        texture->SetBlendMode(this->m_alphaMode);
    }
}
