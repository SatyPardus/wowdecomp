#include "Engine/Source/Frame/CSimpleEmbeddedTexture.h"
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleTexture.h"

// INLINED
CSimpleEmbeddedTexture::CSimpleEmbeddedTexture() {
    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    this->m_texture = data ? new (data) CSimpleTexture(nullptr, 2, 1) : nullptr;
}

// INLINED
CSimpleEmbeddedTexture::~CSimpleEmbeddedTexture() {
    delete this->m_texture;
}
