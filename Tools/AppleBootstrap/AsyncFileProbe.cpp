#include "Engine/Source/Services/AsyncFile.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/Gx/Blit.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

void CheckServicesTexturePixels(CGxTex* texture, const BLP::Image& reference);
void CheckServicesTexturePixels(CGxTex* texture, const BLP::Image& reference, CTexture* atlasResource);
void CheckGameModels();
void CheckGameCubeTextures();
void CheckGameTerrain();
void CheckGameWmoRoots();
void CheckGameWmoScene();

namespace {
void Require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
struct Read {
    CAsyncObject* object = nullptr;
    std::vector<uint8_t> expected, bytes;
    int succeeded = 0, failed = 0;
    bool cleanupOnFailure = false;
    uint32_t mainThread = SGetCurrentThreadId();
    Read(const char* name, bool pastEnd = false) {
        SFileImpl* file = nullptr;
        Require(SFile::OpenEx(nullptr, name, 0, &file), "Cannot open async probe game resource");
        const uint32_t size = SFileGetFileSize(file, nullptr);
        expected.resize(size);
        Require(SFileReadFile(file, expected.data(), size, nullptr, nullptr, 0), "Cannot read async reference bytes");
        SFile::SetFilePointer(file, 0, nullptr, 0);
        bytes.resize(size + (pastEnd ? 1 : 0));
        object = AsyncFileReadAllocObject();
        object->m_file = file; object->m_buffer = bytes.data(); object->m_size = uint32_t(bytes.size());
        object->m_userArg = this;
        object->m_userPostloadCallback = reinterpret_cast<void*>(Success);
        object->m_userPostUnloadCallback = reinterpret_cast<void*>(Failure);
    }
    ~Read() { if (object) { AsyncFileReadDestroyObject(object); } }
    static void Success(void* arg) {
        auto& read = *static_cast<Read*>(arg);
        Require(SGetCurrentThreadId() == read.mainThread, "Async resource callback escaped the game thread");
        Require(read.bytes == read.expected && !read.object->m_readFailed, "Async game resource bytes changed");
        ++read.succeeded;
    }
    static void Failure(void* arg) {
        auto& read = *static_cast<Read*>(arg);
        Require(SGetCurrentThreadId() == read.mainThread, "Async resource cleanup escaped the game thread");
        ++read.failed;
        if (read.cleanupOnFailure) { AsyncFileReadDestroyObject(read.object); read.object = nullptr; }
    }
};
HPROPCONTEXT resourceContext = nullptr;
int32_t Initialize(const void*, void*) { resourceContext = PropGetSelectedContext(); AsyncFileReadInitialize(0, 20); TextureInitialize(); return 1; }
int32_t Destroy(const void*, void*) { TextureDestroy(); AsyncFileReadDestroy(); return 1; }
}

void CheckGameAsyncFiles(bool modelsOnly) {
    for (int cycle = 0; cycle < 2; ++cycle) {
        PropInitialize(); EventInitialize(1, 0);
        struct Cleanup { bool active = true; ~Cleanup() { if (active) { EventDestroy(); } PropDestroy(); } } cleanup;
        EventCreateContextEx(1, Initialize, Destroy, 1, 0);
        EventProcessPending();
        if (!modelsOnly) CheckGameCubeTextures();
        PropSelectContext(resourceContext);
        CheckGameModels();
        if (modelsOnly) continue;
        CheckGameTerrain();
        CheckGameWmoRoots();
        CheckGameWmoScene();
        {
            CStatus status;
            auto* texture = TextureCreate("Interface\\GLUES\\COMMON\\GLUES-WOW-BCLOGO.BLP", CGxTexFlags(GxTex_NearestMipNearest), &status, 1);
            Require(texture != nullptr, "Original TextureCreate returned no texture");
            struct CloseTexture { CTexture* texture; ~CloseTexture() { HandleClose(reinterpret_cast<HOBJECT>(texture)); } } close{texture};
            Require(TextureGetGxTex(texture, 1, &status) != nullptr && status.IsEmpty(), "Original Services texture did not become ready");
            uint32_t width = 0, height = 0;
            Require(TextureGetDimensions(texture, &width, &height, 1) && width == 256 && height == 128, "Original Services texture dimensions changed");
            auto* cached = TextureCreate("interface\\glues\\common\\glues-wow-bclogo.blp", CGxTexFlags(GxTex_NearestMipNearest), &status, 1);
            Require(cached == texture && TextureGetRefCnt(texture) == 2, "Services filename/flags cache failed");
            HandleClose(reinterpret_cast<HOBJECT>(cached));
            Read source("Interface\\GLUES\\COMMON\\GLUES-WOW-BCLOGO.BLP");
            CheckServicesTexturePixels(texture->m_gxTex, BLP::Image::Load(source.expected.data(), source.expected.size(), 256 * 1024 * 1024));
        }
        if (!cycle) {
            const auto pattern = (std::filesystem::temp_directory_path() / "wow-texture-XXXXXX").string();
            std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
            char* directory = temporary.data();
            Require(mkdtemp(directory) != nullptr, "Cannot create texture probe scratch directory");
            struct Remove { const char* path; ~Remove() { std::filesystem::remove_all(path); } } remove{directory};
            BLPHeader header{};
            header.m_magic = 0x32504C42; header.m_formatVersion = 1; header.m_colorEncoding = 1;
            header.m_preferredFormat = PIXEL_ARGB8888; header.m_alphaSize = 8;
            header.m_width = header.m_height = 2048;
            header.m_mipOffsets[0] = sizeof(header); header.m_mipSizes[0] = 2048 * 2048 * 2;
            header.m_extended.m_palette[1] = {17, 34, 51, 0};
            std::vector<uint8_t> bytes(sizeof(header) + header.m_mipSizes[0], 255);
            std::memcpy(bytes.data(), &header, sizeof(header));
            std::memset(bytes.data() + sizeof(header), 1, 2048 * 2048);
            const auto filename = std::string(directory) + "/large.blp";
            { std::ofstream file(filename, std::ios::binary); file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); Require(bool(file), "Cannot save texture probe asset"); }
            CStatus status;
            auto* large = TextureCreate(filename.c_str(), CGxTexFlags(GxTex_Linear), &status, 1);
            Require(large && large->m_asyncObject && large->m_asyncObject->m_buffer, "Oversized async texture never started");
            struct Close { CTexture* t; ~Close() { HandleClose(reinterpret_cast<HOBJECT>(t)); } } close{large};
            auto* queued = TextureCreate(filename.c_str(), CGxTexFlags(GxTex_Nearest), &status, 1);
            Require(queued && queued != large && queued->m_asyncObject && !queued->m_asyncObject->m_buffer, "Texture budget did not queue the second request");
            HandleClose(reinterpret_cast<HOBJECT>(queued));
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (large->m_asyncObject && std::chrono::steady_clock::now() < deadline) { EventProcessPending(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
            Require(!large->m_asyncObject && large->m_gxTex && large->m_status.IsEmpty(), "Oversized asynchronous palette texture failed");
            CheckServicesTexturePixels(large->m_gxTex, BLP::Image::Load(bytes.data(), bytes.size(), 256 * 1024 * 1024));
            // Exercise the actual Services format selection, not just direct Gx
            // uploads. Keep every authored mip and reload after base changes.
            for (auto preferred : {PIXEL_RGB565, PIXEL_ARGB1555, PIXEL_ARGB4444}) {
                header.m_width = header.m_height = 16; header.m_hasMips = 1;
                header.m_preferredFormat = preferred;
                for (unsigned i = 0; i < 256; ++i) { header.m_extended.m_palette[i] = {uint8_t(i * 13), uint8_t(255 - i), uint8_t(i * 7), 0}; }
                size_t total = sizeof(header);
                for (unsigned level = 0; level < 5; ++level) {
                    header.m_mipOffsets[level] = uint32_t(total); header.m_mipSizes[level] = (16u >> level) * (16u >> level) * 2;
                    total += header.m_mipSizes[level];
                }
                bytes.assign(total, 0); std::memcpy(bytes.data(), &header, sizeof(header));
                for (unsigned level = 0; level < 5; ++level) {
                    const unsigned count = header.m_mipSizes[level] / 2;
                    for (unsigned i = 0; i < count; ++i) { bytes[header.m_mipOffsets[level] + i] = uint8_t(i + level * 7); bytes[header.m_mipOffsets[level] + count + i] = uint8_t(i * 3 + level * 13); }
                }
                const auto path = std::string(directory) + "/palette-" + std::to_string(preferred) + ".blp";
                { std::ofstream file(path, std::ios::binary); file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); Require(bool(file), "Cannot write palette texture probe"); }
                auto* palette = TextureCreate(path.c_str(), CGxTexFlags(GxTex_NearestMipNearest), &status, 1);
                Require(palette != nullptr, "Cannot open Services palette texture");
                Close closePalette{palette};
                Require(TextureGetGxTex(palette, 1, &status) && status.IsEmpty(), "Services packed palette texture failed");
                auto reference = BLP::Image::Load(bytes.data(), bytes.size(), 256 * 1024 * 1024);
                reference.format = preferred == PIXEL_RGB565 ? BLP::Format::RGB565 : preferred == PIXEL_ARGB1555 ? BLP::Format::ARGB1555 : BLP::Format::ARGB4444;
                const BlitFormat blit = preferred == PIXEL_RGB565 ? BlitFormat_Rgb565 : preferred == PIXEL_ARGB1555 ? BlitFormat_Argb1555 : BlitFormat_Argb4444;
                for (auto& mip : reference.mips) {
                    std::vector<uint8_t> packed(size_t(mip.width) * mip.height * 2);
                    Require(Blit(C2iVector{int32_t(mip.width), int32_t(mip.height)}, BlitAlpha_0, mip.bytes.data(), mip.rowBytes, BlitFormat_Argb8888, packed.data(), mip.width * 2, blit), "Cannot build original palette conversion reference");
                    mip.rowBytes = mip.width * 2; mip.bytes = std::move(packed);
                }
                CheckServicesTexturePixels(palette->m_gxTex, reference);
            }

        }
        {
            std::vector<std::unique_ptr<Read>> reads;
            for (const char* name : {"DBFilesClient\\Map.dbc", "Interface\\GlueXML\\GlueStrings.lua", "Cameras\\FlyByDwarf.m2",
                "World\\Maps\\AhnQiraj\\AhnQiraj_26_46.adt", "World\\wmo\\Azeroth\\Buildings\\AltarOfStorms\\AltarOfStorms.wmo"}) {
                reads.push_back(std::make_unique<Read>(name));
                AsyncFileReadObject(reads.back()->object, cycle);
            }
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            bool done = false;
            while (!done && std::chrono::steady_clock::now() < deadline) {
                EventProcessPending(); done = true;
                for (const auto& read : reads) { done &= read->succeeded == 1; }
                if (!done) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
            }
            Require(done, "Async game resource worker did not complete");
            // Waiting on already processed objects must not poison later waits.
            for (const auto& read : reads) { AsyncFileReadWait(read->object); }
            Read failed("DBFilesClient\\Map.dbc", true);
            AsyncFileReadObject(failed.object, 1);
            AsyncFileReadWait(failed.object);
            Require(failed.failed == 1 && failed.succeeded == 0 && failed.object->m_readFailed,
                "Failed async read was reported as successful");
            Read canceled("Cameras\\FlyByDwarf.m2");
            AsyncFile::EnterQueueLock();
            AsyncFileReadObject(canceled.object, 0);
            const auto canceledNow = AsyncFile::Cancel(canceled.object, nullptr);
            AsyncFile::LeaveQueueLock();
            Require(canceledNow == 1, "Queued async read was not canceled under its queue lock");
            canceled.object = nullptr; // Cancel returned the object to the recycler.
            AsyncFileReadWaitAll();
            Require(!AsyncFileReadIsReading(), "Async file queue did not become idle");
        }
        // A queued or just-completed read is cleaned up exactly once during
        // shutdown, without a success callback after its context is destroyed.
        Read abandoned("World\\Maps\\AhnQiraj\\AhnQiraj_26_46.adt");
        abandoned.cleanupOnFailure = true;
        AsyncFileReadObject(abandoned.object, 0);
        EventDestroy(); cleanup.active = false;
        Require(abandoned.failed == 1 && !abandoned.succeeded && !abandoned.object,
            "Async shutdown lost or duplicated pending resource cleanup");
    }
}

void CheckGameTextureAtlases() {
    for (unsigned cycle=0;cycle<2;++cycle) {
        PropInitialize(); EventInitialize(1,0);
        struct ContextCleanup { ~ContextCleanup() { TextureEnableAtlas(0); EventDestroy(); PropDestroy(); } } contextCleanup;
        EventCreateContextEx(1,Initialize,Destroy,1,0); EventProcessPending(); PropSelectContext(resourceContext);
        TextureEnableAtlas(1);
        char directory[]="/tmp/wow-native-atlas-XXXXXX";
        Require(mkdtemp(directory), "Cannot create atlas fixture directory");
        struct Files { const char* path; ~Files() { std::error_code error; std::filesystem::remove_all(path,error); } } files{directory};
        struct Textures { std::vector<CTexture*> handles; ~Textures() { for (auto* h:handles) if(h) HandleClose(reinterpret_cast<HOBJECT>(h)); } } owned;
        const char* names[]{"Interface\\Icons\\INV_Misc_QuestionMark.blp", "Interface\\Icons\\Spell_Nature_Rejuvenation.blp", "Interface\\Icons\\Spell_Nature_HealingTouch.blp"};
        for (unsigned family=0;family<3;++family) {
            SFileImpl* file=nullptr;
            Require(SFile::OpenEx(nullptr,names[family],0,&file), "Cannot open original atlas reference icon");
            std::vector<uint8_t> bytes(SFileGetFileSize(file,nullptr));
            const bool read=SFileReadFile(file,bytes.data(),bytes.size(),nullptr,nullptr,0); SFileCloseFile(file);
            Require(read && bytes.size()>sizeof(BLPHeader), "Cannot read original atlas reference icon");
            auto icon=BLP::Image::Load(bytes.data(),bytes.size(),16*1024*1024);
            Require(BLP::IsBlockCompressed(icon.format) && icon.mips[0].width==64 && icon.mips[0].height==64, "Unexpected reference icon format");
            // Test eligibility only; all original encoded pixel blocks remain unchanged.
            bytes[11] |= 0x10;
            BLP::Image expected; expected.format=icon.format;
            const auto& tile=icon.mips[0]; const uint32_t rowBytes=tile.rowBytes*8;
            expected.mips.push_back({512,512,rowBytes,std::vector<uint8_t>(rowBytes*128)});
            CTextureAtlas* atlas=nullptr; CTexture* representative=nullptr;
            auto load=[&](unsigned index) {
                const auto path=std::string(directory)+"/icon-"+std::to_string(family)+"-"+std::to_string(index)+".blp";
                { std::ofstream out(path,std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); Require(bool(out),"Cannot write atlas fixture"); }
                CStatus status; auto* texture=TextureCreate(path.c_str(),CGxTexFlags(GxTex_Nearest),&status,5);
                Require(texture!=nullptr,"Cannot create atlas texture"); owned.handles.push_back(texture);
                Require(TextureGetGxTex(texture,1,&status) && status.IsEmpty() && texture->m_atlas,"Atlas icon upload failed");
                if (!atlas) atlas=texture->m_atlas;
                Require(atlas==texture->m_atlas && atlas->m_updateSlot==-1,"Atlas allocation or callback state changed");
                const unsigned slot=texture->m_atlasBlockIndex;
                Require(slot<64,"Atlas slot out of range");
                auto& image=expected.mips[0];
                for (unsigned row=0;row<16;++row) std::memcpy(image.bytes.data()+((slot/8)*16+row)*rowBytes+(slot%8)*tile.rowBytes,tile.bytes.data()+row*tile.rowBytes,tile.rowBytes);
                C2Vector offset; float scale;
                Require(TextureGetAtlasOffsetAndScale(texture,offset,scale) && scale==0.125f && offset.x==float(slot%8)/8 && offset.y==float(slot/8)/8,"Original atlas UV layout changed");
                return texture;
            };
            for (unsigned i=0;i<12;++i) { auto* iconTexture=load(i); if (!i) representative=iconTexture; }
            Require(atlas->m_used==12,"Atlas occupied slot count changed");
            CheckServicesTexturePixels(atlas->m_gxTex,expected,representative);
            auto* freed=owned.handles[owned.handles.size()-2]; const auto slot=freed->m_atlasBlockIndex;
            HandleClose(reinterpret_cast<HOBJECT>(freed)); owned.handles[owned.handles.size()-2]=nullptr;
            auto* reused=load(12); Require(reused->m_atlasBlockIndex==slot && atlas->m_used==12,"Atlas did not reuse its released slot");
            CheckServicesTexturePixels(atlas->m_gxTex,expected,representative);
        }
    }
    std::puts("Original texture atlas: BC1/2/3 tile creation, nonzero row/column updates, 78 icon loads, slot reuse, UV layout and all occupied pixels after recreation passed in two lifecycle cycles");
}
