access: ModCore

The global `ModCore` table, created by `mods/system/bootstrap.lua`.

:::note
The schema lists methods only. Fields, the sub-modules, and a few extra functions are described here.
:::

## Fields

| Field | Type | Description |
| --- | --- | --- |
| `Version` | `string` | Framework version (`"1.1.0"`). |
| `BasePath` | `string` | Absolute mods directory with a trailing `/`, set by C++ before bootstrap runs. |
| `LoadedMods` | `table<string, table>` | Mounted mod contexts by id. |
| `Hooks` | `table` | The [Hooks module](/reference/api/hooks/). |
| `Net` | `table` | The [Net module](/reference/api/net/). |
| `DBC` | `table` | The [DBC module](/reference/api/dbcengine/). |
| `ApiExport` | `table` | The [API export registry](/reference/api-export/). |

## Logging

`LogDebug`, `LogInfo`, `LogWarn`, and `LogError` format with `string.format`. If formatting fails, `tostring(fmt)` is logged. Output goes to the process's standard output as `[LUA <LEVEL>] message`. All levels are always printed; the `logLevel` config key is not read.

## Registering and firing hooks

`On`, `RegisterHook`, and `Hooks.Register` are equivalent. These are not in the schema:

| Function | Description |
| --- | --- |
| `ModCore.HasListeners(hookName)` | `true` if at least one listener is registered. |
| `ModCore.Hooks.OnXxx(callback, priority)` | Registration sugar for `ModCore.On("OnXxx", ...)`. |

:::caution
Any undefined `ModCore.OnXxx` member resolves through a metatable to a function that **fires** hook `OnXxx`. It does not register a listener.
:::

## Native globals

Registered by C++ and used by the framework. Mods should not call the `_`-prefixed ones.

| Global | Purpose |
| --- | --- |
| `loadfile(path)` | Replaces the stock function. Loads through the [bytecode cache](/internals/bytecode-cache/) and returns the chunk, or `nil, message`. |
| `_MODCORE_LOG(level, msg)` | Backend of `ModCore.Log*`. |
| `ModCore_ExportApi()` | Backend of `ModCore.ExportApi`. |
| `_ModCore_SetCastAtCursor`, `_ModCore_IsCastAtCursor`, `_ModCore_ClickTerrain`, `_ModCore_GetLastHitGuid`, `_ModCore_GetLastHitPoint` | Backends of the world interaction functions. |
| `_CLIENT_NETWORK`, `_LUA_LOG` | Backends of [packet I/O and logging](/reference/packet-io/). |

## @RegisterHook

Alias of `On`. Returns `true` on success.

## @On

Registers a listener. `priority` defaults to `100` and higher runs first. See [Hooks and return values](/guides/hooks/).

## @UnregisterHook

Removes every registration of `callback`, matched by function identity.

## @FireHook

Calls listeners in priority order and returns the first non-nil result.

## @DispatchHook

The entry point the engine calls. Equivalent to `FireHook`.

## @GetHookList

Returns the sorted names of hooks that have listeners, not the full hook catalog.

## @MountMod

Normally called by C++. Requires `manifest.id`, refuses duplicates, runs `<modDir>/<entryPoint or "init.lua">` under `pcall` with a context table, stores the context in `LoadedMods`, and fires `OnModMounted(id, context)`. See [Directory layout](/getting-started/directory-layout/).

## @GetApiSchema

Returns `{ version, classes, functions, hooks, opcodes }` built from the Lua-side registry.

## @ExportApi

Regenerates `mods/api/*` through C++. Returns `true`, or `false` if the native function is missing.

## @SetCastAtCursor

While armed, the next terrain-tracking event disarms it and synthesizes a left terrain click at the cursor position.

```lua
ModCore.On("OnKeyDown", function(key, modifiers, keyString)
    if keyString == "F" then
        CastSpellByName("Blizzard")     -- starts ground targeting
        ModCore.SetCastAtCursor(true)
        return true
    end
end)
```

## @ClickTerrain

:::caution
This always forwards three arguments, so calling it with missing coordinates clicks `(0, 0, 0)` instead of the player. Use `ClickSelfTerrain()` for that. The `?` markers in the schema are misleading.
:::

## @GetLastHitGuid

Returns the GUID as a hex string (`"0x..."`) and as a Lua number. Returns `"0", 0` if the natives are missing. Updated by every [`OnWorldHitTest`](/reference/hooks/lifecycle-world/onworldhittest/).

## @GetLastHitPoint

Returns `0, 0, 0` when there has been no hit. Updated by every `OnWorldHitTest`.
