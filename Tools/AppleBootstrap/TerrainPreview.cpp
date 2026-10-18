#include "Tools/AppleBootstrap/TerrainPreview.h"
#include "Tools/AppleBootstrap/TerrainLiquidPreview.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>

namespace {
std::vector<uint8_t> Read(const std::string& name, uint32_t budget) {
    SFileImpl* file = nullptr;
    if (!SFile::OpenEx(nullptr, name.c_str(), 0, &file)) throw std::runtime_error("Cannot open terrain asset: " + name);
    struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
    uint32_t high = 0, size = SFileGetFileSize(file, &high), read = 0;
    if (high || !size || size > budget) throw std::runtime_error("Terrain asset exceeds size limit: " + name);
    std::vector<uint8_t> bytes(size);
    if (!SFileReadFile(file, bytes.data(), size, &read, nullptr, 0) || read != size) throw std::runtime_error("Incomplete terrain asset: " + name);
    return bytes;
}
uint32_t Word(const std::vector<uint8_t>& bytes, size_t offset) {
    if (offset > bytes.size() || bytes.size()-offset < 4) throw std::runtime_error("Truncated terrain map header");
    uint32_t result; std::memcpy(&result, bytes.data()+offset, 4); return result;
}
void BlendUpload(EGxTexCommand command, uint32_t, uint32_t, uint32_t, uint32_t, void* context, uint32_t& stride, const void*& data) {
    auto& blend = *static_cast<Terrain::BlendTexture*>(context);
    if (command == GxTex_Latch) {
        stride = blend.side * (blend.rgba.empty() ? 2 : 4);
        data = blend.rgba.empty() ? static_cast<const void*>(blend.packed.data()) : blend.rgba.data();
    }
}
void CloseTexture(CTexture*& texture) {
    if (texture) { HandleClose(reinterpret_cast<HOBJECT>(texture)); texture = nullptr; }
}
}

TerrainPreview::TerrainPreview(const std::string& selection) {
    try {
        const auto a = selection.find(':'), b = selection.find(':', a == std::string::npos ? 0 : a+1);
        if (a == std::string::npos || b == std::string::npos || selection.find(':', b+1) != std::string::npos)
            throw std::runtime_error("WOW_TERRAIN must be Map:tileX:tileY (for example Azeroth:32:48)");
        const auto map = selection.substr(0, a);
        if (map.empty() || map.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != std::string::npos)
            throw std::runtime_error("Invalid terrain map name");
        auto coordinate = [](const std::string& value) {
            if (value.empty() || value.size() > 2 || value.find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error("Terrain tile coordinates must be integers from 0 to 63");
            const auto result = std::stoul(value);
            if (result > 63) throw std::runtime_error("Terrain tile coordinate exceeds map bounds");
            return uint32_t(result);
        };
        const auto x = coordinate(selection.substr(a+1, b-a-1)), y = coordinate(selection.substr(b+1));
        const std::string base = "World\\Maps\\" + map + "\\" + map;
        const auto wdt = Read(base + ".wdt", 1024*1024);
        if (Word(wdt, 0) != 0x4D564552 || Word(wdt, 8) != 18 || Word(wdt, 12) != 0x4D504844 || Word(wdt, 16) != 32)
            throw std::runtime_error("Unsupported terrain WDT header");
        const auto adt = Read(base + "_" + std::to_string(x) + "_" + std::to_string(y) + ".adt", 64*1024*1024);
        m_surface = Terrain::ReadSurface(adt.data(), adt.size(), (Word(wdt, 20) & 4) != 0);
        m_chunks.resize(m_surface.chunks.size()); m_textures.resize(m_surface.textures.size());
        std::vector<bool> needed(m_textures.size());
        for (const auto& chunk : m_surface.chunks) {
            if (chunk.layers.empty()) throw std::runtime_error("Terrain preview requires authored material layers in every surface chunk");
            for (size_t i = 0; i < chunk.layers.size(); ++i) {
                const auto& layer = chunk.layers[i];
                // Original Terrain programs expose a cube only in sampler 0.
                if (i && (layer.flags & 0x400)) throw std::runtime_error("Terrain cube layer must be the base material");
                needed[layer.textureID] = true;
            }
        }
        for (size_t i = 0; i < m_textures.size(); ++i) if (needed[i]) {
            // CMapArea::LoadTerrainTexture with specular terrain enabled.
            auto name = m_surface.textures[i];
            if (!(m_surface.textureFlags[i] & 1)) {
                const auto extension = name.rfind('.');
                if (extension == std::string::npos) throw std::runtime_error("Terrain texture has no extension: " + name);
                name.replace(extension, std::string::npos, "_s.blp");
            }
            CStatus status;
            m_textures[i] = TextureCreate(name.c_str(), CGxTexFlags(GxTex_LinearMipLinear, 1, 1), &status, 1);
            if (!m_textures[i] || !status.IsEmpty()) throw std::runtime_error("Cannot load terrain material: " + name);
        }
        auto& device = *g_theGxDevicePtr;
        device.ShaderCreate(m_vertexShaders, GxSh_Vertex, "Shaders\\Vertex", "Terrain", 128);
        device.ShaderCreate(m_pixelShaders, GxSh_Pixel, "Shaders\\Pixel", "Terrain1", 32);
        std::vector<CGxVertexPNC> vertices(m_chunks.size()*145);
        std::vector<uint16_t> indices; indices.reserve(m_chunks.size()*768);
        C3Vector low(1e30f, 1e30f, 1e30f), high(-1e30f, -1e30f, -1e30f);
        for (size_t i = 0; i < m_chunks.size(); ++i) {
            const auto& source = m_surface.chunks[i]; auto& chunk = m_chunks[i];
            const Terrain::GeometryView geometry{{int32_t(x*16+source.x), int32_t(y*16+source.y)}, source.origin.z,
                source.heights.data(), source.normals.data(), source.hasColors ? source.colors.data() : nullptr, false};
            Terrain::CreateVerticesWorld(vertices.data()+i*145, geometry);
            chunk.origin = vertices[i*145].m_position; chunk.origin.z = source.origin.z;
            std::array<uint16_t, 768> chunkIndices;
            const auto count = Terrain::CreateIndices(chunkIndices.data(), source.holes, int32_t(i*145));
            chunk.batch = {GxPrim_Triangles, uint32_t(indices.size()), count, uint16_t(i*145), uint16_t(i*145+144)};
            indices.insert(indices.end(), chunkIndices.begin(), chunkIndices.begin()+count); m_triangles += count/3;
            const auto layers = uint32_t(source.layers.size());
            const bool environment = (source.layers[0].flags & 0x400) != 0;
            m_environmentChunks += environment;
            chunk.vertexPermutation = 16*(layers-1) + 8 + ((Word(wdt, 20) & 2) ? 4 : 0) + (environment ? 2 : 0);
            bool mixedLighting = false; for (const auto& layer : source.layers) mixedLighting |= (layer.flags & 0x80) != 0;
            chunk.pixelPermutation = 2*(layers-1) + (m_surface.bigAlpha ? 8 : 0) + (mixedLighting ? 16 : 0) + environment;
            chunk.blend = Terrain::CreateBlendTexture(m_surface, i, false, true);
            const auto format = chunk.blend.rgba.empty() ? GxTex_Argb4444 : GxTex_Argb8888;
            chunk.texture = TextureCreate(chunk.blend.side, chunk.blend.side, format, format, CGxTexFlags(GxTex_Linear),
                &chunk.blend, BlendUpload, "Original terrain blend and shadow", 0);
            if (!chunk.texture) throw std::runtime_error("Cannot create terrain blend texture");
        }
        for (const auto& vertex : vertices) {
            const auto& p = vertex.m_position;
            low.x = std::min(low.x, p.x); low.y = std::min(low.y, p.y); low.z = std::min(low.z, p.z);
            high.x = std::max(high.x, p.x); high.y = std::max(high.y, p.y); high.z = std::max(high.z, p.z);
        }
        m_center = (low+high)*0.5f;
        const auto diagonal = high-low; m_radius = std::sqrt(diagonal.x*diagonal.x+diagonal.y*diagonal.y+diagonal.z*diagonal.z)*0.5f;
        if (indices.empty() || !(m_radius > 0)) throw std::runtime_error("Terrain tile has no drawable surface");
        m_vertexPool = device.PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Static, uint32_t(vertices.size()*sizeof(vertices[0])), GxPoolHintBit_Unk0, "ADT terrain vertices");
        m_indexPool = device.PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, uint32_t(indices.size()*sizeof(indices[0])), GxPoolHintBit_Unk0, "ADT terrain indices");
        m_vertices = device.BufCreate(m_vertexPool, sizeof(vertices[0]), uint32_t(vertices.size()), 0);
        m_indices = device.BufCreate(m_indexPool, sizeof(indices[0]), uint32_t(indices.size()), 0);
        GxBufData(m_vertices, vertices.data(), uint32_t(vertices.size()*sizeof(vertices[0])), 0);
        GxBufData(m_indices, indices.data(), uint32_t(indices.size()*sizeof(indices[0])), 0);
        m_liquid = std::make_unique<TerrainLiquidPreview>(adt, m_surface, x, y, m_radius, map);
        std::fprintf(stderr, "Terrain surface %s: %zu chunks (%u cube reflection), %u triangles, %zu materials, %s alpha, specular textures enabled\n",
            selection.c_str(), m_chunks.size(), m_environmentChunks, m_triangles, size_t(std::count(needed.begin(), needed.end(), true)), m_surface.bigAlpha ? "8-bit" : "4-bit");
    } catch (...) { Shutdown(); throw; }
}
TerrainPreview::~TerrainPreview() { Shutdown(); }
size_t TerrainPreview::LiquidLayerCount() const { return m_liquid ? m_liquid->LayerCount() : 0; }
size_t TerrainPreview::LiquidBatchCount() const { return m_liquid ? m_liquid->BatchCount() : 0; }
void TerrainPreview::Shutdown() {
    m_liquid.reset();
    auto& device = *g_theGxDevicePtr;
    for (auto& chunk : m_chunks) CloseTexture(chunk.texture);
    for (auto& texture : m_textures) CloseTexture(texture);
    device.BufDestroy(m_vertices); device.BufDestroy(m_indices);
    if (m_vertexPool) { device.PoolDestroy(m_vertexPool); m_vertexPool = nullptr; }
    if (m_indexPool) { device.PoolDestroy(m_indexPool); m_indexPool = nullptr; }
    for (auto& shader : m_vertexShaders) device.ShaderDestroy(shader);
    for (auto& shader : m_pixelShaders) device.ShaderDestroy(shader);
}
bool TerrainPreview::Ready(bool wait) {
    bool ready = true;
    for (auto* texture : m_textures) if (texture) {
        CStatus status; ready &= TextureGetGxTex(texture, wait, &status) != nullptr;
        if (!status.IsEmpty() || !texture->m_status.IsEmpty()) throw std::runtime_error(std::string("Terrain texture failed: ") + texture->m_filename);
    }
    if (m_liquid) ready &= m_liquid->Ready(wait);
    return ready;
}
bool TerrainPreview::Draw(float elapsedSeconds, uint32_t width, uint32_t height, bool fullViewport, bool liquidOnly) {
    if (!Ready()) return false;
    auto& device = *g_theGxDevicePtr;
    device.RsPush(); struct Pop { CGxDevice& device; ~Pop() { device.RsPop(); } } pop{device};
    m_rotation = std::fmod(m_rotation + elapsedSeconds*0.04f, 6.2831853f);
    const C2Vector velocity[] = {{-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}};
    for (unsigned i = 0; i < 8; ++i) {
        // Original World::texVect integration, including its positive-only reset.
        m_texMotion[i].x += velocity[i].x*elapsedSeconds; m_texMotion[i].y += velocity[i].y*elapsedSeconds;
        if (m_texMotion[i].x >= 64) m_texMotion[i].x = 0;
        if (m_texMotion[i].y >= 64) m_texMotion[i].y = 0;
    }
    std::array<std::array<float, 4>, 37> c{};
    C44Matrix view = C44Matrix::RotationAroundZ(-0.2f+m_rotation) * C44Matrix::RotationAroundX(2.5f);
    view.Translate(m_center*-1.0f); view.d2 += 2*m_radius;
    C44Matrix projection(1);
    const float aspect = (fullViewport ? float(width) : width*0.48f) / (fullViewport ? float(height) : height*0.72f);
    projection.a0 = 0.95f/m_radius; projection.b1 = 0.95f/m_radius; projection.c2 = 1/(4*m_radius); projection.d2 = 0;
    if (aspect > 1) projection.a0 /= aspect; else projection.b1 *= aspect;
    std::memcpy(c.data(), &view, 64); std::memcpy(c.data()+4, &projection, 64);
    // Original InitializeVertexShaderConstants rotates reflected view-space
    // directions by the transposed view before its x/z/y cube-coordinate swizzle.
    const auto environmentRotation = view.Transpose();
    std::memcpy(c.data()+8, &environmentRotation, 64);
    const auto sun = TransformDirection(C3Vector(-0.3f, -0.4f, 0.8660254f), view);
    c[24] = {sun.x, sun.y, sun.z, 1}; c[25] = {0.3f, 0.3f, 0.35f, 1}; c[26] = {0.7f, 0.65f, 0.6f, 1}; c[27] = {0.35f, 0.35f, 0.3f, 20};
    // Explicit preview atmosphere, not a substitute for World DayNight data.
    c[12] = {-1/(4*m_radius), 1.4f, 1, 0};
    const float fog[] = {0.12f, 0.14f, 0.18f, 1}; GxShaderConstantsSet(GxSh_Pixel, 2, fog, 1);
    GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_Culling, 0); GxRsSet(GxRs_DepthTest, 1); GxRsSet(GxRs_DepthWrite, 1); GxRsSet(GxRs_DepthFunc, 0);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque); GxRsSet(GxRs_AlphaRef, 0); GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_ScissorTest, 0); GxRsSet(GxRs_ClipPlaneMask, 0); GxRsSet(GxRs_PointSprite, 0); GxRsSet(GxRs_PolygonOffset, 0);
    if (fullViewport) GxXformSetViewport(0, 1, 0, 1, 0, 1); else GxXformSetViewport(0.5f, 0.98f, 0.2f, 0.92f, 0, 1);
    GxPrimVertexPtr(m_vertices, GxVBF_PNC); device.PrimIndexPtr(m_indices);
    const float geoToTex = 1.0f/4.1666665f;
    const float animationRate[] = {64,48,32,16,8,4,2,1};
    if (m_liquid) m_liquid->Prepare();
    if (!liquidOnly) for (size_t i = 0; i < m_chunks.size(); ++i) {
        auto& chunk = m_chunks[i]; if (!chunk.batch.m_count) continue;
        const auto& layers = m_surface.chunks[i].layers;
        std::array<float, 4> lighting{1,1,1,1};
        for (unsigned j = 13; j <= 22; ++j) c[j] = {};
        for (unsigned j = 0; j < layers.size(); ++j) {
            const auto& layer = layers[j];
            device.RsSet(EGxRenderState(GxRs_Texture0+j), TextureGetGxTex(m_textures[layer.textureID], 0, nullptr));
            c[18+j] = {-geoToTex, -geoToTex, 0, 0};
            if (layer.flags & 0x80) lighting[j] = 0;
            if (layer.flags & 0x40) {
                const auto motion = m_texMotion[layer.flags & 7];
                const float scale = 1/geoToTex/animationRate[(layer.flags >> 3) & 7];
                c[13+j] = {-(motion.x*scale*0.125f), -(motion.y*scale*0.125f), 0, 0};
            }
        }
        c[18+layers.size()] = {-geoToTex*0.125f, -geoToTex*0.125f, 0, 0};
        c[23] = {chunk.origin.x, chunk.origin.y, chunk.origin.z, 0};
        device.RsSet(EGxRenderState(GxRs_Texture0+layers.size()), TextureGetGxTex(chunk.texture, 1, nullptr));
        GxShaderConstantsSet(GxSh_Vertex, 0, c[0].data(), 37); GxShaderConstantsSet(GxSh_Pixel, 1, lighting.data(), 1);
        device.RsSet(GxRs_VertexShader, m_vertexShaders[chunk.vertexPermutation]); device.RsSet(GxRs_PixelShader, m_pixelShaders[chunk.pixelPermutation]);
        GxDraw(&chunk.batch, 1);
    }
    if (m_liquid) {
        GxXformPush(GxXform_View);
        const auto previousProjection = device.m_projNative;
        struct Restore { CGxDevice& device; C44Matrix projection; ~Restore() {device.m_projNative=projection;GxXformPop(GxXform_View);} } restore{device,previousProjection};
        device.m_projNative = projection; GxXformSetView(view);
        m_liquid->Draw(C3Vector(0,0,0));
    }
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    return true;
}
