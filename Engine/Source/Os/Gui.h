#ifndef ENGINE_SOURCE_OS_GUI_H
#define ENGINE_SOURCE_OS_GUI_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CiRect.h"

enum : int32_t {
    OSGUI_MESSAGE_STYLE_OK = 0,
    OSGUI_MESSAGE_STYLE_OKCANCEL = 1,
    OSGUI_MESSAGE_STYLE_YESNO = 2,
    OSGUI_MESSAGE_STYLE_YESNOCANCEL = 3,
};

enum : int32_t {
    OSGUI_MESSAGE_RESULT_YESOK = 0,
    OSGUI_MESSAGE_RESULT_NO = 1,
    OSGUI_MESSAGE_RESULT_CANCEL = 2,
};

enum : int32_t {
    OSGUI_KEY_CONTROL = 0,
    OSGUI_KEY_SHIFT = 1,
    OSGUI_KEY_ALT = 2,
    OSGUI_NUM_MOD_KEYS = 3,
};

enum : int32_t {
    OSGUI_WINDOW_TYPE_GX = 0,
    OSGUI_WINDOW_TYPE_ACTIVE = 1,
    OSGUI_WINDOW_TYPE_FRONT = 2,
    NUM_OSGUI_WINDOW_TYPES = 3,
};

typedef void* OsGuiHandle;
typedef void* OsGuiTreeItem;

class COsDialog;
class COsMenu;
class COsTreeView;

struct OsGuiCallbackParams {
    /* 0000 */ int32_t type;
    /* 0004 */ int32_t subType;
    /* 0008 */ int32_t code;
    /* 000C */ void* user;
};

typedef void (*OsGuiCallback)(const OsGuiCallbackParams& params);

struct OsGuiTVDDInfo {
    /* 0000 */ COsTreeView* treeView;
    /* 0004 */ int32_t action;
    /* 0008 */ OsGuiTreeItem dragItem;
    /* 000C */ OsGuiTreeItem targItem;
    /* 0010 */ int32_t targX;
    /* 0014 */ int32_t targY;
};

struct OsGuiTreeItemParams {
    /* 0000 */ int32_t used;
    /* 0004 */ CImVector color;
    /* 0008 */ void* user;
};

class COsControl {
    public:
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ COsDialog* m_dialog;
    /* 000C */ int16_t m_id;
    /* 0010 */ int32_t m_type;
    /* 0014 */ OsGuiHandle m_handle;
    /* 0018 */ OsGuiCallback m_callback;
    /* 001C */ void* m_callbackParam;
    /* 0020 */ COsMenu* m_contextMenu;
    /* 0024 */ int32_t m_contextMenuEnabled;
    /* 0028 */ int32_t m_redrawLevel;

    virtual ~COsControl();
    virtual void OnDestroy();
    virtual int32_t OnEvent(int32_t itemId, int32_t notifyCode, int32_t code);
    virtual int32_t OnDraw(void* dc, uint32_t state, CiRect& rect);
    virtual void* OnSetColors(void* dc);
    virtual int32_t OnReturn();
    virtual int32_t OnEscape();
    virtual int32_t OnMouseDown();
    virtual int32_t OnMouseUp();
    virtual void OnMouseMove(int32_t x, int32_t y);
    virtual void OnSizeChange();
    virtual void OnTextChange();
    virtual int32_t OnNotify(int32_t code, void* data);
    virtual int32_t OnCommand(int32_t code);
    virtual int32_t OnScroll(int32_t code);
    virtual int32_t OnMouseWheel(int32_t delta);
    virtual int32_t OnContextMenu(int32_t x, int32_t y);
    virtual int32_t IsHandleFromControl(OsGuiHandle handle);
    virtual int32_t CanDoClipboardAction(int32_t action);
    virtual int32_t DoClipboardAction(int32_t action);
    virtual void SetValue(int32_t value);
    virtual int32_t GetValue();

    int32_t SendEvent(int32_t event, int32_t code);
};

class COsEditBox : public COsControl {
    public:
    /* 002C */ int32_t m_filtersEnabled;
    /* 0030 */ uint32_t m_filters;
    /* 0034 */ int32_t m_selSize;

    int32_t IsCharacterAllowed(char c);
};

class COsTreeView : public COsControl {
    public:
    /* 002C */ void* m_images;
    /* 0030 */ TSGrowableArray<int32_t> m_unusedImageIDs;
    /* 0040 */ int32_t m_dragging;
    /* 0044 */ OsGuiTVDDInfo m_dragInfo;
    /* 005C */ void* m_dragImage;
    /* 0060 */ int32_t (*m_dragHandler)(const OsGuiTVDDInfo& info, void* param);
    /* 0064 */ void* m_dragHandlerParam;
    /* 0068 */ int32_t (*m_canEditFunc)(OsGuiTreeItem item, void* param);
    /* 006C */ void* m_canEditParam;
    /* 0070 */ void (*m_expandFunc)(OsGuiTreeItem item, void* param);
    /* 0074 */ void* m_expandParam;
    /* 0078 */ int32_t m_textLimit;
    /* 007C */ int32_t m_filtersEnabled;
    /* 0080 */ uint32_t m_filters;
    /* 0084 */ TSGrowableArray<OsGuiTreeItemParams> m_itemParams;

    OsGuiHandle GetEditControl();
    int32_t IsCharacterAllowed(char c);
};

class COsTabControl : public COsControl {
    public:
    int32_t GetNumItems();
    int32_t OnControlTab();
};

class COsDialog {
    public:
    /* 0004 */ OsGuiHandle m_handle;
    /* 0008 */ void* m_tooltips;
    /* 000C */ OsGuiCallback m_callback;
    /* 0010 */ void* m_callbackParam;
    /* 0014 */ TSGrowableArray<COsControl*> m_controls;
    /* 0024 */ COsControl* m_cancelButton;
    /* 0028 */ int32_t m_trackMouse;
    /* 002C */ int32_t m_mouseInside;
    /* 0030 */ int32_t m_needNewTrack;
    /* 0034 */ int32_t m_tooltipsEnabled;
    /* 0038 */ int32_t m_contextMenuEnabled;
    /* 003C */ COsMenu* m_contextMenu;
    /* 0040 */ uint32_t m_flags;
    /* 0044 */ TSGrowableArray<OsGuiHandle> m_disabledWindows;
    /* 0054 */ C2iVector m_minSize;

    virtual void SetSize(int32_t width, int32_t height);

    COsControl* FindControl(OsGuiHandle handle);
    OsGuiHandle GetParentWindow();
    int32_t HasFlag(uint32_t flag);
    int32_t IsInFront();
    int32_t OnCancel();
    int32_t OnControlTab();
    int32_t OnEvent(int32_t itemId, int32_t notifyCode, int32_t code);
    int32_t ProcessMessage(void* message);
};

class COsMenuBar {
    public:
    /* 0000 */ TSGrowableArray<COsMenu*> m_menus;
    /* 0010 */ OsGuiHandle m_windowHandle;
    /* 0014 */ void* m_menuBarHandle;
    /* 0018 */ void* m_accelerators;
};

void OsGuiBeep();
OsGuiHandle OsGuiGetWindow(int32_t type);
int32_t OsGuiIsModifierKeyDown(int32_t key);
int32_t OsGuiProcessMessage(void* message);
void OsGuiSetGxWindow(OsGuiHandle window);
int32_t OsGuiMessageBox(OsGuiHandle parentWindow, int32_t style, const char* message, const char* title);
void OsGuiSetWindowTitle(OsGuiHandle window, const char* title);

#endif
