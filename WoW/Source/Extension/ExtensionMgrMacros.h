#ifndef WOW_SOURCE_EXTENSION_EXTENSIONMGRMACROS_H
#define WOW_SOURCE_EXTENSION_EXTENSIONMGRMACROS_H

#include "WoW/Source/Extension/ExtensionMgr.h"

// The dispatch loop every ExtensionMgr::OnXxx uses: run action for each script of scriptType, in
// registration order. action refers to the current script as `script`. Scripts are never added or
// removed while a hook runs, so the plain range loop is safe.
#define CALL_HOOKS(scriptType, action) \
    for (scriptType* script : ScriptRegistry<scriptType>::ScriptPointerList) { action; }

#endif
