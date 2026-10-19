#pragma once
#include <iosfwd>
class ClientData;
void CheckAudioResources(const ClientData& client, std::ostream& report);
enum class AudioSessionCheck { Reset, Interrupt, Resume };
void CheckAudioOutput(const ClientData& client, std::ostream& report, void (*notify)(AudioSessionCheck) = nullptr);
