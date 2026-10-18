#include "Engine/Source/Os/Gui.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "Tools/AppleBootstrap/MetalView.h"
#include "Tools/AppleBootstrap/NativeView.h"
#include "Tools/AppleBootstrap/ModelPreview.h"
#include "Tools/AppleBootstrap/TerrainPreview.h"
#include "Tools/AppleBootstrap/WmoPreview.h"
#include "Tools/AppleBootstrap/WorldPreview.h"
#include "Tools/AppleBootstrap/GameAudioPreview.h"
#include "Tools/AppleBootstrap/UIComponents.h"
#include "Tools/AppleBootstrap/MoviePlayback.h"
#include "Tools/AppleBootstrap/NativeDisplayProbe.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Metal/NativeHost.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Base.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Apple/Input.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "WoW/Source/WowServices/Apple/ClientSettings.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
class GameStorm {
    static bool s_owned;
public:
    GameStorm() {
        if (s_owned) throw std::logic_error("A native scene host already owns Storm");
        StormInitialize(); s_owned = true;
    }
    ~GameStorm() { GxLogClose(); StormDestroy(); s_owned = false; }
};
bool GameStorm::s_owned = false;

class GameBase {
public:
    GameBase() { BaseInitializeGlobal(); }
    ~GameBase() { BaseDestroyGlobal(); }
};

class GameConfig {
public:
    GameConfig() {
        if (CVar::m_initialized) throw std::logic_error("A game configuration already owns the native host");
        std::string file;
        if (!NativeSettingsFile(file, "Config.wtf", true)) throw std::runtime_error("Cannot locate writable application settings");
        CVar::Initialize(file.c_str());
        // Initialize has no load status. Before registering callbacks, repeat
        // the SET-only load with its checked API so failures remain visible.
        if ([NSFileManager.defaultManager fileExistsAtPath:[NSString stringWithUTF8String:file.c_str()]] && !CVar::Load(file.c_str())) {
            CVar::m_needsSave = false; // Do not overwrite an unreadable configuration with a partial load.
            Shutdown();
            throw std::runtime_error("Cannot load native Config.wtf");
        }
    }
    ~GameConfig() { Shutdown(); }
    void Save() {
        if (!CVar::SaveAll()) NSLog(@"Cannot save native Config.wtf: %s", CVar::s_configFile);
    }
private:
    static void Shutdown() {
        CVar::Destroy();
        ConsoleCommandDestroy();
        CVar::s_configFile = nullptr;
    }
};

struct GameDisplayDelete {
    void operator()(CGxDeviceMetal*) const { ConsoleDeviceDestroy(); }
};

class GameEvents {
    static GameEvents* s_current;
    void (*m_paint)(void*, float);
    float m_elapsedSeconds = 0;
    void* m_owner;
    HPROPCONTEXT m_context = nullptr;
    std::string m_error;
    static int32_t Paint(const void*, void* arg) {
        auto& self = *static_cast<GameEvents*>(arg);
        const float elapsed = self.m_elapsedSeconds; self.m_elapsedSeconds = 0;
        self.m_paint(self.m_owner, elapsed);
        return 1;
    }
    static int32_t Update(const void* data, void* arg) {
        static_cast<GameEvents*>(arg)->m_elapsedSeconds += static_cast<const EVENT_DATA_UPDATE*>(data)->m_elapsedSec;
        return 1;
    }
    static int32_t Destroy(const void*, void*) { TextureDestroy(); AsyncFileReadDestroy(); BaseDestroyContext(); return 1; }
    static int32_t Initialize(const void*, void*) {
        s_current->m_context = PropGetSelectedContext();
        try { BaseInitializeContext(); AsyncFileReadInitialize(0, 20); TextureInitialize(); }
        catch (const std::exception& error) { s_current->m_error = error.what(); }
        EventRegisterEx(EVENT_ID_UPDATE, Update, s_current, 0);
        EventRegisterEx(EVENT_ID_PAINT, Paint, s_current, 0);
        return 1;
    }
public:
    void Select() { PropSelectContext(m_context); }
    void Check() { if (!m_error.empty()) { throw std::runtime_error(m_error); } }
    GameEvents(void (*paint)(void*, float), void* owner) : m_paint(paint), m_owner(owner) {
        if (s_current) { throw std::logic_error("A native game event host already exists"); }
        s_current = this;
        try {
            EventInitialize(1, 0);
            if (!EventCreateContextEx(1, Initialize, Destroy, 1, 0)) { throw std::runtime_error("Cannot create native game Event context"); }
        } catch (...) {
            EventDestroy(); s_current = nullptr; throw;
        }
    }
    ~GameEvents() {
        EventDestroy();
        s_current = nullptr;
    }
};
GameEvents* GameEvents::s_current;

// Own the original Services handle: filename/flags cache, asynchronous load,
// mip recreation and release all follow the game's texture implementation.
class GameTexture {
    CTexture* m_texture = nullptr;
    std::string m_name;
public:
    explicit GameTexture(const char* name) : m_name(name) {}
    ~GameTexture() { if (m_texture) { HandleClose(reinterpret_cast<HOBJECT>(m_texture)); } }
    CGxTex* Get() {
        CStatus status;
        if (!m_texture) { m_texture = TextureCreate(m_name.c_str(), CGxTexFlags(GxTex_LinearMipLinear), &status, 1); }
        if (!m_texture || !status.IsEmpty()) { throw std::runtime_error("Cannot open the Services preview texture: " + m_name); }
        auto* gpu = TextureGetGxTex(m_texture, 0, &status);
        if (!status.IsEmpty() || !m_texture->m_status.IsEmpty()) { throw std::runtime_error("Services preview texture load failed: " + m_name); }
        return gpu;
    }
};

class GameText {
    CGxFont* m_font = nullptr;
    CGxString* m_string = nullptr;
    CGxStringBatch* m_batch = nullptr;
public:
    GameText() { GxuFontInitialize(); }
    ~GameText() {
        if (m_batch) { GxuFontDestroyBatch(m_batch); }
        if (m_string) { GxuFontDestroyString(m_string); }
        if (m_font) { GxuFontDestroyFont(m_font); }
        GxuFontShutdown();
        TextureGxCacheFlush();
    }
    void Draw(uint32_t width, uint32_t height) {
        CoordinateSetAspectRatio(float(width) / height);
        GxuFontWindowSizeChanged();
        if (!m_font && !GxuFontCreateFont("Fonts\\ARIALN.TTF", 0.065f, m_font, 0)) {
            throw std::runtime_error("Cannot open the game font for the native preview");
        }
        if (!m_batch) { m_batch = GxuFontCreateBatch(false, false); }
        if (!m_string) {
            if (!GxuFontCreateString(m_font, "World of Warcraft\n|cffffcc00Wrath of the Lich King|r", 0.065f,
                C3Vector(0.05f, 0.01f, 0), 0.9f, 0.16f, 0, m_string, GxVJ_Top, GxHJ_Center, 0,
                CImVector(255, 255, 255, 255), 0, 1)) {
                throw std::runtime_error("Cannot lay out the native preview's game text");
            }
            GxuFontAddToBatch(m_batch, m_string);
        }
        GxuFontAddShadow(m_string, CImVector(0, 0, 0, 255), C2Vector(1.5f / width, -1.5f / height));
        GxuFontRenderBatch(m_batch);
    }
};


}

@interface WowTextureRenderer ()
- (void)renderGameFrame:(float)elapsedSeconds;
- (void)stopAfterError:(const char*)message;
@end

@implementation WowTextureRenderer {
    // Device destruction uses registered CVars; configuration and Base must
    // outlive it. Event/resources are still released before the device.
    std::unique_ptr<GameStorm> _storm;
    std::unique_ptr<GameBase> _base;
    std::unique_ptr<GameConfig> _config;
    std::unique_ptr<CGxDeviceMetal, GameDisplayDelete> _device;
    // Texture handles and fonts die before Services; Services dies before Gx.
    std::unique_ptr<GameEvents> _events;
    std::unique_ptr<GameAudioPreview> _audio;
    std::unique_ptr<GameText> _text;
    std::unique_ptr<GameTexture> _image;
    std::unique_ptr<ModelPreview> _model;
    std::unique_ptr<TerrainPreview> _terrain;
    std::unique_ptr<WmoPreview> _wmo;
    std::unique_ptr<WorldPreview> _world;
    std::unique_ptr<UIComponents> _movieUI;
    std::unique_ptr<MoviePlayback> _movie;
    std::unique_ptr<NativeDisplayProbe> _displayProbe;
    NSString* _movieSelection;
    CGxBuf* _vertices;
    CGxBuf* _indices;
    CGxShader* _vertexShaders[2];
    CGxShader* _pixelShader;
    __weak MTKView* _view;
    BOOL _active;
    uint32_t _presentedFrames;
    BOOL _displayChecked;
}
- (instancetype)initWithView:(MTKView*)view textureName:(const char*)textureName {
    if ((self = [super init])) {
        if (g_theGxDevicePtr) { throw std::runtime_error("A game Gx device already owns the native host"); }
        _storm = std::make_unique<GameStorm>();
        _base = std::make_unique<GameBase>();
        _config = std::make_unique<GameConfig>();
        OsGuiSetGxWindow((__bridge void*)view);
        ConsoleDeviceInitialize("World of Warcraft", false, false);
        _device.reset(static_cast<CGxDeviceMetal*>(g_theGxDevicePtr));
        if (!_device || _device->DeviceWindow() != (__bridge void*)view) { throw std::runtime_error("Cannot create the native game Gx device"); }
        _image = std::make_unique<GameTexture>(textureName);
        _device->ShaderCreate(_vertexShaders, GxSh_Vertex, "Shaders\\Vertex", "UI", 2);
        _device->ShaderCreate(&_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "UI", 1);
        struct Vertex { float x, y, z; uint32_t color; float u, v; };
        const Vertex vertices[] = {{-1, 1, 0, 0xFFFFFFFF, 0, 0}, {-1, -1, 0, 0xFFFFFFFF, 0, 1},
            {1, 1, 0, 0xFFFFFFFF, 1, 0}, {1, -1, 0, 0xFFFFFFFF, 1, 1}};
        const uint16_t indices[] = {0, 1, 2, 2, 1, 3};
        auto* vp = _device->PoolCreate(GxPoolTarget_Vertex, GxPoolUsage_Static, sizeof(vertices), GxPoolHintBit_Unk0, "preview vertices");
        auto* ip = _device->PoolCreate(GxPoolTarget_Index, GxPoolUsage_Static, sizeof(indices), GxPoolHintBit_Unk0, "preview indices");
        _vertices = _device->BufCreate(vp, sizeof(Vertex), 4, 0);
        _indices = _device->BufCreate(ip, sizeof(uint16_t), 6, 0);
        GxBufData(_vertices, vertices, sizeof(vertices), 0); GxBufData(_indices, indices, sizeof(indices), 0);
        _view = view;
        _movieSelection = NSProcessInfo.processInfo.environment[@"WOW_MOVIE"];
        if (_movieSelection) {
            for (NSString* key in @[@"WOW_WORLD", @"WOW_TERRAIN", @"WOW_WMO", @"WOW_CHARACTER", @"WOW_MODEL", @"WOW_CREATURE_DISPLAY", @"WOW_AUDIO"])
                if (NSProcessInfo.processInfo.environment[key]) throw std::runtime_error("WOW_MOVIE requires its own scene/audio selection");
        }
        view.device = _device->GPU().device;
        view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        view.depthStencilPixelFormat = MTLPixelFormatDepth32Float; view.clearDepth = 1;
        view.framebufferOnly = NO; // Gx screenshot / framebuffer copy source.
        view.clearColor = MTLClearColorMake(0.12, 0.14, 0.18, 1);
        view.paused = YES; view.enableSetNeedsDisplay = NO; view.delegate = self;
        _events = std::make_unique<GameEvents>([](void* owner, float elapsed) {
            [(__bridge WowTextureRenderer*)owner renderGameFrame:elapsed];
        }, (__bridge void*)self);
        OsInputSetDrawableSize(uint32_t(_device->m_format.m_size.x), uint32_t(_device->m_format.m_size.y));
    }
    return self;
}
- (void)dealloc {
    if (_movie || _movieUI) {
        _events->Select(); _movie.reset(); _movieUI.reset();
    }
}
- (void)resume {
    if (_active) return;
#if TARGET_OS_OSX
    if ([_view isKindOfClass:WowNativeView.class]) [(WowNativeView*)_view setGameInputActive:YES];
#endif
    try { if (_audio) _audio->SetActive(true); if (_movie) _movie->SetActive(true); }
    catch (const std::exception& error) { [self stopAfterError:error.what()]; return; }
    _active = YES;
    EventResetElapsedTime();
    OsQueuePut(OS_INPUT_FOCUS, 1, 0, 0, 0);
    _view.paused = NO;
    [_view draw];
}
- (void)suspend {
    if (!_active) return;
#if TARGET_OS_OSX
    if ([_view isKindOfClass:WowNativeView.class]) [(WowNativeView*)_view setGameInputActive:NO];
#endif
    _active = NO; _view.paused = YES;
    try { if (_audio) _audio->SetActive(false); if (_movie) _movie->SetActive(false); }
    catch (const std::exception& error) { NSLog(@"Game audio suspension: %s", error.what()); }
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0);
    if (!_movieUI) EventProcessPending();
    _device->Drain();
    if (_config) _config->Save();
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    OsInputSetDrawableSize(uint32_t(size.width), uint32_t(size.height));
    OsQueuePut(OS_INPUT_SIZE, int32_t(size.width), int32_t(size.height), 0, 0);
    // The active display loop draws the new size. Do not recursively render
    // while the host is preparing this frame's drawable and input bounds.
}
- (void)drawInMTKView:(MTKView*)view {
    if (!_active) { return; }
    try {
        if (!GxMetal::PrepareHostDrawable((__bridge void*)view)) return;
        if (!_displayChecked && NSProcessInfo.processInfo.environment[@"WOW_DISPLAY_CHECK"]) {
            if (!_displayProbe) _displayProbe = std::make_unique<NativeDisplayProbe>(*_device, view);
            if (_displayProbe->Step()) { _displayChecked = YES; _displayProbe.reset(); }
            return;
        }
        if (_movieSelection) {
            if (!_movieUI) {
                // Initialize the original Event/Services context before adding
                // Scrn's PAINT handler. Its later frames own UI presentation.
                if (!EventProcessPending()) { [self suspend]; return; }
                _events->Check(); _events->Select();
                _movieUI = std::make_unique<UIComponents>();
                _movie = std::make_unique<MoviePlayback>(_movieSelection.UTF8String);
            }
            auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
            if (!pass || !drawable) return;
            _device->BeginFrame(pass, drawable);
        }
        if (!EventProcessPending()) { [self suspend]; return; }
        if (_movieSelection) {
            if (_device->FrameCommand()) _device->ScenePresent();
            if (++_presentedFrames == 120 || _presentedFrames == 420) {
                _movie->Report(); NSLog(@"Original MovieFrame/Scrn: %u visible frames presented", _presentedFrames);
            }
        }
    }
    catch (const std::exception& error) { [self stopAfterError:error.what()]; }
}
- (void)stopAfterError:(const char*)message {
    _displayProbe.reset();
#if TARGET_OS_OSX
    if ([_view isKindOfClass:WowNativeView.class]) [(WowNativeView*)_view setGameInputActive:NO];
#endif
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0);
    if (_device->FrameCommand()) _device->ScenePresent();
    _active = NO; _view.paused = YES;
    _audio.reset();
    try { if (_movie) _movie->SetActive(false); }
    catch (const std::exception& error) { NSLog(@"Movie suspension after error: %s", error.what()); }
    NSLog(@"Metal Gx preview stopped: %s", message);
}
- (void)renderGameFrame:(float)elapsedSeconds {
    if (_movieSelection) return; // The original Scrn PAINT handler presents this scene.
    MTKView* view = _view;
    if (!_active || !view.drawableSize.width || !view.drawableSize.height) { return; }
    MTLRenderPassDescriptor* pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!pass || !drawable) { return; }
    try {
        _events->Check();
        NSString* audio = NSProcessInfo.processInfo.environment[@"WOW_AUDIO"];
        if (audio && !_audio) _audio = std::make_unique<GameAudioPreview>(audio.UTF8String);
        if (_audio) _audio->Stats(); // Surface asynchronous stream/output failures.
        auto* texture = _image->Get();
        NSString* terrain = NSProcessInfo.processInfo.environment[@"WOW_TERRAIN"];
        NSString* wmo = NSProcessInfo.processInfo.environment[@"WOW_WMO"];
        NSString* world = NSProcessInfo.processInfo.environment[@"WOW_WORLD"];
        NSString* character = NSProcessInfo.processInfo.environment[@"WOW_CHARACTER"];
        if (world && !_world) {
            if (character || wmo || terrain || NSProcessInfo.processInfo.environment[@"WOW_MODEL"] || NSProcessInfo.processInfo.environment[@"WOW_CREATURE_DISPLAY"])
                throw std::runtime_error("WOW_WORLD cannot be combined with other scene selections");
            _world = std::make_unique<WorldPreview>(world.UTF8String);
#if TARGET_OS_OSX
            _world->EnableInput();
#endif
            NSString* time = NSProcessInfo.processInfo.environment[@"WOW_TIME_MINUTES"];
            if (time) {
                NSScanner* scanner = [NSScanner scannerWithString:time]; double minutes;
                if (![scanner scanDouble:&minutes] || !scanner.isAtEnd) throw std::runtime_error("WOW_TIME_MINUTES must be a number");
                _world->SetTime(float(minutes));
            }
        }
        if (wmo && !_wmo) {
            if (character || terrain || NSProcessInfo.processInfo.environment[@"WOW_MODEL"] || NSProcessInfo.processInfo.environment[@"WOW_CREATURE_DISPLAY"])
                throw std::runtime_error("WOW_WMO cannot be combined with other scene selections");
            _wmo = std::make_unique<WmoPreview>(wmo.UTF8String);
        }
        if (terrain && !_terrain) {
            if (character || NSProcessInfo.processInfo.environment[@"WOW_MODEL"] || NSProcessInfo.processInfo.environment[@"WOW_CREATURE_DISPLAY"])
                throw std::runtime_error("WOW_TERRAIN cannot be combined with other scene selections");
            _terrain = std::make_unique<TerrainPreview>(terrain.UTF8String, true);
            NSString* time = NSProcessInfo.processInfo.environment[@"WOW_TIME_MINUTES"];
            if (time) {
                NSScanner* scanner = [NSScanner scannerWithString:time];
                double minutes;
                if (![scanner scanDouble:&minutes] || !scanner.isAtEnd) throw std::runtime_error("WOW_TIME_MINUTES must be a number");
                _terrain->SetTime(float(minutes));
            }
        }
        if (!world && !terrain && !wmo && !_model) {
            NSString* name = NSProcessInfo.processInfo.environment[@"WOW_MODEL"];
            NSString* display = NSProcessInfo.processInfo.environment[@"WOW_CREATURE_DISPLAY"];
            if (character && (name || display)) throw std::runtime_error("WOW_CHARACTER cannot be combined with WOW_MODEL or WOW_CREATURE_DISPLAY");
            uint32_t displayID = (name || character) ? 0 : 165; // Original timber wolf display.
            if (display) {
                NSScanner* scanner = [NSScanner scannerWithString:display];
                unsigned long long value = 0;
                if (name || ![scanner scanUnsignedLongLong:&value] || !scanner.isAtEnd || !value || value > INT32_MAX)
                    throw std::runtime_error("WOW_CREATURE_DISPLAY must be a positive DBC ID and cannot be combined with WOW_MODEL");
                displayID = uint32_t(value);
            }
            _model = std::make_unique<ModelPreview>(name.UTF8String, displayID, character.UTF8String);
        }
        _device->BeginFrame(pass, drawable);
        GxXformSetViewport(0, 1, 0, 1, 0, 1);
        if (texture && !_world) {
            GxRsSet(GxRs_VertexShader, _vertexShaders[0]); GxRsSet(GxRs_PixelShader, _pixelShader);
            GxRsSet(GxRs_Texture0, texture);
            GxRsSet(GxRs_Fog, 0); GxRsSet(GxRs_Culling, 0); GxRsSet(GxRs_DepthTest, 0); GxRsSet(GxRs_DepthWrite, 0);
            GxRsSet(GxRs_BlendingMode, int32_t(GxBlend_Alpha)); GxRsSet(GxRs_AlphaRef, 1);
            const double imageAspect = double(texture->m_width) / texture->m_height;
            const double viewAspect = (view.drawableSize.width * 0.44) / (view.drawableSize.height * 0.72);
            C44Matrix matrix(1);
            matrix.Scale(C3Vector(0.44 * (imageAspect > viewAspect ? 1 : imageAspect / viewAspect), 0.72 * (imageAspect > viewAspect ? viewAspect / imageAspect : 1), 1));
            matrix.Translate(C3Vector(-0.5f, 0.24f, 0));
            const auto registers = matrix.Transpose(); GxShaderConstantsSet(GxSh_Vertex, 0, &registers.a0, 4);
            GxPrimVertexPtr(_vertices, GxVBF_PCT); _device->PrimIndexPtr(_indices);
            CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 3}; GxDraw(&batch, 1);
        }
        const bool sceneReady = _world ? _world->Draw(elapsedSeconds, uint32_t(view.drawableSize.width), uint32_t(view.drawableSize.height)) : _wmo ? _wmo->Draw(elapsedSeconds, uint32_t(view.drawableSize.width), uint32_t(view.drawableSize.height)) : _terrain ? _terrain->Draw(elapsedSeconds, uint32_t(view.drawableSize.width), uint32_t(view.drawableSize.height)) :
            _model->Draw(elapsedSeconds, uint32_t(view.drawableSize.width), uint32_t(view.drawableSize.height));
        if (!_text) { _text = std::make_unique<GameText>(); }
        _text->Draw(uint32_t(view.drawableSize.width), uint32_t(view.drawableSize.height));
        GxScenePresent();
        if (texture && sceneReady && ++_presentedFrames == 120) {
            NSLog(@"Original Event native frame loop: 120 frames presented; Services TextureCreate and %@ rendered", _world ? @"original world scene" : _wmo ? @"original WMO group" : _terrain ? @"original ADT terrain surface" : @"original M2 scene");
            if (_audio) {
                const auto stats = _audio->Stats();
                NSLog(@"Original SESound archive stream: %llu frames, %llu nonzero samples, playing=%d", stats.frames, stats.nonzeroSamples, _audio->IsPlaying());
            }
        }
    } catch (const std::exception& error) {
        [self stopAfterError:error.what()];
    }
}
@end
