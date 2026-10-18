#include "NativeView.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/Storm/Source/MD5.h"
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
}
@interface OriginalUIPreview : NSObject <MTKViewDelegate, NSWindowDelegate, NSApplicationDelegate>
@property(nonatomic, weak) WowNativeView* view;
@property(nonatomic) BOOL active;
@property(nonatomic) BOOL stopping;
@property(nonatomic) NSUInteger frames;
@property(nonatomic) NSUInteger frameLimit;
@property(nonatomic, copy) NSString* error;
@end
@implementation OriginalUIPreview
- (void)stop:(id)sender {
    self.stopping = YES; [self suspend]; [NSApp stop:nil];
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint modifierFlags:0
        timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
}
- (void)suspend {
    if (!self.active) return;
    self.active = NO; self.view.paused = YES; self.view.gameInputActive = NO;
    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0); renderDevice->Drain();
}
- (void)resume {
    if (self.active || self.stopping) return;
    self.active = YES; self.view.gameInputActive = YES; EventResetElapsedTime();
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
    auto pass = view.currentRenderPassDescriptor; auto drawable = view.currentDrawable;
    if (!pass || !drawable) return;
    try {
        renderDevice->BeginFrame(pass, drawable);
        if (!EventProcessPending()) { [self stop:nil]; return; }
        PropSelectContext(context);
        if (renderDevice->FrameCommand()) renderDevice->ScenePresent();
        [self.view.inputContext invalidateCharacterCoordinates];
        if (++self.frames == 120) NSLog(@"Original Scrn/Glue UI: 120 visible frames presented");
        if (self.frameLimit && self.frames >= self.frameLimit) [self stop:nil];
    } catch (const std::exception& error) {
        self.error = [NSString stringWithUTF8String:error.what()];
        if (renderDevice->FrameCommand()) renderDevice->ScenePresent();
        [self stop:nil];
    }
}
@end

int main(int argc, char** argv) {
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
            C2iVector size{600, 300};
            CGxFormat format(true, size, CGxFormat::Fmt_Argb8888, CGxFormat::Fmt_Ds320, 0, 1, true, false, false, false, 0);
            std::unique_ptr<CGxDevice> owner(CreateGxDevice(GxDefaultApi(), static_cast<GxWindowProc>(nullptr), format));
            Require(owner && owner->DeviceCreate((__bridge void*)view, format), "Cannot create UI device");
            auto& device = *static_cast<CGxDeviceMetal*>(owner.get());
            CVar::Initialize("/private/tmp/wow-ui-probe.wtf"); PropInitialize(); EventInitialize(1, 0);
            EventCreateContextEx(1, Initialize, Destroy, 1, 0); Step(); OsInputSetDrawableSize(600, 300);
            auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:600 height:300 mipmapped:NO];
            descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
            renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
            renderPass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
            renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderDevice = &device;
            FrameScript_Initialize(0); ScrnInitialize(0);
            CSimpleFrame::CreateScriptMetaTable(); CSimpleTexture::CreateScriptMetaTable();
            CSimpleFont::CreateScriptMetaTable(); CSimpleFontString::CreateScriptMetaTable(); CSimpleEditBox::CreateScriptMetaTable();
            CSimpleButton::CreateScriptMetaTable();
            FrameXML_RegisterFactory("Button", Create_SimpleButton, false);
            FrameXML_RegisterFactory("EditBox", Create_SimpleEditBox, false);
            FrameXML_RegisterFactory("Frame", Create_SimpleFrame, false);
            {
                auto top = std::make_unique<CSimpleTop>();
                CStatus status; MD5_CTX md5; MD5Init(&md5);
                for (const char* file : {"GlueFonts.xml", "GlueFontStyles.xml", "GlueButtons.xml", "GlueTemplates.xml"}) {
                    const auto path = std::string("Interface\\GlueXML\\") + file;
                    Require(FrameXML_ProcessFile(path.c_str(), nullptr, &md5, &status), "Cannot load original Glue XML");
                }
                const char* xml = R"(<Ui><Button name="AppleOriginalButton" inherits="GluePanelButtonTemplate" text="Metal UI">
                    <Size><AbsDimension x="180" y="40"/></Size><Anchors><Anchor point="CENTER"><Offset><AbsDimension x="0" y="60"/></Offset></Anchor></Anchors>
                    <Scripts><OnClick>self.clicks = (self.clicks or 0) + 1; self:SetText("Clicked")</OnClick></Scripts>
                    </Button></Ui>)";
                auto* tree = XMLTree_Load(xml, uint32_t(std::strlen(xml)));
                Require(tree, "Cannot parse UI instance XML");
                auto* button = FrameXML_CreateFrame(XMLTree_GetRoot(tree)->m_child, nullptr, &status);
                XMLTree_Free(tree);
                if (!status.IsEmpty()) { char errors[4096]; throw std::runtime_error(status.GetErrorStr(errors, sizeof(errors), STATUS_INFO, "\n")); }
                Require(button, "Cannot instantiate original button template");
                auto* memory = CSimpleEditBox::s_allocator.GetData(1, __FILE__, __LINE__);
                auto* edit = new (memory) CSimpleEditBox(nullptr);
                edit->SetWidth(0.65f); edit->SetHeight(0.08f); edit->SetPoint(FRAMEPOINT_CENTER, top.get(), FRAMEPOINT_CENTER, 0, 0, 1);
                auto* localizedFont = CSimpleFont::GetFont("EditBoxFont_Large", 0);
                Require(localizedFont, "Original Glue edit-box font is missing");
                edit->m_font->m_attributes.SetFont(localizedFont->m_attributes.m_font.m_str, 0.04f, 0); edit->m_font->UpdateObjects();
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
                if (cycle == 1 && argc == 3) {
                    __attribute__((objc_precise_lifetime)) OriginalUIPreview* delegate = [OriginalUIPreview new];
                    delegate.view = view;
                    delegate.frameLimit = [NSProcessInfo.processInfo.environment[@"WOW_UI_FRAMES"] integerValue];
                    NSApp.delegate = delegate; window.delegate = delegate; view.delegate = delegate;
                    view.device = device.GPU().device; view.framebufferOnly = NO; view.autoResizeDrawable = YES;
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
            CSimpleEditBox::DestroyScriptMetaTable(); CSimpleFontString::DestroyScriptMetaTable();
            CSimpleButton::DestroyScriptMetaTable(); FrameXML_FreeHashNodes(); FrameXML_ClearFactories();
            CSimpleFont::DestroyScriptMetaTable(); CSimpleTexture::DestroyScriptMetaTable(); CSimpleFrame::DestroyScriptMetaTable();
            ScrnDestroy(); FrameScript_Destroy(); XMLTree_Cleanup();
            renderDevice = nullptr; renderPass = nil;
            PropSelectContext(nullptr); EventDestroy(); PropDestroy(); CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager();
            std::puts("Original CSimpleTop/edit box: native Unicode and composition, layout, rendering and teardown passed");
            }
            return 0;
        } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
    }
}
