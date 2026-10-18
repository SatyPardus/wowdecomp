title: DBC (DbcEngine)
access: ModCore.DBC
status: partial

An in-memory DBC layer in `mods/system/dbc.lua`. It is loaded by `bootstrap.lua` as `ModCore.DBC` and is also the global `DBC`.

:::caution[The schema does not match the implementation]
The exported schema lists `GetTable`, which does not exist, and misses most of the real functions. The actual API is below. Nothing in C++ consults this module, so it does **not** change what the client reads from real `.dbc` files, and `DBC.GetRow` cannot read the game's own tables.
:::

## Actual API

| Function | Description |
| --- | --- |
| `DBC.RegisterTable(name, schema [, initialRows])` | Declare a table. `schema` is an ordered list of column names. `initialRows` is a map `id → row`. |
| `DBC.DefineRow(name, id, row)` | Add or replace a row in a registered table. Sets `row.id = id`. |
| `DBC.RegisterVirtualRow(name, id, row)` | Add a row that takes precedence over table rows. Works without `RegisterTable`. |
| `DBC.OverrideRow(name, id, fields)` | Overlay fields on a row. The stored row is not modified; `GetRow` returns a copy. |
| `DBC.OnRowLookup(name, fn)` | Add a lookup hook `fun(id): table?`. The first non-nil result wins. |
| `DBC.GetRow(name, id) → table?` | Resolve a row. |
| `DBC.GetRowUnpacked(name, id) → ...` | The row's values in schema order. Without a schema, unpacks an array-style row, otherwise returns the row table. |
| `DBC.GetRows(name) → table` | The `id → row` map of registered rows (excludes virtual rows). |
| `DBC.GetNumRows(name) → integer` | Number of registered rows (excludes virtual rows). |

`_CDBC_GET` and `GetCDBCRow` are globals equal to `DBC.GetRowUnpacked`, for callers that follow the C++ calling convention.

## Lookup order

`DBC.GetRow(table, id)` stops at the first hit:

1. Lookup hooks from `DBC.OnRowLookup`.
2. Rows from `DBC.RegisterVirtualRow`.
3. Rows from `DBC.RegisterTable` or `DBC.DefineRow`, with `DBC.OverrideRow` fields applied to a copy.

`id` is converted with `tonumber` when possible.

```lua
local DBC = ModCore.DBC

DBC.RegisterTable("MyTable", { "id", "name", "value" }, {
    [1] = { name = "First",  value = 10 },
    [2] = { name = "Second", value = 20 },
})
DBC.RegisterVirtualRow("MyTable", 3, { name = "Third", value = 30 })
DBC.OverrideRow("MyTable", 1, { value = 99 })

local row = DBC.GetRow("MyTable", 1)    -- { id = 1, name = "First", value = 99 }
```
