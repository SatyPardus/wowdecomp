#if defined(WOWDECOMP_ARCH_32)

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <windows.h>
#include <imm.h>
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/OsIME.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00D4165C
static HIMC s_context;

// STATIC: 0x00D41660
static uint32_t s_enableCount;

// VERIFY: 6474e2bb auto
// OFFSET: 0x0086CC20
OS_IME_LANGUAGEMODE OsIMEGetLanguageMode() {
    HWND window = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
    HIMC context = ImmGetContext(window);
    if (!context) {
        return OS_IME_MODE_ROMAN;
    }

    OS_IME_LANGUAGEMODE mode = OS_IME_MODE_ROMAN;
    DWORD conversion = 0;
    DWORD sentence;
    ImmGetConversionStatus(context, &conversion, &sentence);

    if (conversion & IME_CMODE_NATIVE) {
        HKL layout = GetKeyboardLayout(0);

        switch (reinterpret_cast<uintptr_t>(layout) & 0xFF) {
            case LANG_CHINESE:
                mode = reinterpret_cast<uintptr_t>(layout) == 0x08040804 ? OS_IME_MODE_ROMAN : OS_IME_MODE_CHINESE;
                break;
            case LANG_JAPANESE:
                mode = OS_IME_MODE_JAPANESE;
                break;
            case LANG_KOREAN:
                mode = OS_IME_MODE_KOREAN;
                break;
        }
    }

    ImmReleaseContext(window, context);
    return mode;
}

// VERIFY: 7197492d auto
// OFFSET: 0x0086CCD0
static int32_t GetCompositionString(DWORD index, char* buffer, uint32_t bufferSize) {
    memset(buffer, 0, bufferSize);

    HWND window = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
    HIMC context = ImmGetContext(window);
    if (!context) {
        return 0;
    }

    uint16_t wide[512];
    uint32_t written;
    ImmGetCompositionStringA(context, index, buffer, bufferSize);
    MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, buffer, -1, reinterpret_cast<LPWSTR>(wide), 512);
    ConvertUTF16toUTF8(buffer, bufferSize - 1, wide, 512, &written, nullptr);
    buffer[written] = '\0';

    ImmReleaseContext(window, context);
    return 1;
}

// VERIFY: 485248aa auto
// OFFSET: 0x0086CD70
int32_t OsIMEGetCompositionString(char* buffer, uint32_t bufferSize) {
    return GetCompositionString(GCS_COMPSTR, buffer, bufferSize);
}

// VERIFY: 8998c161 auto
// OFFSET: 0x0086CD90
int32_t OsIMEGetCompositionResult(char* buffer, uint32_t bufferSize) {
    return GetCompositionString(GCS_RESULTSTR, buffer, bufferSize);
}

// VERIFY: b5921ccd manual
// OFFSET: 0x0086CDB0
int32_t OsIMEGetClauseInfo(uint32_t& clauseStart, uint32_t& clauseEnd, uint32_t& cursor) {
    UINT codePage = GetACP();

    HWND window = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
    HIMC context = ImmGetContext(window);
    if (!context) {
        return 0;
    }

    uint32_t cursorPos = ImmGetCompositionStringA(context, GCS_CURSORPOS, nullptr, 0) & 0xFFFF;

    uint32_t clauseSize = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, nullptr, 0);
    if (clauseSize <= 4) {
        ImmReleaseContext(window, context);
        return 0;
    }

    DWORD* clauses = static_cast<DWORD*>(_alloca(clauseSize));
    memset(clauses, 0, clauseSize);

    clauseSize = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, clauses, clauseSize);

    uint32_t clause = 0;

    if (clauseSize == IMM_ERROR_NODATA || clauseSize == IMM_ERROR_GENERAL) {
        ImmReleaseContext(window, context);
        return 0;
    }

    uint32_t clauseCount = clauseSize >> 2;
    clauses[0] = 0;

    if (clauseCount > 2) {
        for (uint32_t i = 0; i < clauseCount - 1; i++) {
            if (cursorPos >= clauses[i] && cursorPos < clauses[i + 1]) {
                clause = i;
            }
        }
    }

    char attributes[512];
    ImmGetCompositionStringA(context, GCS_COMPATTR, attributes, sizeof(attributes));

    for (uint32_t i = 0; i < clauseCount - 1; i++) {
        if (attributes[clauses[i]] == ATTR_INPUT) {
            clause = i;
        }
    }

    char composition[512];
    memset(composition, 0, sizeof(composition));
    ImmGetCompositionStringA(context, GCS_COMPSTR, composition, sizeof(composition));

    ImmReleaseContext(window, context);

    int32_t cursorWide = MultiByteToWideChar(codePage, MB_PRECOMPOSED, composition, cursorPos, nullptr, 0);
    int32_t startWide = MultiByteToWideChar(codePage, MB_PRECOMPOSED, composition, clauses[clause], nullptr, 0);
    int32_t lengthWide = MultiByteToWideChar(codePage, MB_PRECOMPOSED, composition, clauses[clause + 1] - clauses[clause], nullptr, 0);

    clauseStart = startWide;
    clauseEnd = startWide + lengthWide;
    cursor = cursorWide;

    return 1;
}

// VERIFY: 2af47370 auto
// OFFSET: 0x0086CF90
void OsIMEEnable(int32_t enable) {
    HWND window = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));

    if (enable) {
        if (++s_enableCount == 1) {
            ImmAssociateContext(window, s_context);
        }
    } else if (s_enableCount) {
        if (--s_enableCount == 0) {
            ImmAssociateContext(window, nullptr);
        }
    }
}

// VERIFY: 3444e0a4 auto
// OFFSET: 0x0086CFF0
void OsIMESync() {
    HWND window = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
    HIMC context = ImmGetContext(window);

    if (s_enableCount) {
        if (context != s_context) {
            ImmAssociateContext(window, s_context);
        }
    } else if (context) {
        ImmAssociateContext(window, nullptr);
    }
}

// VERIFY: 215fd519 auto
// OFFSET: 0x0086D030
void OsIMESetLanguageMode(int32_t native) {
    HIMC context = ImmGetContext(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)));
    if (!context) {
        return;
    }

    DWORD conversion = 0;
    DWORD sentence;
    ImmGetConversionStatus(context, &conversion, &sentence);

    if (native) {
        conversion |= IME_CMODE_NATIVE;
    } else {
        conversion &= ~IME_CMODE_NATIVE;
    }

    ImmSetConversionStatus(context, conversion, sentence);
}

// VERIFY: a0a00041 auto
// OFFSET: 0x0086D0A0
void OsIMEInitialize() {
    s_context = ImmAssociateContext(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), nullptr);
}

// VERIFY: 1c6b3a2c auto
// OFFSET: 0x0086D0C0
void OsIMEDestroy() {
    ImmAssociateContextEx(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), nullptr, IACE_DEFAULT);
    s_context = nullptr;
}

// VERIFY: 85ca57f2 manual
// OFFSET: 0x0086D1D0
int32_t OsIMEGetCandidates(uint32_t listMask, uint32_t& pageStart, uint32_t& pageSize, uint32_t& count, uint32_t& selection, TSGrowableArray<OsIMECandidate>& candidates) {
    candidates.Clear();

    HWND window = static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX));
    HIMC context = ImmGetContext(window);
    if (!context) {
        return 0;
    }

    if (!listMask) {
        ImmReleaseContext(window, context);
        return 0;
    }

    DWORD index = -1;
    while (listMask & 1) {
        listMask >>= 1;
        index++;
    }

    DWORD listSize = ImmGetCandidateListA(context, index, nullptr, 0);
    if (!listSize) {
        ImmReleaseContext(window, context);
        return 0;
    }

    int32_t changed = 0;
    CANDIDATELIST* list = static_cast<CANDIDATELIST*>(SMemAlloc(listSize, __FILE__, __LINE__, 0));
    ImmGetCandidateListA(context, index, list, listSize);

    if (!list->dwPageSize) {
        ImmNotifyIME(context, NI_SETCANDIDATE_PAGESIZE, index, 9);
        SMemFree(list, __FILE__, __LINE__, 0);
        ImmReleaseContext(window, context);
        return 0;
    }

    while (list->dwSelection < list->dwPageStart) {
        changed = 1;
        list->dwPageStart = list->dwPageStart > list->dwPageSize ? list->dwPageStart - list->dwPageSize : 0;
    }

    while (list->dwPageStart + list->dwPageSize <= list->dwSelection) {
        list->dwPageStart += list->dwPageSize;
        changed = 1;
    }

    if (changed) {
        ImmNotifyIME(context, NI_SETCANDIDATE_PAGESTART, index, list->dwPageStart);
        SMemFree(list, __FILE__, __LINE__, 0);
        ImmReleaseContext(window, context);
        return 0;
    }

    pageStart = list->dwPageStart;
    pageSize = list->dwPageSize;
    count = list->dwCount;
    selection = list->dwSelection;

    for (uint32_t i = 0; i < list->dwPageSize; i++) {
        OsIMECandidate* candidate = candidates.New();
        uint32_t candidateIndex = i + list->dwPageStart;
        uint32_t written = 0;

        if (candidateIndex < list->dwCount) {
            uint16_t wide[512];
            MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, reinterpret_cast<char*>(list) + list->dwOffset[candidateIndex], -1, reinterpret_cast<LPWSTR>(wide), 512);
            ConvertUTF16toUTF8(candidate->m_text, sizeof(candidate->m_text) - 1, wide, 512, &written, nullptr);
        }

        candidate->m_text[written] = '\0';
    }

    SMemFree(list, __FILE__, __LINE__, 0);
    ImmReleaseContext(window, context);
    return 1;
}

#endif
