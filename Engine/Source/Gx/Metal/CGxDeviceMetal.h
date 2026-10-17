#pragma once
#include <memory>
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Metal/TextureDevice.h"

// The host owns its NSView/UIView and drawable acquisition. This device owns
// game resources and an ordered frame command stream. Only verified native
// effects are accepted; unsupported game operations fail with their name.
class CGxDeviceMetal final : public CGxDevice {
public:
    explicit CGxDeviceMetal(id<MTLDevice> device);
    ~CGxDeviceMetal() override;
    GxMetal::TextureDevice& GPU();
    struct EncodingStatistics {
        uint64_t geometryDraws = 0, renderPasses = 0, bufferUploadPasses = 0;
    };
    EncodingStatistics EncodingStats() const;
    void BeginFrame(MTLRenderPassDescriptor* pass, id<MTLDrawable> drawable = nil);
    id<MTLCommandBuffer> FrameCommand() const;
    void Drain();
    int32_t DeviceCreate(GxWindowProc proc, const CGxFormat& format) override;
    int32_t DeviceCreate(void* window, const CGxFormat& format) override;
    void DeviceDestroy() override;
    int32_t DeviceSetFormat(const CGxFormat& format) override;
    void* DeviceWindow() override;
    int32_t DeviceWM(EGxWM, uintptr_t, uintptr_t) override;
    void CapsWindowSize(CRect& rect) override;
    void CapsWindowSizeInScreenCoords(CRect& rect) override;
    void ScenePresent() override;
    void SceneClear(uint32_t mask, CImVector color) override;
    void Draw(CGxBatch* batch, int32_t indexed) override;
    void IRsSendToHw(EGxRenderState which) override;
    void XformSetProjection(const C44Matrix& matrix) override;
    void ShaderConstantsSet(EGxShTarget target, uint32_t index, const float* values, uint32_t count) override;
    void PoolSizeSet(CGxPool* pool, uint32_t size) override;
    void PoolDestroy(CGxPool* pool) override;
    char* BufLock(CGxBuf* buf) override;
    int32_t BufUnlock(CGxBuf* buf, uint32_t size) override;
    void BufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset) override;
    int32_t TexCreate(EGxTexTarget, uint32_t, uint32_t, uint32_t, EGxTexFormat, EGxTexFormat,
        CGxTexFlags, void*, GxTexUserFunc, const char*, CGxTex*&) override;
    void TexDestroy(CGxTex* texture) override;
    void ITexMarkAsUpdated(CGxTex* texture) override;
    void DeviceSetBaseMipLevel(uint32_t level) override;
    void ShaderCreate(CGxShader** shaders, EGxShTarget target, const char* path, const char* name, uint32_t permutations) override;
    void ShaderDestroy(CGxShader*& shader) override;
    void IShaderReload(CGxShader*, const char*, const char*) override;
    void IShaderCreate(CGxShader* shader) override;
    void DeviceReadPixels(CiRect&, TSGrowableArray<CImVector>&) override;
    void DeviceReadDepths(CiRect&, TSGrowableArray<float>&) override;
    void DeviceTakeScreenShot() override;
    void DeviceSetRenderTarget(EGxBuffer, CGxTex*, uint32_t) override;
    void DeviceSaveDepthSurface() override;
    void DeviceRestoreDepthSurface() override;
    void DeviceResolveDepthBuffer(CGxTex*) override;
    void DeviceCopyTex(CGxTex*, uint32_t, CGxTex*, uint32_t) override;
    int32_t TexCopy(CGxTex*, const C2iVector&, const C2iVector&, const C2iVector&, uint32_t, uint32_t) override;
    int32_t TexStretch(CGxTex*, const CiRect*, const CiRect*, uint32_t, uint32_t) override;
    bool QueryBegin(CGxQuery*) override;
    void QueryCreate(CGxQuery*&, EGxQueryType) override;
    bool QueryEnd(CGxQuery*) override;
    bool QueryGetParam(CGxQuery*, EGxQueryParam, uint32_t&) override;
    bool QueryGetData(CGxQuery*, uint32_t&) override;
    void StereoSetConvergence(float) override;
    float StereoGetConvergence() override;
    void StereoSetSeparation(float) override;
    float StereoGetSeparation() override;
    bool StereoEnabled() override { return false; }
    void PrimPointSize(float) override;
    void PrimLineWidth(float) override;
    void DeviceSetGamma(float) override;
    void DeviceSetGamma(const CGxGammaRamp&) override;
    void DeviceUnk81(uint32_t, uint32_t) override {} // Also empty in D3D9.
    void DeviceUnk82() override {}
    void DeviceUnk83(uint32_t, uint32_t) override {}
private:
    void ApplyRenderTarget();
    MTLViewport Viewport() const;
    MTLScissorRect Scissor() const;
    struct Impl;
    std::unique_ptr<Impl> m;
};
