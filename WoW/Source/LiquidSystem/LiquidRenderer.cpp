#include "Liquid.h"
#include <cstdlib>
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxLight.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B23F6C
Liquid::CRenderer::DrawListSorter Liquid::CRenderer::s_drawListSorter[2] = {
    Liquid::CRenderer::CompareOpaque,
    Liquid::CRenderer::CompareTransparent
};

// VERIFY: 3d910c8b manual
// OFFSET: 0x00791910
Liquid::CRenderer::~CRenderer() {
}

// VERIFY: c8176254 manual
// OFFSET: 0x008A1980
int32_t Liquid::CRenderer::CompareOpaque(const void* a, const void* b) {
    const Liquid::CInstance* l = *static_cast<Liquid::CInstance* const*>(a);
    const Liquid::CInstance* r = *static_cast<Liquid::CInstance* const*>(b);

    if (l->m_material < r->m_material) {
        return -1;
    }

    if (l->m_material > r->m_material) {
        return 1;
    }

    if (l->m_settings < r->m_settings) {
        return -1;
    }

    if (l->m_settings > r->m_settings) {
        return 1;
    }

    if (l->m_geomFactory < r->m_geomFactory) {
        return -1;
    }

    if (l->m_geomFactory > r->m_geomFactory) {
        return 1;
    }

    if (l->m_transform.d3 > r->m_transform.d3) {
        return 1;
    }

    if (l->m_transform.d3 < r->m_transform.d3) {
        return -1;
    }

    return 0;
}

// VERIFY: abfe3e46 manual
// OFFSET: 0x008A19E0
int32_t Liquid::CRenderer::CompareTransparent(const void* a, const void* b) {
    const Liquid::CInstance* l = *static_cast<Liquid::CInstance* const*>(a);
    const Liquid::CInstance* r = *static_cast<Liquid::CInstance* const*>(b);

    if (l->m_material < r->m_material) {
        return -1;
    }

    if (l->m_material > r->m_material) {
        return 1;
    }

    if (l->m_settings < r->m_settings) {
        return -1;
    }

    if (l->m_settings > r->m_settings) {
        return 1;
    }

    if (l->m_geomFactory < r->m_geomFactory) {
        return -1;
    }

    if (l->m_geomFactory > r->m_geomFactory) {
        return 1;
    }

    if (l->m_transform.d3 > r->m_transform.d3) {
        return -1;
    }

    if (l->m_transform.d3 < r->m_transform.d3) {
        return 1;
    }

    return 0;
}

// VERIFY: f6fe74d2 manual
// OFFSET: 0x008A20C0
void Liquid::CRenderer::Draw(Liquid::CInstance* instance) {
    if (!instance->m_queued) {
        this->m_instances[instance->m_settings->m_transparent != 0].New(instance);
        instance->m_queued = 1;
    }
}

// VERIFY: d17a0633 manual
// OFFSET: 0x008A2240
void Liquid::CRenderer::Flush(const C3Vector& cameraPos, uint32_t pass) {
    CGxLight lights[4];

    for (uint32_t i = 0; i < 4; i++) {
        g_theGxDevicePtr->Light(i, lights[i]);
    }

    TSGrowableArray<Liquid::CInstance*>& instances = this->m_instances[pass];
    qsort(instances.m_data, instances.Count(), sizeof(Liquid::CInstance*), Liquid::CRenderer::s_drawListSorter[pass]);

    uint32_t count = instances.Count();

    for (uint32_t i = 0; i < count; i++) {
        Liquid::CInstance* instance = instances.m_data[i];
        instance->m_material->Render(instance->m_environment, instance->m_geomFactory, instance->m_waveManager, cameraPos, &instance->m_transform, &instance->m_bounds, instance->m_settings);
        instance->m_queued = 0;
    }

    instances.Clear();

    for (uint32_t i = 0; i < 4; i++) {
        if (lights[i].m_flags & 0x1) {
            g_theGxDevicePtr->LightSet(i, lights[i], C3Vector(0.0f, 0.0f, 0.0f));
            g_theGxDevicePtr->LightEnable(i, 1);
        }
    }
}

// VERIFY: 520282aa manual
// OFFSET: 0x005EEB70
void Liquid::CRenderer::Validate() {
}
