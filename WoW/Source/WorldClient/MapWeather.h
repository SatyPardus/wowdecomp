#ifndef WOW_SOURCE_WORLDCLIENT_MAPWEATHER_H
#define WOW_SOURCE_WORLDCLIENT_MAPWEATHER_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/WorldClient/Mists.h"
#include "WoW/Source/WorldClient/Rain.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CiRect.h"
#include "Engine/Source/Tempest/C2iVector.h"

struct CVar;
class WeatherRec;

class CGxShader;
class CTexture;
struct SnowFlakeParticle {
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_velocity;
    /* 0018 */ float m_startTime;
    /* 001C */ float m_endTime;
};

struct SnowFlakeParticleBuffer {
    static const char* const s_packetName;
    static const uint32_t s_vertexSize = 32;
    static const uint32_t s_verticesPerParticle = 1;

    /* 0000 */ uint32_t m_particleCount;
    /* 0004 */ SnowFlakeParticle m_particles [6144];

    SnowFlakeParticleBuffer();
};

class Snow {
    public:
    /* 0000 */ uint8_t m_useWeatherShaders;
    /* 0001 */ uint8_t m_fadeOut;
    /* 0002 */ uint8_t unused_0002[2];
    /* 0004 */ CAaBox m_bbox;
    /* 001C */ Mists m_mists;
    /* 0070 */ PacketList<Packet<SnowFlakeParticleBuffer>> m_snowFlakePacketList;
    /* 0090 */ Packet<SnowFlakeParticleBuffer>* m_snowFlakePacketPtr;
    /* 0094 */ float m_maxBuildTime;
    /* 0098 */ CTexture* m_snowFlakeTex;
    /* 009C */ CGxShader* m_shaderPtr;
    /* 00A0 */ float m_particlesPerFrame;
    /* 00A4 */ float m_intensity;

    Snow(float width, float height);
    void SetStormIntensity(float intensity);
    ~Snow();
    int32_t CountActivePackets();
    void InitFlake(SnowFlakeParticle* flake, float time, const C3Vector* origin, const C3Vector* wind);
    void Render();
    void RenderPackets();
    void RenderPacketsSW();
    void UpdatePacket(Packet<SnowFlakeParticleBuffer>* packet, float count, const C3Vector* origin, const C3Vector* wind);
    void UpdatePackets();
};

struct WeatherGridCell {
    /* 0000 */ float m_heights[1024];
    /* 1000 */ int32_t m_cellX;
    /* 1004 */ int32_t m_cellY;

    WeatherGridCell();
    float GetHeight(const C3Vector* position);
};

class WeatherGridBase {
    public:
    virtual CiRect* GetCellBounds(CiRect* bounds) = 0;
    virtual void UpdateOriginCell(const C3Vector* position) = 0;
    virtual bool Raycast(const C3Vector* start, const C3Vector* end, C3Vector* hit) = 0;
    virtual float GetHeight(const C3Vector* position) = 0;
    virtual void GetCellSpacing(float* cellSize, float* cellsPerYard) = 0;
    virtual ~WeatherGridBase();
};

class WeatherGrid : public WeatherGridBase {
    public:
    /* 0004 */ float unused_0004;
    /* 0008 */ float unused_0008;
    /* 000C */ float unused_000C;
    /* 0010 */ int32_t m_originX;
    /* 0014 */ int32_t m_originY;
    /* 0018 */ int32_t m_minCellX;
    /* 001C */ int32_t m_minCellY;
    /* 0020 */ int32_t m_minX;
    /* 0024 */ int32_t m_minY;
    /* 0028 */ WeatherGridCell* m_cells[5][5];

    CiRect* GetCellBounds(CiRect* bounds) override;
    void UpdateOriginCell(const C3Vector* position) override;
    bool Raycast(const C3Vector* start, const C3Vector* end, C3Vector* hit) override;
    float GetHeight(const C3Vector* position) override;
    void GetCellSpacing(float* cellSize, float* cellsPerYard) override;
    ~WeatherGrid() override;

    WeatherGrid();
    float GetSubCellHeight(const C2iVector* cell, float z);
};

struct PosDelta : public TSLinkedNode<PosDelta> {
    /* 0008 */ C3Vector m_delta;
    /* 0014 */ uint32_t m_time;
};

class Sand;

class Weather {
    public:
    static int32_t TimeToFixed(float seconds);

    /* 0000 */ float m_targetIntensity;
    /* 0004 */ float m_startIntensity;
    /* 0008 */ float m_intensity;
    /* 000C */ float m_targetFogIntensity;
    /* 0010 */ float m_startFogIntensity;
    /* 0014 */ float m_fogIntensity;
    /* 0018 */ uint32_t m_intensityStartTime;
    /* 001C */ uint32_t m_fogStartTime;
    /* 0020 */ int32_t m_pendingType;
    /* 0024 */ uint8_t m_densityChanged;
    /* 0025 */ uint8_t m_immediate;
    /* 0026 */ uint8_t unused_0026[2];
    /* 0028 */ float m_densityBlend;
    /* 002C */ float m_startDensityBlend;
    /* 0030 */ float m_targetDensityBlend;
    /* 0034 */ CImVector m_color;
    /* 0038 */ char m_precipTexture[260];
    /* 013C */ Rain* m_rain;
    /* 0140 */ Snow* m_snow;
    /* 0144 */ Sand* m_sand;
    /* 0148 */ WeatherGrid* m_grid;
    /* 014C */ TSList<PosDelta, TSGetLink<PosDelta> > m_playerDeltas;
    /* 0158 */ TSList<PosDelta, TSGetLink<PosDelta> > m_transportDeltas;
    /* 0164 */ float m_playerVelocityX;
    /* 0168 */ float m_playerVelocityY;
    /* 016C */ float m_playerVelocityZ;
    /* 0170 */ float m_transportVelocityX;
    /* 0174 */ float m_transportVelocityY;
    /* 0178 */ float m_transportVelocityZ;
    /* 017C */ float m_positionX;
    /* 0180 */ float m_positionY;
    /* 0184 */ float m_positionZ;
    /* 0188 */ uint8_t m_onTaxi;
    /* 0189 */ uint8_t unused_0189[3];
    /* 018C */ float m_facing;
    /* 0190 */ float m_speed;
    /* 0194 */ uint8_t m_hasUpdated;
    /* 0195 */ uint8_t unused_0195[3];

    Weather();
    bool GetActiveEffectBounds(CAaBox* bounds);
    void Render();
    void Update();
    ~Weather();
    void Clear();
    void Reset();
    void SetType(uint32_t type, float intensity, const WeatherRec* rec, bool immediate, float transitionTime);
    void UpdatePosDeltas();
    uint32_t GetType() const;
    C3Vector* GetPosition(C3Vector* position);
    void FillInDefaultPrecipTexturePath(uint32_t type);
    void SetStormIntensity(float target, float current);
};

extern float s_weatherDensity;
extern uint8_t s_weatherTypeForced;
extern CVar* s_useShaders;

bool CVarHandler_weatherDensity(CVar* var, const char* oldValue, const char* value, void* arg);

class MapWeather {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint32_t unused_0018;
    /* 001C */ uint32_t unused_001C;
    /* 0020 */ uint32_t unused_0020;
    /* 0024 */ uint8_t unused_0024;
    /* 0025 */ uint8_t unused_0025;
    /* 0026 */ uint8_t unused_0026[2];
    /* 0028 */ uint32_t unused_0028;
    /* 002C */ uint32_t unused_002C;
    /* 0030 */ uint32_t unused_0030;
    /* 0034 */ uint8_t unused_0034[172];
    /* 00E0 */ uint8_t unused_00E0;
    /* 00E1 */ uint8_t unused_00E1[91];
    /* 013C */ uint32_t unused_013C;
    /* 0140 */ uint32_t unused_0140;
    /* 0144 */ uint32_t unused_0144;
    /* 0148 */ uint32_t unused_0148;
    /* 014C */ uint8_t unused_014C[24];
    /* 0164 */ uint32_t unused_0164;
    /* 0168 */ uint32_t unused_0168;
    /* 016C */ uint8_t unused_016C[4];
    /* 0170 */ uint32_t unused_0170;
    /* 0174 */ uint32_t unused_0174;
    /* 0178 */ uint8_t unused_0178[4];
    /* 017C */ uint32_t unused_017C;
    /* 0180 */ uint32_t unused_0180;
    /* 0184 */ uint32_t unused_0184;
    /* 0188 */ uint8_t unused_0188;
    /* 0189 */ uint8_t unused_0189[3];
    /* 018C */ uint32_t unused_018C;
    /* 0190 */ uint32_t unused_0190;
    /* 0194 */ uint8_t unused_0194;
    /* 0195 */ uint8_t unused_0195[3];
};

#endif
