#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <functional>

struct lua_State;

namespace ModCore
{
    class ModManager
    {
    public:
        static ModManager& Get();

        void Initialize();
        void Shutdown();

        // Lifecycle dispatches
        void OnEngineInit();
        void OnEngineShutdown();
        void OnGameInitialize();
        void OnGameDestroy();
        void OnFrameTick(float dt = 0.0f);

        // World / Map dispatches
        void OnEnterWorld();
        void OnLeaveWorld();
        void OnMapChange(const char* mapName, uint32_t zoneId);
        void OnWorldUpdate(float dt = 0.0f);
        void OnWorldRender(void* worldFrame = nullptr);
        void OnWorldHitTest(uint64_t guid, const void* point, bool hitObject);
        bool OnTerrainTrack(const void* point);
        bool OnTerrainClick(int32_t button, float x, float y, float z);

        // World hit test state
        uint64_t GetLastHitGuid() const { return m_lastHitGuid; }
        void GetLastHitPoint(float& x, float& y, float& z) const
        {
            x = m_lastHitPointX;
            y = m_lastHitPointY;
            z = m_lastHitPointZ;
        }
        float GetLastHitPointX() const { return m_lastHitPointX; }
        float GetLastHitPointY() const { return m_lastHitPointY; }
        float GetLastHitPointZ() const { return m_lastHitPointZ; }

        // Ground targeting cursor bypass
        void SetCastAtCursor(bool enable) { m_castAtCursor = enable; }
        bool IsCastAtCursor() const { return m_castAtCursor; }

        // Glue / UI dispatches
        void OnGlueResume();
        void OnGlueIdle();
        void OnGlueScreenChange(const char* screenName);
        bool IsLoginBlocked();
        void OnCharacterSelect(int32_t charIndex);
        void OnCharacterCreate(const char* name, uint8_t race, uint8_t classId, uint8_t gender);

        // Network dispatches
        bool OnPacketReceived(uint32_t opcode, uint32_t time, void* dataStore);
        bool OnPacketSend(uint32_t opcode, uint32_t size, void* dataStore);
        void OnDisconnect();

        // Spell dispatches
        bool OnSpellQuery(const char* queryType, uint32_t spellId, int32_t* outResult);
        void OnSpellCast(uint32_t spellId, uint64_t casterGuid, uint64_t targetGuid);
        void OnSpellPacket(uint32_t opcode, void* msg);
        void OnChannelStart(void* msg);
        void OnChannelUpdate(void* msg);
        void OnChannelStop();
        void OnSpellDestroy();

        // Tooltip dispatches
        bool OnTooltipEvaluate(uint32_t spellId, void* caster, const char* defaultText, std::string& outText);
        void OnTooltipSetSpell(void* tooltip, uint32_t spellId);
        void OnTooltipSetItem(void* tooltip, uint32_t itemId);
        void OnTooltipSetUnit(void* tooltip, uint64_t unitGuid);

        // ObjectMgr dispatches
        void OnObjectCreate(uint64_t guid, uint32_t typeId, void* objectPtr);
        void OnObjectDestroy(uint64_t guid, uint32_t typeId, void* objectPtr);
        void OnTargetChanged(uint64_t oldGuid, uint64_t newGuid);

        // Input dispatches
        bool OnKeyDown(uint32_t key, uint32_t modifiers, const char* keyString);
        bool OnKeyUp(uint32_t key, uint32_t modifiers, const char* keyString);
        bool OnMouseDown(int32_t button, int32_t x, int32_t y);
        bool OnMouseUp(int32_t button, int32_t x, int32_t y);
        bool OnMouseWheel(int32_t delta);

        // Chat dispatches
        bool OnChatMessage(int32_t type, const char* sender, const char* message, const char* channel);
        bool OnSendChatMessage(const char* message, int32_t type, int32_t language, const char* target);

        // Environment & Weather dispatches
        void OnWeatherChange(uint32_t type, float intensity);
        void OnZoneChange(uint32_t areaId, uint32_t subzoneId, const char* zoneName);

        // Loot dispatches
        void OnLootOpen(uint64_t guid, int32_t coins, uint32_t lootType);
        void OnLootSlot(uint32_t slot);
        void OnLootClose();

        // Merchant dispatches
        bool OnMerchantBuy(int32_t index, int32_t count);

        // Quest & Gossip dispatches
        bool OnQuestAccept(uint64_t questGiverGuid);
        bool OnQuestComplete(uint64_t questGiverGuid);
        bool OnQuestReward(int32_t rewardIndex);
        bool OnGossipSelect(int32_t optionIndex);

        // Action & Macro dispatches
        bool OnActionUse(int32_t slot);
        bool OnMacroExecute(const char* name, const char* body);

        // Cooldown dispatches
        void OnCooldownSet(const char* name, float start, float duration);

        // Sound & Music dispatches
        bool OnPlaySound(int32_t soundKitId);
        bool OnPlayMusic(const char* musicFile);

        // Item & Container dispatches
        bool OnItemUse(int32_t bagIndex, int32_t slotIndex);

        // Trade dispatches
        void OnTradeInitiate(uint64_t targetGuid, int32_t useCursorItem);
        void OnTradeAccept();

        // Combat Feedback / Floating Text dispatches
        bool OnCombatFeedback(uint64_t guid, int32_t amount, uint32_t schoolMask, uint32_t flags);

        // Camera dispatches
        void OnCameraUpdate(float x, float y, float z, float fov);

        // Nameplate dispatches
        void OnNamePlateCreate(uint64_t unitGuid, void* namePlatePtr);

        // Simplified generic hook dispatchers
        bool DispatchHook(const char* hookName);
        bool DispatchHookEx(const char* hookName, 
                            std::function<int(lua_State*)> pushArgs = nullptr, 
                            std::function<void(lua_State*, int)> readResult = nullptr);

        // Native script registration
        void RegisterLuaBindings(lua_State* L);

        // Mod discovery and mounting
        void DiscoverAndMountMods();
        bool MountMod(const std::filesystem::path& modDir);
        bool ReloadScript(const std::filesystem::path& scriptPath);

        const std::filesystem::path& GetBasePath() const { return m_modsPath; }
        lua_State* GetLuaState() const { return m_L; }

    private:
        ModManager() = default;
        ~ModManager();

        void LoadConfig();
        std::filesystem::path ResolveModsDirectory();

        std::filesystem::path m_modsPath;
        lua_State* m_L = nullptr;
        bool m_initialized = false;
        bool m_devMode = true;
        bool m_cacheEnabled = true;
        bool m_hotReloadEnabled = true;
        bool m_aiExportOnBoot = true;
        bool m_castAtCursor = false;
        uint64_t m_lastHitGuid = 0;
        float m_lastHitPointX = 0.0f;
        float m_lastHitPointY = 0.0f;
        float m_lastHitPointZ = 0.0f;
    };

    // Concise singleton accessor
    inline ModManager& Manager()
    {
        return ModManager::Get();
    }
}
