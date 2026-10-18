#include "WoW/Source/DB/DBClient/DBCache.h"
#include "WoW/Source/Object/ItemNameCache.h"
#include "WoW/Source/UIUtil/BannedAddOns.h"
#include "WoW/Source/Client.h"
#include <CommonCrypto/CommonDigest.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct CacheRoot {
    std::filesystem::path root;
    std::string previous;
    bool hadOverride = false;
    char locale[5];
    uint32_t localeId = SFile::s_locale;
    CacheRoot() {
        if (const char* value = std::getenv("WOW_CACHE_ROOT")) { previous = value; hadOverride = true; }
        std::memcpy(locale, s_localeCode, 5);
        std::string pattern = (std::filesystem::temp_directory_path() / "wow-cache-XXXXXX").string();
        Require(mkdtemp(pattern.data()) != nullptr, "Cannot create isolated cache check root");
        root = pattern;
        setenv("WOW_CACHE_ROOT", pattern.c_str(), 1);
        Select("zhCN", 'zhCN');
    }
    static void Select(const char* name, uint32_t id) { std::memcpy(s_localeCode, name, 5); SFile::s_locale = id; }
    ~CacheRoot() {
        DestroyBannedAddOns();
        std::memcpy(s_localeCode, locale, 5); SFile::s_locale = localeId;
        if (hadOverride) setenv("WOW_CACHE_ROOT", previous.c_str(), 1); else unsetenv("WOW_CACHE_ROOT");
        std::error_code error; std::filesystem::remove_all(root, error);
    }
};
std::vector<uint8_t> Read(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary); Require(bool(input), "Cannot read saved cache");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void Write(const std::filesystem::path& file, const std::vector<uint8_t>& bytes) {
    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    Require(bool(output), "Cannot write cache check fixture");
}
void Append(std::vector<uint8_t>& bytes, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes.push_back(uint8_t(value >> (i * 8)));
}
using ItemCache = DBCache<ItemNameCache_C, int32_t, HASHKEY_INT>;
void Add(ItemCache& cache, int32_t key, const char* name, uint32_t inventoryType) {
    CDataStore response; response.Put(uint32_t(key)); response.PutString(name); response.Put(inventoryType); response.Finalize();
    cache.AddItems(&response, true);
}
void CheckItem(ItemCache& cache, int32_t key, const char* name, uint32_t type) {
    auto* entry = cache.m_entries.Ptr(DBCacheHashKey(key), key);
    Require(entry && entry->m_loaded && !std::strcmp(entry->m_record.m_name, name) && entry->m_record.m_inventoryType == type,
        "Original item cache did not preserve its record");
}
}

void CheckClientCache() {
    CacheRoot scope;
    char path[260];
    Require(GetCacheFilePath(path, sizeof(path), "itemnamecache.wdb"), "Cannot select native cache file");
    const std::filesystem::path itemPath = scope.root / "WDB/zhCN/itemnamecache.wdb";
    Require(std::filesystem::weakly_canonical(path) == std::filesystem::weakly_canonical(itemPath), "Cache escaped its configured locale root");
    ItemCache cache('WNDB', "itemnamecache.wdb", 0, 0, true, true, 0);
    cache.Load(); cache.VerifyAndUpdateVersion(73);
    const char* name = "\xe9\x93\x81\xe5\x89\x91";
    Add(cache, 501, name, 13); Add(cache, 502, "Temporary", 7); cache.SetTemporary(502);
    cache.Destroy();
    std::vector<uint8_t> expected;
    for (uint32_t field : {uint32_t('WNDB'), 12340u, uint32_t('zhCN'), uint32_t(sizeof(ItemNameCache_C)), 1u, 73u, 501u, uint32_t(std::strlen(name) + 5)}) Append(expected, field);
    expected.insert(expected.end(), name, name + std::strlen(name) + 1); Append(expected, 13); Append(expected, 0); Append(expected, 0);
    Require(Read(itemPath) == expected, "Original cache file header, payload or temporary-record policy changed");
    cache.Load(); CheckItem(cache, 501, name, 13);
    Require(cache.m_version == 73 && !cache.m_entries.Ptr(502, 502), "Original cache version or temporary record changed");
    cache.VerifyAndUpdateVersion(73); CheckItem(cache, 501, name, 13);
    cache.VerifyAndUpdateVersion(74); Require(!cache.m_entries.m_fulllist.Head(), "Cache version change did not invalidate records");
    cache.Destroy(); cache.Load(); Require(cache.m_version == 74 && !cache.m_entries.m_fulllist.Head(), "Cache invalidation did not persist"); cache.Destroy();
    Write(itemPath, expected);
    CacheRoot::Select("enUS", 'enUS'); cache.Load(); Require(!cache.m_entries.m_fulllist.Head(), "Cache locale partitions leaked records");
    Add(cache, 501, "Iron Sword", 13); cache.Destroy(); cache.Load(); CheckItem(cache, 501, "Iron Sword", 13); cache.Destroy();
    CacheRoot::Select("zhCN", 'zhCN'); cache.Load(); CheckItem(cache, 501, name, 13); cache.Destroy();
    auto invalid = expected; invalid[4] ^= 1; Write(itemPath, invalid); cache.Load();
    Require(!cache.m_entries.m_fulllist.Head(), "Wrong-build cache was accepted"); cache.m_loaded = false;
    invalid = expected; invalid.resize(34); Write(itemPath, invalid); cache.Load();
    Require(!cache.m_entries.m_fulllist.Head(), "Truncated cache payload was accepted"); cache.m_loaded = false;

    int32_t timestamp = 100, updated = 0;
    auto* addon = RegisterBannedAddOn(777, &timestamp, &updated);
    Require(addon && updated, "Cannot register addon cache fixture");
    addon->m_id = 777; addon->m_lastModified = timestamp; addon->m_flags = 1;
    CC_MD5("AppleCacheFixture", 17, addon->m_nameMd5); CC_MD5("1.2", 3, addon->m_versionMd5);
    Require(SaveBannedAddOns(), "Original addon cache save failed");
    const auto addonPath = scope.root / "WDB/zhCN/baddons.wcf";
    std::vector<uint8_t> expectedAddon; Append(expectedAddon, 777);
    expectedAddon.insert(expectedAddon.end(), addon->m_nameMd5, addon->m_nameMd5 + 16);
    expectedAddon.insert(expectedAddon.end(), addon->m_versionMd5, addon->m_versionMd5 + 16);
    Append(expectedAddon, timestamp); Append(expectedAddon, 1);
    Require(Read(addonPath) == expectedAddon, "Original addon cache record bytes changed");
    DestroyBannedAddOns(); Require(LoadBannedAddOns(), "Original addon cache reload failed");
    addon = FindBannedAddOn("AppleCacheFixture", "1.2");
    Require(addon && addon->m_id == 777 && addon->m_lastModified == 100 && !FindBannedAddOn("AppleCacheFixture", "1.3"), "Reloaded addon cache lookup changed");
    for (size_t length = 1; length < expectedAddon.size(); ++length) {
        DestroyBannedAddOns(); invalid.assign(expectedAddon.begin(), expectedAddon.begin() + length); Write(addonPath, invalid);
        Require(!LoadBannedAddOns() && !FindBannedAddOn("AppleCacheFixture", "1.2"), "Truncated addon cache published a partial record");
    }

    setenv("WOW_CACHE_ROOT", "relative-cache", 1); Require(!GetCacheDirectory(path) && !path[0], "Relative cache override was accepted");
    const std::string tooLong = scope.root.string() + "/" + std::string(260, 'x');
    setenv("WOW_CACHE_ROOT", tooLong.c_str(), 1); Require(!GetCacheDirectory(path), "Overlong native cache root was truncated");
    const auto fileRoot = scope.root / "ordinary-file"; Write(fileRoot, {1}); setenv("WOW_CACHE_ROOT", fileRoot.c_str(), 1);
    Require(!GetCacheDirectory(path), "Invalid cache directory silently fell back elsewhere");
    setenv("WOW_CACHE_ROOT", scope.root.c_str(), 1);
    Require(!GetCacheFilePath(path, 8, "itemnamecache.wdb"), "Cache filename truncation was accepted");
    std::fprintf(stderr, "Original writable cache: exact WDB/addon bytes, reload, temporary records, version invalidation, language partitions, corruption and invalid roots passed\n");
}
