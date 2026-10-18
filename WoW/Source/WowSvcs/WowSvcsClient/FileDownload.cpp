#include "FileDownload.h"
#include <cstdint>
#include <cstring>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/IOUnit/IOFileUnit.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/WowServices/BattlenetLogin.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/HttpDownload.h"

// VERIFY: a4caa0f9 manual
// OFFSET: 0x006BAA50
int32_t BattlenetFileDownload::GetStatus() {
    if (static_cast<uint32_t>(this->m_status) <= 1) {
        bool complete = true;
        bool done = true;

        for (int32_t i = 0; i < this->m_numDownloads; i++) {
            int64_t size = this->m_downloads[i]->Size();

            if (this->m_downloads[i]->Progress() != size) {
                complete = false;
            }

            if (!this->m_downloads[i]->Done()) {
                done = false;
            }
        }

        if (done) {
            this->m_status = complete ? 2 : 3;
        }
    }

    return this->m_status;
}

// VERIFY: 7f002935 manual
// OFFSET: 0x006BAB30
float BattlenetFileDownload::GetProgress() {
    int64_t total = 0;
    int64_t progress = 0;

    for (int32_t i = 0; i < this->m_numDownloads; i++) {
        progress += this->m_downloads[i]->Progress();
        int64_t size = this->m_downloads[i]->Size();

        if (size < 0) {
            size = 100000000;
        }

        total += size;
    }

    if (total <= 0 || progress <= 0) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(progress) / static_cast<double>(total));
}

// VERIFY: 30c6343a auto
// OFFSET: 0x006BABF0
void BattlenetFileDownload::StopDownload() {
    this->m_status = 3;
    this->m_reason = 3;

    for (int32_t i = 0; i < this->m_numDownloads; i++) {
        this->m_downloads[i]->Cancel();
    }
}

// VERIFY: 94e03e84 auto
// OFFSET: 0x006BAC30
void BattlenetFileDownload::CompleteDownload() {
    for (int32_t i = 0; i < this->m_numDownloads; i++) {
        const char* name = this->m_downloads[i]->TargetName();

        char path[260];
        CGlueMgr::GetPatchFolder(path, sizeof(path));
        SStrPack(path, "/", sizeof(path));
        SStrPack(path, name, sizeof(path));

        Storm::IOFileParam param = { path, 0xC0000000, 0, 2, 0 };
        Storm::IOUnit* unit = Storm::IOFileUnit::Create(param, nullptr);

        if (!unit) {
            this->m_reason = 1;
            this->m_status = 3;
            return;
        }

        bool written = unit->Write(this->m_downloads[i]->GetBytes(), 0, static_cast<uint32_t>(this->m_downloads[i]->Size()));
        unit->Release();

        if (!written) {
            this->m_reason = 1;
            this->m_status = 3;
            return;
        }
    }
}

// VERIFY: 2bf73f77 auto
// OFFSET: 0x006BAE80
BattlenetFileDownload::BattlenetFileDownload() {
}

// VERIFY: b66ff56a auto
// OFFSET: 0x006BAEE0
int32_t BattlenetFileDownload::GetReason(uint32_t* data) {
    if (data) {
        *data = this->m_reasonData;
    }

    return this->m_reason;
}

// VERIFY: 69399821 auto
// OFFSET: 0x006BAF00
bool BattlenetFileDownload::Accepted() {
    return this->m_accepted;
}

// VERIFY: f60321a0 auto
// OFFSET: 0x00653A10
void BattlenetFileDownload::Proceed(const uint8_t* data, uint32_t size) {
}

// VERIFY: 844a8a3e auto
// OFFSET: 0x006BAF10
bool BattlenetFileDownload::Initialize(const char* fileName, const uint8_t* md5, uint64_t size, uint64_t& offset) {
    BattlenetLogin* login = static_cast<BattlenetLogin*>(ClientServices::LoginConnection());

    if (!login || login->GetLoginServerType() != 1) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (login->GetLoginServerType() != 1) {
        return false;
    }

    this->m_numDownloads = 0;

    for (const char* instruction = login->GetPatchInstruction(0); instruction; instruction = login->GetPatchInstruction(++this->m_numDownloads)) {
        if (!SStrCmp(instruction, "redirect monolithic", SStrLen("redirect monolithic"))) {
            this->m_status = 4;
            return true;
        }
    }

    this->m_numDownloads = 0;

    for (const char* instruction = login->GetPatchInstruction(0); instruction; instruction = login->GetPatchInstruction(++this->m_numDownloads)) {
        char fields[4][250];
        memset(fields, 0, sizeof(fields));

        for (int32_t i = 0; i < 4; i++) {
            SStrTokenize(&instruction, fields[i], sizeof(fields[i]), ";", nullptr);
        }

        if (!fields[0][0]) {
            continue;
        }

        if (!fields[1][0]) {
            const char* slash = SStrChrR(fields[0], '/');

            if (!slash) {
                continue;
            }

            SStrCopy(fields[1], slash + 1, sizeof(fields[1]));
        }

        CGlueMgr::LogConnectionStatus("Downloading patch %s;%s;%s;%s", fields[0], fields[1], fields[2], fields[3]);

        HttpDownload* download = new HttpDownload(fields[0], fields[1]);
        this->m_downloads.Add(1, &download);
    }

    this->m_numDownloads = this->m_downloads.Count();
    offset = 0;

    return true;
}

// VERIFY: e0fa0897 manual
// OFFSET: 0x006BB150
void BattlenetFileDownload::Destroy(bool deleteFile) {
    this->StopDownload();

    for (int32_t i = 0; i < this->m_numDownloads; i++) {
        if (this->m_downloads[i]) {
            delete this->m_downloads[i];
        }

        this->m_downloads[i] = nullptr;
    }

    this->m_numDownloads = 0;
    this->m_downloads.Clear();
}

// VERIFY: 41ce410a manual
// OFFSET: 0x006BB1F0
BattlenetFileDownload::~BattlenetFileDownload() {
    this->Destroy(false);
}
