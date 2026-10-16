#ifndef ENGINE_SOURCE_IMAGES_IMAGEWRITER_H
#define ENGINE_SOURCE_IMAGES_IMAGEWRITER_H

#include <cstddef>
#include <cstdint>

struct IWRect {
    /* 0000 */ int16_t x;
    /* 0002 */ int16_t width;
    /* 0004 */ int16_t y;
    /* 0006 */ int16_t height;
};

struct IWEmbedParams {
    /* 0000 */ const uint8_t* m_bits;
    /* 0004 */ uint32_t m_byteCount;
    /* 0008 */ uint8_t* m_pixels;
    /* 000C */ uint32_t m_width;
    /* 0010 */ uint32_t m_height;
    /* 0014 */ uint32_t m_bytesPerPixel;
    /* 0018 */ IWRect* m_rects;
    /* 001C */ uint32_t m_rectCount;
};

struct IWJpegParams {
    /* 0000 */ const void* m_pixels;
    /* 0004 */ const char* m_fileName;
    /* 0008 */ uint32_t m_width;
    /* 000C */ uint32_t m_height;
    /* 0010 */ uint32_t m_bytesPerPixel;
    /* 0014 */ uint32_t m_quality;
    /* 0018 */ int32_t m_subsample;
    /* 001C */ const char* m_comment;
    /* 0020 */ int32_t unused_0020;
};

class IWBitMatrix {
    public:
    static int32_t ResetScratch();
    static void FreeScratch();

    IWBitMatrix();
    ~IWBitMatrix();
    int32_t Initialize(uint32_t size, const char* name, const uint8_t* data);

    /* 0000 */ const char* m_name;
    /* 0004 */ uint32_t m_rows;
    /* 0008 */ uint32_t m_columns;
    /* 000C */ const uint8_t* m_data;
    /* 0010 */ uint32_t m_byteSize;
    /* 0014 */ int32_t m_static;
};

class IWEncoder {
    public:
    IWEncoder();
    ~IWEncoder();
    virtual int32_t Initialize(uint32_t codeBits, uint32_t repeat) = 0;
    virtual void Reset();
    virtual uint32_t Encode(const uint8_t* data, uint32_t size) = 0;

    /* 0004 */ uint32_t m_codeBits;
    /* 0008 */ uint32_t m_sourceBitCount;
    /* 000C */ uint32_t m_checkBitCount;
    /* 0010 */ uint32_t m_repeat;
    /* 0014 */ const uint8_t* m_data;
    /* 0018 */ uint32_t m_dataSize;
    /* 001C */ uint8_t* m_encoded;
    /* 0020 */ uint32_t m_encodedCapacity;
    /* 0024 */ uint32_t m_encodedSize;
};

struct IWCheckNode;

struct IWBitNode {
    /* 0000 */ uint16_t m_index;
    /* 0002 */ uint8_t unused_0002;
    /* 0003 */ uint8_t m_checkCount;
    /* 0004 */ uint8_t unused_0004[16];
    /* 0014 */ IWCheckNode* m_checks[6];
};

struct IWCheckNode {
    /* 0000 */ int32_t m_bitCount;
    /* 0004 */ IWBitNode* m_bits[8];
};

class IWLdpcEncoder : public IWEncoder {
    public:
    IWLdpcEncoder();
    ~IWLdpcEncoder();
    int32_t Initialize(uint32_t codeBits, uint32_t repeat) override;
    void Reset() override;
    uint32_t Encode(const uint8_t* data, uint32_t size) override;
    uint32_t GetEncodedSize(uint32_t size);
    int32_t BuildGraph();

    /* 0028 */ uint32_t m_checkNodeCount;
    /* 002C */ uint32_t m_columnWeight;
    /* 0030 */ uint32_t m_rowWeight;
    /* 0034 */ IWBitMatrix m_sourceBit;
    /* 004C */ IWBitMatrix m_checkBit;
    /* 0064 */ IWBitMatrix m_invCheckBit;
    /* 007C */ IWCheckNode* m_checkNodes;
    /* 0080 */ IWBitNode* m_bitNodes;
    /* 0084 */ uint8_t* m_codeword;
    /* 0088 */ uint8_t* m_sourceBits;
};

int32_t ImageWriterEmbedData(const void* pixels, uint32_t width, uint32_t height, uint32_t bytesPerPixel, const uint8_t* data, uint32_t dataSize);
int32_t ImageWriterInitialize(uint32_t dataSize, void* (*allocFunc)(size_t bytes), void (*freeFunc)(void* ptr), int32_t flags);
int32_t ImageWriterShutdown();
int32_t ImageWriterWriteJpeg(const char* fileName, const void* pixels, uint32_t width, uint32_t height, uint32_t bytesPerPixel, uint32_t quality);

#endif
