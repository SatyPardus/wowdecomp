#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Services/Texture.h"
#include <string>

// VERIFY: d3d12b40 auto
// OFFSET: 0x004E7790
void CCharacterComponent::ApplyMonsterGeosets(CM2Model* model, const CreatureDisplayInfoRec* displayRec) {
    if (!model || !displayRec || !displayRec->m_creatureGeosetData) {
        return;
    }

    for (uint32_t group = 100, shift = 0; group < 900; group += 100, shift += 4) {
        uint32_t geoset = (displayRec->m_creatureGeosetData >> shift) & 0xF;

        if (geoset) {
            model->SetGeometryVisible(group, group + 99, 0);
            model->SetGeometryVisible(group + geoset, group + geoset, 1);
        }
    }

    model->OptimizeVisibleGeometry();
}

// VERIFY: 7234ffe0 manual
// OFFSET: 0x004EA9E0
void CCharacterComponent::ReplaceParticleColor(int32_t particleColorID, CM2Model* model) {
    if (!model || !particleColorID) {
        return;
    }

    ParticleColorRec* colorRec = g_particleColorDB.GetRecord(particleColorID);

    if (colorRec) {
        for (uint32_t i = 0; i < 3; i++) {
            model->ReplaceParticleColor(i + 11, *reinterpret_cast<CImVector*>(&colorRec->m_start[i]), *reinterpret_cast<CImVector*>(&colorRec->m_mid[i]), *reinterpret_cast<CImVector*>(&colorRec->m_end[i]));
        }
    } else {
        CImVector green;
        *reinterpret_cast<uint32_t*>(&green) = 0xFF00FF00;

        for (uint32_t i = 0; i < 3; i++) {
            model->ReplaceParticleColor(i + 11, green, green, green);
        }
    }
}

// VERIFY: 505c73ad manual
// OFFSET: 0x004F20C0
void CCharacterComponent::ReplaceMonsterSkin(CM2Model* model, const CreatureDisplayInfoRec* displayRec, const CreatureModelDataRec* modelRec) {
    if (!model || !displayRec || !modelRec) {
        return;
    }

    CStatus status;
    const std::string modelPath(modelRec->m_modelName);
    const auto slash = modelPath.find_last_of('\\');
    const std::string directory = slash == std::string::npos ? "" : modelPath.substr(0, slash + 1);

    CGxTexFlags flags(GxTex_LinearMipLinear, 1, 1, 0, 0, 0, 1);

    for (uint32_t i = 0; i < 3; i++) {
        if (*displayRec->m_textureVariation[i]) {
            const std::string path = directory + displayRec->m_textureVariation[i];
            CTexture* texture = TextureCreate(path.c_str(), flags, &status, 0);

            if (texture) {
                model->ReplaceTexture(i + 11, texture);
                HandleClose(reinterpret_cast<HOBJECT>(texture));
            }
        }
    }

    ReplaceParticleColor(displayRec->m_particleColorID, model);
}
