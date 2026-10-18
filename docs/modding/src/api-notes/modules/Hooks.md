title: Hooks module
access: ModCore.Hooks

The event bus in `mods/system/hooks.lua`. Also available through `require("system.hooks")`. For how listeners, priorities, and return values interact see [Hooks and return values](/guides/hooks/).

Registration sugar, which is not in the schema: `Hooks.OnXxx(callback, priority, doc)` is shorthand for `Hooks.Register("OnXxx", ...)` and works for any key starting with `On` that is not already a field of the module.

```lua
ModCore.Hooks.OnWorldUpdate(function(dt) end, 100)
```

## @Register

Returns `false` and logs an error if `hookName` is missing or `callback` is not a function. `doc` is stored once per hook name and exposed by `GetMetadata`. Registering the same function twice adds two listeners.

## @Unregister

Removes every listener entry whose function equals `callback`.

## @Fire

Calls listeners in priority order under `pcall`. Errors are logged and skipped. Returns the first non-nil result, or `nil`.

## @GetMetadata

Map from hook name to the `doc` string passed at registration.

## @GetHookList

Sorted names of hooks that have had listeners registered.
