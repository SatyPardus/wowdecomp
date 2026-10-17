#pragma once
#include "WoW/Common/TerrainSurface.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include <array>
#include <string>
class CTexture;
struct CGxBuf;
struct CGxPool;
class CGxShader;

// A complete ADT surface, using shared original geometry/alpha generation and
// Services texture ownership. World objects, liquids and day/night selection
// are not supplied by this preview; its camera and lighting are explicit inputs.
class TerrainPreview {
public:
    explicit TerrainPreview(const std::string& selection); // Map:tileX:tileY
    ~TerrainPreview();
    TerrainPreview(const TerrainPreview&) = delete;
    TerrainPreview& operator=(const TerrainPreview&) = delete;
    bool Ready(bool wait = false);
    bool Draw(float elapsedSeconds, uint32_t width, uint32_t height, bool fullViewport = false);
    size_t ChunkCount() const { return m_chunks.size(); }
    uint32_t TriangleCount() const { return m_triangles; }
private:
    struct Chunk {
        Terrain::BlendTexture blend;
        CTexture* texture = nullptr;
        CGxBatch batch{};
        C3Vector origin;
        uint32_t vertexPermutation = 0, pixelPermutation = 0;
    };
    void Shutdown();
    Terrain::Surface m_surface;
    std::vector<Chunk> m_chunks;
    std::vector<CTexture*> m_textures;
    CGxBuf *m_vertices = nullptr, *m_indices = nullptr;
    CGxPool *m_vertexPool = nullptr, *m_indexPool = nullptr;
    CGxShader *m_vertexShaders[128]{}, *m_pixelShaders[32]{};
    C3Vector m_center;
    float m_radius = 1, m_rotation = 0;
    std::array<C2Vector, 8> m_texMotion{};
    uint32_t m_triangles = 0;
};
