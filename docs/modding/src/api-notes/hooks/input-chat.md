## OnKeyDown

source: `CGWorldFrame::OnLayerKeyDown` (`WorldFrame.cpp`)

Only fires for input the world frame receives; input a UI frame already handled never reaches it.

- `modifiers` is a bit mask: 1 LShift, 2 RShift, 4 LCtrl, 8 RCtrl, 16 LAlt, 32 RAlt. Lua 5.1 has no `&`, so test a bit with `math.floor(mask / 2^bit) % 2 == 1`.
- `keyString` is the key-binding string **with modifier prefixes**, such as `LSHIFT-H`, `SHIFT-H` (both shifts held), `CTRL-K`, or `F5`.
- Pure modifier presses and auto-repeat events do not fire the hook.
- Returning `true` skips the normal key-binding execution.

```lua
ModCore.On("OnKeyDown", function(key, modifiers, keyString)
    if keyString == "LSHIFT-H" then
        ModCore.LogInfo("Toggled custom UI")
        return true
    end
end)
```

## OnKeyUp

source: `CGWorldFrame::OnLayerKeyUp` (`WorldFrame.cpp`)

Same arguments and rules as [`OnKeyDown`](/reference/hooks/input-chat/onkeydown/).

## OnMouseDown

source: `CGWorldFrame` mouse handling (`WorldFrame.cpp`)

`button` is the client's button index: `1` left, `2` middle, `3` right, `4` and up for extra buttons. `x` and `y` are screen pixels.

## OnMouseUp

source: `CGWorldFrame` mouse handling (`WorldFrame.cpp`)

Same button numbering as [`OnMouseDown`](/reference/hooks/input-chat/onmousedown/).

## OnMouseWheel

source: `CGWorldFrame` mouse handling (`WorldFrame.cpp`)

`delta` is the wheel amount reported by the OS layer.

## OnChatMessage

source: `CGChat::AddChatMessage` (`ChatFrame.cpp`)

`true` hides the message. `sender` and `channel` are empty strings when absent.

```lua
ModCore.On("OnChatMessage", function(chatType, sender, message, channel)
    if string.find(message, "spam", 1, true) then
        return true
    end
end)
```

## OnSendChatMessage

source: `SendChatMessage` script (`ChatFrame_Scripts.cpp`)

`true` cancels the send. Only messages sent through the `SendChatMessage` script function are seen, which includes the default chat edit box.

:::note
Returning a string does not rewrite the message. To change it, consume it and send your own.
:::
