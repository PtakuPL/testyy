#include "TextShaper.h"
#include <stdexcept>

static hb_script_t toHbScript(const std::string& s) {
  if (s == "Cyrl") return HB_SCRIPT_CYRILLIC;
  if (s == "Grek") return HB_SCRIPT_GREEK;
  if (s == "Arab") return HB_SCRIPT_ARABIC;
  if (s == "Hani") return HB_SCRIPT_HAN;
  return HB_SCRIPT_LATIN;
}

static hb_direction_t toHbDir(TextDirection d) {
  switch (d) {
    case TextDirection::RTL: return HB_DIRECTION_RTL;
    case TextDirection::LTR: return HB_DIRECTION_LTR;
    default: return HB_DIRECTION_LTR; // AUTO -> heurystyka możliwa później
  }
}

std::vector<ShapedGlyph> TextShaper::shape(const std::u32string& text32,
                                           hb_font_t* hbFont,
                                           const ShapeParams& params) {
  std::vector<ShapedGlyph> out;
  if (!hbFont) return out;

  hb_buffer_t* buf = hb_buffer_create();

  // Konwersja UTF-32 → UTF-8 (HB wymaga kodowania, ale buf ma też add_codepoints)
  std::vector<uint32_t> codepoints(text32.begin(), text32.end());
  hb_buffer_add_codepoints(buf, codepoints.data(), (int)codepoints.size(), 0, (int)codepoints.size());

  hb_buffer_set_script(buf, toHbScript(params.script));
  hb_buffer_set_direction(buf, toHbDir(params.direction));
  hb_buffer_set_language(buf, hb_language_from_string(params.language.c_str(), -1));

  hb_shape(hbFont, buf, nullptr, 0);

  unsigned int glyphCount = 0;
  hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &glyphCount);
  hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &glyphCount);

  out.reserve(glyphCount);
  float x = 0.0f, y = 0.0f;

  for (unsigned int i = 0; i < glyphCount; ++i) {
    ShapedGlyph g;
    g.glyphIndex = info[i].codepoint;
    g.x = x + (pos[i].x_offset / 64.0f);
    g.y = y - (pos[i].y_offset / 64.0f);
    g.advanceX = pos[i].x_advance / 64.0f;
    g.advanceY = pos[i].y_advance / 64.0f;
    x += g.advanceX;
    y += g.advanceY;
    out.push_back(g);
  }

  hb_buffer_destroy(buf);
  return out;
}