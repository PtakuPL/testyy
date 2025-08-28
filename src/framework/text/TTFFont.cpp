#include "TTFFont.h"

#include <cmath>
// OTClient rendering helpers
#include <framework/graphics/drawpoolmanager.h> // g_drawPool
#include <framework/util/rect.h>
#include <framework/util/color.h>
#include <framework/util/point.h>
#include <framework/util/size.h>

TTFFont::TTFFont() {}
TTFFont::~TTFFont() {
  if (m_hbFont) hb_font_destroy(m_hbFont);
  for (auto* f : m_fallbackHbFonts) if (f) hb_font_destroy(f);
  if (m_face) FT_Done_Face(m_face);
  for (auto f : m_fallbackFaces) if (f) FT_Done_Face(f);
  if (m_ftLib) FT_Done_FreeType(m_ftLib);
}

bool TTFFont::load(const std::string& mainTtf,
                   const std::vector<std::string>& /*fallbackTtfs*/,
                   int pixelSize) {
  if (FT_Init_FreeType(&m_ftLib)) return false;
  if (FT_New_Face(m_ftLib, mainTtf.c_str(), 0, &m_face)) return false;
  FT_Set_Pixel_Sizes(m_face, 0, pixelSize);
  m_pixelSize = pixelSize;

  // Create HarfBuzz face/font from FT_Face
  hb_face_t* hbFace = hb_ft_face_create(m_face, nullptr);
  m_hbFont = hb_ft_font_create(m_face, nullptr);
  hb_face_destroy(hbFace);

  ensureAtlas();
  return true;
}

int TTFFont::ensureAtlas() {
  Atlas a;
  a.width = 2048; a.height = 2048;
  a.penX = a.penY = a.rowH = 0;

  // Create blank RGBA image and corresponding GPU texture
  a.image = std::make_shared<Image>(Size(a.width, a.height), 4 /*bpp*/, nullptr);
  a.texture = std::make_shared<Texture>(a.image, /*buildMipmaps*/false, /*compress*/false);
  a.texture->setSmooth(true);

  m_atlases.push_back(a);
  return static_cast<int>(m_atlases.size() - 1);
}

const AtlasGlyph* TTFFont::cacheGlyph(uint32_t glyphIndex) {
  auto it = m_glyphs.find(glyphIndex);
  if (it != m_glyphs.end()) return &it->second;

  // Render glyph using FreeType
  if (FT_Load_Glyph(m_face, glyphIndex, FT_LOAD_DEFAULT)) return nullptr;
  if (FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL)) return nullptr;

  FT_GlyphSlot g = m_face->glyph;
  const int w = g->bitmap.width;
  const int h = g->bitmap.rows;

  // Space/empty glyph
  if (w == 0 || h == 0) {
    AtlasGlyph ag{};
    ag.texture = m_atlases.back().texture;
    ag.x = ag.y = ag.w = ag.h = 0;
    ag.bearingX = g->bitmap_left;
    ag.bearingY = g->bitmap_top;
    ag.advance  = static_cast<int>(g->advance.x);
    m_glyphs[glyphIndex] = ag;
    return &m_glyphs[glyphIndex];
  }

  // Pack into current atlas; make a new one if needed
  Atlas* A = &m_atlases.back();
  if (A->penX + w + 2 > A->width) { A->penX = 0; A->penY += A->rowH + 2; A->rowH = 0; }
  if (A->penY + h + 2 > A->height) { ensureAtlas(); A = &m_atlases.back(); }

  // Convert FT grayscale bitmap -> RGBA (white with alpha)
  ImagePtr glyphImage = std::make_shared<Image>(Size(w, h), 4);
  for (int yy = 0; yy < h; ++yy) {
    const uint8_t* srcRow = g->bitmap.buffer + yy * g->bitmap.pitch;
    for (int xx = 0; xx < w; ++xx) {
      const uint8_t a = srcRow[xx];
      uint8_t* dst = glyphImage->getPixel(xx, yy);
      dst[0] = 255; dst[1] = 255; dst[2] = 255; dst[3] = a;
    }
  }

  // Copy to CPU atlas and upload to GPU
  A->image->blit(Point(A->penX, A->penY), glyphImage);
  A->texture->uploadPixels(A->image, /*buildMipmaps*/false, /*compress*/false);

  // Register glyph metrics
  AtlasGlyph ag{};
  ag.texture  = A->texture;
  ag.x = A->penX; ag.y = A->penY; ag.w = w; ag.h = h;
  ag.bearingX = g->bitmap_left;
  ag.bearingY = g->bitmap_top;
  ag.advance  = static_cast<int>(g->advance.x);
  m_glyphs[glyphIndex] = ag;

  // Advance pen
  A->penX += w + 2;
  if (h > A->rowH) A->rowH = h;

  return &m_glyphs[glyphIndex];
}

void TTFFont::drawText(const std::u32string& text32,
                       float x, float y,
                       const ShapeParams& params) {
  if (!m_hbFont || text32.empty()) return;

  const auto shaped = TextShaper::shape(text32, m_hbFont, params);

  float penX = x, penY = y;
  for (const auto& sg : shaped) {
    const AtlasGlyph* ag = cacheGlyph(sg.glyphIndex);
    if (!ag) { penX += sg.advanceX; penY += sg.advanceY; continue; }

    // Destination (screen) rect; baseline at (x,y)
    const float dx = penX + ag->bearingX + sg.x;
    const float dy = penY - ag->bearingY - sg.y;
    const Rect destRect(static_cast<int>(std::lround(dx)),
                        static_cast<int>(std::lround(dy)),
                        ag->w, ag->h);

    // Source (atlas) rect
    const Rect srcRect(ag->x, ag->y, ag->w, ag->h);

    // Draw
    g_drawPool.addTexturedRect(destRect, ag->texture, srcRect, Color::white);

    penX += sg.advanceX;
    penY += sg.advanceY;
  }
}

float TTFFont::measureTextWidth(const std::u32string& text32,
                                const ShapeParams& params) {
  if (!m_hbFont || text32.empty()) return 0.f;
  const auto shaped = TextShaper::shape(text32, m_hbFont, params);
  float w = 0.f;
  for (const auto& sg : shaped) w += sg.advanceX;
  return w;
}
