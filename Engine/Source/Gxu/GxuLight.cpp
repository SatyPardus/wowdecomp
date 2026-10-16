#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Gxu/GxuLight.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"

// STATIC: 0x00AD982C
static uint8_t s_dirty = 1;

// STATIC: 0x00AD9830
static uint32_t s_maxLights = 4;

// STATIC: 0x00AD9868
static float s_gridRound = 0.5f;

// STATIC: 0x00AD986C
static float s_gridSize = 1024.0f;

// STATIC: 0x00AD9870
static float s_gridHalfSize = 512.0f;

// STATIC: 0x00AD9874
static TSList<CGxuLight, TSGetLink<CGxuLight> > s_lights;

// STATIC: 0x00AD9880
static TSList<CGxuLight, TSGetLink<CGxuLight> > s_freeLights;

// STATIC: 0x00AD988C
static TSExplicitList<CGxuLightLink, offsetof(CGxuLightLink, m_lightLink)> s_freeLinks;

// STATIC: 0x00AD9898
static CLightList s_globalLights;

// STATIC: 0x00C7C028
static uint32_t s_lastHashval;

// STATIC: 0x00C7C030
static uint32_t s_frame;

// STATIC: 0x00C7C040
static TSHashTableReuse<CLightList, HASHKEY_DWORD> s_lightGrid;

// STATIC: 0x00C7C074
static CGxuLightQueue s_lightQueue({ 32, 32 });

static void GxuLightInitializeImpl() {
}

// VERIFY: f3382317 auto
// OFFSET: 0x006BB510
static CGxLight* GxuLightLockImpl(CGxuLight* light) {
    light->m_lockCount++;

    return &light->m_light;
}

// VERIFY: 8a90a392 auto
// OFFSET: 0x006BB520
static void GxuLightSetEnabledImpl(CGxuLight* light, bool enabled) {
    CGxLight* gxLight = GxuLightLock(light);
    gxLight->m_flags ^= (enabled ^ gxLight->m_flags) & 1;
    GxuLightUnlock(light);
}

// VERIFY: fac290f6 auto
// OFFSET: 0x006BB550
static int32_t GxuLightGetEnabledImpl(CGxuLight* light) {
    int32_t enabled = GxuLightLock(light)->m_flags & 1;
    GxuLightUnlock(light);

    return enabled;
}

// VERIFY: d402ee7a auto
// OFFSET: 0x006BB580
static void GxuLightSetMaxLightsImpl(uint32_t maxLights) {
    s_maxLights = maxLights;

    if (maxLights >= 4) {
        s_maxLights = 4;
    }
}

// VERIFY: d75edb97 auto
// OFFSET: 0x006BB5A0
static float GxuLightGetGridSizeImpl() {
    return s_gridSize;
}

// VERIFY: fd3c112d auto
// OFFSET: 0x006BB5B0
static void GxuLightSetGridSizeImpl(float gridSize) {
    s_gridSize = gridSize;
    s_gridHalfSize = gridSize * 0.5;
    GxuLightInvalidate();
}

// VERIFY: 81504112 auto
// OFFSET: 0x006BB5D0
static void GxuLightInvalidateImpl() {
    s_dirty = 1;
}

// VERIFY: 53003b8d manual
// OFFSET: 0x006BB5E0
float CGxuLight::CalcIntensity(const C3Vector& point, float linearAttenuation, float quadraticAttenuation) {
    C3Vector delta = point - this->m_light.m_dir;
    float distance = sqrt(delta.SquaredMag());
    float dirMag = sqrt(this->m_light.m_dirColor.SquaredMag());
    double intensity = dirMag / (distance * distance * quadraticAttenuation + distance * linearAttenuation + 1.0);
    float ambientMag = sqrt(this->m_light.m_ambientColor.SquaredMag());

    return ambientMag + intensity;
}

// VERIFY: 99f107c6 manual
// OFFSET: 0x006BB6D0
CGxuLightLink::~CGxuLightLink() {
}

// VERIFY: 7f7c46e8 manual
// OFFSET: 0x006BB750
void CGxuLight::ClearLinks() {
    CGxuLightLink* link = this->m_links.Head();

    while (link) {
        CGxuLightLink* next = this->m_links.Next(link);

        link->m_lightLink.Unlink();
        link->m_listLink.Unlink();

        if (this->m_light.m_flags & 2) {
            CLightList* list = link->m_list;

            if (list->m_links.Head()) {
                s_lightGrid.Delete(list);
            }
        }

        s_freeLinks.LinkToTail(link);
        link = next;
    }
}

// VERIFY: 87d08be1 manual
// OFFSET: 0x006BB970
CGxuLightLink* CGxuLight::NewLink() {
    CGxuLightLink* link = s_freeLinks.Head();

    if (!link) {
        link = s_freeLinks.NewNode(2, 0, 0);
    }

    link->m_lightLink.Unlink();
    this->m_links.LinkToTail(link);

    return link;
}

// VERIFY: ac47e974 manual
// OFFSET: 0x006BBA50
CLightList::~CLightList() {
}

// VERIFY: ff5460b5 manual
// OFFSET: 0x006BBAA0
static void GxuLightDestroyImpl(CGxuLight* light) {
    s_dirty = 1;
    s_lights.UnlinkNode(light);
    light->ClearLinks();
    s_freeLights.LinkToTail(light);
}

// VERIFY: 4e8645ef manual
// OFFSET: 0x006BBB60
CGxuLight::~CGxuLight() {
}

// INLINED
CGxuLight::CGxuLight() : m_lockCount(0) {
}

// VERIFY: 919d9cac manual
// OFFSET: 0x006BBEC0
static CGxuLight* GxuLightCreateImpl() {
    CGxuLight* light = s_freeLights.Head();

    if (!light) {
        return s_lights.NewNode(2, 0, 0);
    }

    s_freeLights.UnlinkNode(light);
    s_lights.LinkToTail(light);
    new (&light->m_light) CGxLight();

    return light;
}

// VERIFY: 442981c1 manual
// OFFSET: 0x006BBF40
static void GxuLightApplyImpl(float x, float y, int32_t unused, const C3Vector& viewPos, uint32_t maxLights) {
    if (!s_lights.Head()) {
        return;
    }

    if (maxLights >= s_maxLights) {
        maxLights = s_maxLights;
    }

    int32_t cellY = CMath::fint_(y / s_gridSize - s_gridRound);
    int32_t cellX = CMath::fint_(x / s_gridSize - s_gridRound);

    HASHKEY_DWORD key;
    key.m_key = (cellY << 16) | static_cast<uint16_t>(cellX);
    uint32_t hashval = key.m_key % 0x1FFF;
    CLightList* list = s_lightGrid.Ptr(hashval, key);

    s_frame++;
    uint32_t index = 0;

    for (CGxuLightLink* link = s_globalLights.m_links.Head(); link; link = s_globalLights.m_links.Next(link)) {
        g_theGxDevicePtr->LightSet(index, link->m_light->m_light, viewPos);
        g_theGxDevicePtr->LightEnable(index, 1);
        link->m_light->m_frame = s_frame;
        link->m_light->m_index = index;
        index++;
    }

    if (list) {
        for (CGxuLightLink* link = list->m_links.Head(); link && index < maxLights; link = list->m_links.Next(link)) {
            g_theGxDevicePtr->LightSet(index, link->m_light->m_light, viewPos);
            g_theGxDevicePtr->LightEnable(index, 1);
            link->m_light->m_frame = s_frame;
            link->m_light->m_index = index;
            index++;
        }
    }

    for (; index != 4; index++) {
        g_theGxDevicePtr->LightEnable(index, 0);
    }

    s_dirty = 0;
    s_lastHashval = hashval;
}

// VERIFY: 1d2dd109 manual
// OFFSET: 0x006BC360
static void GxuLightShutdownImpl() {
    s_lightGrid.Destroy();
    s_freeLights.Clear();
    s_freeLinks.Clear();
}

// VERIFY: 7579993a manual
// OFFSET: 0x006BC960
static void GxuLightUnlockImpl(CGxuLight* light) {
    light->m_lockCount--;
    s_dirty = 1;
    light->ClearLinks();

    uint32_t flags = light->m_light.m_flags;

    if (!(flags & 1)) {
        return;
    }

    if (!(flags & 2)) {
        CGxuLightLink* link = light->NewLink();
        s_globalLights.m_links.LinkToTail(link);
        link->m_light = light;
        link->m_list = &s_globalLights;
        return;
    }

    float ambientMag = sqrt(light->m_light.m_ambientColor.SquaredMag());
    float dirMag = sqrt(light->m_light.m_dirColor.SquaredMag());
    float radius = (dirMag + ambientMag) / 0.05f;

    if (light->m_light.m_linearAttenuation != 0.0f) {
        radius = radius / light->m_light.m_linearAttenuation;
    }

    float minX = light->m_light.m_dir.x - radius;
    float minY = light->m_light.m_dir.y - radius;
    float minZ = light->m_light.m_dir.z - radius;
    float maxX = light->m_light.m_dir.x + radius;
    float maxY = light->m_light.m_dir.y + radius;

    int32_t minCellY = CMath::fint_(minY / s_gridSize - s_gridRound);
    int32_t minCellX = CMath::fint_(minX / s_gridSize - s_gridRound);
    int32_t maxCellY = CMath::fint_(maxY / s_gridSize - s_gridRound);
    int32_t maxCellX = CMath::fint_(maxX / s_gridSize - s_gridRound);

    for (int32_t cellY = minCellY; cellY <= maxCellY; cellY++) {
        for (int32_t cellX = minCellX; cellX <= maxCellX; cellX++) {
            HASHKEY_DWORD key;
            key.m_key = (cellY << 16) | static_cast<uint16_t>(cellX);
            uint32_t hashval = key.m_key % 0x1FFF;
            CLightList* list = s_lightGrid.Ptr(hashval, key);

            if (!list) {
                list = s_lightGrid.New(hashval, key, 0, 0);
            }

            C3Vector center(cellX * s_gridSize + s_gridHalfSize, s_gridHalfSize + s_gridSize * cellY, minZ);
            float intensity = light->CalcIntensity(center, light->m_light.m_linearAttenuation, light->m_light.m_quadraticAttenuation);

            CGxuLightLink* link = light->NewLink();
            link->m_intensity = intensity;
            link->m_light = light;
            link->m_list = list;

            CGxuLightLink* node = list->m_links.Head();

            while (node && node->m_intensity > intensity) {
                node = list->m_links.Next(node);
            }

            if (node) {
                list->m_links.LinkNode(link, 2, node);
            } else {
                list->m_links.LinkToTail(link);
            }
        }
    }
}

// STATIC: 0x00AD9834
void (*GxuLightInitialize)() = GxuLightInitializeImpl;

// STATIC: 0x00AD9838
void (*GxuLightShutdown)() = GxuLightShutdownImpl;

// STATIC: 0x00AD983C
CGxuLight* (*GxuLightCreate)() = GxuLightCreateImpl;

// STATIC: 0x00AD9840
void (*GxuLightDestroy)(CGxuLight* light) = GxuLightDestroyImpl;

// STATIC: 0x00AD9844
CGxLight* (*GxuLightLock)(CGxuLight* light) = GxuLightLockImpl;

// STATIC: 0x00AD9848
void (*GxuLightUnlock)(CGxuLight* light) = GxuLightUnlockImpl;

// STATIC: 0x00AD984C
void (*GxuLightApply)(float x, float y, int32_t unused, const C3Vector& viewPos, uint32_t maxLights) = GxuLightApplyImpl;

// STATIC: 0x00AD9850
int32_t (*GxuLightGetEnabled)(CGxuLight* light) = GxuLightGetEnabledImpl;

// STATIC: 0x00AD9854
void (*GxuLightSetEnabled)(CGxuLight* light, bool enabled) = GxuLightSetEnabledImpl;

// STATIC: 0x00AD9858
void (*GxuLightSetMaxLights)(uint32_t maxLights) = GxuLightSetMaxLightsImpl;

// STATIC: 0x00AD985C
float (*GxuLightGetGridSize)() = GxuLightGetGridSizeImpl;

// STATIC: 0x00AD9860
void (*GxuLightSetGridSize)(float gridSize) = GxuLightSetGridSizeImpl;

// STATIC: 0x00AD9864
void (*GxuLightInvalidate)() = GxuLightInvalidateImpl;

// VERIFY: 78842f2d manual
// OFFSET: 0x006BB8F0
CGxuLightQueue::CGxuLightQueue(const uint32_t (&params)[2]) : CMemBlock(sizeof(CGxuLight*) * params[0], 0, STORM_TYPE_NAME(CGxuLight*), SLOG_OBJECT) {
    uint32_t count = params[0];
    uint32_t chunk = params[1];

    this->m_chunk = chunk;
    this->m_capacity = count;
    this->m_count = 0;

    if (count) {
        this->m_count++;
        return;
    }

    if (!chunk) {
        return;
    }

    uint32_t grow = chunk;

    if (grow <= 1 - count) {
        grow = 1 - count;
    }

    if (this->Resize(sizeof(CGxuLight*) * (grow + count), true)) {
        this->m_capacity += grow;
        this->m_count++;
    }
}
