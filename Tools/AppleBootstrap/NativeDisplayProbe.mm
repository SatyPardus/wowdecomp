#include "NativeDisplayProbe.h"
#include "NativeGuiProbe.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Os/Apple/Input.h"
#include <TargetConditionals.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
struct NativeDisplayProbe::Impl {
    CGxDeviceMetal& device;
    MTKView* view;
    CGxFormat original;
    BOOL automatic;
    CBoundingBox viewport;
    unsigned phase = 0;
    C2iVector sizes[3];
    Impl(CGxDeviceMetal& d, MTKView* v) : device(d), view(v), original(d.m_format), automatic(v.autoResizeDrawable), viewport(d.m_viewport) {
        original.m_size = {int32_t(view.drawableSize.width), int32_t(view.drawableSize.height)};
        sizes[0] = {320, 180}; sizes[1] = {641, 359}; sizes[2] = original.m_size;
    }
    ~Impl() {
        if (device.FrameCommand()) device.ScenePresent();
        id<MTKViewDelegate> delegate = view.delegate; view.delegate = nil;
        device.DeviceSetFormat(original); view.autoResizeDrawable = automatic;
        view.delegate = delegate;
        device.XformSetViewport(viewport.m_x.m_l, viewport.m_x.m_h, viewport.m_y.m_l, viewport.m_y.m_h, viewport.m_z.m_l, viewport.m_z.m_h);
    }
    void Request(C2iVector& size) {
        CGxFormat format = original; format.m_size = size;
        Require(device.DeviceSetFormat(format), "Native windowed format request failed");
        const auto accepted = device.m_format.m_size;
#if TARGET_OS_OSX
        const CGFloat scale = view.window.backingScaleFactor;
        Require(std::abs(accepted.x - size.x) < scale && std::abs(accepted.y - size.y) < scale,
            "Window size adjustment exceeded native point rounding");
#else
        Require(accepted.x == size.x && accepted.y == size.y, "UIKit changed the explicit render resolution");
#endif
        size = accepted;
    }
    bool Step();
};
NativeDisplayProbe::NativeDisplayProbe(CGxDeviceMetal& device, MTKView* view) : m(std::make_unique<Impl>(device, view)) {}
NativeDisplayProbe::~NativeDisplayProbe() = default;
bool NativeDisplayProbe::Step() { return m->Step(); }
bool NativeDisplayProbe::Impl::Step() {
    Require(!device.FrameCommand(), "Native format check requires an idle host");
    device.XformSetViewport(0, 1, 0, 1, 0, 1);
    if (!phase) { CheckNativeGui((__bridge void*)view); Request(sizes[phase++]); return false; }
    {
        const C2iVector size = sizes[phase - 1];
        CGxFormat requested = original; requested.m_size = size;
        Require(view.drawableSize.width == size.x && view.drawableSize.height == size.y,
            "DeviceSetFormat left the drawable at its old resolution");
#if TARGET_OS_OSX
        NSRect backing = [view convertRectToBacking:view.bounds];
        Require(backing.size.width == size.x && backing.size.height == size.y, "Window points did not map to requested drawable pixels");
#endif
        tagRECT bounds{}; OsGetDefaultWindowRect(&bounds);
        Require(bounds.right == size.x && bounds.bottom == size.y, "Native input retained the previous pixel bounds");
        const auto accepted = device.m_format;
        auto reject = [&](const CGxFormat& invalid) {
            Require(!device.DeviceSetFormat(invalid), "Unsupported native presentation was accepted");
            Require(device.m_format.m_size.x == accepted.m_size.x && device.m_format.m_size.y == accepted.m_size.y &&
                view.drawableSize.width == size.x && view.drawableSize.height == size.y, "Rejected native format changed drawable or active size");
        };
        CGxFormat invalid = accepted; invalid.m_size.x = 16385; reject(invalid);
        invalid = accepted; invalid.m_maximize = 1; reject(invalid);
        invalid = accepted; invalid.m_window = false; reject(invalid);
        invalid = accepted; invalid.m_refreshRate = 777; reject(invalid);
        invalid = accepted; invalid.m_depthFormat = CGxFormat::Fmt_Ds24X; reject(invalid);
#if !TARGET_OS_OSX
        invalid = accepted; invalid.m_vsync = 0; reject(invalid);
#endif
        int32_t workerResult = 1;
        std::thread worker([&] { workerResult = device.DeviceSetFormat(accepted); }); worker.join();
        Require(!workerResult, "Background thread mutated the native window");
        auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
        Require(pass && drawable && pass.colorAttachments[0].texture.width == size.x && pass.colorAttachments[0].texture.height == size.y &&
            pass.depthAttachment.texture.width == size.x && pass.depthAttachment.texture.height == size.y,
            "Native drawable/depth attachments did not rebuild at the new size");
        device.BeginFrame(pass, drawable);
        invalid = accepted; invalid.m_size = {123, 77}; reject(invalid);
        device.SceneClear(3, CImVector(31, 73, 149, 255));
        CiRect rect(0, 0, size.y, size.x); TSGrowableArray<CImVector> pixels;
        device.DeviceReadPixels(rect, pixels);
        Require(pixels.Count() == uint32_t(size.x * size.y), "Resized framebuffer readback size changed");
        for (uint32_t i = 0; i < pixels.Count(); ++i)
            Require(pixels[i].b == 31 && pixels[i].g == 73 && pixels[i].r == 149 && pixels[i].a == 255, "Resized framebuffer lost clear coverage");
        CiRect corner(size.y - 1, size.x - 1, size.y, size.x); TSGrowableArray<float> depth;
        device.DeviceReadDepths(corner, depth);
        Require(depth.Count() == 1 && depth[0] == 1.0f, "Resized depth attachment did not clear its final pixel");
        device.ScenePresent(); device.Drain();
        [view releaseDrawables];
#if TARGET_OS_OSX
        requested.m_vsync = 0;
        Require(device.DeviceSetFormat(requested) && !((CAMetalLayer*)view.layer).displaySyncEnabled, "Native vsync disable did not reach the layer");
        requested.m_vsync = 1;
        Require(device.DeviceSetFormat(requested) && ((CAMetalLayer*)view.layer).displaySyncEnabled, "Native vsync restore did not reach the layer");
#endif
    }
    if (phase < 3) { Request(sizes[phase++]); return false; }
    std::fprintf(stderr, "Native display: real drawable/depth resize, odd pixel dimensions, full-surface GPU readback, input bounds, presentation rejection and active-frame/thread safety passed\n");
    return true;
}
