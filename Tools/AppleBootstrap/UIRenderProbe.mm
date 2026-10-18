#include "UIComponents.h"
#include "WoW/Source/Console/ConsoleClient.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Frame/CSimpleColorSelect.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Os/Apple/Input.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <vector>
void CheckMovieFrame(CGxDeviceMetal& device);

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
HPROPCONTEXT uiContext;
int32_t Initialize(const void*, void*) {
    uiContext = PropGetSelectedContext(); AsyncFileReadInitialize(0, 20); TextureInitialize(); return 1;
}
int32_t Destroy(const void*, void*) { TextureDestroy(); AsyncFileReadDestroy(); return 1; }
}

void CheckOriginalUI(CGxDeviceMetal& device) {
    const auto pattern = (std::filesystem::temp_directory_path() / "wow-ui-XXXXXX").string();
    std::vector<char> path(pattern.begin(), pattern.end()); path.push_back(0);
    Require(mkdtemp(path.data()), "Cannot create UI check configuration directory");
    struct Directory {
        const char* path;
        ~Directory() { std::error_code error; std::filesystem::remove_all(path, error); }
    } directory{path.data()};
    std::vector<CImVector> first;
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        CVar::Initialize((std::string(path.data()) + "/Config.wtf").c_str());
        PropInitialize(); EventInitialize(1, 0);
        struct Context {
            CGxDeviceMetal& device;
            ~Context() {
                if (device.FrameCommand()) device.ScenePresent();
                device.Drain(); PropSelectContext(nullptr); EventDestroy(); PropDestroy();
                CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager(); uiContext = nullptr;
            }
        } context{device};
        EventCreateContextEx(1, Initialize, Destroy, 1, 0); EventProcessPending(); PropSelectContext(uiContext);
        OsInputSetDrawableSize(600, 300); OsInputSetLanguage(3);
        int32_t pointerX = -1, pointerY = -1; OsInputGetMousePosition(&pointerX, &pointerY);
        Require(pointerX == 0 && pointerY == 0, "Headless native pointer did not start at the drawable origin");
        auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:600 height:300 mipmapped:NO];
        descriptor.storageMode = MTLStorageModePrivate; descriptor.usage = MTLTextureUsageRenderTarget;
        auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        device.BeginFrame(pass); device.ScenePresent();
        UIComponents components;
        auto* edit = components.Edit();
        auto step = [&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            device.BeginFrame(pass); Require(EventProcessPending(), "UI Event context stopped early"); PropSelectContext(uiContext);
            if (device.FrameCommand()) device.ScenePresent();
        };
        CSimpleEditBox::SetKeyboardFocus(edit); step();
        CRect before; Require(edit->m_cursorTexture->GetRect(&before), "Original UI caret has no rectangle");
        const uint16_t text[] = {0x4f60, 0x597d}; OsInputQueueText(text, 2);
        OsInputQueueComposition(1, {}); OsInputQueueComposition(2, {"ni", 0, 2});
        OsInputQueueComposition(2, {u8"你", 0, 1});
        OsInputQueueComposition(2, {u8"你", 0, 0}, true); OsInputQueueComposition(6, {}); step();
        Require(std::string(edit->m_text) == u8"Apple 你好你", "Original UI lost native text or composition replacement");
        CRect after; Require(edit->m_cursorTexture->GetRect(&after) && after.minX > before.minX,
            "Original UI caret did not follow committed text");
        for (unsigned frame = 0; frame < 120; ++frame) step();
        CRect button; Require(components.Button()->GetRect(&button), "Original Glue button has no rectangle");
        float x, y; DDCToNDC((button.minX + button.maxX) * .5f, (button.minY + button.maxY) * .5f, &x, &y);
        const int32_t px = int32_t(x * 600), py = int32_t((1 - y) * 300);
        OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, px, py, 0);
        OsQueuePut(OS_INPUT_MOUSE_DOWN, MOUSE_BUTTON_LEFT, px, py, 0);
        OsQueuePut(OS_INPUT_MOUSE_UP, MOUSE_BUTTON_LEFT, px, py, 0); step();
        OsInputGetMousePosition(&pointerX, &pointerY);
        Require(pointerX == px && pointerY == py, "Native pointer query lost the latest absolute event");
        auto* lua = FrameScript_GetContext();
        Require(luaL_loadstring(lua, "assert(AppleOriginalButton.clicks == 1)") == 0 && lua_pcall(lua, 0, 0, 0) == 0,
            "Original UI hit testing did not invoke its Lua callback");
        Require(!CSimpleEditBox::s_currentFocus, "Original Lua button did not release edit-box focus");
        OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, 0, 0, 0); step();
        auto read = [&] {
            MTLRenderPassDescriptor* preserved = [pass copy]; preserved.colorAttachments[0].loadAction = MTLLoadActionLoad;
            device.BeginFrame(preserved); CiRect rect(0, 0, 300, 600); TSGrowableArray<CImVector> pixels;
            GxReadPixels(rect, pixels); device.ScenePresent();
            return std::vector<CImVector>(pixels.Ptr(), pixels.Ptr() + pixels.Count());
        };
        const auto neutralColor = read();
        Require(luaL_loadstring(lua, "AppleOriginalColorSelect:SetColorHSV(120, 1, 1); local r,g,b = AppleOriginalColorSelect:GetColorRGB(); assert(r == 0 and g == 1 and b == 0)") == 0 &&
                    lua_pcall(lua, 0, 0, 0) == 0, "Original color-selector Lua HSV/RGB methods failed");
        step();
        const auto greenColor = read();
        size_t colorChanges = 0;
        for (size_t i = 0; i < greenColor.size(); ++i)
            colorChanges += std::memcmp(&neutralColor[i], &greenColor[i], sizeof(CImVector)) != 0;
        Require(colorChanges > 10, "Original color-selector value texture update did not reach the GPU");
        auto clickRegion = [&](CSimpleTexture* texture, float u, float v) {
            CRect bounds;
            Require(texture && texture->GetRect(&bounds), "Original color-selector texture has no rectangle");
            float x, y;
            DDCToNDC(bounds.minX + u * (bounds.maxX - bounds.minX), bounds.minY + v * (bounds.maxY - bounds.minY), &x, &y);
            const int px = int(x * 600), py = int((1 - y) * 300);
            OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, px, py, 0);
            OsQueuePut(OS_INPUT_MOUSE_DOWN, MOUSE_BUTTON_LEFT, px, py, 0);
            OsQueuePut(OS_INPUT_MOUSE_UP, MOUSE_BUTTON_LEFT, px, py, 0); step();
        };
        clickRegion(components.ColorSelect()->m_wheelTexture, .1f, .5f);
        clickRegion(components.ColorSelect()->m_valueTexture, .5f, .5f);
        Require(luaL_loadstring(lua, "local c = AppleOriginalColorSelect; assert(c.changes >= 3 and c.red > .3 and c.red < .7 and c.red > c.green*2 and c.red > c.blue*2)") == 0 &&
                    lua_pcall(lua, 0, 0, 0) == 0, "Original color wheel/value mouse tracking or Lua callback failed");
        Require(!components.ColorSelect()->m_wheelTracking && !components.ColorSelect()->m_valueTracking,
                "Original color selector retained mouse tracking after release");
        OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, 0, 0, 0); step();
        const auto pixels = read(); size_t ink = 0, red = 0;
        for (const auto& pixel : pixels) { ink += pixel.r || pixel.g || pixel.b; red += pixel.r > pixel.g + 10; }
        Require(ink > 100 && red > 100, "Original Glue text or button texture did not reach the GPU");
        if (cycle) Require(pixels.size() == first.size() && !std::memcmp(pixels.data(), first.data(), pixels.size() * sizeof(CImVector)),
            "Original UI recreation changed its GPU output");
        else first = pixels;
        // A host drawable change must rebuild layout/glyph dimensions and then
        // reproduce the same output when the original dimensions are restored.
        for (unsigned width : {900u, 600u}) {
            descriptor.width = width; descriptor.height = width == 900 ? 600 : 300;
            pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
            OsInputSetDrawableSize(width, descriptor.height); OsQueuePut(OS_INPUT_SIZE, width, descriptor.height, 0, 0); step(); step();
        }
        const auto restored = read();
        Require(restored.size() == pixels.size() && !std::memcmp(restored.data(), pixels.data(), pixels.size() * sizeof(CImVector)),
            "Original UI drawable resize/restore changed its output");
        std::fprintf(stderr, "Original UI cycle %u: native text/composition, Lua hit test, color wheel/value input (%zu changed pixels), %zu visible / %zu red pixels; resize/restore exact\n", cycle, colorChanges, ink, red);
        auto key = [&](KEY key) {
            OsQueuePut(OS_INPUT_KEY_DOWN, key, 1, 0, 0);
            OsQueuePut(OS_INPUT_KEY_UP, key, 1, 0, 0); step();
        };
        auto type = [&](const char* text) {
            std::vector<uint16_t> chars;
            for (; *text; ++text) chars.push_back(uint8_t(*text));
            OsInputQueueText(chars.data(), chars.size()); step();
        };
        key(KEY_TILDE); Require(!ConsoleGetActive(), "Disabled console hotkey opened the overlay");
        ConsoleDeviceSetConsoleAccess(true); key(KEY_TILDE);
        Require(ConsoleGetActive(), "Console hotkey did not open the original overlay");
        ConsoleCommandExecute("bgcolor 192 64 32 16", 0);
        for (int i = 0; i < 30; ++i) step();
        const auto consolePixels = read(); size_t consoleChanges = 0;
        for (size_t i = 0; i < consolePixels.size(); ++i)
            consoleChanges += std::memcmp(&consolePixels[i], &restored[i], sizeof(CImVector)) != 0;
        Require(consoleChanges > 1000, "Original console background/text did not reach Metal");
        type("consolelines 12"); key(KEY_ENTER); step();
        Require(s_consoleLines == 12 && ConsoleCommandHistory(0) && !std::strcmp(ConsoleCommandHistory(0), "consolelines 12"),
            "Console command input did not reach the original dispatcher/history");
        key(KEY_UP);
        Require(!std::strcmp(reinterpret_cast<const char*>(s_linelist.Head()->m_buffer), "> consolelines 12"), "Console history navigation changed input");
        key(KEY_ESCAPE); type("consol"); key(KEY_TAB);
        Require(!std::strcmp(reinterpret_cast<const char*>(s_linelist.Head()->m_buffer), "> consolelines"), "Console completion lost the original command");
        key(KEY_ESCAPE); key(KEY_DELETE); key(KEY_DELETE);
        Require(s_linelist.Head()->m_chars == 2 && s_linelist.Head()->m_inputpos == 2,
            "Delete at end-of-line changed the prompt length");
        const std::string longInput(300, 'z'); type(longInput.c_str()); key(KEY_TAB);
        Require(s_linelist.Head()->m_chars == 302, "Long completion changed the input buffer");
        key(KEY_ESCAPE);
        const uint32_t oldColor = s_colorArray[DEFAULT_COLOR];
        ConsoleCommandExecute(("fontcolor " + longInput + " 1 2 3").c_str(), 0);
        Require(s_colorArray[DEFAULT_COLOR] == oldColor, "Invalid color class changed the console palette");
        const uint16_t unicode[] = {0x4F60, 0x597D, 0xD83D, 0xDE00};
        OsInputQueueText(unicode, 4); step();
        Require(!std::strcmp(reinterpret_cast<const char*>(s_linelist.Head()->m_buffer), u8"> 你好😀"), "Console lost native Unicode scalars");
        key(KEY_LEFT); key(KEY_BACKSPACE); type("X");
        Require(!std::strcmp(reinterpret_cast<const char*>(s_linelist.Head()->m_buffer), u8"> 你X😀"), "Console cursor/backspace split a UTF-8 character");
        key(KEY_DELETE); key(KEY_LEFT); key(KEY_LEFT); key(KEY_RIGHT); key(KEY_DELETE);
        Require(!std::strcmp(reinterpret_cast<const char*>(s_linelist.Head()->m_buffer), u8"> 你"), "Console Unicode forward delete changed adjacent bytes");
        key(KEY_ESCAPE);
        type("settings"); key(KEY_ENTER); step();
        std::atomic<bool> logged{false};
        std::thread logger([&] {
            for (int i = 0; i < 270; ++i) ConsoleWriteA("worker console line %d", WARNING_COLOR, i);
            logged = true;
        });
        while (!logged) step();
        logger.join(); step();
        Require(s_NumLines == 256, "Console retained-line limit changed");
        bool newest = false;
        for (auto* line = s_linelist.Head(); line; line = s_linelist.Next(line))
            newest |= line->m_colorType == WARNING_COLOR && !std::strcmp(reinterpret_cast<const char*>(line->m_buffer), "worker console line 269");
        Require(newest, "Worker console output was not delivered on the game thread");
        for (unsigned width : {900u, 600u}) {
            descriptor.width = width; descriptor.height = width == 900 ? 600 : 300;
            pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
            OsInputSetDrawableSize(width, descriptor.height); OsQueuePut(OS_INPUT_SIZE, width, descriptor.height, 0, 0); step(); step();
            Require(ConsoleGetActive() && s_NumLines == 256, "Drawable resize lost the console state or scrollback");
        }
        type("clear"); key(KEY_ENTER); step();
        Require(s_NumLines == 1 && s_linelist.Head()->m_inputpos == 2, "Console clear did not rebuild the input line");
        type("closeconsole"); key(KEY_ENTER);
        Require(!ConsoleGetActive(), "Original closeconsole command failed");
        for (int i = 0; i < 120; ++i) step();
        const auto closed = read();
        Require(closed.size() == restored.size() && !std::memcmp(closed.data(), restored.data(), closed.size() * sizeof(CImVector)),
            "Closing console did not restore the underlying UI output");
        ConsoleCommandExecute("consolelines 10", 0);
        ConsoleCommandExecute("bgcolor 192 0 0 0", 0);
        ConsoleDeviceSetConsoleAccess(false);
        std::fprintf(stderr, "Original console cycle %u: access/hotkey, commands/history/completion, Unicode cursor/deletion, worker logs/256-line retention/resize, clear/close, %zu changed pixels and exact underlying UI restore\n", cycle, consoleChanges);
        CheckMovieFrame(device);
        device.Drain();
    }
}
