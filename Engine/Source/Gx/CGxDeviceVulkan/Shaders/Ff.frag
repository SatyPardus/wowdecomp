// The fixed-function fragment program: D3D9's texture stages 0 and 1 as the engine configures them
// (GxRs_ColorOp0 / GxRs_AlphaOp0 over the texture and the vertex colour, then GxRs_ColorOp1 /
// GxRs_AlphaOp1 over the second texture and that result -- the minimap's circular mask, the
// terrain's alpha maps), then the alpha test, then fog. Without a texture at stage 0 the vertex
// colour passes through.
//
// Fog is D3D9's linear vertex fog (CGxDeviceD3d sets D3DRS_FOGVERTEXMODE to LINEAR): the factor
// (end - z) / (end - start) over the eye-space z, clamped, blended as f * rgb + (1 - f) * fogColor
// while fog is on. The device puts (fogColor.rgb, fogOn) in regs.p[15] and (start, end) in
// regs.p[14]. The far terrain (CMap::RenderLowDetail) relies on it: white vertices with fog from 0
// to 1, drawn as the fog colour.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Push.glsl"
#include "Stage.glsl"
#include "AlphaTest.glsl"

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vTexCoord0;
layout(location = 2) in vec2 vTexCoord1;
layout(location = 3) in float vFogZ;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;

// The translated interface's slice (Tools/VulkanShaders/bls/emitter.py): only p[14] and p[15] are read.
layout(std140, set = 1, binding = 0) uniform Regs
{
    vec4 p[16];
    vec4 misc;
    vec4 v[64];
} regs;

void main()
{
    // A stage without a texture binds the backend's 1x1 white, which Stages() does not read.
    vec4 cur = Stages(vColor, texture(tex0, vTexCoord0), texture(tex1, vTexCoord1));
    AlphaTest(cur.a);

    if (regs.p[15].w > 0.0)
    {
        float range = regs.p[14].y - regs.p[14].x;
        float f = range != 0.0 ? (regs.p[14].y - vFogZ) / range : (vFogZ < regs.p[14].y ? 1.0 : 0.0);
        cur.rgb = mix(regs.p[15].rgb, cur.rgb, clamp(f, 0.0, 1.0));
    }

    outColor = cur;
}
