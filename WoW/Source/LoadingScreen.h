#ifndef WOW_SOURCE_LOADINGSCREEN_H
#define WOW_SOURCE_LOADINGSCREEN_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/DB/WowClientDB.h"

struct BackgroundImage {
    /* 0000 */ const char* m_path;
    /* 0004 */ bool m_fill;
    /* 0008 */ float m_x;
    /* 000C */ float m_y;
    /* 0010 */ float m_width;
    /* 0014 */ float m_height;
};

struct DYNAMICELEMENTVERT {
    /* 0000 */ C3Vector m_position;
    /* 000C */ C2Vector m_texCoord;
    /* 0014 */ CImVector m_color;
};

template <class T>
class CLoadingScreenShipInfo {
    public:
    struct LOADINGSCREENTAXIPATHINFO {
        /* 0000 */ int32_t m_startNodeID;
        /* 0004 */ int32_t m_endNodeID;

        LOADINGSCREENTAXIPATHINFO();
    };

    struct PATHINFO {
        /* 0000 */ TSGrowableArray<uint32_t> m_legs;
    };

    /* 0000 */ TSGrowableArray<PATHINFO> m_paths;
    /* 0010 */ TSGrowableArray<LOADINGSCREENTAXIPATHINFO> m_legs;

    int32_t FindLeg(WowClientDB<T>* nodeDB, uint32_t path, int32_t continentID);
    bool GetLeg(uint32_t path, uint32_t leg, int32_t* startNodeID, int32_t* endNodeID);
    void Initialize(WowClientDB<T>* nodeDB, uint32_t pathCount);
};

int32_t IsStillLoading();
void LoadingScreenAsyncCallback(float progress, void* param);
bool LoadingScreenDrawing();
void LoadingScreenDisable();
void LoadingScreenEnableShipLeg(int32_t entryId, uint32_t leg);
void LoadingScreenEnable(int32_t mapID, bool loadingWorld);
void LoadingScreenEnableShip(uint32_t transportEntry, int32_t transportMap, int32_t mapID);
bool LoadDynamicBackground();
void LoadingScreenInitialize();
void LoadingScreenMiniDisable();
void LoadingScreenMiniEnable();
void LoadingScreenMiniUpdate(float progress);
void LoadingScreenSetScreenTipForRaceData(bool show);
void LoadingScreenSetTip(const char* tip);
void LoadingScreenSetTransparent(bool transparent);
void LoadingScreenShutdown();
void LoadingScreenSpellUpdateProgress(float progress);
void LoadingScreenWorldCallback(float progress, void* param);
void LoadingScreenXMLCallback(float progress, void* param);
void PaintDynamicLoadingBar();

// INLINED
template <class T>
CLoadingScreenShipInfo<T>::LOADINGSCREENTAXIPATHINFO::LOADINGSCREENTAXIPATHINFO() {
    this->m_startNodeID = 0;
    this->m_endNodeID = 0;
}

// VERIFY: dd3ee301 manual
// OFFSET: 0x0040B0B0
template <class T>
void CLoadingScreenShipInfo<T>::Initialize(WowClientDB<T>* nodeDB, uint32_t pathCount) {
    this->m_paths.SetCount(pathCount);

    T* prev = nullptr;
    int32_t startNodeID = 0;
    int32_t endNodeID = 0;

    for (int32_t i = 0; i < nodeDB->m_numRecords; i++) {
        T* node = &nodeDB->m_records[i];

        if (!node) {
            continue;
        }

        if (!prev || prev->m_pathID != node->m_pathID) {
            if (prev && startNodeID > 0) {
                this->m_paths[prev->m_pathID].m_legs.New(this->m_legs.Count());

                LOADINGSCREENTAXIPATHINFO* info = this->m_legs.New();
                info->m_endNodeID = endNodeID;
                info->m_startNodeID = startNodeID;
            }

            startNodeID = 0;
        }

        if (node->m_flags & 1) {
            this->m_paths[node->m_pathID].m_legs.New(this->m_legs.Count());

            LOADINGSCREENTAXIPATHINFO* info = this->m_legs.New();
            info->m_startNodeID = startNodeID;
            info->m_endNodeID = endNodeID;

            startNodeID = 0;
        } else if (prev && prev->m_pathID == node->m_pathID && prev->m_continentID != node->m_continentID) {
            this->m_paths[prev->m_pathID].m_legs.New(this->m_legs.Count());

            LOADINGSCREENTAXIPATHINFO* info = this->m_legs.New();
            info->m_endNodeID = endNodeID;
            info->m_startNodeID = startNodeID;

            startNodeID = 0;
        } else if (node->m_flags & 2) {
            endNodeID = node->m_id;

            if (!startNodeID) {
                startNodeID = node->m_id;
            }
        }

        prev = node;
    }
}

// VERIFY: 89cea504 auto
// OFFSET: 0x00408C80
template <class T>
bool CLoadingScreenShipInfo<T>::GetLeg(uint32_t path, uint32_t leg, int32_t* startNodeID, int32_t* endNodeID) {
    if (path >= this->m_paths.Count()) {
        return false;
    }

    PATHINFO& info = this->m_paths.m_data[path];

    if (leg >= info.m_legs.Count()) {
        return false;
    }

    LOADINGSCREENTAXIPATHINFO& legInfo = this->m_legs.m_data[info.m_legs.m_data[leg]];

    if (legInfo.m_startNodeID <= 0 || legInfo.m_endNodeID <= 0) {
        return false;
    }

    *startNodeID = legInfo.m_startNodeID;
    *endNodeID = legInfo.m_endNodeID;
    return true;
}

// VERIFY: a1285220 auto
// OFFSET: 0x00408CD0
template <class T>
int32_t CLoadingScreenShipInfo<T>::FindLeg(WowClientDB<T>* nodeDB, uint32_t path, int32_t continentID) {
    int32_t count = path < this->m_paths.Count() ? this->m_paths.m_data[path].m_legs.Count() : 0;

    if (!count) {
        return -1;
    }

    for (int32_t i = 0; i < count; i++) {
        int32_t startNodeID;
        int32_t endNodeID;

        if (!this->GetLeg(path, i, &startNodeID, &endNodeID)) {
            break;
        }

        T* node = nodeDB->GetRecord(startNodeID);

        if (node && node->m_continentID == continentID) {
            return i;
        }
    }

    return -1;
}

#endif
