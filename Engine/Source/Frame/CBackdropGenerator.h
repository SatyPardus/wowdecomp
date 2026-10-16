#ifndef ENGINE_SOURCE_FRAME_CBACKDROPGENERATOR_H
#define ENGINE_SOURCE_FRAME_CBACKDROPGENERATOR_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/CImVector.h"

class CRect;
class CSimpleFrame;
class CStatus;
class XMLNode;
class CSimpleTexture;

class CBackdropGenerator {
    public:
    /* 0000 */ CSimpleTexture* m_backgroundTexture;
    /* 0004 */ CSimpleTexture* m_leftTexture;
    /* 0008 */ CSimpleTexture* m_rightTexture;
    /* 000C */ CSimpleTexture* m_topTexture;
    /* 0010 */ CSimpleTexture* m_bottomTexture;
    /* 0014 */ CSimpleTexture* m_topLeftTexture;
    /* 0018 */ CSimpleTexture* m_topRightTexture;
    /* 001C */ CSimpleTexture* m_bottomLeftTexture;
    /* 0020 */ CSimpleTexture* m_bottomRightTexture;
    /* 0024 */ RCString m_bgFile;
    /* 0030 */ RCString m_edgeFile;
    /* 003C */ uint32_t m_pieces;
    /* 0040 */ uint32_t m_tile;
    /* 0044 */ uint32_t unused_0044;
    /* 0048 */ float m_edgeSize;
    /* 004C */ float m_tileSize;
    /* 0050 */ float m_insetTop;
    /* 0054 */ float m_insetBottom;
    /* 0058 */ float m_insetLeft;
    /* 005C */ float m_insetRight;
    /* 0060 */ CImVector m_backgroundColor;
    /* 0064 */ CImVector m_borderColor;
    /* 0068 */ EGxBlend m_alphaMode;

    CBackdropGenerator();
    void DestroyTextures();
    void Generate(const CRect* rect);
    void LoadXML(XMLNode* node, CStatus* status);
    void SetBorderVertexColor(const CImVector& color);
    void SetOutput(CSimpleFrame* frame);
    void SetVertexColor(const CImVector& color);
};

#endif
