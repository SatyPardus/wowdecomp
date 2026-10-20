#pragma once
#if defined(__OBJC__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
// Borderless game windows must retain keyboard focus. The window also scopes
// Dock/menu presentation to its own key-window lifetime.
@interface GxMetalWindow : NSWindow
@property(nonatomic, readonly) BOOL gameFullScreenTransition;
- (void)gameFullScreenTransitionFailed;
@end
@protocol GxMetalCursorHost
- (void)updateGameCursor;
@end
#endif
#endif
class CGxFormat;

namespace GxMetal {
// Bracket the scene and its post-processing; screen UI keeps output pixels.
void BeginSceneResolution(float scale);
void EndSceneResolution();
// UI layout limits are in native points; glyphs and name labels use pixels.
float HostPixelScale();
void SetFontRendering(bool enabled);
void RefreshHostCursor(void* nativeView);
#if defined(__OBJC__) && TARGET_OS_OSX
NSCursor* CursorForHostView(void* nativeView);
#endif
// The native host owns its window/scene. Defaults describe its real drawable,
// independently of the legacy DBC's Windows framebuffer choice.
bool GetHostFormat(CGxFormat& format);
// Call before acquiring a host drawable. macOS windowed/fullscreen output uses
// native backing pixels, independently of the scene rendering scale.
// False means a stale activation/resize drawable was discarded; retry next frame.
bool PrepareHostDrawable(void* nativeView);
// Native green-button/menu requests return through original video CVars.
void SetHostFullScreenRequest(void (*request)(const CGxFormat&));
// Publish an actual drawable resize before the original size events run. This
// only updates game display state; the callback must not restart the device.
void SetHostSizeChanged(void (*changed)(const CGxFormat&));
// Coalesce display restarts requested during input/update/paint. The complete
// restart runs after submission, before the next drawable is acquired.
bool DeferDisplayRestart(void (*restart)());
}
