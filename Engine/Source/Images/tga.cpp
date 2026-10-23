#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/Images/tga.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"

// INLINED
CTgaFile::CTgaFile() {
    this->m_file = nullptr;
    this->m_image = nullptr;
    this->m_addlHeaderData = nullptr;
    this->m_imageBytes = 0;
    this->m_colorMap = nullptr;
}

// INLINED
CTgaFile::~CTgaFile() {
    this->Close();
}

// INLINED
uint32_t CTgaFile::AlphaBits() const {
    return this->m_header.m_desc.m_bAlphaChannelBits;
}

// INLINED
int32_t CTgaFile::BytesPerPixel() const {
    return (this->m_header.m_bPixelDepth + 7) / 8;
}

// INLINED
uint32_t CTgaFile::Height() const {
    return this->m_header.m_wHeight;
}

// INLINED
int32_t CTgaFile::IsTopDown() const {
    return this->m_header.m_desc.m_bTopBottomOrder;
}

// INLINED
uint32_t CTgaFile::Width() const {
    return this->m_header.m_wWidth;
}

// VERIFY: ebdf4fdf manual
// OFFSET: 0x006AA350
uint32_t CTgaFile::ColorMapEntryBytes() {
    int32_t bits = this->m_header.m_bColorMapEntrySize / 3;

    if (bits >= 8) {
        bits = 8;
    }

    return 3 * bits / 8;
}

// VERIFY: 473a99ae manual
// OFFSET: 0x006AA380
uint32_t CTgaFile::ColorMapBytes() {
    return this->m_header.m_wColorMapEntries * this->ColorMapEntryBytes();
}

// VERIFY: ff3c9ea4 auto
// OFFSET: 0x006AA3B0
int32_t CTgaFile::ValidateColorDepth() {
    if (this->m_header.m_bPixelDepth - this->AlphaBits() == 24) {
        return 1;
    }

    SErrSetLastError(this->m_header.m_bPixelDepth == 16 ? 0xF720007C : 0xF720007D);

    return 0;
}

// VERIFY: 9527dbf2 manual
// OFFSET: 0x006AA3E0
uint32_t CTgaFile::PreImageBytes() {
    return sizeof(TGAHeader) + this->m_header.m_bIDLength + this->ColorMapBytes();
}

// VERIFY: 624d1043 manual
// OFFSET: 0x006AA420
void CTgaFile::AddAlphaChannel(uint8_t* dest, uint8_t* source, const uint8_t* alpha) {
    uint32_t pixels = this->Width() * this->Height();

    if (alpha) {
        while (pixels--) {
            memmove(dest, source, this->BytesPerPixel());
            source += this->BytesPerPixel();
            dest += this->BytesPerPixel();
            *dest++ = *alpha++;
        }
    } else {
        while (pixels--) {
            memmove(dest, source, this->BytesPerPixel());
            source += this->BytesPerPixel();
            dest += this->BytesPerPixel();
            *dest++ = 0xFF;
        }
    }

    uint32_t alphaBits = this->AlphaBits();
    this->m_header.m_desc.m_bAlphaChannelBits = 8;
    this->m_header.m_bPixelDepth += 8 - alphaBits;
    this->m_imageBytes = this->Width() * this->Height() * this->BytesPerPixel();
}

// VERIFY: 2da73a68 manual
// OFFSET: 0x006AA520
int32_t CTgaFile::ReadRawImage(uint32_t flags) {
    int32_t addAlpha = (flags & 1) && !this->AlphaBits() ? 1 : 0;

    if (SFile::SetFilePointer(this->m_file, this->PreImageBytes(), nullptr, 0) == -1) {
        return 0;
    }

    uint32_t bytes = this->Width() * this->Height() * this->BytesPerPixel();
    this->m_image = static_cast<uint8_t*>(SMemAlloc(bytes + addAlpha * this->Width() * this->Height(), __FILE__, __LINE__, 0));

    if (!this->m_image || !SFileReadFile(this->m_file, &this->m_image[addAlpha * this->Width() * this->Height()], bytes, nullptr, nullptr, 0)) {
        return 0;
    }

    if (addAlpha) {
        this->AddAlphaChannel(this->m_image, &this->m_image[this->Width() * this->Height()], nullptr);
    }

    return 1;
}

// VERIFY: 6803203f manual
// OFFSET: 0x006AA630
int32_t CTgaFile::RLEDecompressImage(uint8_t* rleData, uint8_t* data) {
    int32_t pixels = this->Width() * this->Height();
    int32_t bytesPerPixel = this->BytesPerPixel();

    while (pixels) {
        uint8_t packet = *rleData++;

        if (packet & 0x80) {
            int32_t count = (packet & 0x7F) + 1;
            pixels -= count;

            while (count--) {
                memcpy(data, rleData, bytesPerPixel);
                data += bytesPerPixel;
            }

            rleData += bytesPerPixel;
        } else {
            int32_t count = packet + 1;
            pixels -= count;
            memcpy(data, rleData, bytesPerPixel * count);
            rleData += bytesPerPixel * count;
            data += bytesPerPixel * count;
        }

        if (pixels < 0) {
            SErrSetLastError(0xF7200077);
            return 0;
        }
    }

    this->m_header.m_bImageType -= 8;

    return 1;
}

// VERIFY: c8c32a57 manual
// OFFSET: 0x006AA700
int32_t CTgaFile::SetTopDown(int32_t topDown) {
    if (topDown) {
        if (this->IsTopDown()) {
            return 1;
        }
    } else if (!this->IsTopDown()) {
        return 1;
    }

    if (this->m_header.m_bImageType >= TGA_RLE_COLOR_MAPPED && this->m_header.m_bImageType <= TGA_RLE_BLACK_N_WHITE) {
        SErrSetLastError(0xF7200083);
        return 0;
    }

    uint8_t* image = static_cast<uint8_t*>(SMemAlloc(this->Width() * this->Height() * this->BytesPerPixel(), __FILE__, __LINE__, 0));
    uint8_t* source = this->m_image;
    uint8_t* dest = &image[this->Width() * this->BytesPerPixel() * (this->Height() - 1)];

    for (uint32_t row = this->Height(); row; row--) {
        memcpy(dest, source, this->Width() * this->BytesPerPixel());
        source += this->Width() * this->BytesPerPixel();
        dest -= this->Width() * this->BytesPerPixel();
    }

    SMemFree(this->m_image, __FILE__, __LINE__, 0);
    this->m_header.m_desc.m_bTopBottomOrder = topDown != 0;
    this->m_image = image;

    return 1;
}

// VERIFY: e8b28006 auto
// OFFSET: 0x006AA820
uint8_t* CTgaFile::Image() {
    if (!this->m_image) {
        SErrSetLastError(0xF720007F);
        return nullptr;
    }

    return this->m_image;
}

// VERIFY: 76a8714f auto
// OFFSET: 0x006AA840
TGA32Pixel* CTgaFile::ImageTGA32Pixel() {
    if (!this->m_image) {
        SErrSetLastError(0xF720007F);
        return nullptr;
    }

    if (this->m_header.m_bPixelDepth != 32) {
        SErrSetLastError(0xF720007D);
        return nullptr;
    }

    return reinterpret_cast<TGA32Pixel*>(this->m_image);
}

// VERIFY: 186ad0ea manual
// OFFSET: 0x006AA870
int32_t CTgaFile::RemoveAlphaChannels() {
    if (!this->m_image) {
        SErrSetLastError(0xF720007F);
        return 0;
    }

    if (!this->AlphaBits()) {
        if (this->m_header.m_bPixelDepth == 24) {
            return 1;
        }

        SErrSetLastError(0x8720012D);
        this->m_header.m_desc.m_bAlphaChannelBits = this->m_header.m_bPixelDepth - 8;
    }

    if (this->AlphaBits() != 8) {
        SErrSetLastError(0xF7200082);
        return 0;
    }

    this->m_header.m_bPixelDepth -= 8;
    this->m_header.m_desc.m_bAlphaChannelBits = 0;

    uint32_t pixels = this->Width() * this->Height();
    uint8_t* dest = this->m_image;
    uint8_t* source = this->m_image;

    while (pixels--) {
        memmove(dest, source, this->BytesPerPixel());
        dest += this->BytesPerPixel();
        source += this->BytesPerPixel() + 1;
    }

    return 1;
}

// VERIFY: 57c94d27 manual
// OFFSET: 0x006AA950
int32_t CTgaFile::SetImage(const void* bits, uint16_t width, uint16_t height, uint8_t pixelDepth, uint8_t alphaBits, int32_t topBottomOrder, int32_t leftRightOrder) {
    if (!bits || (pixelDepth != 32 && pixelDepth != 24) || (alphaBits && alphaBits != 8)) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    memset(&this->m_header, 0, sizeof(this->m_header));
    this->m_header.m_wHeight = height;
    this->m_header.m_desc.m_bAlphaChannelBits = alphaBits;
    this->m_header.m_desc.m_bLeftRightOrder = leftRightOrder != 0;
    this->m_header.m_desc.m_bTopBottomOrder = topBottomOrder != 0;
    this->m_header.m_bPixelDepth = pixelDepth;
    this->m_header.m_wWidth = width;
    this->m_imageBytes = width * height * ((pixelDepth + 7) / 8);
    this->m_header.m_bImageType = TGA_TRUE_COLOR;

    if (this->m_image) {
        SMemFree(this->m_image, __FILE__, __LINE__, 0);
    }

    this->m_image = static_cast<uint8_t*>(SMemAlloc(this->m_imageBytes, __FILE__, __LINE__, 0));

    if (!this->m_image) {
        return 0;
    }

    memcpy(this->m_image, bits, this->m_imageBytes);
    this->m_footer.m_dwExtensionOffset = 0;
    this->m_footer.m_dwDeveloperOffset = 0;
    SStrCopy(this->m_footer.m_szSigniture, "TRUEVISION-XFILE.", sizeof(this->m_footer.m_szSigniture));

    return 1;
}

// VERIFY: 10a5f209 manual
// OFFSET: 0x006AAA70
int32_t CTgaFile::CountRun(const uint8_t* source, int32_t count) {
    int32_t bytesPerPixel = this->BytesPerPixel();
    int32_t run = 0;
    uint32_t pixel = 0;
    memcpy(&pixel, source, bytesPerPixel);

    if (count > 128) {
        count = 128;
    }

    while (count--) {
        if (memcmp(source, &pixel, bytesPerPixel)) {
            break;
        }

        source += bytesPerPixel;
        run++;
    }

    return run;
}

// VERIFY: 4aec56da manual
// OFFSET: 0x006AAB70
int32_t CTgaFile::RleCompressLine(uint8_t** source, uint8_t** dest) {
    uint8_t* in = *source;
    uint8_t* out = *dest;
    uint8_t* rawPacket = nullptr;
    int32_t remaining = this->Width();

    while (remaining) {
        int32_t run = this->CountRun(in, remaining);

        if (run >= 2) {
            this->m_imageBytes += this->BytesPerPixel() + 1;
            rawPacket = nullptr;

            if (this->m_imageBytes >= this->BytesPerPixel() * this->Width() * this->Height()) {
                return 0;
            }

            *out++ = (run - 1) | 0x80;
            memcpy(out, in, this->BytesPerPixel());
            out += this->BytesPerPixel();
            in += run * this->BytesPerPixel();
            remaining -= run;
        } else {
            if (!rawPacket || *rawPacket == 127) {
                rawPacket = out;
                *out++ = 0;
                this->m_imageBytes++;
            } else {
                (*rawPacket)++;
            }

            this->m_imageBytes += this->BytesPerPixel();

            if (this->m_imageBytes >= this->BytesPerPixel() * this->Width() * this->Height()) {
                return 0;
            }

            memcpy(out, in, this->BytesPerPixel());
            out += this->BytesPerPixel();
            remaining--;
            in += this->BytesPerPixel();
        }
    }

    *source = in;
    *dest = out;

    return 1;
}

// VERIFY: 37799519 manual
// OFFSET: 0x006AACE0
int32_t CTgaFile::Compress() {
    if (!this->m_image) {
        SErrSetLastError(0xF7200081);
        return 0;
    }

    if (this->m_header.m_bImageType >= TGA_RLE_COLOR_MAPPED) {
        SErrSetLastError(0xF7200083);
        return 0;
    }

    uint8_t* source = this->m_image;
    uint8_t* compressed = static_cast<uint8_t*>(SMemAlloc(this->m_imageBytes, __FILE__, __LINE__, 0));
    uint8_t* dest = compressed;

    if (!compressed) {
        return 0;
    }

    this->m_imageBytes = 0;

    for (uint32_t row = this->Height(); row--; ) {
        if (!this->RleCompressLine(&source, &dest)) {
            SMemFree(compressed, __FILE__, __LINE__, 0);
            this->m_imageBytes = this->Height() * this->Width() * this->BytesPerPixel();
            return 1;
        }
    }

    this->m_header.m_bImageType += 8;
    SMemFree(this->m_image, __FILE__, __LINE__, 0);
    this->m_image = compressed;

    return 1;
}

// VERIFY: 6d8bf332 manual
// OFFSET: 0x006AADE0
int32_t CTgaFile::Write(const char* fileName) {
    if (!fileName) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!this->m_image) {
        SErrSetLastError(0xF7200081);
        return 0;
    }

    HOSFILE file = OsCreateFile(fileName, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file == OS_INVALID_FILE) {
        return 0;
    }

    int32_t result = 0;
    uint32_t written;

    if (OsWriteFile(file, &this->m_header, sizeof(this->m_header), &written) && written == sizeof(this->m_header)
        && (!this->m_header.m_bIDLength || (OsWriteFile(file, this->m_addlHeaderData, this->m_header.m_bIDLength, &written) && written == this->m_header.m_bIDLength))
        && (!this->m_header.m_bColorMapType || (OsWriteFile(file, this->m_colorMap, this->ColorMapBytes(), &written) && written == this->ColorMapBytes()))
        && OsWriteFile(file, this->m_image, this->m_imageBytes, &written) && written == this->m_imageBytes
        && OsWriteFile(file, &this->m_footer, sizeof(this->m_footer), &written) && written == sizeof(this->m_footer)) {
        result = 1;
    }

    OsCloseFile(file);

    if (!result) {
        OsDeleteFile(fileName);
    }

    return result;
}

// VERIFY: 7fb86d57 manual
// OFFSET: 0x006AAF40
void CTgaFile::Close() {
    if (this->m_image) {
        SMemFree(this->m_image, __FILE__, __LINE__, 0);
    }

    this->m_image = nullptr;

    if (this->m_file) {
        SFileCloseFile(this->m_file);
    }

    this->m_file = nullptr;

    if (this->m_addlHeaderData) {
        SMemFree(this->m_addlHeaderData, __FILE__, __LINE__, 0);
    }

    this->m_addlHeaderData = nullptr;
    this->m_header.m_bIDLength = 0;

    if (this->m_colorMap) {
        SMemFree(this->m_colorMap, __FILE__, __LINE__, 0);
    }

    this->m_colorMap = nullptr;
}

// VERIFY: 3fd6ee84 manual
// OFFSET: 0x006AAFB0
int32_t CTgaFile::Open(const char* fileName, int32_t flags) {
    if (!fileName || !*fileName) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    this->Close();

    if (!SFile::OpenEx(nullptr, fileName, flags != 0, &this->m_file)) {
        return 0;
    }

    if (!SFileReadFile(this->m_file, &this->m_header, sizeof(this->m_header), nullptr, nullptr, 0)) {
        return 0;
    }

    if (this->m_header.m_bIDLength) {
        this->m_addlHeaderData = static_cast<uint8_t*>(SMemAlloc(this->m_header.m_bIDLength, __FILE__, __LINE__, 0));

        if (!SFileReadFile(this->m_file, this->m_addlHeaderData, this->m_header.m_bIDLength, nullptr, nullptr, 0)) {
            return 0;
        }
    } else {
        this->m_addlHeaderData = nullptr;
    }

    if (this->m_header.m_bColorMapType) {
        this->m_colorMap = static_cast<uint8_t*>(SMemAlloc(this->ColorMapBytes(), __FILE__, __LINE__, 0));

        if (!SFileReadFile(this->m_file, this->m_colorMap, this->ColorMapBytes(), nullptr, nullptr, 0)) {
            return 0;
        }
    } else {
        this->m_colorMap = nullptr;
    }

    if (this->m_header.m_bPixelDepth == 24 && this->AlphaBits() == 8) {
        this->m_header.m_desc.m_bAlphaChannelBits = 0;
        SErrSetLastError(0x8720012E);
    }

    if (this->m_header.m_bPixelDepth == 32 && !this->AlphaBits()) {
        this->m_header.m_desc.m_bAlphaChannelBits = 8;
        SErrSetLastError(0x8720012F);
    }

    return 1;
}

// VERIFY: e42758f1 manual
// OFFSET: 0x006AB0E0
void CTgaFile::ConvertColorMapped(uint32_t flags) {
    uint8_t* indices = this->m_image;
    uint32_t pixelDepth = this->AlphaBits() + 24;
    uint32_t addAlpha = flags & 1;
    this->m_image = static_cast<uint8_t*>(SMemAlloc(addAlpha * this->Width() * this->Height() + this->Width() * this->Height() * (pixelDepth >> 3), __FILE__, __LINE__, 0));

    uint8_t* dest = &this->m_image[addAlpha * this->Width() * this->Height()];
    uint8_t* index = indices;

    for (uint32_t pixels = this->Width() * this->Height(); pixels; pixels--) {
        memcpy(dest, &this->m_colorMap[(*index++ - this->m_header.m_wColorMapStartIndex) * this->ColorMapEntryBytes()], this->ColorMapEntryBytes());
        dest += this->ColorMapEntryBytes();
    }

    SMemFree(indices, __FILE__, __LINE__, 0);
    SMemFree(this->m_colorMap, __FILE__, __LINE__, 0);
    this->m_colorMap = nullptr;
    this->m_header.m_wColorMapEntries = 0;
    this->m_header.m_bColorMapType = 0;
    this->m_header.m_bImageType = TGA_TRUE_COLOR;
    this->m_header.m_bPixelDepth = pixelDepth;
    this->m_imageBytes = this->Width() * this->Height() * ((pixelDepth + 7) / 8);

    if (addAlpha) {
        this->AddAlphaChannel(this->m_image, &this->m_image[this->Width() * this->Height()], nullptr);
    }
}

// VERIFY: e3c0914e manual
// OFFSET: 0x006AB220
int32_t CTgaFile::ReadRleImage(uint32_t flags) {
    int32_t addAlpha = (flags & 1) && !this->AlphaBits() ? 1 : 0;
    this->m_image = static_cast<uint8_t*>(SMemAlloc(this->Height() * this->Width() * this->BytesPerPixel() + addAlpha * this->Height() * this->Width(), __FILE__, __LINE__, 0));

    if (!this->m_image) {
        return 0;
    }

    uint32_t bytes = SFileGetFileSize(this->m_file, nullptr) - this->PreImageBytes();

    if (bytes == 0xFFFFFFFF) {
        return 0;
    }

    uint8_t* rleData = static_cast<uint8_t*>(SMemAlloc(bytes, __FILE__, __LINE__, 0));

    if (!rleData) {
        return 0;
    }

    if (SFile::SetFilePointer(this->m_file, this->PreImageBytes(), nullptr, 0) == -1) {
        return 0;
    }

    if (!SFileReadFile(this->m_file, rleData, bytes, nullptr, nullptr, 0)) {
        return 0;
    }

    int32_t result = this->RLEDecompressImage(rleData, &this->m_image[addAlpha * this->Height() * this->Width()]);
    SMemFree(rleData, __FILE__, __LINE__, 0);

    if (!result) {
        return 0;
    }

    if (addAlpha) {
        this->AddAlphaChannel(this->m_image, &this->m_image[this->Height() * this->Width()], nullptr);
    }

    return 1;
}

// VERIFY: a86156e1 manual
// OFFSET: 0x006AB390
int32_t CTgaFile::AddAlphaChannel(const void* alpha) {
    if (!this->m_image) {
        SErrSetLastError(0xF720007F);
        return 0;
    }

    if (this->m_header.m_bImageType >= TGA_RLE_COLOR_MAPPED && this->m_header.m_bImageType <= TGA_RLE_BLACK_N_WHITE) {
        SErrSetLastError(0xF7200083);
        return 0;
    }

    if (this->AlphaBits()) {
        this->RemoveAlphaChannels();
    }

    uint8_t* image = static_cast<uint8_t*>(SMemAlloc(this->Width() * this->Height() * (this->BytesPerPixel() + 1), __FILE__, __LINE__, 0));

    if (!image) {
        return 0;
    }

    this->AddAlphaChannel(image, this->m_image, static_cast<const uint8_t*>(alpha));
    SMemFree(this->m_image, __FILE__, __LINE__, 0);
    this->m_image = image;

    return 1;
}

// VERIFY: 94d6cd9e auto
// OFFSET: 0x006AB450
int32_t CTgaFile::ReadColorMappedImage(uint32_t flags) {
    if (!this->m_header.m_bColorMapType) {
        SErrSetLastError(0xF7200084);
        return 0;
    }

    int32_t result = this->m_header.m_bImageType >= TGA_RLE_COLOR_MAPPED ? this->ReadRleImage(0) : this->ReadRawImage(0);

    if (this->m_header.m_bColorMapType && (flags & 2)) {
        this->ConvertColorMapped(flags);
    }

    return result;
}

// VERIFY: 68530b8f manual
// OFFSET: 0x006AB4B0
int32_t CTgaFile::LoadImageData(uint32_t flags) {
    if (this->m_image) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 1;
    }

    if (!this->m_file) {
        SErrSetLastError(0xF720007E);
        return 0;
    }

    this->m_imageBytes = this->Width() * this->Height() * this->BytesPerPixel();

    switch (this->m_header.m_bImageType) {
        case TGA_NO_IMAGE_DATA:
            SErrSetLastError(0xF7200078);
            return 0;

        case TGA_COLOR_MAPPED:
        case TGA_RLE_COLOR_MAPPED:
            return this->ReadColorMappedImage(flags);

        case TGA_TRUE_COLOR:
            if (!this->ValidateColorDepth()) {
                return 0;
            }

            return this->ReadRawImage(flags);

        case TGA_BLACK_N_WHITE:
        case TGA_RLE_BLACK_N_WHITE:
            SErrSetLastError(0xF720007A);
            return 0;

        case TGA_RLE_TRUE_COLOR:
            if (!this->ValidateColorDepth()) {
                return 0;
            }

            return this->ReadRleImage(flags);

        default:
            SErrSetLastError(0xF720007B);
            return 0;
    }
}
