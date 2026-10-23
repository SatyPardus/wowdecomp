#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleFontable.h"
#include "Engine/Source/Frame/CSimpleFont.h"

// INLINED
CSimpleFontable::CSimpleFontable() {
    this->m_fontableFlags = FLAG_COMPLETE_UPDATE;
    this->m_fontObject = nullptr;
}

// VERIFY: 85bbcb87 auto
// OFFSET: [0x004851B0, 0x00485210]
CSimpleFontable::~CSimpleFontable() {
    this->SetFontObject(nullptr);
}

// VERIFY: c87fff35 auto
// OFFSET: 0x00485160
void CSimpleFontable::SetFontObject(CSimpleFont* font) {
    if (this->m_fontObject == font) {
        return;
    }

    if (this->m_fontObject) {
        this->m_fontObject->RemoveFontable(this);
    }

    this->m_fontObject = font;

    if (font) {
        font->m_fontableList.LinkToTail(this);
        font->m_attributes.m_flags |= FLAG_COMPLETE_UPDATE;
        this->FontObjectUpdated(font->m_attributes);
        font->m_attributes.m_flags &= ~FLAG_COMPLETE_UPDATE;
    }
}
