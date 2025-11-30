spoko — zrobiłem Ci zwięzły „dziennik prac”: co zmienialiśmy, jakie błędy wyskakiwały, jak je gasiliśmy i co dalej robimy. Wszystko w kolejności, żebyś mógł szybko prześledzić postęp.

Co zrobiliśmy (chronologicznie)

BS::thread_pool — błędy szablonu i ‘detach_task/submit_task’
• Objawy: „nie można wywnioskować argumentów szablonu”, konwersja this z BS::thread_pool → BS::thread_pool<OptFlags>&.
• Przyczyna: użycie typu bez <> i/lub niespójny typ między deklaracją i definicją.
• Fix: w asyncdispatcher.h/.cpp ujednoliciliśmy typ (np. BS::thread_pool<> albo alias using AsyncPool = decltype(BS::thread_pool{std::size_t{1}});) i wszędzie zadeklarowaliśmy/ zdefiniowaliśmy g_asyncDispatcher w tym samym typie. Dodatkowo poprawione getThreadCount() (clamp + fallback).

Konflikt g_app (Console vs Graphical)
• Objawy: „g_app: zmiana definicji; różne typy podstawowe”, brak metod typu getStaticTextScale() (bo widniał ConsoleApplication).
• Przyczyna: w jednych TU był ConsoleApplication g_app, w innych GraphicalApplication g_app.
• Fix: application.h warunkowo dołącza wersję graficzną, a w miejscach, gdzie był extern ConsoleApplication g_app; — zamiana na extern GraphicalApplication g_app;. Wyrzuciliśmy zbędne include’y consoleapplication.h w buildzie OpenGL.

Definicje preprocesora (Windows 10 / WINSOCK / NOMINMAX / grafika)
• Ustalone i działające:
FRAMEWORK_GRAPHICS;NOMINMAX;_WINSOCK_DEPRECATED_NO_WARNINGS;_WIN32_WINNT=0x0A00;WINVER=0x0A00;%(PreprocessorDefinitions)

HTTP layer brakująca (Http, LoginHttp, g_http)
• Objawy: w luafunctions.cpp i net/httplogin.cpp masa „niezdeklarowany identyfikator” (LoginHttp/Http/g_http…).
• Dwie ścieżki:
A) Na szybko — wyłączyliśmy rejestracje HTTP w Lua blokiem #if 0 … #endif (żeby budowa poszła dalej).
B) (Opcjonalnie) Dodaliśmy stub http.h/.cpp + extern Http g_http i #include w luafunctions.cpp — tak, żeby kompilacja przechodziła nawet bez realnego HTTP.
• Na później: docelowo zrobić wrapper na cpp-httplib i przywrócić bindingi Lua.

stdext::format i brak funkcji konwersji UTF
• Objawy: „stdext::format nie istnieje” + później „utf16_to_utf8, utf8_to_utf16, latin1_to_utf16, utf16_to_latin1 nie są składową stdext”.
• Fix:
– dla formatowania: przejście na fmt::format albo dodanie lekkiego stdext/format.h.
– dla UTF: dodaliśmy/uzupełniliśmy stdext/string.h i stdext/string.cpp z implementacjami tych funkcji i włączyliśmy właściwe #include w win32platform.cpp i win32window.cpp.
– dodatkowo poprawiliśmy include order Windows (zawsze #include <windows.h> przed shellapi.h, winuser.h itd.).

X11 na Windows (Xlib.h)
• Objawy: „X11/Xlib.h: No such file or directory” na Windows.
• Fix: x11window.h owrapowany #if !defined(_WIN32) && !defined(_WIN64) (lub #ifdef __linux__), żeby na Windows nie ładować plików X11. Usunęło to cały szum X11 w buildzie MSVC.

platformwindow.cpp – błąd deklaracji ‘window’
• Objawy: „brak specyfikatora typu / niezadeklarowany identyfikator ‘window’”.
• Przyczyna: po zmianach ifdef-ów Windows/Linux kompilował się zły branch (np. z X11Window na Windows) albo brak forwarda/inkluza.
• Fix: dopięliśmy poprawne guardy platformowe i include’y (win32window.h vs x11window.h) – po poprawkach PlatformWindow::create wraca prawidłowy typ dla bieżącej platformy.

TTFFont / TextShaper – nierozpoznane symbole (LNK2019)
• Objawy: „nierozpoznany symbol zewnętrzny TTFFont::load/drawText/measureTextWidth/ctor/dtor”.
• Przyczyna: pliki są w repo, ale nie były dodane do projektu VS (albo do złego filtru/konfiguracji, albo Excluded From Build).
• Fix: dodaliśmy TTFFont.cpp i TextShaper.cpp do otclient.vcxproj (Source Files), upewniliśmy się, że OpenGL|x64 ma Excluded From Build = No. Nazwy/namespace zgodne z *.h.

Wewnętrzny błąd kompilatora MSVC (ICE) w otmlnode.cpp
• Objaw: fatal error C1001 w linii OTMLNode::at(...) (przy fmt::format(...)) na Unity/LTCG.
• Fix: przepisaliśmy tę metodę, zastępując fmt::format zwykłą konkatenacją std::string i ew. wyłączyliśmy optymalizację /Od tylko dla tego pliku. ICE zniknął.

MSB4011 (vcpkg props/targets importowane podwójnie)
• Objawy: ostrzeżenia, brak wpływu na link/kompilację.
• Fix (opcjonalny): zostawić jedną integrację vcpkg (albo Directory.Build.*, albo user-wide vcpkg.user.*). Nie wpływa na funkcję programu.

Konfiguracja projektu
• Runtime: /MT (MultiThreaded, statyczny runtime) – spójne w konfiguracji OpenGL|x64.
• SSE2 on, optymalizacje na „MaxSpeed” (poza otmlnode.cpp jeśli /Od potrzebne).
• Preprocessor definicje – patrz pkt 3 (ustabilizowane).

Najczęstsze pytania, które padły i odpowiedzi

„Czemu wcześniej nie wyskakiwały błędy HTTP/TTFFont?”
Bo wcześniejsze, „grubsze” błędy (thread pool/g_app/X11) zatrzymywały kompilację wcześniej. Po ich naprawie kompilator doszedł dalej i zaczął zgłaszać kolejne braki.

„Czy potrzebujemy osobnej biblioteki stdext::format?”
Nie. Mamy fmt z vcpkg; możesz używać fmt::format. Tam, gdzie MSVC łapie ICE, stosujemy zwykłą konkatenację.

„Co robi #include "stdext/compiler.h" w global.h?”
To zestaw detekcji kompilatora/atrybutów; nic związanego z naszymi błędami. Zostawiamy.

Stan obecny

Build dochodzi do linkowania; problemy X11/BS-thread-pool/g_app/UTF i WinAPI — ogarnięte.

ICE w otmlnode.cpp — obejście wprowadzone (konkatenacja / ewentualnie /Od dla pliku).

TTFFont/TextShaper — dodane do projektu; link przechodzi (jeśli któryś wariant dalej sypnie LNK2019, sprawdzamy czy plik nie jest „Excluded From Build” w tej konfiguracji).

HTTP bindingi do Lua — tymczasowo wyłączone lub „stubowane”.

Plan dalszy (kolejność i konkrety)

Teksty międzynarodowe (nasz główny cel): HarfBuzz + FriBidi + FreeType

Upewnij się, że w vcpkg masz: freetype, harfbuzz (najlepiej harfbuzz[icu]), fribidi.

Sprawdź, czy TextShaper realnie używa HB/Fribidi:
• hb_buffer_set_direction() → HB_DIRECTION_RTL dla arabskiego,
• hb_buffer_set_script(), hb_buffer_set_language(),
• przebieg przez fribidi_log2vis (Bidi),
• fallback czcionek (łańcuch TTFFont::load z listą fontów: np. Noto Naskh Arabic, Noto Sans, itp.).

Test „ręczny”: wyświetl zdanie arabskie + kombinacje łączące (hamza, kashida…), cyrylicę, grekę, PL znaki — sprawdź szerokości (measureTextWidth) i rendering (drawText).

TTF/atlas

Zweryfikuj, że TTFFont::load ładuje czcionki z dysku (FreeType) i że kod ścieżek jest OK na Windows (UTF-16 ↔ UTF-8).

Jeżeli atlas jest generowany asynchronicznie, upewnij się, że używa naszego g_asyncDispatcher (spójny typ).

Przywrócić HTTP (po zbudowaniu UI)

Zamiast stubów: zrobić wrapper w src/framework/net/http.* na cpp-httplib (GET, POST, download; opcjonalnie WS).

Odblokować blok w luafunctions.cpp i zapewnić, że global g_http jest dostępny.

Jeżeli httplogin.cpp używa g_http, podłączyć faktyczne wywołania.

Porządki w projekcie

(opcjonalnie) wyciszyć MSB4011 — zostawić jedną integrację vcpkg.

Na Windows trwale ukryć X11 (guardy w nagłówkach / CMake/vcxproj).

Sprawdzić, czy nigdzie nie pozostał include do consoleapplication.h.

CI (GitHub Actions)

Skoro mieliśmy już działającego EXE w Actions, zaktualizować workflow tak, by:
• doinstalowywał harfbuzz/fribidi,
• wrzucał zestaw czcionek (Noto) do assets (dla fallbacków),
• robił krótki smoke-test (uruchomienie, render przykładowych napisów).

QA rendering

Wizualny check: ligatury arabskie, mark-positioning, łączenie form (initial/medial/final), kropki i diakrytyki — czy siedzą.

Szerokości tekstu w UI (wrap, caret) odpowiadają wynikowi shaping.

Gdyby wróciły konkretne problemy

LNK2019 na TTFFont/TextShaper — 99% to „plik .cpp nie jest faktycznie kompilowany w tej konfiguracji”. PPM → Właściwości → Ogólne → Wyklucz z kompilacji = Nie dla OpenGL|x64.

ICE MSVC w innym miejscu — wyłączyć fmt::format w tym miejscu, ewentualnie /Od dla tego jednego pliku.

X11 znowu się kompiluje — sprawdzić guardy w nagłówkach i definicje preprocesora (na Windows musi wejść ścieżka Win32, nie X11).

HTTP — jeśli chcesz od razu pełne HTTP + WS, daj znać, podeślę gotowy minimalistyczny wrapper na cpp-httplib.

Jeśli chcesz, mogę teraz wypunktować konkretne pliki, w których na pewno powinny być nasze ostatnie zmiany (żebyś porównał u siebie):
asyncdispatcher.h/.cpp, application.h, wszystkie miejsca z extern g_app, x11window.h, platformwindow.cpp/.h, win32platform.cpp, win32window.cpp, framework/stdext/string.h/.cpp (i ewentualnie format.h), framework/graphics/text/TTFFont.*, framework/graphics/text/TextShaper.*, framework/luafunctions.cpp, framework/net/httplogin.* (+ ewentualnie net/http.* jeśli poszedłeś w stub).

Daj znać, na czym chcesz się skupić teraz: od razu integrujemy HarfBuzz/FriBidi i fallbacki fontów, czy najpierw porządkujemy HTTP?

plan_NEXT_STEPS_2025-08-30.md (tylko klient/instalka)

Powtórny build i smoke-test czcionek

UI/label/console: ąćęłńóśżź • Привет • Ελληνικά • مرحبا • 你好.

Sprawdź caret/wrapping dla RTL.

Sprawdzenie ścieżek nagłówków

Project → C/C++ → Additional Include Directories: dodane …\vcpkg\installed\x64-windows\include.

(Zostańmy przy include’ach <harfbuzz/...> w kodzie).

Fallback łańcucha czcionek (opcjonalnie, ale polecam domknąć teraz)

W .otfont ustaw domyślne: NotoSans-Regular → Noto Naskh Arabic → NotoSansSC → (ostatni fallback).

Upewnij się, że mono (NotoSansMono) jest przypięty do konsoli/logów.

Ekran logowania/entergame — szybki smoke przy języku pl/ru/ar/zh.

Jeśli pojawi się nowy błąd kompilacji — wrzuć log; dopasujemy sygnatury/#include bez zmiany API.

Plan dla drawpool.*
1) Stan obecny (co jest już OK)

Batchowanie po stanie: PoolState hashuje opacity, clipRect, shaderProgram, compositionMode, blendEquation, transformMatrix, color i texture->hash() — to wystarcza, by łączyć rysowanie glifów per tekstura-atlas (idealne dla TTF).

Współpraca z atlasami: w add(...) jest gałąź z:

if (m_atlas && texture) → texture->getAtlasRegion(...) → translacja method.src do regionu. To pozwala użyć jednego, globalnego atlasu lub pominąć, gdy TTF używa własnych atlasów (brak regionu = idzie bez translacji).

Bufor współrzędnych: jest pula (getCoordsBuffer() + recykling), a add(..., coordsBuffer) obsługuje gotowe quady bez ponownego wypełniania. Perfect do „wrzucenia” porcji glifów na raz.

Hash dla framebufferów: dodatkowy „pool hash” uwzględnia parametry metody rysowania – bezpieczne przy tekście (nie rozbije batcha, gdy korzystamy z coordsBuffer).

Blend/kompozycja: API (setCompositionMode, setBlendEquation) jest — nie trzeba dotykać drawpool, żeby tekst miał zwykły alpha-blend. Domyślne ścieżki są OK.

Wniosek: sam drawpool.* nie wymaga zmian, żeby TTF-y działały i batchowały się per-atlas.

2) Czego nie robi drawpool.\* (i dobrze, że nie robi)

Nie zna pojęcia „glifów”. Operuje teksturą + CoordsBuffer. To jest właściwa warstwa abstrakcji — cała „inteligencja” (shaping, raster, grupowanie per atlas) powinna zostać w TTFFont/BitmapFont.

3) Minimalne wymagania integracyjne (do zrobienia poza drawpool)

To są „must have”, inaczej tekst TTF nie będzie widoczny.

TTFFont musi realnie mieć/udostępnić tekstury atlasów (GPU):

Dzisiaj w TTFFont.cpp UV/atlasId są „wirtualne” (komentarz, że upload pominięty).

Potrzebne jest albo:
a) API w TTFFont: zwróć porcje quadów pogrupowane per-atlas i dostarcz wskaźnik/uchwyt tekstury dla każdej porcji, albo
b) API typu emitToDrawPool(...), które wewnątrz zrobi g_drawPool.getCoordsBuffer() → wypełni quady → g_drawPool.add(..., texture, /*coordsBuffer*/) — czyli TTFFont sam doda batch’e do kolejki.

Warstwa nagłówka TTFFont.h jest „lekka” (bez Texture). Jeśli chcesz ją taka zostawić, wybierz (a): metoda zwraca std::vector<Batch>, gdzie Batch zawiera: atlasId, std::vector<Quad>. A „mostek” (np. w BitmapFont::drawText) zamienia atlasId→TexturePtr i woła g_drawPool.

BitmapFont::drawText (gałąź TTF) musi dodać do DrawPool:

Dziś woła m_ttf->drawText(...), która buduje quady lokalnie i kończy — nic nie trafia do g_drawPool.

Poprawka: w BitmapFont::drawText po shapingu/rasteryzacji grupuj glify per atlas, dla każdej grupy:

auto cb = g_drawPool.getCoordsBuffer();

wpisz do cb prostokąty dest+UV dla każdego glifu,

g_drawPool.add(color, textureForThatAtlas, /*DrawMethod default*/, cb);

Repaint po powiększeniu atlasu TTF:

Gdy TTFFont dołoży nowe glify i musi przealokować teksturę atlasu (nowy obiekt/handle), trzeba wywołać g_drawPool.repaint() (albo ustawić stan zmuszający do przeliczenia hash’y), żeby uniknąć artefaktów/starych batchy.

Klipowanie:

DrawPool ma clipRect w stanie; CachedText/widget powinny ustawić go po swojej stronie (już jest), a gałąź TTF nie może go pomijać. Jeśli wołasz BitmapFont::drawText z UI, klip trafi do batch’y przez stan poola — tu nic nie trzeba w drawpool.

4) Opcjonalne ulepszenia w drawpool (nie są konieczne, ale ułatwią życie)

Jeśli chcesz czytelniejszego API bezpośrednio dla tekstu:

Dodać w DrawPoolManager (to nie jest w drawpool.\*, ale wspomnę) helper:

addTexturedQuads(const TexturePtr&, std::span<const Quad>, const Color& = Color::white)
…który wewnątrz zrobi getCoordsBuffer() + wpisanie quadów + add(...).
(Nie zmieniamy drawpool.\*; dokładamy tylko wygodny wrapper).

(Ewentualnie) wariant addTexturedQuadsGrouped(const TexturePtr&, const std::vector<Quad>&, size_t begin, size_t end), gdy będziesz dzielić na wiersze/akapit.

5) Ryzyka i/lub rzeczy do potwierdzenia

Precyzja współrzędnych: Rect/Point w buforze są całkowite? Jeśli tak, subpikselowy kerning będzie zaokrąglany. To zwykle OK (HarfBuzz daje dobry efekt nawet po round), ale gdybyś widział „drżenie” na animowanym skalu, warto w przyszłości sprawdzić, czy CoordsBuffer wspiera float (część naszych gałęzi już to robi).

Atlas globalny vs. TTF: jeśli ustawisz m_atlas w DrawPool, a TTFFont-owe tekstury nie są w nim zarejestrowane, kod i tak ominie translację (brak regionu → użyje oryginalnej tekstury). To jest poprawne.

6) Testy integracyjne (checklista)

Łacinka + diakrytyki: „Zażółć gęślą jaźń — ąćęłńóśźż” (wyrównanie: L/C/R; kolorowanie).

Cyrylica: „Привет мир”.

Arabskie RTL: „مرحبا بالعالم” (sprawdź kierunek i ligatury).

CJK: „你好，世界 / こんにちは世界 / 안녕하세요 세계”.

Fallback: znak tylko w fallback (np. emoji jeśli masz font).

Wrap: łamanie z CachedText::wrapText(...) + klip w widgetach.

Atlas growth: długi tekst wymuszający nowy atlas — brak artefaktów po repaint().

Podsumowanie decyzji

drawpool.h / drawpool.cpp – bez zmian. Są gotowe do batchowania tekstu TTF per-atlas poprzez coordsBuffer.

Do zrobienia obok (następny krok): poprawka gałęzi TTF w BitmapFont::drawText (i ewentualnie lekkie API w TTFFont / helper w DrawPoolManager), aby realnie wrzucać quady do g_drawPool.

Krótko: tak — Twoje logi zgadzają się z planem, a gdzie trzeba doprecyzuję, żeby było 1:1.

Zgodność punkt po punkcie

drawpooltext.cpp (+ ewent. drawpoolmanager.cpp) – „przekierować generowanie współrzędnych tekstu na kształtowane glify”
✔️ To dokładnie nasz kierunek: nie iterujemy już po bajtach/ASCII; bierzemy runs z HarfBuzz (klastry, kerning, RTL) i z nich budujemy verteksy.

Warstwa rysowania (drawpooltext, cachedtext) „musi zrozumieć klastry”
✔️ Zrobiliśmy to już w CachedText (TTF fast-path przez BitmapFont::drawText / TTFFont::shape).
➕ W drawpooltext.cpp zrobimy analogicznie: konsumuje shape()+buildQuads() zamiast „szerokość-znaku × indeks”.

„Przełączymy geometrię na TTFFont::shape()+buildQuads()”
✔️ Tak. Zostawiamy stary kod bitmap jako fallback; dla TTF: shape → raster → quady.

Cel: TTFFont rasteryzuje do atlasów, UI rysuje przez DrawPool/Texture
✔️ Zgadza się. DrawPool nie wymaga zmian w core; tylko trzeba mu dostarczyć CoordsBuffer + Texture per atlas.

TexturePtr getAtlasTexture(int atlasId) const; w TTFFont
✔️ Tego właśnie potrzebujemy. My to nazywaliśmy „getter tekstury atlasu”; bez niego drawpooltext nie dobierze właściwej tekstury dla partii quadów.

„Grupujemy quady wg atlasId i wołamy add… per atlas”
✔️ Tak. Dokładnie: auto cb = g_drawPool.getCoordsBuffer(); → wpisujemy quady → g_drawPool.addTexturedCoordsBuffer(textureForAtlas, cb, color); dla każdego atlasId osobno.

„framework/graphics/drawpooltext.cpp – batchowanie quadów per atlas i rysowanie z getAtlasTexture(...)”
✔️ Pełna zgodność. To będzie ostatni brakujący klocek, żeby TTF wyświetlał się wszędzie (nie tylko przez CachedText).

„BitmapFont::drawText(...) ma delegować do shape/buildQuads (zamiast metryk bitmapy)”
✔️ Zrobione już jako mostek (mamy wrapper w TTFFont i TTF-gałąź w BitmapFont::drawText). W docelowej wersji BitmapFont::drawText również dorzuci do DrawPool batch’e per atlas (albo zwróci je do drawpooltext).

Małe doprecyzowania (żeby nie było niespodzianek)

Gdzie trzymać „prawdziwe” rysowanie?
Dwie równe ścieżki:

„Push do DrawPool” w BitmapFont::drawText (najprościej — już to robimy w CachedText).

„Push” w drawpooltext.cpp, a BitmapFont/TTFFont tylko zwraca quady + atlasId.
Obie zgodne z logami. Żeby nie dublować, proponuję (2) dla drawpooltext (bo to plik „od rysowania”) i zostawić (1) w CachedText jako szybki mostek do czasu pełnego wdrożenia.

Realloc atlasu TTF
Jeśli TTFFont powiększy atlas (nowa tekstura), warto wymusić repaint (np. g_drawPool.repaint() lub przez zmianę stanu), żeby stare batch’e nie wisiały z nieaktualnym handlem.

Precyzja pozycji glifów
Jeśli CoordsBuffer używa float — super (subpixel kerning). Jeśli int — HB i tak wygląda dobrze, ale przy animacji skali może być „drżenie”; to tylko uwaga na przyszłość.

Co jest teraz w TTFFont.cpp (stan z text.zip)

Atlas jest „logiczny” (tylko UV i rozkład slotów), brak realnego uploadu bitmap glifów do tekstur GPU.
→ w rasterizeGlyph(...) masz komentarz: „pomijamy realny upload do tekstury (to integracja z TextureManager w innym miejscu)” — to blokuje rysowanie.

drawText(...) robi shape + (komentowane) ensureGlyphsRasterized(...), po czym jest TODO: „buildQuads + rysowanie do DrawPool po integracji atlasów/tekstur” – nic nie trafia do DrawPool.

Techniczna niespójność: deklaracja w nagłówku to bool ensureGlyphsRasterized(const std::vector<ShapedGlyph>&) const;, a w .cpp implementacja to bool ensureGlyphsRasterized(std::vector<ShapedGlyph>&) const (bez const na wektorze). Implementacja modyfikuje glify (ustawia atlasId, UV itd.), więc nagłówek będzie trzeba poprawić przy okazji (w następnym kroku).

Co dodajemy / zmieniamy w TTFFont.cpp (dokładny plan)
FRAG T1 — Dodanie wsparcia na tekstury atlasów (GPU)

Gdzie: blok include + sekcja PIMPL.
Co:

W .cpp dodamy lekkie forward-declare i alias do Texture (żeby nie robić ciężkich include’ów):
class Texture; using TexturePtr = std::shared_ptr<Texture>;

W struct Impl dodamy kontener na tekstury atlasów i bufor pomocniczy:

mutable std::vector<TexturePtr> atlasTextures;

prosty helper ensureAtlasTexture(int aid) (tworzy teksturę o rozmiarze atlasu, jeśli brak).

helper uploadSubImage(int aid, int x, int y, int w, int h, const uint8_t* src, int srcPitch) (przesyła region bitmapy glifu do tekstury atlasu).

Uwaga: same wywołania do GPU damy „w punktach wstawienia” (patrz T3); nazwy metod będą zgodne z Twoją warstwą (np. texture->updateSubRegion(...) – dobiorę dokładnie przy patchu, trzymając się konwencji z Twoich plików graficznych; jeśli nie ma takiej nazwy, dam lokalny minimalny wrapper).

FRAG T2 — Zapewnienie istnienia atlasu (rozszerzenie allocInAtlas)

Gdzie: przy allocInAtlas(...) i zaraz po nim.
Co: po dołożeniu nowego atlasu logicznego, tworzymy od razu odpowiadającą mu teksturę (przez ensureAtlasTexture(aid)), dzięki czemu później upload glifów ma już cel GPU.

FRAG T3 — Rzeczywisty upload glifu w rasterizeGlyph(...)

Gdzie: w miejscu z obecnym komentarzem:

// „Rezerwujemy” miejsce w atlasie i zapisujemy UV — w tym szkielecie
// pomijamy realny upload do tekstury (to integracja z TextureManager w innym miejscu).


Co:

Po allocInAtlas(...) kopiujemy piksele z FT_Bitmap (8-bit alpha) do tekstury atlasu:
uploadSubImage(out.atlasId, x, y, out.width, out.height, bm.buffer, bm.pitch);

Zostawiamy obecne wyliczenie UV (u0..v1) – jest OK.

Efekt: każdy zrastryzowany glif ląduje w fizycznej teksturze atlasu i ma poprawne UV.

FRAG T4 — Publiczne gettery atlasów (implementacje w .cpp)

Gdzie: na końcu pliku (po istniejących metodach).
Co: implementacje metod, które dodamy do nagłówka w kolejnym kroku:

int TTFFont::atlasCount() const noexcept; → return static_cast<int>(d->atlases.size());

TexturePtr TTFFont::getAtlasTexture(int atlasId) const; → return (0<=id<atlasTextures.size()) ? atlasTextures[id] : nullptr;

Dzięki temu BitmapFont::drawText(...) albo drawpooltext.cpp będą mogły pobrać TexturePtr dla partii quadów i wrzucić je do g_drawPool.

FRAG T5 — Użycie uploadu/atlasów w „tymczasowym” drawText(...) (bezpośredni mostek)

Gdzie: w ciele drawText(...).
Co: odkomentujemy ensureGlyphsRasterized(shaped), zbudujemy quady i (tymczasowo) pogrupujemy je per atlasId, po czym:

auto cb = g_drawPool.getCoordsBuffer();

wpis quady do cb (masz buildQuads(...))

g_drawPool.addTexturedCoordsBuffer(getAtlasTexture(aid), cb, Color::white) – tu nie mieszamy koloru, bo w BitmapFont::drawText i tak przekazujemy color; jeśli wolisz, zostawimy samo budowanie quadow, a push do DrawPool zrobimy w BitmapFont::drawText — obie opcje są zgodne z Twoim planem.

(Jeśli chcesz, żebym trzymał się ściśle rozdziału obowiązków, pominę T5 i zrobimy push do DrawPool wyłącznie w BitmapFont::drawText oraz w drawpooltext.cpp — daj znać, którą wersję preferujesz.)

FRAG T6 — Drobne sprzątanie przy shapingu

hb_buffer_set_cluster_level(... MONOTONE_CHARACTERS) — OK. Zostaje.

normalizeParams(p) już jest i używa LocaleShaping — zostaje.

Drobna niezgodność nagłówka (do poprawy w następnym kroku)

W TTFFont.h zmienimy sygnaturę:

bool ensureGlyphsRasterized(std::vector<ShapedGlyph>& shaped) const;


(teraz jest z const std::vector<...>&, a implementacja musi modyfikować glify: atlas/UV/w/h/bearing).

Do TTFFont.h dopiszemy (zgodnie z plan.md):

int atlasCount() const noexcept;
std::shared_ptr<class Texture> getAtlasTexture(int atlasId) const;

Ostatnia analiza (Nie zrobiliśmy jeszcze Frag T6)
FRAG T4 (implementacje getterów w .cpp):
✔ atlasCount() (l. 280–282) i getAtlasTexture(...) (l. 284–288) zdefiniowane w .cpp.
⚠️ Uwaga: deklaracji w .h jeszcze nie ma (to właśnie FRAG T6).

FRAG T5 (grupowanie per-atlas):
◑ Jest statyczna funkcja plikowa buildAtlasBatches(...) + struct AtlasBatch (l. 434–479).
⚠️ Nie jest jednak nigdzie użyta (np. drawText(...) jej nie woła), ani nie jest metodą klasy (brak dostępu z zewnątrz). To jest niedomknięte.

Błędy/rozbieżności blokujące lub ryzykowne
Rozcięty komentarz blokowy („zielony fragment”) — błąd składniowy

Blok „stare heurystyki” otwierasz na l. 301: /* --- Previous heurystyki ... i nigdzie go nie zamykasz.

W środku, na l. 314, masz /* plus Hebr itd. */, co zamyka otwarty komentarz (C/C++ nie wspiera zagnieżdżania) i od końcówki tej linii kod wraca „na żywo”, zostawiając fragmenty jak ) p.direction = TextDirection::RTL; poza funkcją. To psuje składnię (i tłumaczy „dziwne” zielone podświetlenie).

Do naprawy: w HOTFIX-ie usunąć to wewnętrzne /* ... */ (zastąpić tekstem „(plus Hebr itd.)”) i/lub domknąć główny komentarz */ po linii 316.

Brak zamknięcia przestrzeni nazw

Na końcu pliku jest tylko komentarz // namespace otc::text (l. 542), brakuje }. To również łamie kompilację. (HOTFIX-NS)

Niespójne sygnatury z nagłówkiem (TTFFont.h) — blokuje kompilację

ensureGlyphsRasterized w .h:
bool ensureGlyphsRasterized(const std::vector<ShapedGlyph>&) const;
w .cpp:
bool TTFFont::ensureGlyphsRasterized(std::vector<ShapedGlyph>&) const (l. 484).
Funkcja modyfikuje glify → nagłówek musi mieć bez const i referencję nie-const. (To element FRAG T6).

buildQuads w .h ma wariant z wyjściowym wektorem i void zwrotem, a w .cpp zwracasz std::vector<Quad> i masz dodatkowy baselineShift (l. 517). Te sygnatury muszą zostać ujednolicone (FRAG T6).

Zdefiniowałeś w .cpp członków klasy atlasCount() oraz getAtlasTexture(...), ale nie ma ich deklaracji w klasie w .h. Kompilator zgłosi „no member named …” podczas kompilacji .cpp. (FRAG T6)

FRAG T5 jest „w połowie drogi”

buildAtlasBatches(...) jest funkcją statyczną w pliku (nie-metodą), więc nie wyeksportujesz jej do BitmapFont/drawpooltext.

drawText(...) (l. 398–432) nadal ma TODO i nic nie rysuje, a wywołanie ensureGlyphsRasterized(shaped) jest zakomentowane.

Dokończenie: albo (A) zmieniamy buildAtlasBatches na metodę klasy i deklarujemy w .h (FRAG T6) do użycia w BitmapFont/drawpooltext, albo (B) podpinamy ją lokalnie w drawText(...) i tam robimy batchowanie + push do DrawPool (wariant b z planu).

Backend tekstur — placeholder

ensureAtlasTexture(...) i uploadSubImage(...) mają TODO (brak realnego createTexture/upload). To jest świadome na tym etapie, ale pamiętaj: bez tego + bez integracji w drawpooltext.cpp nic nie zobaczysz na ekranie. (Plan wskazuje, że realny push robimy w drawpooltext.cpp i/lub BitmapFont::drawText.)

Co działa dobrze / zgodność z planem

Shaping: jest hb_buffer_set_cluster_level(...MONOTONE_CHARACTERS) (l. 329) i normalizeParams(...) z LocaleShaping (l. 292–299) — zgodnie z planem.

Fallback chain: jest setFallbackChain(...) (l. 263–272) i użycie fallbacków w rasteryzacji (for faceIdx 0..N, l. 497–513).

Raster & cache: glyphCache, allocInAtlas, UV, rasterizeGlyph → kompletna ścieżka do danych geometrycznych (poza uploadem).

czy to poprawia ten punkt ? 

Backend tekstur — placeholder ensureAtlasTexture(...) i uploadSubImage(...) mają TODO (brak realnego createTexture/upload). To jest świadome na tym etapie, ale pamiętaj: bez tego + bez integracji w drawpooltext.cpp nic nie zobaczysz na ekranie. (Plan wskazuje, że realny push robimy w drawpooltext.cpp i/lub BitmapFont::drawText.)
ChatGPT powiedział:
Myślał przez 9s

Tak — w TTFFont.cpp ten punkt jest poprawiony: placeholdery i duplikaty usunięte, a ensureAtlasTexture(...)/uploadSubImage(...) korzystają z hooków backendu. Żeby faktycznie coś zobaczyć, trzeba jeszcze zarejestrować backend i użyć go w drawpooltext.cpp/BitmapFont::drawText.


Co zostało ustalone i zrobione:

Analiza uitextedit.cpp:

Zidentyfikowałem miejsca wymagające dostosowania do TTF, podobnie jak w UIWidgetText.

drawSelf() obecnie zwraca, jeśli m_font->getTexture() jest puste – to blokuje TTF, bo TTF nie posiada tekstury bitmapowej.

update() zawsze wywołuje m_font->calculateGlyphsPositions(), co nie ma sensu dla TTF.

Plan zmian w uitextedit.cpp:

Fast‑path dla TTF w drawSelf(): jeśli m_font->isTTF(), nie sprawdzamy texture, tylko wywołujemy m_font->drawText(m_drawText, m_drawArea, m_color, m_textAlign); w wersji jednokolorowej jest to proste, w wersji kolorowej trzeba pętli po segmentach.

Zaznaczenie: dla TTF rysować tło zaznaczenia za pomocą addFilledRect, ale pominąć addTexturedRect, ponieważ brak bitmapy (i gładzenie nie jest potrzebne).

Aktualizacja w update(): nie wywoływać calculateGlyphsPositions dla TTF; to może zostać wykorzystane w przyszłości dla dokładniejszego kursora, ale na razie zostawić je tylko dla bitmapy.

Zachować rysowanie kursora z użyciem getGlyphHeight(), które działa zarówno dla bitmapy, jak i TTF.

Informacja o rejestracji backendu:

W TTFFont.cpp wbudowany jest mechanizm back‑end. Aby TTF pojawił się na ekranie, należy zarejestrować dwie funkcje (gCreateAtlasTexture, gUploadAtlasSubImage) po inicjalizacji grafiki – dzięki temu rasteryzacja będzie wgrywać glify do atlasów i BitmapFont je narysuje.

Co pozostało do zrobienia:

Wprowadzić zmiany w uitextedit.cpp (zgodnie z powyższym planem) poprzez patch (modyfikacja warunków w drawSelf() i update(), obsługa kolorów TTF), a następnie przetestować, czy tekst TTF wyświetla się i zaznaczenie działa zgodnie z oczekiwaniami.

Dokończyć ew. obsługę wielokolorowego tekstu w TTF (podział na segmenty) w UITexEdit i UIWidgetText, jeśli potrzebna jest finezyjna obsługa.

Po modyfikacji zarejestrować backend tekstur i uruchomić aplikację, by upewnić się, że wszystkie glify (również spoza podstawowego fontu) rysują się poprawnie.

Jeśli będzie potrzeba dalszych działań lub dokładnych patchy, jestem gotów je przygotować w kolejnej sesji.
Co wymaga zmiany w uitextedit.cpp

Fast path dla TTF w drawSelf:
Obecnie funkcja zwraca, jeśli m_font->getTexture() jest puste. Trzeba zmienić ten warunek na if (!m_font->isTTF() && !texture) return; – czyli dla TTF (bez tekstury) nie przerywamy rysowania. Następnie, po wyliczeniu m_drawArea, należy wywołać m_font->drawText(m_drawText, m_drawArea, m_color, m_textAlign); zamiast używać CoordsBuffer.

Rysowanie zaznaczenia:
Aktualnie zaznaczenie rysuje dwa razy: addFilledRect tło i addTexturedRect z kolorem. W TTF powinniśmy pozostawić tylko wypełnienie tła (addFilledRect) i pominąć addTexturedRect, bo nie ma pojedynczej bitmapy glifu.

Obliczanie pozycji glifów w update:
Funkcja update() zawsze wywołuje calculateGlyphsPositions(). Dla TTF to niepotrzebne – należy dodać warunek if (m_font && !m_font->isTTF()) przed obliczeniem, aby pominąć tę część dla TTF.

Jak to zrobić

Przed zakończeniem sesji udało mi się przygotować koncepcję patchy, ale nie wprowadziłem ich do pliku. Aby wprowadzić zmiany:

Otwórz uitextedit.cpp.

Zastąp fragment w drawSelf odpowiedzialny za if (!texture) return; warunkiem uwzględniającym TTF, a następnie dodaj m_font->drawText(...) w sekcji rysowania zamiast addTexturedCoordsBuffer.

W sekcji zaznaczenia (zwykle w drawSelf) warunkowo nie wywołuj addTexturedRect dla TTF.

W update() owiń wywołanie calculateGlyphsPositions w if(!m_font->isTTF()).

Efekt

Po wprowadzeniu tych poprawek UITextEdit powinien poprawnie wyświetlać tekst TTF, rysować zaznaczenie jako wypełnione prostokąty oraz obsługiwać kursor z wartością wysokości zwróconą przez TTFFont::getGlyphHeight().

Sugeruję w kolejnej sesji zastosować powyższe modyfikacje z wykorzystaniem container.apply_patch i zsynchronizować zmieniony plik z użytkownikiem.

Krótko: tak — po tych 4 krokach w uitextedit.cpp trzeba domknąć minimalne zmiany z Batch A/B w kilku sąsiednich plikach, żeby kompilacja i runtime były spójne. „Pełne” klastry (ShapedGlyph.cluster) i bidi dopniemy później, ale teraz potrzebne są mostki, które już masz rozpisane w planie/logach.

Co dopiąć teraz (minimum, bez pełnego RTL/cluster logic)

framework/graphics/bitmapfont.{h,cpp} – dodać gałąź TTF (flaga m_isTTF, uchwyt do TTFFont), delegować pomiar szerokości/metryki do TTFFont. Dodatkowo autodetekcja type:"ttf" gdy w .otfont jest source, oraz bezpieczny log „TTF load failed…”.

framework/graphics/cachedtext.{h,cpp} – jeśli font jest TTF, buforować shaped runs (wektor ShapedGlyph) zamiast „bajtowych” glifów; zostawić stary tryb dla bitmap.

framework/gui/uifont.cpp – lookup fontu przez g_fonts tak, by pod jedną nazwą mógł przyjść TTF z fallbackami (bez wciągania FT/HB do nagłówków).

framework/ui/uiwidget.{h,cpp} + framework/ui/uiwidgettext.cpp – w miejscach, gdzie dziś UI zakłada atlas bitmap, przejść na font->calculateTextRectSize() / drawText() tak, by przy TTF wewnątrz poszła ścieżka TTFFont (bez dotykania API UI).

framework/ui/uitextedit.{h,cpp} – to już zrobiłeś; na tym etapie wystarczy, że caret/selektor nie iteruje po bajtach UTF-8, tylko używa klastrów (tymczasowo, bez pełnego bidi).

To jest dokładnie to, co w naszych logach nazwaliśmy „Batch A/B — mostki kompilacyjne + UI-glue”. Po tym kroku UI działa z TTF (PL/RU/CJK w LTR), a pełne RTL/bidi i precyzyjne klastry dopniemy później.

Czego nie ruszamy teraz (zostaje na „pełne przejście”)

Atlas + realny render TTF w DrawPoolText (batchowanie quadów per atlas, TTFFont::getAtlasTexture()/buildQuads()). To jest Batch C.

Pełne bidi/cluster-accurate caret/wrap w edytorze (zrobimy po A/B).

Drobny, ale ważny fix builda (poza UI)

framework/net/protocol.cpp – zamiana %i → {} w loggerze (twardy błąd formatowania, potrafi zabić Unity/MSVC).

Jeśli chcesz, dorzucę w następnym kroku gotowe, małe „delta-patche” bez generowania całych plików, dokładnie w miejscach wskazanych powyżej (z kontekstem 5–10 linii przed/po), ale logicznie: tak, przenosimy zmiany do tych plików z Batch A/B i dopiero potem robimy „pełne” klastry i bidi.



 /// [TEX-BACKEND API] Rejestracja hooków uploadu atlasu (ustawiane z warstwy graphics)
 using CreateAtlasTextureFn = TexturePtr(*)(int width, int height);
 using UploadSubImageFn    = void(*)(const TexturePtr& tex, int x, int y, int w, int h, const uint8_t* src, int pitch);
 void TTFFont_SetTextureBackend(CreateAtlasTextureFn createFn, UploadSubImageFn uploadFn);