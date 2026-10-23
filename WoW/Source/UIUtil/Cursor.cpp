#include "Cursor.h"
#include <cstring>
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Ui/LootFrame.h"

enum CURSORITEMTYPE : uint32_t {
    CURSOR_EMPTY = 0,
    CURSOR_ITEM = 1,
    CURSOR_MONEY = 2,
    CURSOR_SPELL = 3,
    // NUM_CURSOR_ITEM_TYPES = 4,
};

// STATIC: 0x00AD2808
static const char* s_cursorNames[NUM_CURSOR_IMAGES] = {
    nullptr,
    "Point",
    "Cast",
    "Buy",
    "Attack",
    "Interact",
    "Speak",
    "Inspect",
    "Pickup",
    "Taxi",
    "Trainer",
    "Mine",
    "Skin",
    "GatherHerbs",
    "PickLock",
    "Mail",
    "LootAll",
    "Repair",
    "RepairNPC",
    "Item",
    "SkinHorde",
    "SkinAlliance",
    "Innkeeper",
    "Quest",
    "QuestRepeatable",
    "QuestTurnIn",
    "vehichleCursor",
    "UnablePoint",
    "UnableCast",
    "UnableBuy",
    "UnableAttack",
    "UnableInteract",
    "UnableSpeak",
    "UnableInspect",
    "UnablePickup",
    "UnableTaxi",
    "UnableTrainer",
    "UnableMine",
    "UnableSkin",
    "UnableGatherHerbs",
    "UnablePickLock",
    "UnableMail",
    "UnableLootAll",
    "UnableRepair",
    "UnableRepairNPC",
    "UnableItem",
    "UnableSkinHorde",
    "UnableSkinAlliance",
    "UnableInnkeeper",
    "UnableQuest",
    "UnableQuestRepeatable",
    "UnableQuestTurnIn",
    "UnablevehichleCursor",
    nullptr,
};

// STATIC: 0x00C25CD0
static char s_cursorFile[260];

// STATIC: 0x00C25DD4
static uint32_t s_cursorItemMipsHeight;

// STATIC: 0x00C25DD8
static uint32_t s_cursorItemMipsWidth;

// STATIC: 0x00C25DDC
static MipBits* s_cursorItemMips;

// STATIC: 0x00C25DE0
static CURSORITEMTYPE s_cursorItemType;

// STATIC: 0x00C25DE4
static CURSORMODE s_cursorResetMode;

// STATIC: 0x00C25DE8
static uint32_t s_cursorItem[1024];

// STATIC: 0x00C26DE8
static CURSORMODE s_cursorMode;

// STATIC: 0x00C26DF0
static uint32_t s_cursorImages[NUM_CURSOR_IMAGES][1024];

// VERIFY: dd7c01bd auto
// OFFSET: 0x006167E0
void CursorDropItem() {
    if (s_cursorItemType) {
        s_cursorItemType = CURSOR_EMPTY;
        UpdateCursor();
    }
}

// VERIFY: be139389 auto
// OFFSET: 0x00616260
CURSORMODE CursorGetResetMode() {
    return s_cursorResetMode;
}

// VERIFY: 2ca86d65 auto
// OFFSET: 0x00616270
void CursorSetResetMode(CURSORMODE mode) {
    s_cursorResetMode = mode;
}

// VERIFY: 510ad1f1 manual
// OFFSET: 0x006160B0
int32_t CopyCursorImage(uint32_t* image, MipBits* mipImages, uint32_t width, uint32_t height) {
    if (mipImages) {
        if (width == 32) {
            if (height == 32) {
                memcpy(image, mipImages->m_mip[0], 4096);
                return 1;
            }
        } else if (width == 64 && height == 64) {
            const uint32_t* source = reinterpret_cast<const uint32_t*>(mipImages->m_mip[0]);

            for (uint32_t* row = image; row < image + 1024; row += 32) {
                const uint32_t* top = source;
                const uint32_t* bottom = source + 64;

                for (uint32_t* pixel = row; pixel < row + 32; pixel++) {
                    uint32_t redBlue = ((top[0] & 0xFF00FF) + (top[1] & 0xFF00FF) + (bottom[0] & 0xFF00FF) + (bottom[1] & 0xFF00FF) + 0x20002) >> 2;
                    uint32_t green = ((top[0] >> 2) & 0x3FC0) + ((top[1] >> 2) & 0x3FC0) + ((bottom[0] >> 2) & 0x3FC0) + ((bottom[1] >> 2) & 0x3FC0) + 0x80;
                    *pixel = (redBlue & 0xFFFF00FF) | (green & 0xFF00) | 0xFF000000;

                    top += 2;
                    bottom += 2;
                }

                source += 128;
            }

            return 1;
        }
    }

    memset(image, 0, 4096);
    return 0;
}

// VERIFY: ffd92c97 manual
// OFFSET: 0x006162C0
void UpdateCursor() {
    uint32_t* pixels = g_theGxDevicePtr->CursorLock();

    if (s_cursorItemType) {
        if (s_cursorItemMips) {
            memcpy(pixels, s_cursorImages[ITEM_CURSOR], 4096);
        } else {
            for (uint32_t i = 0; i < 1024; i++) {
                uint32_t pixel = s_cursorImages[ITEM_CURSOR][i];

                if (!(pixel & 0xFF000000)) {
                    pixel = s_cursorItem[i];
                }

                pixels[i] = pixel;
            }
        }
    } else {
        memcpy(pixels, s_cursorImages[s_cursorMode], 4096);
    }

    g_theGxDevicePtr->CursorUnlock(0, 0);
}

// VERIFY: 448b2353 auto
// OFFSET: 0x006163B0
void CursorInitialize() {
    s_cursorMode = POINT_CURSOR;
    s_cursorResetMode = POINT_CURSOR;
    s_cursorItemType = CURSOR_EMPTY;
    s_cursorItemMips = nullptr;
    s_cursorItemMipsWidth = 0;
    s_cursorItemMipsHeight = 0;

    for (uint32_t i = 0; i < NUM_CURSOR_IMAGES; i++) {
        MipBits* images = nullptr;
        uint32_t width;
        uint32_t height;
        PIXEL_FORMAT format;

        if (s_cursorNames[i]) {
            char fileName[260];
            SStrPrintf(fileName, sizeof(fileName), "Interface\\Cursor\\%s.blp", s_cursorNames[i]);

            width = 0;
            height = 0;
            format = PIXEL_ARGB8888;
            images = TextureLoadImage(fileName, &width, &height, &format, nullptr, nullptr, nullptr, 0);
        }

        if (images) {
            CopyCursorImage(s_cursorImages[i], images, width, height);
            TextureFreeMippedImg(images, format, width, height);
        } else {
            memset(s_cursorImages[i], 0, sizeof(s_cursorImages[i]));
        }
    }

    s_cursorFile[0] = '\0';
    UpdateCursor();
}

// VERIFY: 80dc5ddb auto
// OFFSET: 0x00616280
CURSORMODE CursorGetModeFromName(const char* name) {
    for (uint32_t i = POINT_CURSOR; i < NUM_CURSOR_MODES; i++) {
        if (!SStrCmpI(name, s_cursorNames[i], STORM_MAX_STR)) {
            return static_cast<CURSORMODE>(i);
        }
    }

    return NO_CURSOR;
}

// VERIFY: c3ebf462 auto
// OFFSET: 0x00616800
void CursorSetMode(CURSORMODE mode) {
    if (mode != NUM_CURSOR_MODES && s_cursorMode != mode) {
        s_cursorMode = mode;
        UpdateCursor();
        FrameScript_SignalEvent(SCRIPT_EVENT_CURSOR_UPDATE, nullptr);
    }
}

// VERIFY: de091fd1 auto
// OFFSET: 0x00616720
void CursorGrabVirtualItem(int32_t displayId) {
    if (s_cursorItemMips) {
        TextureFreeMippedImg(s_cursorItemMips);
        s_cursorItemMips = nullptr;
        s_cursorItemMipsWidth = 0;
        s_cursorItemMipsHeight = 0;
    }

    if (displayId) {
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        const char* separator = "\\";

        if (!*path) {
            separator = "";
        }

        char fileName[260];
        SStrPrintf(fileName, sizeof(fileName), "%s%s%s", path, separator, CGItem_C::GetInventoryArt(displayId));
        PIXEL_FORMAT format = PIXEL_ARGB8888;
        s_cursorItemMips = TextureLoadImage(fileName, &s_cursorItemMipsWidth, &s_cursorItemMipsHeight, &format, nullptr, nullptr, nullptr, 0);
    }

    s_cursorItemType = CURSOR_ITEM;
    CursorReleaseItemMips();
}

// VERIFY: ed01e032 manual
// OFFSET: 0x00616830
int32_t CursorSetFromName(const char* name) {
    int32_t result = !SStrCmpI(s_cursorFile, name, STORM_MAX_STR);
    int32_t changed = !result || s_cursorMode != NUM_CURSOR_MODES;

    if (!result) {
        uint32_t width = 0;
        uint32_t height = 0;
        PIXEL_FORMAT format = PIXEL_ARGB8888;
        MipBits* images = TextureLoadImage(name, &width, &height, &format, nullptr, nullptr, nullptr, 0);

        if (images) {
            if (CopyCursorImage(s_cursorImages[NUM_CURSOR_MODES], images, width, height)) {
                result = 1;
            }

            TextureFreeMippedImg(images, format, width, height);
        } else {
            memset(s_cursorImages[NUM_CURSOR_MODES], 0, sizeof(s_cursorImages[NUM_CURSOR_MODES]));
        }

        SStrCopy(s_cursorFile, name, sizeof(s_cursorFile));
    }

    if (changed) {
        s_cursorMode = NUM_CURSOR_MODES;
        UpdateCursor();
        FrameScript_SignalEvent(SCRIPT_EVENT_CURSOR_UPDATE, nullptr);
    }

    return result;
}

// VERIFY: 0107efc6 auto
// OFFSET: 0x00616920
void CursorResetMode() {
    if (s_cursorResetMode != NUM_CURSOR_MODES && s_cursorMode != s_cursorResetMode) {
        s_cursorMode = s_cursorResetMode;
        UpdateCursor();
        FrameScript_SignalEvent(SCRIPT_EVENT_CURSOR_UPDATE, nullptr);
    }
}

// VERIFY: 27d3e602 manual
// OFFSET: 0x006164B0
void CursorReleaseItemMips() {
    MipBits* mips = s_cursorItemMips;

    if (!mips) {
        return;
    }

    if (s_cursorItemType) {
        CopyCursorImage(s_cursorItem, mips, s_cursorItemMipsWidth, s_cursorItemMipsHeight);
        TextureFreeMippedImg(mips);
    } else {
        TextureFreeMippedImg(s_cursorItemMips);
    }

    s_cursorItemMipsHeight = 0;
    s_cursorItemMipsWidth = 0;
    s_cursorItemMips = nullptr;
    UpdateCursor();
}

// VERIFY: b444a3d5 manual
// OFFSET: 0x00616630
void CursorSetHeldItem(WOWGUID item) {
    if (s_cursorItemMips) {
        TextureFreeMippedImg(s_cursorItemMips);
        s_cursorItemMips = nullptr;
        s_cursorItemMipsWidth = 0;
        s_cursorItemMipsHeight = 0;
    }

    CGItem_C* itemObject = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (itemObject) {
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        const char* art = itemObject->GetInventoryArt();
        const char* separator = "\\";

        if (!path || !*path || !art || !*art) {
            separator = "";
        }

        char fileName[260];
        SStrPrintf(fileName, sizeof(fileName), "%s%s%s", path, separator, art);
        PIXEL_FORMAT format = PIXEL_ARGB8888;
        s_cursorItemMips = TextureLoadImage(fileName, &s_cursorItemMipsWidth, &s_cursorItemMipsHeight, &format, nullptr, nullptr, nullptr, 0);
    }

    s_cursorItemType = CURSOR_ITEM;
    CursorReleaseItemMips();
}

// VERIFY: 22c5ea5f auto
// OFFSET: 0x00616510
void CursorGrabMoney(uint32_t amount) {
    if (s_cursorItemMips) {
        TextureFreeMippedImg(s_cursorItemMips);
        s_cursorItemMips = nullptr;
        s_cursorItemMipsWidth = 0;
        s_cursorItemMipsHeight = 0;
    }

    char fileName[260];
    GetCurrencyIcon(amount, fileName, sizeof(fileName));
    PIXEL_FORMAT format = PIXEL_ARGB8888;
    s_cursorItemMips = TextureLoadImage(fileName, &s_cursorItemMipsWidth, &s_cursorItemMipsHeight, &format, nullptr, nullptr, nullptr, 0);
    s_cursorItemType = CURSOR_MONEY;
    CursorReleaseItemMips();
}

// VERIFY: 9fa05f0a auto
// OFFSET: 0x006165B0
void CursorGrabSpell(const char* texture) {
    if (s_cursorItemMips) {
        TextureFreeMippedImg(s_cursorItemMips);
        s_cursorItemMips = nullptr;
        s_cursorItemMipsWidth = 0;
        s_cursorItemMipsHeight = 0;
    }

    if (texture) {
        PIXEL_FORMAT format = PIXEL_ARGB8888;
        s_cursorItemMips = TextureLoadImage(texture, &s_cursorItemMipsWidth, &s_cursorItemMipsHeight, &format, nullptr, nullptr, nullptr, 0);
    }

    s_cursorItemType = CURSOR_SPELL;
    CursorReleaseItemMips();
}

// VERIFY: 10223bc1 auto
// OFFSET: 0x00616220
void CursorDestroy() {
    if (!s_cursorItemMips) {
        return;
    }

    TextureFreeMippedImg(s_cursorItemMips);
    s_cursorItemMips = nullptr;
    s_cursorItemMipsWidth = 0;
    s_cursorItemMipsHeight = 0;
}
