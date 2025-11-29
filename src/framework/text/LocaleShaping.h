#pragma once
#include <string>
#include <string_view>
#include <cstdint>

#include "TTFFont.h" // ShapeParams, TextDirection

namespace otc::text {

struct LocaleInfo {
  std::string   language;              // "pl"
  std::string   script;                // "Latn", "Cyrl", "Arab", "Hebr", "Hans", "Hant", "Jpan", "Kore", "Grek", "Geor", "Deva", "Thai", "Ethi", ...
  std::string   region;                // "PL", "US", "TW", ...
  TextDirection direction{ TextDirection::Auto };
};

class LocaleShaping {
public:
  // Parsowanie BCP-47: "sr-Latn-RS", "zh-Hant-TW", "pl-PL", "ar"
  static LocaleInfo fromBCP47(std::string_view tag);

  // Heurystyczne wykrycie po treści (UTF-8): sprawdza m.in. zakresy Arab/Hebr/CJK
  static LocaleInfo probeUtf8(std::string_view utf8, const std::string& fallbackLanguage = {});

  // Zastosuj LocaleInfo do ShapeParams (uzupełnij tylko brakujące pola, chyba że overrideExplicit = true)
  static void applyToShapeParams(const LocaleInfo& loc, ShapeParams& params, bool overrideExplicit = false);

  // Kierunek sugerowany przez skrypt (RTL dla Arab/Hebr/Syrc/Thaa/Nkoo/Adlm)
  static TextDirection directionForScript(const std::string& iso15924);

  // NOWOŚĆ: mapowanie polskich nazw na kanoniczne tagi BCP-47 (np. "Brazylijski" -> "pt-BR").
  // Zwraca true jeśli rozpoznano; outTag otrzymuje np. "es-MX", "ar-EG", "ko-KR".
  static bool canonicalBCP47ForDisplayName(std::string_view polishName, std::string& outTag);

private:
  static std::string toLowerASCII(std::string_view s);
  static std::string toUpperASCII(std::string_view s);
  static std::string toTitleASCII(std::string_view s);
  static std::string normalizeKey(std::string_view s); // lower + trim + collapse spaces

  static bool isAlphaASCII(char c);
  static bool isDigitASCII(char c);
  static bool isScriptSubtag(std::string_view t);  // 4 litery
  static bool isRegionSubtag(std::string_view t);  // 2 litery lub 3 cyfry

  // Domyślne mapowania language -> script (gdy skrypt nie podany w tagu)
  static std::string defaultScriptForLanguage(const std::string& lang, const std::string& region);

  // Minimalne rozpoznawanie klas znaków dla UTF-8 (Arab/Hebr/CJK)
  static bool isStrongRTL(uint32_t cp);    // Hebr/Arab/… (kierunek)
  static bool isCJK(uint32_t cp);          // CJK/Hiragana/Katakana/Hangul
  static uint32_t nextCodepoint(const char*& p, const char* end); // dekoder UTF-8 (bez wyjątków)
};

} // namespace otc::text