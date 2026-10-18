#pragma once
class CSimpleCamera;

namespace World {
// A host owns the camera. Binding is scoped to one synchronous world update or
// render, including nested horizon/shadow callbacks; UI callers keep their
// existing active WorldFrame camera when no explicit binding is present.
class ViewCameraScope {
public:
    explicit ViewCameraScope(const CSimpleCamera& camera);
    ~ViewCameraScope();
    ViewCameraScope(const ViewCameraScope&) = delete;
    ViewCameraScope& operator=(const ViewCameraScope&) = delete;
private:
    const CSimpleCamera* m_previous;
};
const CSimpleCamera& ViewCamera();
struct ScenePassCallbacks {
    void* context = nullptr;
    void (*afterOpaque)(void*) = nullptr;
    void (*afterTranslucent)(void*) = nullptr;
};
struct LoadNotifications {
    void (*begin)(int mapId, bool loadingWorld) = nullptr;
    void (*progress)(float progress, void* context) = nullptr;
    void (*end)() = nullptr;
};
}
