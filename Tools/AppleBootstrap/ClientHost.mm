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
uint32_t synchronousFrames;
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
[[noreturn]] void Fatal(const char* message) {
    // Original initialization and packet dispatch are not exception-safe.
    // Do not unwind partially registered game objects through their full
    // shutdown routines; fail the process visibly at the native boundary.
    std::fprintf(stderr, "Native client stopped: %s\n", message);
    std::fflush(nullptr);
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
    auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
    if (!pass || !drawable) return;
    try {
        device->BeginFrame(pass, drawable);
        const bool running = EventProcessPending();
        PropSelectContext(context);
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
        if (++self.frames == 120) std::fprintf(stderr, "Original client presented 120 frames; Glue screen: %s\n", CGlueMgr::m_currentScreen);
        if (self.frames == (self.frameLimit ? self.frameLimit : 120)) {
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
        else if (!std::strcmp(argv[argument], "--enter-world") && argument + 1 < argc) enterCharacter = argv[++argument];
        else break;
        ++argument;
    }
    if (argc - argument != 2 || ((loginInput || !enterCharacter.empty()) && !local)) {
        std::fprintf(stderr, "Usage: AppleClient [--local-unchecked] [--login-stdin] [--enter-world CHARACTER] /absolute/path/to/Data /absolute/path/to/writable-runtime\n");
        return 2;
    }
    @autoreleasepool {
        try {
            const std::filesystem::path data = std::filesystem::canonical(argv[argument]);
            SelectRuntime(data, argv[argument + 1]);
            if (local) {
                EnableNativeLocalServerPolicy();
                std::fputs("Local development mode: only loopback Grunt/realm endpoints; executable check omitted (server StrictVersionCheck=0 required).\n", stderr);
            }
            if (loginInput) { pendingLogin = std::make_unique<NativeLoginInput>(); pendingLogin->Read(); }
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            StormInitialize();
            SErrRegisterHandler([](uint32_t, const char*, const char*, int32_t, const char*) -> int32_t {
                void* frames[40]; const int count = backtrace(frames, 40);
                backtrace_symbols_fd(frames, count, 2); return 1;
            });
            BaseInitializeGlobal(); SFile::Initialize();
            auto mounts = std::make_unique<ClientData>(data.string(), "");
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
