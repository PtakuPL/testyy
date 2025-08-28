#pragma once
#include <string>
#include <vector>
#include <memory>

// HarfBuzz / FriBidi
#include <hb.h>
#include <hb-ft.h>
#include <fribidi.h>

// Minimalny zestaw danych wyjściowych po shapingu
struct ShapedGlyph {
  unsigned int glyphIndex; // indeks glifu w FT/HB
  float x;                 // pozycja x (piksele)
  float y;                 // pozycja y (piksele, baseline = 0)
  float advanceX;          // przesunięcie po glifie (x)
  float advanceY;          // zwykle 0 dla poziomego skryptu
};

enum class TextDirection { LTR, RTL, AUTO };

struct ShapeParams {
  std::string language;     // "pl", "ru", "el", "ar", "zh", ...
  std::string script;       // "Latn", "Cyrl", "Grek", "Arab", "Hani", ...
  TextDirection direction;  // LTR / RTL / AUTO
};

class TextShaper {
public:
  // hbFont to hb_font_t* powiązany z FT_Face (tworzony w TTFFont)
  static std::vector<ShapedGlyph> shape(const std::u32string& text32,
                                        hb_font_t* hbFont,
                                        const ShapeParams& params);
};