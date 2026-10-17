#ifndef ENGINE_SOURCE_STORM_H_STPL_H
#define ENGINE_SOURCE_STORM_H_STPL_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <typeinfo>
#include "Engine/Source/Storm/H/storm.h"

// Allocation labels only; preserve the original MSVC spelling on Windows.
inline const char* StormTypeName(const std::type_info& type) {
#if defined(_MSC_VER)
    return type.raw_name();
#else
    return type.name();
#endif
}

template <class T>
class TSLink {
    public:
    /* 0000 */ TSLink<T>* m_prevLink;
    /* 0004 */ T* m_next;

    TSLink();
    ~TSLink();
    int32_t IsLinked();
    T* Next();
    TSLink<T>* NextLink(ptrdiff_t linkoffset);
    T* Prev();
    T* RawNext();
    void Unlink();
};

template <class T>
class TSGetLink {
    public:
    static TSLink<T>* Link(T* nodeptr, ptrdiff_t linkoffset);
};

template <class T>
class TSGetExplicitLink {
    public:
    static TSLink<T>* Link(T* nodeptr, ptrdiff_t linkoffset);
};

template <class T, class TGetLink = TSGetLink<T> >
class TSList {
    public:
    /* 0000 */ ptrdiff_t m_linkoffset;
    /* 0004 */ TSLink<T> m_terminator;

    static int32_t IsEnd(T* ptr);

    TSList();
    TSList(const TSList<T, TGetLink>& source);
    ~TSList();
    void ChangeLinkOffset(ptrdiff_t linkoffset);
    void Clear();
    void Combine(TSList<T, TGetLink>* list, uint32_t linktype, T* existingptr);
    T* DeleteNode(T* ptr);
    T* Head();
    void InitializeTerminator();
    int32_t IsEmpty();
    int32_t IsLinked(T* ptr);
    TSLink<T>* Link(T* ptr);
    void LinkNode(T* ptr, uint32_t linktype, T* existingptr);
    void LinkToHead(T* ptr);
    void LinkToTail(T* ptr);
    T* NewNode(uint32_t location, size_t extrabytes, uint32_t flags);
    T* Next(T* ptr);
    T* Prev(T* ptr);
    T* RawNext(T* ptr);
    void SetLinkOffset(ptrdiff_t linkoffset);
    T* Tail();
    void UnlinkAll();
    void UnlinkNode(T* ptr);
};

template <class T, int offset = static_cast<int>(0xDDDDDDDD)>
class TSExplicitList : public TSList<T, TSGetExplicitLink<T> > {
    public:
    TSExplicitList();
};

template <class T>
class TSLinkedNode {
    public:
    /* 0000 */ TSLink<T> m_link;

    T* Next();
    T* RawNext();
};

template <class T>
class TSBaseArray {
    public:
    /* 0000 */ uint32_t m_alloc;
    /* 0004 */ uint32_t m_count;
    /* 0008 */ T* m_data;

    T& operator[](uint32_t index);
    uint32_t Count();
    const char* MemFileName();
    int32_t MemLineNo();
    T* Ptr();
    T* Top();
};

template <class T>
class TSFixedArray : public TSBaseArray<T> {
    public:
    TSFixedArray();
    TSFixedArray(const TSFixedArray<T>& source);
    ~TSFixedArray();
    TSFixedArray<T>& operator=(const TSFixedArray<T>& source);
    void Clear();
    void ReallocAndClearData(uint32_t count);
    void ReallocData(uint32_t count);
    void Set(uint32_t count, const T* data);
    void SetCount(uint32_t count);
};

template <class T>
class TSBitField {
    public:
    /* 0000 */ uint32_t m_count;
    /* 0004 */ T m_bits;

    void Load(const void* data, uint32_t bytes, int32_t bits);
};

template <class T>
class TSStackArray {
    public:
    /* 0000 */ uint32_t m_count;
    /* 0004 */ T* m_data;

    TSStackArray(T* data, uint32_t count, int32_t construct);
    void SetCount(uint32_t count);
};

template <class T>
class TSGrowableArray : public TSFixedArray<T> {
    public:
    /* 000C */ uint32_t m_chunk;

    TSGrowableArray();
    uint32_t Add(uint32_t count, const T* data);
    uint32_t Add(uint32_t count, uint32_t incr, const T* data);
    uint32_t CalcChunkSize(uint32_t count);
    void GrowToFit(uint32_t index, int32_t zero);
    T* New();
    T* New(const T& source);
    void Reserve(uint32_t count, int32_t round);
    void SetCount(uint32_t count);
};

template <class T, uint32_t N>
class TSCGrowableArray {
    public:
    /* 0000 */ union {
        T m_inline[N];
        TSGrowableArray<T> m_dynamic;
    };
    uint32_t m_count;

    TSCGrowableArray();
    ~TSCGrowableArray();
    T& operator[](uint32_t index);
    void BecomeDynamic();
    T* New(const T& source);
    void SetCount(uint32_t count);
};

template <class T, class TKey>
class TSHashObject {
    public:
    /* 0000 */ uint32_t m_hashval;
    /* 0004 */ TSLink<T> m_linktoslot;
    /* 000C */ TSLink<T> m_linktofull;
    /* 0014 */ TKey m_key;

    ~TSHashObject();
};

template <class T, class TKey>
class TSHashTable {
    public:
    /* 0004 */ TSExplicitList<T> m_fulllist;
    /* 0010 */ uint32_t m_fullnessIndicator;
    /* 0014 */ TSGrowableArray<TSExplicitList<T> > m_slotlistarray;
    /* 0024 */ uint32_t m_slotmask;

    virtual void InternalDelete(T* ptr);
    virtual T* InternalNew(TSExplicitList<T>* listptr, size_t extrabytes, uint32_t flags);
    virtual ~TSHashTable();
    virtual void Destroy();

    TSHashTable();
    void Clear();
    void Delete(T* ptr);
    T* DeleteNode(T* ptr);
    void GrowListArray(uint32_t newarraysize);
    T* Head();
    void Initialize();
    void Insert(T* ptr, uint32_t hashval, const TKey& key);
    void Insert(T* ptr, uint32_t hashval, const char* str);
    void Insert(T* ptr, const char* str);
    void InternalClear(int32_t warn);
    void InternalLinkNode(T* ptr, uint32_t hashval);
    T* InternalNewNode(uint32_t hashval, size_t extrabytes, uint32_t flags);
    int32_t MonitorFullness(uint32_t slot);
    T* New(uint32_t hashval, const TKey& key, size_t extrabytes, uint32_t flags);
    T* New(uint32_t hashval, const char* str, size_t extrabytes, uint32_t flags);
    T* New(const char* str, size_t extrabytes, uint32_t flags);
    T* Next(T* ptr);
    T* Ptr(uint32_t hashval, const TKey& key);
    T* Ptr(const char* str);
    void Unlink(T* ptr);
};

struct HASHKEY_CONSTSTR {
    /* 0000 */ const char* m_str;

    HASHKEY_CONSTSTR& operator=(const char* str);
    bool operator==(const char* str);
};

struct HASHKEY_CONSTSTRI : public HASHKEY_CONSTSTR {
    HASHKEY_CONSTSTRI& operator=(const char* str);
    bool operator==(const char* str);
};

struct HASHKEY_DWORD {
    /* 0000 */ uint32_t m_key;

    bool operator==(const HASHKEY_DWORD& key);
};

struct HASHKEY_NONE {
    bool operator==(const HASHKEY_NONE& key);
};

struct HASHKEY_PTR {
    /* 0000 */ void* m_key;

    bool operator==(const HASHKEY_PTR& key);
};

struct HASHKEY_STR {
    /* 0000 */ char* m_str;

    HASHKEY_STR();
    ~HASHKEY_STR();
    HASHKEY_STR& operator=(const char* str);
    bool operator==(const char* str);
};

struct HASHKEY_STRI : public HASHKEY_STR {
    HASHKEY_STRI& operator=(const char* str);
    bool operator==(const char* str);
};

struct HASHKEY_UTF8I {
    /* 0000 */ char* m_str;

    HASHKEY_UTF8I& operator=(const char* str);
    bool operator==(const char* str);
};

template <class T, class TKey>
class TSHashTableReuse : public TSHashTable<T, TKey> {
    public:
    /* 0028 */ TSExplicitList<T> m_reuseList;

    virtual void InternalDelete(T* ptr);
    virtual T* InternalNew(TSExplicitList<T>* listptr, size_t extrabytes, uint32_t flags);
    virtual ~TSHashTableReuse();
    virtual void Destroy();

    TSHashTableReuse();
    void Destructor();
};

template <class T, class THandle>
class TSExportTableSimpleReuse : public TSHashTableReuse<T, HASHKEY_NONE> {
    public:
    /* 0034 */ HASHKEY_NONE m_key;
    /* 0038 */ uint32_t m_sequence;
    /* 003C */ int32_t m_wrapped;

    TSExportTableSimpleReuse();
    void Delete(T* ptr);
    THandle GenerateUniqueHandle();
    T* New(THandle* handle);
    T* Ptr(THandle handle);
};

template <class T, class THandle, class TLockedHandle, class TSync>
class TSExportTableSyncReuse : public TSExportTableSimpleReuse<T, THandle> {
    public:
    /* 0040 */ TSync m_sync;

    ~TSExportTableSyncReuse();
    void Delete(THandle handle);
    void DeleteUnlock(T* ptr, TLockedHandle lockedhandle);
    int32_t IsForWriting(TLockedHandle lockedhandle);
    T* Lock(THandle handle, TLockedHandle* lockedhandle, int32_t forwriting);
    T* NewLock(THandle* handle, TLockedHandle* lockedhandle);
    void SyncEnterLock(TLockedHandle* lockedhandle, int32_t forwriting);
    void SyncLeaveLock(TLockedHandle lockedhandle);
    void Unlock(TLockedHandle lockedhandle);
};

// INLINED
template <class T>
TSLink<T>::TSLink() {
    this->m_prevLink = nullptr;
    this->m_next = nullptr;
}

// INLINED
template <class T>
TSLink<T>::~TSLink() {
    this->Unlink();
}

// INLINED
template <class T>
int32_t TSLink<T>::IsLinked() {
    return this->m_next != nullptr;
}

// INLINED
template <class T>
T* TSLink<T>::Next() {
    return (reinterpret_cast<uintptr_t>(this->m_next) & 1) ? nullptr : this->m_next;
}

// VERIFY: 0f453bf0 manual
// OFFSET: [0x0047DB10<BFSNODE>]
template <class T>
TSLink<T>* TSLink<T>::NextLink(ptrdiff_t linkoffset) {
    uintptr_t next = reinterpret_cast<uintptr_t>(this->m_next);

    if ((next & 1) || !next) {
        return reinterpret_cast<TSLink<T>*>(next & ~static_cast<uintptr_t>(1));
    }

    if (linkoffset < 0) {
        linkoffset = reinterpret_cast<uint8_t*>(this) - reinterpret_cast<uint8_t*>(this->m_prevLink->m_next);
    }

    return reinterpret_cast<TSLink<T>*>(reinterpret_cast<uint8_t*>(this->m_next) + linkoffset);
}

// INLINED
template <class T>
T* TSLink<T>::Prev() {
    return this->m_prevLink->m_prevLink->Next();
}

// INLINED
template <class T>
T* TSLink<T>::RawNext() {
    return this->m_next;
}

// VERIFY: ecb66e10 auto
// OFFSET: [0x0079F820<VBBList_Block>, 0x00603CF0]
template <class T>
void TSLink<T>::Unlink() {
    if (this->m_prevLink) {
        this->NextLink(-1)->m_prevLink = this->m_prevLink;
        this->m_prevLink->m_next = this->m_next;
        this->m_prevLink = nullptr;
        this->m_next = nullptr;
    }
}

// INLINED
template <class T>
TSLink<T>* TSGetLink<T>::Link(T* nodeptr, ptrdiff_t linkoffset) {
    return &static_cast<TSLinkedNode<T>*>(nodeptr)->m_link;
}

// INLINED
template <class T>
TSLink<T>* TSGetExplicitLink<T>::Link(T* nodeptr, ptrdiff_t linkoffset) {
    return reinterpret_cast<TSLink<T>*>(reinterpret_cast<uint8_t*>(nodeptr) + linkoffset);
}

// VERIFY: 718ec6f6 auto
// OFFSET: 0x00407900
template <class T, class TGetLink>
int32_t TSList<T, TGetLink>::IsEnd(T* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) & 1) || !ptr;
}

// INLINED
template <class T, class TGetLink>
TSList<T, TGetLink>::TSList() {
    this->SetLinkOffset(0);
}

// INLINED
template <class T, class TGetLink>
TSList<T, TGetLink>::TSList(const TSList<T, TGetLink>& source) {
    this->SetLinkOffset(source.m_linkoffset);
}

// VERIFY: 4c44cf25 manual
// OFFSET: [0x00488A70<CSimpleAnimGroup, TSGetExplicitLink<CSimpleAnimGroup>>, 0x0056CF00<CGChatBubbleFrame, TSGetLink<CGChatBubbleFrame>>, 0x005F37D0]
template <class T, class TGetLink>
TSList<T, TGetLink>::~TSList() {
    this->UnlinkAll();
}

// INLINED
template <class T, class TGetLink>
void TSList<T, TGetLink>::ChangeLinkOffset(ptrdiff_t linkoffset) {
    if (linkoffset != this->m_linkoffset) {
        this->UnlinkAll();
        this->SetLinkOffset(linkoffset);
    }
}

// VERIFY: ab66a452 manual
// OFFSET: [0x0047C470, 0x0047E7C0, 0x0047E830, 0x0047E8A0, 0x00485970<RENDERCALLBACKNODE, TSGetLink<RENDERCALLBACKNODE>>, 0x00488940, 0x00489A60<CLayoutFrame::FRAMENODE, TSGetLink<CLayoutFrame::FRAMENODE>>, 0x00498AE0, 0x00498B80<CSimpleAnimGroup, TSGetExplicitLink<CSimpleAnimGroup>>, 0x0049A610<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE>>, 0x004B73E0<CGxTexCache, TSGetExplicitLink<CGxTexCache>>, 0x004B7460<CMipBitsCache, TSGetExplicitLink<CMipBitsCache>>, 0x004BA230, 0x004BA2A0, 0x004BCEF0, 0x004C9650, 0x004CA310<SuppressorSound, TSGetLink<SuppressorSound>>, 0x004CDA60<SI2DspNode, TSGetLink<SI2DspNode>>, 0x004DA510<ConsoleString, TSGetExplicitLink<ConsoleString>>, 0x004F1650<CComponentMipBits, TSGetExplicitLink<CComponentMipBits>>, 0x004F92A0<CModelRecord, TSGetLink<CModelRecord>>, 0x00502270<PENDINGUSERLIST, TSGetExplicitLink<PENDINGUSERLIST>>, 0x00503060<THROTTLEDCHAT, TSGetExplicitLink<THROTTLEDCHAT>>, 0x00527B00<CapturePointUIManagerNode, TSGetLink<CapturePointUIManagerNode>>, 0x00528690<AUTOCOMPLETE_NAMENODE, TSGetLink<AUTOCOMPLETE_NAMENODE>>, 0x0052F090<STYLEREC, TSGetLink<STYLEREC>>, 0x0053EEB0, 0x0055FD20<OVERRIDEKEYBINDINGENTRY, TSGetLink<OVERRIDEKEYBINDINGENTRY>>, 0x0056C9C0<CGChatBubbleFrame, TSGetLink<CGChatBubbleFrame>>, 0x005AE0C0<EQUIPMENT_SET_NODE, TSGetLink<EQUIPMENT_SET_NODE>>, 0x005B2170<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>>, 0x005B2260<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>>, 0x005B4CF0<COMPLETED_QUESTS, TSGetLink<COMPLETED_QUESTS>>, 0x005BE660<CALENDARHOLIDAYNODE, TSGetLink<CALENDARHOLIDAYNODE>>, 0x005BE6D0<CALENDARRAIDRESETNODE, TSGetLink<CALENDARRAIDRESETNODE>>, 0x005BE740<CalendarPendingInvite, TSGetLink<CalendarPendingInvite>>, 0x005F3DB0<_BANNEDADDONLOADNODE, TSGetLink<_BANNEDADDONLOADNODE>>, 0x00606480<CameraShake, TSGetLink<CameraShake>>, 0x00615FF0<BFSNODE, TSGetLink<BFSNODE>>, 0x00617210<PORTRAITREQUEST, TSGetLink<PORTRAITREQUEST>>, 0x00633470<NETEVENTQUEUENODE, TSGetLink<NETEVENTQUEUENODE>>, 0x00670070<DBCACHECALLBACK, TSGetLink<DBCACHECALLBACK>>, 0x00687FD0<CGxBuf, TSGetLink<CGxBuf>>, 0x006BC100, 0x006BDDC0<CGxFont, TSGetLink<CGxFont>>, 0x006BDE40<CGxString, TSGetLink<CGxString>>, 0x006BF210<CGxStringBatch, TSGetLink<CGxStringBatch>>, 0x006E4B80<PlayerPendingItemExpiration, TSGetExplicitLink<PlayerPendingItemExpiration>>, 0x006EBF90<CPlayerMoveEvent, TSGetExplicitLink<CPlayerMoveEvent>>, 0x006F2B70, 0x006F2BE0, 0x006FAF40, 0x0074F4D0<InvalidatedName, TSGetLink<InvalidatedName>>, 0x0074F550<PendingCombatLogData, TSGetLink<PendingCombatLogData>>, 0x0074F640<CombatLogFilter, TSGetLink<CombatLogFilter>>, 0x00753810<CombatLogEntry, TSGetLink<CombatLogEntry>>, 0x00764420, 0x00771C20<HANDLER, TSGetLink<HANDLER>>, 0x00772EF0<CMDDEF, TSGetLink<CMDDEF>>, 0x00788B20<PosDelta, TSGetLink<PosDelta>>, 0x007CB680, 0x007FBC20, 0x00804D90<PendingSpellCast, TSGetLink<PendingSpellCast>>, 0x00804E00<PeriodicClientTrigger, TSGetLink<PeriodicClientTrigger>>, 0x00807650, 0x0081A530<EVENTLISTENERNODE, TSGetLink<EVENTLISTENERNODE>>, 0x0083D2D0<CM2SequencePlayBack, TSGetLink<CM2SequencePlayBack>>, 0x008489F0<POIIcon, TSGetLink<POIIcon>>, 0x00848A60<MAPDATA, TSGetLink<MAPDATA>>, 0x0086E850, 0x008C8550, 0x008C8640<BattlenetLogin::BN_EVENT, TSGetLink<BattlenetLogin::BN_EVENT>>, 0x0095B8E0<CLCDBASE_NODE, TSGetLink<CLCDBASE_NODE>>, 0x005F72D0, 0x00783780, 0x007982A0, 0x007CCAE0, 0x0079F2B0, 0x007C37C0]
template <class T, class TGetLink>
void TSList<T, TGetLink>::Clear() {
    T* node;

    while ((node = this->Head()) != nullptr) {
        this->DeleteNode(node);
    }
}

// VERIFY: 2b2e6d24 manual
// OFFSET: [0x004652A0<RealmConnection::REALMCONNECTIONNODE, TSGetLink<RealmConnection::REALMCONNECTIONNODE>>, 0x0047E600, 0x00480720<EvtHandler, TSGetExplicitLink<EvtHandler>>, 0x004807A0<EvtKeyDown, TSGetExplicitLink<EvtKeyDown>>, 0x00485570<CSimpleEmbeddedTexture, TSGetExplicitLink<CSimpleEmbeddedTexture>>, 0x00489AD0, 0x00490D00, 0x0049A680<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE>>, 0x004D4FB0<CMirrorHandler, TSGetLink<CMirrorHandler>>, 0x005022E0<PENDINGUSERLIST, TSGetExplicitLink<PENDINGUSERLIST>>, 0x005030F0<PENDINGCHAT, TSGetExplicitLink<PENDINGCHAT>>, 0x00527B70<CapturePointUIManagerNode, TSGetLink<CapturePointUIManagerNode>>, 0x0053EF20, 0x0057AE00<AUTOCOMPLETE_NAMENODE, TSGetLink<AUTOCOMPLETE_NAMENODE>>, 0x005AE130<EQUIPMENT_SET_NODE, TSGetLink<EQUIPMENT_SET_NODE>>, 0x005B21E0<CLIENT_ACHIEVEMENT_CRITERIA, TSGetLink<CLIENT_ACHIEVEMENT_CRITERIA>>, 0x005B22D0<CLIENT_ACHIEVEMENTS, TSGetLink<CLIENT_ACHIEVEMENTS>>, 0x005B4C70<SPECIFIC_CRITERIA, TSGetLink<SPECIFIC_CRITERIA>>, 0x005B4D60<COMPLETED_QUESTS, TSGetLink<COMPLETED_QUESTS>>, 0x005BE7B0, 0x005EDA10<ENCOUNTER_TIMER, TSGetLink<ENCOUNTER_TIMER>>, 0x005EDA90<ENCOUNTER_OBJECTIVE, TSGetLink<ENCOUNTER_OBJECTIVE>>, 0x006064F0<CameraShake, TSGetLink<CameraShake>>, 0x006330F0<NetClient::HELDMESSAGE, TSGetLink<NetClient::HELDMESSAGE>>, 0x00633130, 0x0066FF60<DBCACHECALLBACK, TSGetLink<DBCACHECALLBACK>>, 0x006BDE00, 0x006F20A0<CObjectEffectRef, TSGetLink<CObjectEffectRef>>, 0x0074F5C0<PendingCombatLogData, TSGetLink<PendingCombatLogData>>, 0x0077E830<WNDREC, TSGetLink<WNDREC>>, 0x00788B90<PosDelta, TSGetLink<PosDelta>>, 0x007C3330, 0x007CB600<VBBList_Block, TSGetExplicitLink<VBBList_Block>>, 0x007D5CD0<FVBBList::Block, TSGetExplicitLink<FVBBList::Block>>, 0x007FC070<MountTransitionObject, TSGetLink<MountTransitionObject>>, 0x00804E70<PeriodicClientTrigger, TSGetLink<PeriodicClientTrigger>>, 0x0081A5A0<EVENTLISTENERNODE, TSGetLink<EVENTLISTENERNODE>>, 0x0082DB30<CM2SequenceLoad, TSGetLink<CM2SequenceLoad>>, 0x00831BB0<CM2SequencePlayBack, TSGetLink<CM2SequencePlayBack>>, 0x008C85C0, 0x008C86B0, 0x008CED10<Grunt::Pending::Data, TSGetExplicitLink<Grunt::Pending::Data>>, 0x0095AA30<CLCDConnection::CB_EVENT_NODE, TSGetLink<CLCDConnection::CB_EVENT_NODE>>, 0x0096CE80<CONTENTNODE, TSGetLink<CONTENTNODE>>, 0x0079F100]
template <class T, class TGetLink>
T* TSList<T, TGetLink>::DeleteNode(T* ptr) {
    T* next = this->Next(ptr);

    ptr->~T();
    SMemFree(ptr, StormTypeName(typeid(T)), SLOG_OBJECT, 0);

    return next;
}

// VERIFY: 1639e241 manual
// OFFSET: 0x006D7BA0
template <class T, class TGetLink>
T* TSList<T, TGetLink>::Head() {
    return this->m_terminator.Next();
}

// INLINED
template <class T, class TGetLink>
void TSList<T, TGetLink>::InitializeTerminator() {
    this->m_terminator.m_prevLink = &this->m_terminator;
    this->m_terminator.m_next = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(&this->m_terminator) | 1);
}

// VERIFY: a0ec46df manual
// OFFSET: [0x004F7180<CModelRecord, TSGetLink<CModelRecord>>]
template <class T, class TGetLink>
int32_t TSList<T, TGetLink>::IsEmpty() {
    return this->Head() == nullptr;
}

// VERIFY: e5b0ed2f auto
// OFFSET: 0x004D4820
template <class T, class TGetLink>
int32_t TSList<T, TGetLink>::IsLinked(T* ptr) {
    return this->Link(ptr)->IsLinked();
}

// INLINED
template <class T, class TGetLink>
TSLink<T>* TSList<T, TGetLink>::Link(T* ptr) {
    return ptr ? TGetLink::Link(ptr, this->m_linkoffset) : &this->m_terminator;
}

// VERIFY: 2f0fe850 manual
// OFFSET: [0x004F71A0<CONSOLELINE, TSGetLink<CONSOLELINE>>, 0x006714A0]
template <class T, class TGetLink>
void TSList<T, TGetLink>::LinkNode(T* ptr, uint32_t linktype, T* existingptr) {
    TSLink<T>* link = this->Link(ptr);
    link->Unlink();

    TSLink<T>* existinglink = this->Link(existingptr);

    if (linktype == 1) {
        link->m_prevLink = existinglink;
        link->m_next = existinglink->m_next;
        existinglink->NextLink(this->m_linkoffset)->m_prevLink = link;
        existinglink->m_next = ptr;
    } else {
        link->m_prevLink = existinglink->m_prevLink;
        link->m_next = existinglink->m_prevLink->m_next;
        existinglink->m_prevLink->m_next = ptr;
        existinglink->m_prevLink = link;
    }
}

// VERIFY: c759121a manual
// OFFSET: [0x0055E610<ProfileInternal::STRINGBLOCK, TSGetLink<ProfileInternal::STRINGBLOCK>>, 0x007B5020<CMapChunkBufBlock, TSGetExplicitLink<CMapChunkBufBlock>>]
template <class T, class TGetLink>
void TSList<T, TGetLink>::LinkToHead(T* ptr) {
    this->LinkNode(ptr, 1, nullptr);
}

// VERIFY: e89481d9 auto
// OFFSET: [0x0056CA00<CGChatBubbleFrame, TSGetLink<CGChatBubbleFrame>>, 0x006DED60<DBCache<GuildStats_C, int, HASHKEY_INT>::DBCACHEHASH, TSGetExplicitLink<DBCache<GuildStats_C, int, HASHKEY_INT>::DBCACHEHASH>>, 0x0086E200<WowConnection::SENDNODE, TSGetLink<WowConnection::SENDNODE>>, 0x008795F0]
template <class T, class TGetLink>
void TSList<T, TGetLink>::LinkToTail(T* ptr) {
    this->LinkNode(ptr, 2, nullptr);
}

// VERIFY: 0fb37856 manual
// OFFSET: [0x00496C00<SIMPLEFONT, TSGetExplicitLink<SIMPLEFONT>>, 0x004B6E40, 0x004BFFC0, 0x004F3420<CACHEENTRY, TSGetExplicitLink<CACHEENTRY>>, 0x004F89E0, 0x0055A1D0<LFGListParty, TSGetExplicitLink<LFGListParty>>, 0x00560A00<OVERRIDEKEYBINDING, TSGetExplicitLink<OVERRIDEKEYBINDING>>, 0x005BDF00, 0x006329D0, 0x0066CCB0<DBCACHECALLBACK, TSGetLink<DBCACHECALLBACK>>, 0x006705D0, 0x00671030, 0x006883B0<CGxShader, TSGetExplicitLink<CGxShader>>, 0x006BBDB0<CGxuLight, TSGetLink<CGxuLight>>, 0x0074F2D0<CombatLogEntry, TSGetLink<CombatLogEntry>>, 0x00776BC0<RGN, TSGetExplicitLink<RGN>>, 0x007FBC60<LightningObject, TSGetLink<LightningObject>>, 0x007FBCE0<MountTransitionObject, TSGetLink<MountTransitionObject>>, 0x00783100]
template <class T, class TGetLink>
T* TSList<T, TGetLink>::NewNode(uint32_t location, size_t extrabytes, uint32_t flags) {
    T* node = new (SMemAlloc(sizeof(T) + extrabytes, StormTypeName(typeid(T)), SLOG_OBJECT, flags | SMEM_FLAG_ZEROMEMORY)) T;

    if (location) {
        this->LinkNode(node, location, nullptr);
    }

    return node;
}

// INLINED
template <class T, class TGetLink>
T* TSList<T, TGetLink>::Next(T* ptr) {
    return this->Link(ptr)->Next();
}

// INLINED
template <class T, class TGetLink>
T* TSList<T, TGetLink>::Prev(T* ptr) {
    return this->Link(ptr)->Prev();
}

// VERIFY: 230ebf8d auto
// OFFSET: [0x004B6670, 0x004D4770]
template <class T, class TGetLink>
T* TSList<T, TGetLink>::RawNext(T* ptr) {
    return this->Link(ptr)->RawNext();
}

// INLINED
template <class T, class TGetLink>
void TSList<T, TGetLink>::SetLinkOffset(ptrdiff_t linkoffset) {
    this->m_linkoffset = linkoffset;
    this->InitializeTerminator();
}

// INLINED
template <class T, class TGetLink>
T* TSList<T, TGetLink>::Tail() {
    return this->m_terminator.Prev();
}

// VERIFY: cdd999f0 manual
// OFFSET: [0x0047D170, 0x0056CA70, 0x005BD800<CSimpleRegion, TSGetExplicitLink<CSimpleRegion>>, 0x007CECD0<Grunt::Timer::Event, TSGetExplicitLink<Grunt::Timer::Event>>, 0x00879980]
template <class T, class TGetLink>
void TSList<T, TGetLink>::UnlinkAll() {
    T* node;

    while ((node = this->Head()) != nullptr) {
        this->UnlinkNode(node);
    }
}

// INLINED
template <class T, class TGetLink>
void TSList<T, TGetLink>::Combine(TSList<T, TGetLink>* list, uint32_t linktype, T* existingptr) {
    if (!list) {
        DebugValidate("list");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (list == this) {
        DebugValidate("list != this");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (list->m_linkoffset != this->m_linkoffset) {
        DebugValidate("list->m_linkoffset == m_linkoffset");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (list->m_terminator.m_prevLink == &list->m_terminator) {
        return;
    }

    TSLink<T>* link = existingptr ? this->Link(existingptr) : &this->m_terminator;

    if (linktype == 1) {
        link = link->NextLink(this->m_linkoffset);
    }

    TSLink<T>* prev = link->m_prevLink;
    T* next = prev->m_next;
    prev->m_next = list->m_terminator.m_next;
    link->m_prevLink = list->m_terminator.m_prevLink;
    list->m_terminator.NextLink(this->m_linkoffset)->m_prevLink = prev;
    list->m_terminator.m_prevLink->m_next = next;
    list->m_terminator.m_prevLink = &list->m_terminator;
    list->m_terminator.m_next = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(&list->m_terminator) | 1);
}

// VERIFY: 58dea224 auto
// OFFSET: [0x004BFEE0<CLayoutFrame, TSGetExplicitLink<CLayoutFrame>>, 0x005B1EF0<CModelRecord, TSGetLink<CModelRecord>>, 0x00723300]
template <class T, class TGetLink>
void TSList<T, TGetLink>::UnlinkNode(T* ptr) {
    this->Link(ptr)->Unlink();
}

// INLINED
template <class T, int offset>
TSExplicitList<T, offset>::TSExplicitList() {
    this->SetLinkOffset(offset);
}

// INLINED
template <class T>
T* TSLinkedNode<T>::Next() {
    return this->m_link.Next();
}

// INLINED
template <class T>
T* TSLinkedNode<T>::RawNext() {
    return this->m_link.RawNext();
}

// VERIFY: d9cd8150 manual
// OFFSET: 0x0075B560
template <class T>
void TSSwap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// INLINED
template <class T>
T& TSBaseArray<T>::operator[](uint32_t index) {
    return this->m_data[index];
}

// INLINED
template <class T>
uint32_t TSBaseArray<T>::Count() {
    return this->m_count;
}

// INLINED
template <class T>
const char* TSBaseArray<T>::MemFileName() {
    return StormTypeName(typeid(T));
}

// INLINED
template <class T>
int32_t TSBaseArray<T>::MemLineNo() {
    return -2;
}

// INLINED
template <class T>
T* TSBaseArray<T>::Ptr() {
    return this->m_data;
}

// VERIFY: af9b80d8 auto
// OFFSET: 0x0077AC60
template <class T>
T* TSBaseArray<T>::Top() {
    return this->m_count ? &this->m_data[this->m_count - 1] : nullptr;
}

// INLINED
template <class T>
TSFixedArray<T>::TSFixedArray() {
    this->m_alloc = 0;
    this->m_count = 0;
    this->m_data = nullptr;
}

// INLINED
template <class T>
TSFixedArray<T>::TSFixedArray(const TSFixedArray<T>& source) {
    this->m_alloc = 0;
    this->m_count = 0;
    this->m_data = nullptr;
    this->Set(source.m_count, source.m_data);
}

// VERIFY: 0f3f0aeb manual
// OFFSET: [0x0047C630, 0x0047C6C0, 0x00490BE0<TSExplicitList<FRAMEATTR, -572662307>>, 0x004967E0, 0x004B74F0<CTexture*>, 0x004BAFD0, 0x004BC2F0, 0x004BC380, 0x004BE2D0, 0x004C0260, 0x004C9520<TSExplicitList<TIMESTAMPSTRUCT, -572662307>>, 0x004CA720, 0x004CC6F0, 0x004CC780, 0x004CDDD0, 0x004CF070, 0x004D0A70, 0x004D3130, 0x004D33C0, 0x004D4E10<TSExplicitList<CGObject_C, -572662307>>, 0x004F31F0, 0x00502360<PENDINGUSERLIST>, 0x005032A0, 0x00503330, 0x005033C0, 0x00503450, 0x005053E0, 0x00527BF0, 0x00527C80, 0x00527E70, 0x0052F100, 0x0053F160, 0x00546FE0<WorldMapContinentInfo>, 0x00548B70, 0x00558C50<LFGListParty*>, 0x00558CE0<LFGListPlayer*>, 0x0055F560<TSExplicitList<KEYCOMMAND, -572662307>>, 0x0055F5F0<TSExplicitList<KEYBINDING, -572662307>>, 0x0055F680<TSExplicitList<OVERRIDEKEYBINDING, -572662307>>, 0x0055F710<TSExplicitList<MODIFIEDCLICK, -572662307>>, 0x0055F7A0, 0x00566700, 0x00575F60, 0x005920F0, 0x005B4DE0, 0x005B4E70, 0x005BE830, 0x005BE8C0, 0x005C7290<TSExplicitList<TalentGroupItem_C, -572662307>>, 0x005C7320<TSExplicitList<TalentGroupTierItem_C, -572662307>>, 0x005C91F0<TalentGroupTab_C>, 0x005D24E0, 0x005E6880<QuestPOIBlob>, 0x005F03B0, 0x005F3E20, 0x005F5AD0, 0x005F5B60<TSExplicitList<ADDONSTATE, -572662307>>, 0x005F5BF0, 0x005FCB80<TSExplicitList<MOUSELOOKBINDING, -572662307>>, 0x00617AA0, 0x00617B30, 0x00617BC0, 0x0066E830, 0x0066E8C0, 0x0066E950, 0x0066E9E0, 0x0066EA70, 0x0066EB00, 0x0066EB90, 0x0066EC20, 0x0066ECB0, 0x0066ED40, 0x0066EDD0, 0x0066EE60, 0x0066EEF0, 0x0066EF80, 0x0066F010, 0x0066F0A0, 0x0066F130, 0x0066F1C0, 0x0066F250, 0x0066F2E0, 0x0066F370, 0x0066F400, 0x0066F490, 0x0066F520, 0x0066F5B0, 0x0066F640, 0x0066F6D0, 0x0066F760, 0x0066F7F0, 0x0066F880, 0x006881A0<TSExplicitList<CGxShader, -572662307>>, 0x006BBE30, 0x006BEE90, 0x006C2650<TSExplicitList<CHARCODEDESC, -572662307>>, 0x006C2AC0<TSExplicitList<KERNNODE, -572662307>>, 0x006C2B50, 0x006C8380, 0x006E4EB0, 0x006E4F40, 0x006F2C50, 0x006F2CE0, 0x006F2D70<TSExplicitList<OBJ_EFFECT_GROUP_LOOKUP, -572662307>>, 0x006F2E00<TSExplicitList<OBJ_EFFECT_REF_LIST_LOOKUP, -572662307>>, 0x006F2E90, 0x006F2F20, 0x006F2FB0, 0x00709AA0, 0x00709B30, 0x007131B0, 0x0072FFC0<TSExplicitList<UnitThreat, -572662307>>, 0x007675F0, 0x007685D0, 0x00776AD0<TSExplicitList<RGN, -572662307>>, 0x007B04D0, 0x007B5DF0, 0x007C0B40, 0x007C0BD0<CMapObjDef*>, 0x007DD360, 0x007DD3F0, 0x007E01C0, 0x007F2A20, 0x007F4ED0, 0x00804EF0, 0x00804F80, 0x008109F0, 0x00810A80, 0x00810B10, 0x00812BC0, 0x00812C50, 0x00814910<CXMLAttribute>, 0x0081A620<FrameScript_EventObject*>, 0x00848800, 0x0084B570, 0x00876550, 0x00877760<TSExplicitList<CacheData, -572662307>>, 0x0087B0F0, 0x00965260<CSimpleEditBox::EditHistory>, 0x009681C0, 0x009694A0<CSimpleMessageScrollFrameLine>, 0x009694F0, 0x0097BB80<CParticleEmitter2*>, 0x0040A790, 0x00848AD0<CHATMESSAGE>, 0x004E3080<CharacterSelectionDisplay>, 0x00591ED0, 0x00591F20, 0x005F01E0, 0x0056BBA0<CGCommentator::MapInfo>, 0x007A0160<CMapFootprintTexture>, 0x0079D4D0]
template <class T>
TSFixedArray<T>::~TSFixedArray() {
    for (uint32_t i = 0; i < this->m_count; i++) {
        this->m_data[i].~T();
    }

    if (this->m_data) {
        SMemFree(this->m_data, this->MemFileName(), this->MemLineNo(), 0);
    }
}

// VERIFY: 88921419 auto
// OFFSET: [0x00409770<int>, 0x004D9FB0<char>, 0x005918E0, 0x00591960, 0x00879950<SEChannelGroup>, 0x008C6E30]
template <class T>
void TSFixedArray<T>::Clear() {
    this->~TSFixedArray<T>();
    this->m_alloc = 0;
    this->m_count = 0;
    this->m_data = nullptr;
}

// VERIFY: de17895f manual
// OFFSET: [0x004D2E90<CObjectHeap>]
template <class T>
void TSFixedArray<T>::ReallocAndClearData(uint32_t count) {
    for (uint32_t i = 0; i < this->m_count; i++) {
        this->m_data[i].~T();
    }

    if (count != this->m_count) {
        this->m_alloc = count;

        if (this->m_data || count) {
            this->m_data = static_cast<T*>(SMemReAlloc(this->m_data, sizeof(T) * count, this->MemFileName(), this->MemLineNo(), 0));
        }
    }
}

// VERIFY: e2efd710 manual
// OFFSET: [0x00403760<char*>, 0x004037F0<SArchive*>, 0x00408340<DYNAMICELEMENTVERT>, 0x004083F0, 0x00408490, 0x00408D80<int>, 0x004644C0<CHARACTER_INFO>, 0x004657D0<REALM_INFO>, 0x00466910<unsigned int>, 0x0047CCC0<void*>, 0x0047D9F0<SThread*>, 0x0047DA80<EvtContext*>, 0x00480EE0<EvtTimer*>, 0x004827C0<CSimpleBatchedMesh>, 0x00490A80, 0x00493A40<CFrameStrataNode*>, 0x00493AD0<FRAMEPRIORITY*>, 0x00496680, 0x00497970<CSimpleControlPoint*>, 0x00497A00<TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE>>*>, 0x004B7580, 0x004B9990<void (__cdecl*)(void)>, 0x004B9A20<int (__cdecl*)(void)>, 0x004BB060, 0x004BBDA0, 0x004BBF00, 0x004BC060, 0x004BE170, 0x004BEBB0<CBaseManaged*>, 0x004C00B0, 0x004C46C0<C3Vector>, 0x004C4750<float>, 0x004CA380, 0x004CB930<VSOUNDKITDEF*>, 0x004CB9C0<WorldStateZoneSoundOverride*>, 0x004CC360, 0x004CC4C0, 0x004CDB20, 0x004CEA00, 0x004CEAA0, 0x004CEF00, 0x004D0910<IMPACTSOUNDARRAY>, 0x004D2D90<CObjectHeap>, 0x004D34A0<CObjectHeapList>, 0x004D4FF0, 0x004D85A0<char const*>, 0x004DE520<RealmCategory*>, 0x004E0EC0<ChrClassesRec*>, 0x004E25C0<CharacterSelectionDisplay>, 0x004E6660<DictionaryRecord>, 0x004F2FB0, 0x004FCFC0<ChatColorInfo>, 0x004FD050<RaidInstanceInfo>, 0x004FD150, 0x004FFCE0<regex_t>, 0x005014A0<PENDINGUSERLIST>, 0x005023E0, 0x00502540, 0x005026A0, 0x00502800, 0x005034E0<ChatChannel>, 0x00514100<NearestUnitData>, 0x005141A0, 0x0051A2E0<unsigned __int64>, 0x00525720, 0x00525880, 0x00527D10, 0x0052EF00, 0x0053B790<SpellBookSkillInfo*>, 0x0053B820<SpellUsableInfo>, 0x0053EFA0, 0x005436A0<WorldMapLandmarkInfo>, 0x00547C20<WorldMapContinentInfo>, 0x00548640, 0x005489C0, 0x0054EC50<C2iVector>, 0x0054ECF0<C3iVector>, 0x00553220<CGLookingForGroup::LFGEntryInfo>, 0x005532C0, 0x00553350, 0x005533E0, 0x00558710, 0x00558870, 0x0055EAD0, 0x0055EC30, 0x0055ED90, 0x0055EEF0, 0x0055F050, 0x00564CE0<MacroCommand>, 0x00566790, 0x005685D0<CGCommentator::QueueNode>, 0x0056BAD0<CGCommentator::InstanceInfo>, 0x0056BD40<CGCommentator::MapInfo>, 0x0056D780<StationeryInfo*>, 0x0056D810<MailboxMessageInfo*>, 0x00575DC0, 0x00577410, 0x0057C120<POIINFO>, 0x0057C1B0<OBJINFO>, 0x0057E360<C2Vector>, 0x0057E400<unsigned short>, 0x0058E030<QuestPOIPoint>, 0x0058F8F0<CGQuestPOIFrame::QuestPOIRenderData>, 0x005907A0, 0x00590870<TSGrowableArray<TAXIROUTELINE>>, 0x00590920, 0x005909C0<TAXILINE>, 0x00590A60, 0x00591F70, 0x00592180, 0x00592290, 0x00593FC0<TrainerServiceInfo*>, 0x00594050<TrainerSkillLineInfo*>, 0x00599030<CImVector>, 0x0059B150<AuctionItemInfo>, 0x0059B1F0<OutbidItemInfo*>, 0x005A2AF0, 0x005A46F0, 0x005B49B0, 0x005B4B10, 0x005B8C80<CalendarDayEvent*>, 0x005BDF60, 0x005BE0C0, 0x005C58D0<TalentTabInfo*>, 0x005C5960<TalentGroupTier_C*>, 0x005C6C60, 0x005C6DC0, 0x005C9080<TalentGroupTab_C>, 0x005CA3D0<GuildMemberInfo*>, 0x005CD240<SkillLineInfo*>, 0x005CD2D0<SkillTypeInfo*>, 0x005CEB50<PetitionSignerInfo>, 0x005CFFB0, 0x005D0080, 0x005D0110, 0x005D2360, 0x005D3260, 0x005DA3F0<TradeSkillInfo*>, 0x005DA480<TradeSkillSubClassInfo*>, 0x005DA510, 0x005E6E40, 0x005E7940, 0x005ED620<ENCOUNTER_UNIT>, 0x005EEB80<MacroSubstring>, 0x005F0240, 0x005F0440<MacroCondition>, 0x005F1420<EventInviteInfo*>, 0x005F39B0, 0x005F4E30, 0x005F5020, 0x005F5180, 0x005F52E0, 0x005FC9C0, 0x005FEF70<CFacet>, 0x0060A1C0<UniqueSignal>, 0x0060A260<VehicleUIIndSeatRec*>, 0x006152E0<CRect>, 0x006169A0, 0x00616C20, 0x00617280, 0x006173E0, 0x00617540, 0x0061BD20<CSimpleFontString*>, 0x0061BDB0<FrameStackInfo>, 0x00634790, 0x00634820, 0x00669EB0, 0x0066CD10, 0x0066CE70, 0x0066CFD0, 0x0066D130, 0x0066D290, 0x0066D3F0, 0x0066D550, 0x0066D6B0, 0x0066D810, 0x0066D970, 0x0066DAD0, 0x0066DC30, 0x0066DD90, 0x0066DEF0, 0x0066E050, 0x00681800<GxDrawListEntry>, 0x006822B0<CGxFormat>, 0x00683760<CGxAppRenderState>, 0x00683810<CGxStateBom>, 0x006838B0<CGxPushedRenderState>, 0x00683960<enum EGxRenderState>, 0x00688040, 0x00689F90<CGxMonitorMode>, 0x006A54A0<CGxDeviceD3d::GxVertexDecl>, 0x006A9440<CGxDeviceD3d9Ex::GxVertexDecl>, 0x006B1EF0<Grunt::CdKey::Info>, 0x006BA590, 0x006BBBE0, 0x006C26E0, 0x006C2840, 0x006C4210<VERT>, 0x006C4E20, 0x006C5EE0<GXUFONTHYPERLINKINFO>, 0x006C5F80<GXUEMBEDDEDTEXTUREINFO>, 0x006C6020<GRADIENTINFO>, 0x006C8220, 0x006C9DF0<TEXTURECACHEROW>, 0x006CF4A0<InitialSpellStruct>, 0x006CF540<GUILDUPDATETIMESTAMP>, 0x006CF5E0<ITEMSWAP>, 0x006E4BF0, 0x006E4D50, 0x006F2120, 0x006F2280, 0x006F23E0, 0x006F2540, 0x006F26A0, 0x006F2800, 0x006F2960, 0x006FECD0<SpellCastTargetPoint>, 0x00709580, 0x007096E0, 0x00716260<DelayedSpellVisualKit>, 0x00716300<AuraData>, 0x007163B0<SortedAuraInfo>, 0x0072FE60, 0x00746400<DEATTHUDSOUNDINFO>, 0x00747C10<CVehiclePassenger_C*>, 0x00747CA0<VehiclePassengerRescueTransition>, 0x007583D0<SimpleScriptFunction>, 0x00767490, 0x00768460, 0x00769D80<void (__cdecl*)(enum OPTION_PAGE)>, 0x0076D060, 0x007760A0<_SOURCE>, 0x00776950, 0x00786B90<Mists::Mist>, 0x00790250<CMapObjDefGroup*>, 0x007902E0<CGxVertexPCT0T1>, 0x00790370, 0x007917D0<CGxVertexPC>, 0x00791870<CPortalView>, 0x0079B370, 0x0079BA30<World::LiquidQueryResult>, 0x0079D510<CWaterRipple>, 0x0079F650<CGxVertexPT0T1>, 0x007A01C0<CMapFootprintTexture>, 0x007B0370, 0x007B0D60<CDetailDoodadInstAdd>, 0x007B0DF0<CDetailDoodadData*>, 0x007B0E80<CGxBuf*>, 0x007B4F10<CMapLight*>, 0x007BA120<CMapChunkBufBlock>, 0x007BDFE0, 0x007BE140, 0x007C30B0<CMapAreaTexture>, 0x007CC020, 0x007CCBE0<C4Plane>, 0x007CCC80<CClipVolume>, 0x007CDEC0<CChunkLiquid*>, 0x007DBB10<ComSatCallback>, 0x007DBBC0<ComSatEvent>, 0x007DD0A0, 0x007DD200, 0x007E0050, 0x007E6600, 0x007E7FF0<FFX::Pass*>, 0x007EBEA0, 0x007ECFF0<DayNight::AreaLightOverride>, 0x007F1F80, 0x007F3BF0<AreaPOIRec*>, 0x007F3D70<POIDIRECTIONDATA>, 0x007F4CF0, 0x007F78F0, 0x007F79C0, 0x007F8E30<ShipPathLeg>, 0x007F9F80<LightningObject::Bolt>, 0x007FA020, 0x007FA0B0, 0x00804830, 0x00804990, 0x00810370<SkillLineLinkInfo>, 0x00810500, 0x00810660, 0x008107C0, 0x008128D0, 0x00812A30, 0x00814800<CXMLAttribute>, 0x008196A0, 0x0081A2F0, 0x0081CDC0<M2Element>, 0x00824660<CRibbonMat>, 0x00824700<M2ModelCallback>, 0x00848890<CHATMESSAGE>, 0x0084B140, 0x0084B2A0, 0x008555E0<MemChunk*>, 0x0086D0E0<OsIMECandidate>, 0x0086E8E0, 0x008704B0<W32Joystick>, 0x00876370, 0x00878E90<SEDriverInfo>, 0x00878F30<SEChannelGroup>, 0x0087AEB0, 0x00887490<ComSat::Sound::OutputChannel*>, 0x008A17D0<Liquid::CMaterialSettings*>, 0x008A1860<Liquid::IMaterial*>, 0x008A18F0<Liquid::CInstance*>, 0x00959260<CEzLcdPage*>, 0x0095E2C0<MOVIECAPTION>, 0x009633C0, 0x00967E50<MessageData>, 0x00967EF0<CSimpleMessageFrameLine>, 0x009690F0<CSimpleMessageScrollFrameLine>, 0x00969540<CSimpleMessageScrollFrameDisplayNode>, 0x00978EC0<CParticle2>, 0x0097BBE0<CParticle2_Model>, 0x0097F650, 0x00980EE0<CParticleEmitter2*>, 0x009A80F0<GxuSceneObject*>, 0x009A8D50<LightningJoint>, 0x009A8DF0<CLightning*>, 0x0040A800, 0x008C64B0, 0x008C6550]
template <class T>
void TSFixedArray<T>::ReallocData(uint32_t count) {
    T* olddata = this->m_data;

    for (uint32_t i = count; i < this->m_count; i++) {
        olddata[i].~T();
    }

    this->m_alloc = count;
    this->m_data = static_cast<T*>(SMemReAlloc(olddata, sizeof(T) * count, this->MemFileName(), this->MemLineNo(), SMEM_FLAG_NOREALLOC));

    if (!this->m_data) {
        this->m_data = static_cast<T*>(SMemAlloc(sizeof(T) * count, this->MemFileName(), this->MemLineNo(), 0));

        if (olddata) {
            uint32_t copycount = count < this->m_count ? count : this->m_count;

            for (uint32_t i = 0; i < copycount; i++) {
                new (&this->m_data[i]) T(olddata[i]);
                olddata[i].~T();
            }

            SMemFree(olddata, this->MemFileName(), this->MemLineNo(), 0);
        }
    }
}

// VERIFY: 3666e861 manual
// OFFSET: [0x00409470, 0x004C4B10<C3Vector>, 0x004D3180, 0x0056B7C0<CGCommentator::InstanceInfo>, 0x0058EB60, 0x0058EBD0<C2Vector>, 0x00591250<TaxiPathRec*>, 0x005912C0<TAXIROUTELINE>, 0x005C5F00<TalentGroupTier_C*>, 0x005E0800, 0x005EF6A0<MacroSubstring>, 0x006EAAB0<float>, 0x006FF5C0<SpellCastTargetPoint>, 0x006FF630, 0x00700C80, 0x0077AC80, 0x007F8240, 0x0097FE80<CRibbonMat>]
template <class T>
void TSFixedArray<T>::Set(uint32_t count, const T* data) {
    this->ReallocAndClearData(count);

    for (uint32_t i = 0; i < count; i++) {
        new (&this->m_data[i]) T(data[i]);
    }

    this->m_count = count;
}

// VERIFY: 30433c72 manual
// OFFSET: [0x00464B30<CHARACTER_INFO>, 0x00465AC0<REALM_INFO>, 0x004670D0, 0x004957B0<CFrameStrataNode*>, 0x004D0B00, 0x004F5840<int>, 0x00547E20, 0x00592F00, 0x00592FC0, 0x00599820<CImVector>, 0x005C9690, 0x006171B0, 0x0061F9C0<CSimpleFontString*>, 0x006C0270<C3Vector>, 0x006CA0B0<TEXTURECACHEROW>, 0x007476F0, 0x0079E080<CWaterRipple>, 0x007F1220<float>, 0x007F1EA0<C2Vector>, 0x007F1F20<unsigned short>, 0x00966B10<CSimpleEditBox::EditHistory>, 0x009AAF10<LightningJoint>, 0x0084A730]
template <class T>
void TSFixedArray<T>::SetCount(uint32_t count) {
    if (count != this->m_count) {
        if (count) {
            this->ReallocData(count);

            for (uint32_t i = this->m_count; i < count; i++) {
                new (&this->m_data[i]) T;
            }

            this->m_count = count;
        } else {
            this->Clear();
        }
    }
}

// INLINED
template <class T>
TSFixedArray<T>& TSFixedArray<T>::operator=(const TSFixedArray<T>& source) {
    if (this != &source) {
        this->Set(source.m_count, source.m_data);
    }

    return *this;
}

// INLINED
template <class T>
TSGrowableArray<T>::TSGrowableArray() {
    this->m_chunk = 0;
}

// VERIFY: d0bba67f manual
// OFFSET: [0x004BBE20<char>, 0x0057FCB0<C3Vector>, 0x0057FD50<C2Vector>, 0x0057FEA0<unsigned short>, 0x0061F7F0<FrameStackInfo>, 0x006BADD0, 0x006C6550<GXUFONTHYPERLINKINFO>, 0x006C6760<GXUEMBEDDEDTEXTUREINFO>, 0x0071C780<AuraData>, 0x0071C830<SortedAuraInfo>, 0x00723410<int>, 0x00758630<SimpleScriptFunction>, 0x00791EB0<CMapObjDefGroup*>, 0x00791F80<CGxVertexPCT0T1>, 0x007920A0, 0x007940F0<CPortalView>, 0x007A3DB0<CFacet>, 0x007A8E90<CRect>, 0x007CD140<C4Plane>, 0x007CD3F0<CClipVolume>]
template <class T>
uint32_t TSGrowableArray<T>::Add(uint32_t count, const T* data) {
    uint32_t newcount = this->m_count + count;

    if (newcount > this->m_alloc) {
        uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(newcount);

        if (newcount % chunk) {
            newcount += chunk - newcount % chunk;
        }

        this->ReallocData(newcount);
    }

    T* element = &this->m_data[this->m_count];
    T* end = element + count;

    for (; element != end; element++) {
        new (element) T(*data++);
    }

    this->m_count += count;

    return this->m_count - count;
}

// VERIFY: e11e147e manual
// OFFSET: [0x006C6430<CImVector>, 0x006C64B0<VERT>, 0x006C69A0<GRADIENTINFO>]
template <class T>
uint32_t TSGrowableArray<T>::Add(uint32_t count, uint32_t incr, const T* data) {
    uint32_t newcount = this->m_count + count;

    if (newcount > this->m_alloc) {
        uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(newcount);

        if (newcount % chunk) {
            newcount += chunk - newcount % chunk;
        }

        this->ReallocData(newcount);
    }

    T* element = &this->m_data[this->m_count];
    T* end = element + count;

    for (; element != end; element++) {
        new (element) T(*data);
        data = reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(data) + incr);
    }

    this->m_count += count;

    return this->m_count - count;
}

// VERIFY: 52330a88 manual
// OFFSET: [0x004BBD60, 0x00548600<unsigned __int64>, 0x0057C0E0<unsigned short>, 0x00590830<DayNight::AreaLightOverride>, 0x00590AE0<C4Plane>, 0x005D0040<FFX::Pass*>, 0x006C41D0, 0x00790400<CPortalView>, 0x007AE270<C3Vector>, 0x007F7980<CGxVertexPCT0T1>]
template <class T>
uint32_t TSGrowableArray<T>::CalcChunkSize(uint32_t count) {
    uint32_t maxchunk = 256 / sizeof(T) > 8 ? 256 / sizeof(T) : 8;

    if (count >= maxchunk) {
        this->m_chunk = maxchunk;
        return maxchunk;
    }

    uint32_t result = count;

    for (uint32_t bits = count & (count - 1); bits; bits &= bits - 1) {
        result = bits;
    }

    return result ? result : 1;
}

// VERIFY: c55e1baf auto
// OFFSET: [0x00481080<EvtTimer*>, 0x008A1B30<Liquid::CMaterialSettings*>, 0x008A1BB0<Liquid::IMaterial*>]
template <class T>
void TSGrowableArray<T>::GrowToFit(uint32_t index, int32_t zero) {
    if (index >= this->m_count) {
        uint32_t newcount = index + 1;

        if (newcount > this->m_alloc) {
            uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(newcount);
            uint32_t alloc = newcount;

            if (newcount % chunk) {
                alloc = newcount + chunk - newcount % chunk;
            }

            this->ReallocData(alloc);
        }

        if (zero) {
            memset(&this->m_data[this->m_count], 0, sizeof(T) * (index - this->m_count + 1));
        }

        this->m_count = newcount;
    }
}

// VERIFY: 910046a1 auto
// OFFSET: [0x00405970<char*>, 0x00409410<CLoadingScreenShipInfo<TaxiPathNodeRec>::LOADINGSCREENTAXIPATHINFO>, 0x0047DE50<SThread*>, 0x00498BC0<TSList<SIMPLEANIMNODE, TSGetLink<SIMPLEANIMNODE>>*>, 0x004C69F0<int>, 0x004CEBE0<TEXTEMOTELOOKUP>, 0x004D31E0<CObjectHeap>, 0x004D35C0<CObjectHeapList>, 0x004E3760<CharacterSelectionDisplay>, 0x00500F60, 0x005221D0<NearestUnitData>, 0x00565420<MacroCommand>, 0x00579AD0, 0x0057F670<OBJINFO>, 0x0058FEF0<CGQuestPOIFrame::QuestPOIRenderData>, 0x005916F0<TAXIROUTELINE>, 0x00591890<TAXILINE>, 0x005C6480<TalentGroupTier_C*>, 0x00615660<CRect>, 0x006850E0<CGxPushedRenderState>, 0x00685130, 0x006851D0<C3Vector>, 0x0068A030<CGxFormat>, 0x0068A090<CGxMonitorMode>, 0x006B2A40<Grunt::CdKey::Info>, 0x006DEC70<InitialSpellStruct>, 0x007761E0<_SOURCE>, 0x007CD1D0<C4Plane>, 0x007E6E50, 0x007F0CC0, 0x007F4450<AreaPOIRec*>, 0x00821670<M2Element>, 0x0082DB70<M2ModelCallback>, 0x0086D180<OsIMECandidate>, 0x00870550<W32Joystick>, 0x00879900<SEChannelGroup>, 0x008BFE30, 0x008C6DE0, 0x008C6E60, 0x00968330<MessageData>, 0x00980FB0<CParticleEmitter2*>, 0x00524930, 0x00782490]
template <class T>
T* TSGrowableArray<T>::New() {
    uint32_t newcount = this->m_count + 1;

    if (newcount > this->m_alloc) {
        uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(newcount);

        if (newcount % chunk) {
            newcount += chunk - newcount % chunk;
        }

        this->ReallocData(newcount);
    }

    T* element = &this->m_data[this->m_count++];

    return new (element) T;
}

// VERIFY: 864fc686 auto
// OFFSET: [0x004097A0, 0x00480FD0<unsigned int>, 0x00498B20<CSimpleControlPoint*>, 0x004B9C00, 0x00503130<PENDINGUSERLIST>, 0x0054EFE0<C2iVector>, 0x005D1490, 0x005F2980<EventInviteInfo*>, 0x00614390, 0x00616D10<CTexture*>, 0x00749730<CVehiclePassenger_C*>, 0x0076A1C0, 0x007CEC70<CChunkLiquid*>, 0x007EA1C0<FFX::Pass*>, 0x007FB270, 0x008A1C30<Liquid::CInstance*>, 0x009A9C40<CLightning*>, 0x00723100, 0x007824F0]
template <class T>
T* TSGrowableArray<T>::New(const T& source) {
    uint32_t newcount = this->m_count + 1;

    if (newcount > this->m_alloc) {
        uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(newcount);

        if (newcount % chunk) {
            newcount += chunk - newcount % chunk;
        }

        this->ReallocData(newcount);
    }

    T* element = &this->m_data[this->m_count++];

    return new (element) T(source);
}

// INLINED
template <class T>
void TSGrowableArray<T>::Reserve(uint32_t count, int32_t round) {
    uint32_t newcount = this->m_count + count;

    if (newcount <= this->m_alloc) {
        return;
    }

    if (round) {
        uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(newcount);

        if (newcount % chunk) {
            newcount += chunk - newcount % chunk;
        }
    }

    this->ReallocData(newcount);
}

// VERIFY: aa735f18 auto
// OFFSET: [0x00409280<DYNAMICELEMENTVERT>, 0x0040AFE0, 0x00481030<unsigned int>, 0x004845F0<CSimpleBatchedMesh>, 0x004920B0, 0x004949C0<FRAMEPRIORITY*>, 0x004969C0, 0x004B8610<CTexture*>, 0x004BB540, 0x004BC4A0, 0x004BC5B0, 0x004BE360, 0x004C05B0, 0x004C4B80, 0x004C4C80, 0x004CABE0, 0x004CC810, 0x004CC920, 0x004CDE60, 0x004CEC60<TEXTEMOTELOOKUP*>, 0x004CF320, 0x004D6640, 0x004D99F0<char const*>, 0x004E1BD0<int>, 0x004F3280, 0x00500EC0<char*>, 0x00500F10<RaidInstanceInfo>, 0x005031B0<PENDINGUSERLIST>, 0x005054F0, 0x00505600, 0x00505710, 0x00505820, 0x00506210<ChatChannel>, 0x00522220<NearestUnitData>, 0x00528700, 0x00528810, 0x00529BC0, 0x0052F1F0, 0x0053F440, 0x005462C0<WorldMapLandmarkInfo>, 0x00548C00, 0x00556D70, 0x00556DC0, 0x005595E0<LFGListParty*>, 0x005596F0, 0x0055FDB0, 0x0055FEC0, 0x0055FFD0, 0x005600E0, 0x005601F0, 0x00565490<MacroCommand>, 0x00567830, 0x0056BC00<CGCommentator::InstanceInfo>, 0x0056BE70, 0x005714D0<MailboxMessageInfo*>, 0x00575FF0, 0x0057F5D0<POIINFO>, 0x0057F6E0<OBJINFO>, 0x0057FDD0<C2Vector>, 0x0057FF20<unsigned short>, 0x0058EC40, 0x00591760<TAXIROUTELINE>, 0x00591910, 0x005923A0, 0x0059DF60<OutbidItemInfo*>, 0x005B5310, 0x005B5420, 0x005BF870, 0x005BF980, 0x005C8310, 0x005C8420, 0x005CC510<GuildMemberInfo*>, 0x005CF530<PetitionSignerInfo>, 0x005D2570, 0x005D5A60, 0x005DCA90, 0x005E72B0, 0x005EFFD0, 0x005F0570, 0x005F0680, 0x005F3ED0, 0x005F5C80, 0x005F5D90, 0x005F5EA0, 0x005FCC60, 0x00603930<CFacet>, 0x006156C0<CRect>, 0x00616CA0<CImVector>, 0x00616D70, 0x00617CE0, 0x00617DF0, 0x00617F00, 0x006700E0, 0x00671CB0, 0x00671DC0, 0x00671ED0, 0x00671FE0, 0x006720F0, 0x00672200, 0x00672310, 0x00672420, 0x00672530, 0x00672640, 0x00672750, 0x00672860, 0x00672970, 0x00672A80, 0x00672B90, 0x00682790<GxDrawListEntry>, 0x00682A40<CGxFormat>, 0x00685180, 0x00685E40<unsigned char>, 0x00688450, 0x0068A0F0<CGxMonitorMode>, 0x006BC250, 0x006C2E60, 0x006C2F70, 0x006C49B0<VERT>, 0x006C59D0, 0x006C65E0<GXUFONTHYPERLINKINFO>, 0x006C6800<GXUEMBEDDEDTEXTUREINFO>, 0x006C6A20<GRADIENTINFO>, 0x006C8410, 0x006DECC0<ITEMSWAP>, 0x006E6920, 0x006E6A30, 0x006F31A0, 0x006F32B0, 0x006F33C0, 0x006F34D0, 0x006F35E0, 0x006F36F0, 0x006F3800, 0x00700CF0<SpellCastTargetPoint>, 0x00709BC0, 0x00709CD0, 0x00723170<DelayedSpellVisualKit>, 0x007233C0<AuraData>, 0x00734800, 0x00767AB0, 0x00768B00, 0x0076D260, 0x00776CF0, 0x00788930<Mists::Mist>, 0x00791F30<CMapObjDefGroup*>, 0x00792010<CGxVertexPCT0T1>, 0x00792130, 0x00793FD0<CGxVertexPC>, 0x00794190<CPortalView>, 0x0079F980<CGxVertexPT0T1>, 0x007A02F0<CMapFootprintTexture>, 0x007B06F0, 0x007B25B0<CDetailDoodadInstAdd>, 0x007B2710<CGxBuf*>, 0x007BA250<CMapChunkBufBlock>, 0x007BE2A0, 0x007BE3B0<CMapObjDef*>, 0x007CD250<C4Plane>, 0x007CD470<CClipVolume>, 0x007DD480, 0x007DD590, 0x007E02E0, 0x007E6EF0, 0x007F2AB0, 0x007F4A70<POIDIRECTIONDATA>, 0x007F4F60, 0x007FB220<LightningObject::Bolt>, 0x008076C0, 0x008077D0, 0x00810BA0, 0x00810CB0, 0x00810DC0, 0x00812D40, 0x00812E50, 0x008149E0<CXMLAttribute>, 0x0081AB60, 0x0082DAC0<CRibbonMat>, 0x0082DBC0<M2ModelCallback>, 0x0084B600, 0x0084B710, 0x0086EED0, 0x008765E0, 0x008798B0<SEDriverInfo>, 0x0087BB70, 0x00968390<MessageData>, 0x00968400<CSimpleMessageFrameLine>, 0x00969800<CSimpleMessageScrollFrameLine>, 0x00969B80<CSimpleMessageScrollFrameDisplayNode>, 0x0097D600<CParticle2>, 0x0097DF90<CParticle2_Model>, 0x00980810<CGxVertexPCT>, 0x007F8FA0<ShipPathLeg>, 0x004E37E0<CharacterSelectionDisplay>]
template <class T>
void TSGrowableArray<T>::SetCount(uint32_t count) {
    if (count > this->m_count) {
        if (count > this->m_alloc) {
            uint32_t chunk = this->m_chunk ? this->m_chunk : this->CalcChunkSize(count);
            uint32_t alloc = count;

            if (count % chunk) {
                alloc = count + chunk - count % chunk;
            }

            this->ReallocData(alloc);
        }

        for (uint32_t i = this->m_count; i < count; i++) {
            new (&this->m_data[i]) T;
        }
    } else if (count < this->m_count) {
        for (uint32_t i = count; i < this->m_count; i++) {
            this->m_data[i].~T();
        }
    }

    this->m_count = count;
}

// VERIFY: 8457e24c auto
// OFFSET: [0x0047C560<FONTHASHOBJ, HASHKEY_STR>, 0x005C6800<CMapObj, HASHKEY_NONE>, 0x007F4AC0<OVERRIDEKEYBINDING, HASHKEY_STRI>]
template <class T, class TKey>
TSHashObject<T, TKey>::~TSHashObject() {
}

// VERIFY: fcbebc9d auto
// OFFSET: [0x0047C7B0<PrefetchNode, HASHKEY_STRI>, 0x0047CA30<UncachableNode, HASHKEY_STRI>, 0x00492080<FRAMEATTR, HASHKEY_STRI>, 0x00496980<SIMPLEFONT, HASHKEY_STRI>, 0x004B83F0<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB2D0<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BC7D0<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BD1B0<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BE4D0<FONTHASHOBJ, HASHKEY_STR>, 0x004C0510<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004C9820<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x004CADB0<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CCB80<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CCD70<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE310<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF500<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D5DD0<CGObject_C, CHashKeyGUID>, 0x004F33F0<CACHEENTRY, HASHKEY_NONE>, 0x00506060<CHATTHROTTLE, HASHKEY_STR>, 0x005063F0<WORDLIST, HASHKEY_LANGUAGE>, 0x00506590<PROFANITYCHECK, HASHKEY_STR>, 0x00506790<SPAMCHECK, HASHKEY_STR>, 0x00529680<CGGameUI::THREATMAP, CHashKeyGUID>, 0x00529720<SAVEDVARIABLE, HASHKEY_STR>, 0x005299B0, 0x0052F3D0<USEABLESTYLE, HASHKEY_NONE>, 0x0053F740<SPELLSBYNAME, HASHKEY_UTF8I>, 0x00549010<WORLDSTATE, HASHKEY_NONE>, 0x0055A190<LFGListParty, CHashKeyGUID>, 0x0055A3D0<LFGListPlayer, CHashKeyGUID>, 0x00560650<KEYCOMMAND, HASHKEY_STRI>, 0x00560800<KEYBINDING, HASHKEY_STRI>, 0x00560C10<MODIFIEDCLICK, HASHKEY_STRI>, 0x00560E70<CLICKFRAME, HASHKEY_STR>, 0x00561DA0<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x00567670<MACRONODE, HASHKEY_NONE>, 0x00576300<KNOWNDANCES, HASHKEY_STR>, 0x00592DD0<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B64C0<ACHIEVEMENT, HASHKEY_NONE>, 0x005B6660<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C15F0, 0x005C1850<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C8590<TalentGroupItem_C, HASHKEY_NONE>, 0x005C8790<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D2780<FACTION, HASHKEY_NONE>, 0x005F0820<MacroConditions, HASHKEY_STR>, 0x005F4040<_BANNEDADDON, HASHKEY_NONE>, 0x005F6050<UIADDON::METADATA, HASHKEY_STRI>, 0x005F6210<ADDONSTATE, HASHKEY_STRI>, 0x005F7240<UIADDON, HASHKEY_STRI>, 0x005FCDD0<MOUSELOOKBINDING, HASHKEY_STRI>, 0x00618400<PLAYERPORTRAIT, CHashKeyGUID>, 0x00618630<UNITPORTRAIT, HASHKEY_DWORD>, 0x00618850<ITEMPORTRAIT, HASHKEY_STRI>, 0x00670390, 0x00670540, 0x00670730, 0x006708E0, 0x00670A90, 0x00670C40, 0x00670DF0, 0x00670FA0, 0x006711A0, 0x00671350, 0x006715C0, 0x00671770, 0x00671920, 0x00671AD0, 0x00671C80, 0x00678F70, 0x00678FA0, 0x00678FD0, 0x00679000, 0x00679030, 0x00679060, 0x00679090, 0x006790C0, 0x006790F0, 0x00679120, 0x00679150, 0x00679180, 0x006791B0, 0x006791E0, 0x00679210, 0x00688380<CGxShader, HASHKEY_STRI>, 0x006BC1A0<CLightList, HASHKEY_DWORD>, 0x006BEF80, 0x006C2C80<CHARCODEDESC, HASHKEY_NONE>, 0x006C30E0<KERNNODE, KERNINGHASHKEY>, 0x006C8580<FACEDATA, HASHKEY_STRI>, 0x006E6590<TRADESKILLLINE, HASHKEY_NONE>, 0x006E6690<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F3BE0<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F3D70<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F4090<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F4280<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5970<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5EC0<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F5F70<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x00709EC0<INVENTORYART, HASHKEY_NONE>, 0x0070A120<ITEMBYNAME, HASHKEY_UTF8I>, 0x00734760<UnitThreat, CHashKeyGUID>, 0x00767A00<CVar, HASHKEY_STRI>, 0x00768C70<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x00776CC0<RGN, HASHKEY_NONE>, 0x007B0660<CMapObj, HASHKEY_NONE>, 0x007B7110, 0x007C0CE0<CMapDoodadDef, HASHKEY_DWORD>, 0x007C0DD0<CMapObjDef, HASHKEY_NONE>, 0x007DD810<TALKINGPLAYER, CHashKeyGUID>, 0x007DDA00<MUTEDPLAYER, CHashKeyGUID>, 0x007E04B0<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F2C20<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F53D0<MINIMAPMD5NAME, HASHKEY_STRI>, 0x0080A400, 0x0080A5F0<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00811080<RACECLASSINFO, HASHKEY_NONE>, 0x00811270<SKILLCOSTSINFO, HASHKEY_NONE>, 0x00811400<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x00813230<HashedNode, HASHKEY_STRI>, 0x008134B0<FrameFactoryNode, HASHKEY_STRI>, 0x0081AFA0<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084A180<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0084B8E0<BITMAPBYICON, HASHKEY_NONE>, 0x00876750<CShaderEffect, HASHKEY_STRI>, 0x00877850<CacheData, HASHKEY_NONE>, 0x0087C010<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTable<T, TKey>::InternalDelete(T* ptr) {
    ptr->~T();
    SMemFree(ptr, StormTypeName(typeid(T)), SLOG_OBJECT, 0);
}

// VERIFY: 2a1eed0a auto
// OFFSET: [0x0047C800<PrefetchNode, HASHKEY_STRI>, 0x0047C9D0<UncachableNode, HASHKEY_STRI>, 0x00492020<FRAMEATTR, HASHKEY_STRI>, 0x00496EB0<SIMPLEFONT, HASHKEY_STRI>, 0x004B92B0<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB300<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BC760<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BD1F0<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BE550<FONTHASHOBJ, HASHKEY_STR>, 0x004C0540<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004C96C0<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x004CAD50<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CCB20<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CCD10<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE340<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF530<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D65E0<CGObject_C, CHashKeyGUID>, 0x004F34B0<CACHEENTRY, HASHKEY_NONE>, 0x00506090<CHATTHROTTLE, HASHKEY_STR>, 0x00506380<WORDLIST, HASHKEY_LANGUAGE>, 0x005065D0<PROFANITYCHECK, HASHKEY_STR>, 0x005067D0<SPAMCHECK, HASHKEY_STR>, 0x00529610<CGGameUI::THREATMAP, CHashKeyGUID>, 0x005296C0<SAVEDVARIABLE, HASHKEY_STR>, 0x005299F0, 0x0052F360<USEABLESTYLE, HASHKEY_NONE>, 0x0053F6D0<SPELLSBYNAME, HASHKEY_UTF8I>, 0x00548FB0<WORLDSTATE, HASHKEY_NONE>, 0x0055A250<LFGListParty, CHashKeyGUID>, 0x0055A400<LFGListPlayer, CHashKeyGUID>, 0x00560690<KEYCOMMAND, HASHKEY_STRI>, 0x00560840<KEYBINDING, HASHKEY_STRI>, 0x00560A90<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x00560C40<MODIFIEDCLICK, HASHKEY_STRI>, 0x00560E10<CLICKFRAME, HASHKEY_STR>, 0x005676A0<MACRONODE, HASHKEY_NONE>, 0x005762A0<KNOWNDANCES, HASHKEY_STR>, 0x00592D70<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B6450<ACHIEVEMENT, HASHKEY_NONE>, 0x005B65F0<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C1620, 0x005C17F0<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C85C0<TalentGroupItem_C, HASHKEY_NONE>, 0x005C87C0<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D2710<FACTION, HASHKEY_NONE>, 0x005F07B0<MacroConditions, HASHKEY_STR>, 0x005F4070<_BANNEDADDON, HASHKEY_NONE>, 0x005F6090<UIADDON::METADATA, HASHKEY_STRI>, 0x005F61B0<ADDONSTATE, HASHKEY_STRI>, 0x005F7270<UIADDON, HASHKEY_STRI>, 0x005FCE10<MOUSELOOKBINDING, HASHKEY_STRI>, 0x00618380<PLAYERPORTRAIT, CHashKeyGUID>, 0x006185B0<UNITPORTRAIT, HASHKEY_DWORD>, 0x006187E0<ITEMPORTRAIT, HASHKEY_STRI>, 0x00670270, 0x00670330, 0x00670420, 0x006704E0, 0x00670650, 0x006706D0, 0x006707C0, 0x00670880, 0x00670970, 0x00670A30, 0x00670B20, 0x00670BE0, 0x00670CD0, 0x00670D90, 0x00670E80, 0x00670F40, 0x006710C0, 0x00671140, 0x00671230, 0x006712F0, 0x006713E0, 0x00671560, 0x00671650, 0x00671710, 0x00671800, 0x006718C0, 0x006719B0, 0x00671A70, 0x00671B60, 0x00671C20, 0x00688430<CGxShader, HASHKEY_STRI>, 0x006BC1D0<CLightList, HASHKEY_DWORD>, 0x006BEFB0, 0x006C2CB0<CHARCODEDESC, HASHKEY_NONE>, 0x006C3110<KERNNODE, KERNINGHASHKEY>, 0x006C85B0<FACEDATA, HASHKEY_STRI>, 0x006E6520<TRADESKILLLINE, HASHKEY_NONE>, 0x006E6630<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F3B70<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F3D10<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F4020<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F42B0<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F59A0<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5F00<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F5FA0<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x00709F00<INVENTORYART, HASHKEY_NONE>, 0x0070A0C0<ITEMBYNAME, HASHKEY_UTF8I>, 0x00734700<UnitThreat, CHashKeyGUID>, 0x00767A50<CVar, HASHKEY_STRI>, 0x00768CA0<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x00776C40<RGN, HASHKEY_NONE>, 0x007B0800<CMapObj, HASHKEY_NONE>, 0x007B7180, 0x007C0D10<CMapDoodadDef, HASHKEY_DWORD>, 0x007C0E00<CMapObjDef, HASHKEY_NONE>, 0x007DD7B0<TALKINGPLAYER, CHashKeyGUID>, 0x007DD9A0<MUTEDPLAYER, CHashKeyGUID>, 0x007E0450<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F2C50<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F5370<MINIMAPMD5NAME, HASHKEY_STRI>, 0x0080A3A0, 0x0080A590<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00811020<RACECLASSINFO, HASHKEY_NONE>, 0x00811210<SKILLCOSTSINFO, HASHKEY_NONE>, 0x00811430<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x00813280<HashedNode, HASHKEY_STRI>, 0x00813450<FrameFactoryNode, HASHKEY_STRI>, 0x0081AF40<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084A100<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0084B880<BITMAPBYICON, HASHKEY_NONE>, 0x00876780<CShaderEffect, HASHKEY_STRI>, 0x008778B0<CacheData, HASHKEY_NONE>, 0x0087C040<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::InternalNew(TSExplicitList<T>* listptr, size_t extrabytes, uint32_t flags) {
    return listptr->NewNode(1, extrabytes, flags);
}

// VERIFY: 4a402d4d manual
// OFFSET: [0x0047C870, 0x0047CA60, 0x0047CB60, 0x0047CB90, 0x004921C0, 0x004922C0, 0x00496AD0, 0x00496BD0, 0x004B84E0, 0x004B85E0, 0x004BB410, 0x004BB510, 0x004BC6C0, 0x004BC930, 0x004BCA30, 0x004BCA60, 0x004BE5C0, 0x004BE6C0, 0x004C0890, 0x004C0990, 0x004C95B0, 0x004C97F0, 0x004CAE00, 0x004CAF00, 0x004CCBB0, 0x004CCDA0, 0x004CCEA0, 0x004CCED0, 0x004CE3C0, 0x004CE4C0, 0x004CF590, 0x004CF690, 0x004D6040, 0x004D6140, 0x004F34D0, 0x004F35D0, 0x00506110, 0x00506430, 0x00506630, 0x00506830, 0x00506930, 0x00506960, 0x00506990, 0x005069C0, 0x00529750, 0x00529850, 0x00529A60, 0x00529B60, 0x00529B90, 0x0052A130, 0x0052F400, 0x0052F500, 0x0053F770, 0x0053F870, 0x00549040, 0x00549140, 0x0055A270, 0x0055A480, 0x0055A580, 0x0055A5B0, 0x00560700, 0x005608A0, 0x00560AB0, 0x00560CB0, 0x00560EA0, 0x00560FA0, 0x00560FD0, 0x00561000, 0x00561030, 0x00561060, 0x00567700, 0x00567800, 0x00576330, 0x00576430, 0x00592E00, 0x00593080, 0x005B64F0, 0x005B6690, 0x005B6790, 0x005B67C0, 0x005C1690, 0x005C1880, 0x005C1980, 0x005C19B0, 0x005C8630, 0x005C8820, 0x005C8920, 0x005C8950, 0x005D27C0, 0x005D28C0, 0x005F0850, 0x005F0950, 0x005F40E0, 0x005F41E0, 0x005F5FB0, 0x005F6240, 0x005F6400, 0x005F6500, 0x005F6530, 0x005F6560, 0x005FCE70, 0x005FCF70, 0x00618450, 0x00618680, 0x006188A0, 0x006189A0, 0x006189D0, 0x00618A00, 0x00672CA0, 0x00672E50, 0x00672F50, 0x00673100, 0x00673200, 0x006733B0, 0x006734B0, 0x00673660, 0x00673760, 0x00673910, 0x00673A10, 0x00673BD0, 0x00673CD0, 0x00673E70, 0x00673F70, 0x00674110, 0x00674210, 0x006743C0, 0x006744C0, 0x00674670, 0x00674770, 0x00674910, 0x00674A10, 0x00674BB0, 0x00674CB0, 0x00674E50, 0x00674F50, 0x006750F0, 0x006751F0, 0x006753B0, 0x006754B0, 0x006754E0, 0x00675510, 0x00675540, 0x00675570, 0x006755A0, 0x006755D0, 0x00675600, 0x00675630, 0x00675660, 0x00675690, 0x006756C0, 0x006756F0, 0x00675720, 0x00675750, 0x00675780, 0x006757B0, 0x006757E0, 0x00675810, 0x00675840, 0x00675870, 0x006758A0, 0x006758D0, 0x00675900, 0x00675930, 0x00675960, 0x00675990, 0x006759C0, 0x006759F0, 0x00675A20, 0x00688560, 0x00688660, 0x006BC390, 0x006BC490, 0x006BF030<BATCHEDRENDERFONTDESC, HASHKEY_PTR>, 0x006BF130, 0x006C2D30, 0x006C2E30, 0x006C3180, 0x006C3280, 0x006C8620, 0x006C8720, 0x006E66C0, 0x006E67C0, 0x006E68C0, 0x006E68F0, 0x006F3AD0, 0x006F3C70, 0x006F3EC0, 0x006F40C0, 0x006F4310, 0x006F44D0, 0x006F4690, 0x006F4790, 0x006F47C0, 0x006F47F0, 0x006F4820, 0x006F4850, 0x006F4880, 0x006F48B0, 0x00709F60, 0x0070A150, 0x0070A250, 0x0070A280, 0x00734910, 0x00734A10, 0x00767BC0, 0x00767CC0, 0x00768D00, 0x00768E00, 0x00776E00<RGN, HASHKEY_NONE>, 0x007B0860, 0x007B0960, 0x007B71F0, 0x007B72F0, 0x007C0E60, 0x007C0F60, 0x007C1060, 0x007C1090, 0x007DD840, 0x007DDA30, 0x007DDB30, 0x007DDB60, 0x007E04E0, 0x007E05E0, 0x007F2CC0, 0x007F2DC0, 0x007F5400, 0x007F5500, 0x0080A430<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE>, 0x0080A620, 0x0080A720, 0x0080A750, 0x008110B0, 0x008112A0, 0x008114A0, 0x008115A0, 0x008115D0, 0x00811D10, 0x008132F0, 0x008134E0, 0x008135E0, 0x00813610, 0x0081AFD0, 0x0081B0D0, 0x0084A1B0, 0x0084A2B0, 0x0084B910, 0x0084BA10, 0x008767E0, 0x008768E0, 0x00877910, 0x00877A10, 0x0087C0A0, 0x0087C1A0]
template <class T, class TKey>
TSHashTable<T, TKey>::~TSHashTable() {
    this->InternalClear(1);
}

// VERIFY: 8328884c auto
// OFFSET: [0x0047C750<PrefetchNode, HASHKEY_STRI>, 0x0047C970<UncachableNode, HASHKEY_STRI>, 0x00491FC0<FRAMEATTR, HASHKEY_STRI>, 0x00496920<SIMPLEFONT, HASHKEY_STRI>, 0x004B8390<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB270<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BC870<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BC8D0<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BE470<FONTHASHOBJ, HASHKEY_STR>, 0x004C04B0<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004CACF0<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CCAC0<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CCCB0<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE2B0<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF430<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D5D70<CGObject_C, CHashKeyGUID>, 0x004F3390<CACHEENTRY, HASHKEY_NONE>, 0x00506000<CHATTHROTTLE, HASHKEY_STR>, 0x00506320<WORDLIST, HASHKEY_LANGUAGE>, 0x00506530<PROFANITYCHECK, HASHKEY_STR>, 0x00506730<SPAMCHECK, HASHKEY_STR>, 0x005295B0<CGGameUI::THREATMAP, CHashKeyGUID>, 0x00529950, 0x0052F300<USEABLESTYLE, HASHKEY_NONE>, 0x0053F670<SPELLSBYNAME, HASHKEY_UTF8I>, 0x00548F50<WORLDSTATE, HASHKEY_NONE>, 0x0055A130<LFGListParty, CHashKeyGUID>, 0x0055A370<LFGListPlayer, CHashKeyGUID>, 0x005605F0<KEYCOMMAND, HASHKEY_STRI>, 0x005609A0<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x00560BB0<MODIFIEDCLICK, HASHKEY_STRI>, 0x00560DB0<CLICKFRAME, HASHKEY_STR>, 0x00567610<MACRONODE, HASHKEY_NONE>, 0x00576240<KNOWNDANCES, HASHKEY_STR>, 0x00592D10<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B63F0<ACHIEVEMENT, HASHKEY_NONE>, 0x005C1790<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C8530<TalentGroupItem_C, HASHKEY_NONE>, 0x005C8730<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D26B0<FACTION, HASHKEY_NONE>, 0x005F0750<MacroConditions, HASHKEY_STR>, 0x005F3FE0<_BANNEDADDON, HASHKEY_NONE>, 0x005F6150<ADDONSTATE, HASHKEY_STRI>, 0x005F6340, 0x005F63A0<UIADDON::METADATA, HASHKEY_STRI>, 0x005FCD70<MOUSELOOKBINDING, HASHKEY_STRI>, 0x00618320<PLAYERPORTRAIT, CHashKeyGUID>, 0x00618550<UNITPORTRAIT, HASHKEY_DWORD>, 0x00618780<ITEMPORTRAIT, HASHKEY_STRI>, 0x00670210, 0x006702D0, 0x006703C0, 0x00670480, 0x00670570, 0x00670670, 0x00670760, 0x00670820, 0x00670910, 0x006709D0, 0x00670AC0, 0x00670B80, 0x00670C70, 0x00670D30, 0x00670E20, 0x00670EE0, 0x00670FD0, 0x006710E0, 0x006711D0, 0x00671290, 0x00671380, 0x00671440, 0x006715F0, 0x006716B0, 0x006717A0, 0x00671860, 0x00671950, 0x00671A10, 0x00671B00, 0x00671BC0, 0x006BC140<CLightList, HASHKEY_DWORD>, 0x006BEF20, 0x006C2BE0<CHARCODEDESC, HASHKEY_NONE>, 0x006C3080<KERNNODE, KERNINGHASHKEY>, 0x006C8520<FACEDATA, HASHKEY_STRI>, 0x006E64C0<TRADESKILLLINE, HASHKEY_NONE>, 0x006E65D0<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F3E00<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F3E60<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F3FC0<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F41C0<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F4410<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F45D0<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x006F4630<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x00709E60<INVENTORYART, HASHKEY_NONE>, 0x0070A060<ITEMBYNAME, HASHKEY_UTF8I>, 0x007346A0<UnitThreat, CHashKeyGUID>, 0x007679A0<CVar, HASHKEY_STRI>, 0x00768C10<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x0076D370<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x00776B60<RGN, HASHKEY_NONE>, 0x007B0600<CMapObj, HASHKEY_NONE>, 0x007B70B0, 0x007C0C80<CMapDoodadDef, HASHKEY_DWORD>, 0x007C0D70<CMapObjDef, HASHKEY_NONE>, 0x007DD750<TALKINGPLAYER, CHashKeyGUID>, 0x007DD940<MUTEDPLAYER, CHashKeyGUID>, 0x007E03F0<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F2BC0<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F5310<MINIMAPMD5NAME, HASHKEY_STRI>, 0x0080A340<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE>, 0x0080A530<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00810FC0<RACECLASSINFO, HASHKEY_NONE>, 0x008111B0<SKILLCOSTSINFO, HASHKEY_NONE>, 0x008113A0<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x008131D0<HashedNode, HASHKEY_STRI>, 0x008133F0<FrameFactoryNode, HASHKEY_STRI>, 0x0081AE80<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084A0A0<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0084B820<BITMAPBYICON, HASHKEY_NONE>, 0x008766F0<CShaderEffect, HASHKEY_STRI>, 0x008777F0<CacheData, HASHKEY_NONE>, 0x0087BFB0<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
TSHashTable<T, TKey>::TSHashTable() {
    this->m_fullnessIndicator = 0;
    this->m_fulllist.ChangeLinkOffset(offsetof(T, m_linktofull));
    this->m_slotmask = 0xFFFFFFFF;
}

// INLINED
template <class T, class TKey>
void TSHashTable<T, TKey>::Clear() {
    this->InternalClear(0);
}

// VERIFY: 05dc3778 auto
// OFFSET: [0x0047C910<PrefetchNode, HASHKEY_STRI>, 0x0047CB00<UncachableNode, HASHKEY_STRI>, 0x00492260<FRAMEATTR, HASHKEY_STRI>, 0x00496B70<SIMPLEFONT, HASHKEY_STRI>, 0x004B8580<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB4B0<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BC810<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BC9D0<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BE660<FONTHASHOBJ, HASHKEY_STR>, 0x004C0930<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004C9790<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x004CAEA0<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CCC50<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CCE40<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE460<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF630<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D60E0<CGObject_C, CHashKeyGUID>, 0x004F3570<CACHEENTRY, HASHKEY_NONE>, 0x005061B0<CHATTHROTTLE, HASHKEY_STR>, 0x005064D0<WORDLIST, HASHKEY_LANGUAGE>, 0x005066D0<PROFANITYCHECK, HASHKEY_STR>, 0x005068D0<SPAMCHECK, HASHKEY_STR>, 0x005297F0<SAVEDVARIABLE, HASHKEY_STR>, 0x005298F0<CGGameUI::THREATMAP, CHashKeyGUID>, 0x00529B00, 0x0052F4A0<USEABLESTYLE, HASHKEY_NONE>, 0x0053F810<SPELLSBYNAME, HASHKEY_UTF8I>, 0x005490E0<WORLDSTATE, HASHKEY_NONE>, 0x0055A310<LFGListParty, CHashKeyGUID>, 0x0055A520<LFGListPlayer, CHashKeyGUID>, 0x005607A0<KEYCOMMAND, HASHKEY_STRI>, 0x00560940<KEYBINDING, HASHKEY_STRI>, 0x00560B50<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x00560D50<MODIFIEDCLICK, HASHKEY_STRI>, 0x00560F40<CLICKFRAME, HASHKEY_STR>, 0x005677A0<MACRONODE, HASHKEY_NONE>, 0x005763D0<KNOWNDANCES, HASHKEY_STR>, 0x00592EA0<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B6590<ACHIEVEMENT, HASHKEY_NONE>, 0x005B6730<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C1730, 0x005C1920<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C86D0<TalentGroupItem_C, HASHKEY_NONE>, 0x005C88C0<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D2860<FACTION, HASHKEY_NONE>, 0x005F08F0<MacroConditions, HASHKEY_STR>, 0x005F4180<_BANNEDADDON, HASHKEY_NONE>, 0x005F60F0<UIADDON::METADATA, HASHKEY_STRI>, 0x005F62E0<ADDONSTATE, HASHKEY_STRI>, 0x005F64A0<UIADDON, HASHKEY_STRI>, 0x005FCF10<MOUSELOOKBINDING, HASHKEY_STRI>, 0x006184F0<PLAYERPORTRAIT, CHashKeyGUID>, 0x00618720<UNITPORTRAIT, HASHKEY_DWORD>, 0x00618940<ITEMPORTRAIT, HASHKEY_STRI>, 0x00672D40, 0x00672EF0, 0x00672FF0, 0x006731A0, 0x006732A0, 0x00673450, 0x00673550, 0x00673700, 0x00673800, 0x006739B0, 0x00673AB0, 0x00673C70, 0x00673D70, 0x00673F10, 0x00674010, 0x006741B0, 0x006742B0, 0x00674460, 0x00674560, 0x00674710, 0x00674810, 0x006749B0, 0x00674AB0, 0x00674C50, 0x00674D50, 0x00674EF0, 0x00674FF0, 0x00675190, 0x00675290, 0x00675450, 0x00688600<CGxShader, HASHKEY_STRI>, 0x006BC430<CLightList, HASHKEY_DWORD>, 0x006BF0D0, 0x006C2DD0<CHARCODEDESC, HASHKEY_NONE>, 0x006C3220<KERNNODE, KERNINGHASHKEY>, 0x006C86C0<FACEDATA, HASHKEY_STRI>, 0x006E6760<TRADESKILLLINE, HASHKEY_NONE>, 0x006E6860<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F3C10<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F3DA0<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F3F60<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F4160<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F43B0<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F4570<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F4730<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x0070A000<INVENTORYART, HASHKEY_NONE>, 0x0070A1F0<ITEMBYNAME, HASHKEY_UTF8I>, 0x007349B0<UnitThreat, CHashKeyGUID>, 0x00767C60<CVar, HASHKEY_STRI>, 0x00768DA0<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x00776EA0<RGN, HASHKEY_NONE>, 0x007B0900<CMapObj, HASHKEY_NONE>, 0x007B7290, 0x007C0F00<CMapDoodadDef, HASHKEY_DWORD>, 0x007C1000<CMapObjDef, HASHKEY_NONE>, 0x007DD8E0<TALKINGPLAYER, CHashKeyGUID>, 0x007DDAD0<MUTEDPLAYER, CHashKeyGUID>, 0x007E0580<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F2D60<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F54A0<MINIMAPMD5NAME, HASHKEY_STRI>, 0x0080A4D0, 0x0080A6C0<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00811150<RACECLASSINFO, HASHKEY_NONE>, 0x00811340<SKILLCOSTSINFO, HASHKEY_NONE>, 0x00811540<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x00813390<HashedNode, HASHKEY_STRI>, 0x00813580<FrameFactoryNode, HASHKEY_STRI>, 0x0081B070<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084A250<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0084B9B0<BITMAPBYICON, HASHKEY_NONE>, 0x00876880<CShaderEffect, HASHKEY_STRI>, 0x008779B0<CacheData, HASHKEY_NONE>, 0x0087C140<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTable<T, TKey>::Destroy() {
    this->InternalClear(1);
    this->m_slotmask = 0xFFFFFFFF;
    this->m_slotlistarray.Clear();
}

// INLINED
template <class T, class TKey>
void TSHashTable<T, TKey>::Delete(T* ptr) {
    this->Unlink(ptr);
    this->InternalDelete(ptr);
}

// VERIFY: 5884344f manual
// OFFSET: [0x007671D0<CVar, HASHKEY_STRI>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::DeleteNode(T* ptr) {
    T* next = this->m_fulllist.Next(ptr);
    this->Unlink(ptr);
    this->InternalDelete(ptr);

    return next;
}

// VERIFY: fecb3f7f manual
// OFFSET: [0x00767000<CVar, HASHKEY_STRI>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::Next(T* ptr) {
    return this->m_fulllist.Next(ptr);
}

// VERIFY: 35086749 auto
// OFFSET: [0x00492C70<FRAMEATTR, HASHKEY_STRI>, 0x00496CE0<SIMPLEFONT, HASHKEY_STRI>, 0x004B9000<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB800<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BCB50<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BCD20<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BE750<FONTHASHOBJ, HASHKEY_STR>, 0x004C0A20<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004CAF90<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CCFC0<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CD190<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE550<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF720<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D6800<CGObject_C, CHashKeyGUID>, 0x004F3660<CACHEENTRY, HASHKEY_NONE>, 0x00506AB0<WORDLIST, HASHKEY_LANGUAGE>, 0x00506D40<CHATTHROTTLE, HASHKEY_STR>, 0x00506F10<PROFANITYCHECK, HASHKEY_STR>, 0x005070E0<SPAMCHECK, HASHKEY_STR>, 0x00529D90<SAVEDVARIABLE, HASHKEY_STR>, 0x00529F60, 0x0052A240, 0x0052F590<USEABLESTYLE, HASHKEY_NONE>, 0x0053F900<SPELLSBYNAME, HASHKEY_UTF8I>, 0x005491D0<WORLDSTATE, HASHKEY_NONE>, 0x0055A6A0<LFGListParty, CHashKeyGUID>, 0x0055A870<LFGListPlayer, CHashKeyGUID>, 0x00561270<KEYCOMMAND, HASHKEY_STRI>, 0x00561440<KEYBINDING, HASHKEY_STRI>, 0x00561610<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x005617E0<MODIFIEDCLICK, HASHKEY_STRI>, 0x005619B0<CLICKFRAME, HASHKEY_STR>, 0x005679A0<MACRONODE, HASHKEY_NONE>, 0x005764C0, 0x00593110<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B68B0<ACHIEVEMENT, HASHKEY_NONE>, 0x005B6A80<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C1AA0, 0x005C1C70<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C8A40<TalentGroupItem_C, HASHKEY_NONE>, 0x005C8C10<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D2950<FACTION, HASHKEY_NONE>, 0x005F09E0<MacroConditions, HASHKEY_STR>, 0x005F4270<_BANNEDADDON, HASHKEY_NONE>, 0x005F66B0<UIADDON::METADATA, HASHKEY_STRI>, 0x005F6880<ADDONSTATE, HASHKEY_STRI>, 0x005F6A50<UIADDON, HASHKEY_STRI>, 0x005FD000<MOUSELOOKBINDING, HASHKEY_STRI>, 0x00618AF0<PLAYERPORTRAIT, CHashKeyGUID>, 0x00618CC0<UNITPORTRAIT, HASHKEY_DWORD>, 0x00618EF0<ITEMPORTRAIT, HASHKEY_STRI>, 0x00675AB0, 0x006773F0, 0x006775C0, 0x00677790, 0x00677960, 0x00677B30, 0x00677D00, 0x00677ED0, 0x006780A0, 0x00678270, 0x00678440, 0x00678610, 0x006787E0, 0x006789B0, 0x00678B80, 0x00678D50, 0x006894C0<CGxShader, HASHKEY_STRI>, 0x006BC520<CLightList, HASHKEY_DWORD>, 0x006C3370<CHARCODEDESC, HASHKEY_NONE>, 0x006C3540<KERNNODE, KERNINGHASHKEY>, 0x006C5B40, 0x006C87B0<FACEDATA, HASHKEY_STRI>, 0x006E74A0, 0x006E7670<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F4B80<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F4D50<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F4F20<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F50F0<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F52C0<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5490<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5660<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x0070A310<INVENTORYART, HASHKEY_NONE>, 0x0070A540<ITEMBYNAME, HASHKEY_UTF8I>, 0x007378D0<UnitThreat, CHashKeyGUID>, 0x00767D50<CVar, HASHKEY_STRI>, 0x00768E90<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x0076D430<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x00777120<RGN, HASHKEY_NONE>, 0x007B09F0<CMapObj, HASHKEY_NONE>, 0x007BE580, 0x007BE760<CMapObjDef, HASHKEY_NONE>, 0x007DDC50<TALKINGPLAYER, CHashKeyGUID>, 0x007DDE20<MUTEDPLAYER, CHashKeyGUID>, 0x007E0670<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F2E50<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F5590, 0x0080A840<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE>, 0x0080AA10<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00811720<RACECLASSINFO, HASHKEY_NONE>, 0x008118F0<SKILLCOSTSINFO, HASHKEY_NONE>, 0x00811AC0<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x00813700<HashedNode, HASHKEY_STRI>, 0x008138D0<FrameFactoryNode, HASHKEY_STRI>, 0x0081B160<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084BB00<BITMAPBYICON, HASHKEY_NONE>, 0x0084BCD0<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0086F280<OsNet::NETSELSOCKPTR, HASHKEY_NONE>, 0x00876970<CShaderEffect, HASHKEY_STRI>, 0x0087C230<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTable<T, TKey>::GrowListArray(uint32_t newarraysize) {
    uint32_t oldarraysize = this->m_slotmask + 1;

    TSExplicitList<T> templist;
    templist.ChangeLinkOffset(offsetof(T, m_linktoslot));

    for (uint32_t i = 0; i < oldarraysize; i++) {
        T* node;

        while ((node = this->m_slotlistarray[i].Head()) != nullptr) {
            templist.LinkToTail(node);
        }
    }

    this->m_slotlistarray.SetCount(newarraysize);

    for (uint32_t i = 0; i < newarraysize; i++) {
        this->m_slotlistarray[i].ChangeLinkOffset(offsetof(T, m_linktoslot));
    }

    this->m_slotmask = newarraysize - 1;

    T* node;

    while ((node = templist.Head()) != nullptr) {
        this->m_slotlistarray[node->m_hashval & this->m_slotmask].LinkToTail(node);
    }
}

// VERIFY: 31469875 manual
// OFFSET: [0x004C9FD0<VSOUND_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::Head() {
    return this->m_fulllist.Head();
}

// VERIFY: f91710d3 manual
// OFFSET: [0x00492C10<FRAMEATTR, HASHKEY_STRI>, 0x00496C80<SIMPLEFONT, HASHKEY_STRI>, 0x004B8FA0<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB7A0<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BCA90<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BCAF0<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BE6F0<FONTHASHOBJ, HASHKEY_STR>, 0x004C09C0<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004CAF30<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CCF00<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CCF60<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE4F0<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF6C0<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D67A0<CGObject_C, CHashKeyGUID>, 0x004F3600<CACHEENTRY, HASHKEY_NONE>, 0x005069F0<WORDLIST, HASHKEY_LANGUAGE>, 0x00506A50<CHATTHROTTLE, HASHKEY_STR>, 0x00506C80<PROFANITYCHECK, HASHKEY_STR>, 0x00506CE0<SPAMCHECK, HASHKEY_STR>, 0x00529CD0<SAVEDVARIABLE, HASHKEY_STR>, 0x00529D30, 0x0052A1E0, 0x0052F530<USEABLESTYLE, HASHKEY_NONE>, 0x0053F8A0<SPELLSBYNAME, HASHKEY_UTF8I>, 0x00549170<WORLDSTATE, HASHKEY_NONE>, 0x0055A5E0<LFGListParty, CHashKeyGUID>, 0x0055A640<LFGListPlayer, CHashKeyGUID>, 0x00561090<KEYCOMMAND, HASHKEY_STRI>, 0x005610F0<KEYBINDING, HASHKEY_STRI>, 0x00561150<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x005611B0<MODIFIEDCLICK, HASHKEY_STRI>, 0x00561210<CLICKFRAME, HASHKEY_STR>, 0x00567940<MACRONODE, HASHKEY_NONE>, 0x00576460, 0x005930B0<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B67F0<ACHIEVEMENT, HASHKEY_NONE>, 0x005B6850<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C19E0, 0x005C1A40<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C8980<TalentGroupItem_C, HASHKEY_NONE>, 0x005C89E0<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D28F0<FACTION, HASHKEY_NONE>, 0x005F0980<MacroConditions, HASHKEY_STR>, 0x005F4210<_BANNEDADDON, HASHKEY_NONE>, 0x005F6590<UIADDON::METADATA, HASHKEY_STRI>, 0x005F65F0<ADDONSTATE, HASHKEY_STRI>, 0x005F6650<UIADDON, HASHKEY_STRI>, 0x005FCFA0<MOUSELOOKBINDING, HASHKEY_STRI>, 0x00618A30<PLAYERPORTRAIT, CHashKeyGUID>, 0x00618A90<UNITPORTRAIT, HASHKEY_DWORD>, 0x00618E90<ITEMPORTRAIT, HASHKEY_STRI>, 0x00675A50, 0x00676E50, 0x00676EB0, 0x00676F10, 0x00676F70, 0x00676FD0, 0x00677030, 0x00677090, 0x006770F0, 0x00677150, 0x006771B0, 0x00677210, 0x00677270, 0x006772D0, 0x00677330, 0x00677390, 0x00689460<CGxShader, HASHKEY_STRI>, 0x006BC4C0<CLightList, HASHKEY_DWORD>, 0x006C32B0<CHARCODEDESC, HASHKEY_NONE>, 0x006C3310<KERNNODE, KERNINGHASHKEY>, 0x006C5AE0, 0x006C8750<FACEDATA, HASHKEY_STRI>, 0x006E73E0, 0x006E7440<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F48E0<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F4940<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F49A0<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F4A00<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F4A60<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F4AC0<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F4B20<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x0070A2B0<INVENTORYART, HASHKEY_NONE>, 0x0070A4E0<ITEMBYNAME, HASHKEY_UTF8I>, 0x00737870<UnitThreat, CHashKeyGUID>, 0x00767CF0<CVar, HASHKEY_STRI>, 0x00768E30<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x0076D3D0<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x007770C0<RGN, HASHKEY_NONE>, 0x007B0990<CMapObj, HASHKEY_NONE>, 0x007BE4C0, 0x007BE520<CMapObjDef, HASHKEY_NONE>, 0x007DDB90<TALKINGPLAYER, CHashKeyGUID>, 0x007DDBF0<MUTEDPLAYER, CHashKeyGUID>, 0x007E0610<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F2DF0<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F5530, 0x0080A780<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE>, 0x0080A7E0<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00811600<RACECLASSINFO, HASHKEY_NONE>, 0x00811660<SKILLCOSTSINFO, HASHKEY_NONE>, 0x008116C0<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x00813640<HashedNode, HASHKEY_STRI>, 0x008136A0<FrameFactoryNode, HASHKEY_STRI>, 0x0081B100<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084BA40<BITMAPBYICON, HASHKEY_NONE>, 0x0084BAA0<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0086F220<OsNet::NETSELSOCKPTR, HASHKEY_NONE>, 0x00876910<CShaderEffect, HASHKEY_STRI>, 0x0087C1D0<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTable<T, TKey>::Initialize() {
    this->m_slotmask = 3;
    this->m_slotlistarray.SetCount(4);

    for (uint32_t i = 0; i <= this->m_slotmask; i++) {
        this->m_slotlistarray[i].ChangeLinkOffset(offsetof(T, m_linktoslot));
    }
}

// VERIFY: cb2fc38e manual
// OFFSET: [0x004B93B0<CTexture, HASHKEY_TEXTUREFILE>, 0x004D6A70<CGObject_C, CHashKeyGUID>, 0x004F38D0<CACHEENTRY, HASHKEY_NONE>, 0x0073F200<UnitThreat, CHashKeyGUID>, 0x007B0C60<CMapObj, HASHKEY_NONE>, 0x007BEA70, 0x007BEAE0<CMapObjDef, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTable<T, TKey>::Insert(T* ptr, uint32_t hashval, const TKey& key) {
    this->InternalLinkNode(ptr, hashval);
    ptr->m_key = key;
}

// VERIFY: 97fea5a3 manual
// OFFSET: 0x006C8A20
template <class T, class TKey>
void TSHashTable<T, TKey>::Insert(T* ptr, uint32_t hashval, const char* str) {
    this->InternalLinkNode(ptr, hashval);
    ptr->m_key = str;
}

// VERIFY: 831e7198 auto
// OFFSET: 0x006C8AB0
template <class T, class TKey>
void TSHashTable<T, TKey>::Insert(T* ptr, const char* str) {
    this->Insert(ptr, SStrHashHT(str), str);
}

// VERIFY: 48e089d6 manual
// OFFSET: [0x00591990<CALENDAREVENTHASH, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTable<T, TKey>::InternalClear(int32_t warn) {
    this->m_fullnessIndicator = 0;
    this->m_fulllist.UnlinkAll();

    if (warn) {
        for (uint32_t i = 0; i < this->m_slotlistarray.Count(); i++) {
            this->m_slotlistarray[i].UnlinkAll();
        }
    } else {
        for (uint32_t i = 0; i < this->m_slotlistarray.Count(); i++) {
            T* node;

            while ((node = this->m_slotlistarray[i].Head()) != nullptr) {
                this->InternalDelete(node);
            }
        }
    }
}

// INLINED
template <class T, class TKey>
void TSHashTable<T, TKey>::InternalLinkNode(T* ptr, uint32_t hashval) {
    if (this->m_slotmask == 0xFFFFFFFF) {
        this->Initialize();
    }

    uint32_t slot = hashval & this->m_slotmask;

    if (this->MonitorFullness(slot)) {
        slot = hashval & this->m_slotmask;
    }

    this->m_slotlistarray[slot].LinkToTail(ptr);
    this->m_fulllist.LinkToTail(ptr);
    ptr->m_hashval = hashval;
}

// VERIFY: 532abd43 auto
// OFFSET: [0x00492F10<FRAMEATTR, HASHKEY_STRI>, 0x004BD260<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BD2C0<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004CB520<VSOUND_LOOKUP, HASHKEY_NONE>, 0x005082C0<PROFANITYCHECK, HASHKEY_STR>, 0x0052A5F0<SAVEDVARIABLE, HASHKEY_STR>, 0x005626A0<KEYCOMMAND, HASHKEY_STRI>, 0x00562700<KEYBINDING, HASHKEY_STRI>, 0x00562760<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x005627C0<MODIFIEDCLICK, HASHKEY_STRI>, 0x005B6D90<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C9020<TalentGroupItem_C, HASHKEY_NONE>, 0x005C9190<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005F73B0<UIADDON::METADATA, HASHKEY_STRI>, 0x005FD370<MOUSELOOKBINDING, HASHKEY_STRI>, 0x006192A0<PLAYERPORTRAIT, CHashKeyGUID>, 0x00679D30<DBCache<NameCache, unsigned __int64, CHashKeyGUID>::REVERSEENTRY, HASHKEY_CONSTSTR>, 0x0067AF50<DBCache<CreatureStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067AFB0<DBCache<GameObjectStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B010, 0x0067B070<DBCache<ItemStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B0D0<DBCache<NPCText, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B130<DBCache<NameCache, unsigned __int64, CHashKeyGUID>::DBCACHEHASH, CHashKeyGUID>, 0x0067B190<DBCache<GuildStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B1F0<DBCache<QuestCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B250<DBCache<PageTextCache_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B2B0<DBCache<PetNameCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B310<DBCache<CGPetition, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B370<DBCache<ItemTextCache_C, unsigned __int64, CHashKeyGUID>::DBCACHEHASH, CHashKeyGUID>, 0x0067B3D0, 0x0067B430<DBCache<ArenaTeamCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x0067B490<DBCache<DanceCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x00689760<CGxShader, HASHKEY_STRI>, 0x006C3C50<KERNNODE, KERNINGHASHKEY>, 0x006C5DB0<BATCHEDRENDERFONTDESC, HASHKEY_PTR>, 0x006E7A40<TRADESKILLLINE, HASHKEY_NONE>, 0x006E7AA0<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F5D20<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F6080<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x0076D7B0<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x0086F4F0<OsNet::NETSELSOCKPTR, HASHKEY_NONE>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::InternalNewNode(uint32_t hashval, size_t extrabytes, uint32_t flags) {
    if (this->m_slotmask == 0xFFFFFFFF) {
        this->Initialize();
    }

    uint32_t slot = hashval & this->m_slotmask;

    if (this->MonitorFullness(slot)) {
        slot = hashval & this->m_slotmask;
    }

    T* node = this->InternalNew(&this->m_slotlistarray[slot], extrabytes, flags);
    this->m_fulllist.LinkToTail(node);

    return node;
}

// VERIFY: 96f0e432 manual
// OFFSET: [0x00492E70<FRAMEATTR, HASHKEY_STRI>, 0x00496ED0<SIMPLEFONT, HASHKEY_STRI>, 0x004B9310<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB9D0<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BD010<ProfileInternal::KEYVALUE, HASHKEY_CONSTSTRI>, 0x004BD0B0<ProfileInternal::SECTION, HASHKEY_CONSTSTRI>, 0x004BE920<FONTHASHOBJ, HASHKEY_STR>, 0x004C0DC0<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004CB310<VSOUND_LOOKUP, HASHKEY_NONE>, 0x004CD360<SOUNDKITLOOKUP, HASHKEY_STRI>, 0x004CD400<ZONESOUNDOVERRIDELOOKUP, HASHKEY_STRI>, 0x004CE720<DSP_CHAIN_HEAD_LOOKUP, HASHKEY_STRI>, 0x004CF8F0<FOOTSTEPSNDCACHE, HASHKEY_NONE>, 0x004D69D0<CGObject_C, CHashKeyGUID>, 0x004F3830<CACHEENTRY, HASHKEY_NONE>, 0x00507E10<WORDLIST, HASHKEY_LANGUAGE>, 0x00507EB0<CHATTHROTTLE, HASHKEY_STR>, 0x00507F50<PROFANITYCHECK, HASHKEY_STR>, 0x00507FF0<SPAMCHECK, HASHKEY_STR>, 0x0052A410<SAVEDVARIABLE, HASHKEY_STR>, 0x0052A4B0, 0x0052A550, 0x0052F910<USEABLESTYLE, HASHKEY_NONE>, 0x005405B0<SPELLSBYNAME, HASHKEY_UTF8I>, 0x005493A0<WORLDSTATE, HASHKEY_NONE>, 0x0055AC40<LFGListParty, CHashKeyGUID>, 0x0055ACE0<LFGListPlayer, CHashKeyGUID>, 0x00561DD0<KEYCOMMAND, HASHKEY_STRI>, 0x00561E70<KEYBINDING, HASHKEY_STRI>, 0x00561F10<OVERRIDEKEYBINDING, HASHKEY_STRI>, 0x00561FB0<MODIFIEDCLICK, HASHKEY_STRI>, 0x00562050<CLICKFRAME, HASHKEY_STR>, 0x00567B70<MACRONODE, HASHKEY_NONE>, 0x00576690, 0x00593830<TAXIPATHNODESCLIENT, HASHKEY_NONE>, 0x005B6C50<ACHIEVEMENT, HASHKEY_NONE>, 0x005B6CF0<ACHIEVEMENTCRITERIALIST, HASHKEY_NONE>, 0x005C1EF0, 0x005C1F90<DYNAMICHOLIDAYHASH, HASHKEY_NONE>, 0x005C8E50<TalentGroupItem_C, HASHKEY_NONE>, 0x005C8EF0<TalentGroupTierItem_C, HASHKEY_NONE>, 0x005D2B20<FACTION, HASHKEY_NONE>, 0x005F0D50<MacroConditions, HASHKEY_STR>, 0x005F4440<_BANNEDADDON, HASHKEY_NONE>, 0x005F7060<UIADDON::METADATA, HASHKEY_STRI>, 0x005F7100<ADDONSTATE, HASHKEY_STRI>, 0x005F71A0<UIADDON, HASHKEY_STRI>, 0x005FD220<MOUSELOOKBINDING, HASHKEY_STRI>, 0x006190C0<PLAYERPORTRAIT, CHashKeyGUID>, 0x00619160<UNITPORTRAIT, HASHKEY_DWORD>, 0x00619200<ITEMPORTRAIT, HASHKEY_STRI>, 0x00679240, 0x006793D0, 0x00679470, 0x00679510, 0x006795B0, 0x00679650, 0x006796F0, 0x00679790, 0x00679830, 0x006798D0, 0x00679970, 0x00679A10, 0x00679AB0, 0x00679B50, 0x00679BF0, 0x00679C90, 0x006896C0<CGxShader, HASHKEY_STRI>, 0x006BC8C0<CLightList, HASHKEY_DWORD>, 0x006C3820<CHARCODEDESC, HASHKEY_NONE>, 0x006C38C0<KERNNODE, KERNINGHASHKEY>, 0x006C5D10, 0x006C8980<FACEDATA, HASHKEY_STRI>, 0x006E7900, 0x006E79A0<PROFICIENCYSPELL, HASHKEY_NONE>, 0x006F5A00<OBJ_EFFECT_GROUP_DEF_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F5AA0<OBJ_EFFECT_REF_LOOKUP, HASHKEY_NONE>, 0x006F5B40<OBJ_EFFECT_REF_LIST_LOOKUP, HASHKEY_NONE>, 0x006F5BE0<OBJ_EFFECT_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5C80<OBJ_EFFECT_GROUP_DEF_LOOKUP, HASHKEY_NONE>, 0x006F5D80<OBJ_EFFECT_GROUP_LOOKUP, HASHKEY_NONE>, 0x006F5E20<OBJ_EFFECT_PACKAGE_DEF_LOOKUP, HASHKEY_NONE>, 0x0070A710<INVENTORYART, HASHKEY_NONE>, 0x0070A7B0<ITEMBYNAME, HASHKEY_UTF8I>, 0x0073A7F0<UnitThreat, CHashKeyGUID>, 0x00767F20<CVar, HASHKEY_STRI>, 0x00769060<CONSOLECOMMAND, HASHKEY_CONSTSTRI>, 0x0076D6A0<TIMESTAMPSTRUCT, HASHKEY_NONE>, 0x00777380<RGN, HASHKEY_NONE>, 0x007B0BC0<CMapObj, HASHKEY_NONE>, 0x007BE930, 0x007BE9D0<CMapObjDef, HASHKEY_NONE>, 0x007DF2D0<TALKINGPLAYER, CHashKeyGUID>, 0x007DF370<MUTEDPLAYER, CHashKeyGUID>, 0x007E0840<DECLINEDWORD, HASHKEY_CONSTSTR>, 0x007F3020<DayNight::DNOverrideSky, HASHKEY_STRI>, 0x007F5B00, 0x0080C650<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE>, 0x0080C6F0<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID>, 0x00811D40<RACECLASSINFO, HASHKEY_NONE>, 0x00811DE0<SKILLCOSTSINFO, HASHKEY_NONE>, 0x00811E80<SKILLLINEREC, HASHKEY_SKILLLINE>, 0x00813AA0<HashedNode, HASHKEY_STRI>, 0x00813B40<FrameFactoryNode, HASHKEY_STRI>, 0x0081B550<FrameScript_EventObject, HASHKEY_CONSTSTRI>, 0x0084BEA0<BITMAPBYICON, HASHKEY_NONE>, 0x0084BF40<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x0086F450<OsNet::NETSELSOCKPTR, HASHKEY_NONE>, 0x00876B40<CShaderEffect, HASHKEY_STRI>, 0x0087C670<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE>]
template <class T, class TKey>
int32_t TSHashTable<T, TKey>::MonitorFullness(uint32_t slot) {
    if (this->m_slotmask >= 0x1FFF) {
        return 0;
    }

    this->m_fullnessIndicator = this->m_fullnessIndicator > 3 ? this->m_fullnessIndicator - 3 : 0;

    for (T* node = this->m_slotlistarray[slot].Head(); node; node = this->m_slotlistarray[slot].Next(node)) {
        if (++this->m_fullnessIndicator > 13) {
            this->m_fullnessIndicator = 0;
            this->GrowListArray(2 * this->m_slotmask + 2);
            return 1;
        }
    }

    return 0;
}

// VERIFY: f5512cc0 auto
// OFFSET: [0x0067B600, 0x0067B630, 0x0067B660, 0x00619300]
template <class T, class TKey>
T* TSHashTable<T, TKey>::New(uint32_t hashval, const TKey& key, size_t extrabytes, uint32_t flags) {
    T* node = this->InternalNewNode(hashval, extrabytes, flags);
    node->m_hashval = hashval;
    node->m_key = key;

    return node;
}

// VERIFY: 5082a074 auto
// OFFSET: [0x00509720<PROFANITYCHECK, HASHKEY_STR>, 0x00562870<KEYCOMMAND, HASHKEY_STRI>, 0x005628D0<KEYBINDING, HASHKEY_STRI>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::New(uint32_t hashval, const char* str, size_t extrabytes, uint32_t flags) {
    T* node = this->InternalNewNode(hashval, extrabytes, flags);
    node->m_hashval = hashval;
    node->m_key = str;

    return node;
}

// VERIFY: 69b20f8a auto
// OFFSET: [0x005097A0<PROFANITYCHECK, HASHKEY_STR>, 0x00562960<KEYBINDING, HASHKEY_STRI>, 0x00562930]
template <class T, class TKey>
T* TSHashTable<T, TKey>::New(const char* str, size_t extrabytes, uint32_t flags) {
    return this->New(SStrHashHT(str), str, extrabytes, flags);
}

// VERIFY: cb777ac9 manual
// OFFSET: [0x004B6D90<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB1E0<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004D4BB0<CGObject_C, CHashKeyGUID>, 0x00505480<WORDLIST, HASHKEY_LANGUAGE>, 0x006792E0<DBCache<NameCache, unsigned __int64, CHashKeyGUID>::DBCACHEHASH, CHashKeyGUID>, 0x00679350<DBCache<WardenCachedModule, CWardenKey, CWardenKey>::DBCACHEHASH, CWardenKey>, 0x006BBD40<CLightList, HASHKEY_DWORD>, 0x006C2A50<KERNNODE, KERNINGHASHKEY>, 0x006F6020<DBCache<GuildStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>, 0x00734790<UnitThreat, CHashKeyGUID>, 0x007B0690<CMapObj, HASHKEY_NONE>, 0x007BDD80<CMapDoodadDef, HASHKEY_DWORD>, 0x007BDDF0<CMapObjDef, HASHKEY_NONE>, 0x00810970]
template <class T, class TKey>
T* TSHashTable<T, TKey>::Ptr(uint32_t hashval, const TKey& key) {
    if (this->m_slotmask == 0xFFFFFFFF) {
        return nullptr;
    }

    TSExplicitList<T>& list = this->m_slotlistarray[hashval & this->m_slotmask];

    for (T* node = list.Head(); node; node = list.Next(node)) {
        if (node->m_hashval == hashval && node->m_key == key) {
            return node;
        }
    }

    return nullptr;
}

// VERIFY: fca99b11 manual
// OFFSET: [0x004BC410<LCDHANDLE, HASHKEY_CONSTSTRI>, 0x004BE0E0<FONTHASHOBJ, HASHKEY_STR>, 0x0053F3B0<SPELLSBYNAME, HASHKEY_UTF8I>, 0x0055F4D0<MODIFIEDCLICK, HASHKEY_STRI>, 0x00617C50<KNOWNDANCES, HASHKEY_STR>, 0x00687870<CGxShader, HASHKEY_STRI>, 0x006C8100<FACEDATA, HASHKEY_STRI>, 0x007E0250<DECLINEDWORD, HASHKEY_CONSTSTR>]
template <class T, class TKey>
T* TSHashTable<T, TKey>::Ptr(const char* str) {
    if (this->m_slotmask == 0xFFFFFFFF) {
        return nullptr;
    }

    uint32_t hashval = SStrHashHT(str);
    TSExplicitList<T>& list = this->m_slotlistarray[hashval & this->m_slotmask];

    for (T* node = list.Head(); node; node = list.Next(node)) {
        if (node->m_hashval == hashval && node->m_key == str) {
            return node;
        }
    }

    return nullptr;
}

// VERIFY: b253b260 manual
// OFFSET: [0x004D4790<CGObject_C, CHashKeyGUID>, 0x00565030<CACHEENTRY, HASHKEY_NONE>, 0x00685230<CMapObj, HASHKEY_NONE>, 0x0079E6A0<CMapObjDef, HASHKEY_NONE>, 0x00723270, 0x007BFE80<CMapDoodadDef, HASHKEY_DWORD>]
template <class T, class TKey>
void TSHashTable<T, TKey>::Unlink(T* ptr) {
    if (ptr->m_linktoslot.IsLinked()) {
        ptr->m_linktoslot.Unlink();
        ptr->m_linktofull.Unlink();
    }
}

// VERIFY: b87209d2 manual
// OFFSET: [0x00776F30<RGN, HASHKEY_NONE>]
template <class T, class TKey>
TSHashTableReuse<T, TKey>::TSHashTableReuse() {
    this->m_reuseList.ChangeLinkOffset(offsetof(T, m_linktoslot));
}

// VERIFY: dcbf18cd manual
// OFFSET: [0x004B8F10, 0x004B8F70, 0x004BB710<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004BB770, 0x004C0D30<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x004C0D90, 0x006BC830<CLightList, HASHKEY_DWORD>, 0x006BC890, 0x007772F0<RGN, HASHKEY_NONE>, 0x00777350]
template <class T, class TKey>
TSHashTableReuse<T, TKey>::~TSHashTableReuse() {
    this->Destructor();
}

// VERIFY: 1442481d auto
// OFFSET: [0x004B8EF0, 0x004BB650<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004C0BF0<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x006BBB40<CLightList, HASHKEY_DWORD>, 0x007768E0<RGN, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTableReuse<T, TKey>::Destroy() {
    this->Clear();
    this->Destructor();
}

// VERIFY: 6604cd54 auto
// OFFSET: [0x004B6690, 0x004BAEF0<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004C0040<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x006BB880<CLightList, HASHKEY_DWORD>, 0x00776870<RGN, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTableReuse<T, TKey>::Destructor() {
    T* node;

    while ((node = this->m_reuseList.Head()) != nullptr) {
        this->m_reuseList.UnlinkNode(node);
        SMemFree(node, StormTypeName(typeid(T)), SLOG_OBJECT, 0);
    }
}

// VERIFY: a0b4896c manual
// OFFSET: [0x004B8E70, 0x004C0C10<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x006BC6F0<CLightList, HASHKEY_DWORD>, 0x00777000<RGN, HASHKEY_NONE>]
template <class T, class TKey>
void TSHashTableReuse<T, TKey>::InternalDelete(T* ptr) {
    this->m_fulllist.UnlinkNode(ptr);
    ptr->~T();
    new (&ptr->m_linktoslot) TSLink<T>;
    this->m_reuseList.LinkToHead(ptr);
}

// VERIFY: 187b5e87 manual
// OFFSET: [0x004B9240<CTexture, HASHKEY_TEXTUREFILE>, 0x004BB670<CModelBlob::CHashEntry, HASHKEY_MODELBLOBFILE>, 0x004C0C90<CTextureBlobTexture, HASHKEY_TEXTUREBLOBFILE>, 0x006BC770<CLightList, HASHKEY_DWORD>, 0x00776F90<RGN, HASHKEY_NONE>]
template <class T, class TKey>
T* TSHashTableReuse<T, TKey>::InternalNew(TSExplicitList<T>* listptr, size_t extrabytes, uint32_t flags) {
    T* node = this->m_reuseList.Head();

    if (!node) {
        return listptr->NewNode(1, 0, 0);
    }

    this->m_reuseList.UnlinkNode(node);
    new (node) T;
    listptr->LinkToHead(node);

    return node;
}

// INLINED
template <class T, class THandle>
TSExportTableSimpleReuse<T, THandle>::TSExportTableSimpleReuse() {
    this->m_sequence = static_cast<uint32_t>(~reinterpret_cast<uintptr_t>(this) & 0xFFFFFFF);
    this->m_wrapped = 0;
}

// INLINED
template <class T, class THandle>
void TSExportTableSimpleReuse<T, THandle>::Delete(T* ptr) {
    TSHashTable<T, HASHKEY_NONE>::Delete(ptr);
}

// VERIFY: 50a85424 manual
// OFFSET: 0x00777080
template <class T, class THandle>
THandle TSExportTableSimpleReuse<T, THandle>::GenerateUniqueHandle() {
    do {
        while (++this->m_sequence == 0) {
            this->m_wrapped = 1;
        }
    } while (this->m_wrapped && this->Ptr(reinterpret_cast<THandle>(static_cast<uintptr_t>(this->m_sequence))));

    return reinterpret_cast<THandle>(static_cast<uintptr_t>(this->m_sequence));
}

// INLINED
template <class T, class THandle>
T* TSExportTableSimpleReuse<T, THandle>::New(THandle* handle) {
    THandle uniquehandle = this->GenerateUniqueHandle();
    *handle = uniquehandle;

    return TSHashTable<T, HASHKEY_NONE>::New(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(uniquehandle)), this->m_key, 0, 0);
}

// INLINED
template <class T, class THandle>
T* TSExportTableSimpleReuse<T, THandle>::Ptr(THandle handle) {
    return TSHashTable<T, HASHKEY_NONE>::Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(handle)), this->m_key);
}

// VERIFY: 252836f7 auto
// OFFSET: 0x00777900
template <class T, class THandle, class TLockedHandle, class TSync>
TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::~TSExportTableSyncReuse() {
}

// VERIFY: 21249c03 manual
// OFFSET: 0x007778A0
template <class T, class THandle, class TLockedHandle, class TSync>
void TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::Delete(THandle handle) {
    TLockedHandle lockedhandle;
    T* ptr = this->Lock(handle, &lockedhandle, 1);
    this->DeleteUnlock(ptr, lockedhandle);
}

// INLINED
template <class T, class THandle, class TLockedHandle, class TSync>
void TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::DeleteUnlock(T* ptr, TLockedHandle lockedhandle) {
    TSExportTableSimpleReuse<T, THandle>::Delete(ptr);
    this->Unlock(lockedhandle);
}

// INLINED
template <class T, class THandle, class TLockedHandle, class TSync>
int32_t TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::IsForWriting(TLockedHandle lockedhandle) {
    return lockedhandle == reinterpret_cast<TLockedHandle>(1);
}

// INLINED
template <class T, class THandle, class TLockedHandle, class TSync>
T* TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::Lock(THandle handle, TLockedHandle* lockedhandle, int32_t forwriting) {
    this->SyncEnterLock(lockedhandle, forwriting);
    T* ptr = this->Ptr(handle);

    if (!ptr) {
        this->SyncLeaveLock(*lockedhandle);
        *lockedhandle = nullptr;
    }

    return ptr;
}

// INLINED
template <class T, class THandle, class TLockedHandle, class TSync>
T* TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::NewLock(THandle* handle, TLockedHandle* lockedhandle) {
    this->SyncEnterLock(lockedhandle, 1);
    return this->New(handle);
}

// INLINED
template <class T, class THandle, class TLockedHandle, class TSync>
void TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::SyncEnterLock(TLockedHandle* lockedhandle, int32_t forwriting) {
    this->m_sync.Enter(forwriting);
    *lockedhandle = reinterpret_cast<TLockedHandle>(static_cast<intptr_t>(2 * (forwriting != 0) - 1));
}

// INLINED
template <class T, class THandle, class TLockedHandle, class TSync>
void TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::SyncLeaveLock(TLockedHandle lockedhandle) {
    if (lockedhandle) {
        this->m_sync.Leave(this->IsForWriting(lockedhandle));
    }
}

// VERIFY: d83857d4 manual
// OFFSET: 0x00776240
template <class T, class THandle, class TLockedHandle, class TSync>
void TSExportTableSyncReuse<T, THandle, TLockedHandle, TSync>::Unlock(TLockedHandle lockedhandle) {
    this->SyncLeaveLock(lockedhandle);
}

// INLINED
inline bool HASHKEY_CONSTSTR::operator==(const char* str) {
    return this->m_str == str || SStrCmp(this->m_str, str, STORM_MAX_STR) == 0;
}

// INLINED
inline HASHKEY_CONSTSTR& HASHKEY_CONSTSTR::operator=(const char* str) {
    this->m_str = str;
    return *this;
}

// INLINED
inline HASHKEY_CONSTSTRI& HASHKEY_CONSTSTRI::operator=(const char* str) {
    this->m_str = str;
    return *this;
}

// INLINED
inline bool HASHKEY_CONSTSTRI::operator==(const char* str) {
    return this->m_str == str || SStrCmpI(this->m_str, str, STORM_MAX_STR) == 0;
}

// INLINED
inline bool HASHKEY_NONE::operator==(const HASHKEY_NONE& key) {
    return true;
}

// INLINED
inline bool HASHKEY_DWORD::operator==(const HASHKEY_DWORD& key) {
    return this->m_key == key.m_key;
}

// INLINED
inline bool HASHKEY_PTR::operator==(const HASHKEY_PTR& key) {
    return this->m_key == key.m_key;
}

// INLINED
inline HASHKEY_STR::HASHKEY_STR() {
    this->m_str = nullptr;
}

// INLINED
inline HASHKEY_STR::~HASHKEY_STR() {
    if (this->m_str) {
        SMemFree(this->m_str, __FILE__, __LINE__, 0);
    }
}

// INLINED
inline HASHKEY_STR& HASHKEY_STR::operator=(const char* str) {
    if (this->m_str != str) {
        if (this->m_str) {
            SMemFree(this->m_str, __FILE__, __LINE__, 0);
        }

        this->m_str = SStrDupA(str, __FILE__, __LINE__);
    }

    return *this;
}

// INLINED
inline bool HASHKEY_STR::operator==(const char* str) {
    return SStrCmp(this->m_str, str, STORM_MAX_STR) == 0;
}

// INLINED
inline HASHKEY_STRI& HASHKEY_STRI::operator=(const char* str) {
    HASHKEY_STR::operator=(str);
    return *this;
}

// INLINED
inline bool HASHKEY_STRI::operator==(const char* str) {
    return SStrCmpI(this->m_str, str, STORM_MAX_STR) == 0;
}

// INLINED
inline HASHKEY_UTF8I& HASHKEY_UTF8I::operator=(const char* str) {
    if (this->m_str != str) {
        if (this->m_str) {
            SMemFree(this->m_str, __FILE__, __LINE__, 0);
        }

        this->m_str = SStrDupA(str, __FILE__, __LINE__);
    }

    return *this;
}

// INLINED
inline bool HASHKEY_UTF8I::operator==(const char* str) {
    return SStrCmpUTF8I(this->m_str, str, STORM_MAX_STR) == 0;
}

// INLINED
template <class T>
void TSBitField<T>::Load(const void* data, uint32_t bytes, int32_t bits) {
    uint32_t count = bits == -1 ? bytes * 8 : bits;
    this->m_count = count;
    this->m_bits.SetCount(count ? ((count - 1) >> 5) + 1 : 0);
    memcpy(this->m_bits.m_data, data, bytes);
}

// VERIFY: d4042b48 manual
// OFFSET: [0x00408270, 0x00801020]
template <class T>
TSStackArray<T>::TSStackArray(T* data, uint32_t count, int32_t construct) {
    this->m_data = data;

    if (construct) {
        this->m_count = count;

        for (uint32_t i = 0; i < count; i++) {
            new (&this->m_data[i]) T();
        }
    } else {
        this->m_count = 0;
    }
}

// INLINED
template <class T, uint32_t N>
TSCGrowableArray<T, N>::TSCGrowableArray() {
    this->m_count = 0;
}

// INLINED
template <class T, uint32_t N>
TSCGrowableArray<T, N>::~TSCGrowableArray() {
    if (this->m_count == 0xFFFFFFFF) {
        this->m_dynamic.~TSGrowableArray<T>();
    } else {
        this->m_count = 0;
    }
}

// VERIFY: 47bab0b9 auto
// OFFSET: [0x0071C640, 0x007917B0]
template <class T, uint32_t N>
T& TSCGrowableArray<T, N>::operator[](uint32_t index) {
    if (this->m_count == 0xFFFFFFFF) {
        return this->m_dynamic.m_data[index];
    }

    return this->m_inline[index];
}

// VERIFY: ab2e9eb9 manual
// OFFSET: [0x00728B10, 0x00728B90, 0x00728C10<unsigned int, 16>, 0x007B3B70<CMapObjDefGroup*, 4>]
template <class T, uint32_t N>
void TSCGrowableArray<T, N>::BecomeDynamic() {
    const uint32_t overlap = (sizeof(TSGrowableArray<T>) + sizeof(T) - 1) / sizeof(T);
    uint32_t count = this->m_count;

    if (count > overlap) {
        count = overlap;
    }

    T saved[overlap];

    if (count) {
        memcpy(saved, this->m_inline, sizeof(T) * count);
    }

    new (&this->m_dynamic) TSGrowableArray<T>();
    this->m_dynamic.Add(count, saved);
    this->m_dynamic.Add(this->m_count - count, &this->m_inline[count]);
    this->m_count = 0xFFFFFFFF;
}

// INLINED
template <class T, uint32_t N>
void TSCGrowableArray<T, N>::SetCount(uint32_t count) {
    if (this->m_count == 0xFFFFFFFF) {
        this->m_dynamic.SetCount(count);
    } else if (count <= N) {
        this->m_count = count;
    } else {
        this->BecomeDynamic();
        this->m_dynamic.SetCount(count);
    }
}

// VERIFY: dbe4dc73 manual
// OFFSET: [0x00801120]
template <class T>
void TSStackArray<T>::SetCount(uint32_t count) {
    if (count == this->m_count) {
        return;
    }

    for (uint32_t i = this->m_count; i < count; i++) {
        new (&this->m_data[i]) T();
    }

    this->m_count = count;
}

#endif
