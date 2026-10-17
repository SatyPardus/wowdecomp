#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_RENDERSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_RENDERSCRIPT_H

#include "WoW/Source/Extension/ScriptObject.h"

// Frame rendering. The frame is painted by the screen layer system (Engine/Source/Scrn/Layer.cpp,
// the EVENT_ID_PAINT handler): every layer (world frame and UI, console, loading screen, system
// messages, fades) paints in z-order, then the scene is presented. The world itself is drawn by one
// of those layers through CGWorldFrame::RenderWorld (WoW/Source/Ui/WorldFrame.cpp). All three hooks
// run with the current CGxDevice (g_theGxDevicePtr) and may draw with it.
class RenderScript : public ScriptObject {
    protected:
    explicit RenderScript(const char* name);

    public:
    // Start of the paint handler, once the Gx context exists and the window is visible, before any
    // layer paints. The viewport is whatever the previous frame left; nothing of this frame is
    // drawn yet. elapsedSec is the time accumulated since the last paint. Not fired for a hidden
    // window (the handler only presents and sleeps in that case).
    virtual void OnFrameBegin(float elapsedSec) {}

    // CGWorldFrame::RenderWorld, after CGWorldFrame::OnWorldRender and the world text batch: the
    // terrain, models, liquids, effects and world text are drawn into the world frame's viewport
    // and the device still holds the world camera's view and projection (they are restored right
    // after this hook). Draw world-space geometry here. Fires only while the world frame renders.
    virtual void OnWorldRendered() {}

    // End of the paint handler: every layer has painted, the full-screen viewport is restored and
    // the font system flushed, the scene has not been presented yet. Draw screen-space overlays on
    // top of the UI here. Fires once per painted frame, whether or not the frame is presented
    // (the no-present debug mode skips the present only).
    virtual void OnFrameEnd() {}
};

#endif
