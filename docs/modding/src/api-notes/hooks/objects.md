## OnObjectCreate

source: `ObjectMgrClient.cpp`

`typeId` is the client object type id. `objectPtr` cannot be used from Lua, so track the `guid` instead (see [Entity types](/reference/api/cgobject-c/)). GUIDs are doubles; see [argument types](/guides/hooks/#argument-types).

## OnObjectDestroy

source: `ObjectMgrClient.cpp`

Fired while the object is being removed from the client object manager.

## OnTargetChanged

source: `CGGameUI` target handling (`GameUI.cpp`)

Runs after the new target is stored and sent to the server. An empty target is GUID `0`.
