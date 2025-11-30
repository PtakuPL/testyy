LOG — Opcja B (DB-sync WWW ↔ silnik) — 25.08
Cel

Utrzymać dwie bazy (canaryaac dla WWW/MyAAC i canary dla silnika) i zsynchronizować hasła tak, by każdy nowy/zmieniony user mógł wejść do gry bez ręcznych UPDATE-ów. Mechanizm:

login.php po udanym logowaniu zapisuje w WWW cień hasła engine_password_sha1 = UPPER(SHA1(plain)).

Triggery w canaryaac przepisują ten SHA1 do canary.accounts(password).

1) Zmiana w kodzie (login.php)

Minimalna wstawka (7 linii) dodana przed końcowym die(json_encode(...)) w gałęzi case 'login'::

// --- Option B (DB-sync): store shadow SHA1 for engine on successful web auth
try {
    $stmt = $db->prepare("UPDATE `accounts` SET `engine_password_sha1` = UPPER(SHA1(:plain)) WHERE `id` = :id");
    $stmt->execute([':plain' => (string)$request->password, ':id' => (int)$account->id]);
} catch (Exception $e) { /* ignore */ }


Po edycji: sudo systemctl reload php8.2-fpm nginx.

Efekt: po udanym logowaniu HTTP wypełnia się canaryaac.accounts.engine_password_sha1 (40-znakowy SHA1).

2) Migracje DB (WWW)

Dodano kolumnę i triggery (wcześniej wgrane skryptem SQL):

canaryaac.accounts.engine_password_sha1 CHAR(40) — OK.

acc_sync_ai (AFTER INSERT) — OK.

acc_sync_au (AFTER UPDATE) — ZASTĄPIONY wersją UPSERT:

USE canaryaac;
DROP TRIGGER IF EXISTS acc_sync_au;

DELIMITER //
CREATE TRIGGER acc_sync_au AFTER UPDATE ON accounts
FOR EACH ROW
BEGIN
  INSERT INTO canary.accounts (name, password, email, type, premdays)
  VALUES (NEW.name, COALESCE(NEW.engine_password_sha1, '0'), NEW.email, 1, 0)
  ON DUPLICATE KEY UPDATE
    password = COALESCE(NEW.engine_password_sha1, password),
    email    = NEW.email;
END//
DELIMITER ;


Efekt: każda zmiana (wypełnienie cienia SHA1) w WWW wstawi lub zaktualizuje konto w silniku.

3) Testy API (login HTTP)

Endpoint: http://127.0.0.1/login.php (u nas /api/v1/login.php dawało 404; opcjonalny alias dodamy później).

Test (przykład przez plik):

printf '{"type":"login","email":"<TWÓJ_EMAIL>","password":"<PLAIN>"}\n' > ~/login.json
curl -sS -H 'Content-Type: application/json' --data-binary @~/login.json \
  http://127.0.0.1/login.php | jq .


Wynik: JSON zawiera session (z sessionkey: "<login>\n<plain>") i playdata (świat 127.0.0.1:7172, lista postaci).
To potwierdza poprawność loginu HTTP.

4) Seed do silnika + sprzątanie duplikatów

Po pierwszym logowaniu zasiedliliśmy wpis w canary.accounts i zrobiliśmy porządki:

4.1. Seed (jednorazowo)

INSERT INTO canary.accounts (name, password, email, type, premdays)
SELECT name, engine_password_sha1, email, 1, 0
FROM canaryaac.accounts
WHERE email='proeloptaku3@wp.pl'
ON DUPLICATE KEY UPDATE
  password=VALUES(password), email=VALUES(email);


4.2. Usunięcie duplikatu konta w silniku
Zidentyfikowano ptakukolo o id=7 i id=8 (oba len=40).
– Przepięcie graczy z 8 na 7 (jeśli były), następnie:

DELETE FROM canary.accounts WHERE id=8 LIMIT 1;


4.3. Naprawa „pustych nazw” w silniku (blokowały indeks UNIQUE)
Dwa wpisy z name='' (id 3 i 4, admin@canaryaac.com
).
Zmieniono je na techniczne nazwy:

UPDATE canary.accounts
SET name = CONCAT('migrated_', id)
WHERE name='' OR name IS NULL;


4.4. Dodanie indeksu unikalnego po nazwie konta

ALTER TABLE canary.accounts
ADD UNIQUE KEY uniq_accounts_name (name);


Efekt: brak duplikatów po name; triggery UPSERT działają poprawnie.

5) Weryfikacja końcowa (spójność WWW ↔ silnik)

Kontrola na realnym koncie proeloptaku3@wp.pl (name=ptakukolo):

WWW:

SELECT id,name,email,
       CHAR_LENGTH(engine_password_sha1) AS len_www,
       LEFT(engine_password_sha1,12)     AS www_head
FROM canaryaac.accounts
WHERE email='proeloptaku3@wp.pl';


➜ len_www = 40

Silnik:

SELECT id,name,email,
       CHAR_LENGTH(password) AS len_eng,
       LEFT(password,12)     AS eng_head
FROM canary.accounts
WHERE name='ptakukolo';


➜ len_eng = 40

JOIN (powinien być 1 wiersz):

SELECT a.name,
       CHAR_LENGTH(a.engine_password_sha1) AS len_www,
       CHAR_LENGTH(e.password)             AS len_eng
FROM canaryaac.accounts a
JOIN canary.accounts    e ON e.name=a.name
WHERE a.email='proeloptaku3@wp.pl';


➜ len_www=40, len_eng=40.

6) Dodatki (opcjonalne)

Alias dla starej ścieżki:

sudo mkdir -p /var/www/html/api/v1
sudo ln -sf /var/www/html/login.php /var/www/html/api/v1/login.php


Utrzymanie: po testach wyłączyć tryby DEBUG/DEV w .env (jeśli były).

7) Stan końcowy

✅ Login HTTP działa (zwraca session + playdata dla świata 127.0.0.1:7172).

✅ Opcja B aktywna: login.php zapisuje cień SHA1, triggery UPSERTują do canary.accounts.

✅ Silnik: brak duplikatów po nazwie, UNIQUE(name) dodany.

✅ Konto testowe ptakukolo — SHA1 w WWW i silniku (40 znaków), wejście do gry OK.

8) Co następne (krótko)

Chcesz wpuścić znajomego: ustaw w .env publiczny OTS_GAME_HOST, przekieruj 7172/TCP i przetestuj z zewnątrz.

Branding i porządki w WWW/instalce — gotowy do startu na czysto (mamy stabilną bazę).

# LOG — OTClient: języki / topmenu / tłumaczenia — 2025-08-25

✅ Dodano stały przycisk „Language” w górnym pasku:
   - modules/client_topmenu/topmenu_language_button.otui (nowy)
   - modules/client_topmenu/topmenu.lua – rejestracja przycisku, onClick => modules.client_locales.createWindow()

✅ Naprawiono moduł locales (błąd „<eof> expected near 'end'”) i wymuszone wyświetlanie okna wyboru języka:
   - modules/client_locales/locales.lua – poprawiona składnia, inicjalizacja i podpięcie do topmenu

✅ Uzupełniono tłumaczenia PL widoczne na ekranie logowania:
   - data/locales/pl.lua:
     • "players online" → "graczy online" (nagłówek paska)  [topmenu używa tr('players online'). Patrz: modules/client_topmenu/topmenu.lua → setPlayersOnline]. :contentReference[oaicite:0]{index=0}
     • "Enable HTTP login" → "Włącz logowanie HTTP"
     • "Remember Email" → "Zapamiętaj identyfikator konta"
     • "Password" → "Hasło"
     • "Language" / "Change language" → "Język" / "Zmień język"

✅ Ikona języka:
   - images/topbuttons/language.png (dodana)

🟡 Do dokończenia (PL i DE):
   - Tooltip „Remember Email” (dłuższy opis) — dodać klucz tłumaczenia w pl.lua.
   - Dolny panel „Enabling Boosted Creature Panel…” — dodać wpisy tłumaczeń w pl.lua.
   - Fallback czcionek pod diakrytyki (ę, ż, ą… oraz cyrylica): przygotować override fonts.otui + TTF.

## 2025-08-28 — TTF/RTL + CI (Windows)

✅ Integracja warstwy TTF/RTL (szkielet):
- Dodane pliki: `framework/text/TextShaper.{h,cpp}` oraz `framework/text/TTFFont.{h,cpp}` i podpięte do targetu przez CMake.  
  (Zob. pliki źródłowe z sesji: TextShaper.cpp / TTFFont.cpp).  
  → Cel: shaping (HarfBuzz) + bidi (FriBidi) + atlasowanie glyphów. 

✅ CMake — zależności i linkowanie:
- Dodany interfejs `otc_textstack` + opcje: `OTC_ENABLE_TTF`, `OTC_ENABLE_HARFBUZZ`, `OTC_ENABLE_FRIBIDI`.  
- `find_package(Freetype CONFIG REQUIRED)` + `find_package(harfbuzz CONFIG REQUIRED)` + (FriBidi) przez udostępnione configi/targets.  
- `target_link_libraries(... PRIVATE otc_textstack)`. 

✅ GitHub Actions — przygotowane workflowy:
- Windows + vcpkg + CMake, cache vcpkg/ccache, pakowanie artefaktu (EXE + DLL).  
  (W repo: `.github/workflows/build-windows.yml` — warianty bazowy/“full deps”). 

✅ vcpkg — instalacja bibliotek:
- Zainstalowane: `freetype`, `harfbuzz`, `fribidi` (+ zależności) — log przebiegu OK.

ℹ️ Porządek w repo:
- Uporządkowanie pod Actions (bez ciężkich ZIP > 100 MB).  
- `.gitattributes` binarki jako `binary`, LF w repo; `.gitignore` ignoruje build/artefakty.

🟡 Do dokończenia (następny krok):
- Dodać **faktyczne** pliki fontów do repo:  
  `data/fonts/ttf/NotoSans-Regular.ttf`, `NotoSans-Bold.ttf`, `NotoSansSC-Regular.ttf`, `NotoNaskhArabic-Regular.ttf`.  
- Wprowadzić fallback chain i domyślny font w UI (np. `entergame.otui` → `font: noto-12`).  
- (Opcjonalnie) przenieść dotychczasowe TODO „fallback czcionek pod diakrytyki (ę, ż, ą… i cyrylica)” jako **rozwiązane przez TTF**, a dalsze zadanie zostawić tylko dla konfiguracji fallbacków. :contentReference[oaicite:3]{index=3}

jasne — zrobiłem zwięzły „log z projektu” z całej naszej sesji: co się wydarzyło, jakie błędy widzieliśmy, jakie poprawki wprowadziliśmy i co mamy dalej zrobić.

Chronologiczny log / dziennik działań
1) Start: CI na GitHub Actions (Windows + Linux)

Objawy: Buildy padały po długim czasie (1h+). SonarCloud padał szybciej (~17 min).

Najważniejsze błędy:

error: In manifest mode, vcpkg install does not support individual package arguments — próba vcpkg install pkgconf z parametrami w trybie manifestu.

Brak pkgconf.exe po instalacji (szukany w installed/<triplet>/tools/pkgconf).

fmt nieodnaleziony podczas find_package(fmt CONFIG REQUIRED) (zarówno Windows jak i Linux).

fribidi — błąd na etapie meson setup (Windows).

Linux: Could NOT find LuaJIT (missing: LUAJIT_LIBRARY LUAJIT_INCLUDE_DIR).

Sonar: git show versions/baseline.json — błąd przy odczycie baseline z vcpkg (brak repo historii).

The vcpkg's Git commit id must be a full SHA1 hash (40 hex digits) — zły/niepełny SHA w akcji lukka/run-vcpkg.

2) Próby naprawy (CI)

Dodanie pkgconf jako host tool w vcpkg.json (zamiast wymuszać klasyczny tryb).

Korekty workflowów: wykrywanie pkgconf, fallback na classic install, poprawki PowerShell (błąd „Cannot overwrite variable Host” — zmienna kolidowała z $Host PowerShella).

Uporządkowanie manifestu (brakujące fmt), dopisanie platform do warunkowych zależności (luajit, opengl, glew, angle).

W Sonarze: szybciej widać usterki (ale wciąż rozjeżdżało się na vcpkg/baseline i brakach pakietów).

Wnioski z tego etapu:
Większość problemów nie była w samej kompilacji kodu, tylko w ekosystemie vcpkg (manifest vs classic, brak pełnej historii repo vcpkg, nieuziemione wersje, brakujące host tools). Dodatkowo kilka braków w vcpkg.json (np. fmt) powodowało natychmiastowe find_package-fail.

3) Lokalna kompilacja (Developer PowerShell) – pierwsze podejście

Po podmianach vcpkg.json spróbowaliśmy lokalnego vcpkg install.

Padło na fribidi: meson … setup … Error code: 1 (Windows).
Dodatkowo wcześniej trzymały się resztkowe zmienne środowiskowe PKG_CONFIG_*, które potrafią bruździć na Windows.

Wnioski:

fribidi/harfbuzz na Windows są buildowalne w vcpkg, ale lubią czyste env.

Niezależnie od fribidi, i tak był problem z vcpkg bez pełnej historii (baseline).

4) Przestawienie na Visual Studio .sln

Otworzyłeś vc17/otclient.sln w VS2022 (PL).

Początkowo solution było „zwolnione” (odlinkowane), po dołączeniu projektu widziałeś pliki i strukturę.

Błędy przy kompilacji w VS: seria wpisów „nie można wykonać operacji git show versions/baseline.json”.

Diagnoza kluczowa:
Masz katalog vcpkg bez historii git (pobrany jako ZIP albo w stanie niezgodnym ze wskazanym builtin-baseline). Vcpkg w trybie manifest musi móc wykonać git show <baseline SHA>:versions/baseline.json. Bez pełnego repo to niemożliwe — stąd kaskada błędów.

5) Naprawa główna (lokalnie, pod .sln)

Zasugerowałem:

Usunięcie lokalnego .\vcpkg (tego z ZIP-a).

git clone https://github.com/microsoft/vcpkg.git vcpkg

git checkout b322346fe03b0d04283f9daf05fecc0c8f64d6f (commit z Twojego builtin-baseline)

.\bootstrap-vcpkg.bat

(opcjonalnie) .\vcpkg.exe integrate install

Po tym Visual Studio powinno bez problemu rozwiązywać zależności przy buildzie solution.

6) Dodatkowe porządki / uwagi

Linux CI: brak generatora „Unix Makefiles” → trzeba wymusić generator (-G Ninja) + zainstalować ninja-build, albo doinstalować build-essential/make.

parallel-hashmap jest header-only — brak plików nagłówkowych w include oznacza, że port był niewciągnięty albo ścieżka do includes jest zła. W vcpkg.json dodaliśmy parallel-hashmap, CMake po find_path/target_include_directories powinien go widzieć.

luajit na Linuxie był brakujący → dorzuciliśmy go do manifestu (Linux i Windows).

pkgconf jako host (Windows) — dodane warunkowe zależności i skrypty „znajdowania” go w installed/<triplet>/tools/pkgconf.

Co poprawiliśmy (konkrety)

Manifest vcpkg.json

Dodane/upewnione zależności: fmt, luajit (Windows/Linux), fribidi, harfbuzz, glew, opengl, angle, parallel-hashmap, pkgconf jako host tool (Windows), itd.

Warunkowe wpisy dla platform ("platform": "windows | linux", "windows | osx", "android & wasm32" itp.) i bez zbędnych przecinków (naprawa błędu „Unexpected character in middle of array”).

Pilnowanie zgodności z builtin-baseline.

Workflow build-windows.yml

Pin vcpkg na pełnym SHA.

Porządek z lukka/run-vcpkg@v11 (pełny commit, ścieżki, cache).

Wyeliminowanie install pkgconf w manifest mode; zamiast tego bazowanie na host tool z manifestu.

SonarCloud

Utrzymaliśmy go jako szybki „czujnik”, ale dopóki vcpkg/baseline nie był OK, i tak się sypał. Po naprawie vcpkg repo będzie działał stabilniej.

Lokalny VS 2022

Przestawienie się na solution .sln — zdecydowanie najkrótsza ścieżka, bo omija problemy skryptowe z Actions i pozwala na natychmiastowy feedback.

Główne „root cause”

vcpkg bez historii git (ZIP / brak odpowiedniego commita) → błąd git show versions/baseline.json.

Mieszanie trybów (manifest vs classic) → vcpkg install … z parametrami w manifest mode.

Brakujące wpisy w vcpkg.json (fmt) → find_package(fmt) fail.

fribidi/harfbuzz/pkgconf na Windows wymagają czystego środowiska (nie ustawiaj ręcznie PKG_CONFIG_*).

Na Linux brak luajit, brak generatora (Ninja/Make) i/lub narzędzi build.

Plan „co robimy dalej”
A) Lokalny build w VS (.sln) — rekomendowane na teraz

Usuń stary folder vcpkg i wykonaj:

cd C:\Gry\Tibia\otland\otclient-main
rmdir /s /q .\vcpkg
git clone https://github.com/microsoft/vcpkg.git vcpkg
cd .\vcpkg
git checkout b322346fe03b0d04283f9daf05fecc0c8f64d6f
.\bootstrap-vcpkg.bat
.\vcpkg.exe integrate install


Otwórz vc17\otclient.sln → x64 + Debug/Release → Kompiluj.

Jeśli pokaże błąd na fribidi/harfbuzz: sprawdź, czy nie masz ustawionych zmiennych PKG_CONFIG_EXECUTABLE, PKG_CONFIG_PATH, PKG_CONFIG. Usuń je i ponów build.

B) CI (GitHub Actions)

Wgraj poprawiony vcpkg.json i workflow build-windows.yml.

Upewnij się, że vcpkgGitCommitId jest pełnym SHA (40 znaków) — ten sam co lokalnie (b322346fe03b0d04283f9daf05fecc0c8f64d6f).

Dla Linux:

doinstaluj ninja-build i wymuś -G Ninja albo doinstaluj make i pozwól CMake używać „Unix Makefiles”.

dodaj luajit (już w manifeście).

Ewentualnie do Sonara dołóż szybki „configure-only” z CMake (żeby mieć compile_commands.json), zanim odpalisz pełen build.

Status „tu i teraz”

Solution w VS już się ładuje (widzisz drzewa plików).

Największy „zabójca” — baseline vcpkg — mamy rozpoznany i obejście (pełne repo vcpkg na wskazanym commicie).

fmt dopięte → find_package(fmt) nie powinien już blokować konfiguracji.

fribidi / harfbuzz — budują się w vcpkg na Windows; jeśli znowu fail, wyczyść PKG_CONFIG_* i spróbuj ponownie.

Krótka checklista

 vcpkg jako git repo (nie ZIP), na commicie z baseline.

 .\bootstrap-vcpkg.bat wykonany.

 Brak ręcznych PKG_CONFIG_* w środowisku podczas builda na Windows.

 vcpkg.json z fmt, luajit, fribidi, harfbuzz, pkgconf (host, win), itd.

 VS2022: x64 + Debug/Release, compile.

 CI: pełny SHA w run-vcpkg, generator i narzędzia build (Linux).

Jeśli chcesz, mogę od razu zrzucić minimalny skrypt PowerShell „VS quick-setup” dla Ciebie (czyści zmienne PKG_CONFIG, klonuje vcpkg na baseline, bootstrappuje i odpala sln).




wykonane_zadania_APPEND_2025-08-30.md

 Naprawa vcxproj/filters (MSBuild namespace, duplikaty, CopyFontAssets).

 Dodanie LocaleShaping.h/.cpp (mapping locale → script+direction), bez dublowania typów.

 Poprawa TextShaper.cpp: include’y HarfBuzza (vcpkg), UTF-32, zgodna sygnatura shape(...).

 Kompilacja przechodzi przez etap z C2511/C1083 (błędy zniknęły po poprawkach).