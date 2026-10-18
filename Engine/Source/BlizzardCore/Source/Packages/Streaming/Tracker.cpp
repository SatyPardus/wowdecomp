#include "Engine/Source/Base/CRTRandom.h"
#include "Tracker.h"
#include <cstdint>
#include <cstdlib>
#include <string>
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/Os/TimeManager.h"

// STATIC: 0x00AB9414
static const uint32_t s_frameTimeBuckets[25] = { 0, 1, 8, 11, 14, 18, 24, 32, 42, 56, 74, 97, 128, 170, 224, 295, 390, 515, 680, 897, 1184, 1563, 2063, 2724, 3595 };

// STATIC: 0x00B38B90
static int32_t s_speedTest;

// STATIC: 0x00B38B98
static double s_frameTimeHistogram[24];

// STATIC: 0x00B38C58
const char* Tracker::trackerAddress;

// STATIC: 0x00B38C5C
static char s_locale[5];

// STATIC: 0x00B38C64
static int32_t s_buildnum;

// STATIC: 0x00B38C68
static uint32_t s_lastFrameTime;

// STATIC: 0x00B38C6C
static char s_peerId[21];

// STATIC: 0x00B38C84
static char s_key[9];

// VERIFY: fab8b414 manual
// OFFSET: 0x004621F0
void Tracker::TrackerHttpDataDownloadCallBack::Completed(Blizzard::URL::DownloadCallback::Status status) {
    Blizzard::Lock::EventSet(this->m_event);
}

// VERIFY: 6b51736a auto
// OFFSET: 0x00462200
char* Tracker::GetPeerID() {
    if (!s_peerId[0]) {
        for (int32_t i = 0; i < 20; i++) {
            s_peerId[i] = ClientRand() % 222 + 33;
        }

        s_peerId[20] = '\0';
    }

    return s_peerId;
}

// VERIFY: 19df183c auto
// OFFSET: 0x00462240
char* Tracker::GetKey() {
    if (!s_key[0]) {
        for (int32_t i = 0; i < 8; i++) {
            s_key[i] = ClientRand() % 9 + '0';
        }

        s_key[8] = '\0';
    }

    return s_key;
}

// VERIFY: bb4bd1a3 auto
// OFFSET: 0x00462280
const char* Tracker::GetTrackerAddress(const char* locale) {
    if (!Blizzard::String::Equal(locale, "enUS") || !Blizzard::String::Equal(locale, "esMX")) {
        return "http://us.tracker.worldofwarcraft.com:3724/announce";
    }

    if (!Blizzard::String::Equal(locale, "deDE") || !Blizzard::String::Equal(locale, "enGB") || !Blizzard::String::Equal(locale, "esES") || !Blizzard::String::Equal(locale, "frFR")) {
        return "http://eu.tracker.worldofwarcraft.com:3724/announce";
    }

    return nullptr;
}

// VERIFY: 2b68a577 auto
// OFFSET: 0x00462310
void Tracker::SetLocale(const char* locale) {
    Tracker::trackerAddress = GetTrackerAddress(locale);
    Blizzard::String::Copy(s_locale, locale, sizeof(s_locale));
}

// VERIFY: 76903da7 auto
// OFFSET: 0x00462340
void Tracker::SetBuildnum(int32_t buildnum) {
    s_buildnum = buildnum;
}

// VERIFY: 2cb7c213 auto
// OFFSET: 0x00462350
void StartupReportSetSpeedTest(int32_t speedTest) {
    s_speedTest = speedTest;
}

// VERIFY: a213c71d auto
// OFFSET: 0x00462360
void StartupReportFrameCallback() {
    uint32_t now = OsGetAsyncTimeMs();

    if (s_lastFrameTime) {
        uint32_t elapsed = now - s_lastFrameTime;

        int32_t bucket = 0;
        while (bucket < 24 && elapsed >= s_frameTimeBuckets[bucket + 1]) {
            bucket++;
        }

        if (bucket == 0) {
            s_lastFrameTime = now;
            s_frameTimeHistogram[0] += 1.0;
            return;
        }

        if (bucket >= 23) {
            s_lastFrameTime = now;
            s_frameTimeHistogram[23] += 1.0;
            return;
        }

        double low;
        double high;
        double middle = (s_frameTimeBuckets[bucket] + s_frameTimeBuckets[bucket + 1] - 1) * 0.5;

        if (middle <= elapsed) {
            low = middle;
            high = (s_frameTimeBuckets[bucket + 2] + s_frameTimeBuckets[bucket + 1] - 1) * 0.5;
        } else {
            high = middle;
            low = (s_frameTimeBuckets[bucket - 1] + s_frameTimeBuckets[bucket] - 1) * 0.5;
            bucket--;
        }

        double weight = (high - elapsed) / (high - low);
        s_frameTimeHistogram[bucket] += weight;
        s_frameTimeHistogram[bucket + 1] += 1.0 - weight;
    }

    s_lastFrameTime = now;
}

// VERIFY: f7ae1f10 manual
// OFFSET: 0x00462500
Tracker::TrackerHttpDataDownloadCallBack::TrackerHttpDataDownloadCallBack() {
    this->m_timeout = 10000;
    this->m_shouldBlock = true;
    Blizzard::Lock::EventCreate(this->m_event);
}

// VERIFY: 1664abee manual
// OFFSET: 0x00462570
int32_t Tracker::TrackerHttpDataDownloadCallBack::GetTimeout() {
    return this->m_timeout;
}

// VERIFY: bef26aea manual
// OFFSET: 0x00462580
bool Tracker::TrackerHttpDataDownloadCallBack::ShouldBlock() {
    return this->m_shouldBlock;
}

// VERIFY: 9b1fa545 manual
// OFFSET: 0x00462590
void Tracker::TrackerHttpDataDownloadCallBack::SetTimeOutValue(uint32_t timeout) {
    this->m_timeout = timeout;
}

// VERIFY: 5d062b5f manual
// OFFSET: [0x004625A0, 0x00462610]
Tracker::TrackerHttpDataDownloadCallBack::~TrackerHttpDataDownloadCallBack() {
    Blizzard::Lock::EventDestroy(this->m_event);
}

// VERIFY: 70242d9a auto
// OFFSET: 0x00462640
bool Tracker::TrackerHttpDataDownloadCallBack::ReceivedData(const void* data, int32_t size) {
    this->m_data.append(static_cast<const char*>(data), size);
    return true;
}

// VERIFY: 5902bb30 manual
// OFFSET: 0x00462660
std::string Tracker::ByteToHex(uint8_t byte) {
    char digits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    std::string hex;
    hex.append(1, digits[byte >> 4]);
    hex.append(1, digits[byte & 0xF]);
    return hex;
}

// VERIFY: 8039dbb0 manual
// OFFSET: 0x004627D0
std::string Tracker::BytesToHex(const uint8_t* bytes, int32_t count) {
    std::string hex;

    for (int32_t i = 0; i < count; i++) {
        hex += ByteToHex(bytes[i]);
    }

    return hex;
}

// VERIFY: b29c0978 manual
// OFFSET: 0x00462870
std::string Tracker::Quote(const char* str, uint32_t length) {
    std::string quoted;

    for (uint32_t i = 0; i < length; i++) {
        char c = str[i];

        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '.' || c == '-' || c == '/') {
            quoted.append(1, str[i]);
        } else {
            quoted.append(1, '%');
            quoted += BytesToHex(reinterpret_cast<const uint8_t*>(&str[i]), 1);
        }
    }

    return quoted;
}

// VERIFY: ed98d863 manual
// OFFSET: 0x00462970
std::string Tracker::MakeTrackerURL(const char* address, const char* infoHash, int32_t event, int64_t downloaded, const char* stats) {
    std::string key = GetKey();
    std::string peerId = GetPeerID();
    std::string port = "3724";

    std::string url = address;
    url += "?info_hash=" + Quote(infoHash, 20);
    url += "&peer_id=" + Quote(peerId.c_str(), 20);
    url += "&key=" + Quote(key.c_str(), 8);
    url += "&port=";
    url += port.c_str();

    if (!downloaded) {
        downloaded = 2;
    }

    switch (event) {
        case 0:
            url += "&uploaded=0";
            url += "&downloaded=0";
            url += "&left=2";
            url += "&event=started";

            if (stats) {
                url += stats;
            } else {
                url += Blizzard::String::QuickFormat<1024>("&stats=v,%d,b,%d,l,%s", 5, s_buildnum, s_locale).m_buffer;
            }

            break;

        case 1:
            url += "&uploaded=0";
            url += Blizzard::String::QuickFormat<1024>("&downloaded=%lld", downloaded).m_buffer;
            url += "&left=0";
            url += "&event=completed";

            if (stats) {
                url += stats;
            } else {
                url += Blizzard::String::QuickFormat<1024>("&stats=v,%d,b,%d,l,%s", 5, s_buildnum, s_locale).m_buffer;
            }

            break;

        case 2:
            url += "&uploaded=0";
            url += Blizzard::String::QuickFormat<1024>("&downloaded=%lld", downloaded).m_buffer;
            url += "&left=0";
            url += "&event=stopped";

            if (stats) {
                url += stats;
            } else {
                url += Blizzard::String::QuickFormat<1024>("&stats=v,%d,b,%d,l,%s", 5, s_buildnum, s_locale).m_buffer;
            }

            break;

        case 3:
            url += "&uploaded=0";
            url += "&downloaded=1";
            url += "&left=1";
            url += "&event=stopped";

            if (stats) {
                url += stats;
            } else {
                url += Blizzard::String::QuickFormat<1024>("&stats=v,%d,b,%d,l,%s", 5, s_buildnum, s_locale).m_buffer;
            }

            break;
    }

    return url;
}

// VERIFY: 514e4984 manual
// OFFSET: 0x00462E20
void Tracker::SendToTracker(const char* infoHash, int32_t event, int64_t downloaded, const char* stats) {
    if (!Tracker::trackerAddress) {
        return;
    }

    TrackerHttpDataDownloadCallBack callback;
    std::string url = MakeTrackerURL(Tracker::trackerAddress, infoHash, event, downloaded, stats);
    Blizzard::URL::DownloadURL(url.c_str(), &callback);
    Blizzard::Lock::EventWaitForever(callback.m_event);
}

// VERIFY: 498455b5 manual
// OFFSET: 0x00462EF0
void StartupReportSendTelemetry(bool start, bool unique) {
    if (start) {
        if (unique) {
            Tracker::SendToTracker("HOWMANYUNIQUEUSERARE", 0, 0, nullptr);
            Tracker::SendToTracker("HOWMANYUNIQUEUSERARE", 1, 0, nullptr);
            Tracker::SendToTracker("HOWMANYUNIQUEUSERARE", 2, 0, nullptr);
        }

        Tracker::SendToTracker("DOUSERSLOVESTREAMING", 0, 0, nullptr);
        return;
    }

    double elapsed = static_cast<double>(Blizzard::Streaming::s_elapsedSinceStart);
    int32_t milliseconds = static_cast<int32_t>(elapsed * 0.000001);
    double seconds = elapsed * 0.000000001;
    int32_t readRate = static_cast<int32_t>(Blizzard::Streaming::s_statBytesRead / seconds);
    int32_t downloadRate = static_cast<int32_t>(Blizzard::Streaming::s_statBytesDownloaded / seconds);
    int32_t localRate = static_cast<int32_t>(Blizzard::Streaming::s_statBytesLocal / seconds);

    char stats[260];
    Blizzard::String::Format(stats, sizeof(stats), "&stats=v,%d,b,%d,l,%s,st,%d,t,%d,sr,%d,sd,%d,sl,%d,rt,%d,sl,%d,ft,", 5, s_buildnum, s_locale, s_speedTest, milliseconds, readRate, downloadRate, localRate, Blizzard::Streaming::s_statRetries, Blizzard::Streaming::FileManifest::GetBGPreloadSleep());

    double histogram[24];
    Blizzard::String::MemCopy(histogram, s_frameTimeHistogram, sizeof(histogram));

    double carry = 0.0;

    for (int32_t i = 0; i < 24; i++) {
        double value = carry + histogram[i];
        histogram[i] = value;

        const char* format = "%d,";
        carry = value - static_cast<int32_t>(value + 0.5);

        if (i == 23) {
            format = "%d";
        }

        char item[260];
        Blizzard::String::Format(item, sizeof(item), format, static_cast<int32_t>(value + 0.5));
        Blizzard::String::Append(stats, item, sizeof(stats));
        Blizzard::String::Append(Blizzard::Streaming::s_statFrameTimes, item, sizeof(Blizzard::Streaming::s_statFrameTimes));
    }

    Tracker::SendToTracker("DOUSERSLOVESTREAMING", 1, Blizzard::Streaming::s_statBytesRead + Blizzard::Streaming::s_statBytesDownloaded, stats);
    Tracker::SendToTracker("DOUSERSLOVESTREAMING", 2, Blizzard::Streaming::s_statBytesRead + Blizzard::Streaming::s_statBytesDownloaded, nullptr);
}
