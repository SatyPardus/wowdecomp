#include <cmath>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Services/RibbonEmitter.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Vector.h"

static const float MIN_EDGE_LIFE_SPAN = 0.25f;

// STATIC: 0x00B2D658
static int32_t s_ribbonRenderEnable = 1;

// INLINED
CRibbonMat::CRibbonMat() {
    this->m_lit = 1;
    this->m_fogged = 1;
    this->m_depthTest = 1;
    this->m_depthWrite = 1;
    this->m_cull = 1;
    this->m_blendMode = 0;
}

// VERIFY: 68bb48ac manual
// OFFSET: 0x0097F510
void CRibbonEmitter::ConvertTexSlotToTexCoords() {
    uint32_t cols = this->m_cols;
    uint32_t row = this->m_texSlot / cols;
    float u = (this->m_texSlot % cols) * this->m_texCellU + this->m_texCoords.minY;
    this->m_texSlotCoords.minY = u;
    float v = row * this->m_texCellV + this->m_texCoords.minX;
    this->m_texSlotCoords.minX = v;
    this->m_texSlotCoords.maxY = u + this->m_texCellU;
    this->m_texSlotCoords.maxX = v + this->m_texCellV;
}

// VERIFY: 507021d0 manual
// OFFSET: 0x0097F570
void CRibbonEmitter::SetDataEnabled(int32_t enabled) {
    this->m_flags ^= (this->m_flags ^ (4 * enabled)) & 0x4;

    if (!(this->m_flags & 0x4)) {
        this->m_flags &= ~0x1;
    }
}

// VERIFY: 2a6d93b6 manual
// OFFSET: 0x0097F5B0
void CRibbonEmitter::SetUserEnabled(uint32_t enabled) {
    this->m_flags ^= (this->m_flags ^ (8 * enabled)) & 0x8;

    if (!(this->m_flags & 0x8)) {
        this->m_flags &= ~0x1;
    }
}

// VERIFY: f8d931dc auto
// OFFSET: 0x0097F5F0
void CRibbonEmitter::SetTexSlot(uint32_t slot) {
    if (this->m_texSlot != slot) {
        this->m_texSlot = slot;
        this->ConvertTexSlotToTexCoords();
    }
}

// VERIFY: a4ee293a auto
// OFFSET: 0x0097F610
void CRibbonEmitter::SetAbove(float above) {
    this->m_above = above;
}

// VERIFY: 3fe02f2e auto
// OFFSET: 0x0097F620
void CRibbonEmitter::SetBelow(float below) {
    this->m_below = below;
}

// VERIFY: ca13231a auto
// OFFSET: 0x0097F630
void CRibbonEmitter::SetGravity(float gravity) {
    this->m_gravity = gravity;
}

// VERIFY: bfe848c9 auto
// OFFSET: 0x0097F640
int32_t CRibbonEmitter::IsDead() {
    return this->m_readPos == this->m_writePos;
}

// VERIFY: 6515ce90 manual
// OFFSET: 0x0097F700
void CRibbonEmitter::InitInterpDeltas() {
    float dz = this->m_prevPos.z - this->m_currPos.z;
    float dy = this->m_prevPos.y - this->m_currPos.y;
    float dx = this->m_prevPos.x - this->m_currPos.x;
    float distance = sqrt(dx * dx + (dy * dy + dz * dz));

    this->m_startBelow = C3Vector(this->m_prevPos.x - this->m_prevUp.x * this->m_below, this->m_prevPos.y - this->m_prevUp.y * this->m_below, this->m_prevPos.z - this->m_below * this->m_prevUp.z);
    this->m_endBelow = C3Vector(this->m_currPos.x - this->m_currUp.x * this->m_below, this->m_currPos.y - this->m_currUp.y * this->m_below, this->m_currPos.z - this->m_below * this->m_currUp.z);
    this->m_startAbove = C3Vector(this->m_above * this->m_prevUp.x + this->m_prevPos.x, this->m_prevUp.y * this->m_above + this->m_prevPos.y, this->m_above * this->m_prevUp.z + this->m_prevPos.z);
    this->m_endAbove = C3Vector(this->m_currUp.x * this->m_above + this->m_currPos.x, this->m_currUp.y * this->m_above + this->m_currPos.y, this->m_above * this->m_currUp.z + this->m_currPos.z);
    this->m_startTangent = C3Vector(this->m_prevDir.x * distance, this->m_prevDir.y * distance, this->m_prevDir.z * distance);
    this->m_endTangent = C3Vector(this->m_currDir.x * distance, this->m_currDir.y * distance, distance * this->m_currDir.z);
}

// VERIFY: 946634b1 manual
// OFFSET: 0x0097F900
void CRibbonEmitter::CloseTextureHandles() {
    uint32_t count = this->m_textures.Count();

    for (uint32_t i = 0; i < count; i++) {
        if (this->m_textures[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[i]));
        }
    }
}

// VERIFY: 684a7387 manual
// OFFSET: 0x0097F940
void CRibbonEmitter::SetPos(const C44Matrix& transform, const C3Vector& position, const C44Matrix* frameOfReference) {
    if (!(this->m_flags & 0x4) || !(this->m_flags & 0x8)) {
        return;
    }

    C44Matrix matrix = transform;
    matrix.d0 = position.x + matrix.d0;
    matrix.d1 = position.y + matrix.d1;
    matrix.d2 = position.z + matrix.d2;

    if (frameOfReference) {
        matrix *= frameOfReference->AffineInverse();
    }

    this->m_localPos = position;

    if (this->m_flags & 0x1) {
        this->m_prevPos = this->m_currPos;
        this->m_prevDir = this->m_currDir;
        this->m_prevUp = this->m_currUp;
    } else {
        this->m_prevDir = C3Vector(matrix.c0, matrix.c1, matrix.c2);
        this->m_edgeFraction = 0.0f;
        this->m_prevUp = C3Vector(matrix.b0, matrix.b1, matrix.b2);
        this->m_prevPos = C3Vector(matrix.d0, matrix.d1, matrix.d2);
        this->m_flags |= 0x1;
    }

    this->m_currPos = C3Vector(matrix.d0, matrix.d1, matrix.d2);
    this->m_currDir = C3Vector(matrix.c0, matrix.c1, matrix.c2);
    this->m_currUp = C3Vector(matrix.b0, matrix.b1, matrix.b2);
}

// VERIFY: 139ccf62 manual
// OFFSET: 0x0097FAD0
int32_t CRibbonEmitter::ReplaceTexture(uint32_t type, CTexture* texture) {
    int32_t replaced = 0;

    for (uint32_t i = 0; i < this->m_textureTypes.Count(); i++) {
        if (this->m_textureTypes[i] == type) {
            if (this->m_textures[i]) {
                HandleClose(reinterpret_cast<HOBJECT>(this->m_textures[i]));
            }

            replaced++;
            this->m_textures[i] = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture)));
        }
    }

    return replaced;
}

// VERIFY: b358630b auto
// OFFSET: 0x0097FB60
void CRibbonEmitter::SetColor(float r, float g, float b) {
    this->m_color.Set(this->m_color.a * 255.0f, r, g, b);
}

// VERIFY: f4dee58f manual
// OFFSET: 0x0097FBA0
void CRibbonEmitter::SetAlpha(float alpha) {
    this->m_color.a = static_cast<uint8_t>(static_cast<int32_t>(alpha * 255.0f + 0.5f));
}

// VERIFY: a976a0e7 manual
// OFFSET: 0x0097FBE0
void CRibbonEmitter::ChangeFrameOfReference(const C44Matrix* matrix) {
    const C44Matrix& m = *matrix;

    this->m_prevPos *= m;
    this->m_prevDir = C3Vector(this->m_prevDir.y * m.b0 + this->m_prevDir.z * m.c0 + this->m_prevDir.x * m.a0, m.c1 * this->m_prevDir.z + this->m_prevDir.x * m.a1 + m.b1 * this->m_prevDir.y, this->m_prevDir.y * m.b2 + m.a2 * this->m_prevDir.x + this->m_prevDir.z * m.c2);
    this->m_prevUp = C3Vector(this->m_prevUp.y * m.b0 + this->m_prevUp.x * m.a0 + this->m_prevUp.z * m.c0, m.c1 * this->m_prevUp.z + m.b1 * this->m_prevUp.y + this->m_prevUp.x * m.a1, this->m_prevUp.x * m.a2 + this->m_prevUp.z * m.c2 + this->m_prevUp.y * m.b2);

    this->m_currPos *= m;
    this->m_currDir = C3Vector(this->m_currDir.z * m.c0 + this->m_currDir.y * m.b0 + this->m_currDir.x * m.a0, this->m_currDir.x * m.a1 + m.c1 * this->m_currDir.z + m.b1 * this->m_currDir.y, this->m_currDir.z * m.c2 + this->m_currDir.y * m.b2 + this->m_currDir.x * m.a2);
    this->m_currUp = C3Vector(this->m_currUp.x * m.a0 + this->m_currUp.y * m.b0 + this->m_currUp.z * m.c0, this->m_currUp.x * m.a1 + this->m_currUp.z * m.c1 + this->m_currUp.y * m.b1, this->m_currUp.z * m.c2 + this->m_currUp.y * m.b2 + m.a2 * this->m_currUp.x);

    for (uint32_t i = 0; i < this->m_vertices.Count(); i++) {
        this->m_vertices.m_data[i].p *= m;
    }
}

// VERIFY: b79ec970 auto
// OFFSET: 0x0097FE40
uint32_t CRibbonEmitter::GetNumBatches() {
    return this->m_materials.Count();
}

// VERIFY: a63fdf92 manual
// OFFSET: 0x0097FE50
uint32_t CRibbonEmitter::GetNumPrimitives() {
    if (this->m_readPos >= this->m_writePos) {
        return 2 * (this->m_writePos + this->m_edgeAges.Count() - this->m_readPos);
    }

    return 2 * (this->m_writePos - this->m_readPos);
}

// VERIFY: bcdbf864 manual
// OFFSET: 0x0097FEF0
void CRibbonEmitter::InterpEdge(float age, float t, uint32_t advance) {
    CGxVertexPCT* vertex = &this->m_vertices.m_data[2 * this->m_writePos];
    float s = 1.0f - t;

    vertex[0].p.x = (this->m_endBelow.x - s * this->m_endTangent.x) * t + (this->m_startTangent.x * t + this->m_startBelow.x) * s;
    vertex[0].p.y = (this->m_endBelow.y - this->m_endTangent.y * s) * t + (this->m_startBelow.y + this->m_startTangent.y * t) * s;
    vertex[0].p.z = (this->m_endBelow.z - s * this->m_endTangent.z) * t + (this->m_startBelow.z + this->m_startTangent.z * t) * s;

    vertex[1].p.x = (this->m_endAbove.x - this->m_endTangent.x * s) * t + (t * this->m_startTangent.x + this->m_startAbove.x) * s;
    vertex[1].p.y = (this->m_endAbove.y - this->m_endTangent.y * s) * t + (this->m_startTangent.y * t + this->m_startAbove.y) * s;
    vertex[1].p.z = s * (t * this->m_startTangent.z + this->m_startAbove.z) + (this->m_endAbove.z - s * this->m_endTangent.z) * t;

    this->m_edgeAges.m_data[this->m_writePos] = age;
    this->m_writePos += advance;

    if (this->m_writePos >= this->m_edgeAges.Count()) {
        this->m_writePos -= this->m_edgeAges.Count();
    }
}

// VERIFY: b2cbdd07 manual
// OFFSET: 0x00980090
void CRibbonEmitter::Update(float deltaTime, int32_t suppressNewEdges) {
    float dt = deltaTime;
    float start = 1.0f;

    if (!(this->m_flags & 0x10) && this->m_edgesPerSec > 0.0f) {
        dt = 1.0f / this->m_edgesPerSec + 0.000099999997f;
    }

    if (dt < 0.0f) {
        dt = 0.0f;
    } else if (this->m_edgeLifetime <= dt) {
        dt = this->m_edgeLifetime;
    }

    while (this->m_readPos != this->m_writePos) {
        if (this->m_edgeAges.m_data[this->m_readPos] + dt <= this->m_edgeLifetime) {
            break;
        }

        if (++this->m_readPos >= this->m_edgeAges.Count()) {
            this->m_readPos -= this->m_edgeAges.Count();
        }
    }

    if (!suppressNewEdges && (this->m_flags & 0x4) && (this->m_flags & 0x8) && (this->m_flags & 0x1)) {
        float target = dt * this->m_edgesPerSec + this->m_edgeFraction;
        CImVector color = this->m_color;

        if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
            color.b = this->m_color.r;
            color.g = this->m_color.g;
            color.r = this->m_color.b;
            color.a = this->m_color.a;
        }

        this->InitInterpDeltas();

        if (target >= 1.0f) {
            float scale = 1.0f / (target - this->m_edgeFraction);
            int64_t steps = static_cast<int64_t>(floor(target - 1.0f));

            for (int32_t count = static_cast<int32_t>(steps) + 1; count; count--) {
                float t = (start - this->m_edgeFraction) * scale;
                this->m_vertices.m_data[2 * this->m_writePos].c = color;
                this->m_vertices.m_data[2 * this->m_writePos + 1].c = color;
                this->InterpEdge(-(t * dt), t, 1);
                start = start + 1.0f;
            }
        }

        this->m_edgeFraction = target - floor(target);
        this->InterpEdge(0.0f, 1.0f, 0);

        CGxVertexPCT* vertex = &this->m_vertices.m_data[2 * this->m_writePos];
        vertex[0].tc.x = this->m_texSlotCoords.minY;
        vertex[0].tc.y = this->m_texSlotCoords.minX;
        vertex[1].tc.x = this->m_texSlotCoords.minY;
        vertex[1].tc.y = this->m_texSlotCoords.maxX;
        vertex[0].c = color;
        vertex[1].c = color;
    }

    this->m_worldBounds.b.x = 3.4028235e38f;
    this->m_worldBounds.b.y = 3.4028235e38f;
    this->m_worldBounds.b.z = 3.4028235e38f;
    this->m_worldBounds.t.x = -3.4028235e38f;
    this->m_worldBounds.t.y = -3.4028235e38f;
    this->m_worldBounds.t.z = -3.4028235e38f;

    for (uint32_t i = this->m_readPos; i != this->m_writePos;) {
        CGxVertexPCT* vertex = &this->m_vertices.m_data[2 * i];
        float drop = (this->m_edgeAges.m_data[i] * 2.0f + dt) * this->m_gravity * dt;
        vertex[0].p.z = vertex[0].p.z + drop;
        vertex[1].p.z = drop + vertex[1].p.z;

        for (uint32_t j = 0; j < 2; j++) {
            const C3Vector& p = vertex[j].p;

            if (this->m_worldBounds.b.x > p.x) {
                this->m_worldBounds.b.x = p.x;
            }

            if (this->m_worldBounds.b.y > p.y) {
                this->m_worldBounds.b.y = p.y;
            }

            if (this->m_worldBounds.b.z > p.z) {
                this->m_worldBounds.b.z = p.z;
            }

            if (this->m_worldBounds.t.x < p.x) {
                this->m_worldBounds.t.x = p.x;
            }

            if (this->m_worldBounds.t.y < p.y) {
                this->m_worldBounds.t.y = p.y;
            }

            if (this->m_worldBounds.t.z < p.z) {
                this->m_worldBounds.t.z = p.z;
            }
        }

        this->m_edgeAges.m_data[i] = dt + this->m_edgeAges.m_data[i];

        float u = this->m_edgeAges.m_data[i] * this->m_texCellU * this->m_invEdgeLifetime + this->m_texSlotCoords.minY;
        vertex[0].tc.x = u;
        vertex[0].tc.y = this->m_texSlotCoords.minX;
        vertex[1].tc.x = u;
        vertex[1].tc.y = this->m_texSlotCoords.maxX;

        if (++i >= this->m_edgeAges.Count()) {
            i -= this->m_edgeAges.Count();
        }
    }

    this->m_flags = (this->m_flags & ~0x30) | 0x10;
}

// VERIFY: 05a5de95 manual
// OFFSET: 0x00980510
static void DuplicateTextures(const TSGrowableArray<CTexture*>& source, TSGrowableArray<CTexture*>& dest) {
    uint32_t count = source.m_count;
    dest.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        dest.m_data[i] = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(source.m_data[i])));
    }
}

// VERIFY: 6449e9b3 manual
// OFFSET: 0x00980590
CRibbonEmitter::~CRibbonEmitter() {
    this->CloseTextureHandles();
    this->m_flags &= ~0x2;
}

// VERIFY: 459f4678 auto
// OFFSET: 0x00980630
CRibbonEmitter::CRibbonEmitter() {
    this->m_refCount = 1;
    this->m_texSlotCoords.minX = 0.0f;
    this->m_texSlotCoords.minY = 0.0f;
    this->m_texSlotCoords.maxX = 0.0f;
    this->m_texSlotCoords.maxY = 0.0f;
    this->m_worldBounds.b.x = 3.4028235e38f;
    this->m_worldBounds.b.y = 3.4028235e38f;
    this->m_worldBounds.b.z = 3.4028235e38f;
    this->m_worldBounds.t.x = -3.4028235e38f;
    this->m_worldBounds.t.y = -3.4028235e38f;
    this->m_worldBounds.t.z = -3.4028235e38f;
    this->m_texCoords.minX = 0.0f;
    this->m_texCoords.minY = 0.0f;
    this->m_texCoords.maxX = 0.0f;
    this->m_texCoords.maxY = 0.0f;
    this->m_flags &= ~0x12;
}

// VERIFY: 85ed808e manual
// OFFSET: 0x009808A0
void CRibbonEmitter::Initialize(float edgesPerSecond, float edgeLifetime, const CImVector& color, const TSGrowableArray<CTexture*>& textures, const TSGrowableArray<CRibbonMat>& materials, const TSGrowableArray<uint32_t>& textureTypes, const CAaRect& texCoords, uint32_t rows, uint32_t cols) {
    float edgesPerSec = ceil(edgesPerSecond);

    if (edgeLifetime < MIN_EDGE_LIFE_SPAN) {
        edgeLifetime = MIN_EDGE_LIFE_SPAN;
    }

    uint32_t edgeCount = static_cast<uint32_t>(ceil(edgesPerSec * edgeLifetime) + 2.0f);
    this->m_edgeAges.SetCount(edgeCount);

    this->m_flags &= ~0x1;
    this->m_edgeFraction = 0.0f;
    this->m_readPos = 0;
    this->m_writePos = 0;

    this->m_vertices.SetCount(2 * edgeCount);
    this->m_indices.SetCount(4 * edgeCount);

    for (uint32_t i = 0; i < this->m_indices.Count(); i++) {
        this->m_indices.m_data[i] = i % (2 * edgeCount);
    }

    this->m_invEdgeLifetime = 1.0f / edgeLifetime;
    this->m_texCellU = (texCoords.maxY - texCoords.minY) / cols;
    this->m_texCellV = (texCoords.maxX - texCoords.minX) / rows;
    this->m_invTexCellU = 1.0f / this->m_texCellU;
    this->m_invTexCellV = 1.0f / this->m_texCellV;
    this->m_edgesPerSec = edgesPerSec;
    this->m_edgeLifetime = edgeLifetime;
    this->m_color = color;
    this->m_materials = materials;
    DuplicateTextures(textures, this->m_textures);
    this->m_textureTypes = textureTypes;
    this->m_texCoords = texCoords;
    this->m_rows = rows;
    this->m_cols = cols;
    this->m_texSlot = 0;
    this->ConvertTexSlotToTexCoords();
    this->m_flags |= 0xE;
    this->m_above = 10.0f;
    this->m_below = 10.0f;
    this->m_gravity = 0.0f;
}

// VERIFY: 6008d884 auto
// OFFSET: 0x00980B50
void CRibbonEmitter::DecRef() {
    if (this->m_refCount-- == 1) {
        delete this;
    }
}

// VERIFY: b28c0986 manual
// OFFSET: 0x00980B70
int32_t CRibbonEmitter::Render(const C44Matrix* frameOfReference) {
    if (!s_ribbonRenderEnable || this->m_readPos == this->m_writePos) {
        return 0;
    }

    C44Matrix world;

    if (frameOfReference) {
        world = *frameOfReference;
    }

    world.d0 = world.d0 - this->m_localPos.x;
    world.d1 = world.d1 - this->m_localPos.y;
    world.d2 = world.d2 - this->m_localPos.z;
    g_theGxDevicePtr->XformPushAndSet(GxXform_World, world);

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPCT), this->m_vertices.Count());
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), this->m_indices.Count());
    GxBufData(vertexBuf, this->m_vertices.m_data, sizeof(CGxVertexPCT) * this->m_vertices.Count(), 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);

    uint32_t indexCount;

    if (this->m_readPos >= this->m_writePos) {
        indexCount = 2 * (this->m_writePos + this->m_edgeAges.Count() - this->m_readPos) + 2;
    } else {
        indexCount = 2 * (this->m_writePos - this->m_readPos) + 2;
    }

    GxBufData(indexBuf, &this->m_indices.m_data[2 * this->m_readPos], sizeof(uint16_t) * indexCount, 0);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

    CGxBatch batch;
    batch.m_primType = GxPrim_TriangleStrip;
    batch.m_start = 0;
    batch.m_count = indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = this->m_vertices.Count() - 1;

    for (uint32_t i = 0; i < this->m_materials.Count(); i++) {
        g_theGxDevicePtr->RsPush();

        const CRibbonMat& material = this->m_materials.m_data[i];
        C4Vector emissive;

        if (material.m_lit) {
            emissive.x = 0.0f;
            emissive.y = 0.0f;
            emissive.z = 0.0f;
            emissive.w = 0.0f;
        } else {
            emissive.x = 1.0f;
            emissive.y = 1.0f;
            emissive.z = 1.0f;
            emissive.w = 0.0f;
        }

        CShaderEffect::SetEmissive(emissive);
        CShaderEffect::SetFogEnabled(material.m_fogged);
        CShaderEffect::SetLightingEnabled(material.m_lit);
        g_theGxDevicePtr->RsSet(GxRs_DepthTest, static_cast<int32_t>(material.m_depthTest));
        g_theGxDevicePtr->RsSet(GxRs_DepthWrite, static_cast<int32_t>(material.m_depthWrite));
        g_theGxDevicePtr->RsSet(GxRs_Culling, static_cast<int32_t>(material.m_cull));
        g_theGxDevicePtr->RsSet(GxRs_BlendingMode, static_cast<int32_t>(material.m_blendMode));
        CShaderEffect::SetAlphaRefDefault();

        CGxTex* gxTex = TextureGetGxTex(this->m_textures.m_data[i], 0, nullptr);

        if (gxTex) {
            g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);
            GxTexSetWrap(gxTex, GxTex_Clamp, GxTex_Clamp);
            CShaderEffect::SetDefaultShaders(0);
            CShaderEffect::UpdateWorldViewMatrix();
            g_theGxDevicePtr->Draw(&batch, 1);
        }

        g_theGxDevicePtr->RsPop();
    }

    GxXformPop(GxXform_World);

    return 1;
}
