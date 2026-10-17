#ifndef ENGINE_SOURCE_MODEL2_M2SKINNING_H
#define ENGINE_SOURCE_MODEL2_M2SKINNING_H

#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Tempest/C44Matrix.h"

void M2TransformVertices_cpp(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord);
void M2TransformVerticesOneBone_cpp(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord);
void M2TransformVertices_simd(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord);
void M2TransformVerticesOneBone_simd(const C44Matrix* bones, const M2Vertex* vertex, uint32_t count, uint8_t* vertices, int32_t texCoord);

#endif
