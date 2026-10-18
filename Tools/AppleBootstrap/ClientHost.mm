#include "NativeView.h"
#include "LoginInput.h"
#include "WoW/Source/Apple/LocalServerPolicy.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowServices/Apple/ClientSettings.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CharacterSelection.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#import <Carbon/Carbon.h>
#include "Engine/Source/Base/Base.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/NativeHost.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/OsIME.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Storm/H/storm.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <execinfo.h>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <cmath>

extern HEVENTCONTEXT g_clientEventContext;
void LoadMopaqFileSigningData(const char* locale);
int32_t InitializeEngineCallback(const void*, void*);
int32_t DestroyEngineCallback(const void*, void*);

namespace {
CGxDeviceMetal* device;
HPROPCONTEXT context;
bool initialized;
std::unique_ptr<NativeLoginInput> pendingLogin;
std::string enterCharacter;
bool entryQueued;
bool checkWorldInput, worldInputDone;
bool appBundle;
uint32_t synchronousFrames;
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
// Explicit local development option: exercise the native input boundary and
// original bindings/movement, never set player coordinates or camera state.
void CheckWorldInput(WowNativeView* view) {
    static unsigned readyFrames, phase;
    static std::chrono::steady_clock::time_point started;
    static C3Vector start, jumpStart;
    static float facing, distance;
    if (worldInputDone) return;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase, "World left during native input check"); return;
    }
    if (!phase && ++readyFrames < 120) return;
    auto* camera = CGWorldFrame::GetActiveCamera();
    Require(camera != nullptr, "World input check has no camera");
    auto key = [&](unsigned short code, NSString* text, bool down) {
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto wheel = [&](int delta) {
        CGEventRef event = CGEventCreateScrollWheelEvent(nullptr,kCGScrollEventUnitLine,1,delta);
        Require(event != nullptr,"Cannot create native wheel check event");
        // A synthesized NSEvent has no window. For direct view delivery, make
        // locationInWindow contain this view's center in window coordinates.
        const NSPoint point=[view convertPoint:NSMakePoint(NSMidX(view.bounds),NSMidY(view.bounds)) toView:nil];
        // Original UI wheel hit-testing uses the last mouse-move position.
        [view mouseMoved:[NSEvent mouseEventWithType:NSEventTypeMouseMoved location:point modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil
            eventNumber:0 clickCount:0 pressure:0]];
        CGEventSetLocation(event,CGPointMake(point.x,NSMaxY(NSScreen.screens.firstObject.frame)-point.y));
        NSEvent* native=[NSEvent eventWithCGEvent:event];
        Require(NSPointInRect([view convertPoint:native.locationInWindow fromView:nil],view.bounds),"Synthetic wheel missed the native game view");
        [view scrollWheel:native]; CFRelease(event);
    };
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now-started).count();
    auto next = [&] { ++phase; started=now; };
    if (!phase) {
        auto* bindings=CGUIBindings::s_bindings;
        Require(bindings != nullptr,"Native input check requires original loaded bindings");
        for (const auto& pair : {std::pair{"W","MOVEFORWARD"}, {"S","MOVEBACKWARD"}, {"LEFT","TURNLEFT"},
            {"RIGHT","TURNRIGHT"}, {"MOUSEWHEELUP","CAMERAZOOMIN"}, {"MOUSEWHEELDOWN","CAMERAZOOMOUT"}, {"SPACE","JUMP"}}) {
            const char* action=bindings->GetCommandAction(BINDING_MODE_ACTIVE,pair.first,1);
            Require(action && !std::strcmp(action,pair.second),"Native input check keys do not match current character bindings");
        }
        start=player->GetPosition(); key(kVK_ANSI_W,@"w",true); next();
    }
    else if (phase==1 && elapsed>=0.4) { key(kVK_ANSI_W,@"w",false); next(); }
    else if (phase==2 && elapsed>=0.2) {
        const auto position=player->GetPosition();
        const float moved=std::hypot(position.x-start.x,position.y-start.y);
        std::fprintf(stderr,"Native world input: forward moved %.3f units\n",moved);
        Require(moved>0.3f && moved<15.0f,"Native W did not move the player as expected");
        key(kVK_ANSI_S,@"s",true); next();
    } else if (phase==3 && elapsed>=0.6) { key(kVK_ANSI_S,@"s",false); next(); }
    else if (phase==4 && elapsed>=0.3) { facing=player->GetFacing(); key(kVK_LeftArrow,@"\uF702",true); next(); }
    else if (phase==5 && elapsed>=0.2) { key(kVK_LeftArrow,@"\uF702",false); next(); }
    else if (phase==6 && elapsed>=0.3) {
        const float turned=std::remainder(player->GetFacing()-facing,6.283185307f);
        std::fprintf(stderr,"Native world input: turn changed facing %.3f radians\n",turned);
        Require(std::fabs(turned)>0.05f && std::fabs(turned)<2,"Native left arrow did not turn the player as expected");
        key(kVK_RightArrow,@"\uF703",true); next();
    } else if (phase==7 && elapsed>=0.2) { key(kVK_RightArrow,@"\uF703",false); next(); }
    else if (phase==8 && elapsed>=0.3) { distance=camera->m_smoothDistance.m_target; wheel(1); next(); }
    else if (phase==9 && elapsed>=0.6) {
        std::fprintf(stderr,"Native world input: camera target distance %.3f -> %.3f\n",distance,camera->m_smoothDistance.m_target);
        Require(distance-camera->m_smoothDistance.m_target>0.5f && distance-camera->m_smoothDistance.m_target<2.0f,"Native wheel did not zoom in by the original step");
        wheel(-1); next();
    } else if (phase==10 && elapsed>=0.6) {
        Require(std::fabs(camera->m_smoothDistance.m_target-distance)<0.2f,"Native zoom-out did not restore the camera target");
        jumpStart=player->GetPosition(); key(kVK_Space,@" ",true); next();
    }
    else if (phase==11 && elapsed>=0.05) { key(kVK_Space,@" ",false); next(); }
    else if (phase==12 && elapsed>=0.2) {
        const float risen=player->GetPosition().z-jumpStart.z;
        std::fprintf(stderr,"Native world input: jump rose %.3f units\n",risen);
        Require(risen>0.1f,"Native space did not start a jump"); next();
    } else if (phase==13 && elapsed>=1.5) {
        Require(std::fabs(player->GetPosition().z-jumpStart.z)<1.0f,"Player did not land after native jump");
        worldInputDone=true;
        std::fputs("Native world input: movement, turn, camera wheel and jump/landing passed through original bindings\n",stderr);
    }
}
[[noreturn]] void Fatal(const char* message) {
    // Original initialization and packet dispatch are not exception-safe.
    // Do not unwind partially registered game objects through their full
    // shutdown routines; fail the process visibly at the native boundary.
    std::fprintf(stderr, "Native client stopped: %s\n", message);
    std::fflush(nullptr);
    if (appBundle && NSThread.isMainThread && !std::getenv("WOW_CLIENT_FRAMES")) {
        if (device && device->DeviceWindow()) ((__bridge MTKView*)device->DeviceWindow()).paused = YES;
        NSAlert* alert=[NSAlert new];
        alert.messageText=@"WoW Apple 无法继续运行";
        alert.informativeText=[NSString stringWithUTF8String:message] ?: @"发生未知错误，请查看运行目录中的日志。";
        [alert addButtonWithTitle:@"关闭"]; [alert runModal];
    }
    std::_Exit(EXIT_FAILURE);
}
int32_t Initialize(const void* data, void* param) {
    context = PropGetSelectedContext();
    std::fprintf(stderr, "Starting original engine and client initialization\n");
    const int32_t result = InitializeEngineCallback(data, param);
    initialized = true;
    std::fprintf(stderr, "Original client initialized; Glue screen: %s\n", CGlueMgr::m_currentScreen);
    return result;
}
int32_t EnterSelectedCharacter(const void*, void*) {
    const auto index = CCharacterSelection::s_selectionIndex;
    Require(CGlueMgr::m_idleState == IDLE_NONE && !CGlueMgr::m_suspended &&
        index >= 0 && uint32_t(index) < CCharacterSelection::s_characterList.Count(), "Selected character is not ready for world entry");
    const auto& info = CCharacterSelection::s_characterList[index].m_characterInfo;
    Require(enterCharacter == info.m_name, "World entry requires the explicitly named character to be selected");
    std::fprintf(stderr, "Entering original world as %s on map %u\n", info.m_name, info.m_mapID);
    enterCharacter.clear();
    CGlueMgr::EnterWorld();
    return 1;
}
int32_t Destroy(const void* data, void* param) {
    if (!initialized) return 1;
    initialized = false;
    const int32_t result = DestroyEngineCallback(data, param);
    std::fprintf(stderr, "Original client shutdown completed\n");
    return result;
}
bool AcquireSynchronousFrame(CGxDeviceMetal& gpu, void* owner) {
    MTKView* view = (__bridge MTKView*)owner;
    if (!view || view.paused || !view.window) return false;
    if (!GxMetal::PrepareHostDrawable(owner)) return false;
    // MTKView normally retains one drawable for its draw callback. Original
    // map loading may present several times before that callback returns.
    [view releaseDrawables];
    auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
    if (!pass || !drawable) return false;
    gpu.BeginFrame(pass, drawable); ++synchronousFrames;
    return true;
}
void SelectRuntime(const std::filesystem::path& data, const std::filesystem::path& requested) {
    namespace fs = std::filesystem;
    Require(data.is_absolute() && requested.is_absolute(), "Data and runtime roots must be absolute paths");
    const auto install = fs::canonical(data).parent_path();
    const auto root = fs::weakly_canonical(requested);
    const auto relative = root.lexically_relative(install);
    Require(!relative.empty() && *relative.begin() == "..", "Runtime root must be outside the original client installation");
    fs::create_directories(root);
    for (const auto& entry : {std::pair{"WOW_SETTINGS_ROOT", "WTF"}, {"WOW_CACHE_ROOT", "Cache"}, {"WOW_LOG_ROOT", "Logs"}}) {
        const auto path = root / entry.second;
        fs::create_directories(path);
        Require(!setenv(entry.first, path.c_str(), 1), "Cannot select writable native runtime paths");
    }
    // Remaining legacy relative output paths (GlueXML.log, screenshots, etc.)
    // are confined to this explicit writable runtime, never the installation.
    fs::current_path(root);
}
bool SelectBundlePaths(std::filesystem::path& data, std::filesystem::path& runtime, bool& local) {
    NSUserDefaults* preferences=NSUserDefaults.standardUserDefaults;
    NSString* saved=[preferences stringForKey:@"NativeDataRoot"];
    NSString* savedRuntime=[preferences stringForKey:@"NativeRuntimeRoot"];
    const bool choose=(NSEvent.modifierFlags & NSEventModifierFlagOption) || !saved.length ||
        !std::filesystem::is_directory(saved.fileSystemRepresentation);
    if (choose) {
        NSOpenPanel* panel=[NSOpenPanel openPanel];
        panel.title=@"选择原版 3.3.5a 客户端的 Data 文件夹";
        panel.message=@"游戏资源保持只读；配置、缓存和日志保存在独立运行目录。按住 Option 启动可重新选择。";
        panel.canChooseFiles=NO; panel.canChooseDirectories=YES; panel.allowsMultipleSelection=NO;
        if (saved.length) panel.directoryURL=[NSURL fileURLWithPath:saved isDirectory:YES];
        NSButton* development=[NSButton checkboxWithTitle:@"本机 AzerothCore（跳过原 Windows 可执行文件校验）" target:nil action:nil];
        development.state=(local || [preferences boolForKey:@"NativeLocalDevelopment"]) ? NSControlStateValueOn : NSControlStateValueOff;
        panel.accessoryView=development;
        [NSApp activateIgnoringOtherApps:YES];
        if ([panel runModal]!=NSModalResponseOK) return false;
        saved=panel.URL.path; local=development.state==NSControlStateValueOn;
    } else {
        local=local || [preferences boolForKey:@"NativeLocalDevelopment"];
    }
    if (!savedRuntime.length) {
        NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;
        Require(support.length,"Cannot locate application support directory");
        savedRuntime=[support stringByAppendingPathComponent:@"WoWDecompClient"];
    }
    data=saved.fileSystemRepresentation; runtime=savedRuntime.fileSystemRepresentation;
    return true;
}
}

@interface OriginalClientHost : NSObject <MTKViewDelegate, NSWindowDelegate, NSApplicationDelegate>
@property(nonatomic, weak) WowNativeView* view;
@property(nonatomic) BOOL active;
@property(nonatomic) BOOL stopping;
@property(nonatomic) NSUInteger frames;
@property(nonatomic) NSUInteger characterFrames;
@property(nonatomic) NSUInteger frameLimit;
@end
@implementation OriginalClientHost
- (void)stop:(id)sender {
    if (self.stopping) return;
    self.stopping = YES;
    [self suspend];
    [NSApp stop:nil];
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint modifierFlags:0
        timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
}
- (void)suspend {
    if (!self.active) return;
    self.active = NO; self.view.paused = YES; self.view.gameInputActive = NO;
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0);
    try {
        if (initialized) {
            const int32_t focus = 0;
            PropSelectContext(context); SI2::OnFocusChanged(&focus, nullptr);
        }
        device->Drain();
    }
    catch (const std::exception& error) { Fatal(error.what()); }
}
- (void)resume {
    if (self.active || self.stopping) return;
    self.active = YES; self.view.gameInputActive = YES;
    EventResetElapsedTime(); OsQueuePut(OS_INPUT_FOCUS, 1, 0, 0, 0);
    self.view.paused = NO;
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
    auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
    if (!pass || !drawable) return;
    if (!self.frames) std::fprintf(stderr,"Native display: bounds %.0fx%.0f, backing %.0fx%.0f, window scale %.1f, layer scale %.1f, drawable %.0fx%.0f, texture %lux%lu, format %dx%d\n",
        view.bounds.size.width,view.bounds.size.height,[view convertRectToBacking:view.bounds].size.width,[view convertRectToBacking:view.bounds].size.height,
        view.window.backingScaleFactor,view.layer.contentsScale,view.drawableSize.width,view.drawableSize.height,
        (unsigned long)drawable.texture.width,(unsigned long)drawable.texture.height,device->m_format.m_size.x,device->m_format.m_size.y);
    try {
        device->BeginFrame(pass, drawable);
        const bool running = EventProcessPending();
        PropSelectContext(context);
        if (checkWorldInput) CheckWorldInput(self.view);
        if (device->FrameCommand()) device->ScenePresent();
        [view.inputContext invalidateCharacterCoordinates];
        if (pendingLogin && self.frames == 2) {
            Require(CGlueMgr::m_acceptedTOS && CGlueMgr::m_acceptedEULA, "Original agreement preferences are required before stdin login");
            Require(!std::strcmp(CGlueMgr::m_currentScreen, "login") && CGlueMgr::m_idleState == IDLE_NONE,
                "Stdin login requires the idle original login screen");
            CGlueMgr::LoginServerLogin(pendingLogin->account.c_str(), pendingLogin->password.data());
            pendingLogin.reset();
            Require(CGlueMgr::m_idleState == IDLE_LOGIN_SERVER_LOGIN, "Original Glue did not start login");
        }
        if (!enterCharacter.empty() && !entryQueued && !std::strcmp(CGlueMgr::m_currentScreen, "charselect") && CGlueMgr::m_idleState == IDLE_NONE) {
            if (++self.characterFrames == 60) {
                // Enter through the original timer phase of the next game
                // frame, with a drawable active for loading-screen paints.
                entryQueued = true; EventSetTimer(0, EnterSelectedCharacter, nullptr);
            }
        }
        if (++self.frames == 120) std::fprintf(stderr, "Original client presented 120 frames; in world: %u; Glue screen: %s\n", CGGameUI::s_inWorld, CGlueMgr::m_currentScreen);
        if (self.frames == (self.frameLimit ? self.frameLimit : 120)) {
            if (self.frameLimit) Require(!checkWorldInput || worldInputDone,"Frame limit reached before native world input check completed");
            if (NSString* file = NSProcessInfo.processInfo.environment[@"WOW_CLIENT_CAPTURE"]) {
                MTLRenderPassDescriptor* readPass = [pass copy];
                readPass.colorAttachments[0].loadAction = MTLLoadActionLoad;
                readPass.depthAttachment.loadAction = MTLLoadActionLoad;
                device->BeginFrame(readPass);
                const uint32_t width = uint32_t(drawable.texture.width), height = uint32_t(drawable.texture.height);
                TSGrowableArray<CImVector> pixels;
                CiRect rect(0, 0, height, width); GxReadPixels(rect, pixels);
                device->ScenePresent();
                NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr
                    pixelsWide:width pixelsHigh:height bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES
                    isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:width * 4 bitsPerPixel:32];
                Require(bitmap != nil, "Cannot allocate native client capture");
                for (uint32_t i = 0; i < pixels.Count(); ++i) {
                    auto* out = bitmap.bitmapData + i * 4;
                    out[0] = pixels[i].r; out[1] = pixels[i].g; out[2] = pixels[i].b; out[3] = 255;
                }
                Require([[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:file atomically:YES],
                    "Cannot save native client capture");
            }
        }
        if (!running || (self.frameLimit && self.frames >= self.frameLimit)) [self stop:nil];
    } catch (const std::exception& error) { Fatal(error.what()); }
}
@end

int main(int argc, char** argv) {
    int argument = 1;
    bool local = false, loginInput = false;
    while (argument < argc && !std::strncmp(argv[argument], "--", 2)) {
        if (!std::strcmp(argv[argument], "--local-unchecked")) local = true;
        else if (!std::strcmp(argv[argument], "--login-stdin")) loginInput = true;
        else if (!std::strcmp(argv[argument], "--check-world-input")) checkWorldInput = true;
        else if (!std::strcmp(argv[argument], "--enter-world") && argument + 1 < argc) enterCharacter = argv[++argument];
        else break;
        ++argument;
    }
    if ((argc - argument != 2 && argc - argument != 0) || ((loginInput || !enterCharacter.empty()) && !local) || (checkWorldInput && enterCharacter.empty())) {
        std::fprintf(stderr, "Usage: AppleClient [--local-unchecked] [--login-stdin] [--enter-world CHARACTER] [--check-world-input] /absolute/path/to/Data /absolute/path/to/writable-runtime\n");
        return 2;
    }
    @autoreleasepool {
        try {
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            appBundle=[NSBundle.mainBundle.bundleURL.pathExtension isEqualToString:@"app"];
            std::filesystem::path data, runtime;
            if (argc - argument == 2) { data=argv[argument]; runtime=argv[argument+1]; }
            else {
                Require(appBundle,"The command-line client requires Data and runtime paths");
                if (!SelectBundlePaths(data,runtime,local)) return 0;
            }
            data=std::filesystem::canonical(data);
            SelectRuntime(data, runtime);
            if (local) {
                EnableNativeLocalServerPolicy();
                std::fputs("Local development mode: only loopback Grunt/realm endpoints; executable check omitted (server StrictVersionCheck=0 required).\n", stderr);
            }
            if (loginInput) { pendingLogin = std::make_unique<NativeLoginInput>(); pendingLogin->Read(); }
            StormInitialize();
            SErrRegisterHandler([](uint32_t, const char*, const char*, int32_t, const char*) -> int32_t {
                void* frames[40]; const int count = backtrace(frames, 40);
                backtrace_symbols_fd(frames, count, 2); return 1;
            });
            BaseInitializeGlobal(); SFile::Initialize();
            auto mounts = std::make_unique<ClientData>(data.string(), "");
            if (appBundle) {
                NSUserDefaults* preferences=NSUserDefaults.standardUserDefaults;
                [preferences setObject:[NSString stringWithUTF8String:data.c_str()] forKey:@"NativeDataRoot"];
                [preferences setObject:[NSString stringWithUTF8String:std::filesystem::current_path().c_str()] forKey:@"NativeRuntimeRoot"];
                [preferences setBool:local forKey:@"NativeLocalDevelopment"];
                NSMenu* menu=[NSMenu new]; NSMenuItem* item=[NSMenuItem new]; [menu addItem:item];
                NSMenu* application=[NSMenu new];
                [application addItemWithTitle:@"退出 WoW Apple" action:@selector(terminate:) keyEquivalent:@"q"];
                item.submenu=application; NSApp.mainMenu=menu;
            }
            ConsoleInitializeClientCommand();
            std::string settings;
            Require(NativeSettingsFile(settings, "Config.wtf", true), "Cannot locate native Config.wtf");
            CVar::Initialize(settings.c_str());
            if (std::filesystem::exists(settings)) Require(CVar::Load(settings.c_str()), "Cannot load native Config.wtf");
            CVar::ArchiveCodeRegisteredOnly();
            CVar::Register("locale", "Game locale selected from mounted Data", 0, mounts->Locale().c_str(), nullptr, CATEGORY_DEFAULT, false, nullptr, false);
            ClientServices::InitLoginServerCVars(0, mounts->Locale().c_str());
            if (local) ClientServices::s_realmListVar->Set("127.0.0.1:3724", true, false, false, true);
            LoadMopaqFileSigningData(mounts->Locale().c_str());
            EventInitialize(1, 0); OsTimeStartup(OsTimeManager::BestAvailable); OsIMEInitialize();

            NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1024, 768)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                backing:NSBackingStoreBuffered defer:NO];
            window.releasedWhenClosed = NO;
            WowNativeView* view = [[WowNativeView alloc] initWithFrame:window.contentView.bounds device:nil];
            view.paused = YES; window.contentView = view;
            view.autoResizeDrawable = NO; view.drawableSize = CGSizeMake(1024, 768);
            OsGuiSetGxWindow((__bridge void*)view);
            ConsoleDeviceInitialize("World of Warcraft", false, false);
            device = static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
            Require(device && device->DeviceWindow() == (__bridge void*)view, "Cannot create the native client display");
            view.device = device->GPU().device;
            view.colorPixelFormat = MTLPixelFormatBGRA8Unorm; view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
            device->SetHostFrameProvider(AcquireSynchronousFrame, (__bridge void*)view);
            view.framebufferOnly = NO; view.clearColor = MTLClearColorMake(0, 0, 0, 1);
            view.textInputRectProvider = ^NSRect {
                auto* focused = CSimpleEditBox::s_currentFocus; CRect rect;
                if (!focused || !focused->m_cursorTexture || !focused->m_cursorTexture->GetRect(&rect)) return NSZeroRect;
                float left, bottom, right, top;
                DDCToNDC(rect.minX, rect.minY, &left, &bottom); DDCToNDC(rect.maxX, rect.maxY, &right, &top);
                return NSMakeRect(left, bottom, right - left, top - bottom);
            };
            g_clientEventContext = EventCreateContextEx(1, Initialize, Destroy, 1, 0);
            Require(g_clientEventContext != nullptr, "Cannot create the original client context");
            OriginalClientHost* host = [OriginalClientHost new]; host.view = view;
            host.frameLimit = [NSProcessInfo.processInfo.environment[@"WOW_CLIENT_FRAMES"] integerValue];
            view.delegate = host; window.delegate = host; NSApp.delegate = host;
            [window center]; [window makeKeyAndOrderFront:nil]; [NSApp activateIgnoringOtherApps:YES];
            [host resume]; [NSApp run]; [host suspend];
            view.delegate = nil; window.delegate = nil; NSApp.delegate = nil;
            OsIMEDestroy(); OsTimeShutdown(); EventDestroy();
            device->SetHostFrameProvider(nullptr, nullptr);
            ConsoleDeviceDestroy(); device = nullptr;
            std::fprintf(stderr, "Original synchronous loading frames: %u\n", synchronousFrames);
            Require(CVar::SaveAll(), "Cannot save native Config.wtf");
            CVar::Destroy(); ConsoleCommandDestroy(); CVar::s_configFile = nullptr;
            mounts.reset(); BaseDestroyGlobal(); GxLogClose(); StormDestroy();
            [window close];
            return 0;
        } catch (const std::exception& error) { Fatal(error.what()); }
    }
}
