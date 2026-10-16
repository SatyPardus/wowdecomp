#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_CGGAMEOBJECT_C_TYPE_TRANSPORT_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_CGGAMEOBJECT_C_TYPE_TRANSPORT_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"

class TransportRotationRec;

struct TransportFrame {
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint32_t m_time;
    /* 000C */ float m_pos[3];
    /* 0018 */ uint32_t m_sequenceId;
};

class CGGameObject_C_Type_Transport : public CGGameObject_C_Type_TransportBase {
    public:
    /* 0040 */ TransportFrame* m_frames;
    /* 0044 */ uint32_t m_frameCount;
    /* 0048 */ uint32_t m_currentFrame;
    /* 004C */ TransportRotationRec* m_rotations;
    /* 0050 */ uint32_t m_rotationCount;
    /* 0054 */ uint32_t m_currentRotation;
    /* 0058 */ uint32_t m_lastSequence;
    /* 005C */ uint32_t m_stateChangeTime;
    /* 0060 */ uint32_t m_stateChangeSequenceTime;

    virtual void UpdateState(int32_t oldState, int32_t newState);
    virtual void PostInitWithStats();
    virtual void Reenable();
    virtual void PostReenable();
    virtual void UpdateMovement(uint32_t time, uint32_t elapsed);
    virtual uint32_t GetSequenceTimeFromRealTime(uint32_t time) const;

    CGGameObject_C_Type_Transport(CGGameObject_C* owner);
    uint32_t GetSequenceTimeFromRealTime(int32_t state, uint32_t time) const;
    void InitializeMovement();
    void InitializeStateChangeTime(uint32_t time);
    void Interpolate(uint32_t time, C3Vector* offset, C4Quaternion* rotation);
    uint32_t GetStateDuration(int32_t state);
    float GetStateProgress(uint32_t time, int32_t state);
    uint32_t ResetStateChange(uint32_t time);
    C3Vector GetMovement(uint32_t time);
    C4Quaternion GetRotation(uint32_t time);
    C44Matrix& UpdateOwnerMatrix();
};

#endif
