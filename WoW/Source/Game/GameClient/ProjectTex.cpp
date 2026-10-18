#include "ProjectTex.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/Ui/WorldFrame.h"

// VERIFY: f43753f0 manual
// OFFSET: 0x007E2D60
void ProjectTex2dMakeMatrices(C44Matrix& texMatrix, C44Matrix& fadeMatrix, const CAaBox& box, const C44Matrix* rotation, float fade, int32_t local) {
    C44Matrix translation;
    float centerX = (box.b.x + box.t.x) * 0.5f;
    float centerY = (box.t.y + box.b.y) * 0.5f;
    float centerZ = (box.t.z + box.b.z) * 0.5f;
    C3Vector move;

    // World-local projections need only the box center; the UI camera is
    // required only for the camera-relative branch.
    if (local) {
        move.x = -centerX;
        move.y = -centerY;
        move.z = -centerZ;
    } else {
        C3Vector cameraPos(0.0f, 0.0f, 0.0f);
        CGWorldFrame::GetCameraPosition(&cameraPos);
        move.x = cameraPos.x - centerX;
        move.y = cameraPos.y - centerY;
        move.z = cameraPos.z - centerZ;
    }

    translation.Translate(move);

    float sizeX = box.t.x - box.b.x;
    float sizeY = box.t.y - box.b.y;
    float sizeZ = box.t.z - box.b.z;

    if (sizeX < 0.001f || sizeY < 0.001f) {
        return;
    }

    C44Matrix scale(1.0f / sizeX, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / sizeY, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    C44Matrix spin = C44Matrix::Rotation(-1.5707964f, C3Vector(0.0f, 0.0f, 1.0f), true);
    texMatrix = translation * scale * spin;

    if (rotation) {
        texMatrix *= *rotation;
    }

    texMatrix.d0 = texMatrix.d0 + 0.5f;
    texMatrix.d1 = texMatrix.d1 + 0.5f;
    texMatrix.d2 = texMatrix.d2;

    float depthScale = 1.0f / sizeZ;
    C44Matrix fadeScale(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, depthScale, 0.0f, depthScale, 0.0f, fade, 0.5f, fade, 1.0f);
    fadeMatrix = translation * fadeScale;
}
