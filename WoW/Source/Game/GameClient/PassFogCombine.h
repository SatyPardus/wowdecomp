#ifndef WOW_SOURCE_GAME_GAMECLIENT_PASSFOGCOMBINE_H
#define WOW_SOURCE_GAME_GAMECLIENT_PASSFOGCOMBINE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/FFX/FFX.h"

class CGxShader;
struct CGxBuf;
struct CGxPool;

class PassFogCombine : public FFX::Pass {
    public:
    /* 0030 */ uint32_t m_useRect;
    /* 0034 */ CGxShader* m_pixelShaders[2];
    /* 003C */ CGxPool* m_vertexPool;
    /* 0040 */ CGxBuf* m_vertexBuf;
    /* 0044 */ CGxPool* m_indexPool;
    /* 0048 */ CGxBuf* m_indexBuf;
    /* 004C */ uint32_t m_lastWidth;
    /* 0050 */ uint32_t m_lastHeight;
    /* 0054 */ float m_density;
    /* 0058 */ float m_fadeTime;

    ~PassFogCombine() override;
    bool Valid() override;
    void Execute() override;

    PassFogCombine(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk);
    void BuildIndices();
    void BuildVertices(const C2iVector& size, FFX::Tex* tex0, FFX::Tex* tex1);
};

#endif
