#include "MapWeather.h"
#include "WoW/Source/WorldClient/WorldConstants.h"
#include <cfloat>
#include <new>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/WorldClient/Sand.h"
#include "WoW/Source/WorldClient/Rain.h"
#include <cmath>
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/DB/Rec/WeatherRec.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "Engine/Source/Os/Time.h"

// STATIC: 0x00ADF1F8
float s_weatherDensity = 0.66f;

// STATIC: 0x00CD8528
uint8_t s_weatherTypeForced;

// VERIFY: 63454e2e manual
// OFFSET: 0x00783B30
int32_t Weather::TimeToFixed(float seconds) {
    float ticks = seconds * 1024.0f;
    return CMath::fint_(ticks - 0.5f);
}

// INLINED
WeatherGridCell::WeatherGridCell() {
    this->m_cellX = -9999;
    this->m_cellY = -9999;

    for (uint32_t i = 0; i < sizeof(this->m_heights) / sizeof(this->m_heights[0]); i++) {
        this->m_heights[i] = FLT_MAX;
    }
}

// VERIFY: 4502f4fc auto
// OFFSET: 0x00783BD0
static float __stdcall GetHeightOrDefault(const C3Vector* position) {
    float height;

    if (CMap::QueryHeight(*position, 200.0f, &height)) {
        return height;
    }

    return position->z - 200.0f;
}

// VERIFY: 4e33c407 manual
// OFFSET: 0x00783C10
float WeatherGridCell::GetHeight(const C3Vector* position) {
    float x = position->x - this->m_cellX * 33.333332f;
    float y = position->y - this->m_cellY * 33.333332f;

    if (x < 0.0f || y < 0.0f) {
        return FLT_MAX;
    }

    float scaledY = y * 0.96f;
    int32_t cellY = CMath::fint_(scaledY - 0.5f);
    float scaledX = x * 0.96f;
    int32_t cellX = CMath::fint_(scaledX - 0.5f);

    if (cellX < 0 || cellY < 0 || cellX >= 32 || cellY >= 32) {
        return FLT_MAX;
    }

    float* height = &this->m_heights[cellY * 32 + cellX];

    if (*height == FLT_MAX) {
        *height = GetHeightOrDefault(position);
    }

    return *height;
}

// VERIFY: 46769949 auto
// OFFSET: 0x00783CE0
float WeatherGrid::GetSubCellHeight(const C2iVector* cell, float z) {
    WeatherGridCell* gridCell = this->m_cells[cell->y >> 5][cell->x >> 5];
    float* height = &gridCell->m_heights[(cell->y & 0x1F) * 32 + (cell->x & 0x1F)];
    C3Vector position(cell->x * 1.0416666f + this->m_minCellX * 33.333332f + 0.52083331f, cell->y * 1.0416666f + this->m_minCellY * 33.333332f + 0.52083331f, z);

    if (*height != FLT_MAX) {
        return *height;
    }

    float result;

    if (CMap::QueryHeight(position, 200.0f, &result)) {
        *height = result;
    } else {
        *height = z - 200.0f;
    }

    return *height;
}

// VERIFY: bbd3deed manual
// OFFSET: 0x00783DC0
CiRect* WeatherGrid::GetCellBounds(CiRect* bounds) {
    bounds->minY = 0xFFFF;
    bounds->maxY = -0xFFFF;
    bounds->minX = 0xFFFF;
    bounds->maxX = -0xFFFF;

    for (uint32_t y = 0; y < 5; y++) {
        for (uint32_t x = 0; x < 5; x++) {
            WeatherGridCell* cell = this->m_cells[y][x];

            if (!cell) {
                continue;
            }

            if (cell->m_cellX < bounds->minX) {
                bounds->minX = cell->m_cellX;
            }

            if (cell->m_cellX > bounds->maxX) {
                bounds->maxX = cell->m_cellX;
            }

            if (cell->m_cellY < bounds->minY) {
                bounds->minY = cell->m_cellY;
            }

            if (cell->m_cellY > bounds->maxY) {
                bounds->maxY = cell->m_cellY;
            }
        }
    }

    return bounds;
}

// VERIFY: b61c5f42 manual
// OFFSET: 0x00783FB0
WeatherGridBase::~WeatherGridBase() {
}

// VERIFY: d6faa539 manual
// OFFSET: 0x00784AB0
WeatherGrid::WeatherGrid() {
    this->unused_0004 = 0.0f;
    this->unused_0008 = 0.0f;
    this->unused_000C = 0.0f;
    this->m_originX = 0;
    this->m_originY = 0;
    this->m_minCellX = 0;
    this->m_minCellY = 0;
    this->m_minX = 0;
    this->m_minY = 0;

    for (uint32_t y = 0; y < 5; y++) {
        for (uint32_t x = 0; x < 5; x++) {
            this->m_cells[y][x] = new (SMemAlloc(sizeof(WeatherGridCell), __FILE__, __LINE__, 0)) WeatherGridCell();
        }
    }
}

// VERIFY: 6499eaa9 auto
// OFFSET: [0x00784B80, 0x00786850]
WeatherGrid::~WeatherGrid() {
    for (uint32_t y = 0; y < 5; y++) {
        for (uint32_t x = 0; x < 5; x++) {
            if (this->m_cells[y][x]) {
                SMemFree(this->m_cells[y][x], "delete", SLOG_FUNCTION, 0);
            }
        }
    }
}

// VERIFY: b6c66264 auto
// OFFSET: 0x00784B60
void WeatherGrid::GetCellSpacing(float* cellSize, float* cellsPerYard) {
    *cellSize = 1.0416666f;
    *cellsPerYard = 0.96f;
}

// VERIFY: 8e6d2779 auto
// OFFSET: 0x00784BE0
float WeatherGrid::GetHeight(const C3Vector* position) {
    uint32_t x = static_cast<int32_t>(floor(position->x * OO_COORD_TO_CHUNK)) - this->m_minCellX;
    uint32_t y = static_cast<int32_t>(floor(position->y * OO_COORD_TO_CHUNK)) - this->m_minCellY;

    if (x > 4 || y > 4) {
        return FLT_MAX;
    }

    return this->m_cells[y][x]->GetHeight(position);
}

// VERIFY: efebdce6 manual
// OFFSET: 0x00784C60
bool WeatherGrid::Raycast(const C3Vector* start, const C3Vector* end, C3Vector* hit) {
    *hit = *start;
    int32_t startX = static_cast<int32_t>(floor(start->x * 0.96f)) - this->m_minX;
    int32_t startY = static_cast<int32_t>(floor(start->y * 0.96f)) - this->m_minY;
    int32_t endX = static_cast<int32_t>(floor(end->x * 0.96f)) - this->m_minX;
    int32_t endY = static_cast<int32_t>(floor(end->y * 0.96f)) - this->m_minY;
    C3Vector delta(end->x - start->x, end->y - start->y, end->z - start->z);
    C2iVector distance;
    distance.x = abs(endX - startX);
    distance.y = abs(endY - startY);
    int32_t steps;
    int32_t error;
    int32_t errorDiagonal;
    int32_t errorStraight;
    int32_t straightX;
    int32_t straightY;
    int32_t diagonalX;

    if (distance.MajorAxis()) {
        steps = distance.y;
        error = 2 * distance.x - distance.y;
        errorDiagonal = 2 * (distance.x - distance.y);
        errorStraight = 2 * distance.x;
        straightX = 0;
        diagonalX = 1;
        straightY = 1;
    } else {
        steps = distance.x;
        error = 2 * distance.y - distance.x;
        errorStraight = 2 * distance.y;
        errorDiagonal = 2 * (distance.y - distance.x);
        straightX = 1;
        diagonalX = 1;
        straightY = 0;
    }

    int32_t diagonalY = 1;

    if (end->x < start->x) {
        straightX = -straightX;
        diagonalX = -1;
    }

    if (end->y < start->y) {
        straightY = -straightY;
        diagonalY = -1;
    }

    int32_t count = steps <= 0 ? 1 : steps;
    C2iVector cell;
    cell.x = startX;
    cell.y = startY;
    float step = 1.0f / count;
    int32_t i = 0;

    while (cell.x >= 0 && cell.y >= 0 && cell.x < 160 && cell.y < 160) {
        float t = i * step;
        hit->x = delta.x * t + start->x;
        hit->y = delta.y * t + start->y;
        hit->z = delta.z * t + start->z;
        float z = hit->z;
        float height = this->GetSubCellHeight(&cell, hit->z);

        if (height > z) {
            hit->z = height;
            return true;
        }

        if (error > 0) {
            error += errorDiagonal;
            cell.x += diagonalX;
            cell.y += diagonalY;
        } else {
            error += errorStraight;
            cell.x += straightX;
            cell.y += straightY;
        }

        if (i == count) {
            *hit = *end;
        }

        if (++i > count) {
            return false;
        }
    }

    *hit = *start;
    return false;
}

// VERIFY: 32392fc5 auto
// OFFSET: 0x00784F20
void WeatherGrid::UpdateOriginCell(const C3Vector* position) {
    int32_t x = static_cast<int32_t>(floor(position->x * OO_COORD_TO_CHUNK));
    int32_t y = static_cast<int32_t>(floor(position->y * OO_COORD_TO_CHUNK));

    if (x == this->m_originX && y == this->m_originY) {
        return;
    }

    this->m_originY = y;
    this->m_minCellY = y - 2;
    this->m_minCellX = x - 2;
    this->m_originX = x;
    this->m_minX = 32 * this->m_minCellX;
    this->m_minY = 32 * this->m_minCellY;
    WeatherGridCell* cells[25];

    for (uint32_t row = 0; row < 5; row++) {
        for (uint32_t col = 0; col < 5; col++) {
            cells[row * 5 + col] = this->m_cells[row][col];
            this->m_cells[row][col] = nullptr;
        }
    }

    for (int32_t row = 0; row < 5; row++) {
        for (int32_t col = 0; col < 5; col++) {
            for (uint32_t i = 0; i < 25; i++) {
                if (cells[i] && cells[i]->m_cellX == col + this->m_minCellX && cells[i]->m_cellY == row + this->m_minCellY) {
                    this->m_cells[row][col] = cells[i];
                    cells[i] = nullptr;
                    break;
                }
            }
        }
    }

    for (int32_t row = 0; row < 5; row++) {
        for (int32_t col = 0; col < 5; col++) {
            if (this->m_cells[row][col]) {
                continue;
            }

            for (uint32_t i = 0; i < 25; i++) {
                if (!cells[i]) {
                    continue;
                }

                WeatherGridCell* cell = cells[i];
                this->m_cells[row][col] = cell;
                cells[i] = nullptr;
                cell->m_cellX = col + this->m_minCellX;
                cell->m_cellY = row + this->m_minCellY;

                for (uint32_t j = 0; j < sizeof(cell->m_heights) / sizeof(cell->m_heights[0]); j++) {
                    cell->m_heights[j] = FLT_MAX;
                }

                break;
            }
        }
    }
}

// VERIFY: 9756c64f manual
// OFFSET: 0x007844F0
void Snow::SetStormIntensity(float intensity) {
    uint8_t useShaders = this->m_useWeatherShaders;
    this->m_intensity = intensity;
    float rate = useShaders ? 14000.0f : 1300.0f;
    this->m_particlesPerFrame = s_weatherDensity * rate * intensity;
    this->m_maxBuildTime = rate * 0.00016276042f;
    float mistRate = useShaders ? 48.0f : 24.0f;
    float excess = intensity - 0.5f;
    float scale = excess >= 0.0f ? s_weatherDensity * (excess + excess) : s_weatherDensity * (0.0f + 0.0f);
    this->m_mists.m_spawnRate = mistRate * scale;
}

// VERIFY: 3630477a manual
// OFFSET: 0x00784850
void Weather::SetStormIntensity(float target, float current) {
    uint32_t now = OsGetAsyncTimeMs();
    float delta = current - target;
    float t = (now - this->m_intensityStartTime) * 0.001f / (fabs(delta + 0.001f) * 10.0f);

    if (t < 0.0f) {
        t = 0.0f;
    } else if (t >= 1.0f) {
        t = 1.0f;
    }

    this->m_intensity = target + delta * t;
    float intensity = (this->m_intensity - 0.25f) * 1.3333334f;

    if (intensity < 0.0f) {
        intensity = 0.0f;
    }

    if (this->m_rain) {
        this->m_rain->SetStormIntensity(intensity);
    }

    if (this->m_snow) {
        this->m_snow->SetStormIntensity(intensity);
    }

    if (this->m_sand) {
        this->m_sand->SetStormIntensity(intensity);
    }

    float elapsed = (now - this->m_fogStartTime) * 0.001f;
    float fade = elapsed / (fabs((this->m_targetFogIntensity - this->m_startFogIntensity) * 4.0f + 0.001f) * 10.0f);

    if (fade < 0.0f) {
        fade = 0.0f;
    } else if (fade >= 1.0f) {
        fade = 1.0f;
    }

    this->m_fogIntensity = this->m_startFogIntensity + fade * (this->m_targetFogIntensity - this->m_startFogIntensity);
    float blend = elapsed / (fabs(0.001f + 4.0f * (this->m_targetDensityBlend - this->m_startDensityBlend)) * 5.0f);

    if (blend < 0.0f) {
        blend = 0.0f;
    } else if (1.0f <= blend) {
        blend = 1.0f;
    }

    this->m_densityBlend = blend * (this->m_targetDensityBlend - this->m_startDensityBlend) + this->m_startDensityBlend;
    DayNight::GetInfo()->m_weatherIntensity = this->m_fogIntensity * this->m_densityBlend;
    DayNight::ResetFogAccumulators(false);
    this->m_densityChanged = 0;
}

// VERIFY: c46e1162 manual
// OFFSET: 0x00787780
Weather::Weather() {
    this->m_targetIntensity = 0.0f;
    this->m_startIntensity = 0.0f;
    this->m_intensity = 0.0f;
    this->m_targetFogIntensity = 0.0f;
    this->m_startFogIntensity = 0.0f;
    this->m_fogIntensity = 0.0f;
    this->m_intensityStartTime = 0;
    this->m_fogStartTime = 0;
    this->m_pendingType = -1;
    this->m_densityChanged = 0;
    this->m_immediate = 0;
    this->m_densityBlend = 1.0f;
    this->m_startDensityBlend = 1.0f;
    this->m_targetDensityBlend = 1.0f;
    this->m_color.b = 0xFF;
    this->m_color.g = 0xFF;
    this->m_color.r = 0xFF;
    this->m_color.a = 0xFF;
    this->m_rain = nullptr;
    this->m_snow = nullptr;
    this->m_sand = nullptr;
    this->m_grid = nullptr;
    this->m_playerVelocityX = 0.0f;
    this->m_playerVelocityY = 0.0f;
    this->m_playerVelocityZ = 0.0f;
    this->m_transportVelocityX = 0.0f;
    this->m_transportVelocityY = 0.0f;
    this->m_transportVelocityZ = 0.0f;
    this->m_positionX = 0.0f;
    this->m_positionY = 0.0f;
    this->m_positionZ = 0.0f;
    this->m_onTaxi = 0;
    this->m_facing = 0.0f;
    this->m_speed = 0.0f;
    this->m_hasUpdated = 0;

    this->m_grid = new (SMemAlloc(sizeof(WeatherGrid), __FILE__, __LINE__, 0)) WeatherGrid();

    CVar::Register("weatherDensity", nullptr, 0, "2", CVarHandler_weatherDensity, CATEGORY_DEFAULT, false, nullptr, false);
    s_useShaders = CVar::Register("useWeatherShaders", nullptr, 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);

    uint32_t forcedType = 0;

    if (SRegLoadValue("Internal", "force-weather-type-on", 0, &forcedType) && forcedType > 0 && forcedType < 4) {
        this->SetType(forcedType, 1.0f, nullptr, false, 1.0f);
        s_weatherTypeForced = 1;
    }

    this->m_precipTexture[0] = '\0';
}

// VERIFY: 99319c82 manual
// OFFSET: 0x0078D540
Weather::~Weather() {
    this->Clear();

    delete this->m_grid;
    this->m_grid = nullptr;

    this->m_playerDeltas.Clear();
    this->m_transportDeltas.Clear();
}

// VERIFY: f3eaf113 auto
// OFFSET: 0x0078D0B0
void Weather::Clear() {
    if (this->m_rain) {
        Rain* rain = this->m_rain;
        rain->~Rain();
        SMemFree(rain, "delete", SLOG_FUNCTION, 0);
    }

    this->m_rain = nullptr;

    if (this->m_snow) {
        Snow* snow = this->m_snow;
        snow->~Snow();
        SMemFree(snow, "delete", SLOG_FUNCTION, 0);
    }

    this->m_snow = nullptr;

    if (this->m_sand) {
        Sand* sand = this->m_sand;
        sand->~Sand();
        SMemFree(sand, "delete", SLOG_FUNCTION, 0);
    }

    this->m_sand = nullptr;
}

// VERIFY: 8429a026 auto
// OFFSET: 0x0078D130
void Weather::Reset() {
    if (s_weatherTypeForced) {
        return;
    }

    this->Clear();
    this->m_startIntensity = 0.0f;
    this->m_pendingType = -1;
    this->m_targetIntensity = 0.0f;
    this->m_precipTexture[0] = '\0';
    this->m_intensity = 0.0f;
    this->m_startFogIntensity = 0.0f;
    this->m_targetFogIntensity = 0.0f;
    this->m_fogIntensity = 0.0f;
    this->m_targetDensityBlend = 1.0f;
    this->m_startDensityBlend = 1.0f;
    this->m_densityBlend = 1.0f;
}

// VERIFY: 2089fdca manual
// OFFSET: 0x00784A30
bool Weather::GetActiveEffectBounds(CAaBox* bounds) {
    if (this->m_rain) {
        *bounds = this->m_rain->m_bbox;
        return true;
    }

    if (this->m_snow) {
        *bounds = this->m_snow->m_bbox;
        return true;
    }

    if (this->m_sand) {
        *bounds = this->m_sand->GetBoundingBox();
        return true;
    }

    return false;
}

// OFFSET: 0x0078CA50
void Weather::Render() {
    if (!this->m_hasUpdated) {
        return;
    }

    if (this->m_rain) {
        this->m_rain->Render();
    }

    if (this->m_snow) {
        this->m_snow->Render();
    }

    if (this->m_sand) {
        this->m_sand->Render();
    }
}

// VERIFY: 2ffa4bed manual
// OFFSET: 0x0078D170
void Weather::Update() {
    this->UpdatePosDeltas();
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (activePlayer) {
        CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(activePlayer, TYPE_PLAYER, __FILE__, __LINE__));

        if (player) {
            C3Vector delta(this->m_playerVelocityX, this->m_playerVelocityY, 0.0f);

            if (sqrtf(delta.y * delta.y + delta.x * delta.x) < 1.0f || (player->m_unit->m_unitFieldFlags & 0x100000)) {
                this->m_facing = player->GetFacing();
            } else {
                delta.Normalize();
                float angle = atan2f(delta.y, delta.x);
                this->m_facing = angle;

                if (angle < 0.0f) {
                    this->m_facing = angle + 6.2831855f;
                }
            }

            this->m_onTaxi = (player->m_unit->m_unitFieldFlags >> 20) & 1;
            C3Vector position = player->GetPosition();
            this->m_positionX = position.x;
            this->m_positionY = position.y;
            this->m_positionZ = position.z;

            Weather* weather = World::s_weather;

            if (weather->m_transportVelocityX * weather->m_transportVelocityX + weather->m_transportVelocityY * weather->m_transportVelocityY + weather->m_transportVelocityZ * weather->m_transportVelocityZ <= 2.0f) {
                this->m_speed = player->m_movement->m_currentSpeed;
            } else {
                this->m_speed = sqrtf(this->m_transportVelocityX * this->m_transportVelocityX + this->m_transportVelocityY * this->m_transportVelocityY);
            }
        }
    }

    bool stormUpdated = false;

    if (this->m_pendingType != -1) {
        int32_t active = 0;

        if (this->m_rain) {
            active = this->m_rain->CountActivePackets();
        }

        if (this->m_snow) {
            active += this->m_snow->CountActivePackets();
        }

        if (this->m_sand) {
            active += this->m_sand->CountActivePackets();
        }

        if (active > 0 && this->m_immediate) {
            this->SetStormIntensity(this->m_startIntensity, this->m_targetIntensity < 0.25f ? this->m_targetIntensity : 0.25f);
            stormUpdated = true;

            if (this->m_snow) {
                this->m_snow->m_fadeOut = 1;
                this->m_snow->m_mists.m_fadeOut = 1;
            }

            if (this->m_rain) {
                this->m_rain->m_fadeOut = 1;
                this->m_rain->m_mists.m_fadeOut = 1;
            }

            if (this->m_sand) {
                this->m_sand->m_fadeOut = 1;
                this->m_sand->m_mists.m_fadeOut = 1;
            }
        } else {
            this->Clear();

            switch (this->m_pendingType) {
                case 1: {
                    void* mem = SMemAlloc(sizeof(Rain), __FILE__, __LINE__, 0);
                    this->m_rain = mem ? new (mem) Rain(130.0f, 75.0f) : nullptr;
                    break;
                }

                case 2: {
                    void* mem = SMemAlloc(sizeof(Snow), __FILE__, __LINE__, 0);
                    this->m_snow = mem ? new (mem) Snow(90.0f, 60.0f) : nullptr;
                    break;
                }

                case 3: {
                    void* mem = SMemAlloc(sizeof(Sand), __FILE__, __LINE__, 0);
                    this->m_sand = mem ? new (mem) Sand(40.0f, 25.0f) : nullptr;
                    break;
                }
            }

            this->m_intensityStartTime = OsGetAsyncTimeMs();
            this->m_startIntensity = this->m_intensity;
            this->m_pendingType = -1;
        }
    }

    if (this->m_rain || this->m_snow || this->m_sand) {
        this->m_grid->UpdateOriginCell(&CWorldScene::s_activeWorldView);
    }

    bool changed = fabsf(this->m_intensity - this->m_targetIntensity) >= 0.00000023841858f || fabsf(this->m_fogIntensity - this->m_targetFogIntensity) >= 0.00000023841858f || fabsf(this->m_densityBlend - this->m_targetDensityBlend) >= 0.00000023841858f;

    if ((changed && !stormUpdated) || this->m_densityChanged) {
        this->SetStormIntensity(this->m_startIntensity, this->m_targetIntensity);
    }

    this->m_hasUpdated = 1;
}

// VERIFY: f7fa9fb7 manual
// OFFSET: 0x00783B60
uint32_t Weather::GetType() const {
    if (this->m_rain) {
        return 1;
    }

    if (this->m_snow) {
        return 2;
    }

    if (this->m_sand) {
        return 3;
    }

    return 0;
}

// VERIFY: 9b9ae04a manual
// OFFSET: 0x007846A0
void Weather::SetType(uint32_t type, float intensity, const WeatherRec* rec, bool immediate, float transitionTime) {
    if (s_weatherTypeForced || type > 3) {
        return;
    }

    this->m_immediate = immediate;
    bool unchanged = true;

    if (this->GetType() != type) {
        this->m_precipTexture[0] = '\0';
        unchanged = false;
    }

    bool copied = false;

    if (rec) {
        this->m_color.Set(1.0f, rec->m_effectColor[0], rec->m_effectColor[1], rec->m_effectColor[2]);

        if (SStrLen(rec->m_effectTexture) > 0 && SStrCmpI(this->m_precipTexture, rec->m_effectTexture, STORM_MAX_STR)) {
            unchanged = false;
            SStrCopy(this->m_precipTexture, rec->m_effectTexture, STORM_MAX_STR);
            copied = true;
        }
    }

    if (SStrLen(this->m_precipTexture) <= 0 && !copied) {
        this->FillInDefaultPrecipTexturePath(type);
        unchanged = false;
    }

    if (this->GetType() != type || !unchanged) {
        this->m_pendingType = type;
    }

    float density = intensity < 0.0f ? 0.0f : (intensity >= 1.0f ? 1.0f : intensity);
    this->m_startIntensity = this->m_targetIntensity;
    this->m_targetIntensity = density;

    float time;

    if (this->GetType() == 0) {
        this->m_targetDensityBlend = transitionTime;
        time = transitionTime;
    } else if (type == 0) {
        time = this->m_targetDensityBlend;
    } else {
        time = transitionTime;
    }

    this->m_startDensityBlend = this->m_targetDensityBlend;
    this->m_targetDensityBlend = time;

    if (!immediate) {
        this->m_densityChanged = 1;
        this->m_startIntensity = density;
        this->m_intensity = density;
        this->m_startDensityBlend = time;
        this->m_densityBlend = time;
    }

    this->m_targetFogIntensity = density < 0.0f ? 0.0f : (density >= 0.25f ? 0.25f : density);
    this->m_startFogIntensity = this->m_startIntensity < 0.0f ? 0.0f : (this->m_startIntensity >= 0.25f ? 0.25f : this->m_startIntensity);

    uint32_t now = OsGetAsyncTimeMs();
    this->m_intensityStartTime = now;
    this->m_fogStartTime = now;
}

// VERIFY: 54aaddcb manual
// OFFSET: 0x0078C500
void Weather::UpdatePosDeltas() {
    uint32_t total = 0;

    for (PosDelta* delta = this->m_playerDeltas.Head(); delta;) {
        total += delta->m_time;
        PosDelta* next = this->m_playerDeltas.Next(delta);

        if (total >= 150) {
            this->m_playerDeltas.DeleteNode(this->m_playerDeltas.Head());
        }

        delta = next;
    }

    total = 0;

    for (PosDelta* delta = this->m_transportDeltas.Head(); delta;) {
        total += delta->m_time;
        PosDelta* next = this->m_transportDeltas.Next(delta);

        if (total >= 150) {
            this->m_transportDeltas.DeleteNode(this->m_transportDeltas.Head());
        }

        delta = next;
    }

    C3Vector playerPos;
    C3Vector playerVelocity;
    C3Vector transportPos;
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (activePlayer) {
        CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(activePlayer, TYPE_PLAYER, __FILE__, __LINE__));

        if (player) {
            playerPos = player->GetPosition();
            playerVelocity = player->m_movement->m_moveDir * player->m_movement->m_currentSpeed;

            if (player->GetTransportGUID()) {
                CGObject_C* transport = ClntObjMgrObjectPtr(WOWGUID{ player->GetTransportGUID() }, TYPE_OBJECT, __FILE__, __LINE__);

                if (transport) {
                    transportPos = transport->GetPosition();
                }
            }
        }
    }

    uint32_t now = OsGetAsyncTimeMs();
    static C3Vector s_lastTransportPos = transportPos;
    static uint32_t s_lastTime = now;

    if (now != s_lastTime) {
        C3Vector delta = transportPos - s_lastTransportPos;
        PosDelta* node = this->m_transportDeltas.NewNode(2, 0, 0);
        node->m_delta = delta;
        node->m_time = now - s_lastTime;
    }

    s_lastTransportPos = transportPos;

    if (now - s_lastTime) {
        C3Vector delta = playerPos - C3Vector(this->m_positionX, this->m_positionY, this->m_positionZ);
        PosDelta* node = this->m_playerDeltas.NewNode(2, 0, 0);
        node->m_delta = delta;
        node->m_time = now - s_lastTime;
    }

    s_lastTime = now;

    C3Vector playerSum;
    total = 0;

    for (PosDelta* delta = this->m_playerDeltas.Head(); delta; delta = this->m_playerDeltas.Next(delta)) {
        total += delta->m_time;
        playerSum.x += delta->m_delta.x;
        playerSum.y += delta->m_delta.y;
        playerSum.z += delta->m_delta.z;
    }

    float playerScale = 1.0f / (static_cast<float>(total + 1) * 0.001f);
    this->m_playerVelocityX = playerSum.x * playerScale;
    this->m_playerVelocityY = playerSum.y * playerScale;
    this->m_playerVelocityZ = playerSum.z * playerScale;

    C3Vector transportSum;
    total = 0;

    for (PosDelta* delta = this->m_transportDeltas.Head(); delta; delta = this->m_transportDeltas.Next(delta)) {
        total += delta->m_time;
        transportSum.x += delta->m_delta.x;
        transportSum.y += delta->m_delta.y;
        transportSum.z += delta->m_delta.z;
    }

    float transportScale = 1.0f / (static_cast<float>(total + 1) * 0.001f);
    this->m_transportVelocityX = transportSum.x * transportScale;
    this->m_transportVelocityY = transportSum.y * transportScale;
    this->m_transportVelocityZ = transportSum.z * transportScale;

    if (fabs(playerVelocity.x) < 2.3841858e-07f && fabs(playerVelocity.y) < 2.3841858e-07f && fabs(playerVelocity.z) < 2.3841858e-07f && this->m_transportVelocityX * this->m_transportVelocityX + this->m_transportVelocityY * this->m_transportVelocityY + this->m_transportVelocityZ * this->m_transportVelocityZ <= 2.0f) {
        this->m_playerVelocityX = 0.0f;
        this->m_playerVelocityY = 0.0f;
        this->m_playerVelocityZ = 0.0f;
    }
}

// VERIFY: e5f3f9ba auto
// OFFSET: 0x00783B90
void Weather::FillInDefaultPrecipTexturePath(uint32_t type) {
    if (type == 1) {
        SStrCopy(this->m_precipTexture, "textures\\Weather\\RainDrop01.blp", 0x7FFFFFFF);
    } else if (type == 2) {
        SStrCopy(this->m_precipTexture, "textures\\Weather\\SnowFlake01.blp", 0x7FFFFFFF);
    }
}
