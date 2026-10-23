#include "Engine/Source/Services/SysMessage.h"
#include "WoW/Source/Glue/CWOWClientStatus.h"

// INLINED
CWOWClientStatus::CWOWClientStatus(const char* logFileName) {
    this->m_log = nullptr;

    if (!SLogCreate(logFileName, 0, &this->m_log)) {
        SysMsgPrintf(SYSMSG_WARNING, "Cannot create WOWClient log file \"%s\"!", logFileName);
    }
}

// VERIFY: 92e4f487 manual
// OFFSET: 0x004DA580
CWOWClientStatus::CWOWClientStatus(const char* logFileName, int32_t append) {
    this->m_log = nullptr;

    if (logFileName && *logFileName && !SLogCreate(logFileName, append ? 4 : 0, &this->m_log)) {
        SysMsgPrintf(SYSMSG_WARNING, "Cannot create WOWClient log file \"%s\"!", logFileName);
    }
}

// VERIFY: 8eb968ae manual
// OFFSET: [0x004DA2D0, 0x004DA330]
CWOWClientStatus::~CWOWClientStatus() {
    if (!this->m_log) {
        return;
    }

    for (STATUSENTRY* entry = this->m_statusList.Head(); entry; entry = this->m_statusList.Next(entry)) {
        SLogWrite(this->m_log, entry->m_text);
    }

    SLogClose(this->m_log);
}
