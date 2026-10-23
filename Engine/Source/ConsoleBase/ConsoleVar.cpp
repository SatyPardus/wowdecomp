#include <cstdint>
#include <cstring>
#include <malloc.h>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00CA19F4
const char* CVar::s_configFile;

// STATIC: 0x00CA19F8
bool CVar::m_needsSave;

// STATIC: 0x00CA19F9
bool CVar::m_initialized;

// STATIC: 0x00CA19FA
bool CVar::s_archiveCodeRegisteredOnly;

// STATIC: 0x00CA19FC
static TSHashTable<CVar, HASHKEY_STRI> s_registeredCVars;

// VERIFY: 2dc92f1f auto
// OFFSET: 0x00766320
static int32_t TryCreateDirectory(const char* path) {
    int32_t result = 1;

    if (!path || !*path) {
        return 0;
    }

    const char* separator = SStrChr(path + 1, '\\');

    while (separator) {
        if (separator - path >= 260) {
            return 0;
        }

        char directory[260];
        memcpy(directory, path, separator - path);
        directory[separator - path] = '\0';

        if (!OsCreateDirectory(directory, 0)) {
            uint32_t attributes = OsGetFileAttributes(directory);

            if (attributes == 0xFFFFFFFF || !(attributes & OS_FILE_ATTRIBUTE_DIRECTORY)) {
                return 0;
            }
        }

        separator = SStrChr(separator + 1, '\\');
    }

    return result;
}

// VERIFY: afa54463 manual
// OFFSET: 0x00767220
static int32_t CvarListCommandHandler(const char* command, const char* arguments) {
    for (CVar* var = s_registeredCVars.m_fulllist.Head(); var; var = s_registeredCVars.m_fulllist.Next(var)) {
        char line[256];
        SStrPrintf(line, sizeof(line), "  \"%s\" is \"%s\"", var->m_key.m_str, var->m_stringValue.m_str);

        if (var->m_defaultValue.m_str && SStrCmp(var->m_stringValue.m_str, var->m_defaultValue.m_str, STORM_MAX_STR)) {
            char value[256];
            SStrPrintf(value, sizeof(value), " (default \"%s\")", var->m_defaultValue.m_str);
            SStrPack(line, value, sizeof(line));
        }

        if (var->m_resetValue.m_str && SStrCmp(var->m_stringValue.m_str, var->m_resetValue.m_str, STORM_MAX_STR)) {
            char value[256];
            SStrPrintf(value, sizeof(value), " (reset \"%s\")", var->m_resetValue.m_str);
            SStrPack(line, value, sizeof(line));
        }

        ConsoleWrite(line, DEFAULT_COLOR);
    }

    return 1;
}

// VERIFY: 5bcbe89c manual
// OFFSET: 0x00767680
static int32_t CvarCommandHandler(const char* command, const char* arguments) {
    CVar* var = CVar::Lookup(command);
    const char* value = arguments;

    while (*value == ' ') {
        value++;
    }

    if (!*value) {
        const char* current = var->m_stringValue.m_str;

        if (current) {
            ConsolePrintf("CVar \"%s\" is \"%s\"", command, current);

            if (var->m_defaultValue.m_str && SStrCmp(current, var->m_defaultValue.m_str, STORM_MAX_STR)) {
                ConsolePrintf("  default value \"%s\"", var->m_defaultValue.m_str);
            }

            if (var->m_resetValue.m_str && SStrCmp(current, var->m_resetValue.m_str, STORM_MAX_STR)) {
                ConsolePrintf("  reset value \"%s\"", var->m_resetValue.m_str);
            }

            if (var->m_latchedValue.m_str && SStrCmp(current, var->m_latchedValue.m_str, STORM_MAX_STR)) {
                ConsolePrintf("  pending value \"%s\"", var->m_latchedValue.m_str);
            }
        }

        return 1;
    }

    if (var->m_flags & 0x100) {
        ConsolePrintf("%s is read only.", var->m_key.m_str);
        return 1;
    }

    var->Set(value, true, true, false, true);

    return 1;
}

// VERIFY: 3aa08b50 auto
// OFFSET: 0x00767840
static int32_t CvarResetCommandHandler(const char* command, const char* arguments) {
    char name[64];
    SStrTokenize(&arguments, name, sizeof(name), " ,;\t\"\r\n", nullptr);

    if (name[0]) {
        CVar* var = CVar::Lookup(name);

        if (var) {
            var->Reset();
        } else {
            ConsoleWriteA("No such cvar \"%s\"\n", ERROR_COLOR, name);
        }

        return 1;
    }

    ConsoleWrite("Resetting all cvars\n", DEFAULT_COLOR);

    for (CVar* var = s_registeredCVars.m_fulllist.Head(); var; var = s_registeredCVars.m_fulllist.Next(var)) {
        var->Reset();
    }

    return 1;
}

// VERIFY: d7ea692e auto
// OFFSET: 0x007678F0
static int32_t CvarDefaultCommandHandler(const char* command, const char* arguments) {
    char name[64];
    SStrTokenize(&arguments, name, sizeof(name), " ,;\t\"\r\n", nullptr);

    if (name[0]) {
        CVar* var = CVar::Lookup(name);

        if (var) {
            var->Default();
        } else {
            ConsoleWriteA("No such cvar \"%s\"\n", ERROR_COLOR, name);
        }

        return 1;
    }

    ConsoleWrite("Restoring all cvars\n", DEFAULT_COLOR);

    for (CVar* var = s_registeredCVars.m_fulllist.Head(); var; var = s_registeredCVars.m_fulllist.Next(var)) {
        var->Default();
    }

    return 1;
}

// VERIFY: 4978c326 manual
// OFFSET: 0x007681F0
static int32_t SetCommandHandler(const char* command, const char* arguments) {
    char name[64];
    char value[2048];
    SStrTokenize(&arguments, name, sizeof(name), " ,;\t\"\r\n", nullptr);
    SStrTokenize(&arguments, value, sizeof(value), " ,;\t\"\r\n", nullptr);

    CVar* var = CVar::Lookup(name);

    if (var) {
        var->Set(value, true, false, false, true);
    } else {
        CVar::Register(name, "", 0, value, nullptr, CATEGORY_DEFAULT, true, nullptr, false);
    }

    return 1;
}

// VERIFY: b7fb1761 auto
// OFFSET: 0x007663F0
void CVar::ArchiveCodeRegisteredOnly() {
    CVar::s_archiveCodeRegisteredOnly = true;
}

// VERIFY: ee8d0260 manual
// OFFSET: 0x00766400
int32_t CVar::Load(HOSFILE file) {
    int32_t result = 1;
    uint32_t size = static_cast<uint32_t>(OsGetFileSize(file));

    if (!size) {
        return 1;
    }

    char* buffer;
    bool onStack;

    if (size >= 0x2000) {
        buffer = static_cast<char*>(SMemAlloc(size + 1, __FILE__, __LINE__, 0));
        onStack = false;
    } else {
        buffer = static_cast<char*>(alloca(size + 1));
        onStack = true;
    }

    uint32_t bytesRead;

    if (OsReadFile(file, buffer, size, &bytesRead)) {
        buffer[size] = '\0';
        const char* cur = buffer;

        if (cur && bytesRead >= 3 && static_cast<uint8_t>(cur[0]) == 0xEF && static_cast<uint8_t>(cur[1]) == 0xBB && static_cast<uint8_t>(cur[2]) == 0xBF) {
            cur += 3;
        }

        do {
            char line[2048];
            SStrTokenize(&cur, line, sizeof(line), "\r\n", nullptr);

            if (!SStrCmpI(line, "SET ", 4)) {
                ConsoleCommandExecute(line, 0);
            }
        } while (cur && *cur);
    } else {
        result = 0;
    }

    if (!onStack) {
        SMemFree(buffer, __FILE__, __LINE__, 0);
    }

    return result;
}

// VERIFY: 79f447f6 auto
// OFFSET: 0x00766530
int32_t CVar::Load(const char* fileName) {
    HOSFILE file = OsCreateFile(fileName, OS_GENERIC_READ, 0, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file == OS_INVALID_FILE) {
        char path[260];
        SStrPrintf(path, sizeof(path), "WTF\\%s", fileName);
        file = OsCreateFile(path, OS_GENERIC_READ, 0, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

        if (file == OS_INVALID_FILE) {
            return 0;
        }
    }

    int32_t result = CVar::Load(file);
    OsCloseFile(file);

    return result;
}

// VERIFY: e1606f74 auto
// OFFSET: 0x007665D0
int32_t CVar::Delete(const char* fileName) {
    const char* path = fileName;
    char buffer[260];

    if (!OsFileExists(fileName)) {
        SStrPrintf(buffer, sizeof(buffer), "WTF\\%s", fileName);

        if (!OsFileExists(buffer)) {
            return 0;
        }

        path = buffer;
    }

    OsDeleteFile(path);

    return 1;
}

// VERIFY: 158a6024 auto
// OFFSET: 0x00766640
int32_t CVar::SaveCvar(const char* name, const char* value, void* arg) {
    char line[260];
    SStrPrintf(line, sizeof(line), "SET %s \"%s\"\n", name, value);

    uint32_t written = 0;
    OsWriteFile(static_cast<HOSFILE>(arg), line, SStrLen(line), &written);

    return written != 0;
}

// VERIFY: 14646b6b auto
// OFFSET: 0x007666B0
int32_t CVar::LogCvar(const char* name, const char* value, void* arg) {
    char line[260];
    SStrPrintf(line, sizeof(line), "SET %s \"%s\"\n", name, value);

    CVarLogBuffer* log = static_cast<CVarLogBuffer*>(arg);
    SStrPack(log->m_buffer, line, log->m_size);

    return 1;
}

// VERIFY: 31ab5de3 auto
// OFFSET: 0x00766700
void CVar::SetReadOnly(bool readOnly) {
    if (readOnly) {
        this->m_flags |= 0x4;
    } else {
        this->m_flags &= ~0x4;
    }
}

// VERIFY: e41a25fd manual
// OFFSET: 0x00766720
void CVarIntListWrapper::AddNextValue(uint32_t value) {
    this->m_state = 2;

    if (value <= 93 * 93 * 93 - 1 && this->m_pos < 0xFD) {
        this->m_buffer[this->m_pos++] = static_cast<char>(value / (93 * 93) + 35);
        this->m_buffer[this->m_pos++] = static_cast<char>(value % (93 * 93) / 93 + 35);
        this->m_buffer[this->m_pos++] = static_cast<char>(value % (93 * 93) % 93 + 35);
    }
}

// VERIFY: d7f3e424 manual
// OFFSET: 0x007667B0
void CVar::InternalSet(const char* value, bool setValue, bool setReset, bool setDefault, bool setNeedsSave) {
    bool changed = false;

    if ((this->m_flags & 0x4) || !value) {
        return;
    }

    if (setValue) {
        changed = !this->m_stringValue.m_str || SStrCmpI(value, this->m_stringValue.m_str, STORM_MAX_STR) != 0;

        if (changed) {
            this->m_stringValue.Copy(value);
            this->m_intValue = SStrToInt(value);
            this->m_floatValue = SStrToFloat(value);
        }
    }

    if (setReset && !this->m_resetValue.m_str) {
        changed = true;
        this->m_resetValue.Copy(value);
    }

    if (setDefault && !this->m_defaultValue.m_str) {
        this->m_defaultValue.Copy(value);
    } else if (!changed) {
        return;
    }

    if (setNeedsSave) {
        CVar::m_needsSave = true;
    }
}

// VERIFY: 910ad5c0 manual
// OFFSET: 0x00766860
bool CVarArrayWrapper::RawGetValue(uint32_t index) const {
    const char* str = this->m_cvar->m_stringValue.m_str;

    return *str && index < 2 * (3 * strlen(str) - 3) && (str[index / 6 + 1] & (1 << (index % 6))) != 0;
}

// VERIFY: 8db3b933 auto
// OFFSET: 0x007668C0
bool CVar::Set(const char* value, bool setValue, bool setReset, bool setDefault, bool setNeedsSave) {
    if (setValue) {
        if (this->m_callback) {
            OsValidateFunctionPointer(reinterpret_cast<const void*>(this->m_callback));

            if (!this->m_callback(this, this->m_stringValue.m_str, value, this->m_arg)) {
                return true;
            }
        }

        this->m_modified++;

        if (this->m_flags & CVAR_LATCH) {
            this->m_latchedValue.Copy(value);
            CVar::m_needsSave = true;

            return true;
        }
    }

    this->InternalSet(value, setValue, setReset, setDefault, setNeedsSave);

    return true;
}

// VERIFY: fe26c079 auto
// OFFSET: 0x00766940
bool CVar::Set(int32_t value, bool setValue, bool setReset, bool setDefault, bool setNeedsSave) {
    char buffer[32];
    SStrPrintf(buffer, sizeof(buffer), "%d", value);

    return this->Set(buffer, setValue, setReset, setDefault, setNeedsSave);
}

// VERIFY: 8aea9d36 manual
// OFFSET: 0x00766990
void CVar::Reset() {
    const char* value = this->m_resetValue.m_str;

    if (!value) {
        value = this->m_defaultValue.m_str;
    }

    if (!value) {
        return;
    }

    if (this->m_callback) {
        OsValidateFunctionPointer(reinterpret_cast<const void*>(this->m_callback));

        if (!this->m_callback(this, this->m_stringValue.m_str, value, this->m_arg)) {
            return;
        }
    }

    this->InternalSet(value, true, false, false, true);
}

// VERIFY: 6f2e979e manual
// OFFSET: 0x00766A20
void CVar::Default() {
    const char* value = this->m_defaultValue.m_str;

    if (!value) {
        value = this->m_resetValue.m_str;
    }

    if (!value) {
        return;
    }

    if (this->m_callback) {
        OsValidateFunctionPointer(reinterpret_cast<const void*>(this->m_callback));

        if (!this->m_callback(this, this->m_stringValue.m_str, value, this->m_arg)) {
            return;
        }
    }

    this->InternalSet(value, true, false, false, true);
}

// VERIFY: 0165142c manual
// OFFSET: 0x00766AB0
bool CVar::Update() {
    if (!(this->m_flags & CVAR_LATCH) || !this->m_latchedValue.m_str) {
        return false;
    }

    this->InternalSet(this->m_latchedValue.m_str, true, false, false, true);
    this->m_latchedValue.Copy(nullptr);

    return true;
}

// VERIFY: 1e80d7be manual
// OFFSET: 0x00766B20
void CVarArrayWrapper::UpdateFromVersion(int32_t version) {
    if (version != 1) {
        return;
    }

    CVar* cvar = this->m_cvar;
    char buffer[256];
    uint32_t out = 1;
    int32_t bits = 0;
    int32_t used = 0;
    const char* in = cvar->m_stringValue.m_str + 1;
    buffer[0] = 2;

    while (*in) {
        if (out >= 0xFF) {
            break;
        }

        if (!bits) {
            buffer[out] = 0;
        }

        buffer[out] = static_cast<char>(((buffer[out] | ((*in & 0x7F) << (bits - used))) & 0x3F) | 0x40);

        if (used > bits + 1) {
            in++;
            bits = bits - used + 7;
            used = 0;
        } else {
            if (used < bits + 1) {
                used += 6 - bits;
            } else {
                in++;
                used = 0;
            }

            out++;
            bits = 0;
        }
    }

    if (bits) {
        buffer[out + 1] = 0;
    } else {
        buffer[out] = 0;
    }

    cvar->Set(buffer, true, false, false, true);
}

// VERIFY: 69f5d46c manual
// OFFSET: 0x00766CA0
bool CVarArrayWrapper::RawSetValue(uint32_t index, bool value) {
    const char* str = this->m_cvar->m_stringValue.m_str;
    size_t length = strlen(str);

    if (length >= 0xFF) {
        return false;
    }

    char buffer[256];
    strncpy(buffer, str, sizeof(buffer));

    uint32_t pos = index / 6 + 1;
    buffer[length] = 0;
    buffer[0] = 2;

    if (index / 6 + 2 >= sizeof(buffer)) {
        return false;
    }

    if (value) {
        if (length < pos) {
            memset(&buffer[length], '@', pos - length + 1);
            buffer[pos + 1] = 0;
        }

        buffer[pos] |= 1 << (index % 6);
    } else {
        if (pos > length) {
            return true;
        }

        buffer[pos] &= ~(1 << (index % 6));

        for (size_t i = length - 1; i; i--) {
            if (buffer[i] != '@') {
                break;
            }

            buffer[i] = 0;
        }
    }

    this->m_cvar->Set(buffer, true, false, false, true);

    return true;
}

// VERIFY: a08db58b manual
// OFFSET: 0x00766E80
CVar::CVar() {
}

// VERIFY: 5c888a55 manual
// OFFSET: 0x00766F00
void CVarIntListWrapper::UpdateFromVersion(int32_t version) {
    if (version) {
        return;
    }

    const uint8_t* str = reinterpret_cast<const uint8_t*>(this->m_cvar->m_stringValue.m_str);
    int32_t weights[3] = { 220 * 220, 220, 1 };
    int8_t digit = 0;
    uint32_t value = 0;

    while (*str) {
        value += weights[digit++] * (*str - 35);

        if (digit > 2) {
            this->AddNextValue(value);
            value = 0;
            digit = 0;
        }

        str++;
    }

    this->WriteVersionTag();
}

// VERIFY: 39283ef7 manual
// OFFSET: 0x00766F70
uint32_t CVarIntListWrapper::GetNextValue() {
    if (this->m_pos == 2) {
        uint8_t version = 0;

        if (this->m_cvar && this->m_cvar->m_stringValue.m_str[0] == 'v') {
            version = this->m_cvar->m_stringValue.m_str[1];
        }

        if (version != 1) {
            this->UpdateFromVersion(version);
            this->m_pos = 2;
        }

        this->m_state = 1;
    }

    const char* str = this->m_cvar->m_stringValue.m_str + this->m_pos;

    if (!str[0] || !str[1] || !str[2]) {
        return 0;
    }

    this->m_pos += 3;

    return 93 * (93 * (str[0] - 35) + str[1] - 35) + str[2] - 35;
}

// VERIFY: 417de4a6 manual
// OFFSET: 0x00767030
int32_t CVar::IterateForArchive(uint32_t category, uint32_t excludeFlags, int32_t (*callback)(const char* name, const char* value, void* arg), void* arg) {
    for (CVar* var = s_registeredCVars.m_fulllist.Head(); var; var = s_registeredCVars.m_fulllist.Next(var)) {
        if (!(var->m_flags & CVAR_ARCHIVE) || (var->m_flags & 0x80) || (var->m_flags & 0x30) != category || (var->m_flags & excludeFlags)) {
            continue;
        }

        const char* value = var->m_latchedValue.m_str;

        if (!value) {
            value = var->m_stringValue.m_str;
        }

        if (!value) {
            value = var->m_defaultValue.m_str;
        }

        if (!value) {
            continue;
        }

        if ((!var->m_defaultValue.m_str || SStrCmpI(value, var->m_defaultValue.m_str, STORM_MAX_STR)) && !callback(var->m_key.m_str, value, arg)) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 1e7866dc auto
// OFFSET: 0x007670E0
int32_t CVar::SaveAll(HOSFILE file, uint32_t category, uint32_t excludeFlags) {
    return CVar::IterateForArchive(category, excludeFlags, CVar::SaveCvar, file);
}

// VERIFY: d827a7c7 auto
// OFFSET: 0x00767100
int32_t CVar::SaveAll() {
    if (!CVar::m_needsSave) {
        return 1;
    }

    CVar::m_needsSave = false;
    int32_t result = 0;

    char path[260];
    SStrCopy(path, "WTF\\", sizeof(path));
    SStrPack(path, CVar::s_configFile, sizeof(path));

    HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file != OS_INVALID_FILE) {
        result = CVar::IterateForArchive(0, 0, CVar::SaveCvar, file);
        OsCloseFile(file);
    }

    return result;
}

// VERIFY: be7badef auto
// OFFSET: 0x007671A0
int32_t CVar::LogAll(char* buffer, uint32_t size, uint32_t category, uint32_t excludeFlags) {
    CVarLogBuffer log;
    log.m_buffer = buffer;
    log.m_size = size;

    return CVar::IterateForArchive(category, excludeFlags, CVar::LogCvar, &log);
}

// VERIFY: da3d65c9 manual
// OFFSET: 0x00767350
void CVar::DeleteCharacterCVars() {
    CVar* var = s_registeredCVars.m_fulllist.Head();

    while (var) {
        if ((var->m_flags & CVAR_ARCHIVE) && (var->m_flags & 0x30) == 0x20) {
            var = s_registeredCVars.DeleteNode(var);
        } else {
            var = s_registeredCVars.Next(var);
        }
    }
}

// VERIFY: 318d83a3 manual
// OFFSET: 0x007673A0
void CVar::DeleteAccountCVars() {
    CVar* var = s_registeredCVars.m_fulllist.Head();

    while (var) {
        if ((var->m_flags & CVAR_ARCHIVE) && (var->m_flags & 0x30) == 0x10) {
            var = s_registeredCVars.DeleteNode(var);
        } else {
            var = s_registeredCVars.Next(var);
        }
    }
}

// VERIFY: 047464cf auto
// OFFSET: 0x007673F0
void CVar::Destroy() {
    CVar::m_initialized = false;
    CVar::SaveAll();
    ConsoleCommandUnregister("set");
    ConsoleCommandUnregister("cvar_reset");
    ConsoleCommandUnregister("cvar_default");
    ConsoleCommandUnregister("cvarlist");
    s_registeredCVars.Clear();
}

// VERIFY: 917f454a auto
// OFFSET: 0x00767440
CVar* CVar::Lookup(const char* name) {
    return name ? s_registeredCVars.Ptr(name) : nullptr;
}

// VERIFY: ec2acf96 manual
// OFFSET: 0x00767460
CVar* CVar::LookupRegistered(const char* name) {
    if (!name) {
        return nullptr;
    }

    CVar* var = s_registeredCVars.Ptr(name);

    if (!var) {
        return nullptr;
    }

    if (!(var->m_flags & 0x80000000) && !(var->m_flags & 0x80)) {
        return nullptr;
    }

    return var;
}

// VERIFY: 3b3ad454 manual
// OFFSET: 0x00767FC0
CVar* CVar::Register(const char* name, const char* help, uint32_t flags, const char* value, bool (*callback)(CVar*, const char*, const char*, void*), uint32_t category, bool setCommand, void* arg, bool unk) {
    if (!name || !*name || !value) {
        return nullptr;
    }

    CVar* var = s_registeredCVars.Ptr(name);

    if (var) {
        bool setReset = var->m_resetValue.m_str == nullptr;
        var->m_flags = flags | (var->m_flags & ~0x30);
        bool setDefault = var->m_defaultValue.m_str == nullptr;
        var->m_callback = callback;
        var->m_arg = arg;

        bool setValue = false;

        if (callback && !callback(var, var->m_stringValue.m_str, var->m_stringValue.m_str, arg)) {
            setValue = true;
        }

        var->Set(value, setValue, setReset, setDefault, false);

        if (!setCommand) {
            var->m_flags |= 0x80000000;
        }

        if (unk && !(var->m_flags & 0x80000000)) {
            var->m_flags |= 0x80;
        }

        return var;
    }

    var = s_registeredCVars.New(name, 0, 0);
    var->m_stringValue.Copy(nullptr);
    var->m_floatValue = 0.0f;
    var->m_intValue = 0;
    var->m_modified = 0;
    var->m_category = category;
    var->m_defaultValue.Copy(nullptr);
    var->m_resetValue.Copy(nullptr);
    var->m_latchedValue.Copy(nullptr);
    var->m_callback = callback;
    var->m_flags = 0;
    var->m_arg = arg;
    var->m_help.Copy(help);

    if (setCommand) {
        var->Set(value, true, true, false, false);
    } else {
        var->Set(value, true, false, true, false);
    }

    var->m_flags = flags | CVAR_ARCHIVE;

    if (!setCommand) {
        var->m_flags = flags | (0x80000000 | CVAR_ARCHIVE);
    }

    if (unk && !(var->m_flags & 0x80000000)) {
        var->m_flags |= 0x80;
    }

    ConsoleCommandRegister(var->m_key.m_str, CvarCommandHandler, static_cast<CATEGORY>(category), help);

    return var;
}

// VERIFY: ff762198 auto
// OFFSET: 0x00768340
void CVar::Initialize(const char* fileName) {
    CVar::s_configFile = fileName;
    CVar::m_initialized = true;

    char path[260];
    SFile::GetBasePath(path, sizeof(path));
    SStrPrintf(path, sizeof(path), "%s%s\\", path, "WTF");
    TryCreateDirectory(path);

    ConsoleCommandRegister("set", SetCommandHandler, CATEGORY_DEFAULT, "Set the value of a CVar");
    ConsoleCommandRegister("cvar_reset", CvarResetCommandHandler, CATEGORY_DEFAULT, "Set the value of a CVar to it's startup value");
    ConsoleCommandRegister("cvar_default", CvarDefaultCommandHandler, CATEGORY_DEFAULT, "Set the value of a CVar to it's coded default value");
    ConsoleCommandRegister("cvarlist", CvarListCommandHandler, CATEGORY_DEFAULT, "List cvars");

    CVar::Load(CVar::s_configFile);
}

// VERIFY: 23fe3f84 auto
// OFFSET: 0x005300D0
void CVarIntListWrapper::WriteVersionTag() {
    this->m_buffer[1] = 1;
    this->m_buffer[0] = 'v';
    this->m_buffer[this->m_pos] = 0;
    this->m_cvar->Set(this->m_buffer, true, false, false, true);
}
