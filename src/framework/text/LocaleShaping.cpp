#include "LocaleShaping.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace otc::text {

// --- utils ASCII -------------------------------------------------------------

bool LocaleShaping::isAlphaASCII(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
bool LocaleShaping::isDigitASCII(char c) { return (c >= '0' && c <= '9'); }

std::string LocaleShaping::toLowerASCII(std::string_view s) {
  std::string r; r.reserve(s.size());
  for (char c : s) r.push_back((c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c);
  return r;
}
std::string LocaleShaping::toUpperASCII(std::string_view s) {
  std::string r; r.reserve(s.size());
  for (char c : s) r.push_back((c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c);
  return r;
}
std::string LocaleShaping::toTitleASCII(std::string_view s) {
  std::string r; r.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (i == 0) r.push_back((c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c);
    else        r.push_back((c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c);
  }
  return r;
}

static inline bool isSpace(char c) { return c==' ' || c=='\t' || c=='\n' || c=='\r'; }
std::string LocaleShaping::normalizeKey(std::string_view s) {
  // lower + trim + collapse spaces (nie usuwamy polskich znaków)
  std::string t = toLowerASCII(s);
  // trim
  size_t i=0, j=t.size();
  while (i<j && isSpace(t[i])) ++i;
  while (j>i && isSpace(t[j-1])) --j;
  std::string o; o.reserve(j-i);
  bool prevSpace=false;
  for (size_t k=i; k<j; ++k) {
    char c=t[k];
    if (isSpace(c)) {
      if (!prevSpace) { o.push_back(' '); prevSpace=true; }
    } else { o.push_back(c); prevSpace=false; }
  }
  return o;
}

bool LocaleShaping::isScriptSubtag(std::string_view t) {
  if (t.size() != 4) return false;
  for (char c : t) if (!isAlphaASCII(c)) return false;
  return true;
}
bool LocaleShaping::isRegionSubtag(std::string_view t) {
  if (t.size() == 2) {
    return isAlphaASCII(t[0]) && isAlphaASCII(t[1]);
  }
  if (t.size() == 3) {
    return isDigitASCII(t[0]) && isDigitASCII(t[1]) && isDigitASCII(t[2]);
  }
  return false;
}

// --- kierunek dla skryptu ----------------------------------------------------

TextDirection LocaleShaping::directionForScript(const std::string& s) {
  // Najczęstsze skrypty RTL:
  if (s == "Arab" || s == "Hebr" || s == "Syrc" || s == "Thaa" || s == "Nkoo" || s == "Adlm")
    return TextDirection::RTL;
  return TextDirection::LTR;
}

// --- mapowania domyślne lang->script ----------------------------------------

std::string LocaleShaping::defaultScriptForLanguage(const std::string& lang, const std::string& region) {
  // Bazowe łacińskie
  static const char* kLatn[] = {
    "en","pl","de","fr","es","pt","it","nl","sv","no","nb","nn","da","fi","tr","cs","sk","hu","ro","hr","sl","sq","ga","mt","vi","id","ms","lt","elx","az","uz","tk","fil","tl","yo","ha","zu","sw","af"
  };
  for (auto* l : kLatn) if (lang == l) return "Latn";

  if (lang == "ru" || lang == "uk" || lang == "bg" || lang == "kk" /* Kazachstan obecnie powsz. Cyrl */)
    return "Cyrl";

  if (lang == "sr") {
    // Serbski bywa Cyrl/Latn; domyślnie Cyrl (wybór projektu)
    return "Cyrl";
  }

  if (lang == "el") return "Grek";
  if (lang == "he") return "Hebr";
  if (lang == "ar" || lang == "fa" || lang == "ur") return "Arab";   // ar/farsi/urdu
  if (lang == "ka") return "Geor";   // gruziński
  if (lang == "hi") return "Deva";   // hindi
  if (lang == "th") return "Thai";   // tajski
  if (lang == "am") return "Ethi";   // amharski (Etiopia)

  if (lang == "zh") {
    const std::string R = toUpperASCII(region);
    if (R == "TW" || R == "HK" || R == "MO") return "Hant";
    return "Hans";
  }
  if (lang == "ja") return "Jpan";
  if (lang == "ko") return "Kore";

  // fallback
  return "Latn";
}

// --- dekoder UTF-8 + proste klasy znaków ------------------------------------

uint32_t LocaleShaping::nextCodepoint(const char*& p, const char* end) {
  if (p >= end) return 0;
  unsigned char c = (unsigned char)*p++;
  if (c < 0x80) return c;
  uint32_t cp = 0;
  int extra = 0;
  if ((c >> 5) == 0x6) { cp = (c & 0x1F); extra = 1; }
  else if ((c >> 4) == 0xE) { cp = (c & 0x0F); extra = 2; }
  else if ((c >> 3) == 0x1E) { cp = (c & 0x07); extra = 3; }
  else return 0xFFFD;
  while (extra-- > 0) {
    if (p >= end) return 0xFFFD;
    unsigned char t = (unsigned char)*p++;
    if ((t >> 6) != 0x2) return 0xFFFD;
    cp = (cp << 6) | (t & 0x3F);
  }
  return cp;
}

bool LocaleShaping::isStrongRTL(uint32_t cp) {
  // Hebrajski
  if (cp >= 0x0590 && cp <= 0x05FF) return true;
  // Arabski + suplementy + prezentacyjne
  if ((cp >= 0x0600 && cp <= 0x06FF) ||
      (cp >= 0x0750 && cp <= 0x077F) ||
      (cp >= 0x08A0 && cp <= 0x08FF) ||
      (cp >= 0xFB50 && cp <= 0xFDFF) ||
      (cp >= 0xFE70 && cp <= 0xFEFF))
    return true;
  // Syryjski
  if (cp >= 0x0700 && cp <= 0x074F) return true;
  // Thaana
  if (cp >= 0x0780 && cp <= 0x07BF) return true;
  // N'Ko
  if (cp >= 0x07C0 && cp <= 0x07FF) return true;
  // Adlam
  if (cp >= 0x1E900 && cp <= 0x1E95F) return true;
  return false;
}

bool LocaleShaping::isCJK(uint32_t cp) {
  if ((cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF)) return true; // Han + rozszerzenia
  if ((cp >= 0x3040 && cp <= 0x309F) || (cp >= 0x30A0 && cp <= 0x30FF)) return true; // Hiragana/Katakana
  if (cp >= 0xAC00 && cp <= 0xD7AF) return true; // Hangul
  return false;
}

// --- API: BCP-47 -------------------------------------------------------------

LocaleInfo LocaleShaping::fromBCP47(std::string_view tag) {
  LocaleInfo out;
  if (tag.empty()) {
    out.language = "en"; out.script = "Latn"; out.region.clear();
    out.direction = TextDirection::LTR;
    return out;
  }

  // split
  std::vector<std::string_view> parts;
  {
    size_t start = 0;
    while (start < tag.size()) {
      size_t pos = tag.find('-', start);
      if (pos == std::string_view::npos) { parts.emplace_back(tag.substr(start)); break; }
      parts.emplace_back(tag.substr(start, pos - start));
      start = pos + 1;
    }
  }

  // language
  if (!parts.empty()) out.language = toLowerASCII(parts[0]); else out.language = "en";

  // script/region
  for (size_t i = 1; i < parts.size(); ++i) {
    auto p = parts[i];
    if (isScriptSubtag(p) && out.script.empty()) {
      out.script = toTitleASCII(p);
    } else if (isRegionSubtag(p) && out.region.empty()) {
      out.region = toUpperASCII(p);
    }
  }

  if (out.script.empty()) {
    out.script = defaultScriptForLanguage(out.language, out.region);
  }

  out.direction = directionForScript(out.script);
  return out;
}

LocaleInfo LocaleShaping::probeUtf8(std::string_view utf8, const std::string& fallbackLanguage) {
  LocaleInfo out;
  out.language = fallbackLanguage.empty() ? "en" : toLowerASCII(fallbackLanguage);
  out.script.clear(); // wykryjemy poniżej

  const char* p = utf8.data();
  const char* e = utf8.data() + utf8.size();

  bool seenRTL = false, seenCJK = false;
  int  checked = 0;

  while (p < e && checked < 512) {
    uint32_t cp = nextCodepoint(p, e);
    if (cp == 0) break;
    ++checked;

    if (!seenRTL && isStrongRTL(cp)) seenRTL = true;
    if (!seenCJK && isCJK(cp)) seenCJK = true;

    if (seenRTL && seenCJK) break;
  }

  if (seenRTL) {
    out.script = "Arab";
    if (out.language == "en") out.language = "ar";
  } else if (seenCJK) {
    out.language = (out.language == "en") ? "zh" : out.language;
    out.script   = "Hans";
  } else {
    out.script = defaultScriptForLanguage(out.language, "");
  }

  out.direction = directionForScript(out.script);
  return out;
}

void LocaleShaping::applyToShapeParams(const LocaleInfo& loc, ShapeParams& params, bool overrideExplicit) {
  auto needSet = [overrideExplicit](bool emptyOrAuto) {
    return overrideExplicit || emptyOrAuto;
  };

  if (needSet(params.language.empty())) {
    params.language = loc.language.empty() ? "en" : loc.language;
  }
  if (needSet(params.script.empty())) {
    params.script = loc.script.empty() ? defaultScriptForLanguage(params.language, "") : loc.script;
  }
  if (needSet(params.direction == TextDirection::Auto)) {
    params.direction = loc.direction == TextDirection::Auto ? directionForScript(params.script) : loc.direction;
  }
}

// --- API: display-name → BCP-47 ---------------------------------------------

bool LocaleShaping::canonicalBCP47ForDisplayName(std::string_view polishName, std::string& outTag) {
  static const std::unordered_map<std::string, std::string> kMap = {
    // Europa (+regionalne warianty)
    {"polski", "pl-PL"},
    {"hiszpański", "es-ES"},
    {"meksykański", "es-MX"},
    {"kolumbijski", "es-CO"},
    {"argentyński", "es-AR"},
    {"portugalski", "pt-PT"},
    {"brazylijski", "pt-BR"},
    {"francuski", "fr-FR"},
    {"niemiecki", "de-DE"},
    {"włoski", "it-IT"},
    {"rosyjski", "ru-RU"},
    {"ukraiński", "uk-UA"},
    {"grecki", "el-GR"},
    {"rumuński", "ro-RO"},
    {"węgierski", "hu-HU"},
    {"słowacki", "sk-SK"},
    {"czeski", "cs-CZ"},
    {"szwedzki", "sv-SE"}, {"szwecki", "sv-SE"},
    {"norweski", "nb-NO"},
    {"litewski", "lt-LT"}, {"litfinski", "lt-LT"},
    {"holenderski", "nl-NL"},
    {"turecki", "tr-TR"},
    {"bułgarski", "bg-BG"},
    {"serbski", "sr-Cyrl-RS"}, // świadomie Cyrl; można podmienić na sr-Latn-RS

    // Bliski Wschód / Afryka Półn.
    {"egipski", "ar-EG"},
    {"arabia saudyjska", "ar-SA"},
    {"izrael", "he-IL"},
    {"maroko", "ar-MA"},
    {"tunezja", "ar-TN"},

    // Kaukaz / Azja Centralna
    {"gruzja", "ka-GE"},
    {"kazachstan", "kk-KZ"},   // dziś powszechnie Cyrl
    {"azerbejdżan", "az-AZ"},  // Latn
    {"turkmenistan", "tk-TM"}, // Latn
    {"uzbekistan", "uz-UZ"},   // Latn

    // Azja
    {"chiński", "zh-CN"},     // domyślnie Hans; dla Tajwanu użyj zh-TW
    {"japoński", "ja-JP"},
    {"korea południowa", "ko-KR"},
    {"irański", "fa-IR"}, {"irański", "fa-IR"}, // obie formy
    {"indyjski", "hi-IN"},
    {"tajlandia", "th-TH"},
    {"filipiny", "fil-PH"},
    {"wietnam", "vi-VN"},
    {"pakistan", "ur-PK"},

    // Afryka / in. anglojęzyczne
    {"nigeria", "en-NG"}, // UI: angielski jako najbezpieczniejszy default
    {"etiopski", "am-ET"},

    // Azja Południowo-Wschodnia
    {"malezyjski", "ms-MY"},
    {"indonezyjski", "id-ID"},
  };

  const std::string key = normalizeKey(polishName);
  auto it = kMap.find(key);
  if (it != kMap.end()) { outTag = it->second; return true; }

  // Synonimy/aliasy najczęstszych wariantów (np. „brazylijski portugalski” itp.)
  static const std::unordered_map<std::string, std::string> kAliases = {
    {"portugalski brazylijski", "pt-BR"},
    {"portugalski (brazylia)", "pt-BR"},
    {"hiszpański meksykański", "es-MX"},
    {"hiszpański (meksyk)", "es-MX"},
    {"hiszpański kolumbijski", "es-CO"},
    {"hiszpański argentyński", "es-AR"},
    {"język chiński", "zh-CN"},
    {"język japoński", "ja-JP"},
    {"język koreański", "ko-KR"},
  };
  auto it2 = kAliases.find(key);
  if (it2 != kAliases.end()) { outTag = it2->second; return true; }

  return false;
}

} // namespace otc::text