#pragma once
#include <iosfwd>
class ClientData;
void CheckAudioResources(const ClientData& client, std::ostream& report);
void CheckAudioOutput(const ClientData& client, std::ostream& report);
