#include "WoW/Source/Battlenet/Battlenet.h"
#include "WoW/Source/Battlenet/BattlenetProtocol.h"
#include "WoW/Source/Battlenet/GeneralFunction.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <unistd.h>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
struct Allocator final : Battlenet::Allocator {
    bool fail = false;
    unsigned live = 0;
    void* Alloc(uint32_t size) override {
        void* result = fail ? nullptr : std::malloc(size);
        if (result) ++live;
        return result;
    }
    void Free(void* value) override { if (value) { --live; std::free(value); } }
};
struct DestroyLibrary { void operator()(Battlenet::Library* library) const { if (library) library->Destroy(); } };
using Library = std::unique_ptr<Battlenet::Library, DestroyLibrary>;
struct Callback final : GeneralFunction::Callback {
    mutable unsigned destroyed = 0;
    void Call(int32_t, const GeneralFunction::Arg*) const override {}
    void Destroy() const override { ++destroyed; }
};
template<class T> void CopyProtocol(const T& from, T& to) {
    BSN::BSNEncodedObjectPtr<T, Protocol, 1> source(&from), target(&to);
    Require(source.WalkAndSet(&target), "Original BSN protocol walker rejected a valid native object");
}
}

void CheckBattlenetCore() {
    Allocator allocator;
    for (unsigned cycle = 0; cycle != 2; ++cycle) {
        Library first(Battlenet::Library::Load("/usr/lib/libSystem.B.dylib", allocator));
        Library second(Battlenet::Library::LoadApplicationBundle("/usr/lib/libSystem.B", allocator));
        Require(first && second && allocator.live == 2, "Battle.net native library allocation or dylib name failed");
        auto getProcess = reinterpret_cast<pid_t (*)()>(first->GetProc("getpid"));
        Require(getProcess && getProcess() == getpid(), "Battle.net named native export is not callable");
        Require(!first->GetProc(1u) && errno == ENOTSUP && !first->GetProc("wow_missing_export"),
            "Battle.net native module fabricated a Windows ordinal or missing export");
        first.reset();
        getProcess = reinterpret_cast<pid_t (*)()>(second->GetProc("getpid"));
        Require(allocator.live == 1 && getProcess && getProcess() == getpid(), "Battle.net module lost its independent reference");
        second.reset();
        allocator.fail = true;
        Require(!Battlenet::Library::Load("/usr/lib/libSystem.B.dylib", allocator), "Battle.net module ignored allocation failure");
        allocator.fail = false;
        Require(!Battlenet::Library::Load(nullptr, allocator) &&
            !Battlenet::Library::LoadApplicationBundle("wow_missing_battlenet_component", allocator) && !allocator.live,
            "Battle.net failed module load retained allocator storage");
    }

    GeneralFunction::Arg argument;
    argument = "0xFEDCBA9876543210";
    Require(argument.ToUInt64() == UINT64_C(0xFEDCBA9876543210), "Battle.net argument lost high unsigned bits");
    argument = "-9223372036854775808";
    Require(argument.ToInt64() == INT64_MIN, "Battle.net argument lost its signed 64-bit minimum");
    Callback callback;
    {
        argument = &callback;
        GeneralFunction::Arg copy(argument);
        Require(callback.m_refCount == 2, "Battle.net callback copy did not retain its reference");
        argument = false;
        Require(callback.m_refCount == 1 && !callback.destroyed, "Battle.net callback released a live reference");
    }
    Require(callback.m_refCount == 0 && callback.destroyed == 1, "Battle.net callback did not finish exactly once");

    std::array<uint8_t, 8> bytes{};
    bytes[6] = 0xD3; bytes[7] = 0x79;
    BSN::Hard::Encoder encoder;
    encoder.SetData(bytes.data(), 6);
    Require(!encoder.PutBits(3, 5) && !encoder.PutBits(10, 0x2AB) && !encoder.Align() && !encoder.PutUInt32(0x89ABCDEF),
        "Original BSN bit encoder rejected a valid message");
    const std::array<uint8_t, 8> expected{{0xAD, 0x0B, 0x89, 0xAB, 0xCD, 0xEF, 0xD3, 0x79}};
    Require(bytes == expected && encoder.PutBits(1, 1) == BSN::CODEC_OVERFLOW && bytes == expected,
        "Original BSN bit order, alignment or overflow boundary changed");
    Battlenet::IP4::AddressPort address{{127, 0, 0, 1}, {0x1F, 0x90}}, decoded{};
    encoder.SetData(bytes.data(), 6);
    Require(!ProtocolHard::TypeInfoOf<Battlenet::IP4::AddressPort>::Encode(address, encoder) &&
        !std::memcmp(bytes.data(), &address, 6), "Original BSN address/port wire bytes changed");
    BSN::Hard::Decoder decoder;
    decoder.SetData(bytes.data(), 6);
    Require(!ProtocolHard::TypeInfoOf<Battlenet::IP4::AddressPort>::Decode(decoded, decoder) &&
        !std::memcmp(&decoded, &address, 6), "Original BSN address decoding changed");
    decoder.SetData(bytes.data(), 5);
    Require(ProtocolHard::TypeInfoOf<Battlenet::IP4::AddressPort>::Decode(decoded, decoder) == BSN::CODEC_OVERFLOW,
        "Original BSN address decoding accepted a truncated port");

    Battlenet::Toon::FullName from{}, to{};
    from.m_region = 3; from.m_program = BSN::FourCC("WoW"); from.m_realm = 12340; from.m_name = "测试角色";
    CopyProtocol(from, to);
    Require(to.m_region == 3 && to.m_program.m_value == from.m_program.m_value && to.m_realm == 12340 &&
        !std::strcmp(to.m_name.m_data, "测试角色"), "Original BSN member offsets or UTF-8 adapters changed");
    Battlenet::Presence::BlockList blocks{}, copied{};
    blocks.m_accounts.m_count = 2; blocks.m_accounts[0] = 7; blocks.m_accounts[1] = 0xFEDCBA98;
    blocks.m_toons.m_count = 1; blocks.m_toons[0] = 12340;
    CopyProtocol(blocks, copied);
    Require(copied.m_accounts.m_count == 2 && copied.m_accounts[1] == 0xFEDCBA98 &&
        copied.m_toons.m_count == 1 && copied.m_toons[0] == 12340, "Original BSN array adapters changed");
    Battlenet::Friends::FriendInvite invite{}, copiedInvite{};
    invite.m_presenceId = 42; invite.m_message.m_exists = true; invite.m_message.m_value = "一起冒险";
    invite.unk_0210 = 1; invite.m_time = 12345;
    CopyProtocol(invite, copiedInvite);
    Require(copiedInvite.m_presenceId == 42 && copiedInvite.m_message.m_exists &&
        !std::strcmp(copiedInvite.m_message.m_value.m_data, "一起冒险") && copiedInvite.m_time == 12345,
        "Original BSN optional/string field adapters changed");
    invite.m_message.m_exists = false;
    CopyProtocol(invite, copiedInvite);
    Require(!copiedInvite.m_message.m_exists, "Original BSN optional field did not clear");
    std::fputs("Battle.net shared core: native module ownership/failures, 64-bit arguments/refcounts, exact BSN wire bytes and native protocol field/array/UTF-8/optional adapters passed; external authentication component remains unavailable\n", stderr);
}
