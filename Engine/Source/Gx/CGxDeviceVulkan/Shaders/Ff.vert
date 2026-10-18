// The fixed-function vertex program: the position through the device's World * View * ProjNative,
// the vertex colour forwarded, texture coordinate sets 0 and 1 through their stages' texture
// transforms, as the engine's fixed-function pipeline would (no lighting, no texgen). The fog
// distance is forwarded for Ff.frag's linear vertex fog: D3D9's eye-space z, which the projection
// leaves in clip w.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Push.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;       // D3DCOLOR, fetched as r, g, b, a; opaque white when the vertex has none
layout(location = 2) in vec2 inTexCoord0;   // texcoord set 0; (0, 0) when the vertex has none
layout(location = 3) in vec2 inTexCoord1;   // texcoord set 1; (0, 0) when the vertex has none

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vTexCoord0;
layout(location = 2) out vec2 vTexCoord1;
layout(location = 3) out float vFogZ;

void main()
{
    vec4 clip   = pc.xform * vec4(inPos, 1.0);
    gl_Position = ToVulkanClip(clip);
    vColor      = inColor;
    vTexCoord0  = TexTransform(0u, inTexCoord0);
    vTexCoord1  = TexTransform(1u, inTexCoord1);
    vFogZ       = clip.w;
}
