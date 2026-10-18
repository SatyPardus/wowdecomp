## OnSpellQuery

status: partial
source: `Spell_C_GetPowerCost` (`Spell_C.cpp`), spell book passive check (`SpellBookFrame_Scripts.cpp`)

Only two query types are wired:

| `queryType` | Effect of the return value |
| --- | --- |
| `"PowerCost"` | A number is used as the cost. A boolean becomes `0` or `1`. `nil` keeps the stock calculation. |
| `"IsPassive"` | A truthy value (`true` or non-zero) reports the spell as passive. `false` or `0` reports it as **not** passive. `nil` keeps the DBC flag. |

:::caution
The engine passes only `(queryType, spellId)`; the `caster` parameter in the schema is never supplied. `"PowerCost"` is queried for any unit, not just the player, and returning `false` sets the cost to `0`. The `"Range"` and `"CastTime"` queries mentioned in the schema description are not implemented.
:::

```lua
ModCore.On("OnSpellQuery", function(queryType, spellId)
    if queryType == "IsPassive" and spellId == 48266 then
        return false            -- show Blood Presence on the action bar
    elseif queryType == "PowerCost" and spellId == 98100 then
        return 35
    end
end)
```

## OnSpellCast

source: `Spell_C_CastSpell` (`Spell_C.cpp`)

## OnChannelStart

source: `Spell_C.cpp`

`msg` is an opaque pointer to the server message.

## OnChannelUpdate

source: `Spell_C.cpp`

Fired when a channel duration update arrives.

## OnChannelStop

source: `Spell_C.cpp`

Fired when a channel ends or is interrupted.

## OnSpellDestroy

source: `Spell_C_Destroy` (`Spell_C.cpp`)

Runs when the spell system is torn down.

## OnTooltipEvaluate

source: spell tooltip builder (`Tooltip.cpp`), spell-description parser (`QuestTextParser.cpp`)

Return a string to replace the description. In the description parser, `$` variables left in the returned text are still expanded.

The `caster` argument is a table built for each call:

| Member | Description |
| --- | --- |
| `unit` | The string `"player"`. |
| `_ptr` | Light userdata for the C++ caster pointer. |
| `caster:GetAttackPower()` | The larger of the player's melee and ranged attack power, including positive and negative modifiers. |
| `caster:GetSpellPower([school])` | Bonus spell damage for the player. `school` is `"HOLY"`, `"FIRE"`, `"NATURE"`, `"FROST"`, `"SHADOW"`, `"ARCANE"` (case-insensitive) or a school number 1-6. Anything else means school 0. Never less than `0`. |

:::caution
Both methods read the **active player**, regardless of which unit the tooltip describes. Call them with `:`, because the school is read from argument 2.
:::

```lua
ModCore.On("OnTooltipEvaluate", function(spellId, caster, defaultText)
    if spellId == 98100 then
        local ap = caster:GetAttackPower()
        local sp = caster:GetSpellPower("FIRE")
        return string.format("Strike enemies for %.0f Fire damage.", 300 + ap * 0.45 + sp * 0.2)
    end
end)
```

## OnTooltipSetSpell

source: `CGTooltip::SetSpell` (`Tooltip.cpp`)

`tooltip` is an opaque pointer. Lua cannot add lines through it yet.

## OnTooltipSetItem

source: `CGTooltip::SetItem` (`Tooltip.cpp`)

`tooltip` is an opaque pointer. Lua cannot add lines through it yet.

## OnTooltipSetUnit

source: `CGTooltip::SetUnit` (`Tooltip.cpp`)

`tooltip` is an opaque pointer. Lua cannot add lines through it yet.
