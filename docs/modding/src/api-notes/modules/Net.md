access: ModCore.Net

Declarative custom packets, in `mods/system/net.lua`. Also available through `require("system.net")`. For a walkthrough see [Custom packets](/guides/custom-packets/).

## Definition fields

`Net.DefinePacket(name, def)` takes a table:

| Field | Type | Description |
| --- | --- | --- |
| `opcode` | `integer` | **Required.** Inner opcode (`uint16`). |
| `direction` | `string` | `"SERVER_TO_CLIENT"`, `"CLIENT_TO_SERVER"` (default for export), or `"BIDIRECTIONAL"`. |
| `schema` | `{ {name, type}, ... }` | Ordered field list. |
| `handler` | `fun(data: table)` | Called with the decoded packet. Only wired for server-to-client and bidirectional packets. |

## Schema types

| Type | Lua value | Wire size | Read | Write |
| --- | --- | --- | --- | --- |
| `uint8`, `int8` | number | 1 | yes | yes |
| `uint16`, `int16` | number | 2 | yes | yes |
| `uint32`, `integer`, `int32` | number | 4 | yes | yes |
| `float` | number | 4 | yes | yes |
| `double` | number | 8 | yes | yes |
| `string` | string | 4 + length | yes | yes |
| `guid` | `{ low, high }` (two `uint32`) | 8 | yes | **no** |
| `vector3` | `{ x, y, z }` | 12 | yes | yes |
| `quat` | `{ w, x, y, z }` | 16 | yes | yes |

Integers are little-endian. There is no `uint64` type; use the [raw reader](/reference/packet-io/) for 64-bit fields. Unknown types are skipped on both read and write, which misaligns every later field.

`Net.RegisterSchemaHandler(opcode, def)` is internal; it builds the reader callback that decodes `def.schema` and calls `def.handler`.

## @DefinePacket

Without `opcode`, an error is logged and nothing is defined. The packet is also recorded in the [API export registry](/reference/api-export/) as an opcode. Handler errors are caught and logged as `Error in packet handler '<name>' (0x<opcode>)`.

## @Send

Writes each field from `payload`. Missing fields become `0`, `0.0`, or `""`, and a missing `quat` writes `w = 1`. Returns `false` if the packet is undefined or `CreateCustomPacket` is unavailable.

:::caution
Returns `true` even when the client is not connected, in which case nothing is sent. `guid` fields cannot be encoded and are skipped.
:::

## @OnCustomPacket

Registers a raw handler `fun(reader)` for an inner opcode. Equivalent to the global `OnCustomPacket`; see [Packet I/O](/reference/packet-io/).
