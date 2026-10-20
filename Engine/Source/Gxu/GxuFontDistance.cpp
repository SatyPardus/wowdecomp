#if defined(__APPLE__)
#include "Engine/Source/Gxu/GxuFontFont.h"
#include "Engine/Source/Gxu/GxuFontFace.h"
#include "Engine/Source/Gxu/IGxuFontGlyph.h"
#include "Engine/Source/Storm/H/storm.h"
#include <freetype/ftoutln.h>
#include <msdfgen.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
// FreeType's outline is in 26.6 pixels. Preserve curves and winding; do not
// trace a raster image or alter the original Chinese/Latin font resources.
struct Outline {
    msdfgen::Shape shape;
    msdfgen::Point2 position, start;
    static msdfgen::Point2 Point(const FT_Vector* v) { return {v->x / 64.0, v->y / 64.0}; }
    void Close() {
        if (!shape.contours.empty() && position != start)
            shape.contours.back().addEdge(msdfgen::EdgeHolder(position, start));
    }
    static int Move(const FT_Vector* to, void* context) {
        auto& o = *static_cast<Outline*>(context);
        o.Close(); o.shape.addContour(); o.position = o.start = Point(to); return 0;
    }
    static int Line(const FT_Vector* to, void* context) {
        auto& o = *static_cast<Outline*>(context);
        const auto p = Point(to);
        if (o.position != p) o.shape.contours.back().addEdge(msdfgen::EdgeHolder(o.position, p));
        o.position = p; return 0;
    }
    static int Conic(const FT_Vector* control, const FT_Vector* to, void* context) {
        auto& o = *static_cast<Outline*>(context);
        const auto p = Point(to);
        o.shape.contours.back().addEdge(msdfgen::EdgeHolder(o.position, Point(control), p));
        o.position = p; return 0;
    }
    static int Cubic(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* context) {
        auto& o = *static_cast<Outline*>(context);
        const auto p = Point(to);
        o.shape.contours.back().addEdge(msdfgen::EdgeHolder(o.position, Point(c1), Point(c2), p));
        o.position = p; return 0;
    }
};
}

bool CGxFont::GetDistanceGlyph(GLYPHBITMAPDATA* data, uint32_t code) {
    FT_Face face = FontFaceGetFace(m_fontFaceHandle);
    const auto index = FT_Get_Char_Index(face, code);
    if (!index || FT_Set_Pixel_Sizes(face, DISTANCE_LAYOUT_EM, 0)) return false;
    GLYPHBITMAPDATA layout;
    const uint32_t padding = m_fontFlags & 8 ? 4 : m_fontFlags & 1 ? 2 : 0;
    if (!IGxuFontGlyphRenderGlyph(face, DISTANCE_LAYOUT_EM, code,
        m_baseline / (DISTANCE_EM / DISTANCE_LAYOUT_EM), &layout, m_fontFlags & 2, padding, false)) return false;
    // These metrics also cover blank glyphs and the original terminal-width
    // rounding. The bitmap is discarded; only the outline enters the atlas.
    data->m_layoutBearing = layout.m_glyphBearing;
    data->m_layoutWidth = layout.m_glyphCellWidth;
    const float advance = layout.m_glyphAdvance * (float(DISTANCE_EM) / DISTANCE_LAYOUT_EM);
    if (FT_Set_Pixel_Sizes(face, DISTANCE_EM, 0) ||
        FT_Load_Glyph(face, index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_LINEAR_DESIGN) ||
        face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) return false;

    Outline outline;
    const FT_Outline_Funcs callbacks{Outline::Move, Outline::Line, Outline::Conic, Outline::Cubic, 0, 0};
    if (FT_Outline_Decompose(&face->glyph->outline, &callbacks, &outline)) return false;
    outline.Close();
    auto& shape = outline.shape;
    shape.contours.erase(std::remove_if(shape.contours.begin(), shape.contours.end(),
        [](const auto& c) { return c.edges.empty(); }), shape.contours.end());
    shape.normalize();
    if (!shape.validate()) return false;

    const bool empty = shape.contours.empty();
    const auto bounds = empty ? msdfgen::Shape::Bounds{0, 0, 0, 0} : shape.getBounds();
    const int left = int(std::floor(bounds.l)), bottom = int(std::floor(bounds.b));
    const uint32_t width = std::max(1, int(std::ceil(bounds.r)) - left) + 2 * DISTANCE_PADDING;
    if (width > GetTextureSize() || std::ceil(bounds.t) - bottom > m_cellHeight - 2 * DISTANCE_PADDING) return false;
    const uint32_t height = m_cellHeight;
    data->m_glyphAdvance = advance;
    data->m_glyphBearing = left - float(DISTANCE_PADDING);
    data->m_yOffset = int(DISTANCE_EM - m_baseline) + bottom - int(DISTANCE_PADDING);
    data->m_yStart = 0;
    data->m_glyphWidth = data->m_glyphCellWidth = width;
    data->m_glyphHeight = height;
    data->m_glyphPitch = width * 4;
    data->m_dataSize = data->m_glyphPitch * height;
    data->m_data = SMemAlloc(data->m_dataSize, __FILE__, __LINE__, 0);
    if (!data->m_data) return false;
    std::memset(data->m_data, 0, data->m_dataSize);
    if (empty) return true;

    msdfgen::edgeColoringInkTrap(shape, 3.0);
    msdfgen::Bitmap<float, 3> bitmap(width, height);
    msdfgen::generateMSDF(bitmap, shape,
        msdfgen::Projection(msdfgen::Vector2(1),
            msdfgen::Vector2(double(DISTANCE_PADDING) - left, double(DISTANCE_PADDING) - bottom)),
        msdfgen::Range(DISTANCE_RANGE));
    auto* pixels = static_cast<uint32_t*>(data->m_data);
    auto byte = [](float value) { return uint32_t(std::lround(std::clamp(value, 0.0f, 1.0f) * 255)); };
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const float* d = bitmap(x, height - 1 - y);
            pixels[y * width + x] = 0xff000000 | (byte(d[0]) << 16) | (byte(d[1]) << 8) | byte(d[2]);
        }
    }
    return true;
}
#endif
