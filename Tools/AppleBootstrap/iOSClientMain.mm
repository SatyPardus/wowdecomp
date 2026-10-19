#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#include "LoginInput.h"
#include "ClientMemory.h"
#include "WoW/Source/Apple/LocalServerPolicy.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "WoW/Source/WowServices/Apple/ClientSettings.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CharacterSelection.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/Base/Base.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/OsIME.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Storm/H/storm.h"
#include <filesystem>
#include <memory>
#include <execinfo.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>

extern HEVENTCONTEXT g_clientEventContext;
void LoadMopaqFileSigningData(const char* locale);
int32_t InitializeEngineCallback(const void*, void*);
int32_t DestroyEngineCallback(const void*, void*);

namespace {
CGxDeviceMetal* device;
HPROPCONTEXT context;
std::unique_ptr<ClientData> mounts;
std::string settings, enterCharacter;
#if TARGET_OS_SIMULATOR
std::string loginPipe;
#endif
std::shared_ptr<NativeLoginInput> pendingLogin;
bool initialized, localDevelopment, stdinLogin, entryQueued, processingEvents;
NativeClientMemory memoryPressure;
uint32_t synchronousFrames;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
[[noreturn]] void Fatal(const char* message) {
    // The original engine cannot unwind a partially initialized world or a
    // failed packet handler safely. Normal shutdown uses its full destructor.
    std::fprintf(stderr, "Native iOS client stopped: %s\n", message); std::fflush(nullptr);
    std::_Exit(EXIT_FAILURE);
}
int32_t Initialize(const void* data, void* param) {
    context = PropGetSelectedContext();
    const auto result = InitializeEngineCallback(data, param);
    initialized = true;
    std::fprintf(stderr, "Original iOS client initialized; Glue screen: %s\n", CGlueMgr::m_currentScreen);
    return result;
}
int32_t Destroy(const void* data, void* param) {
    if (!initialized) return 1;
    initialized = false;
    const auto result = DestroyEngineCallback(data, param);
    std::fputs("Original iOS client shutdown completed\n", stderr); return result;
}
int32_t EnterSelectedCharacter(const void*, void*) {
    const auto index = CCharacterSelection::s_selectionIndex;
    Require(CGlueMgr::m_idleState == IDLE_NONE && !CGlueMgr::m_suspended &&
        index >= 0 && uint32_t(index) < CCharacterSelection::s_characterList.Count(), "Selected character is not ready for world entry");
    const auto& info = CCharacterSelection::s_characterList[index].m_characterInfo;
    Require(enterCharacter == info.m_name, "World entry requires the explicitly named character to be selected");
    std::fprintf(stderr, "Entering original iOS world as %s on map %u\n", info.m_name, info.m_mapID);
    enterCharacter.clear(); CGlueMgr::EnterWorld(); return 1;
}
bool AcquireSynchronousFrame(CGxDeviceMetal& gpu, void* owner) {
    MTKView* view = (__bridge MTKView*)owner;
    if (!view || view.paused || !view.window) return false;
    // UIKit MTKView retains the display-link callback's drawable. Original
    // loading screens can present repeatedly inside that callback, so acquire
    // each subsequent surface directly from its Metal layer.
    id<CAMetalDrawable> drawable = [(CAMetalLayer*)view.layer nextDrawable];
    if (!drawable) return false;
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = view.clearColor;
    pass.depthAttachment.texture = view.depthStencilTexture;
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = view.clearDepth;
    gpu.BeginFrame(pass, drawable); ++synchronousFrames; return true;
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
        const auto path = root / entry.second; fs::create_directories(path);
        Require(!setenv(entry.first, path.c_str(), 1), "Cannot select writable native runtime paths");
    }
    fs::current_path(root);
}
void Capture(MTLRenderPassDescriptor* pass, id<CAMetalDrawable> drawable, NSString* file) {
    MTLRenderPassDescriptor* readPass = [pass copy];
    readPass.colorAttachments[0].loadAction = MTLLoadActionLoad;
    readPass.depthAttachment.loadAction = MTLLoadActionLoad;
    device->BeginFrame(readPass);
    const uint32_t width = uint32_t(drawable.texture.width), height = uint32_t(drawable.texture.height);
    TSGrowableArray<CImVector> pixels; CiRect rect(0, 0, height, width); GxReadPixels(rect, pixels);
    device->ScenePresent();
    NSMutableData* rgba = [NSMutableData dataWithLength:size_t(width) * height * 4];
    Require(rgba != nil, "Cannot allocate iOS capture");
    auto* bytes = static_cast<uint8_t*>(rgba.mutableBytes);
    for (uint32_t i = 0; i < pixels.Count(); ++i) {
        bytes[i*4] = pixels[i].r; bytes[i*4+1] = pixels[i].g; bytes[i*4+2] = pixels[i].b; bytes[i*4+3] = 255;
    }
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)rgba);
    CGColorSpaceRef color = CGColorSpaceCreateDeviceRGB();
    CGImageRef image = CGImageCreate(width, height, 8, 32, width*4, color, kCGImageAlphaPremultipliedLast, provider, nullptr, false, kCGRenderingIntentDefault);
    NSData* png = image ? UIImagePNGRepresentation([UIImage imageWithCGImage:image]) : nil;
    if (image) CGImageRelease(image);
    CGColorSpaceRelease(color); CGDataProviderRelease(provider);
    Require(png && [png writeToFile:file atomically:YES], "Cannot save iOS client capture");
}
}

@interface NativeClientController : UIViewController
@property(nonatomic, strong) MTKView* metalView;
@end
@implementation NativeClientController
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    self.metalView.frame = self.view.bounds;
    const CGFloat scale = self.view.window.screen.scale;
    const CGSize pixels = CGSizeMake(self.view.bounds.size.width * scale, self.view.bounds.size.height * scale);
    if (pixels.width > 0 && pixels.height > 0 && !CGSizeEqualToSize(self.metalView.drawableSize, pixels)) {
        self.metalView.autoResizeDrawable = NO; [self.metalView releaseDrawables]; self.metalView.drawableSize = pixels;
    }
}
- (BOOL)prefersStatusBarHidden { return YES; }
@end

@interface NativeClientScene : UIResponder <UIWindowSceneDelegate, MTKViewDelegate>
@property(nonatomic, strong) UIWindow* window;
@property(nonatomic, strong) NativeClientController* controller;
@property(nonatomic, strong) UILabel* status;
@property(nonatomic) BOOL started, active, finished;
@property(nonatomic) NSUInteger frames, characterFrames, frameLimit, resumeFrame;
@end
@implementation NativeClientScene
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
    self.controller = [NativeClientController new];
    self.controller.view.backgroundColor = UIColor.blackColor;
    self.controller.metalView = [[MTKView alloc] initWithFrame:self.window.bounds device:nil];
    self.controller.metalView.paused = YES;
    [self.controller.view addSubview:self.controller.metalView];
    self.status = [[UILabel alloc] initWithFrame:self.window.bounds];
    self.status.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.status.textColor = UIColor.whiteColor; self.status.numberOfLines = 0; self.status.textAlignment = NSTextAlignmentCenter;
    [self.controller.view addSubview:self.status];
    self.window.rootViewController = self.controller; [self.window makeKeyAndVisible];
}
- (void)start {
    NSString* documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    NSString* data = NSProcessInfo.processInfo.environment[@"WOW_DATA_ROOT"] ?: [documents stringByAppendingPathComponent:@"Data"];
    if (!std::filesystem::is_directory(data.fileSystemRepresentation)) {
        self.status.text = @"通过“文件”或 Finder 将原版 Data 文件夹复制到此应用的 Documents。\n配置、缓存和日志使用独立的 Library 目录。\n导入后重新切回应用。";
        return;
    }
    NSString* support = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
    NSString* runtime = NSProcessInfo.processInfo.environment[@"WOW_RUNTIME_ROOT"] ?: [support stringByAppendingPathComponent:@"WoWDecompClient"];
    SelectRuntime(data.fileSystemRepresentation, runtime.fileSystemRepresentation);
    if (localDevelopment) {
        EnableNativeLocalServerPolicy();
        std::fputs("Local development mode: loopback endpoints only; original executable check omitted (StrictVersionCheck=0 required).\n", stderr);
    }
    StormInitialize();
    SErrRegisterHandler([](uint32_t, const char*, const char*, int32_t, const char*) -> int32_t {
        void* frames[40]; const int count = backtrace(frames, 40); backtrace_symbols_fd(frames, count, 2); return 1;
    });
    BaseInitializeGlobal(); SFile::Initialize();
    mounts = std::make_unique<ClientData>(data.fileSystemRepresentation, "");
    ConsoleInitializeClientCommand();
    Require(NativeSettingsFile(settings, "Config.wtf", true), "Cannot locate native Config.wtf");
    CVar::Initialize(settings.c_str());
    if (std::filesystem::exists(settings)) Require(CVar::Load(settings.c_str()), "Cannot load native Config.wtf");
    CVar::ArchiveCodeRegisteredOnly();
    CVar::Register("locale", "Game locale selected from mounted Data", 0, mounts->Locale().c_str(), nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    ClientServices::InitLoginServerCVars(0, mounts->Locale().c_str());
    if (localDevelopment) ClientServices::s_realmListVar->Set("127.0.0.1:3724", true, false, false, true);
    LoadMopaqFileSigningData(mounts->Locale().c_str());
    EventInitialize(1, 0); OsTimeStartup(OsTimeManager::BestAvailable); OsIMEInitialize();
    [self.controller.view layoutIfNeeded];
    MTKView* view = self.controller.metalView;
    OsGuiSetGxWindow((__bridge void*)view); ConsoleDeviceInitialize("World of Warcraft", false, false);
    device = static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    Require(device && device->DeviceWindow() == (__bridge void*)view, "Cannot create the native iOS client display");
    view.device = device->GPU().device;
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm; view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    view.framebufferOnly = NO; view.clearColor = MTLClearColorMake(0, 0, 0, 1);
    device->SetHostFrameProvider(AcquireSynchronousFrame, (__bridge void*)view);
    g_clientEventContext = EventCreateContextEx(1, Initialize, Destroy, 1, 0);
    Require(g_clientEventContext != nullptr, "Cannot create the original iOS client context");
    self.frameLimit = [NSProcessInfo.processInfo.environment[@"WOW_CLIENT_FRAMES"] integerValue];
    view.delegate = self; self.status.hidden = YES; self.started = YES;
    if (stdinLogin) {
        // Keep UIKit's lifecycle responsive while a developer enters credentials.
        // They are held only in memory and cleared after original Glue login.
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            try {
                auto input = std::make_shared<NativeLoginInput>();
#if TARGET_OS_SIMULATOR
                if (!loginPipe.empty()) {
                    // simctl does not forward stdin. Accept only a private FIFO
                    // inside this app's container, never a regular credential file.
                    const auto root = std::filesystem::canonical(NSHomeDirectory().fileSystemRepresentation);
                    const auto path = std::filesystem::weakly_canonical(loginPipe);
                    const auto relative = path.lexically_relative(root);
                    Require(!relative.empty() && *relative.begin() != "..", "Simulator login pipe must be inside this app's container");
                    struct stat info{};
                    Require(!lstat(loginPipe.c_str(), &info) && S_ISFIFO(info.st_mode) && !(info.st_mode & 077) && info.st_uid == geteuid(),
                        "Simulator login requires an owner-only FIFO");
                    struct Descriptor { int fd; ~Descriptor() { if (fd >= 0) close(fd); } } pipe{open(loginPipe.c_str(), O_RDONLY | O_NOFOLLOW)};
                    Require(pipe.fd >= 0 && !fstat(pipe.fd, &info) && S_ISFIFO(info.st_mode) && !(info.st_mode & 077) && info.st_uid == geteuid(),
                        "Simulator login pipe changed or could not open");
                    input->Read(pipe.fd);
                    unlink(loginPipe.c_str()); // The pipe contained kernel-buffered bytes only.
                } else
#endif
                input->Read();
                dispatch_async(dispatch_get_main_queue(), ^{ if (!self.finished) pendingLogin = input; });
            } catch (const std::exception& error) { Fatal(error.what()); }
        });
    }
}
- (void)suspend {
    if (!self.active) return;
    self.active = NO; self.controller.metalView.paused = YES;
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0);
    if (initialized) { const int32_t focus = 0; PropSelectContext(context); SI2::OnFocusChanged(&focus, nullptr); }
    Require(SE2::SetNativeOutputActive(false) == FMOD_OK, "Cannot suspend native audio output");
    device->Drain();
    std::fprintf(stderr, "Original iOS client suspended at frame %lu\n", (unsigned long)self.frames);
}
- (void)shutdown {
    if (!self.started || self.finished) return;
    [self suspend]; self.finished = YES; pendingLogin.reset();
    auto view = self.controller.metalView; view.delegate = nil;
    OsIMEDestroy(); OsTimeShutdown(); EventDestroy();
    device->SetHostFrameProvider(nullptr, nullptr); ConsoleDeviceDestroy(); device = nullptr;
    Require(CVar::SaveAll(), "Cannot save native Config.wtf");
    CVar::Destroy(); ConsoleCommandDestroy(); CVar::s_configFile = nullptr;
    mounts.reset(); BaseDestroyGlobal(); GxLogClose(); StormDestroy();
    [view releaseDrawables]; view.device = nil; view.hidden = YES;
    self.status.hidden = NO; self.status.text = @"原客户端已正常停止。重新打开应用可再次启动。";
    std::fprintf(stderr, "Original iOS synchronous loading frames: %u; completed %lu frames\n", synchronousFrames, (unsigned long)self.frames);
}
- (void)sceneDidBecomeActive:(UIScene*)scene {
    if (self.finished || self.active) return;
    try {
        if (!self.started) [self start];
        if (!self.started) return;
        self.active = YES; EventResetElapsedTime(); OsQueuePut(OS_INPUT_FOCUS, 1, 0, 0, 0);
        Require(SE2::SetNativeOutputActive(true) == FMOD_OK, "Cannot resume native audio output");
        self.resumeFrame = self.frames;
        self.controller.metalView.paused = NO;
        std::fputs("Original iOS client resumed\n", stderr);
    } catch (const std::exception& error) { Fatal(error.what()); }
}
- (void)sceneWillResignActive:(UIScene*)scene { try { [self suspend]; } catch (const std::exception& e) { Fatal(e.what()); } }
- (void)sceneDidEnterBackground:(UIScene*)scene {
    try { if (self.started && !self.finished) Require(CVar::SaveAll(), "Cannot save background Config.wtf"); }
    catch (const std::exception& e) { Fatal(e.what()); }
}
- (void)sceneDidDisconnect:(UIScene*)scene { try { [self shutdown]; } catch (const std::exception& e) { Fatal(e.what()); } }
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    OsInputSetDrawableSize(uint32_t(size.width), uint32_t(size.height));
    OsQueuePut(OS_INPUT_SIZE, int32_t(size.width), int32_t(size.height), 0, 0);
}
- (void)drawInMTKView:(MTKView*)view {
    if (!self.active) return;
    auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
    if (!pass || !drawable) return;
    try {
        const auto previousLoadingFrames = synchronousFrames;
        device->BeginFrame(pass, drawable);
        processingEvents = true;
        const bool running = EventProcessPending();
        processingEvents = false; PropSelectContext(context);
        if (device->FrameCommand()) device->ScenePresent();
        if (initialized) memoryPressure.Frame(*device, CGGameUI::s_inWorld);
        if (pendingLogin && self.frames >= 2) {
            Require(CGlueMgr::m_acceptedTOS && CGlueMgr::m_acceptedEULA, "Original agreement preferences are required before stdin login");
            Require(!std::strcmp(CGlueMgr::m_currentScreen, "login") && CGlueMgr::m_idleState == IDLE_NONE, "Stdin login requires the idle original login screen");
            CGlueMgr::LoginServerLogin(pendingLogin->account.c_str(), pendingLogin->password.data()); pendingLogin.reset();
            Require(CGlueMgr::m_idleState == IDLE_LOGIN_SERVER_LOGIN, "Original Glue did not start login");
        }
        if (!enterCharacter.empty() && !entryQueued && !std::strcmp(CGlueMgr::m_currentScreen, "charselect") && CGlueMgr::m_idleState == IDLE_NONE && ++self.characterFrames == 60) {
            entryQueued = true; EventSetTimer(0, EnterSelectedCharacter, nullptr);
        }
        if (++self.frames == self.resumeFrame + 120) std::fprintf(stderr, "Original iOS client presented 120 frames since activation; in world: %u; Glue screen: %s; pixels: %lux%lu\n",
            CGGameUI::s_inWorld, CGlueMgr::m_currentScreen, (unsigned long)drawable.texture.width, (unsigned long)drawable.texture.height);
        if (self.frames == (self.frameLimit ? self.frameLimit : 120)) {
            if (self.frameLimit && (entryQueued || !enterCharacter.empty())) Require(CGGameUI::s_inWorld, "Frame limit reached before original world entry");
            if (NSString* file = NSProcessInfo.processInfo.environment[@"WOW_CLIENT_CAPTURE"]) Capture(pass, drawable, file);
        }
        if (!running || (self.frameLimit && self.frames >= self.frameLimit)) [self shutdown];
        else if (synchronousFrames != previousLoadingFrames) [view releaseDrawables];
    } catch (const std::exception& error) { Fatal(error.what()); }
}
@end

@interface NativeClientApplication : UIResponder <UIApplicationDelegate>
@end
@implementation NativeClientApplication
- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application {
    memoryPressure.Request();
    // Also service a suspended application, whose display callback is stopped.
    // Nested notifications during original loading/dispatch remain deferred.
    if (initialized && !processingEvents) {
        try { PropSelectContext(context); memoryPressure.Process(*device); }
        catch (const std::exception& error) { Fatal(error.what()); }
    }
}
@end

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--local-unchecked")) localDevelopment = true;
        else if (!std::strcmp(argv[i], "--login-stdin")) stdinLogin = true;
#if TARGET_OS_SIMULATOR
        else if (!std::strcmp(argv[i], "--login-pipe") && i + 1 < argc) { loginPipe = argv[++i]; stdinLogin = true; }
#endif
        else if (!std::strcmp(argv[i], "--enter-world") && i + 1 < argc) enterCharacter = argv[++i];
        else { std::fputs("Unknown native iOS client option\n", stderr); return 2; }
    }
    if ((stdinLogin || !enterCharacter.empty()) && !localDevelopment) {
        std::fputs("Development login/world entry requires --local-unchecked\n", stderr); return 2;
    }
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(NativeClientApplication.class)); }
}
