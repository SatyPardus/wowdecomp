#pragma once
class CGxDeviceMetal;
class CGxTex;
@class MTLRenderPassDescriptor;
// Fill four comparison maps through real depth rasterization, then read each
// texel back. Null outputs are allocated; supplied targets remain caller-owned.
// Optional depths are four contiguous size*size planes. The default pattern is
// shared with the independent five-texel model/WMO/detail reference formulas.
void CreateShadowComparisonDepths(CGxDeviceMetal&, MTLRenderPassDescriptor*, CGxTex*, CGxTex**,
                                  unsigned size = 5, const float* samples = nullptr);
