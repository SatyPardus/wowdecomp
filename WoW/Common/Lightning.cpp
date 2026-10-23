#include <cmath>
#include <cstddef>
#include <cstdint>
#include "WoW/Common/Lightning.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00DCEC98
static C3Vector s_beamAxisX;

// STATIC: 0x00DCECA4
static C3Vector s_beamDir;

// STATIC: 0x00DCECB0
static C3Vector s_beamAxisZ;

// STATIC: 0x00DCECBC
static C3Vector s_jointAxisX;

// STATIC: 0x00DCECC8
static C3Vector s_jointAxisY;

// STATIC: 0x00DCECD4
static C3Vector s_jointAxisZ;

// STATIC: 0x00DCECE0
static C3Vector s_waveOffset;

// STATIC: 0x00DCECEC
static C3Vector s_arcOffset;

// STATIC: 0x00DCECF8
static CRndSeed s_rndSeed(0);

// VERIFY: 02a001ac auto
// OFFSET: 0x009A8CB0
void CLightning::SetTexture(CTexture* texture) {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }

    this->m_texture = reinterpret_cast<CTexture*>(HandleDuplicate(reinterpret_cast<HOBJECT>(texture)));
}

// VERIFY: 306f8179 auto
// OFFSET: 0x009A8CE0
uint32_t CLightning::DecodeChainEffectsID(uint32_t id) {
    uint32_t bits = id & 0xFEFEFEFE;
    uint32_t high = (bits >> 7) & 0x200000;
    high = ((high | (bits & 0x20000000)) >> 3) | (bits & 0x2000000);
    high = ((high >> 4) | (bits & 0x40000000)) >> 3;
    high |= bits & 0x4000000;

    return (high >> 11) | ((((bits >> 4) & 0xFF800000) | bits) & 0xFFFFFF);
}

// VERIFY: d24cdeda manual
// OFFSET: 0x009A8E80
static float RandomFloat(float min, float max) {
    return (max - min) * CRandom::ufloat_(s_rndSeed) + min;
}

// VERIFY: 0e91c5c7 manual
// OFFSET: 0x009A8EC0
void CLightning::Reset() {
    this->m_flags &= ~0x5;
    this->m_waveAngle = (this->m_chainEffects->m_maxWaveAngle - this->m_chainEffects->m_minWaveAngle) * CRandom::ufloat_(s_rndSeed) + this->m_chainEffects->m_minWaveAngle;
    this->m_waveSpin = (this->m_chainEffects->m_maxWaveSpin - this->m_chainEffects->m_minWaveSpin) * CRandom::ufloat_(s_rndSeed) + this->m_chainEffects->m_minWaveSpin;

    if (this->m_chainEffects->m_flags & 0x400) {
        this->m_wavePhase = this->m_chainEffects->m_wavePhase;
    } else {
        this->m_wavePhase = CRandom::ufloat_(s_rndSeed) * TWO_PI;
    }

    this->m_arcAngle = (this->m_chainEffects->m_maxArcAngle - this->m_chainEffects->m_minArcAngle) * CRandom::ufloat_(s_rndSeed) + this->m_chainEffects->m_minArcAngle;
    this->m_arcSpin = (this->m_chainEffects->m_maxArcSpin - this->m_chainEffects->m_minArcSpin) * CRandom::ufloat_(s_rndSeed) + this->m_chainEffects->m_minArcSpin;
    this->m_flickerTimer = (this->m_chainEffects->m_maxFlickerOnDuration - this->m_chainEffects->m_minFlickerOnDuration) * CRandom::ufloat_(s_rndSeed) + this->m_chainEffects->m_minFlickerOnDuration;
    this->m_pulsePos = 0.0f;

    if (this->m_chainEffects->m_pulseSpeed < 0.0f) {
        this->m_flags |= 0x8;
    }

    this->m_texRandom = CRandom::ufloat_(s_rndSeed);
}

// VERIFY: 954df40c manual
// OFFSET: 0x009A90C0
void CLightning::RandomizeJoint(LightningJoint& joint, float radius) {
    float z = CRandom::sfloat_(s_rndSeed);
    joint.m_pos.x = CRandom::sfloat_(s_rndSeed) * radius;
    joint.m_pos.y = 0.0f;
    joint.m_pos.z = radius * z;

    float speed = CRandom::sfloat_(s_rndSeed);
    joint.m_velocity.x = CRandom::sfloat_(s_rndSeed) * this->m_chainEffects->m_jointMoveSpeed;
    joint.m_velocity.y = 0.0f;
    joint.m_velocity.z = this->m_chainEffects->m_jointMoveSpeed * speed;

    if (this->m_flags & 0x1) {
        joint.m_jumpTimer = CRandom::ufloat_(s_rndSeed) * this->m_chainEffects->m_maxDurationBetweenJointJumps;
    } else {
        joint.m_jumpTimer = (this->m_chainEffects->m_maxDurationBetweenJointJumps - this->m_chainEffects->m_minDurationBetweenJointJumps) * CRandom::ufloat_(s_rndSeed) + this->m_chainEffects->m_minDurationBetweenJointJumps;
    }
}

// VERIFY: 805b3321 manual
// OFFSET: 0x009A9260
static void ComputeXZAxesFromYAxis(C3Vector& axisX, const C3Vector& axisY, C3Vector& axisZ) {
    axisX.x = axisY.y;
    axisX.z = 0.0f;

    if (fabs(axisX.x) >= 0.00000023841858f) {
        axisX.y = -axisY.x;
        float scale = 1.0f / sqrt(axisX.y * axisX.y + axisX.x * axisX.x);
        axisX.x = axisX.x * scale;
        axisX.y = axisX.y * scale;
        axisX.z = 0.0f * scale;
    } else if (axisY.x <= 0.0f) {
        axisX.y = 1.0f;
    } else {
        axisX.y = -1.0f;
    }

    float scale = 1.0f / (sqrt(axisY.z * axisY.z + axisY.y * axisY.y + axisY.x * axisY.x) + 0.0000000099999999f);
    axisZ.x = (axisX.y * axisY.z - axisX.z * axisY.y) * scale;
    axisZ.y = (axisY.x * axisX.z - axisX.x * axisY.z) * scale;
    axisZ.z = (axisX.x * axisY.y - axisX.y * axisY.x) * scale;
}

// VERIFY: c4181630 manual
// OFFSET: 0x009A9350
void CLightning::BuildPoint(C3Vector& point, const C3Vector& base, float t, LightningJoint& joint, float scale) {
    C3Vector smooth;

    if (*reinterpret_cast<int32_t*>(&joint.m_smoothPos.x) == -1) {
        smooth = joint.m_pos;
    } else {
        float smoothness = this->m_chainEffects->m_jointSmoothness;
        smooth.x = joint.m_smoothPos.x * smoothness + joint.m_pos.x * (1.0f - smoothness);
        smooth.y = joint.m_smoothPos.y * smoothness + joint.m_pos.y * (1.0f - smoothness);
        smooth.z = joint.m_smoothPos.z * smoothness + joint.m_pos.z * (1.0f - smoothness);
    }

    joint.m_smoothPos = smooth;

    point.x = s_jointAxisX.x * smooth.x;
    point.y = s_jointAxisX.y * smooth.x;
    point.z = smooth.x * s_jointAxisX.z;

    point.x = (s_jointAxisZ.x * smooth.z + point.x) * scale;
    point.y = (s_jointAxisZ.y * smooth.z + point.y) * scale;
    point.z = scale * (smooth.z * s_jointAxisZ.z + point.z);

    point.x = s_jointAxisY.x * t + point.x;
    point.y = s_jointAxisY.y * t + point.y;
    point.z = t * s_jointAxisY.z + point.z;

    point.x = point.x + base.x;
    point.y = point.y + base.y;
    point.z = point.z + base.z;
}

// VERIFY: e17ab14c manual
// OFFSET: 0x009A9490
void CLightning::ApplyPulsingToVertexData(int32_t& start, int32_t& count) {
    const SpellChainEffectsRec* chainEffects = this->m_chainEffects;

    if (!(chainEffects->m_flags & 0x20)) {
        start = 0;
        count = this->m_positions.Count();
        return;
    }

    float dx = this->m_dstPos.x - this->m_srcPos.x;
    float dy = this->m_dstPos.y - this->m_srcPos.y;
    float dz = this->m_dstPos.z - this->m_srcPos.z;
    float step = sqrt(dx * dx + dy * dy + dz * dz) / static_cast<float>(this->m_points.Count() - 1);

    float fadeOutEnd = this->m_pulsePos - chainEffects->m_pulseFadeLength;
    float fadeInEnd = fadeOutEnd - chainEffects->m_pulseOnLength;
    float fadeInStart = fadeInEnd - chainEffects->m_pulseFadeLength;
    uint32_t alpha = this->m_color.a;
    float fadeInScale = 255.0f / (fadeInEnd - fadeInStart);
    float fadeOutScale = 255.0f / (this->m_pulsePos - fadeOutEnd);
    float alphaScale = alpha * (1.0f / 255.0f);

    start = 0;

    float pos = 0.0f;
    uint32_t i = 0;

    for (; i < this->m_colors.Count(); i += 2) {
        if (pos < fadeInStart) {
            this->m_colors.m_data[i].a = 0;
            start = i;
        } else if (pos < fadeInEnd) {
            this->m_colors.m_data[i].a = static_cast<uint8_t>(CMath::fint_(alphaScale * (pos - fadeInStart) * fadeInScale));
        } else if (fadeOutEnd > pos) {
            this->m_colors.m_data[i].a = this->m_color.a;
        } else if (pos >= this->m_pulsePos) {
            this->m_colors.m_data[i].a = 0;
            this->m_colors.m_data[i + 1].a = 0;
            count = i + 1 - start;
            return;
        } else {
            this->m_colors.m_data[i].a = static_cast<uint8_t>(CMath::fint_(alphaScale * (this->m_pulsePos - pos) * fadeOutScale));
        }

        this->m_colors.m_data[i + 1].a = this->m_colors.m_data[i].a;
        pos += step;
    }

    count = i - start;
}

// VERIFY: 02e84672 auto
// OFFSET: 0x009A96A0
void CLightning::SetSrcPos(const C3Vector& pos) {
    this->m_srcPos = pos;
}

// VERIFY: e57631cc auto
// OFFSET: 0x009A96C0
void CLightning::SetDstPos(const C3Vector& pos) {
    this->m_dstPos = pos;
}

// VERIFY: d866a813 manual
// OFFSET: 0x009A96E0
CLightning* CLightningManager::GetTempPtr(uint32_t index) {
    if (index == 0xFFFFFFFF || index >= this->m_bolts.Count()) {
        return nullptr;
    }

    CLightning* bolt = this->m_bolts.m_data[index];

    if (reinterpret_cast<uintptr_t>(bolt) & 0x1) {
        return nullptr;
    }

    return bolt;
}

// VERIFY: e8370b8f manual
// OFFSET: 0x009A9710
void CLightningManager::Move(uint32_t index, C3Vector* srcPos, C3Vector* dstPos) {
    if (srcPos) {
        this->m_bolts.m_data[index]->m_srcPos = *srcPos;

        if (!dstPos) {
            return;
        }
    } else if (!dstPos) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    this->m_bolts.m_data[index]->m_dstPos = *dstPos;
}

// VERIFY: bb005ebb manual
// OFFSET: 0x009A9770
void CLightningManager::SetBoltVisible(uint32_t index, bool visible) {
    if (index == 0xFFFFFFFF || index >= this->m_bolts.Count()) {
        return;
    }

    CLightning* bolt = this->m_bolts.m_data[index];

    if (reinterpret_cast<uintptr_t>(bolt) & 0x1) {
        return;
    }

    if (visible) {
        bolt->m_flags |= 0x2;
    } else {
        bolt->m_flags &= ~0x2;
    }
}

// VERIFY: d3dce56c auto
// OFFSET: 0x009A97B0
CLightning::CLightning() {
    this->m_texture = nullptr;
    this->m_texOffset = 0.0f;
    this->m_flags = 0;
    this->m_waveAngle = 0.0f;
    this->m_waveSpin = 0.0f;
    this->m_wavePhase = 0.0f;
    this->m_arcAngle = 0.0f;
    this->m_arcSpin = 0.0f;
    this->m_flickerTimer = 0.0f;
    this->m_pulsePos = 0.0f;
    this->m_texRandom = 0.0f;
}

// VERIFY: 462cf42b auto
// OFFSET: 0x009A9860
CLightning::~CLightning() {
    if (this->m_texture) {
        HandleClose(reinterpret_cast<HOBJECT>(this->m_texture));
    }
}

// VERIFY: f0e8bc20 manual
// OFFSET: 0x009A9900
void CLightning::Init(const SpellChainEffectsRec* chainEffects) {
    this->m_chainEffects = chainEffects;
    this->m_color.a = chainEffects->m_alpha;
    this->m_color.r = chainEffects->m_red;
    this->m_color.g = chainEffects->m_green;
    this->m_color.b = chainEffects->m_blue;

    if (!(this->m_chainEffects->m_flags & 0x20)) {
        for (uint32_t i = 0; i < this->m_colors.Count(); i++) {
            this->m_colors.m_data[i] = this->m_color;
        }
    }

    this->m_flags &= ~0x30;
    this->Reset();
}

// VERIFY: b00ea34d manual
// OFFSET: 0x009A9980
void CLightning::PrecomputeBeamValues() {
    s_beamDir.x = this->m_dstPos.x - this->m_srcPos.x;
    s_beamDir.y = this->m_dstPos.y - this->m_srcPos.y;
    s_beamDir.z = this->m_dstPos.z - this->m_srcPos.z;
    ComputeXZAxesFromYAxis(s_beamAxisX, s_beamDir, s_beamAxisZ);

    const SpellChainEffectsRec* chainEffects = this->m_chainEffects;

    if (fabs(chainEffects->m_waveHeight) >= 0.00000023841858f && !(chainEffects->m_flags & 0x8)) {
        float c = CMath::cos_(this->m_waveAngle) * chainEffects->m_waveHeight;
        float cx = s_beamAxisZ.x * c;
        float cy = s_beamAxisZ.y * c;
        float cz = c * s_beamAxisZ.z;
        float s = CMath::sin_(this->m_waveAngle) * chainEffects->m_waveHeight;
        s_waveOffset.x = s_beamAxisX.x * s + cx;
        s_waveOffset.y = s_beamAxisX.y * s + cy;
        s_waveOffset.z = s * s_beamAxisX.z + cz;
    }

    chainEffects = this->m_chainEffects;

    if (fabs(chainEffects->m_arcHeight) >= 0.00000023841858f) {
        float c = CMath::cos_(this->m_arcAngle) * chainEffects->m_arcHeight;
        float cx = s_beamAxisZ.x * c;
        float cy = s_beamAxisZ.y * c;
        float cz = c * s_beamAxisZ.z;
        float s = CMath::sin_(this->m_arcAngle) * chainEffects->m_arcHeight;
        s_arcOffset.x = s_beamAxisX.x * s + cx;
        s_arcOffset.y = s_beamAxisX.y * s + cy;
        s_arcOffset.z = s * s_beamAxisX.z + cz;
    }
}

// VERIFY: 95a70aa3 auto
// OFFSET: 0x009A9B30
void CLightning::ApplyWaveToPosition(C3Vector& position, float t, float length) {
    const SpellChainEffectsRec* chainEffects = this->m_chainEffects;
    float envelope = 1.0f - (t - 0.5f) * (t - 0.5f) * 4.0f;
    float angle = t * chainEffects->m_waveFreq * length + this->m_wavePhase;

    if (chainEffects->m_flags & 0x8) {
        float height = envelope * chainEffects->m_waveHeight;
        float s = CMath::sin_(angle) * height;
        float sx = s_beamAxisZ.x * s;
        float sy = s_beamAxisZ.y * s;
        float sz = s * s_beamAxisZ.z;
        float c = CMath::cos_(angle) * height;
        position.x = s_beamAxisX.x * c + sx + position.x;
        position.y = s_beamAxisX.y * c + sy + position.y;
        position.z = c * s_beamAxisX.z + sz + position.z;
    } else {
        float s = envelope * sin(angle);
        position.x = s_waveOffset.x * s + position.x;
        position.y = s_waveOffset.y * s + position.y;
        position.z = s * s_waveOffset.z + position.z;
    }
}

// VERIFY: bd5180ec manual
// OFFSET: 0x009A9CA0
void CLightning::BuildPoints() {
    uint32_t last = this->m_joints.Count() - 1;
    this->m_points.m_data[0] = this->m_srcPos;
    this->m_points.m_data[last] = this->m_dstPos;

    this->PrecomputeBeamValues();

    s_jointAxisX = s_beamAxisX;
    s_jointAxisY = s_beamDir;
    s_jointAxisZ = s_beamAxisZ;

    for (uint32_t i = 1; i < last; i++) {
        this->BuildPoint(this->m_points.m_data[i], this->m_points.m_data[0], static_cast<float>(i) * (1.0f / static_cast<float>(last)), this->m_joints.m_data[i], 1.0f);
    }
}

// VERIFY: 650e5706 manual
// OFFSET: 0x009A9DC0
void CLightning::BuildPointsUsingHierarchy() {
    int32_t last = this->m_joints.Count() - 1;
    this->m_points.m_data[0] = this->m_srcPos;
    this->m_points.m_data[last] = this->m_dstPos;

    this->PrecomputeBeamValues();

    C3Vector beam;
    beam.x = this->m_dstPos.x - this->m_srcPos.x;
    beam.y = this->m_dstPos.y - this->m_srcPos.y;
    beam.z = this->m_dstPos.z - this->m_srcPos.z;
    s_jointAxisY = beam;
    ComputeXZAxesFromYAxis(s_jointAxisX, s_jointAxisY, s_jointAxisZ);

    const SpellChainEffectsRec* chainEffects = this->m_chainEffects;
    int32_t minorStep = chainEffects->m_jointsPerMinorJoint;
    int32_t majorStep = minorStep * chainEffects->m_minorJointsPerMajorJoint;
    float majorT = 1.0f / static_cast<float>(last);
    float majorScale = chainEffects->m_majorJointScale;

    for (int32_t i = majorStep; i < last; i += majorStep) {
        this->BuildPoint(this->m_points.m_data[i], this->m_points.m_data[0], static_cast<float>(i) * majorT, this->m_joints.m_data[i], majorScale);
    }

    float minorScale = chainEffects->m_minorJointScale;
    int32_t next = majorStep < last ? majorStep : last;

    C3Vector base = this->m_points.m_data[0];
    s_jointAxisY.x = this->m_points.m_data[next].x - base.x;
    s_jointAxisY.y = this->m_points.m_data[next].y - base.y;
    s_jointAxisY.z = this->m_points.m_data[next].z - base.z;
    ComputeXZAxesFromYAxis(s_jointAxisX, s_jointAxisY, s_jointAxisZ);

    float minorStepF = static_cast<float>(minorStep);
    float step = minorStepF / static_cast<float>(next);
    float t = step;

    if (minorStep < last) {
        for (int32_t i = minorStep; i < last; i += minorStep) {
            if (i == next) {
                next = i + majorStep;

                if (next >= last) {
                    next = last;
                }

                base = this->m_points.m_data[i];
                s_jointAxisY.x = this->m_points.m_data[next].x - base.x;
                s_jointAxisY.y = this->m_points.m_data[next].y - base.y;
                s_jointAxisY.z = this->m_points.m_data[next].z - base.z;
                ComputeXZAxesFromYAxis(s_jointAxisX, s_jointAxisY, s_jointAxisZ);
                step = minorStepF / static_cast<float>(next - i);
                t = step;
            } else {
                this->BuildPoint(this->m_points.m_data[i], base, t, this->m_joints.m_data[i], minorScale);
                t += step;
            }
        }

        next = minorStep;
    } else {
        next = last;
    }

    base = this->m_points.m_data[0];
    s_jointAxisY.x = this->m_points.m_data[next].x - base.x;
    s_jointAxisY.y = this->m_points.m_data[next].y - base.y;
    s_jointAxisY.z = this->m_points.m_data[next].z - base.z;
    ComputeXZAxesFromYAxis(s_jointAxisX, s_jointAxisY, s_jointAxisZ);

    step = 1.0f / static_cast<float>(next);
    t = step;

    for (int32_t i = 1; i < last; i++) {
        if (i == next) {
            next = i + minorStep;

            if (next >= last) {
                next = last;
            }

            base = this->m_points.m_data[i];
            s_jointAxisY.x = this->m_points.m_data[next].x - base.x;
            s_jointAxisY.y = this->m_points.m_data[next].y - base.y;
            s_jointAxisY.z = this->m_points.m_data[next].z - base.z;
            ComputeXZAxesFromYAxis(s_jointAxisX, s_jointAxisY, s_jointAxisZ);
            step = 1.0f / static_cast<float>(next - i);
            t = step;
        } else {
            this->BuildPoint(this->m_points.m_data[i], base, t, this->m_joints.m_data[i], 1.0f);
            t += step;
        }
    }
}

// VERIFY: c9b19284 manual
// OFFSET: 0x009AA210
void CLightning::Render(uint32_t index, const C3Vector& cameraPos) {
    this->m_flags &= ~0x30;

    if (!(this->m_flags & 0x2) || (this->m_flags & 0x4)) {
        return;
    }

    if (this->m_coordUpdate) {
        C3Vector srcPos = this->m_srcPos;
        C3Vector dstPos = this->m_dstPos;
        this->m_coordUpdate(this->m_coordUpdateParam, index, &srcPos, &dstPos);
        this->m_srcPos = srcPos;
        this->m_dstPos = dstPos;
    }

    if (this->m_chainEffects->m_flags & 0x2) {
        this->BuildPointsUsingHierarchy();
    } else {
        this->BuildPoints();
    }

    uint32_t pointCount = this->m_points.Count();

    this->m_positions.m_data[0] *= 0.0f;
    this->m_positions.m_data[1] *= 0.0f;

    C3Vector prev = this->m_points.m_data[0];
    this->m_bounds.b = prev;
    this->m_bounds.t = prev;

    const SpellChainEffectsRec* chainEffects = this->m_chainEffects;
    bool wave = chainEffects->m_waveHeight != 0.0f;
    bool arc = chainEffects->m_arcHeight != 0.0f;

    float dx = this->m_dstPos.x - this->m_srcPos.x;
    float dy = this->m_dstPos.y - this->m_srcPos.y;
    float dz = this->m_dstPos.z - this->m_srcPos.z;
    float length = sqrt(dx * dx + dz * dz + dy * dy);
    float width = chainEffects->m_width;
    float step = 1.0f / static_cast<float>(pointCount - 1);

    if (!(chainEffects->m_flags & 0x80)) {
        float t = step;
        uint32_t end = 2 * pointCount - 2;

        for (uint32_t i = 0; i < end; i += 2) {
            C3Vector point = this->m_points.m_data[(i + 2) >> 1];

            if (wave) {
                this->ApplyWaveToPosition(point, t, length);
            }

            if (arc) {
                float envelope = 1.0f - (t - 0.5f) * (t - 0.5f) * 4.0f;
                point.x = s_arcOffset.x * envelope + point.x;
                point.y = envelope * s_arcOffset.y + point.y;
                point.z = envelope * s_arcOffset.z + point.z;
            }

            C3Vector view;
            view.x = (point.x + prev.x) * 0.5f - cameraPos.x;
            view.y = (point.y + prev.y) * 0.5f - cameraPos.y;
            view.z = (point.z + prev.z) * 0.5f - cameraPos.z;

            C3Vector segment;
            segment.x = point.x - prev.x;
            segment.y = point.y - prev.y;
            segment.z = point.z - prev.z;

            C3Vector normal;
            normal.x = segment.y * view.z - view.y * segment.z;
            normal.y = segment.z * view.x - view.z * segment.x;
            normal.z = segment.x * view.y - segment.y * view.x;

            float normalLength = sqrt(normal.z * normal.z + normal.x * normal.x + normal.y * normal.y);

            if (normalLength > 0.001f) {
                float scale = 1.0f / normalLength;
                normal.x = normal.x * scale;
                normal.y = normal.y * scale;
                normal.z = scale * normal.z;
            }

            C3Vector offset;
            offset.x = normal.x * width;
            offset.y = normal.y * width;
            offset.z = width * normal.z;

            C3Vector* positions = &this->m_positions.m_data[i];

            positions[0].x = positions[0].x + (offset.x + prev.x) * 0.5f;
            positions[0].y = (offset.y + prev.y) * 0.5f + positions[0].y;
            positions[0].z = (offset.z + prev.z) * 0.5f + positions[0].z;

            positions = &this->m_positions.m_data[i];

            positions[1].x = (prev.x - offset.x) * 0.5f + positions[1].x;
            positions[1].y = (prev.y - offset.y) * 0.5f + positions[1].y;
            positions[1].z = (prev.z - offset.z) * 0.5f + positions[1].z;

            positions = &this->m_positions.m_data[i];

            positions[2].x = (offset.x + point.x) * 0.5f;
            positions[2].y = (offset.y + point.y) * 0.5f;
            positions[2].z = (offset.z + point.z) * 0.5f;

            positions = &this->m_positions.m_data[i];

            positions[3].x = (point.x - offset.x) * 0.5f;
            positions[3].y = (point.y - offset.y) * 0.5f;
            positions[3].z = (point.z - offset.z) * 0.5f;

            prev = point;

            const C3Vector& a = this->m_positions.m_data[i];

            if (this->m_bounds.b.x > a.x) {
                this->m_bounds.b.x = a.x;
            }

            if (this->m_bounds.b.y > a.y) {
                this->m_bounds.b.y = a.y;
            }

            if (this->m_bounds.b.z > a.z) {
                this->m_bounds.b.z = a.z;
            }

            if (this->m_bounds.t.x < a.x) {
                this->m_bounds.t.x = a.x;
            }

            if (this->m_bounds.t.y < a.y) {
                this->m_bounds.t.y = a.y;
            }

            if (this->m_bounds.t.z < a.z) {
                this->m_bounds.t.z = a.z;
            }

            const C3Vector& b = this->m_positions.m_data[i + 1];

            if (this->m_bounds.b.x > b.x) {
                this->m_bounds.b.x = b.x;
            }

            if (this->m_bounds.b.y > b.y) {
                this->m_bounds.b.y = b.y;
            }

            if (this->m_bounds.b.z > b.z) {
                this->m_bounds.b.z = b.z;
            }

            if (this->m_bounds.t.x < b.x) {
                this->m_bounds.t.x = b.x;
            }

            if (this->m_bounds.t.y < b.y) {
                this->m_bounds.t.y = b.y;
            }

            if (this->m_bounds.t.z < b.z) {
                this->m_bounds.t.z = b.z;
            }

            t += step;
        }
    } else {
        C3Vector view;
        view.x = (this->m_dstPos.x + this->m_srcPos.x) * 0.5f - cameraPos.x;
        view.y = (this->m_dstPos.y + this->m_srcPos.y) * 0.5f - cameraPos.y;
        view.z = (this->m_dstPos.z + this->m_srcPos.z) * 0.5f - cameraPos.z;

        C3Vector beam;
        beam.x = this->m_dstPos.x - this->m_srcPos.x;
        beam.y = this->m_dstPos.y - this->m_srcPos.y;
        beam.z = this->m_dstPos.z - this->m_srcPos.z;

        C3Vector normal;
        normal.x = beam.y * view.z - beam.z * view.y;
        normal.y = beam.z * view.x - view.z * beam.x;
        normal.z = beam.x * view.y - view.x * beam.y;

        float normalLength = sqrt(normal.z * normal.z + normal.y * normal.y + normal.x * normal.x);

        if (normalLength > 0.001f) {
            float scale = 1.0f / normalLength;
            normal.x = normal.x * scale;
            normal.y = normal.y * scale;
            normal.z = normal.z * scale;
        }

        C3Vector offset;
        offset.x = normal.x * width;
        offset.y = normal.y * width;
        offset.z = width * normal.z;

        float t = 0.0f;

        for (uint32_t i = 0; i < 2 * pointCount; i += 2) {
            C3Vector point = this->m_points.m_data[i >> 1];

            if (wave) {
                this->ApplyWaveToPosition(point, t, length);
            }

            if (arc) {
                float envelope = 1.0f - (t - 0.5f) * (t - 0.5f) * 4.0f;
                point.x = s_arcOffset.x * envelope + point.x;
                point.y = envelope * s_arcOffset.y + point.y;
                point.z = envelope * s_arcOffset.z + point.z;
            }

            C3Vector* positions = &this->m_positions.m_data[i];

            positions[0].x = offset.x + point.x;
            positions[0].y = offset.y + point.y;
            positions[0].z = offset.z + point.z;

            positions = &this->m_positions.m_data[i];

            positions[1].x = point.x - offset.x;
            positions[1].y = point.y - offset.y;
            positions[1].z = point.z - offset.z;

            const C3Vector& a = this->m_positions.m_data[i];

            if (this->m_bounds.b.x > a.x) {
                this->m_bounds.b.x = a.x;
            }

            if (this->m_bounds.b.y > a.y) {
                this->m_bounds.b.y = a.y;
            }

            if (this->m_bounds.b.z > a.z) {
                this->m_bounds.b.z = a.z;
            }

            if (this->m_bounds.t.x < a.x) {
                this->m_bounds.t.x = a.x;
            }

            if (this->m_bounds.t.y < a.y) {
                this->m_bounds.t.y = a.y;
            }

            if (this->m_bounds.t.z < a.z) {
                this->m_bounds.t.z = a.z;
            }

            const C3Vector& b = this->m_positions.m_data[i + 1];

            if (this->m_bounds.b.x > b.x) {
                this->m_bounds.b.x = b.x;
            }

            if (this->m_bounds.b.y > b.y) {
                this->m_bounds.b.y = b.y;
            }

            if (this->m_bounds.b.z > b.z) {
                this->m_bounds.b.z = b.z;
            }

            if (this->m_bounds.t.x < b.x) {
                this->m_bounds.t.x = b.x;
            }

            if (this->m_bounds.t.y < b.y) {
                this->m_bounds.t.y = b.y;
            }

            if (this->m_bounds.t.z < b.z) {
                this->m_bounds.t.z = b.z;
            }

            t += step;
        }
    }

    this->m_positions.m_data[1] = this->m_points.m_data[0];
    this->m_positions.m_data[0] = this->m_positions.m_data[1];

    uint32_t vertexCount = 2 * pointCount;
    this->m_positions.m_data[vertexCount - 1] = this->m_points.m_data[pointCount - 1];
    this->m_positions.m_data[vertexCount - 2] = this->m_positions.m_data[vertexCount - 1];

    const C3Vector& end = this->m_positions.m_data[vertexCount - 2];

    if (this->m_bounds.b.x > end.x) {
        this->m_bounds.b.x = end.x;
    }

    if (this->m_bounds.b.y > end.y) {
        this->m_bounds.b.y = end.y;
    }

    if (this->m_bounds.b.z > end.z) {
        this->m_bounds.b.z = end.z;
    }

    if (this->m_bounds.t.x < end.x) {
        this->m_bounds.t.x = end.x;
    }

    if (this->m_bounds.t.y < end.y) {
        this->m_bounds.t.y = end.y;
    }

    if (this->m_bounds.t.z < end.z) {
        this->m_bounds.t.z = end.z;
    }

    int32_t start;
    int32_t count;
    this->ApplyPulsingToVertexData(start, count);

    if (start == 0) {
        this->m_flags |= 0x10;
    }

    if (start + count >= static_cast<int32_t>(this->m_positions.Count())) {
        this->m_flags |= 0x20;
    }

    if (count <= 2 || World::FrustumCull(this->m_bounds)) {
        return;
    }

    chainEffects = this->m_chainEffects;
    float texScale;

    if (chainEffects->m_flags & 0x100) {
        texScale = length / chainEffects->m_textureLength;
    } else {
        texScale = 1.0f;
    }

    float texOffset = this->m_texRandom + this->m_texOffset;

    if ((chainEffects->m_flags & 0x100) && (chainEffects->m_flags & 0x200)) {
        texOffset = texOffset + length / chainEffects->m_textureLength;
    }

    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_Tex0];
    stack.m_dirty = 1;
    stack.m_flags[stack.m_level] &= ~CGxMatrixStack::F_Identity;
    stack.m_mtx[stack.m_level] = C44Matrix(texScale, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -texOffset, 0.0f, 0.0f, 1.0f);

    CImVector emissive;
    emissive.b = this->m_color.b;
    emissive.g = this->m_color.g;
    emissive.r = this->m_color.r;
    emissive.a = 0xFF;
    g_theGxDevicePtr->RsSet(GxRs_MatEmissive, *reinterpret_cast<int32_t*>(&emissive));
    g_theGxDevicePtr->RsSet(GxRs_DepthWrite, static_cast<uint8_t>(this->m_chainEffects->m_blendMode) == 2);
    g_theGxDevicePtr->RsSet(GxRs_BlendingMode, static_cast<int32_t>(static_cast<uint8_t>(this->m_chainEffects->m_blendMode)));
    GxUpdateAlphaRef(GxRs_AlphaRef);

    CGxTex* gxTex = TextureGetGxTex(this->m_texture, 0, nullptr);

    if (!gxTex) {
        return;
    }

    g_theGxDevicePtr->RsSet(GxRs_Texture0, gxTex);

    static C3Vector s_normal(0.0f, 0.0f, 1.0f);

    GxRsSet(GxRs_TextureShader0, 1);
    GxPrimLockVertexPtrs(this->m_positions.Count(), this->m_positions.m_data, sizeof(C3Vector), &s_normal, 0, this->m_colors.m_data, sizeof(CImVector), nullptr, 0, this->m_texCoords.m_data, sizeof(C2Vector), nullptr, 0);
    GxDrawLockedElements(GxPrim_TriangleStrip, count, &this->m_indices.m_data[start]);
    GxPrimUnlockVertexPtrs();
}

// VERIFY: 75ea64eb auto
// OFFSET: 0x009AAE80
CLightningManager::CLightningManager() {
}

// VERIFY: 2597f7c8 manual
// OFFSET: 0x009AAEA0
CLightningManager::~CLightningManager() {
    for (uint32_t i = this->m_bolts.Count(); i; i--) {
        CLightning* bolt = reinterpret_cast<CLightning*>(reinterpret_cast<uintptr_t>(this->m_bolts.m_data[i - 1]) & ~0x1);

        if (bolt) {
            delete bolt;
        }
    }
}

// VERIFY: 193d56e8 manual
// OFFSET: 0x009AAFB0
uint32_t CLightningManager::Add(void (*coordUpdate)(void*, uint32_t, C3Vector*, C3Vector*), void* param, const SpellChainEffectsRec* chainEffects) {
    uint32_t index;

    if (this->m_freeList.Count()) {
        index = *this->m_freeList.Top();
        this->m_bolts.m_data[index] = reinterpret_cast<CLightning*>(reinterpret_cast<uintptr_t>(this->m_bolts.m_data[index]) & ~0x1);
        this->m_freeList.SetCount(this->m_freeList.Count() - 1);
        this->m_bolts.m_data[index]->Init(chainEffects);
    } else {
        index = this->m_bolts.Count();
        CLightning* bolt = new (__FILE__, __LINE__) CLightning();
        this->m_bolts.New(bolt);
        this->m_bolts.m_data[index]->Init(chainEffects);
    }

    CLightning* bolt = this->m_bolts.m_data[index];
    bolt->m_coordUpdate = coordUpdate;
    bolt->m_coordUpdateParam = param;

    return index;
}

// VERIFY: 26737101 manual
// OFFSET: 0x009AB070
void CLightningManager::Render(const C3Vector& cameraPos) {
    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->XformPush(GxXform_Tex0);

    C3Vector move(-cameraPos.x, -cameraPos.y, -cameraPos.z);
    CGxMatrixStack& world = g_theGxDevicePtr->m_xforms[GxXform_World];
    world.m_dirty = 1;
    world.m_flags[world.m_level] &= ~CGxMatrixStack::F_Identity;
    world.m_mtx[world.m_level].Translate(move);

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsSet(GxRs_MatDiffuse, static_cast<int32_t>(0xFF000000));
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_Fog, 0);
    g_theGxDevicePtr->RsSet(GxRs_ColorOp0, GxTexOp_Mod);
    g_theGxDevicePtr->RsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    g_theGxDevicePtr->RsSet(GxRs_ColorWrite, 15);

    for (int32_t layer = 0; layer < 4; layer++) {
        for (uint32_t i = this->m_bolts.Count(); i;) {
            i--;

            CLightning* bolt = this->m_bolts.m_data[i];

            if (reinterpret_cast<uintptr_t>(bolt) & 0x1) {
                continue;
            }

            if (bolt->m_chainEffects && bolt->m_chainEffects->m_renderLayer == layer) {
                bolt->Render(i, cameraPos);
            }
        }
    }

    g_theGxDevicePtr->RsPop();

    CGxMatrixStack& worldStack = g_theGxDevicePtr->m_xforms[GxXform_World];
    worldStack.m_dirty = 1;

    if (worldStack.m_level) {
        worldStack.m_level--;
    }

    CGxMatrixStack& texStack = g_theGxDevicePtr->m_xforms[GxXform_Tex0];
    texStack.m_dirty = 1;

    if (texStack.m_level) {
        texStack.m_level--;
    }
}

// VERIFY: ac4c6214 auto
// OFFSET: 0x009AB2B0
void CLightningManager::Remove(uint32_t index) {
    this->m_bolts.m_data[index] = reinterpret_cast<CLightning*>(reinterpret_cast<uintptr_t>(this->m_bolts.m_data[index]) | 0x1);
    this->m_freeList.Add(1, &index);
}

// VERIFY: 286b8892 manual
// OFFSET: 0x009AB2E0
void CLightning::UpdateJointCount(float length, float radius) {
    int32_t count;

    if (this->m_chainEffects->m_flags & 0x1) {
        count = this->m_chainEffects->m_jointCount;
    } else {
        count = static_cast<int32_t>(length / this->m_chainEffects->m_avgSegLen) + 2;
    }

    count++;

    if (count > 1000) {
        count = 1000;
    } else if (count < 2) {
        count = 2;
    }

    int32_t oldCount;

    if (this->m_flags & 0x1) {
        oldCount = this->m_joints.Count();
    } else {
        oldCount = 0;
    }

    this->m_joints.SetCount(count);
    this->m_points.SetCount(count);

    for (int32_t i = oldCount; i < count; i++) {
        this->RandomizeJoint(this->m_joints.m_data[i], radius);
        *reinterpret_cast<int32_t*>(&this->m_joints.m_data[i].m_smoothPos.x) = -1;
    }

    this->m_flags |= 0x1;
}

// VERIFY: 5730309e manual
// OFFSET: 0x009AB3B0
void CLightning::Update(float elapsed) {
    if (!(this->m_flags & 0x2)) {
        return;
    }

    this->m_flickerTimer -= elapsed;

    if (this->m_flags & 0x4) {
        if (this->m_flickerTimer > 0.0f) {
            return;
        }

        this->Reset();
    } else if ((this->m_chainEffects->m_flags & 0x10) && this->m_flickerTimer <= 0.0f) {
        this->m_flickerTimer = RandomFloat(this->m_chainEffects->m_minFlickerOffDuration, this->m_chainEffects->m_maxFlickerOffDuration);
        this->m_flags |= 0x4;
    }

    const SpellChainEffectsRec* chainEffects = this->m_chainEffects;

    if (chainEffects->m_texCoordScale != 0.0f) {
        this->m_texOffset = fmodf(chainEffects->m_texCoordScale * elapsed + this->m_texOffset, 1.0f);
    } else {
        this->m_texOffset = 0.0f;
    }

    float dx = this->m_dstPos.x - this->m_srcPos.x;
    float dy = this->m_dstPos.y - this->m_srcPos.y;
    float dz = this->m_dstPos.z - this->m_srcPos.z;
    float length = sqrt(dz * dz + dy * dy + dx * dx);

    if (length > 10000.0f) {
        length = 10000.0f;
    }

    float radius = chainEffects->m_noiseScale * length + chainEffects->m_jointOffsetRadius;
    this->UpdateJointCount(length, radius);

    uint32_t vertexCount = 2 * this->m_joints.Count();
    float texStep = 1.0f / static_cast<float>(vertexCount - 2);

    if (vertexCount != this->m_texCoords.Count()) {
        this->m_positions.SetCount(vertexCount);
        this->m_texCoords.SetCount(vertexCount);
        this->m_colors.SetCount(vertexCount);
        this->m_indices.SetCount(vertexCount);

        for (uint32_t i = 0; i < vertexCount; i += 2) {
            float u = static_cast<float>(i) * texStep;
            this->m_texCoords.m_data[i].x = u;
            this->m_texCoords.m_data[i].y = 0.0f;
            this->m_texCoords.m_data[i + 1].x = u;
            this->m_texCoords.m_data[i + 1].y = 1.0f;
            this->m_colors.m_data[i] = this->m_color;
            this->m_colors.m_data[i + 1] = this->m_color;
            this->m_indices.m_data[i] = i;
            this->m_indices.m_data[i + 1] = i + 1;
        }

        this->m_texCoords.m_data[1].x = 0.0f;
        this->m_texCoords.m_data[1].y = 0.5f;
        this->m_texCoords.m_data[0].x = 0.0f;
        this->m_texCoords.m_data[0].y = 0.5f;
        this->m_texCoords.m_data[vertexCount - 1].x = 1.0f;
        this->m_texCoords.m_data[vertexCount - 1].y = 0.5f;
        this->m_texCoords.m_data[vertexCount - 2].x = 1.0f;
        this->m_texCoords.m_data[vertexCount - 2].y = 0.5f;
    }

    int32_t jointCount = this->m_joints.Count();

    for (int32_t i = 1; i < jointCount; i++) {
        LightningJoint& joint = this->m_joints.m_data[i];
        joint.m_jumpTimer -= elapsed;

        if ((this->m_chainEffects->m_flags & 0x4) && joint.m_jumpTimer < 0.0f) {
            this->RandomizeJoint(joint, radius);
        } else {
            joint.m_pos.x = joint.m_velocity.x * elapsed + joint.m_pos.x;
            joint.m_pos.y = joint.m_velocity.y * elapsed + joint.m_pos.y;
            joint.m_pos.z = joint.m_velocity.z * elapsed + joint.m_pos.z;
        }
    }

    chainEffects = this->m_chainEffects;
    this->m_waveAngle = this->m_waveSpin * elapsed + this->m_waveAngle;
    this->m_wavePhase = this->m_wavePhase - chainEffects->m_waveSpeed * elapsed;
    this->m_arcAngle = this->m_arcSpin * elapsed + this->m_arcAngle;

    if (!(chainEffects->m_flags & 0x20)) {
        return;
    }

    this->m_pulsePos = chainEffects->m_pulseSpeed * elapsed + this->m_pulsePos;

    if (!(this->m_flags & 0x8) && !(chainEffects->m_flags & 0x40)) {
        return;
    }

    float pulseLength = chainEffects->m_pulseFadeLength + chainEffects->m_pulseFadeLength + chainEffects->m_pulseOnLength;

    if (this->m_pulsePos - pulseLength >= length) {
        this->Reset();
        this->m_flags &= ~0x8;
    } else if (this->m_pulsePos < 0.0f) {
        this->Reset();
        this->m_flags &= ~0x8;
        this->m_pulsePos = pulseLength + length;
    }
}

// VERIFY: c2448686 auto
// OFFSET: 0x009AB730
void CLightningManager::Update(float elapsed) {
    for (uint32_t i = this->m_bolts.Count(); i;) {
        i--;

        if (!(reinterpret_cast<uintptr_t>(this->m_bolts.m_data[i]) & 0x1)) {
            this->m_bolts.m_data[i]->Update(elapsed);
        }
    }
}
