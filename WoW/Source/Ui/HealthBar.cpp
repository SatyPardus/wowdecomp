#include "HealthBar.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"

static const float HPBAR_RED_THRESHOLD = 0.2f;

// STATIC: 0x00DCEC58
CDataAllocator CGSimpleHealthBar::s_simpleHealthBarHeap(sizeof(CGSimpleHealthBar), 5);

// INLINED
void CGSimpleHealthBar::operator delete(void* ptr) {
    if (ptr) {
        CGSimpleHealthBar::s_simpleHealthBarHeap.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 56bf9a4e manual
// OFFSET: 0x009900E0
static int32_t SimpleHealthUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        CGSimpleHealthBar* bar = static_cast<CGSimpleHealthBar*>(param);
        bar->SetMinMaxValues(0.0f, static_cast<float>(static_cast<int32_t>(unit->m_unit->m_unitFieldMaxhealth)));
        bar->SetValue(static_cast<float>(unit->m_unit->m_unitFieldHealth));
    }

    return 1;
}

// VERIFY: a5aa4a8c auto
// OFFSET: 0x00990060
CGSimpleHealthBar::CGSimpleHealthBar(CSimpleFrame* parent) : CSimpleStatusBar(parent) {
    this->m_guid.m_guid = 0;
    this->m_autoColor = 1;
}

// VERIFY: 89bed84f auto
// OFFSET: 0x00990310
CGSimpleHealthBar::~CGSimpleHealthBar() {
    if (ClntObjMgrGetCurrent()) {
        this->RemoveMirrorHandlers();
    }

    this->m_guid.m_guid = 0;
}

// VERIFY: 17ea5cc9 auto
// OFFSET: 0x009900A0
void CGSimpleHealthBar::SetStatusBarColor(const CImVector& color) {
    if (this->m_barTexture) {
        this->m_barTexture->SetVertexColor(color);
    }

    this->m_autoColor = 0;
}

// VERIFY: 59245b1f manual
// OFFSET: 0x00990150
void CGSimpleHealthBar::SetValue(float value) {
    CSimpleStatusBar::SetValue(value);

    if (!this->m_autoColor) {
        return;
    }

    float percent = this->GetAnimValue();

    CImVector color;
    color.b = 0;
    color.g = percent <= HPBAR_RED_THRESHOLD ? 0 : 0xFF;
    color.r = percent <= HPBAR_RED_THRESHOLD ? 0xFF : 0;
    color.a = 0xFF;
    this->SetStatusBarColor(color);
    this->m_autoColor = 1;
}

// VERIFY: 552e8304 auto
// OFFSET: 0x009901D0
void CGSimpleHealthBar::InstallMirrorHandlers() {
    if (!this->m_guid) {
        return;
    }

    ClntObjMgrSetObjMirrorHandler(this->m_guid, ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), sizeof(uint32_t), SimpleHealthUpdateHandler, this, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetObjMirrorHandler(this->m_guid, ID_UNIT, offsetof(CGUnitData, m_unitFieldMaxhealth), sizeof(uint32_t), SimpleHealthUpdateHandler, this, HANDLER_PRIORITY_NORMAL, 0);
}

// VERIFY: 2cae92df auto
// OFFSET: 0x00990230
void CGSimpleHealthBar::RemoveMirrorHandlers() {
    if (!this->m_guid) {
        return;
    }

    ClntObjMgrUnsetObjMirrorHandler(this->m_guid, ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), SimpleHealthUpdateHandler, this);
    ClntObjMgrUnsetObjMirrorHandler(this->m_guid, ID_UNIT, offsetof(CGUnitData, m_unitFieldMaxhealth), SimpleHealthUpdateHandler, this);
}

// VERIFY: d35be450 auto
// OFFSET: 0x00990280
void CGSimpleHealthBar::SetUnit(CGUnit_C* unit) {
    if (ClntObjMgrGetCurrent()) {
        this->RemoveMirrorHandlers();
    }

    if (!unit) {
        this->m_guid.m_guid = 0;
        return;
    }

    this->m_guid = unit->m_obj->m_guid;
    this->InstallMirrorHandlers();
    this->SetMinMaxValues(0.0f, static_cast<float>(static_cast<int32_t>(unit->m_unit->m_unitFieldMaxhealth)));
    this->SetValue(static_cast<float>(unit->m_unit->m_unitFieldHealth));
}
