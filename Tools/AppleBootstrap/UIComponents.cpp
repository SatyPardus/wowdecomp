#include "UIComponents.h"
#include "WoW/Source/Console/ConsoleClient.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Frame/CSimpleColorSelect.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Scrn/Scrn.h"
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
UIComponents::UIComponents(bool enableConsole) {
    Require(!CSimpleTop::s_instance && EventGetCurrentContext(), "UI components require their own selected Event context");
    m_previousAspect = CoordinateGetAspectRatio();
    try {
        // CGlueMgr::Resume normally establishes DDC coordinates before Top.
        // Component hosts must do this too, independently of preceding scenes.
        CRect bounds; g_theGxDevicePtr->CapsWindowSize(bounds);
        CoordinateSetAspectRatio((bounds.maxX - bounds.minX) / (bounds.maxY - bounds.minY));
        if (!FrameScript_GetContext()) { FrameScript_Initialize(0); m_ownsScript = true; }
        ScrnInitialize(0); m_screen = true;
        m_previousConsoleAccess = IsConsoleEnabled();
        ConsoleDeviceSetConsoleAccess(enableConsole);
        ConsoleScreenInitialize("WoW"); m_console = true;
        CSimpleFrame::CreateScriptMetaTable(); CSimpleTexture::CreateScriptMetaTable();
        CSimpleFont::CreateScriptMetaTable(); CSimpleFontString::CreateScriptMetaTable();
        CSimpleEditBox::CreateScriptMetaTable(); CSimpleButton::CreateScriptMetaTable();
        CSimpleColorSelect::CreateScriptMetaTable(); m_metatables = true;
        FrameXML_RegisterFactory("Button", Create_SimpleButton, false);
        FrameXML_RegisterFactory("EditBox", Create_SimpleEditBox, false);
        FrameXML_RegisterFactory("Frame", Create_SimpleFrame, false);
        FrameXML_RegisterFactory("ColorSelect", Create_SimpleColorSelect, false);
        m_top = std::make_unique<CSimpleTop>();
        EventRegisterEx(EVENT_ID_SIZE, Resize, this, 2); m_resize = true;
        CStatus status; MD5_CTX md5; MD5Init(&md5);
        for (const char* file : {"GlueFonts.xml", "GlueFontStyles.xml", "GlueButtons.xml", "GlueTemplates.xml"}) {
            const auto path = std::string("Interface\\GlueXML\\") + file;
            Require(FrameXML_ProcessFile(path.c_str(), nullptr, &md5, &status), "Cannot load original Glue XML");
        }
        const char* xml = R"(<Ui><Button name="AppleOriginalButton" inherits="GluePanelButtonTemplate" text="Metal UI">
            <Size><AbsDimension x="180" y="40"/></Size><Anchors><Anchor point="CENTER"><Offset><AbsDimension x="0" y="60"/></Offset></Anchor></Anchors>
            <Scripts><OnClick>self.clicks = (self.clicks or 0) + 1; self:SetText("Clicked"); AppleOriginalEdit:ClearFocus()</OnClick></Scripts>
            </Button></Ui>)";
        auto* tree = XMLTree_Load(xml, uint32_t(std::strlen(xml)));
        Require(tree, "Cannot parse UI instance XML");
        m_button = FrameXML_CreateFrame(XMLTree_GetRoot(tree)->m_child, nullptr, &status);
        XMLTree_Free(tree);
        if (!status.IsEmpty()) {
            char errors[4096]; throw std::runtime_error(status.GetErrorStr(errors, sizeof(errors), STATUS_INFO, "\n"));
        }
        Require(m_button, "Cannot instantiate original button template");
        const char* colorXML = R"(<Ui><ColorSelect name="AppleOriginalColorSelect">
            <Size><AbsDimension x="150" y="128"/></Size><Anchors><Anchor point="BOTTOMLEFT"><Offset><AbsDimension x="24" y="24"/></Offset></Anchor></Anchors>
            <ColorWheelTexture><Size><AbsDimension x="128" y="128"/></Size><Anchors><Anchor point="LEFT"/></Anchors></ColorWheelTexture>
            <ColorValueTexture><Size><AbsDimension x="16" y="128"/></Size><Anchors><Anchor point="RIGHT"/></Anchors></ColorValueTexture>
            <Scripts><OnColorSelect>self.changes = (self.changes or 0) + 1; self.red, self.green, self.blue = r,g,b</OnColorSelect></Scripts>
            </ColorSelect></Ui>)";
        tree = XMLTree_Load(colorXML, uint32_t(std::strlen(colorXML)));
        Require(tree, "Cannot parse color-selector XML");
        m_colorSelect = static_cast<CSimpleColorSelect*>(FrameXML_CreateFrame(XMLTree_GetRoot(tree)->m_child, nullptr, &status));
        XMLTree_Free(tree);
        if (!status.IsEmpty()) {
            char errors[4096]; throw std::runtime_error(status.GetErrorStr(errors, sizeof(errors), STATUS_INFO, "\n"));
        }
        Require(m_colorSelect, "Cannot instantiate original color selector");
        auto* memory = CSimpleEditBox::s_allocator.GetData(1, __FILE__, __LINE__);
        m_edit = new (memory) CSimpleEditBox(nullptr);
        m_edit->SetName("AppleOriginalEdit");
        m_edit->SetWidth(.65f); m_edit->SetHeight(.08f);
        m_edit->SetPoint(FRAMEPOINT_CENTER, m_top.get(), FRAMEPOINT_CENTER, 0, 0, 1);
        auto* localizedFont = CSimpleFont::GetFont("EditBoxFont_Large", 0);
        Require(localizedFont, "Original Glue edit-box font is missing");
        m_edit->m_font->m_attributes.SetFont(localizedFont->m_attributes.m_font.m_str, .04f, 0);
        m_edit->m_font->UpdateObjects();
        m_edit->SetText("Apple ", nullptr);
    } catch (...) { Shutdown(); throw; }
}
UIComponents::~UIComponents() { Shutdown(); }
int32_t UIComponents::Resize(const void* data, void* owner) {
    const auto& size = *static_cast<const EVENT_DATA_SIZE*>(data);
    auto& self = *static_cast<UIComponents*>(owner);
    if (size.m_w > 0 && size.m_h > 0) {
        CoordinateSetAspectRatio(float(size.m_w) / size.m_h);
        const CRect previous = self.m_top->m_rect;
        NDCToDDC(1, 1, &self.m_top->m_rect.maxX, &self.m_top->m_rect.maxY);
        self.m_top->OnFrameSizeChanged(previous);
    }
    // Run before Top's size handler, so font and region rebuilding see the
    // host's new DDC conversion. No full Glue reload is implied by this viewer.
    return 1;
}
void UIComponents::Shutdown() {
    if (m_console) { ConsoleScreenDestroy(); ConsoleDeviceSetConsoleAccess(m_previousConsoleAccess); m_console = false; }
    if (m_resize) { EventUnregisterEx(EVENT_ID_SIZE, Resize, this, 7); m_resize = false; }
    m_top.reset(); m_button = nullptr; m_edit = nullptr; m_colorSelect = nullptr;
    if (m_metatables) {
        CSimpleEditBox::DestroyScriptMetaTable(); CSimpleFontString::DestroyScriptMetaTable();
        CSimpleColorSelect::DestroyScriptMetaTable();
        CSimpleButton::DestroyScriptMetaTable(); FrameXML_FreeHashNodes(); FrameXML_ClearFactories();
        CSimpleFont::DestroyScriptMetaTable(); CSimpleTexture::DestroyScriptMetaTable(); CSimpleFrame::DestroyScriptMetaTable();
        m_metatables = false;
    }
    if (m_screen) { ScrnDestroy(); m_screen = false; }
    if (m_ownsScript) { FrameScript_Destroy(); m_ownsScript = false; }
    XMLTree_Cleanup();
    if (m_previousAspect > 0) { CoordinateSetAspectRatio(m_previousAspect); m_previousAspect = 0; }
}
