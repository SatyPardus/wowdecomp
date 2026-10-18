## OnPacketReceived

source: `NetClient::ProcessMessage` (`NetClient.cpp`)

- Fired for every inbound message **except** the custom-packet carriers `0x102` and `0x7A00`, which the [custom packet manager](/guides/custom-packets/) consumes first.
- `opcode` is the 16-bit message id. `time` is the receive timestamp.
- The third argument is an opaque pointer to the `CDataStore`, already advanced past the message id. It is not a reader. To parse payloads from Lua use [custom packets](/guides/custom-packets/).
- Return `true` to skip the client's registered handler for that message.

## OnPacketSend

status: partial
source: `NetClient::Send` (`NetClient.cpp`)

:::caution[opcode is always 0]
The hook runs after the message is serialized and the opcode is not extracted. `size` is the number of unread bytes.
:::

Every message the client sends passes through, including ModCore's own custom packets. Return `true` to drop the message.

## OnDisconnect

source: `NetClient.cpp`

Fired when the network connection is torn down.
