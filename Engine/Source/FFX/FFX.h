#ifndef ENGINE_SOURCE_FFX_FFX_H
#define ENGINE_SOURCE_FFX_FFX_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CImVector.h"

class CTexture;
struct CVar;

namespace FFX {
    struct Tex {
        /* 0000 */ CTexture* m_texture;
        /* 0004 */ C2iVector m_texSize;
        /* 000C */ C2iVector m_size;
        /* 0014 */ C2Vector m_texelSize;
    };

    struct VertexPCT2 {
        /* 0000 */ C3Vector p;
        /* 000C */ CImVector c;
        /* 0010 */ C2Vector tc[2];
    };

    struct VertexPCT3 {
        /* 0000 */ C3Vector p;
        /* 000C */ CImVector c;
        /* 0010 */ C2Vector tc[3];
    };

    struct VertexPCT4 {
        /* 0000 */ C3Vector p;
        /* 000C */ CImVector c;
        /* 0010 */ C2Vector tc[4];
    };

    class Pass {
        public:
        /* 0004 */ Tex* m_inputs[8];
        /* 0024 */ Tex* m_output;
        /* 0028 */ EGxBlend m_blend;
        /* 002C */ bool m_flip;

        virtual ~Pass();
        virtual bool Valid() = 0;
        virtual void Execute() = 0;

        Pass(Tex** inputs, uint32_t count, Tex* output, EGxBlend blend, bool unk);
    };

    class Effect {
        public:
        /* 0004 */ CVar* m_cvar;
        /* 0008 */ TSGrowableArray<Pass*> m_passes;
        /* 0018 */ bool m_defaultEnabled;

        virtual ~Effect();
        virtual bool Valid();
        virtual void Execute();
        virtual void End();
        virtual void SetParameters(int32_t count, const int32_t* params);

        Effect();
    };

    extern CGxBatch s_quadBatch;
    extern CGxBatch s_gridBatch;
    extern bool s_useRECT;
    extern CVar* s_cvarMultisample;
    extern CVar* s_cvarFFXRectangle;
    extern CVar* s_cvarFFX;
    extern uint16_t s_quadIndices[4];
    extern Effect* s_effect;
    extern Tex s_screen;
    extern Tex s_full[2];
    extern Tex s_half[2];
    extern Tex s_quart[2];
    extern C3Vector s_gridPos[36];
    extern C2Vector s_gridTex[36];
    extern uint16_t s_gridIndices[150];
    extern C44Matrix s_oldProjMtx;
    extern C44Matrix s_oldViewMtx;

    void AllocTex();
    void CreateTex(Tex& tex, const C2iVector& size, bool bit8, bool renderTarget);
    bool CVarCallback(CVar* var, const char* oldValue, const char* value, void* arg);
    void Destroy();
    void Finish();
    C44Matrix* GetOldViewMtx();
    EGxTexTarget GetTexTarget();
    void Init();
    void SetEffect(Effect* effect);
    void SetupGeo_Grid(const C2iVector& dstSize, const C2iVector& srcSize, const C2iVector& srcTexSize, C3Vector* pos, C2Vector* tex, int32_t count, bool flip);
    void SetupGeo_Quad(const C2iVector& dstSize, const C2iVector& srcSize, const C2iVector& srcTexSize, C3Vector* pos, C2Vector* tex, bool flip);
    void SetupGx(Tex* target);
    void SetupIB();
    void SetupIB(int32_t cols, int32_t rows);
    void SetupReset();
    void SetupVB_PCT(uint32_t count, const C3Vector* pos, const C2Vector* tex, const float* shade);
    void SetupVB_PCT4(const C3Vector* pos, CImVector color, const C2Vector* tex, const C2Vector* offsets, const C2Vector& scale);
    void Start();
}

#endif
