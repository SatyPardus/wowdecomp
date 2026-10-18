#pragma once
#include <string>

// Selects persistent application settings, independently of the read-only
// installation. Account/realm/character components are supplied by the session.
bool NativeSettingsFile(std::string& path, const char* name, bool createParents,
                        const char* account = nullptr, const char* realm = nullptr, const char* character = nullptr);
bool NativeSettingsReplace(const std::string& path, const void* bytes, size_t size);
