# I18N Next Steps — Kompletna lista zadań internacjonalizacji

Data aktualizacji: 2025-11-30

---

## Status projektu

Projekt OTClient Redemption — wdrożenie pełnej obsługi Unicode, TTF, HarfBuzz (shaping) i FriBidi (bidi/RTL) dla wszystkich języków świata.

---

## I. Rdzeń tekstu i kształtowanie (HarfBuzz/FreeType)

### A) Pliki do edycji

| Plik | Status | Opis zmian |
|------|--------|------------|
| `src/framework/text/TTFFont.cpp` | ✅ Szkielet gotowy | PIMPL, shaping HB, metryki FT, fallback chain; wymaga: real upload glifów do atlasów GPU |
| `src/framework/text/TTFFont.h` | ✅ Szkielet gotowy | Lekki nagłówek bez HB/FT; typy: `ShapeParams`, `FontMetrics`, `ShapedGlyph`, `Quad` |
| `src/framework/text/TextShaper.cpp` | ✅ Gotowy | HarfBuzz shaping UTF-32, heurystyki language/script/direction |
| `src/framework/text/TextShaper.h` | ✅ Gotowy | Deklaracje `ShapedGlyph`, `ShapeParams`, `TextShaper::shape` |
| `src/framework/text/LocaleShaping.cpp` | ✅ Gotowy | Parser BCP-47, mapping locale→script/direction |
| `src/framework/text/LocaleShaping.h` | ✅ Gotowy | API: `fromBCP47()`, `probeUtf8()`, `directionForScript()` |

### B) Zadania do wykonania

- [ ] **TTFFont.cpp — Upload glifów do atlasów GPU**
  - Implementacja `ensureAtlasTexture()` i `uploadSubImage()` 
  - Integracja z `TextureManager` / `Texture`
  - Getter `getAtlasTexture(int atlasId)` dla DrawPool

- [ ] **TTFFont.cpp — Rejestracja backendu tekstur**
  - Zarejestrować hooki `gCreateAtlasTexture` i `gUploadAtlasSubImage` po inicjalizacji grafiki
  - Wywołanie `TTFFont_SetTextureBackend(createFn, uploadFn)`

---

## II. Manager czcionek i zasoby (.otfont)

### A) Pliki do edycji

| Plik | Status | Opis zmian |
|------|--------|------------|
| `src/framework/graphics/fontmanager.cpp` | 🔶 Do modyfikacji | Dodać ładowanie `type:"ttf"`, fallback chain, `getTTF()` |
| `src/framework/graphics/fontmanager.h` | 🔶 Do modyfikacji | Forward-declare `TTFFont`, nowe pola `m_ttfFonts`, metody `getTTF()` |
| `src/framework/otml/otmlloader.cpp` | 🔶 Do modyfikacji | Walidacja pól .otfont dla TTF: `file`, `size`, `fallback` |

### B) Zadania do wykonania

- [ ] **FontManager — Obsługa type:"ttf"**
  - Odczyt pola `type` (case-insensitive)
  - Dla `ttf`: utworzenie `TTFFontPtr` zamiast `BitmapFontPtr`
  - Ładowanie pliku TTF (`file` + rozszerzenia: .ttf, .otf, .ttc)
  - Obsługa `size`, `dpi`, `dpiX`/`dpiY`
  - Obsługa `fallback` (lista dodatkowych fontów)

- [ ] **FontManager — Nowa mapa i gettery**
  - `std::unordered_map<std::string, TTFFontPtr> m_ttfFonts`
  - `TTFFontPtr m_defaultTtfFont`
  - Metody: `getTTF(name)`, `fontExists(name)`, `getDefaultTTF()`

- [ ] **OTML — Walidacja .otfont**
  - Sprawdzenie wymaganych pól dla `type:"ttf"`
  - Logowanie błędów brakujących pól

---

## III. Warstwa graficzna i cache tekstu

### A) Pliki do edycji

| Plik | Status | Opis zmian |
|------|--------|------------|
| `src/framework/graphics/bitmapfont.cpp` | 🔶 Do modyfikacji | Gałąź TTF: delegacja do `TTFFont`, pomiar i rysowanie |
| `src/framework/graphics/bitmapfont.h` | 🔶 Do modyfikacji | Pole `m_ttf`, flaga `m_isTTF`, metoda `isTTF()` |
| `src/framework/graphics/cachedtext.cpp` | 🔶 Do modyfikacji | Cache `shaped runs` dla TTF |
| `src/framework/graphics/cachedtext.h` | 🔶 Do modyfikacji | Wektor `ShapedGlyph` zamiast bajtów |
| `src/framework/graphics/drawpooltext.cpp` | 🔶 Do modyfikacji | Batchowanie quadów per atlas, rysowanie TTF |
| `src/framework/graphics/graphics.cpp` | 🔶 Drobne zmiany | DPI/baseline alignment |

### B) Zadania do wykonania

- [ ] **BitmapFont — Mostek TTF (Batch A)**
  - Dodanie `std::shared_ptr<TTFFont> m_ttf` i `bool m_isTTF`
  - W `load()`: rozpoznanie `type:"ttf"` → inicjalizacja `m_ttf`
  - W `calculateTextRectSize()`: delegacja do `m_ttf->measureTextWidth()` + `metrics().lineHeight()`
  - W `drawText()`: delegacja do `m_ttf->shape()` + `buildQuads()`

- [ ] **CachedText — Cache shaped runs (Batch A)**
  - Bufor `std::vector<ShapedGlyph>` gdy `m_isTTF`
  - Aktualizacja przy zmianie tekstu

- [ ] **DrawPoolText — Batchowanie per atlas (Batch C)**
  - Grupowanie quadów według `atlasId`
  - Wywołanie `g_drawPool->addTexturedVerts()` per atlas
  - Użycie `getAtlasTexture(atlasId)` z TTFFont

---

## IV. Warstwa UI (krytyczna migracja)

### A) Pliki do edycji

| Plik | Status | Opis zmian |
|------|--------|------------|
| `src/framework/ui/uiwidget.cpp` | 🔶 Do modyfikacji | Użycie abstrakcji fontu zamiast BitmapFont |
| `src/framework/ui/uiwidget.h` | 🔶 Do modyfikacji | Typ pola fontu |
| `src/framework/ui/uiwidgettext.cpp` | 🔶 Do modyfikacji | `setFont()` → łańcuch fallbacków TTF |
| `src/framework/ui/uitextedit.cpp` | ✅ Podstawowe zmiany | Nawigacja UTF-8, kasowanie kodopunktów |
| `src/framework/ui/uitextedit.h` | 🔶 Drobne zmiany | Ewentualne nowe pola |
| `src/framework/gui/uifont.cpp` | 🔶 Do modyfikacji | Lookup przez `g_fonts` z obsługą TTF |

### B) Zadania do wykonania

- [ ] **UIWidget — Abstrahowanie fontu (Batch B)**
  - Nie zakładać atlasu bitmap w rysowaniu
  - Używać `font->calculateTextRectSize()` zamiast własnych obliczeń

- [ ] **UIWidgetText — Obsługa TTF (Batch B)**
  - `setFont()` pobiera font przez `g_fonts` (TTF lub bitmap)
  - Wywołanie `drawText()` delegujące do odpowiedniej implementacji

- [ ] **UITextEdit — Pełna nawigacja po klastrach**
  - ✅ Podstawowe: `utf8Prev`/`utf8Next`, kasowanie kodopunktów
  - [ ] Zaawansowane: użycie `ShapedGlyph.cluster` dla kursora/selekcji
  - [ ] Obsługa RTL w nawigacji

- [ ] **UIFont — Integracja z FontManager**
  - Pobieranie fontu: najpierw TTF, potem bitmap
  - Obsługa fallbacków

---

## V. Konfiguracja klienta i teksty w świecie

### A) Pliki do edycji

| Plik | Status | Opis zmian |
|------|--------|------------|
| `src/client/gameconfig.cpp` | 🔶 Do modyfikacji | Zmiana typów fontów globalnych na TTF/IFont |
| `src/client/gameconfig.h` | 🔶 Do modyfikacji | Typy fontów: CreatureName, Animated, Static, Widget |
| `src/client/statictext.cpp` | 🔶 Do modyfikacji | Dymki/nadpisy → TTFFont |
| `src/client/statictext.h` | 🔶 Drobne zmiany | Typ fontu |

### B) Zadania do wykonania

- [ ] **GameConfig — Typy fontów**
  - Zmiana z `BitmapFontPtr` na abstrakcję wspierającą TTF
  - Fonty: `CreatureNameFont`, `AnimatedTextFont`, `StaticTextFont`, `WidgetFont`

- [ ] **StaticText — Obsługa TTF**
  - Pomiar przez `TTFFont::measureTextWidth()`
  - Rysowanie przez `shape()` + `buildQuads()`

---

## VI. Stabilizacja buildu (niezbędne poprawki)

### A) Pliki do naprawy

| Plik | Status | Problem | Rozwiązanie |
|------|--------|---------|-------------|
| `src/framework/net/protocol.cpp` | ❌ Błąd | `%i` w loggerze fmt | Zamiana na `{}` |
| `src/framework/graphics/bitmapfont.cpp` | ❌ Błąd | `%s` w "TTF load failed" | Zamiana na `{}` |
| `src/framework/otml/otmlnode.cpp` | ⚠️ ICE | MSVC crash na fmt::format | Konkatenacja std::string |

### B) Zadania do wykonania

- [ ] **protocol.cpp** — Zamiana `%i` → `{}`
  ```cpp
  // Było:
  g_logger.traceError("invalid size of decompressed message - %i", totalSize);
  // Ma być:
  g_logger.traceError("invalid size of decompressed message - {}", totalSize);
  ```

- [ ] **bitmapfont.cpp** — Zamiana `%s` → `{}`
  ```cpp
  // Było:
  g_logger.error(stdext::format("TTF load failed: %s", src));
  // Ma być:
  g_logger.error("TTF load failed: {}", src);
  ```

- [ ] **otmlnode.cpp** — Unikanie fmt przy ICE
  - Użycie prostej konkatenacji `std::string` zamiast `fmt::format`

---

## VII. Zasoby i konfiguracja

### A) Pliki fontów TTF

| Plik | Status | Przeznaczenie |
|------|--------|---------------|
| `data/fonts/ttf/NotoSans-Regular.ttf` | 📁 Do dodania | Główny font (Laciński, Greka, Cyrylica) |
| `data/fonts/ttf/NotoSans-Bold.ttf` | 📁 Do dodania | Wariant pogrubiony |
| `data/fonts/ttf/NotoSansSC-Regular.ttf` | 📁 Do dodania | CJK (chiński uproszczony) |
| `data/fonts/ttf/NotoNaskhArabic-Regular.ttf` | 📁 Do dodania | Arabski (RTL) |
| `data/fonts/ttf/NotoSansMono-Regular.ttf` | 📁 Do dodania | Monospace (konsola, logi) |

### B) Pliki .otfont

Przykładowy plik `data/fonts/noto-12.otfont`:
```yaml
Font
  name: "noto-12"
  type: "ttf"
  file: "fonts/ttf/NotoSans-Regular.ttf"
  size: 12
  dpi: 96
  fallback: [
    "fonts/ttf/NotoSansSC-Regular.ttf",
    "fonts/ttf/NotoNaskhArabic-Regular.ttf"
  ]
  default: true
  widget-default: true
```

### C) Tłumaczenia

| Plik | Status | Języki |
|------|--------|--------|
| `data/locales/pl.lua` | ✅ Częściowo gotowy | Polski |
| `data/locales/de.lua` | 🔶 Do uzupełnienia | Niemiecki |
| `data/locales/ru.lua` | 📁 Do utworzenia | Rosyjski |
| `data/locales/ar.lua` | 📁 Do utworzenia | Arabski |
| `data/locales/zh.lua` | 📁 Do utworzenia | Chiński |

---

## VIII. Plan wdrożenia (Batches)

### Batch A — Mostki kompilacyjne (priorytet 1)

Cel: Build przechodzi, BitmapFont deleguje do TTFFont

1. [ ] `bitmapfont.h/.cpp` — gałąź TTF
2. [ ] `cachedtext.h/.cpp` — cache shaped runs
3. [ ] Naprawki formatowania (`protocol.cpp`, `bitmapfont.cpp`)

### Batch B — Integracja UI (priorytet 2)

Cel: UI używa TTF bez crashy

1. [ ] `uifont.cpp` — lookup TTF
2. [ ] `uiwidget.h/.cpp`, `uiwidgettext.cpp` — abstrakcja fontu
3. [ ] `uitextedit.h/.cpp` — nawigacja po klastrach

### Batch C — Realny render (priorytet 3)

Cel: Glify TTF widoczne na ekranie

1. [ ] `TTFFont.cpp` — upload glifów do atlasów GPU
2. [ ] `drawpooltext.cpp` — batchowanie per atlas
3. [ ] Rejestracja backendu tekstur

### Batch D — Pełne RTL/Bidi (opcjonalny)

Cel: Pełna obsługa języków RTL

1. [ ] Integracja FriBidi na wejściu do `shape()`
2. [ ] Precyzyjny caret/selekcja dla tekstu mieszanego
3. [ ] Poprawna kolejność wyświetlania RTL/LTR

---

## IX. Testy weryfikacyjne

### A) Testy kompilacji

- [ ] Build MSVC (OpenGL|x64) przechodzi bez błędów
- [ ] Brak ostrzeżeń ICE (C1001)
- [ ] Brak błędów formatowania (`%s`, `%i` → `{}`)

### B) Testy runtime

- [ ] Łacinka + diakrytyki: `Zażółć gęślą jaźń — ąćęłńóśźż`
- [ ] Cyrylica: `Привет мир`
- [ ] Greka: `Ελληνικά`
- [ ] Arabski RTL: `مرحبا بالعالم`
- [ ] CJK: `你好，世界 / こんにちは世界 / 안녕하세요`
- [ ] Fallback: znak tylko w fallback foncie
- [ ] Wrap: łamanie tekstu + klip w widgetach
- [ ] Wzrost atlasu: długi tekst wymuszający nowy atlas

---

## X. Referencje

- `WszystkieSRCLOG.md` — szczegółowa analiza plików źródłowych
- `plan.md` — chronologiczny dziennik prac
- `worklog_all.md` — logi sesji i konfiguracji serwera
- `wykonane_zadania.md` — zakończone zadania
- `docs/linux-build-deps.md` — zależności budowania na Linux

---

## Legenda

| Symbol | Znaczenie |
|--------|-----------|
| ✅ | Gotowe / Zrobione |
| 🔶 | Do modyfikacji / W trakcie |
| ❌ | Błąd wymagający naprawy |
| ⚠️ | Ostrzeżenie / Problem |
| 📁 | Do utworzenia/dodania |
| [ ] | Zadanie do wykonania |
| [x] | Zadanie wykonane |
