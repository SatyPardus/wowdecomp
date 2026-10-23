#ifndef ENGINE_SOURCE_MODEL2_M2DATA_H
#define ENGINE_SOURCE_MODEL2_M2DATA_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Model2/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CRange.h"

template <class T>
class M2Array {
    public:
    /* 0000 */ uint32_t m_count;
    /* 0004 */ uint32_t m_offset;

    T& operator[](uint32_t index) const;
    T* Data() const;
    void Relocate(const void* base);
    void Set(const T* data);
};

class fixed16 {
    public:
    /* 0000 */ int16_t m_value;

    fixed16() = default;
    fixed16(float value);
};

class ubyte4 {
    public:
    /* 0000 */ uint8_t b [4];

    uint8_t operator[](uint32_t index) const;
    operator uint32_t() const;
};

template <class T>
class M2SplineKey {
    public:
    /* 0000 */ T m_value;
    T m_inTan;
    T m_outTan;
};

class M2CompQuat {
    public:
    /* 0000 */ uint16_t m_values [4];
};

class M2TrackBase {
    public:
    /* 0000 */ M2TRACKTYPE m_trackType;
    /* 0002 */ uint16_t m_loopIndex;
    /* 0004 */ M2Array<M2Array<uint32_t> > m_sequenceTimes;
};

template <class T>
class M2Track : public M2TrackBase {
    public:
    /* 000C */ M2Array<M2Array<T> > m_sequenceKeys;
};

template <class T>
class M2PartTrack {
    public:
    /* 0000 */ M2Array<fixed16> m_times;
    /* 0008 */ M2Array<T> m_values;
};

class M2Bounds {
    public:
    /* 0000 */ CAaBox m_extent;
    /* 0018 */ float m_radius;
};

class M2Loop {
    public:
    /* 0000 */ uint32_t m_length;
};

class M2Sequence {
    public:
    /* 0000 */ uint16_t m_id;
    /* 0002 */ uint16_t m_variationIndex;
    /* 0004 */ uint32_t m_duration;
    /* 0008 */ float m_moveSpeed;
    /* 000C */ uint32_t m_flags;
    /* 0010 */ uint32_t m_frequency;
    /* 0014 */ CiRange m_replay;
    /* 001C */ uint32_t m_blendTime;
    /* 0020 */ M2Bounds m_bounds;
    /* 003C */ uint16_t m_variationNext;
    /* 003E */ uint16_t m_aliasNext;
};

class M2CompBone {
    public:
    /* 0000 */ uint32_t m_boneId;
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ uint16_t m_parentIndex;
    /* 000A */ uint16_t m_uDistToParent;
    /* 000C */ uint32_t m_boneNameCRC;
    /* 0010 */ M2Track<C3Vector> m_translationTrack;
    /* 0024 */ M2Track<M2CompQuat> m_rotationTrack;
    /* 0038 */ M2Track<C3Vector> m_scaleTrack;
    /* 004C */ C3Vector m_pivot;
};

class M2VertexPNT2 {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ C3Vector m_normal;
    /* 0018 */ C2Vector m_texcoord [2];
};

class M2Vertex {
    public:
    /* 0000 */ C3Vector m_position;
    /* 000C */ ubyte4 m_weights;
    /* 0010 */ ubyte4 m_indices;
    /* 0014 */ C3Vector m_normal;
    /* 0020 */ C2Vector m_texcoord [2];
};

class M2Color {
    public:
    /* 0000 */ M2Track<C3Vector> m_color;
    /* 0014 */ M2Track<fixed16> m_alpha;
};

class M2Texture {
    public:
    /* 0000 */ uint32_t m_type;
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ M2Array<char> m_filename;
};

class M2TextureWeight {
    public:
    /* 0000 */ M2Track<fixed16> m_weight;
};

class M2TextureTransform {
    public:
    /* 0000 */ M2Track<C3Vector> m_translation;
    /* 0014 */ M2Track<C4Quaternion> m_rotation;
    /* 0028 */ M2Track<C3Vector> m_scaling;
};

class M2Material {
    public:
    /* 0000 */ uint16_t m_flags;
    /* 0002 */ M2BLEND m_blendMode;
};

class M2Attachment {
    public:
    /* 0000 */ uint32_t m_id;
    /* 0004 */ uint16_t m_bone;
    /* 0006 */ uint16_t unused_0006;
    /* 0008 */ C3Vector m_position;
    /* 0014 */ M2Track<uint8_t> m_animateAttached;
};

class M2Event {
    public:
    /* 0000 */ uint32_t m_eventId;
    /* 0004 */ uint32_t m_data;
    /* 0008 */ uint16_t m_bone;
    /* 000A */ uint16_t unused_000A;
    /* 000C */ C3Vector m_position;
    /* 0018 */ M2TrackBase m_enabled;
};

class M2Light {
    public:
    /* 0000 */ uint16_t m_type;
    /* 0002 */ uint16_t m_bone;
    /* 0004 */ C3Vector m_position;
    /* 0010 */ M2Track<C3Vector> m_ambientColor;
    /* 0024 */ M2Track<float> m_ambientIntensity;
    /* 0038 */ M2Track<C3Vector> m_diffuseColor;
    /* 004C */ M2Track<float> m_diffuseIntensity;
    /* 0060 */ M2Track<float> m_attenuationStart;
    /* 0074 */ M2Track<float> m_attenuationEnd;
    /* 0088 */ M2Track<uint8_t> m_visibility;
};

class M2Camera {
    public:
    /* 0000 */ uint32_t m_type;
    /* 0004 */ float m_fieldOfView;
    /* 0008 */ float m_farClip;
    /* 000C */ float m_nearClip;
    /* 0010 */ M2Track<M2SplineKey<C3Vector> > m_positions;
    /* 0024 */ C3Vector m_positionBase;
    /* 0030 */ M2Track<M2SplineKey<C3Vector> > m_targetPositions;
    /* 0044 */ C3Vector m_targetPositionBase;
    /* 0050 */ M2Track<M2SplineKey<float> > m_roll;
};

class M2Ribbon {
    public:
    /* 0000 */ uint32_t m_ribbonId;
    /* 0004 */ uint16_t m_boneIndex;
    /* 0006 */ uint16_t unused_0006;
    /* 0008 */ C3Vector m_position;
    /* 0014 */ M2Array<uint16_t> m_textureIndices;
    /* 001C */ M2Array<uint16_t> m_materialIndices;
    /* 0024 */ M2Track<C3Vector> m_colorTrack;
    /* 0038 */ M2Track<fixed16> m_alphaTrack;
    /* 004C */ M2Track<float> m_heightAboveTrack;
    /* 0060 */ M2Track<float> m_heightBelowTrack;
    /* 0074 */ float m_edgesPerSecond;
    /* 0078 */ float m_edgeLifetime;
    /* 007C */ float m_gravity;
    /* 0080 */ uint16_t m_textureRows;
    /* 0082 */ uint16_t m_textureCols;
    /* 0084 */ M2Track<uint16_t> m_textureSlotTrack;
    /* 0098 */ M2Track<uint8_t> m_visibilityTrack;
    /* 00AC */ int16_t m_priorityPlane;
    /* 00AE */ uint16_t unused_00AE;
};

class M2Particle {
    public:
    /* 0000 */ uint32_t m_particleId;
    /* 0004 */ uint32_t m_flags;
    /* 0008 */ C3Vector m_position;
    /* 0014 */ uint16_t m_boneIndex;
    /* 0016 */ uint16_t m_textureIndex;
    /* 0018 */ M2Array<char> m_geometryMdl;
    /* 0020 */ M2Array<char> m_recursionMdl;
    /* 0028 */ M2PARTICLEBLENDMODE m_blendMode;
    /* 0029 */ uint8_t m_emitterType;
    /* 002A */ uint16_t m_colorIndex;
    /* 002C */ uint16_t unused_002C;
    /* 002E */ int16_t m_priorityPlane;
    /* 0030 */ uint16_t m_rows;
    /* 0032 */ uint16_t m_cols;
    /* 0034 */ M2Track<float> m_speedTrack;
    /* 0048 */ M2Track<float> m_variationTrack;
    /* 005C */ M2Track<float> m_latitudeTrack;
    /* 0070 */ M2Track<float> m_longitudeTrack;
    /* 0084 */ M2Track<float> m_gravityTrack;
    /* 0098 */ M2Track<float> m_lifeTrack;
    /* 00AC */ float m_lifeVariation;
    /* 00B0 */ M2Track<float> m_emissionRateTrack;
    /* 00C4 */ float m_emissionRateVariation;
    /* 00C8 */ M2Track<float> m_widthTrack;
    /* 00DC */ M2Track<float> m_lengthTrack;
    /* 00F0 */ M2Track<float> m_zsourceTrack;
    /* 0104 */ M2PartTrack<C3Vector> m_colorTrack;
    /* 0114 */ M2PartTrack<fixed16> m_alphaTrack;
    /* 0124 */ M2PartTrack<C2Vector> m_scaleTrack;
    /* 0134 */ C2Vector m_scaleVariation;
    /* 013C */ M2PartTrack<uint16_t> m_headCellTrack;
    /* 014C */ M2PartTrack<uint16_t> m_tailCellTrack;
    /* 015C */ float m_tailLength;
    /* 0160 */ float m_twinkleFPS;
    /* 0164 */ float m_twinkleOnOff;
    /* 0168 */ CRange m_twinkleScale;
    /* 0170 */ float m_ivelScale;
    /* 0174 */ float m_drag;
    /* 0178 */ float m_initialSpin;
    /* 017C */ float m_initialSpinVariation;
    /* 0180 */ float m_spin;
    /* 0184 */ float m_spinVariation;
    /* 0188 */ CAaBox m_tumble;
    /* 01A0 */ C3Vector m_windVector;
    /* 01AC */ float m_windTime;
    /* 01B0 */ float m_followSpeed1;
    /* 01B4 */ float m_followScale1;
    /* 01B8 */ float m_followSpeed2;
    /* 01BC */ float m_followScale2;
    /* 01C0 */ M2Array<C3Vector> m_spline;
    /* 01C8 */ M2Track<uint8_t> m_visibilityTrack;
};

class M2Batch {
    public:
    /* 0000 */ uint8_t m_flags;
    /* 0001 */ int8_t m_priorityPlane;
    /* 0002 */ uint16_t m_shader;
    /* 0004 */ uint16_t m_skinSectionIndex;
    /* 0006 */ uint16_t m_geosetIndex;
    /* 0008 */ uint16_t m_colorIndex;
    /* 000A */ uint16_t m_materialIndex;
    /* 000C */ uint16_t m_materialLayer;
    /* 000E */ uint16_t m_textureCount;
    /* 0010 */ uint16_t m_textureComboIndex;
    /* 0012 */ uint16_t m_textureCoordComboIndex;
    /* 0014 */ uint16_t m_textureWeightComboIndex;
    /* 0016 */ uint16_t m_textureTransformComboIndex;
};

class M2SkinSection {
    public:
    /* 0000 */ uint32_t m_skinSectionId;
    /* 0004 */ uint16_t m_vertexStart;
    /* 0006 */ uint16_t m_vertexCount;
    /* 0008 */ uint16_t m_indexStart;
    /* 000A */ uint16_t m_indexCount;
    /* 000C */ uint16_t m_boneCount;
    /* 000E */ uint16_t m_boneComboIndex;
    /* 0010 */ uint16_t m_boneInfluences;
    /* 0012 */ uint16_t m_centerBoneIndex;
    /* 0014 */ C3Vector m_centerPosition;
    /* 0020 */ C3Vector m_sortCenterPosition;
    /* 002C */ float m_sortRadius;
};

class M2Data {
    public:
    /* 0000 */ uint32_t m_magic;
    /* 0004 */ uint32_t m_version;
    /* 0008 */ M2Array<char> m_name;
    /* 0010 */ uint32_t m_flags;
    /* 0014 */ M2Array<M2Loop> m_loops;
    /* 001C */ M2Array<M2Sequence> m_sequences;
    /* 0024 */ M2Array<uint16_t> m_sequenceIdxHashById;
    /* 002C */ M2Array<M2CompBone> m_bones;
    /* 0034 */ M2Array<uint16_t> m_boneIndicesById;
    /* 003C */ M2Array<M2Vertex> m_vertices;
    /* 0044 */ uint32_t m_numSkinProfiles;
    /* 0048 */ M2Array<M2Color> m_colors;
    /* 0050 */ M2Array<M2Texture> m_textures;
    /* 0058 */ M2Array<M2TextureWeight> m_textureWeights;
    /* 0060 */ M2Array<M2TextureTransform> m_textureTransforms;
    /* 0068 */ M2Array<uint16_t> m_textureIndicesById;
    /* 0070 */ M2Array<M2Material> m_materials;
    /* 0078 */ M2Array<uint16_t> m_boneCombos;
    /* 0080 */ M2Array<uint16_t> m_textureCombos;
    /* 0088 */ M2Array<uint16_t> m_textureCoordCombos;
    /* 0090 */ M2Array<uint16_t> m_textureWeightCombos;
    /* 0098 */ M2Array<uint16_t> m_textureTransformCombos;
    /* 00A0 */ M2Bounds m_bounds;
    /* 00BC */ M2Bounds m_collisionBounds;
    /* 00D8 */ M2Array<uint16_t> m_collisionIndices;
    /* 00E0 */ M2Array<C3Vector> m_collisionPositions;
    /* 00E8 */ M2Array<C3Vector> m_collisionFaceNormals;
    /* 00F0 */ M2Array<M2Attachment> m_attachments;
    /* 00F8 */ M2Array<uint16_t> m_attachmentIndicesById;
    /* 0100 */ M2Array<M2Event> m_events;
    /* 0108 */ M2Array<M2Light> m_lights;
    /* 0110 */ M2Array<M2Camera> m_cameras;
    /* 0118 */ M2Array<uint16_t> m_cameraIndicesById;
    /* 0120 */ M2Array<M2Ribbon> m_ribbons;
    /* 0128 */ M2Array<M2Particle> m_particles;
    /* 0130 */ M2Array<uint16_t> m_textureCombinerCombos;
};

class M2SkinProfile {
    public:
    /* 0000 */ uint32_t m_magic;
    /* 0004 */ M2Array<uint16_t> m_vertices;
    /* 000C */ M2Array<uint16_t> m_indices;
    /* 0014 */ M2Array<ubyte4> m_bones;
    /* 001C */ M2Array<M2SkinSection> m_skinSections;
    /* 0024 */ M2Array<M2Batch> m_batches;
    /* 002C */ uint32_t m_boneCountMax;
};

// OFFSET: [0x0081CD50]
template <class T>
inline T& M2Array<T>::operator[](uint32_t index) const {
    return this->Data()[index];
}

template <class T>
inline T* M2Array<T>::Data() const {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(this) + static_cast<intptr_t>(static_cast<int32_t>(this->m_offset)));
}

template <class T>
inline void M2Array<T>::Relocate(const void* base) {
    this->Set(this->m_count ? reinterpret_cast<const T*>(static_cast<const uint8_t*>(base) + this->m_offset) : nullptr);
}

template <class T>
inline void M2Array<T>::Set(const T* data) {
    this->m_offset = data ? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data) - reinterpret_cast<uintptr_t>(this)) : 0;
}

#endif
