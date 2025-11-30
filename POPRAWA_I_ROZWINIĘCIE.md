# OTClient - Lista Rzeczy do Poprawy i Rozwinięcia

> **Data analizy**: 2025-11-30  
> **Autor**: Automatyczna analiza kodu  
> **Repozytorium**: PtakuPL/testyy (OTClient Redemption)

---

## 📋 Spis Treści

1. [Krytyczne Błędy Kompilacji](#1-krytyczne-błędy-kompilacji)
2. [Warstwa Tekstu i Czcionek (TTF/i18n)](#2-warstwa-tekstu-i-czcionek-ttfi18n)
3. [Interfejs Użytkownika (UI)](#3-interfejs-użytkownika-ui)
4. [Sieć i Protokół](#4-sieć-i-protokół)
5. [System Budowania (Build System)](#5-system-budowania-build-system)
6. [Lokalizacja i Tłumaczenia](#6-lokalizacja-i-tłumaczenia)
7. [Dokumentacja](#7-dokumentacja)
8. [Optymalizacje Wydajności](#8-optymalizacje-wydajności)
9. [Bezpieczeństwo](#9-bezpieczeństwo)
10. [Nowe Funkcjonalności (Propozycje)](#10-nowe-funkcjonalności-propozycje)

---

## 1. Krytyczne Błędy Kompilacji

### 1.1 Błędne Formatowanie Logów (printf-style zamiast fmt-style)

**Status:** ✅ NAPRAWIONE

**Pliki naprawione:**

| Plik | Linia | Problem | Status |
|------|-------|---------|--------|
| `src/framework/net/protocol.cpp` | ~275 | `%i` → `{}` | ✅ Naprawione |
| `src/client/map.cpp` | ~1455 | `%i` → `{}` | ✅ Naprawione |

**Priorytet:** 🟢 ROZWIĄZANE

---

### 1.2 Błędy Szablonów C++ (BS::thread_pool)

**Status:** ✅ NAPRAWIONE

**Pliki naprawione:**

| Plik | Problem | Status |
|------|---------|--------|
| `src/framework/core/asyncdispatcher.h` | `BS::thread_pool` → `BS::thread_pool<>` | ✅ Naprawione |
| `src/framework/core/asyncdispatcher.cpp` | `BS::thread_pool` → `BS::thread_pool<>` | ✅ Naprawione |
| `src/framework/core/eventdispatcher.h` | Podwójny średnik `;;` | ✅ Naprawione |
| `src/framework/platform/platformwindow.h` | Unused parameter warning | ✅ Naprawione |

**Opis problemu:** Niezgodność typu szablonu między deklaracją extern i definicją powodowała błąd linkera.

**Priorytet:** 🟢 ROZWIĄZANE

---

### 1.3 Problemy z MSVC/Unity Build

**Znane problemy:**
- [ ] ICE (Internal Compiler Error) w `otmlnode.cpp` podczas LTCG
- [ ] Błąd C1128: przekroczenie limitu sekcji - wymaga `/bigobj`
- [ ] Crash CL.exe (MSB6006) przy dużych jednostkach kompilacji

**Rozwiązania:**
1. Dodać `/bigobj` do opcji kompilatora
2. Wyłączyć LTCG (`/LTCG:OFF`) lub Unity Build dla problematycznych plików
3. Rozważyć zmniejszenie `CMAKE_UNITY_BUILD_BATCH_SIZE`

---

### 1.4 Niespójności Nagłówków

- [ ] Sprawdzić wszystkie `#include` w kontekście Windows vs Linux
- [ ] Upewnić się, że X11 headers są opakowane w `#if !defined(_WIN32)`
- [ ] Weryfikacja zgodności sygnatur funkcji między `.h` i `.cpp`

---

## 2. Warstwa Tekstu i Czcionek (TTF/i18n)

### 2.1 Integracja TTFFont z Resztą Systemu

**Status:** 🟡 W TRAKCIE (częściowo zaimplementowane)

**Pliki wymagające zmian:**

| Plik | Status | Opis |
|------|--------|------|
| `framework/text/TTFFont.cpp` | 🟡 | Brak realnego uploadu glifów do tekstur GPU |
| `framework/text/TTFFont.h` | 🟡 | Dodać `getAtlasTexture()`, `atlasCount()` |
| `framework/text/TextShaper.cpp` | 🟢 | Implementacja HarfBuzz - OK |
| `framework/text/LocaleShaping.cpp` | 🟢 | Mapowanie locale→script - OK |
| `framework/graphics/bitmapfont.cpp` | 🟡 | Mostek do TTFFont potrzebuje dopracowania |
| `framework/graphics/fontmanager.cpp` | 🔴 | Wymaga pełnego wsparcia dla type:"ttf" |
| `framework/graphics/drawpooltext.cpp` | 🔴 | Brak batchowania per-atlas dla TTF |
| `framework/graphics/cachedtext.cpp` | 🔴 | Cache shaped runs zamiast bajtów |

### 2.2 Brakujące Elementy TTF

- [ ] **Upload bitmap glifów do atlasów GPU** - `ensureAtlasTexture()` i `uploadSubImage()` mają tylko TODO
- [ ] **Getter tekstur atlasu** - `TTFFont::getAtlasTexture(atlasId)` nieimplementowany
- [ ] **Grupowanie quadów per-atlas** - `buildAtlasBatches()` nie jest używana
- [ ] **Fallback czcionek** - łańcuch fallbacków tylko częściowo działa

### 2.3 Wsparcie RTL (Right-to-Left)

- [ ] Pełna integracja FriBidi dla tekstów arabskich/hebrajskich
- [ ] Poprawne wyświetlanie ligatur
- [ ] Algorytm dwukierunkowy (Bidi) dla mieszanych tekstów

### 2.4 Brakujące Czcionki Fallback

**Obecne w `data/fonts/ttf/`:**
- ✅ NotoSans-Regular.ttf
- ✅ NotoSans-Bold.ttf  
- ✅ NotoSansSC-Regular.ttf (CJK)
- ✅ NotoNaskhArabic-Regular.ttf

**Brakujące:**
- [ ] NotoSansMono (dla konsoli/logów)
- [ ] NotoSansHebrew
- [ ] NotoSansJP / NotoSansKR (japońskie/koreańskie)
- [ ] Emoji font (kolorowy)

---

## 3. Interfejs Użytkownika (UI)

### 3.1 UITextEdit - Wsparcie Unicode

**Plik:** `src/framework/ui/uitextedit.cpp`

**Status:** 🟡 CZĘŚCIOWO NAPRAWIONE

**Naprawione:**
- [x] Blokada TTF przez sprawdzanie `m_font->getTexture()` - dodano ścieżkę dla TTF

**Pozostałe problemy:**
- [ ] Iteracja po bajtach zamiast kodopunktów UTF-8
- [ ] Pozycja kursora błędnie obliczana dla znaków wielobajtowych
- [ ] Zaznaczanie tekstu nie działa poprawnie z RTL (TODO w kodzie)

### 3.2 UIWidgetText

**Status:** 🟡 CZĘŚCIOWO NAPRAWIONE

**Naprawione:**
- [x] Dodano ścieżkę renderowania dla TTFFont

**Pozostałe problemy:**
- [ ] Używać `font->calculateTextRectSize()` zamiast własnych kalkulacji
- [ ] Obsługa klastrów przy zawijaniu tekstu

### 3.3 Brakujące Tłumaczenia UI

**W `data/locales/pl.lua` brakuje:**
- [ ] `"players online"` → `"graczy online"`
- [ ] `"Enable HTTP login"` → `"Włącz logowanie HTTP"`
- [ ] `"Remember Email"` → `"Zapamiętaj identyfikator konta"`
- [ ] `"Enabling Boosted Creature Panel…"` → brak tłumaczenia
- [ ] Wiele pozycji oznaczonych `= false`

---

## 4. Sieć i Protokół

### 4.1 HTTP/HTTPS

- [ ] Przywrócić pełne bindingi Lua dla HTTP (obecnie wyłączone/stubowane)
- [ ] Wrapper na `cpp-httplib` dla `g_http`
- [ ] Obsługa WebSocket

### 4.2 Protokół Gry

- [ ] Wsparcie dla protokołów 15.00~15.10 (obecnie oznaczone ❌)
- [ ] Wheel of Destiny - 1% implementacji
- [ ] Forge - 1% implementacji
- [ ] Sound Tibia 13 - 80% implementacji

---

## 5. System Budowania (Build System)

### 5.1 vcpkg.json

**Obecne problemy:**
- [ ] Brak `png` jako osobnej zależności (używana przez `freetype`)
- [ ] Możliwe konflikty wersji z `builtin-baseline`

### 5.2 CMake

**Do poprawy:**
- [ ] `find_package(fribidi)` - wymaga pkgconfig
- [ ] Opcje `OTC_ENABLE_TTF`, `OTC_ENABLE_HARFBUZZ` - nie wszędzie używane
- [ ] Warunkowe budowanie dla platform (Android, Browser)

### 5.3 Visual Studio Project

**Plik:** `vc17/otclient.vcxproj`

- [ ] Upewnić się że wszystkie nowe pliki (`LocaleShaping.*`, `TextShaper.*`) są dodane
- [ ] Poprawne filtry w `.vcxproj.filters`
- [ ] CopyFontAssets target wewnątrz `</Project>`

### 5.4 GitHub Actions

**Status:** 🟢 NAPRAWIONE

**Naprawione pliki workflow:**
- [x] `build-windows.yml` - naprawiono niepoprawny 39-znakowy vcpkg commit hash (teraz 40 znaków)
- [x] `build-ubuntu.yml` - dodano pełną konfigurację lukka/run-vcpkg
- [x] `build-linux.yml` - ujednolicono vcpkg commit hash z vcpkg.json
- [x] `build-browser.yml` - dodano pełną konfigurację lukka/run-vcpkg
- [x] `build-windows-solution.yml` - dodano pełną konfigurację lukka/run-vcpkg
- [x] `analysis-sonarcloud.yml` - dodano pełną konfigurację lukka/run-vcpkg

---

## 6. Lokalizacja i Tłumaczenia

### 6.1 Brakujące Języki

**Obecnie dostępne:** PL, DE, ES, PT, SV, EN

**Sugerowane do dodania:**
- [ ] Rosyjski (ru.lua)
- [ ] Francuski (fr.lua)
- [ ] Włoski (it.lua)
- [ ] Turecki (tr.lua)
- [ ] Chiński (zh.lua)
- [ ] Arabski (ar.lua)

### 6.2 System Lokalizacji

- [ ] Przycisk "Language" w topmenu - wymaga dopracowania
- [ ] Auto-reload modułów po zmianie języka
- [ ] Fallback do angielskiego gdy brak tłumaczenia

---

## 7. Dokumentacja

### 7.1 Brakująca Dokumentacja

- [ ] API dokumentacja dla TTFFont/TextShaper
- [ ] Instrukcja dodawania nowych języków
- [ ] Tutorial konfiguracji czcionek TTF
- [ ] Opis formatu `.otfont` dla type:"ttf"

### 7.2 README.md

- [ ] Zaktualizować sekcję "Roadmap"
- [ ] Dodać informację o wsparciu TTF/Unicode
- [ ] Instrukcja kompilacji dla różnych platform

### 7.3 Worklog

Istniejące pliki logów są bardzo szczegółowe ale chaotyczne:
- `plan.md` - bardzo długi, wymaga uporządkowania
- `worklog_all.md` - historia zmian
- `WszystkieSRCLOG.md` - analiza plików źródłowych
- `wykonane_zadania.md` - lista wykonanych zadań

**Rekomendacja:** Skonsolidować do jednego `CHANGELOG_DEV.md`

---

## 8. Optymalizacje Wydajności

### 8.1 Renderowanie Tekstu

- [ ] Cache wyników HarfBuzz shaping
- [ ] Lazy loading glifów (rasteryzuj tylko używane)
- [ ] Batch drawing per-atlas texture
- [ ] Inteligentne zarządzanie atlasami (wyrzucanie nieużywanych)

### 8.2 Pamięć

- [ ] Ograniczenie rozmiaru atlasów TTF
- [ ] Garbage collection dla nieużywanych glifów
- [ ] Optymalizacja struktury `ShapedGlyph`

### 8.3 Multi-threading

- [ ] Async ładowanie czcionek
- [ ] Parallel shaping dla długich tekstów
- [ ] Background rasteryzacja glifów

---

## 9. Bezpieczeństwo

### 9.1 Szyfrowanie

- [ ] Obecna implementacja ENABLE_ENCRYPTION oznaczona jako "unsafe"
- [ ] Rozważyć lepszy algorytm szyfrowania assetów

### 9.2 Walidacja Danych

- [ ] Sprawdzanie granic w parsowaniu OTML
- [ ] Walidacja danych wejściowych z sieci
- [ ] Sanityzacja ścieżek plików

---

## 10. Nowe Funkcjonalności (Propozycje)

### 10.1 Krótkoterminowe (1-3 miesiące)

- [ ] Pełne wsparcie Unicode w UI
- [ ] Kompletna obsługa RTL
- [ ] Emoji support (kolorowe)
- [ ] Więcej lokalizacji

### 10.2 Średnioterminowe (3-6 miesięcy)

- [ ] Wheel of Destiny UI
- [ ] Forge system
- [ ] Pełny Analyzer
- [ ] Sound system Tibia 13

### 10.3 Długoterminowe (6-12 miesięcy)

- [ ] Kompilacja na iOS
- [ ] WebAssembly optymalizacje
- [ ] DirectX 12 renderer
- [ ] Vulkan renderer

---

## 📊 Podsumowanie Priorytetów

| Priorytet | Kategoria | Ilość zadań |
|-----------|-----------|-------------|
| 🔴 Krytyczny | Błędy kompilacji | 5 |
| 🟠 Wysoki | TTF/i18n | 15 |
| 🟡 Średni | UI/UX | 10 |
| 🟢 Niski | Optymalizacje | 8 |
| 🔵 Propozycje | Nowe funkcje | 12 |

---

## 📝 Notatki

1. Większość prac nad TTF/i18n jest już rozpoczęta - wymaga dokończenia
2. Błędy formatowania logów są łatwe do naprawienia ale krytyczne
3. System budowania wymaga stabilizacji przed dalszym rozwojem
4. Dokumentacja powinna być aktualizowana równolegle z kodem

---

*Ten dokument powinien być aktualizowany w miarę postępu prac.*
