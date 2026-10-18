#include "NativeView.h"
#include "WorldPreview.h"
#include "UIComponents.h"
#include "MoviePlayback.h"
#include "NativeDisplayProbe.h"
#include "NativeGuiProbe.h"
#include "Engine/Source/Os/Gui.h"
#include "WoW/Source/Console/ConsoleClient.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/Ui/CGCooldown.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/NativeHost.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Scrn/Scrn.h"
#include <memory>
#include <cstdio>
#include <thread>
#include <chrono>
#include <cstring>

namespace {
HPROPCONTEXT context;
CGxDeviceMetal* renderDevice;
MTLRenderPassDescriptor* renderPass;
int32_t Initialize(const void*, void*) { context = PropGetSelectedContext(); AsyncFileReadInitialize(0, 20); TextureInitialize(); return 1; }
int32_t Destroy(const void*, void*) { TextureDestroy(); AsyncFileReadDestroy(); return 1; }
void Step() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    if (renderDevice) renderDevice->BeginFrame(renderPass);
    EventProcessPending(); PropSelectContext(context);
}
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
class BackgroundWorld {
    WorldPreview world;
    HLAYER layer = nullptr;
public:
    explicit BackgroundWorld(const char* selection) : world(selection) {
        world.EnableInput([] { return !ConsoleGetActive() && CSimpleEditBox::s_currentFocus == nullptr; });
        ScrnLayerCreate(nullptr, 0.0f, 4, this, [](void* owner, const RECTF*, const RECTF*, float elapsed) {
            CRect drawable; g_theGxDevicePtr->CapsWindowSize(drawable);
            static_cast<BackgroundWorld*>(owner)->world.Draw(elapsed, uint32_t(drawable.maxX - drawable.minX), uint32_t(drawable.maxY - drawable.minY));
        }, &layer);
        // The original top layer otherwise occludes lower Scrn layers. This
        // component host leaves it transparent over the original world viewer.
        ScrnLayerSetFlags(CSimpleTop::s_instance->m_screenLayer, 5);
    }
    ~BackgroundWorld() { if (layer) HandleClose(reinterpret_cast<HOBJECT>(layer)); }
};
}
@interface OriginalUIPreview : NSObject <MTKViewDelegate, NSWindowDelegate, NSApplicationDelegate>
@property(nonatomic, weak) WowNativeView* view;
@property(nonatomic) BOOL active;
@property(nonatomic) BOOL stopping;
@property(nonatomic) NSUInteger frames;
@property(nonatomic) NSUInteger frameLimit;
@property(nonatomic, copy) NSString* error;
@property(nonatomic, assign) MoviePlayback* movie;
@property(nonatomic) BOOL displayChecked;
@end
@implementation OriginalUIPreview {
    std::unique_ptr<NativeDisplayProbe> _displayProbe;
}
- (void)stop:(id)sender {
    self.stopping = YES; [self suspend]; [NSApp stop:nil];
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint modifierFlags:0
        timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
}
- (void)suspend {
    if (!self.active) return;
    self.active = NO; self.view.paused = YES; self.view.gameInputActive = NO;
    try { if (self.movie) self.movie->SetActive(false); }
    catch (const std::exception& error) { self.error = [NSString stringWithUTF8String:error.what()]; [self stop:nil]; }
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0); renderDevice->Drain();
}
- (void)resume {
    if (self.active || self.stopping) return;
    self.active = YES; self.view.gameInputActive = YES; EventResetElapsedTime();
    try { if (self.movie) self.movie->SetActive(true); }
    catch (const std::exception& error) { self.error = [NSString stringWithUTF8String:error.what()]; [self stop:nil]; return; }
    OsQueuePut(OS_INPUT_FOCUS, 1, 0, 0, 0); self.view.paused = NO;
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender { [self stop:nil]; return NSTerminateCancel; }
- (BOOL)windowShouldClose:(NSWindow*)sender { [self stop:nil]; return NO; }
- (void)windowDidResignKey:(NSNotification*)n { [self suspend]; }
- (void)windowDidBecomeKey:(NSNotification*)n { [self resume]; }
- (void)applicationDidResignActive:(NSNotification*)n { [self suspend]; }
- (void)applicationDidBecomeActive:(NSNotification*)n { if (self.view.window.isKeyWindow) [self resume]; }
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    OsInputSetDrawableSize(uint32_t(size.width), uint32_t(size.height));
    OsQueuePut(OS_INPUT_SIZE, int32_t(size.width), int32_t(size.height), 0, 0);
}
- (void)drawInMTKView:(MTKView*)view {
    if (!self.active) return;
    if (!GxMetal::PrepareHostDrawable((__bridge void*)view)) return;
    try {
        if (!self.displayChecked && NSProcessInfo.processInfo.environment[@"WOW_DISPLAY_CHECK"]) {
            if (!_displayProbe) _displayProbe = std::make_unique<NativeDisplayProbe>(*renderDevice, view);
            if (_displayProbe->Step()) { self.displayChecked = YES; _displayProbe.reset(); }
            return;
        }
    } catch (const std::exception& error) { _displayProbe.reset(); self.error = [NSString stringWithUTF8String:error.what()]; [self stop:nil]; return; }
    auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
    if (!pass || !drawable) return;
    try {
        renderDevice->BeginFrame(pass, drawable);
        if (!EventProcessPending()) { [self stop:nil]; return; }
        PropSelectContext(context);
        if (renderDevice->FrameCommand()) renderDevice->ScenePresent();
        [self.view.inputContext invalidateCharacterCoordinates];
        if (++self.frames == 120) {
            if (self.movie) self.movie->Report();
            NSLog(@"Original Scrn/Glue UI: 120 visible frames presented");
        }
        if (self.frames == (self.frameLimit ? self.frameLimit : 120)) {
            if (self.movie) self.movie->Report();
            if (NSString* path = NSProcessInfo.processInfo.environment[@"WOW_UI_CAPTURE"]) {
                MTLRenderPassDescriptor* readPass = [pass copy];
                readPass.colorAttachments[0].loadAction = MTLLoadActionLoad;
                readPass.depthAttachment.loadAction = MTLLoadActionLoad;
                renderDevice->BeginFrame(readPass);
                const uint32_t width = uint32_t(drawable.texture.width), height = uint32_t(drawable.texture.height);
                CiRect rect(0, 0, height, width); TSGrowableArray<CImVector> pixels;
                GxReadPixels(rect, pixels);
                TSGrowableArray<float> depth; renderDevice->DeviceReadDepths(rect, depth);
                size_t scenePixels = 0;
                for (uint32_t i = 0; i < depth.Count(); ++i) if (depth[i] > 0 && depth[i] < .94f) ++scenePixels;
                std::printf("Combined Scrn readback: %zu main-scene depth pixels\n", scenePixels);
                FILE* file = std::fopen(path.fileSystemRepresentation, "wb");
                Require(file, "Cannot write UI GPU capture");
                std::fprintf(file, "P6\n%u %u\n255\n", width, height);
                for (uint32_t i = 0; i < pixels.Count(); ++i) {
                    const uint8_t rgb[] = {pixels[i].r, pixels[i].g, pixels[i].b}; std::fwrite(rgb, 1, 3, file);
                }
                const int result = std::fclose(file); renderDevice->ScenePresent();
                Require(result == 0, "Cannot finish UI GPU capture");
                if (NSProcessInfo.processInfo.environment[@"WOW_WORLD"]) Require(scenePixels > 1000, "World did not reach the original Scrn composition");
            }
        }
        if (self.frameLimit && self.frames >= self.frameLimit) [self stop:nil];
    } catch (const std::exception& error) {
        self.error = [NSString stringWithUTF8String:error.what()];
        if (renderDevice->FrameCommand()) renderDevice->ScenePresent();
        [self stop:nil];
    }
}
@end

int main(int argc, char** argv) {
    if (argc == 2 && !std::strcmp(argv[1], "--gui-check")) {
        @autoreleasepool {
            try {
                [NSApplication sharedApplication];
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 480, 240)
                    styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
                window.releasedWhenClosed = NO;
                [window makeKeyAndOrderFront:nil]; [NSApp activateIgnoringOtherApps:YES];
                OsGuiSetGxWindow((__bridge void*)window.contentView);
                CheckNativeGui((__bridge void*)window.contentView, true);
                OsGuiSetGxWindow(nullptr); [window close];
                return 0;
            } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
        }
    }
    if ((argc != 2 && argc != 3) || (argc == 3 && std::strcmp(argv[2], "--preview"))) {
        std::fprintf(stderr, "Usage: AppleUIProbe /path/to/Data [--preview]\n"); return 2;
    }
    @autoreleasepool {
        try {
            [NSApplication sharedApplication];
            ClientData client(argv[1], "");
            for (unsigned cycle = 0; cycle < 2; ++cycle) {
            NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 600, 300)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
            window.releasedWhenClosed = NO;
            WowNativeView* view = [[WowNativeView alloc] initWithFrame:window.contentView.bounds device:nil];
            view.paused = YES; window.contentView = view;
            view.autoResizeDrawable = NO; view.drawableSize = CGSizeMake(600, 300);
            OsGuiSetGxWindow((__bridge void*)view);
            NSString* config = [NSTemporaryDirectory() stringByAppendingPathComponent:[@"wow-ui-" stringByAppendingString:NSUUID.UUID.UUIDString]];
            CVar::Initialize(config.fileSystemRepresentation);
            ConsoleDeviceInitialize("WoW native UI", false, false);
            std::unique_ptr<CGxDevice, void (*)(CGxDevice*)> owner(g_theGxDevicePtr, [](CGxDevice*) { ConsoleDeviceDestroy(); });
            Require(owner && owner->DeviceWindow() == (__bridge void*)view, "Cannot create UI device on its native host");
            Require(view.drawableSize.width == owner->m_format.m_size.x && view.drawableSize.height == owner->m_format.m_size.y,
                "Initial UI format did not reach the native drawable");
            auto& device = *static_cast<CGxDeviceMetal*>(owner.get());
            OsInputSetWindowResizeLock(1);
            Require(!(window.styleMask & NSWindowStyleMaskResizable), "Native resize lock did not restrict the host window");
            OsInputSetWindowResizeLock(0);
            Require(window.styleMask & NSWindowStyleMaskResizable, "Native resize unlock lost the original window style");
            window.styleMask &= ~NSWindowStyleMaskResizable;
            OsInputSetWindowResizeLock(1); OsInputSetWindowResizeLock(0);
            Require(!(window.styleMask & NSWindowStyleMaskResizable), "Native resize unlock made a fixed host resizable");
            window.styleMask |= NSWindowStyleMaskResizable;
            std::puts("Native window resize lock/unlock and fixed-window preservation passed");
            PropInitialize(); EventInitialize(1, 0);
            EventCreateContextEx(1, Initialize, Destroy, 1, 0); Step(); OsInputSetDrawableSize(600, 300);
            auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:600 height:300 mipmapped:NO];
            descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
            renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
            renderPass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
            renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderDevice = &device;
            {
                UIComponents components(true);
                auto* button = components.Button();
                auto* edit = components.Edit();
                view.textInputRectProvider = ^NSRect {
                    auto* focused = CSimpleEditBox::s_currentFocus;
                    CRect rect;
                    if (!focused || !focused->m_cursorTexture || !focused->m_cursorTexture->GetRect(&rect)) return NSZeroRect;
                    float left, bottom, right, top;
                    DDCToNDC(rect.minX, rect.minY, &left, &bottom); DDCToNDC(rect.maxX, rect.maxY, &right, &top);
                    return NSMakeRect(left, bottom, right - left, top - bottom);
                };
                edit->SetText("Apple ", nullptr); CSimpleEditBox::SetKeyboardFocus(edit); Step();
                const auto beforeCaret = [view firstRectForCharacterRange:NSMakeRange(0, 0) actualRange:nullptr];
                view.gameInputActive = YES;
                [view insertText:@"\u4f60\u597d" replacementRange:NSMakeRange(NSNotFound, 0)]; Step();
                Require(std::string(edit->m_text) == u8"Apple \u4f60\u597d", "Original edit box lost committed native text");
                [view setMarkedText:@"ni" selectedRange:NSMakeRange(0, 2) replacementRange:NSMakeRange(NSNotFound, 0)];
                [view setMarkedText:@"\u4f60" selectedRange:NSMakeRange(0, 1) replacementRange:NSMakeRange(NSNotFound, 0)];
                [view insertText:@"\u4f60" replacementRange:NSMakeRange(NSNotFound, 0)]; Step();
                Require(std::string(edit->m_text) == u8"Apple \u4f60\u597d\u4f60", "Original edit box duplicated or lost composition replacement");
                const auto afterCaret = [view firstRectForCharacterRange:NSMakeRange(0, 0) actualRange:nullptr];
                Require(afterCaret.origin.x > beforeCaret.origin.x + 1 && afterCaret.size.height > 1,
                    "Native candidate rectangle did not follow the original edit-box caret");
                auto* lua = FrameScript_GetContext();
                const char* click = "AppleOriginalButton:Click(); assert(AppleOriginalButton.clicks == 1)";
                Require(luaL_loadstring(lua, click) == 0 && lua_pcall(lua, 0, 0, 0) == 0, "Original button Lua callback failed");
                for (int i = 0; i < 120; ++i) Step();
                CRect buttonRect; Require(button->GetRect(&buttonRect), "Original button has no layout rectangle");
                float x, y; DDCToNDC((buttonRect.minX + buttonRect.maxX) * .5f, (buttonRect.minY + buttonRect.maxY) * .5f, &x, &y);
                const NSPoint point = [view convertPoint:NSMakePoint(x * view.bounds.size.width, y * view.bounds.size.height) toView:nil];
                NSEvent* mouse = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:point modifierFlags:0 timestamp:0
                    windowNumber:window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
                [view mouseMoved:mouse]; [view mouseDown:mouse]; [view mouseUp:mouse]; Step();
                const char* clicked = "assert(AppleOriginalButton.clicks == 2)";
                Require(luaL_loadstring(lua, clicked) == 0 && lua_pcall(lua, 0, 0, 0) == 0, "Native click did not reach the original Lua button callback");
                renderPass.colorAttachments[0].loadAction = MTLLoadActionLoad;
                device.BeginFrame(renderPass);
                CiRect rect(0, 0, 300, 600); TSGrowableArray<CImVector> pixels;
                GxReadPixels(rect, pixels); device.ScenePresent();
                size_t visible = 0, colored = 0;
                for (uint32_t i = 0; i < pixels.Count(); ++i) {
                    if (pixels[i].r || pixels[i].g || pixels[i].b) ++visible;
                    if (pixels[i].r > pixels[i].g + 10) ++colored;
                }
                Require(visible > 100, "Original UI did not produce visible glyphs");
                std::printf("Original UI readback: %zu visible pixels, %zu red button pixels\n", visible, colored);
                Require(colored > 100, "Original Glue button texture did not reach the GPU");
                auto consoleKey = [&](NSEventType type) {
                    return [NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:0 timestamp:0
                        windowNumber:window.windowNumber context:nil characters:@"`" charactersIgnoringModifiers:@"`" isARepeat:NO keyCode:50];
                };
                [view keyDown:consoleKey(NSEventTypeKeyDown)]; [view keyUp:consoleKey(NSEventTypeKeyUp)]; Step();
                Require(ConsoleGetActive(), "Native tilde did not reach the original console hotkey");
                [view insertText:@"closeconsole" replacementRange:NSMakeRange(NSNotFound, 0)]; Step();
                NSEvent* enter = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
                    windowNumber:window.windowNumber context:nil characters:@"\r" charactersIgnoringModifiers:@"\r" isARepeat:NO keyCode:36];
                [view keyDown:enter];
                [view keyUp:[NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:0
                    windowNumber:window.windowNumber context:nil characters:@"\r" charactersIgnoringModifiers:@"\r" isARepeat:NO keyCode:36]]; Step();
                Require(!ConsoleGetActive(), "Native text/Enter did not execute closeconsole");

                auto* cooldown = static_cast<CGCooldown*>(CScriptObject::GetScriptObjectByName("AppleOriginalCooldown", CGCooldown::GetObjectType()));
                auto* spellButton = static_cast<CSimpleButton*>(CScriptObject::GetScriptObjectByName("AppleOriginalSpellButton", CSimpleButton::GetObjectType()));
                CRect spellRect; Require(cooldown && spellButton && spellButton->GetRect(&spellRect), "Original spell button has no rectangle");
                DDCToNDC((spellRect.minX + spellRect.maxX) * .5f, (spellRect.minY + spellRect.maxY) * .5f, &x, &y);
                const NSPoint spellPoint = [view convertPoint:NSMakePoint(x * view.bounds.size.width, y * view.bounds.size.height) toView:nil];
                mouse = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:spellPoint modifierFlags:0 timestamp:0
                    windowNumber:window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
                [view mouseMoved:mouse]; [view mouseDown:mouse]; [view mouseUp:mouse]; Step();
                Require(cooldown->m_isShown && cooldown->m_duration == 3000 && cooldown->m_elapsed < 1000,
                    "Native spell click did not start the original Lua cooldown");
                std::puts("Native AppKit click -> original Button Lua -> CGCooldown timer passed");

                if (cycle == 1 && argc == 3) {
                    std::unique_ptr<BackgroundWorld> world;
                    std::unique_ptr<MoviePlayback> movie;
                    if (NSString* selection = NSProcessInfo.processInfo.environment[@"WOW_MOVIE"]) {
                        if (NSProcessInfo.processInfo.environment[@"WOW_WORLD"]) throw std::runtime_error("WOW_MOVIE requires a separate viewer from WOW_WORLD");
                        movie = std::make_unique<MoviePlayback>(selection.UTF8String);
                    }
                    if (NSString* selection = NSProcessInfo.processInfo.environment[@"WOW_WORLD"]) world = std::make_unique<BackgroundWorld>(selection.UTF8String);
                    __attribute__((objc_precise_lifetime)) OriginalUIPreview* delegate = [OriginalUIPreview new];
                    delegate.view = view;
                    delegate.movie = movie.get();
                    delegate.frameLimit = [NSProcessInfo.processInfo.environment[@"WOW_UI_FRAMES"] integerValue];
                    NSApp.delegate = delegate; window.delegate = delegate; view.delegate = delegate;
                    view.device = device.GPU().device; view.framebufferOnly = NO;
                    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float; view.clearDepth = 1;
                    view.clearColor = MTLClearColorMake(.08, .09, .12, 1);
                    window.title = @"WoW — original Glue UI components";
                    [window setContentSize:NSMakeSize(900, 600)];
                    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                    NSMenu* menu = [NSMenu new]; NSMenuItem* item = [NSMenuItem new]; [menu addItem:item];
                    item.submenu = [NSMenu new];
                    [item.submenu addItemWithTitle:@"Quit WoW UI" action:@selector(terminate:) keyEquivalent:@"q"];
                    NSApp.mainMenu = menu;
                    [window center]; [window makeKeyAndOrderFront:nil]; [window makeFirstResponder:view];
                    [NSApp activateIgnoringOtherApps:YES]; [delegate resume]; [NSApp run];
                    [delegate suspend]; view.delegate = nil; window.delegate = nil; NSApp.delegate = nil; [window orderOut:nil];
                    if (delegate.error) throw std::runtime_error(delegate.error.UTF8String);
                }
                device.Drain(); view.gameInputActive = NO; view.textInputRectProvider = nil;
            }
            renderDevice = nullptr; renderPass = nil;
            PropSelectContext(nullptr); EventDestroy(); PropDestroy();
            owner.reset();
            CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager(); CVar::s_configFile = nullptr;
            [NSFileManager.defaultManager removeItemAtPath:config error:nil];
            Require(!OsGuiGetWindow(0), "Destroyed Gx device retained its still-live native view binding");
            std::puts("Original CSimpleTop/edit box: native Unicode and composition, layout, rendering and teardown passed");
            }
            return 0;
        } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
    }
}
