#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

#include "TextShaper.h"

// OTClient framework graphics
#include <framework/graphics/declarations.h>  // ImagePtr, TexturePtr
#include <framework/graphics/image.h>
#include <framework/graphics/texture.h>

// Minimal glyph metadata stored in an atlas
struct AtlasGlyph {
  TexturePtr texture;   // texture handle of the atlas this glyph lives in
  int x, y;             // position inside the atlas
  int w, h;             // glyph bitmap size
  int bearingX;         // left bearing (pixels)
  int bearingY;         // top bearing  (pixels from baseline)
  int advance;          // advance from FreeType (26.6 fixed) – HB advance is used at draw time
};

class TTFFont {
public:
  TTFFont();
  ~TTFFont();

  // Load the main TTF and optional fallbacks; pixelSize is the font height in pixels
  bool load(const std::string& mainTtf,
            const std::vector<std::string>& fallbackTtfs,
            int pixelSize);

  // Draw shaped text at baseline position (x,y)
  void drawText(const std::u32string& text32,
                float x, float y,
                const ShapeParams& params);

  // Measure width of a string (uses HarfBuzz shaping)
  float measureTextWidth(const std::u32string& text32,
                         const ShapeParams& params);

  hb_font_t* hbFont() const { return m_hbFont; }

private:
  // Ensure glyph is present in atlas, rasterizing and packing when needed
  const AtlasGlyph* cacheGlyph(uint32_t glyphIndex);

  // Create a new empty atlas and return its index
  int ensureAtlas();

  // FreeType & HarfBuzz state
  FT_Library m_ftLib = nullptr;
  FT_Face    m_face  = nullptr;
  hb_font_t* m_hbFont = nullptr;

  // (Reserved for future) fallback faces and hb_font objects
  std::vector<FT_Face>     m_fallbackFaces;
  std::vector<hb_font_t*>  m_fallbackHbFonts;

  int m_pixelSize = 12;

  struct Atlas {
    TexturePtr texture;   // GPU texture
    ImagePtr   image;     // CPU-side backing store (RGBA)
    int width, height;
    int penX, penY, rowH; // simple row-based packer
  };
  std::vector<Atlas> m_atlases;
  std::unordered_map<uint32_t, AtlasGlyph> m_glyphs;
};
