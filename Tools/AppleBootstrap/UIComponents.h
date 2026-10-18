#ifndef TOOLS_APPLEBOOTSTRAP_UICOMPONENTS_H
#define TOOLS_APPLEBOOTSTRAP_UICOMPONENTS_H
#include <cstdint>
#include <memory>
class CSimpleTop;
class CSimpleFrame;
class CSimpleEditBox;
class CSimpleColorSelect;

// Original Glue components shared by the Mac window and native GPU checks.
// Requires the caller's selected Event context, CVar and Services lifetime.
class UIComponents {
public:
    explicit UIComponents(bool enableConsole = false);
    ~UIComponents();
    UIComponents(const UIComponents&) = delete;
    UIComponents& operator=(const UIComponents&) = delete;
    CSimpleFrame* Button() const { return m_button; }
    CSimpleEditBox* Edit() const { return m_edit; }
    CSimpleColorSelect* ColorSelect() const { return m_colorSelect; }
private:
    void Shutdown();
    static int32_t Resize(const void*, void*);
    std::unique_ptr<CSimpleTop> m_top;
    CSimpleFrame* m_button = nullptr;
    CSimpleEditBox* m_edit = nullptr;
    CSimpleColorSelect* m_colorSelect = nullptr;
    bool m_ownsScript = false, m_screen = false, m_metatables = false;
    bool m_resize = false, m_console = false, m_previousConsoleAccess = false;
    bool m_systemScripts = false;
    float m_previousAspect = 0;
};
#endif
