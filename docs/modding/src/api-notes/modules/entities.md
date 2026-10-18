## C3Vector

status: declared

Hook arguments use plain `{ x, y, z }` tables for positions, not instances of this type.

## Quat

status: declared

Used only as a packet field type: `quat` in a [Net schema](/reference/api/net/) is a table `{ w, x, y, z }`.

## WOWGUID

status: declared

Hook arguments pass GUIDs as Lua numbers, which lose precision above 2^53. `ModCore.GetLastHitGuid()` also returns a hex string. `guid` fields in a [Net schema](/reference/api/net/) are `{ low, high }` tables.

## CGObject_C

status: declared

Hooks such as `OnObjectCreate` pass the object as an opaque light userdata pointer. Lua cannot read it or call through it, so track the GUID instead.

## CGUnit_C

status: declared

No Lua bindings exist. Use the standard 3.3.5 Lua API (`UnitHealth`, `UnitPower`, ...) for unit data.

## CGPlayer_C

status: declared

No Lua bindings exist.

## ClntObjMgr

status: declared

No Lua bindings exist. The `caster` argument of `OnTooltipEvaluate` is the only object-like table passed to Lua, and it exposes just `GetAttackPower()` and `GetSpellPower(school)` for the active player.
