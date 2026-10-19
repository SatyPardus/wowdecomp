#include "Tools/AppleBootstrap/WmoPreview.h"
#include "Tools/AppleBootstrap/WmoLiquidPreview.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectParser.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {
template <class T> struct Restore {
    T &slot;
    T value;
    explicit Restore(T &target) : slot(target), value(target) {}
    ~Restore() { slot = value; }
};
} // namespace
struct WmoPreview::Impl {
    // Restorers outlive the owned resources, including a failed constructor.
    Restore<uint32_t *> groupHeap{CMap::mapObjGroupHeap}, occluderHeap{CMapObj::occluderHeap};
    Restore<int32_t> shaderEnable{CShaderEffect::s_enableShaders}, alphaRef{CShaderEffect::s_useAlphaRef};
    Restore<uint32_t> pcf{CShaderEffect::s_usePcfFiltering}, shadowValue{CShaderEffect::s_shadowValue};
    Restore<float> fogMul{CShaderEffect::s_fogMul};
    Restore<C4Vector> fogColor{CShaderEffect::s_fogColorAlphaRef}, fogParams{CShaderEffect::s_fogParams};
    Restore<CShaderEffect *> currentEffect{CShaderEffect::s_curEffect};
    Restore<EShadowMode> shadowMode{CShadowCache::s_shadowMode};
    Restore<CMapLight *> mapLight{CMap::s_mapLight};
    Restore<DNLightState> lightState{DayNight::GetInfo()->m_light1};
    Restore<DNFogInfo> fog{DayNight::GetInfo()->m_fog}, interiorFog{DayNight::GetInfo()->m_fogInterior};
    Restore<float> daylight{DayNight::GetInfo()->m_curve0};
    Restore<int32_t> localLights{CWorldScene::s_fogPermute};
    std::array<CShaderEffect *, 7> previousEffects{};
    std::array<std::unique_ptr<CShaderEffect>, 7> effects;
    std::unique_ptr<CMapObj> object;
    CMapObjGroup *group = nullptr;
    std::unique_ptr<WmoLiquidPreview> liquid;
    CMapLight light;
    C3Vector center;
    float radius = 1, rotation = 0;
    bool buffersInitialized = false, fixedFunction = false;

    Impl() { std::copy(std::begin(CMapObj::s_unifiedShaders), std::end(CMapObj::s_unifiedShaders), previousEffects.begin()); }
    ~Impl() {
        liquid.reset();
        object.reset();
        if (buffersInitialized)
            CMapObjGroup::Destroy();
        // CShaderEffect's destructor uses the system enable state.
        CShaderEffect::s_enableShaders = 1;
        for (auto &effect : effects)
            effect.reset();
        std::copy(previousEffects.begin(), previousEffects.end(), std::begin(CMapObj::s_unifiedShaders));
    }
};

WmoPreview::WmoPreview(const std::string &selection) : m(std::make_unique<Impl>()) {
    const char* fixed = std::getenv("WOW_WMO_FIXED");
    m->fixedFunction = fixed && !std::strcmp(fixed, "1");
    auto name = selection;
    uint32_t index = 0;
    const auto separator = name.rfind(':');
    if (separator != std::string::npos) {
        const auto number = name.substr(separator + 1);
        if (number.empty() || number.size() > 3 || number.find_first_not_of("0123456789") != std::string::npos)
            throw std::runtime_error("WOW_WMO must be an archive root path with an optional :group index");
        index = uint32_t(std::stoul(number));
        name.resize(separator);
    }
    if (name.size() < 5 || name.size() >= STORM_MAX_STR || SStrCmpI(name.c_str() + name.size() - 4, ".wmo", 4))
        throw std::runtime_error("WOW_WMO requires a .wmo root asset");
    if (CMapObjGroup::vertexVBList.unk_001C.Head() || CMapObjGroup::indexVBList.unk_001C.Head())
        throw std::runtime_error("WMO preview requires an exclusive world buffer context");
    static uint32_t groupHeap = ObjectAllocAddHeap(sizeof(CMapObjGroup), 32, "WMO preview groups", true);
    static uint32_t occluderHeap = ObjectAllocAddHeap(sizeof(CMapObjOccluder), 128, "WMO preview occluders", true);
    CMap::mapObjGroupHeap = &groupHeap;
    CMapObj::occluderHeap = &occluderHeap;
    CShaderEffect::InitShaderSystem(1, 0);
    SFileImpl *effectFile = nullptr;
    if (!SFileOpenFile("Shaders\\Effects\\MapObjU.wfx", &effectFile))
        throw std::runtime_error("Cannot open original WMO effect definitions");
    {
        std::unique_ptr<SFileImpl, decltype(&SFileCloseFile)> close(effectFile, SFileCloseFile);
        uint32_t high = 0, size = SFileGetFileSize(effectFile, &high), read = 0;
        if (high || !size || size > 1024 * 1024)
            throw std::runtime_error("Invalid WMO effect file size");
        std::vector<char> bytes(size + 1, 0);
        if (!SFileReadFile(effectFile, bytes.data(), size, &read, nullptr, 0) || read != size)
            throw std::runtime_error("Incomplete WMO effect definitions");
        CShaderEffectParser::ParseEffectFile(
            bytes.data(), size,
            [](const CShaderEffectParser::ParsedEffect &source, void *context) {
                auto &impl = *static_cast<Impl *>(context);
                const char *names[] = {"MapObjUDiffuse", "MapObjUSpecular", "MapObjUMetal",    "MapObjUEnv",
                                       "MapObjUOpaque",  "MapObjUEnvMetal", "MapObjUComposite"};
                for (unsigned i = 0; i < 7; ++i)
                    if (!SStrCmpI(source.m_name, names[i], STORM_MAX_STR)) {
                        if (impl.effects[i] || !source.m_shaderPassCount)
                            throw std::runtime_error("Invalid WMO effect definition");
                        impl.effects[i] = std::make_unique<CShaderEffect>();
                        impl.effects[i]->InitEffect(source.m_shaderPasses[0].m_vertexShader, source.m_shaderPasses[0].m_pixelShader);
                        for (unsigned j = 0; j < source.m_fixedFuncPassCount; ++j) {
                            const auto &pass = source.m_fixedFuncPasses[j];
                            impl.effects[i]->InitFixedFuncPass(pass.m_colorOps, pass.m_alphaOps, pass.m_pass);
                        }
                    }
            },
            m.get());
    }
    for (unsigned i = 0; i < 7; ++i) {
        if (!m->effects[i])
            throw std::runtime_error("Missing original WMO effect definition");
        CMapObj::s_unifiedShaders[i] = m->effects[i].get();
    }
    CMapObjGroup::Initialize();
    m->buffersInitialized = true;
    CMapObjGroup::vertexVBList.m_usage = GxPoolUsage_Static;
    CMapObjGroup::indexVBList.m_usage = GxPoolUsage_Static;
    m->object = std::make_unique<CMapObj>();
    if (!m->object->Read(name.data()))
        throw std::runtime_error("Cannot load WMO preview root");
    m->object->WaitLoad();
    m->object->ReadGroup(index);
    m->object->WaitLoadGroup(index);
    m->group = m->object->m_mapObjGroupArray[index];
    if (!m->group->m_batchListCount)
        throw std::runtime_error("Selected WMO group has no drawable batches");
    for (int i = 0; i < m->object->m_materialsCount; ++i)
        if (m->object->m_materialList[i].m_shader > 6)
            throw std::runtime_error("WMO material has an unsupported shader family");
    if (!(m->group->m_unkLoadedFlag & 2))
        m->object->AttenTransVerts(m->group);
    m->center = (m->group->m_bbox.b + m->group->m_bbox.t) * 0.5f;
    const auto diagonal = m->group->m_bbox.t - m->group->m_bbox.b;
    m->radius = std::sqrt(diagonal.x * diagonal.x + diagonal.y * diagonal.y + diagonal.z * diagonal.z) * 0.5f;
    if (!(m->radius > 0) || !std::isfinite(m->radius))
        throw std::runtime_error("Invalid WMO preview bounds");
    // Explicit preview atmosphere, supplied to the original world material code.
    auto *info = DayNight::GetInfo();
    info->m_curve0 = 1;
    info->m_light1.m_diffuse = CImVector{153, 166, 179, 255};
    info->m_light1.m_ambient = CImVector{89, 77, 77, 255};
    info->m_light1.m_mid0 = info->m_light1.m_diffuse;
    info->m_light1.m_mid1 = info->m_light1.m_ambient;
    info->m_fog = {CImVector{46, 36, 31, 255}, m->radius, 4 * m->radius, 1};
    info->m_fogInterior = info->m_fog;
    m->light.m_light.m_dir = C3Vector(-0.3f, -0.4f, -0.8660254f);
    m->light.m_light.m_specColor = C3Vector(0.3f, 0.3f, 0.3f);
    CMap::s_mapLight = &m->light;
    CShadowCache::s_shadowMode = SHADOW_LOWEST;
    CWorldScene::s_fogPermute = 0;
    m->object->UpdateMaterials();
    if (m->group->m_liquidVertexList) m->liquid = std::make_unique<WmoLiquidPreview>(*m->object, *m->group);
    std::fprintf(stderr, "WMO original group %s:%u: %d vertices, %d triangles, %d batches (%u two-pass transition batches)\n", name.c_str(),
                 index, m->group->m_vertexListCount, m->group->m_indicesCount / 3, m->group->m_batchListCount,
                 m->group->m_transparencyBatchesCount);
    if (m->fixedFunction) std::fputs("WMO preview: original fixed-function material and light selection enabled\n", stderr);
}
WmoPreview::~WmoPreview() = default;
uint32_t WmoPreview::BatchCount() const { return m->group->m_batchListCount + m->group->m_transparencyBatchesCount + unsigned(bool(m->liquid)); }
bool WmoPreview::Ready(bool wait) {
    bool ready = true;
    for (int i = 0; i < m->object->m_materialsCount; ++i)
        for (auto *texture : {m->object->m_materialList[i].Textures()[0], m->object->m_materialList[i].Textures()[1]})
            if (texture) {
                CStatus status;
                ready &= TextureGetGxTex(texture, wait, &status) != nullptr;
                if (!status.IsEmpty() || !texture->m_status.IsEmpty())
                    throw std::runtime_error("WMO material texture failed");
            }
    if (m->liquid) ready &= m->liquid->Ready(wait);
    return ready;
}
bool WmoPreview::Draw(float elapsedSeconds, uint32_t width, uint32_t height, bool fullViewport, bool liquidOnly) {
    if (!Ready())
        return false;
    auto &device = *g_theGxDevicePtr;
    Restore<int32_t> shaderEnable{CShaderEffect::s_enableShaders};
    CShaderEffect::s_enableShaders = !m->fixedFunction;
    Restore<C44Matrix> savedProjection{device.m_projNative};
    device.RsPush();
    GxXformPush(GxXform_View);
    CFrustum previous = CWorldScene::frustumStack[CWorldScene::frustumIndex];
    struct Pop {
        CGxDevice &device;
        CFrustum &previous;
        ~Pop() {
            CWorldScene::FrustumSet(&previous);
            GxXformPop(GxXform_View);
            device.RsPop();
        }
    } pop{device, previous};
    m->rotation = std::fmod(m->rotation + elapsedSeconds * 0.12f, 6.2831853f);
    C44Matrix view = C44Matrix::RotationAroundZ(-0.2f + m->rotation) * C44Matrix::RotationAroundX(2.2f);
    view.Translate(m->center * -1.0f);
    view.d2 += 2 * m->radius;
    GxXformSetView(view);
    C44Matrix projection(1);
    const float aspect = (fullViewport ? float(width) : width * 0.48f) / (fullViewport ? float(height) : height * 0.72f);
    projection.a0 = projection.b1 = 0.95f / m->radius;
    projection.c2 = 1 / (4 * m->radius);
    projection.d2 = 0;
    if (aspect > 1)
        projection.a0 /= aspect;
    else
        projection.b1 *= aspect;
    device.m_projNative = projection;
    if (m->fixedFunction) {
        GxRsSet(GxRs_VertexShader, 0);
        GxRsSet(GxRs_PixelShader, 0);
    }
    const auto projectionRows = projection.Transpose(), viewRows = view.Transpose();
    GxShaderConstantsSet(GxSh_Vertex, 2, reinterpret_cast<const float *>(&projectionRows), 4);
    GxShaderConstantsSet(GxSh_Vertex, 31, reinterpret_cast<const float *>(&viewRows), 3);
    // Frustum planes from the same [0,w] clip transform used by Metal.
    auto clipRows = (view * projection).Transpose();
    const float *rows = reinterpret_cast<const float *>(&clipRows);
    CFrustum frustum;
    for (unsigned i = 0; i < 6; ++i) {
        float p[4];
        for (unsigned j = 0; j < 4; ++j)
            p[j] = i == 4 ? rows[8 + j] : rows[12 + j] + (i & 1 ? -1 : 1) * rows[(i / 2) * 4 + j];
        const float length = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
        frustum.planes[i].n = C3Vector(p[0] / length, p[1] / length, p[2] / length);
        frustum.planes[i].d = p[3] / length;
    }
    CWorldScene::FrustumSet(&frustum);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_DepthTest, 1);
    GxRsSet(GxRs_DepthWrite, 1);
    GxRsSet(GxRs_DepthFunc, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_ScissorTest, 0);
    GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PointSprite, 0);
    GxRsSet(GxRs_PolygonOffset, 0);
    if (fullViewport)
        GxXformSetViewport(0, 1, 0, 1, 0, 1);
    else
        GxXformSetViewport(0.5f, 0.98f, 0.2f, 0.92f, 0, 1);
    CMapObj::SetEmissiveAmbient(CImVector{0, 0, 0, 0});
    if (m->liquid) m->liquid->Prepare();
    if (!liquidOnly) m->object->UnifiedRender(m->group, 0);
    if (m->liquid) {
        // LiquidSystem reads the native projection directly. Preserve the host
        // projection, while using the same clip transform as the WMO geometry.
        Restore<C44Matrix> savedProjection{device.m_projNative};
        device.m_projNative = projection;
        GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
        m->liquid->Draw(C3Vector(0, 0, 0));
    }
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    return true;
}
