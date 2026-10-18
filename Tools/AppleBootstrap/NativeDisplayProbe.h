#pragma once
#import <MetalKit/MetalKit.h>
#include <memory>
class CGxDeviceMetal;
// Consumes four host draw callbacks: size changes take effect when MTKView
// acquires its next drawable, even if no Gx command has started in this callback.
class NativeDisplayProbe {
    struct Impl;
    std::unique_ptr<Impl> m;
public:
    NativeDisplayProbe(CGxDeviceMetal& device, MTKView* view);
    ~NativeDisplayProbe();
    bool Step(); // true after the original size has been drawn and restored
};
