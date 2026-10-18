access: require("system.visuals")
status: stub

Decal and weapon trail helpers in `mods/system/visuals.lua`.

:::caution[Prototype]
`bootstrap.lua` does not load this module, nothing renders decals yet, and decals never expire because `OnWorldUpdate` passes `dt = 0`. `CreateRibbonTrail` in the schema does not exist; the real function is `AttachWeaponTrail`, which is a stub.
:::

Load it explicitly. This registers listeners on `OnWorldUpdate` (expiry) and `OnWorldRender` (an empty placeholder):

```lua
local Visuals = require("system.visuals")
local decal = Visuals.CreateDecal({ position = { x = 0, y = 0, z = 0 }, radius = 8 })
```

## @CreateDecal

`params` fields:

| Field | Default | Description |
| --- | --- | --- |
| `position` | required | `{ x, y, z }`. Returns `nil` and logs an error if missing. |
| `shape` | `"CIRCLE"` | Free-form string stored on the decal. |
| `radius` | `5.0` | |
| `innerRadius` | `0.0` | |
| `coneAngle` | `90.0` | Degrees. |
| `color` | `{ r=1, g=0.2, b=0.2, a=0.6 }` | |
| `duration` | `3.0` | Seconds. `0` or less means no expiry. |

The returned decal has `id`, the fields above, `elapsed`, `alive`, and the methods `SetPosition(pos)`, `SetRadius(r)`, and `Destroy()`.

## @CreateRibbonTrail

Not implemented. Use `Visuals.AttachWeaponTrail(unit, hand, params)`, which only logs a debug line.
