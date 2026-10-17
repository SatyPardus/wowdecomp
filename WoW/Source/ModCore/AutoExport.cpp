#include "AutoExport.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"

namespace ModCore
{
    AutoExport& AutoExport::Get()
    {
        static AutoExport instance;
        return instance;
    }

    AutoExport::AutoExport()
    {
        PopulateDefaultMetadata();
    }

    void AutoExport::Initialize(const std::filesystem::path& apiDir)
    {
        m_apiDir = apiDir;
        if (!std::filesystem::exists(m_apiDir))
        {
            std::error_code ec;
            std::filesystem::create_directories(m_apiDir, ec);
        }
    }

    void AutoExport::RegisterHook(const HookDefinition& hook)
    {
        for (auto& existing : m_hooks)
        {
            if (existing.name == hook.name)
            {
                if (!hook.params.empty())
                    existing.params = hook.params;
                if (!hook.description.empty() && existing.description.empty())
                    existing.description = hook.description;
                if (!hook.category.empty() && hook.category != "Custom")
                    existing.category = hook.category;
                if (!hook.returnType.empty() && hook.returnType != "any?" && hook.returnType != "void")
                    existing.returnType = hook.returnType;
                if (!hook.returnDoc.empty())
                    existing.returnDoc = hook.returnDoc;
                return;
            }
        }
        m_hooks.push_back(hook);
    }

    void AutoExport::RegisterClass(const ClassDefinition& cls)
    {
        for (auto& existing : m_classes)
        {
            if (existing.name == cls.name)
            {
                if (!cls.methods.empty())
                    existing.methods = cls.methods;
                if (!cls.fields.empty())
                    existing.fields = cls.fields;
                if (!cls.description.empty() && existing.description.empty())
                    existing.description = cls.description;
                if (!cls.parent.empty())
                    existing.parent = cls.parent;
                return;
            }
        }
        m_classes.push_back(cls);
    }

    void AutoExport::RegisterOpcode(const OpcodeDefinition& op)
    {
        for (auto& existing : m_opcodes)
        {
            if (existing.name == op.name || existing.opcode == op.opcode)
            {
                if (!op.description.empty() && existing.description.empty())
                    existing.description = op.description;
                if (!op.direction.empty())
                    existing.direction = op.direction;
                if (existing.name.empty())
                    existing.name = op.name;
                if (existing.opcode == 0)
                    existing.opcode = op.opcode;
                return;
            }
        }
        m_opcodes.push_back(op);
    }

    void AutoExport::PopulateDefaultMetadata()
    {
        // 1. Engine & Game Lifecycle Hooks
        RegisterHook({
            "OnEngineInit", "Lifecycle",
            "Fired when the client engine completes low-level initialization",
            {}, "void", ""
        });
        RegisterHook({
            "OnEngineShutdown", "Lifecycle",
            "Fired when the client engine begins shutdown sequence",
            {}, "void", ""
        });
        RegisterHook({
            "OnGameInitialize", "Lifecycle",
            "Fired when CGGameUI initializes the active gameplay world session",
            {}, "void", ""
        });
        RegisterHook({
            "OnGameDestroy", "Lifecycle",
            "Fired when CGGameUI shuts down the active gameplay world session",
            {}, "void", ""
        });
        RegisterHook({
            "OnFrameTick", "Lifecycle",
            "Fired every main engine loop iteration with frame delta time",
            {{"dt", "number", "Delta time in seconds"}}, "void", ""
        });

        // 2. World & Map Lifecycle Hooks
        RegisterHook({
            "OnEnterWorld", "World",
            "Fired when the player character finishes loading and enters the 3D world",
            {}, "void", ""
        });
        RegisterHook({
            "OnLeaveWorld", "World",
            "Fired when the player character leaves the 3D world (logout or disconnect)",
            {}, "void", ""
        });
        RegisterHook({
            "OnMapChange", "World",
            "Fired when CWorld loads or changes the active map",
            {{"mapName", "string", "Internal map directory name"}, {"zoneId", "integer", "Zone identifier"}},
            "void", ""
        });
        RegisterHook({
            "OnWorldUpdate", "World",
            "Fired every game tick during world update loop (ideal for visual updates and animation)",
            {{"dt", "number", "Delta time in seconds"}}, "void", ""
        });
        RegisterHook({
            "OnWorldRender", "World",
            "Fired each frame during the 3D world rendering pass (for decals, ribbons, gizmos)",
            {{"worldFrame", "any", "Pointer to CGWorldFrame instance"}}, "void", ""
        });
        RegisterHook({
            "OnWorldHitTest", "World",
            "Fired when the mouse raycast hits an entity or world surface",
            {{"guid", "number", "64-bit GUID of hit entity"}, {"point", "any", "3D hit coordinate"}, {"hitObject", "boolean", "True if entity hit"}},
            "void", ""
        });
        RegisterHook({
            "OnTerrainTrack", "World",
            "Fired during mouse terrain tracking queries",
            {{"point", "any", "3D terrain coordinate"}}, "boolean?", "Return true to consume tracking"
        });
        RegisterHook({
            "OnTerrainClick", "World",
            "Fired when user clicks on terrain ground",
            {{"button", "integer", "Mouse button index"}, {"x", "number", "World X"}, {"y", "number", "World Y"}, {"z", "number", "World Z"}},
            "boolean?", "Return true to consume click"
        });

        // 3. Glue & Login Hooks
        RegisterHook({
            "OnGlueResume", "Glue",
            "Fired when GlueXML screen resumes execution",
            {}, "void", ""
        });
        RegisterHook({
            "OnGlueIdle", "Glue",
            "Fired each frame while on GlueXML screens",
            {}, "void", ""
        });
        RegisterHook({
            "OnGlueScreenChange", "Glue",
            "Fired when transitioning between Glue screens (login, charselect, charcreate)",
            {{"screenName", "string", "Target screen name"}}, "void", ""
        });
        RegisterHook({
            "IsLoginBlocked", "Glue",
            "Queried by client before allowing user to login to realm",
            {}, "boolean?", "Return true to block login"
        });
        RegisterHook({
            "OnCharacterSelect", "Glue",
            "Fired when a character is selected on the character selection screen",
            {{"charIndex", "integer", "Character slot index"}}, "void", ""
        });
        RegisterHook({
            "OnCharacterCreate", "Glue",
            "Fired when a character creation request is submitted",
            {{"name", "string", "Character name"}, {"race", "integer", "Race ID"}, {"classId", "integer", "Class ID"}, {"gender", "integer", "Gender ID"}},
            "void", ""
        });

        // 4. Network Hooks
        RegisterHook({
            "OnPacketReceived", "Network",
            "Fired when a network packet arrives from the server",
            {{"opcode", "integer", "Network opcode"}, {"time", "integer", "Server packet timestamp"}, {"reader", "any", "Packet reader buffer"}},
            "boolean?", "Return true to prevent default engine handling"
        });
        RegisterHook({
            "OnPacketSend", "Network",
            "Fired before a packet is transmitted to the server",
            {{"opcode", "integer", "Network opcode"}, {"size", "integer", "Byte length"}, {"dataStore", "any", "Data buffer"}},
            "boolean?", "Return true to block transmission"
        });
        RegisterHook({
            "OnDisconnect", "Network",
            "Fired when the client network connection disconnects",
            {}, "void", ""
        });

        // 5. Spell Hooks
        RegisterHook({
            "OnSpellQuery", "Spells",
            "Fired when client queries dynamic spell attributes (PowerCost, IsPassive, Range, CastTime)",
            {{"queryType", "string", "Query attribute name"}, {"spellId", "integer", "Spell ID"}, {"caster", "any", "Caster entity pointer"}},
            "any?", "Return override value or nil for stock DBC calculation"
        });
        RegisterHook({
            "OnSpellCast", "Spells",
            "Fired when a spell cast begins",
            {{"spellId", "integer", "Spell ID"}, {"casterGuid", "number", "Caster GUID"}, {"targetGuid", "number", "Target GUID"}},
            "void", ""
        });
        RegisterHook({
            "OnChannelStart", "Spells",
            "Fired when a channeled spell begins casting",
            {{"msg", "any", "Channel start data packet"}}, "void", ""
        });
        RegisterHook({
            "OnChannelUpdate", "Spells",
            "Fired on channeled spell duration updates",
            {{"msg", "any", "Channel update data packet"}}, "void", ""
        });
        RegisterHook({
            "OnChannelStop", "Spells",
            "Fired when a channeled spell finishes or is interrupted",
            {}, "void", ""
        });
        RegisterHook({
            "OnSpellDestroy", "Spells",
            "Fired when a client spell instance is destroyed",
            {}, "void", ""
        });

        // 6. Tooltip Hooks
        RegisterHook({
            "OnTooltipEvaluate", "Tooltips",
            "Fired when spell tooltip description text is rendered for dynamic formulas and scaling",
            {{"spellId", "integer", "Spell ID"}, {"caster", "any", "Caster unit"}, {"defaultText", "string", "Parsed DBC tooltip string"}},
            "string?", "Return formatted string to override tooltip text or nil"
        });
        RegisterHook({
            "OnTooltipSetSpell", "Tooltips",
            "Fired when CGTooltip::SetSpell populates a spell tooltip",
            {{"tooltip", "any", "CGTooltip pointer"}, {"spellId", "integer", "Spell ID"}}, "void", ""
        });
        RegisterHook({
            "OnTooltipSetItem", "Tooltips",
            "Fired when CGTooltip::SetItem populates an item tooltip",
            {{"tooltip", "any", "CGTooltip pointer"}, {"itemId", "integer", "Item ID"}}, "void", ""
        });
        RegisterHook({
            "OnTooltipSetUnit", "Tooltips",
            "Fired when CGTooltip::SetUnit populates a unit tooltip",
            {{"tooltip", "any", "CGTooltip pointer"}, {"unitGuid", "number", "Unit GUID"}}, "void", ""
        });

        // 7. Object Manager & Entity Hooks
        RegisterHook({
            "OnObjectCreate", "Objects",
            "Fired when an entity is spawned in the client object manager",
            {{"guid", "number", "Entity GUID"}, {"typeId", "integer", "Object type ID"}, {"objectPtr", "any", "CGObject_C pointer"}},
            "void", ""
        });
        RegisterHook({
            "OnObjectDestroy", "Objects",
            "Fired when an entity is despawned or deleted from client memory",
            {{"guid", "number", "Entity GUID"}, {"typeId", "integer", "Object type ID"}, {"objectPtr", "any", "CGObject_C pointer"}},
            "void", ""
        });
        RegisterHook({
            "OnTargetChanged", "Objects",
            "Fired when the active player changes target entity",
            {{"oldGuid", "number", "Previous target GUID"}, {"newGuid", "number", "New target GUID"}},
            "void", ""
        });

        // 8. Input Hooks
        RegisterHook({
            "OnKeyDown", "Input",
            "Fired on keyboard key down events",
            {{"key", "integer", "Engine key code"}, {"modifiers", "integer", "Shift/Ctrl/Alt mask"}, {"keyString", "string", "Key name"}},
            "boolean?", "Return true to consume input"
        });
        RegisterHook({
            "OnKeyUp", "Input",
            "Fired on keyboard key up events",
            {{"key", "integer", "Engine key code"}, {"modifiers", "integer", "Shift/Ctrl/Alt mask"}, {"keyString", "string", "Key name"}},
            "boolean?", "Return true to consume input"
        });
        RegisterHook({
            "OnMouseDown", "Input",
            "Fired on mouse button press",
            {{"button", "integer", "Button index (0=Left, 1=Right, 2=Middle)"}, {"x", "integer", "Screen X"}, {"y", "integer", "Screen Y"}},
            "boolean?", "Return true to consume input"
        });
        RegisterHook({
            "OnMouseUp", "Input",
            "Fired on mouse button release",
            {{"button", "integer", "Button index (0=Left, 1=Right, 2=Middle)"}, {"x", "integer", "Screen X"}, {"y", "integer", "Screen Y"}},
            "boolean?", "Return true to consume input"
        });
        RegisterHook({
            "OnMouseWheel", "Input",
            "Fired on mouse wheel scroll",
            {{"delta", "integer", "Scroll ticks (-1 or +1)"}},
            "boolean?", "Return true to consume input"
        });

        // 9. Chat Hooks
        RegisterHook({
            "OnChatMessage", "Chat",
            "Fired when an incoming chat message is received from server",
            {{"type", "integer", "Chat type ID"}, {"sender", "string", "Sender character name"}, {"message", "string", "Message text"}, {"channel", "string", "Channel name"}},
            "boolean?", "Return true to suppress chat message"
        });
        RegisterHook({
            "OnSendChatMessage", "Chat",
            "Fired before a chat message is sent from the client",
            {{"message", "string", "Message text"}, {"type", "integer", "Chat type"}, {"language", "integer", "Language ID"}, {"target", "string", "Whisper/channel target"}},
            "boolean?", "Return true to block sending"
        });

        // 10. Environment & Weather Hooks
        RegisterHook({
            "OnWeatherChange", "Environment",
            "Fired when weather type or intensity changes in active zone",
            {{"type", "integer", "Weather type ID"}, {"intensity", "number", "Weather intensity (0.0 to 1.0)"}},
            "void", ""
        });
        RegisterHook({
            "OnZoneChange", "World",
            "Fired when player crosses zone or subzone boundary",
            {{"areaId", "integer", "Area ID"}, {"subzoneId", "integer", "Subzone ID"}, {"zoneName", "string", "Localized or internal zone name"}},
            "void", ""
        });

        // 11. Loot Hooks
        RegisterHook({
            "OnLootOpen", "Loot",
            "Fired when loot window opens for an entity or container",
            {{"guid", "number", "Loot target entity GUID"}, {"coins", "integer", "Dropped coin amount in copper"}, {"lootType", "integer", "Loot type classification"}},
            "void", ""
        });
        RegisterHook({
            "OnLootSlot", "Loot",
            "Fired when an item slot is looted from the active loot window",
            {{"slot", "integer", "Loot window slot index (1-based)"}},
            "void", ""
        });
        RegisterHook({
            "OnLootClose", "Loot",
            "Fired when active loot window is closed",
            {}, "void", ""
        });

        // 12. Merchant Hooks
        RegisterHook({
            "OnMerchantBuy", "Merchant",
            "Fired when buying an item from an active merchant",
            {{"index", "integer", "Merchant item slot index"}, {"count", "integer", "Number of items to purchase"}},
            "boolean?", "Return true to cancel or prevent purchase"
        });

        // 13. Quest & Gossip Hooks
        RegisterHook({
            "OnQuestAccept", "Quests",
            "Fired when accepting a quest from a quest giver",
            {{"questGiverGuid", "number", "Quest giver entity GUID"}},
            "boolean?", "Return true to block accepting quest"
        });
        RegisterHook({
            "OnQuestComplete", "Quests",
            "Fired when completing a quest with a quest giver",
            {{"questGiverGuid", "number", "Quest giver entity GUID"}},
            "boolean?", "Return true to block completing quest"
        });
        RegisterHook({
            "OnQuestReward", "Quests",
            "Fired when selecting and claiming a quest reward",
            {{"rewardIndex", "integer", "Chosen reward item index"}},
            "boolean?", "Return true to block claiming reward"
        });
        RegisterHook({
            "OnGossipSelect", "Quests",
            "Fired when selecting an option in NPC gossip dialog",
            {{"optionIndex", "integer", "Gossip option index"}},
            "boolean?", "Return true to block gossip option execution"
        });

        // 14. Action & Macro Hooks
        RegisterHook({
            "OnActionUse", "ActionBar",
            "Fired when an action bar button is activated",
            {{"slot", "integer", "Action bar slot index (1-120)"}},
            "boolean?", "Return true to prevent default action execution"
        });
        RegisterHook({
            "OnMacroExecute", "ActionBar",
            "Fired before executing macro commands or text",
            {{"name", "string", "Macro name or script identifier"}, {"body", "string", "Macro command text lines"}},
            "boolean?", "Return true to block macro execution"
        });

        // 15. Cooldown Hooks
        RegisterHook({
            "OnCooldownSet", "Cooldowns",
            "Fired when a spell or item cooldown is initialized or refreshed",
            {{"name", "string", "Cooldown identifier or spell name"}, {"start", "number", "Cooldown start timestamp in seconds"}, {"duration", "number", "Cooldown duration in seconds"}},
            "void", ""
        });

        // 16. Sound & Music Hooks
        RegisterHook({
            "OnPlaySound", "Sound",
            "Fired when a sound or sound kit is requested to play",
            {{"soundKitId", "integer", "Sound kit ID from SoundEntries DBC"}},
            "boolean?", "Return true to mute or cancel sound playback"
        });
        RegisterHook({
            "OnPlayMusic", "Sound",
            "Fired when a music track or file is requested to play",
            {{"musicFile", "string", "Music asset path or file name"}},
            "boolean?", "Return true to cancel or override music playback"
        });

        // 17. Item Hooks
        RegisterHook({
            "OnItemUse", "Items",
            "Fired when an item in a container or bag is used or activated",
            {{"bagIndex", "integer", "Bag container index (0-4)"}, {"slotIndex", "integer", "Bag slot index"}},
            "boolean?", "Return true to prevent item usage"
        });

        // 18. Trade Hooks
        RegisterHook({
            "OnTradeInitiate", "Trade",
            "Fired when a player-to-player trade session is initiated",
            {{"targetGuid", "number", "Trade target character GUID"}, {"useCursorItem", "boolean", "True if initiated with an item on the cursor"}},
            "void", ""
        });
        RegisterHook({
            "OnTradeAccept", "Trade",
            "Fired when player clicks Accept in the trade window",
            {}, "void", ""
        });

        // 19. Combat Feedback Hooks
        RegisterHook({
            "OnCombatFeedback", "Combat",
            "Fired on combat feedback / floating combat text events (damage, heals, misses)",
            {{"guid", "number", "Target entity GUID"}, {"amount", "integer", "Feedback amount or value"}, {"schoolMask", "integer", "Damage school bitmask"}, {"flags", "integer", "Combat feedback flags"}},
            "boolean?", "Return true to suppress floating combat text"
        });

        // 20. Camera Hooks
        RegisterHook({
            "OnCameraUpdate", "Camera",
            "Fired every frame when active world camera updates position and FOV",
            {{"x", "number", "Camera World X position"}, {"y", "number", "Camera World Y position"}, {"z", "number", "Camera World Z position"}, {"fov", "number", "Field of View in radians"}},
            "void", ""
        });

        // 21. UI & Nameplate Hooks
        RegisterHook({
            "OnNamePlateCreate", "UI",
            "Fired when a nameplate frame is created or initialized for a unit",
            {{"unitGuid", "number", "Attached unit GUID"}, {"namePlatePtr", "any", "Pointer to CGNamePlateFrame instance"}},
            "void", ""
        });

        // 22. Core Classes
        RegisterClass({
            "ModCore", "", "Master modding framework runtime controller",
            {
                {"Version", "string", "Framework version string"},
                {"BasePath", "string", "Absolute root directory of mods"},
                {"LoadedMods", "table<string, table>", "Dictionary of mounted mod descriptors"}
            },
            {
                {"LogDebug", "Logs a debug message to client console", {{"fmt", "string", "Format string"}, {"...", "any", "Arguments"}}, "void"},
                {"LogInfo", "Logs an info message to client console", {{"fmt", "string", "Format string"}, {"...", "any", "Arguments"}}, "void"},
                {"LogWarn", "Logs a warning message to client console", {{"fmt", "string", "Format string"}, {"...", "any", "Arguments"}}, "void"},
                {"LogError", "Logs an error message to client console", {{"fmt", "string", "Format string"}, {"...", "any", "Arguments"}}, "void"},
                {"MountMod", "Mounts and initializes an external mod package", {{"modDir", "string", "Path"}, {"manifest", "table", "Manifest"}}, "boolean"},
                {"RegisterHook", "Registers a callback listener for an engine or gameplay hook", {{"hookName", "string", "Hook name"}, {"callback", "fun(...): any?", "Listener"}, {"priority?", "integer", "Priority (default 100)"}, {"doc?", "string", "Documentation"}}, "void"},
                {"UnregisterHook", "Unregisters a callback from a hook", {{"hookName", "string", "Hook name"}, {"callback", "fun(...)", "Listener"}}, "void"},
                {"FireHook", "Fires a hook across all listeners and returns early override if non-nil", {{"hookName", "string", "Hook name"}, {"...", "any", "Parameters"}}, "any?"},
                {"On", "Event emitter syntax alias for RegisterHook", {{"hookName", "string", "Hook name"}, {"callback", "fun(...): any?", "Listener"}, {"priority?", "integer", "Priority"}, {"doc?", "string", "Documentation"}}, "void"},
                {"DispatchHook", "Direct hook dispatcher invoked by C++ engine", {{"hookName", "string", "Hook name"}, {"...", "any", "Parameters"}}, "any?"},
                {"GetHookList", "Returns array of all registered hook names and metadata", {}, "table"},
                {"GetApiSchema", "Returns complete machine schema for frontend extension", {}, "table"},
                {"ExportApi", "Triggers immediate autogeneration of all API definitions and schemas", {}, "boolean"},
                {"SetCastAtCursor", "Arms the engine cursor tracker for ground-targeted spell casting", {{"enable", "boolean", "Enable/disable"}}, "void"},
                {"IsCastAtCursor", "Checks if cast-at-cursor mode is active", {}, "boolean"},
                {"ClickTerrain", "Sends a simulated terrain click event at specified 3D world coordinates", {{"x?", "number", "X coordinate"}, {"y?", "number", "Y coordinate"}, {"z?", "number", "Z coordinate"}}, "void"},
                {"ClickSelfTerrain", "Sends a simulated terrain click event at the active player's position", {}, "void"},
                {"GetLastHitGuid", "Returns the GUID string and numeric value under cursor from last world hit test", {}, "string, number"},
                {"GetLastHitPoint", "Returns the 3D world coordinates under cursor from last world hit test", {}, "number, number, number"}
            }
        });

        RegisterClass({
            "Hooks", "", "Event and lifecycle dispatch bus",
            {},
            {
                {"Register", "Registers a callback for an event", {{"hookName", "string", "Hook name"}, {"callback", "fun(...)", "Function"}, {"priority?", "integer", "Priority"}, {"doc?", "string", "Doc"}}, "void"},
                {"Unregister", "Unregisters a callback", {{"hookName", "string", "Hook name"}, {"callback", "fun(...)", "Function"}}, "void"},
                {"Fire", "Fires hook across listeners", {{"hookName", "string", "Hook name"}, {"...", "any", "Args"}}, "any?"},
                {"HasListeners", "Returns true if any callbacks are registered for hook", {{"hookName", "string", "Hook name"}}, "boolean"},
                {"GetMetadata", "Returns dictionary of hook docstrings", {}, "table<string, string>"},
                {"GetHookList", "Returns list of all active hook names", {}, "table<integer, string>"}
            }
        });

        RegisterClass({
            "Net", "", "Declarative packet serialization and opcode router",
            {},
            {
                {"DefinePacket", "Defines a typed binary packet schema", {{"name", "string", "Packet identifier"}, {"definition", "table", "Opcode and schema"}}, "void"},
                {"Send", "Constructs and transmits a binary packet to server", {{"name", "string", "Packet identifier"}, {"data", "table", "Payload"}}, "boolean"},
                {"OnCustomPacket", "Registers a callback for a specific network opcode", {{"opcode", "integer", "Network opcode"}, {"handler", "fun(reader: any)", "Handler"}}, "void"}
            }
        });

        RegisterClass({
            "DbcEngine", "", "Virtual DBC table and row injector",
            {},
            {
                {"GetTable", "Retrieves a table handle for a loaded DBC", {{"tableName", "string", "DBC name"}}, "table"},
                {"RegisterVirtualRow", "Injects an in-memory virtual record into DBC", {{"tableName", "string", "DBC name"}, {"id", "integer", "Record ID"}, {"data", "table", "Record fields"}}, "void"},
                {"OverrideRow", "Overrides specific fields in an existing DBC record", {{"tableName", "string", "DBC name"}, {"id", "integer", "Record ID"}, {"overrides", "table", "Field overrides"}}, "void"},
                {"OnRowLookup", "Intercepts runtime DBC row queries", {{"tableName", "string", "DBC name"}, {"callback", "fun(id: integer): table?", "Handler"}}, "void"}
            }
        });

        RegisterClass({
            "Visuals", "", "Combat decals, telegraphs, and ribbon trail manager",
            {},
            {
                {"CreateDecal", "Emits a ground decal (circle, donut, cone, rect)", {{"params", "table", "Decal parameters"}}, "table"},
                {"CreateRibbonTrail", "Attaches a weapon ribbon trail to player/unit model", {{"params", "table", "Trail parameters"}}, "table"}
            }
        });

        RegisterClass({
            "C3Vector", "", "3D coordinate vector in World space",
            {
                {"x", "number", "X coordinate"},
                {"y", "number", "Y coordinate"},
                {"z", "number", "Z coordinate"}
            },
            {}
        });

        RegisterClass({
            "Quat", "", "4D quaternion for 3D rotation",
            {
                {"w", "number", "Real component"},
                {"x", "number", "X imaginary component"},
                {"y", "number", "Y imaginary component"},
                {"z", "number", "Z imaginary component"}
            },
            {}
        });

        RegisterClass({
            "WOWGUID", "", "64-bit entity identifier in World of Warcraft",
            {
                {"low", "integer", "Low 32 bits"},
                {"high", "integer", "High 32 bits"}
            },
            {
                {"IsValid", "Returns true if GUID is non-zero", {}, "boolean"}
            }
        });

        RegisterClass({
            "CGObject_C", "", "Base entity in client object manager",
            {
                {"m_guid", "WOWGUID", "Entity GUID"}
            },
            {
                {"GetGUID", "Returns entity GUID", {}, "WOWGUID"},
                {"GetPosition", "Returns 3D world position", {}, "C3Vector"},
                {"IsValid", "Returns true if entity pointer is valid", {}, "boolean"}
            }
        });

        RegisterClass({
            "CGUnit_C", "CGObject_C", "Unit entity (player, NPC, pet)",
            {},
            {
                {"GetHealth", "Returns current health", {}, "integer"},
                {"GetMaxHealth", "Returns maximum health", {}, "integer"},
                {"GetPower", "Returns current power for type", {{"powerType", "integer", "Power type"}}, "integer"},
                {"GetMaxPower", "Returns maximum power for type", {{"powerType", "integer", "Power type"}}, "integer"},
                {"IsPlayer", "Returns true if unit is player character", {}, "boolean"},
                {"IsFriendly", "Returns true if unit is friendly to active player", {}, "boolean"},
                {"GetUnitName", "Returns unit name string", {}, "string"}
            }
        });

        RegisterClass({
            "CGPlayer_C", "CGUnit_C", "Player character entity",
            {},
            {
                {"GetTargetGuid", "Returns GUID of currently targeted entity", {}, "WOWGUID"}
            }
        });

        RegisterClass({
            "ClntObjMgr", "", "Client Object Manager query interface",
            {},
            {
                {"GetActivePlayer", "Returns the active player character entity", {}, "CGPlayer_C"},
                {"GetObject", "Queries an entity by its GUID", {{"guid", "WOWGUID", "Entity GUID"}}, "CGObject_C?"}
            }
        });

        // 11. Custom Packet Opcodes
        RegisterOpcode({"CMSG_CUSTOM_PACKET_TEST", 0x51f, "CLIENT_TO_SERVER", "Test custom client-to-server payload"});
        RegisterOpcode({"SMSG_CUSTOM_PACKET_TEST", 0x102, "SERVER_TO_CLIENT", "Test server-to-client payload"});
        RegisterOpcode({"CMSG_HOUSING_PLACE_OBJECT", 0x7001, "CLIENT_TO_SERVER", "Housing placement request"});
        RegisterOpcode({"SMSG_HOUSING_OBJECT_UPDATE", 0x7002, "SERVER_TO_CLIENT", "Housing object spawn/update"});
        RegisterOpcode({"SMSG_SPAWN_DANGER_ZONE", 0x7101, "SERVER_TO_CLIENT", "Boss encounter telegraph decal"});
    }

    void AutoExport::ImportLuaMetadata(lua_State* L)
    {
        if (!L) return;
        int top = lua_gettop(L);

        // Query ModCore.ApiExport if present
        lua_getglobal(L, "ModCore");
        if (lua_istable(L, -1))
        {
            lua_getfield(L, -1, "ApiExport");
            if (lua_istable(L, -1))
            {
                // Hooks from Lua
                lua_getfield(L, -1, "Hooks");
                if (lua_istable(L, -1))
                {
                    lua_pushnil(L);
                    while (lua_next(L, -2) != 0)
                    {
                        if (lua_istable(L, -1))
                        {
                            lua_getfield(L, -1, "name");
                            lua_getfield(L, -2, "doc");
                            lua_getfield(L, -3, "signature");

                            std::string name = lua_isstring(L, -3) ? lua_tostring(L, -3) : "";
                            std::string doc = lua_isstring(L, -2) ? lua_tostring(L, -2) : "";
                            std::string sig = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                            lua_pop(L, 3);

                            if (!name.empty())
                            {
                                std::string ret = "void";
                                size_t retPos = sig.find("): ");
                                if (retPos != std::string::npos)
                                {
                                    ret = sig.substr(retPos + 3);
                                }
                                RegisterHook({name, "Custom", doc, {}, ret, ""});
                            }
                        }
                        lua_pop(L, 1);
                    }
                }
                lua_pop(L, 1); // pop Hooks table

                // Opcodes from Lua
                lua_getfield(L, -1, "Opcodes");
                if (lua_istable(L, -1))
                {
                    lua_pushnil(L);
                    while (lua_next(L, -2) != 0)
                    {
                        if (lua_istable(L, -1))
                        {
                            lua_getfield(L, -1, "name");
                            lua_getfield(L, -2, "opcode");
                            lua_getfield(L, -3, "direction");

                            std::string name = lua_isstring(L, -3) ? lua_tostring(L, -3) : "";
                            uint32_t op = lua_isnumber(L, -2) ? static_cast<uint32_t>(lua_tointeger(L, -2)) : 0;
                            std::string dir = lua_isstring(L, -1) ? lua_tostring(L, -1) : "CLIENT_TO_SERVER";
                            lua_pop(L, 3);

                            if (!name.empty() && op != 0)
                            {
                                RegisterOpcode({name, op, dir, ""});
                            }
                        }
                        lua_pop(L, 1);
                    }
                }
                lua_pop(L, 1); // pop Opcodes table
            }
            lua_pop(L, 1); // pop ApiExport
        }
        lua_pop(L, 1); // pop ModCore

        lua_settop(L, top);
    }

    void AutoExport::ExportAll(lua_State* L)
    {
        if (m_apiDir.empty())
            return;

        if (L)
        {
            ImportLuaMetadata(L);
        }

        GenerateEmmyLua(m_apiDir / "wowdecomp.d.lua");
        GenerateJsonSchema(m_apiDir / "wowdecomp_api.json");
        GenerateAiGuide(m_apiDir / "AI_MODDING_GUIDE.md");

        std::cout << "[ModCore] Auto-export completed. API definitions written to " << m_apiDir.string() 
                  << " (" << m_hooks.size() << " hooks, " << m_classes.size() << " classes, " << m_opcodes.size() << " opcodes)" 
                  << std::endl;
    }

    void AutoExport::GenerateEmmyLua(const std::filesystem::path& outputPath)
    {
        std::ofstream out(outputPath);
        if (!out.is_open()) return;

        out << "---@meta wowdecomp\n";
        out << "-- ==============================================================================\n";
        out << "-- wowdecomp Universal Modding Framework Typed API Definitions (EmmyLua / LuaLS)\n";
        out << "-- Auto-generated by ModCore Reflection Engine\n";
        out << "-- ==============================================================================\n\n";

        // Hook Callback Type Aliases
        out << "-- ------------------------------------------------------------------------------\n";
        out << "-- Hook Callback Signatures\n";
        out << "-- ------------------------------------------------------------------------------\n\n";

        for (const auto& hook : m_hooks)
        {
            std::string paramTypes;
            for (size_t i = 0; i < hook.params.size(); ++i)
            {
                if (i > 0) paramTypes += ", ";
                paramTypes += hook.params[i].name + ": " + hook.params[i].type;
            }
            std::string ret = hook.returnType.empty() ? "void" : hook.returnType;
            out << "---@alias HookCallback_" << hook.name << " fun(" << paramTypes << "): " << ret << "\n";
        }
        out << "\n";

        // Classes & Methods
        for (const auto& cls : m_classes)
        {
            out << "-- ------------------------------------------------------------------------------\n";
            out << "-- " << cls.name << (cls.parent.empty() ? "" : " : " + cls.parent) << "\n";
            out << "-- ------------------------------------------------------------------------------\n";
            if (!cls.description.empty())
                out << "---" << cls.description << "\n";

            if (cls.parent.empty())
                out << "---@class " << cls.name << "\n";
            else
                out << "---@class " << cls.name << " : " << cls.parent << "\n";

            for (const auto& f : cls.fields)
            {
                out << "---@field " << f.name << " " << f.type << (f.doc.empty() ? "" : " " + f.doc) << "\n";
            }
            out << cls.name << " = {}\n\n";

            for (const auto& m : cls.methods)
            {
                if (!m.description.empty())
                    out << "---" << m.description << "\n";

                for (const auto& p : m.params)
                {
                    out << "---@param " << p.name << " " << p.type << (p.doc.empty() ? "" : " " + p.doc) << "\n";
                }
                if (!m.returnType.empty() && m.returnType != "void")
                {
                    out << "---@return " << m.returnType << "\n";
                }

                std::string paramList;
                for (size_t i = 0; i < m.params.size(); ++i)
                {
                    if (i > 0) paramList += ", ";
                    paramList += m.params[i].name;
                }

                out << "function " << cls.name << "." << m.name << "(" << paramList << ") end\n\n";
            }
        }

        // Overloaded Hook Registration Declarations for Autocomplete
        out << "-- ------------------------------------------------------------------------------\n";
        out << "-- Overloaded Hook Registration Autocomplete\n";
        out << "-- ------------------------------------------------------------------------------\n\n";

        for (const auto& hook : m_hooks)
        {
            out << "---Registers callback for `" << hook.name << "` (" << hook.category << ")\n";
            out << "---@param hookName '\"" << hook.name << "\"'\n";
            out << "---@param callback HookCallback_" << hook.name << "\n";
            out << "---@param priority? integer Higher priorities run first (default 100)\n";
            out << "---@param doc? string Optional documentation\n";
            out << "function ModCore.Hooks.Register(hookName, callback, priority, doc) end\n\n";

            out << "---Registers callback for `" << hook.name << "` (" << hook.category << ")\n";
            out << "---@param hookName '\"" << hook.name << "\"'\n";
            out << "---@param callback HookCallback_" << hook.name << "\n";
            out << "---@param priority? integer Higher priorities run first (default 100)\n";
            out << "---@param doc? string Optional documentation\n";
            out << "function ModCore.On(hookName, callback, priority, doc) end\n\n";
        }

        out << "return ModCore\n";
    }

    void AutoExport::GenerateJsonSchema(const std::filesystem::path& outputPath)
    {
        std::ofstream out(outputPath);
        if (!out.is_open()) return;

        out << "{\n";
        out << "  \"$schema\": \"http://json-schema.org/draft-07/schema#\",\n";
        out << "  \"title\": \"wowdecomp Universal Modding Framework API\",\n";
        out << "  \"version\": \"1.1.0\",\n";
        out << "  \"engine\": \"wowdecomp\",\n";

        // Modules
        out << "  \"modules\": {\n";
        for (size_t i = 0; i < m_classes.size(); ++i)
        {
            const auto& cls = m_classes[i];
            out << "    \"" << cls.name << "\": {\n";
            out << "      \"description\": \"" << cls.description << "\",\n";
            out << "      \"parent\": \"" << cls.parent << "\",\n";
            out << "      \"methods\": [\n";
            for (size_t j = 0; j < cls.methods.size(); ++j)
            {
                const auto& m = cls.methods[j];
                out << "        {\n";
                out << "          \"name\": \"" << m.name << "\",\n";
                out << "          \"description\": \"" << m.description << "\",\n";
                out << "          \"returnType\": \"" << m.returnType << "\",\n";
                out << "          \"params\": [\n";
                for (size_t k = 0; k < m.params.size(); ++k)
                {
                    const auto& p = m.params[k];
                    out << "            { \"name\": \"" << p.name << "\", \"type\": \"" << p.type << "\", \"description\": \"" << p.doc << "\" }"
                        << (k + 1 < m.params.size() ? "," : "") << "\n";
                }
                out << "          ]\n";
                out << "        }" << (j + 1 < cls.methods.size() ? "," : "") << "\n";
            }
            out << "      ]\n";
            out << "    }" << (i + 1 < m_classes.size() ? "," : "") << "\n";
        }
        out << "  },\n";

        // Hooks
        out << "  \"hooks\": {\n";
        for (size_t i = 0; i < m_hooks.size(); ++i)
        {
            const auto& h = m_hooks[i];
            out << "    \"" << h.name << "\": {\n";
            out << "      \"category\": \"" << h.category << "\",\n";
            out << "      \"description\": \"" << h.description << "\",\n";
            out << "      \"returnType\": \"" << (h.returnType.empty() ? "void" : h.returnType) << "\",\n";
            out << "      \"returnDoc\": \"" << h.returnDoc << "\",\n";
            out << "      \"params\": [\n";
            for (size_t k = 0; k < h.params.size(); ++k)
            {
                const auto& p = h.params[k];
                out << "        { \"name\": \"" << p.name << "\", \"type\": \"" << p.type << "\", \"description\": \"" << p.doc << "\" }"
                    << (k + 1 < h.params.size() ? "," : "") << "\n";
            }
            out << "      ]\n";
            out << "    }" << (i + 1 < m_hooks.size() ? "," : "") << "\n";
        }
        out << "  },\n";

        // Opcodes
        out << "  \"opcodes\": [\n";
        for (size_t i = 0; i < m_opcodes.size(); ++i)
        {
            const auto& op = m_opcodes[i];
            out << "    {\n";
            out << "      \"name\": \"" << op.name << "\",\n";
            out << "      \"opcode\": " << op.opcode << ",\n";
            out << "      \"opcodeHex\": \"0x" << std::hex << std::uppercase << op.opcode << std::dec << "\",\n";
            out << "      \"direction\": \"" << op.direction << "\",\n";
            out << "      \"description\": \"" << op.description << "\"\n";
            out << "    }" << (i + 1 < m_opcodes.size() ? "," : "") << "\n";
        }
        out << "  ]\n";
        out << "}\n";
    }

    void AutoExport::GenerateAiGuide(const std::filesystem::path& outputPath)
    {
        std::ofstream out(outputPath);
        if (!out.is_open()) return;

        out << "# wowdecomp Universal Modding Framework - AI & Developer Reference Guide\n\n";
        out << "> **Target Audience**: AI assistants (Antigravity, Claude, GPT) and human mod developers.\n";
        out << "> **Purpose**: Complete reference for extending `wowdecomp` client logic externally without recompiling C++.\n\n";
        out << "---\n\n";

        out << "## 1. Quick Start & Mod Structure\n\n";
        out << "Mods live in `mods/<mod_id>/` with a `mod.json` manifest and an `init.lua` entry point:\n\n";
        out << "```json\n";
        out << "{\n";
        out << "  \"id\": \"my-custom-mod\",\n";
        out << "  \"name\": \"My Custom Mod\",\n";
        out << "  \"version\": \"1.0.0\",\n";
        out << "  \"entryPoint\": \"init.lua\",\n";
        out << "  \"priority\": 100\n";
        out << "}\n";
        out << "```\n\n";

        out << "In `init.lua`:\n";
        out << "```lua\n";
        out << "local MyMod = {}\n";
        out << "function MyMod.Init(context)\n";
        out << "    ModCore.LogInfo(\"Initialized MyMod!\")\n";
        out << "    ModCore.On(\"OnWorldUpdate\", function(dt)\n";
        out << "        -- per-frame logic\n";
        out << "    end)\n";
        out << "end\n";
        out << "MyMod.Init(...)\n";
        out << "return MyMod\n";
        out << "```\n\n";

        out << "---\n\n";
        out << "## 2. Exhaustive Hook Catalog (" << m_hooks.size() << " Hooks)\n\n";
        out << "| Hook Name | Category | Signature | Description |\n";
        out << "| :--- | :--- | :--- | :--- |\n";

        for (const auto& h : m_hooks)
        {
            std::string paramTypes;
            for (size_t i = 0; i < h.params.size(); ++i)
            {
                if (i > 0) paramTypes += ", ";
                paramTypes += h.params[i].name + ": " + h.params[i].type;
            }
            std::string ret = h.returnType.empty() ? "void" : h.returnType;
            out << "| `" << h.name << "` | **" << h.category << "** | `fun(" << paramTypes << "): " << ret << "` | " << h.description << " |\n";
        }

        out << "\n---\n\n";
        out << "## 3. Key Subsystem Recipes\n\n";

        out << "### 3.1 Overriding Spell Attributes & Passives\n";
        out << "```lua\n";
        out << "ModCore.On(\"OnSpellQuery\", function(queryType, spellId, caster)\n";
        out << "    if queryType == \"IsPassive\" and spellId == 48266 then\n";
        out << "        return false -- Render Blood Presence on action bar even if passive in DBC\n";
        out << "    elseif queryType == \"PowerCost\" and spellId == 98100 then\n";
        out << "        return 35 -- Custom energy cost\n";
        out << "    end\n";
        out << "    return nil -- Fallback to stock DBC\n";
        out << "end)\n";
        out << "```\n\n";

        out << "### 3.2 Dynamic Tooltip Scaling Formulas\n";
        out << "```lua\n";
        out << "ModCore.On(\"OnTooltipEvaluate\", function(spellId, caster, defaultText)\n";
        out << "    if spellId == 98100 then\n";
        out << "        local ap = caster and caster:GetAttackPower() or 0\n";
        out << "        return string.format(\"Strike enemies for %.0f Shadowflame damage.\", 300 + ap * 0.45)\n";
        out << "    end\n";
        out << "    return nil\n";
        out << "end)\n";
        out << "```\n\n";

        out << "### 3.3 Intercepting Network Packets\n";
        out << "```lua\n";
        out << "local Net = require(\"system.net\")\n";
        out << "Net.DefinePacket(\"SMSG_CUSTOM_ALERT\", {\n";
        out << "    opcode = 0x7005,\n";
        out << "    direction = \"SERVER_TO_CLIENT\",\n";
        out << "    schema = { { name = \"message\", type = \"string\" } },\n";
        out << "    handler = function(packet)\n";
        out << "        ModCore.LogInfo(\"Alert from server: %s\", packet.message)\n";
        out << "    end\n";
        out << "})\n";
        out << "```\n\n";

        out << "### 3.4 Capturing Keyboard & Mouse Input\n";
        out << "```lua\n";
        out << "ModCore.On(\"OnKeyDown\", function(key, modifiers, keyString)\n";
        out << "    if keyString == \"H\" and (modifiers & 1) ~= 0 then -- Shift+H\n";
        out << "        ModCore.LogInfo(\"Toggled Custom UI Frame!\")\n";
        out << "        return true -- Consumes input\n";
        out << "    end\n";
        out << "end)\n";
        out << "```\n\n";

        out << "---\n\n";
        out << "## 4. Frontend Scripting Invariants\n\n";
        out << "1. **Entity Safety**: Always check `if entity and entity:IsValid()` before calling methods on pointers.\n";
        out << "2. **Hook Return Values**: Returning a non-nil value from a query/filter hook acts as an override and halts further listener propagation.\n";
        out << "3. **Hot-Reloading**: Use `ModCore.On(...)` or `ModCore.Hooks.Register(...)`. When scripts are modified on disk, the engine reloads them on the next frame tick without restarting the client.\n";
    }
}
