## OnLootOpen

source: `CGLootInfo::SetObject` (`LootFrame.cpp`)

## OnLootSlot

source: `CGLootInfo::LootSlot` (`LootFrame.cpp`)

## OnLootClose

source: `LootFrame.cpp`

## OnMerchantBuy

source: `BuyMerchantItem` script (`MerchantFrame_Scripts.cpp`)

Returning `true` makes the script return without buying.

## OnQuestAccept

source: `AcceptQuest` script (`QuestFrame_Scripts.cpp`)

## OnQuestComplete

source: `CompleteQuest` script (`QuestFrame_Scripts.cpp`)

## OnQuestReward

source: `GetQuestReward` script (`QuestFrame_Scripts.cpp`)

`rewardIndex` is the chosen reward.

## OnGossipSelect

source: `SelectGossipOption` script (`GossipInfo_Scripts.cpp`)

## OnActionUse

source: `UseAction` script (`ActionBarFrame_Scripts.cpp`)

## OnMacroExecute

source: `RunMacroText` and `RunMacro` scripts (`UIMacros_Scripts.cpp`)

For `RunMacroText`, `name` is `""` and `body` is the text. For `RunMacro`, both come from the macro.

## OnCooldownSet

source: cooldown frame `SetCooldown` script (`CGCooldown_Scripts.cpp`)

`name` is the cooldown frame's name.

## OnPlaySound

source: `SI2::PlayUISound` (`SoundInterface2ZoneSounds.cpp`)

## OnPlayMusic

source: `PlayMusic` script (`SoundInterface2_Scripts.cpp`)

## OnItemUse

source: `UseContainerItem` script (`ContainerFrame_Scripts.cpp`)

## OnTradeInitiate

source: `CGTradeInfo::InitiateTrade` (`Trade_C.cpp`)

## OnTradeAccept

source: `AcceptTrade` script (`TradeFrame_Scripts.cpp`)

## OnCombatFeedback

source: `CGGameUI::ShowCombatFeedback` (`GameUI.cpp`)

This drives floating combat text. Returning `true` suppresses the engine's `WOUND` combat signal for the event. `schoolMask` is the spell school bit mask.

## OnNamePlateCreate

source: `NamePlateFrame.cpp`

`namePlatePtr` is an opaque pointer to the nameplate frame.
