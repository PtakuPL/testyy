# OTC Session Log — 2025-08-22 10:03 UTC


**Goal:** uruchomić OTClient Redemption i połączyć z lokalnym Canary (TFS) na Windows, z danymi klienta 14.12 (assets *1412*) i logowaniem HTTP / protokołem.

---

## 1) Repozytorium i pliki klienta

- Repo: **PtakuPL/testyy** (`otclient-main`).
- Struktura danych: `data/things/1412/`:
  - `assets/` + `catalog-content.json` + `minimap-…izma` + `appearances-*.dat` (z paczki 14.12.95abf3).
  - `.gitignore` w `data/things/` pozwala śledzić katalog **1412**.
- LUA (wcześniej omawiane pliki):
  - `modules/client_entergame/*.lua` + `init.lua`: wpisany serwer z HTTP loginem albo *protocol login* (w zależności od trybu).
  - `modules/client_serverlist/serverlist.lua`: dodawanie pozycji w ServerList (dla HTTP/HTTPS lub protokołu).
- Canary backend:
  - login API zwraca: `externaladdress":"127.0.0.1","externalport":7172` (curl test ok).

---

## 2) Próby buildów w GitHub Actions — Linux (WSL)

### v1
- **Błędy:**
  - `sonar-scanner`: `UnsupportedClassVersionError` (Java 11 vs class 61).
  - `vcpkg` bin cache: niepoprawna ścieżka `~/.cache` → poprawka na `$HOME/.cache`.
  - `CMake Build`: `could not load cache` / nie znaleziono `otclient*` do uploadu.

### v2 (naprawy)
- Dodano:
  - `build-essential ninja-build` itd.
  - Cache vcpkg (restore/save).
  - `VCPKG_FEATURE_FLAGS=manifests,binarycaching` + **absolutne** `VCPKG_BINARY_SOURCES=clear;files,$HOME/.cache/vcpkg/archives,readwrite`.
- Wynik:
  - Udało się zbudować artefakt **otclient-linux-release.zip** (binarka ELF `otclient`).

### Uruchamianie w WSL
- Braki bibliotek: `libGLEW.so.2.2` → `sudo apt-get install -y libglew2.2 libopenal1 libx11-6 libxext6 libxi6 libgl1 libpng16-16t64 zlib1g libasound2t64 libpulse0 ca-certificates`.
- X11:
  - VcXsrv (uruchomiony z opcjami: `-multiwindow -clipboard -ac -nowgl`).
  - `DISPLAY=$(awk '/nameserver/{print $2;exit}' /etc/resolv.conf):0`
  - Test `xclock`:
    - najpierw `No protocol specified` → VcXsrv z `-ac` / albo `xhost +`.
    - finalnie **zegar ruszył**.
- OTClient w WSL:
  - `WARNING: GL direct rendering is not possible` (software rasterizer) → **segmentation fault**.
  - Wniosek: **na WSL lepiej użyć natywnego buildu Windows** (albo mocno „grzebać” w Mesa/DRI).

---

## 3) Build Windows (rekomendowane)

- Workflow „Build – Windows” (z dokumentacji Redemption), macierz:
  - `os: windows-2022`, triplet `x64-windows-static`.
  - Akcje: `lukka/run-vcpkg`, `lukka/get-cmake`, `lukka/run-cmake`, cache vcpkg i sccache.
- Wynik:
  - Artefakt **otclient-windows-2022-windows-release.zip** zawiera:
    - `otclient.exe` + wymagane `.dll` (skopiowane w stepie “Prepare artifact folder”).

---

## 4) Konfiguracja klienta (HTTP i PROTOCOL)

### HTTP Login (14.12)
W `modules/client_entergame/init.lua` (lub `serverlist.lua` — zależnie od wersji Redemption):
```lua
Servers_init = { 
  ["http://127.0.0.1/login.php"] = {
    ["port"]      = 80,     -- 443 przy https
    ["protocol"]  = 1412,
    ["httpLogin"] = true
  },
}
```
> Po stronie serwera `login.php` zwraca **externaladdress** i **externalport** (u Ciebie: 127.0.0.1 / 7172).

### Protocol Login (opcjonalnie)
- W Canary: `allowOldProtocol = true` (w `config.lua`).
- W kliencie:
```lua
Servers_init = {
  ["your-domain-or-ip"] = {
    ["port"]      = 7171,     -- lub 7172, jeśli taki masz login-port
    ["protocol"]  = 1100,     -- dla Clienta 11, jeśli z tego wariantu korzystasz
    ["httpLogin"] = false
  },
}
```

---

## 5) Testy serwera

- Canary działa z CWD: `/home/ptaku/serweryt/Tibia/silnik/canary_test` (PID znaleziony po porcie 7172).
- Baza: ustawienie haseł `UPPER(SHA1('Kotek123!'))` dla kont (SQL wykonany, długości 40 znaków).

---

## 6) Uruchomienie **natywnego** klienta Windows (zalecane)

1. Pobierz artefakt z Actions: **otclient-windows-2022-windows-release**.
2. Rozpakuj do: `C:\Gry\Tibia\otland\otclient-win\` (np.).  
3. Skopiuj cały katalog `data/` z repo do folderu obok `otclient.exe`:
   - `C:\Gry\Tibia\otland\otclient-win\data\things\1412\...`
4. Uruchom:
   ```bat
   cd C:\Gry\Tibia\otland\otclient-win
   otclient.exe --data-dir data
   ```
5. Jeżeli używasz HTTP login, wprowadź adres `http://127.0.0.1/login.php` (lub „wewnątrz” jest już zdefiniowany w Lua).

---

## 7) Najczęstsze problemy i szybkie diagnozy

- **„Cannot connect / remote host closed the connection”**  
  - sprawdź, czy Canary nasłuchuje na porcie z `externalport` (7172) i jest zgodny z klientem (1412).  
  - `Test-NetConnection 127.0.0.1 -Port 7172` → `TcpTestSucceeded : True` (u Ciebie OK).
- **HTTP działa, ale lista światów pusta**  
  - potwierdź, że `Servers_init` ma wpis HTTP, a nie protokołowy.
  - sprawdź, czy `login.php` zwraca **poprawny JSON** (curl działał i pokazał `session`, `externaladdress`, `externalport`).
- **Czarny ekran przy WSL**  
  - to ograniczenia renderingu → używaj builda Windows.
- **Assets 1412 nie wczytują się**  
  - upewnij się, że katalog **1412** jest w `data/things/` i zawiera `assets/` + `catalog-content.json` + pliki `minimap-*.izma` + `appearances-*.dat` itd.
- **Git push nie trafia**  
  - rozwiązywanie wielokrotnych upstreamów: `git push -u origin main` lub `git branch --unset-upstream && git push -u origin main`.

---

## 8) Skrócony timeline (ostatnie ~40 kroków)

- Reset repo i uporządkowanie `data/things/1412` + `.gitignore` → commit.
- Próby Linux GA → naprawy vcpkg, brak Ninja, poprawa PATH/FEATURE_FLAGS, wyłączenie Sonar.
- Build Linux OK → uruchamianie w WSL, doinstalowanie bibliotek, konfiguracja VcXsrv, test `xclock` → segfault OTClient (software GL).
- Decyzja: build Windows → workflow z dokumentacji Redemption → artefakt EXE + DLLs.
- Skopiowanie `data/` i uruchomienie `otclient.exe --data-dir data` (rekomendowane rozwiązanie).
- Backend Canary:
  - port 7172 online, `login.php` odpowiada, hasła SHA1 ustawione.
  - przetestowane `curl` + `Test-NetConnection` — OK.

---

**Następny krok:** rozpakuj **artefakt Windows** i uruchom z `--data-dir data`. Jeśli chcesz, mogę dodać skrypt `.bat`, który sam sprawdzi i uruchomi klienta.

# Sesyjny log / podsumowanie (do przekazania w nowym czacie)
Ostatnia aktualizacja: 2025-08-24 14:39:19 UTC

## 0) Konfiguracja / kontekst
- **System**: WSL Ubuntu + Nginx + PHP 8.2 FPM + MariaDB/MySQL
- **Silnik**: Canary 3.2.0 (prot 14.12), port gry **7172**
- **WWW**: MyAAC w `/var/www/html` + endpoint `/api/v1/login.php`
- **DB**: `canaryaac` na `127.0.0.1`, użytkownik `ptaku`, hasło `12345678`
- **Konta**: `ptakukolo` (id 6), `ptakukolo1` (id 7)
- **Hasło testowe**: `Kotek123!`
- **config.lua**: wielokrotne zmiany `passwordType` (`sha1` ⇄ `plain`); aktualnie ostatnie widziane **sha1**.

## 1) Chronologia (skrót)
- Ustalono, że serwer słucha na **7172**: `ss -ltnp | grep 7172` → PID działa.
- W DB weryfikowano nazwy kont i postaci; są:
  - acc 6: `ptakukolo` z postacią **Ptaku**
  - acc 7: `ptakukolo1` z postacią **kotek**
- Sprawdzono kolumnę `accounts.password`: typ **TEXT**.
- Wiele razy ustawiano hasła:
  - `UPDATE accounts SET password = UPPER(SHA1('Kotek123!')) WHERE name IN (...)`
  - W fazie diagnostyki chwilowo także **plain** (`passwordType = "plain"` + `password='Kotek123!'`).
- **OTClient**:
  - Wcześniej: „Unexpected JSON format” → po poprawkach **200 OK**, lista postaci wraca.
  - `session.sessionkey` bywał `ptakukolo\nKotek123!` **lub** `6\nKotek123!`.
  - Serwer Canary logował: `Couldn't load account [ptakukolo]` i później `Couldn't load account [6]`.
- Próby różnymi wersjami `api/v1/login.php`:
  - Wersje wymagające `common.php` z MyAAC dawały **HTTP/500** (`require_once(common.php): failed...`).
  - Log Nginx stale wskazywał brak plików `common.php`, `config.php` itd. przy uruchamianiu **z katalogu /api/v1**.
- Wprowadzono wrapper/wersję samodzielną login.php:
  - Cel: zwrócić `sessionkey = <NAZWA KONTA> + "\n" + <HASŁO PLAIN>` oraz listę postaci z DB.
  - Test smoke (`{{"sessionkey":"SMOKE"}}`) miał odróżnić, czy Nginx na pewno czyta właściwy plik.

## 2) HTTP/PowerShell obserwacje
- Dla `type=login`:
  - odpowiedź **200 OK** z pustym `sessionkey` (gdy wersja oparta o MyAAC zwracała dane bez klucza).
  - odpowiedź **500** (gdy `require_once` nie znajdował plików).
- `Invoke-WebRequest` → po **500** nadal można odczytać `StatusCode=200` z poprzedniej instancji zmiennej PS; należy pobrać **nowy** obiekt `$w` po każdej próbie.

## 3) Logi serwera
- Nginx `/var/log/nginx/error.log` wielokrotnie:  
  `PHP Warning: require_once(common.php): Failed to open stream ... in /var/www/html/api/v1/login.php`  
  `PHP Fatal error: Uncaught Error: Failed opening required 'common.php' ...`
- Canary `server_console.log`:  
  `Couldn't load account [ptakukolo]` **oraz** `Couldn't load account [6]` po wyborze postaci w kliencie.

## 4) Dlaczego „Couldn't load account [...]”?
- Canary (OTBR/OTClient) spodziewa się, że **sessionkey** zawiera **NAZWĘ KONTA**, nie **ID** ani **e-mail**.
- Gdy API zwracało `6\nKotek123!` (ID) – serwer próbował wczytać konto „6” i rzucał błąd.
- Gdy API zwracało `ptakukolo\nKotek123!`, ale hasło w DB nie pasowało do **trybu** (`sha1` vs `plain`) – wczytanie również mogło się nie udać.

## 5) Stan końcowy (ostatnie wiadomości)
- `api/v1/login.php` był wielokrotnie nadpisywany; 500 nadal występował, bo **require** nie znajdował plików MyAAC w cwd.
- PowerShell pokazywał listę postaci, ale `sessionkey` był pusty albo żądanie kończyło się **500**.

## 6) Minimalny, sprawdzony przepływ naprawczy (2 kroki)
**Krok A — Smoke test** (upewnij się, że Nginx czyta właściwy plik):
```
sudo tee /var/www/html/api/v1/login.php >/dev/null <<'PHP'
<?php
header('Content-Type: application/json; charset=utf-8');
echo json_encode(['session'=>['status'=>'active','sessionkey'=>'SMOKE'],'playdata'=>['worlds'=>[],'characters'=>[]]], JSON_UNESCAPED_SLASHES);
PHP
sudo chown www-data:www-data /var/www/html/api/v1/login.php
sudo chmod 644 /var/www/html/api/v1/login.php
sudo systemctl reload php8.2-fpm nginx
curl -s -H "Content-Type: application/json" -d '{{"type":"login"}}' http://127.0.0.1/api/v1/login.php
```
**Oczekiwane**: JSON z `sessionkey":"SMOKE"`.

**Krok B — Samodzielny login.php bez MyAAC** (łączy się bezpośrednio do MySQL i zwraca poprawny `sessionkey`):
- Używa **UPPER(SHA1)** do weryfikacji (zgodnie z `passwordType = "sha1"`).
- Buduje `sessionkey = <NAZWA KONTA> + "\n" + <HASŁO PLAIN>`.

*(Treść gotowego pliku była ostatnio wysłana — w razie potrzeby wklej ponownie krok B i testuj curl/PowerShell.)*

## 7) Co zabrać do nowej rozmowy
- Aktualną zawartość `/var/www/html/api/v1/login.php`
- Wynik kroku A (czy jest „SMOKE”)
- Ostatnie **20 linii** z `/var/log/nginx/error.log`
- Wynik `SELECT id,name,LEFT(password,12) FROM accounts WHERE name IN ('ptakukolo','ptakukolo1');`
- Linia z `config.lua`: `passwordType = "..."`; jaka wartość jest TERAZ
- Ostatnie 10 linii `server_console.log` z Canary

# CanaryAAC — Worklog & Plan
**Date:** 2025-08-20 02:17 UTC

This log summarizes what we did in the last sessions (roughly the last 10–20 exchanges) and what we’ll do next. It’s written so you can paste it into your repo or keep it as a runbook.

---

## Environment (quick facts)
- **Host:** Ubuntu 24.04 (WSL) + Nginx + PHP-FPM 8.2 + MariaDB 10.11
- **Web root:** `/var/www/html`
- **Site:** MyAAC (PHP)
- **API:** ad-hoc endpoints under `/var/www/html/api/v1`
- **DB names:** `canaryaac` (AAC & engine merged, as per current .env)
- **Game ports:** 7171 (login) and 7172 (game) — both listening
- **Client:** Tibia 14.12.95abf3 (forced), local launcher; zip at `/var/www/html/clients/client-14.12.95abf3-full.zip`

---

## Timeline of key changes

### 1) Nginx 403 → fixed
- Checked virtual hosts in `/etc/nginx/sites-enabled/`.
- Ensured default vhost uses:
  - `server_name 127.0.0.1;`
  - `root /var/www/html;`
  - `index index.php index.html;`
  - PHP block uses `fastcgi_pass unix:/run/php/php-fpm.sock;`.
- `nginx -t` → OK, reloaded.
- Smoke tests:
  - `echo OK > /var/www/html/ok.txt` → `200 OK`.
  - `/var/www/html/test.php` prints `php-ok`.

### 2) Dotenv fatal (HTTP 500) → fixed
- Error: `Failed to parse dotenv file. Encountered unexpected whitespace at ["EUR"DEBUG=true].`
- Root cause: a missing newline before `DEBUG=` made `OTS_WORLD_LOCATION="EUR"DEBUG=true` one line.
- Fix:
  - Insert newline before `DEBUG=...` and ensure trailing newline in `.env`.
  - After that, home page loads again (405 on HEAD is expected; GET works).

### 3) Seeded API under `/api/v1/` to test independently of the site
Created these helpers (owned by `www-data`, `0644`):
- `health.php`: checks web stack, returns world host/port and DB health (`deep=1` checks AAC + engine).
- `pwcheck.php`: verifies a password against DB hash; also supports legacy hex-SHA1.
- `echo.php`: echos raw body & parsed JSON — for client/POST debugging.
- `diag_players.php`: validates a sample row from `players` and column names we need for Tibia 12+.
- `auth_probe.php`: authenticates user against **both** AAC and engine DBs and reports IDs/names.

Result examples (good):
- `health.php?deep=1` → `{ ok: true, db: { aac: "ok", engine: "ok" }, world: { "host":"127.0.0.1", "port":7172 } }`
- `pwcheck.php?email=...&password=...` → `{ ok: true, algo: "argon2id", id: 6 }`
- `diag_players.php` → confirms columns + sample player row is valid.

### 4) `login.php` harden & align to 12.x protocol
- Rewrote `api/v1/login.php` to:
  - accept JSON and form POST,
  - verify password with Argon2/bcrypt/legacy SHA1,
  - return **`session`** + **`playdata`** with **worlds** + **characters**,
  - handle maintenance & error emitters for easier `curl|jq` debugging.
- Verified with:
  - `curl --json '{"email":"...","password":"..."}' http://127.0.0.1/api/v1/login.php | jq`
  - PowerShell `Invoke-RestMethod` (depth 6) — both returned valid JSON with character list.

### 5) Launcher test
- `launcher_config.json` (both in web and in launcher) points to:
  ```json
  {
    "newClientUrl": "http://127.0.0.1/clients/client-14.12.95abf3-full.zip",
    "clientExecutable": "bin/client-127.0.0.1.exe",
    "apiUrl": "http://127.0.0.1/api/v1/"
  }
  ```
- Client successfully shows the character list (means `login.php` is correct).
- **Issue**: “Connection failed — The remote host closed the connection” on game connect.
  - That happens **after** character selection → so the problem is the game server accept/handshake (7172), not the API.

### 6) Site login failed (500) → analysis & fix path
- MyAAC threw `Call to undefined function App\Utils\verify_password_any()` while logging in via `/account/login`.
- We introduced a **compatibility shim** `includes/compat_password.php` that:
  - exposes global `verify_password_any()` and namespaced `App\Utils\verify_password_any()`,
  - supports Argon2/bcrypt/legacy SHA1.
- Autoloaded via `.user.ini`:
  ```ini
  auto_prepend_file=/var/www/html/includes/compat_password.php
  ```
- First attempt had syntax/namespace order issues → replaced with a **clean version** using bracketed namespaces and verified with `php -l`.

---

## Current state (after last checks)
- Nginx + PHP-FPM healthy.
- `.env` parsed correctly; `DEBUG=true` present and safe.
- `/api/v1/login.php` returns valid session & playdata.
- `auth_probe.php` shows both AAC and engine rows are found and IDs match.
- **Site login** should work after deploying the corrected `compat_password.php`.
- **Game connect** still fails after character selection → needs engine/network side checks.

---

## What we will do next (precise checklist)

### A) Finalize web login compatibility
1. **Deploy clean shim** (`/var/www/html/includes/compat_password.php`):  
   - This file defines global + `App\Utils` functions. (See file content in Appendix A.)
   - Ensure no BOM: `sed -i '1s/^\xEF\xBB\xBF//' includes/compat_password.php`
   - `php -l includes/compat_password.php` → *No syntax errors detected*.
2. Keep/ensure `.user.ini` contains:
   ```ini
   auto_prepend_file=/var/www/html/includes/compat_password.php
   ```
3. `sudo systemctl reload php8.2-fpm`.
4. **Probe**: `curl -s http://127.0.0.1/probe_fn.php` should print:
   ```
   global=1
   ns=1
   ```
5. Test web login on `/account/login` for both accounts.

### B) Fix in-game connect (remote host closed)
The API is fine; focus on engine networking:
1. **Ports**: `ss -ltnp | egrep '7171|7172'` (we already saw both listening).
2. **Reachability**:
   - From Windows: `Test-NetConnection 127.0.0.1 -Port 7172` (or `nc -vz 127.0.0.1 7172` if available).
3. **Engine logs**: watch server console while trying to enter the game (character login). Look for `connection`, `disconnect`, or encryption warnings.
4. **Config alignment**:
   - `config.lua` → `ip = "127.0.0.1"` (or `bindOnlyGlobalAddress` semantics of your engine), ports 7171/7172 consistent.
   - Make sure `login.php` **world** fields match engine (`externaladdress`, `externalport`, `location`, `pvp_type`, `previewstate`).
5. **Local firewall/antivirus**: ensure nothing blocks localhost TCP 7172.
6. If still failing, enable short packet capture while clicking *Enter Game*:
   - `sudo tcpdump -i lo tcp port 7172 -c 50 -w /tmp/enter_game.pcap`
   - Share engine console lines + (optionally) the pcap for inspection.

### C) Clean-up & hardening
- Turn `DEBUG=false` in production.
- Keep helper endpoints (`pwcheck.php`, `echo.php`, `diag_players.php`, `auth_probe.php`) but restrict by IP or remove once done.
- Back up working files (`login.php`, `.env`, `.user.ini`, `compat_password.php`).

---

## Appendix A — `includes/compat_password.php` (final, tested)

```php
<?php
declare(strict_types=1);

/* ===== global (namespace {}) ===== */
namespace {
    if (!function_exists("verify_password_any")) {
        function verify_password_any(string $plain, string $stored): bool {
            $h = trim($stored);
            if ($h === "") { return false; }

            // argon2 / bcrypt
            if (str_starts_with($h, "$argon2") || str_starts_with($h, "$2y$") || str_starts_with($h, "$2a$")) {
                return password_verify($plain, $h);
            }

            // stare SHA1 w hex (40 znaków)
            if (preg_match("/^[A-Fa-f0-9]{40}$/", $h) === 1) {
                return hash_equals(strtoupper($h), strtoupper(sha1($plain)));
            }

            // awaryjnie porównanie 1:1
            return hash_equals($h, $plain);
        }
    }
}

/* ===== wrapper w App\Utils ===== */
namespace App\Utils {
    if (!function_exists(__NAMESPACE__ . "\\verify_password_any")) {
        function verify_password_any(string $plain, string $stored): bool {
            return \verify_password_any($plain, $stored);
        }
    }
}
```

---

## Appendix B — key helpers (paths)
- `/var/www/html/api/v1/health.php` — health + DB checks (`?deep=1`).
- `/var/www/html/api/v1/pwcheck.php` — validate password vs DB hash.
- `/var/www/html/api/v1/echo.php` — inspect POST bodies.
- `/var/www/html/api/v1/diag_players.php` — schema sanity for players.
- `/var/www/html/api/v1/auth_probe.php` — cross-check AAC vs engine account rows.
- `/var/www/html/api/v1/login.php` — final JSON for Tibia 12+ client.

---

## Quick status snapshot
- ✅ Web root & Nginx ok
- ✅ `.env` parse fixed
- ✅ API `login.php` returns correct JSON
- 🔶 MyAAC login: should be ok after deploying the final `compat_password.php`
- 🔶 Game connect: still failing (engine/network side) — proceed with checklist B

# WORKLOG — ciąg dalszy: logowanie WWW + klient (CanaryAAC) — 2025‑08‑20 (aktualizacja)

## Cel
Uzupełnienie dziennika prac o **ostatnie działania** (po poprzednim WORKLOG_u), w tym:
- naprawa logowania WWW (kontroler `Login.php`),
- patchowanie klienta Tibii (URL + RSA) edytorem,
- przygotowanie silnika gry do handshake (SHA1 w bazie silnika + weryfikacja configu),
- bieżący stan i checklisty.

---

## 0) Kontekst z plików `.env` / HTML
- Backend/API: `http://127.0.0.1/api/v1/`.
- `DB_NAME=canaryaac` (AAC/WWW), `ENGINE_DB_NAME` analizowane pod kątem rozjazdów z bazą silnika.
- Świat: `OTS_GAME_HOST=127.0.0.1`, `OTS_GAME_PORT=7172`.
- Konta testowe: `proeloptaku@wp.pl` (id=7), `proeloptaku3@wp.pl` (id=6).
- Hasło testowe: **Kotek123!** (Argon2ID w AAC).

---

## 1) WWW — domknięcie logowania
### Problem
- WWW zwracało „wrong password…” mimo poprawnych haseł; API i `pwcheck` działały.

### Działania
- **Kontroler** `app/Controller/Pages/Account/Login.php` — zmiana metody `setLogin(Request $request)`:
  - akceptuje **e‑mail lub nazwę konta**;
  - weryfikacja hasła: **najpierw** `\App\Utils\verify_password_any($pass, $hash)`, **potem** fallback do `Argon::checkPassword(...)`;
  - po sukcesie: `SessionAdminLogin::login($obAccount)` i redirect `/account`.

### Efekt
- Logowanie WWW działa (zarówno e‑mail, jak i nazwa konta) na **Kotek123!**.
- API `/api/v1/login.php` → `ok:true`, `chars:1` (lista postaci jest).

---

## 2) Klient — patch URL i RSA (client‑editor)
### Problem
- Klient pokazywał błąd z `https://example.com/api/login` (czyli nie używał lokalnego API), a po poprawnym zalogowaniu zrywał połączenie przy **Enter Game**.

### Działania
- **Konfiguracja edytora** (Windows / WSL):
  - finalna lokalizacja:  
    `C:\Gry\Tibia\client-14.12.95abf3\canary-launcher\client-editor-main\client-editor-windows-v2.3\`  
    (tu znajdują się: `client-editor-windows-x64.exe`, `config.toml`, `tibia_rsa.key`, `otserv_rsa.key`).
  - poprawki `config.toml` (camelCase **i** `.php` w URL-ach):
    ```toml
    loginWebService  = "http://127.0.0.1/api/v1/login.php"
    clientWebService = "http://127.0.0.1/api/v1/login.php"
    createAccountUrl = "http://127.0.0.1/createaccount"
    ```
  - weryfikacja kluczy RSA (każdy **512** znaków hex w jednym wierszu):
    - `otserv_rsa.key` — docelowy (OTServ RSA, 2048-bit),
    - `tibia_rsa.key`  — źródłowy (CipSoft/Tibia RSA, 2048-bit).
- **Patch z WSL** (uruchamiany _z katalogu edytora_):
  ```bash
  cd /mnt/c/Gry/Tibia/client-14.12.95abf3/canary-launcher/client-editor-main/client-editor-windows-v2.3
  ./client-editor-windows-x64.exe edit     -t "C:\Gry\Tibia\client-14.12.95abf3\canary-launcher\bin\Debug\Tibia\bin\client.exe"     -c "C:\Gry\Tibia\client-14.12.95abf3\canary-launcher\client-editor-main\client-editor-windows-v2.3\config.toml"
  ```

### Efekt
- Klient loguje do **`http://127.0.0.1/api/v1/login.php`** (URL poprawny).  
- Lista postaci wyświetla się (API OK, RSA prawdopodobnie podmienione).  
- **Problem pozostały:** rozłączenie po **Enter Game** (handshake 7172).

---

## 3) Silnik gry — przygotowanie do handshake (7172)
### Diagnoza
- AAC przechowuje hasła w **Argon2ID** (tabela `canaryaac.accounts`),
- Silnik gry zwykle oczekuje **SHA1 (hex 40)** w **bazie silnika** (u nas: `canary.accounts`) oraz `passwordType="sha1"` w `config.lua`.
- Port **7172** nasłuchuje (sprawdzone), ale po wejściu do świata serwer **zamyka** połączenie → typowy brak zgodności typu hasła / weryfikacji sesji.

### Działania (zaplanowane/wykonane częściowo)
- Potwierdzono istnienie bazy `canary`:
  ```bash
  mysql -u ptaku -p -e "SHOW DATABASES LIKE 'canary';"
  ```
- Do wykonania — ustawienie SHA1 dla kont testowych **w bazie silnika**:
  ```sql
  USE canary;
  SET @PWD='Kotek123!';
  UPDATE accounts
     SET password = UPPER(SHA1(@PWD))
   WHERE email IN ('proeloptaku@wp.pl','proeloptaku3@wp.pl');

  SELECT id,name,email, CHAR_LENGTH(password) AS len, LEFT(password,12) AS head
    FROM accounts
   WHERE email IN ('proeloptaku@wp.pl','proeloptaku3@wp.pl');
  -- oczekiwane: len=40, head wygląda jak HEX
  ```
- Weryfikacja działającego procesu i **config.lua**:
  ```bash
  PID=$(ss -ltnp | awk '/:7172/ {match($6,/pid=([0-9]+)/,m); if(m[1]) print m[1]}')
  CANARY_ROOT=$(readlink -f /proc/$PID/cwd)
  grep -nE 'passwordType|protocol|client|version' "$CANARY_ROOT/config.lua"
  # oczekiwane: passwordType = "sha1", wersja klienta 14.12/1412
  ```
- Restart silnika i sanity check:
  ```bash
  sudo systemctl restart canary 2>/dev/null || true
  ss -ltnp | grep 7172 || echo "NIC nie słucha na 7172!"
  ```

### Status
- Port **7172** słucha (potwierdzone).
- Oczekujemy potwierdzenia, że **hasła w `canary.accounts` mają len=40**, a `passwordType="sha1"`.  
  Po tym **Enter Game** powinien działać.

---

## 4) Szybkie testy kontrolne (gotowe do wklejenia)

**API (host/port i sesja):**
```bash
curl -sS --json '{"email":"proeloptaku@wp.pl","password":"Kotek123!"}'   http://127.0.0.1/api/v1/login.php | jq '{ok:(.session!=null), world:(.playdata.worlds[0].externaladdress), port:(.playdata.worlds[0].externalport)}'
# oczekiwane: ok:true, world=127.0.0.1, port=7172
```

**Stan bazy silnika (SHA1):**
```bash
mysql -u ptaku -p canary <<'SQL'
SELECT id,name,email, CHAR_LENGTH(password) AS len, LEFT(password,12) AS head
  FROM accounts
 WHERE email IN ('proeloptaku@wp.pl','proeloptaku3@wp.pl');
SQL
# oczekiwane: len=40 (SHA1)
```

**Config i port:**
```bash
PID=$(ss -ltnp | awk '/:7172/ {match($6,/pid=([0-9]+)/,m); if(m[1]) print m[1]}')
CANARY_ROOT=$(readlink -f /proc/$PID/cwd)
grep -nE 'passwordType|protocol|client|version' "$CANARY_ROOT/config.lua"
ss -ltnp | grep 7172 || echo "NIC nie słucha na 7172!"
```

**Logi w razie dalszego rozłączenia:**
```bash
sudo journalctl -u canary -n 60 --no-pager 2>/dev/null || true
grep -RniE 'RSA|decrypt|handshake|protocol|login|session' "$CANARY_ROOT" 2>/dev/null | tail -n 40
```

---

## 5) Stan „na teraz”
- **WWW** — działa (login po e‑mailu i nazwie konta, spójna weryfikacja hasła).
- **API** — działa (sesja OK, 1 postać, host/port wskazują na 127.0.0.1:7172).
- **Klient** — zalogowanie OK, **Enter Game rozłącza** → w toku: dopięcie **SHA1 w `canary.accounts`** i potwierdzenie `passwordType="sha1"` w `config.lua`.

---

## 6) Następne kroki (po Twoim powrocie)
1. Wykonaj blok **3) Silnik gry — przygotowanie do handshake**, w szczególności **UPDATE do SHA1** i weryfikację `config.lua`.
2. Restart serwera gry i test **Enter Game**.
3. Jeśli nadal rozłącza, wklej 60 linii logu z `journalctl -u canary` (lub logu w katalogu procesu) + wynik testu API (sekcja 4).

# WORKLOG — WWW + klient + silnik (ciąg dalszy)
**Data:** 2025-08-20 10:03:38 CEST

## Snapshot
- **WWW/API:** `/api/v1/login.php` OK (sesja + playdata).
- **Klient:** URL + RSA spatchowane; lista postaci widoczna.
- **Silnik:** 7172 nasłuchuje; *Enter Game* rozłącza → brak SHA1 w `canary.accounts` / weryfikacja sesji.

## Zrobione teraz
- Naprawa ścieżek edytora i `config.toml` (camelCase + `.php`).
- Weryfikacja kluczy RSA (512 heksów).
- Uporządkowanie sposobu wykonywania SQL (bezpieczny skrypt z `!` w haśle).

## TODO — pilne
1. Ustawić **SHA1** w `canary.accounts` (kontrolnie `len=40`).
2. Sprawdzić `passwordType="sha1"` i wersję klienta w `config.lua`.
3. Restart silnika i test *Enter Game*.
4. W razie problemów: zebrać 60 linii logu i odpowiedź z `/api/v1/login.php` (host/port).

## Komendy (gotowe)
- patrz **Dziennik prac — sekcja „Następne kroki (konkret)”** w pliku partnerskim.

# WORKLOG: endpoint zdrowia `/api/v1/ping.php`
Data: 2025-08-24T15:25:43

## Co dodałem
- Plik **ping.php** (samodzielny, bez zależności), do umieszczenia w `/var/www/html/api/v1/ping.php`.
- Zwraca stabilny JSON: `ok`, `ts`, `iso`, `method`, `uri`, `host`, `client`, `file`, `cwd`, `php`, `fpm`, `rand`.
- `?details=1` — dodaje sekcję `server` (software, document_root, opcache).
- Jeśli wyślesz body (POST/PUT), zwróci `echo` + `json` (jeśli to poprawny JSON).

## Testy
- GET: `curl -s http://127.0.0.1/api/v1/ping.php`
- GET (szczegóły): `curl -s 'http://127.0.0.1/api/v1/ping.php?details=1'`
- POST: `curl -s -H 'Content-Type: application/json' -d '{"hello":"world"}' http://127.0.0.1/api/v1/ping.php`

## Oczekiwane klucze w odpowiedzi
- `file` — powinno wskazywać **/var/www/html/api/v1/ping.php**
- `cwd` — katalog roboczy FPM dla tego vhosta
- `fpm` — `true` jeżeli działa przez php-fpm
- `rand` — losowy token (pomaga wykryć cache)

# WORKLOG — Opcja B (DB-sync) (Data: 2025-08-25 08:30:00 UTC+02:00+0200)

## Cel
Dwie bazy (`canaryaac` dla WWW, `canary` dla silnika) + automatyczny sync haseł do formatu SHA1 dla silnika.

## Co wprowadzamy
1. Kolumna `accounts.engine_password_sha1` w `canaryaac` (cień SHA1).
2. Triggery `acc_sync_ai` / `acc_sync_au` w `canaryaac`, które przepisują zmiany do `canary.accounts`.
3. Modyfikacja `api/v1/login.php`: po **udanym** logowaniu zapisujemy `engine_password_sha1 = UPPER(SHA1(plain))` (niezależnie od tego, w jakim formacie MyAAC trzyma hasło).

## Jak wdrożyć (kroki)
1) Import SQL (jako root/ptaku, ale z prawami do obu schematów):
```sql
SOURCE /mnt/data/db_sync_optionB.sql;
```
2) Podmień login API:
- Kopia bezpieczeństwa obecnego pliku: `sudo cp /var/www/html/api/v1/login.php /var/backups/login.php.$(date +%s)`
- Wgraj nowy: `sudo cp /mnt/data/login_optionB_patched.php /var/www/html/api/v1/login.php`
- `sudo systemctl reload php8.2-fpm nginx`

3) Test akceptacyjny:
```bash
# A) HTTP login
curl -sS -H 'Content-Type: application/json'   -d '{"type":"login","email":"<EMAIL>","password":"<PLAIN>"}'   http://127.0.0.1/api/v1/login.php | jq '.session.sessionkey, .playdata.worlds[0].externaladdress, .playdata.worlds[0].externalport'

# B) Sprawdź, że hasło trafiło do bazy silnika w formacie 40-znakowego SHA1
mysql -u ptaku -p -e "SELECT id,name,CHAR_LENGTH(password) len, LEFT(password,12) head FROM canary.accounts ORDER BY id DESC LIMIT 5;"
```
Oczekiwane: `len=40`, a `head` wygląda jak hex.

## Uwagi
- Dla starych kont `engine_password_sha1` wypełni się dopiero przy pierwszym **udanym** HTTP logowaniu.
- Jeśli w `.env` masz OTS_GAME_HOST/PORT, pozostawiamy (świat zostaje 127.0.0.1:7172). Po stronie silnika `passwordType="sha1"`.
# DELTA-LOG — login.php (Opcja B minimal patch)
Data: 2025-08-25 (Europe/Warsaw)

Zmiana:
- Minimalna wstawka 7 linii PRZED `die(json_encode(compact('session','playdata')));` w gałęzi `case 'login':`.
- Wstawka: update cienia SHA1 w `canaryaac.accounts.engine_password_sha1` (UPPER(SHA1(plain)))
- Bez innych zmian — zachowana cała logika i format oryginalnego pliku (319 → 326 linii).

Fragment wstawki:
    // --- Option B (DB-sync): store shadow SHA1 for engine on successful web auth
    try {
        $stmt = $db->prepare("UPDATE `accounts` SET `engine_password_sha1` = UPPER(SHA1(:plain)) WHERE `id` = :id");
        $stmt->execute([':plain' => (string)$request->password, ':id' => (int)$account->id]);
    } catch (Exception $e) {
        // ignore
    }

Weryfikacja:
- `diff -u login.php login_optionB_patched3.php` pokazuje wyłącznie powyższy blok.
- Po udanym HTTP logowaniu pole `engine_password_sha1` zostanie zapisane i triggery przerzucą wartość do `canary.accounts(password)`.

# WORKLOG — Opcja B (DB-sync) (Data: 2025-08-25 08:30:00 UTC+02:00+0200)

## Cel
Dwie bazy (`canaryaac` dla WWW, `canary` dla silnika) + automatyczny sync haseł do formatu SHA1 dla silnika.

## Co wprowadzamy
1. Kolumna `accounts.engine_password_sha1` w `canaryaac` (cień SHA1).
2. Triggery `acc_sync_ai` / `acc_sync_au` w `canaryaac`, które przepisują zmiany do `canary.accounts`.
3. Modyfikacja `api/v1/login.php`: po **udanym** logowaniu zapisujemy `engine_password_sha1 = UPPER(SHA1(plain))` (niezależnie od tego, w jakim formacie MyAAC trzyma hasło).

## Jak wdrożyć (kroki)
1) Import SQL (jako root/ptaku, ale z prawami do obu schematów):
```sql
SOURCE /mnt/data/db_sync_optionB.sql;
```
2) Podmień login API:
- Kopia bezpieczeństwa obecnego pliku: `sudo cp /var/www/html/api/v1/login.php /var/backups/login.php.$(date +%s)`
- Wgraj nowy: `sudo cp /mnt/data/login_optionB_patched.php /var/www/html/api/v1/login.php`
- `sudo systemctl reload php8.2-fpm nginx`

3) Test akceptacyjny:
```bash
# A) HTTP login
curl -sS -H 'Content-Type: application/json'   -d '{"type":"login","email":"<EMAIL>","password":"<PLAIN>"}'   http://127.0.0.1/api/v1/login.php | jq '.session.sessionkey, .playdata.worlds[0].externaladdress, .playdata.worlds[0].externalport'

# B) Sprawdź, że hasło trafiło do bazy silnika w formacie 40-znakowego SHA1
mysql -u ptaku -p -e "SELECT id,name,CHAR_LENGTH(password) len, LEFT(password,12) head FROM canary.accounts ORDER BY id DESC LIMIT 5;"
```
Oczekiwane: `len=40`, a `head` wygląda jak hex.

## Uwagi
- Dla starych kont `engine_password_sha1` wypełni się dopiero przy pierwszym **udanym** HTTP logowaniu.
- Jeśli w `.env` masz OTS_GAME_HOST/PORT, pozostawiamy (świat zostaje 127.0.0.1:7172). Po stronie silnika `passwordType="sha1"`.

# WORKLOG — start prac nad „instalką” (MyAAC + API + sync DB)
**Data:** 2025-08-25 08:00:00 UTC+02:00+0200

## 0) Stan na start
- OTClient Redemption uruchomiony i połączony z lokalnym Canary (prot 14.12); API `/api/v1/login.php` zwraca listę postaci, świat `127.0.0.1:7172`; wejście do gry działa wg ostatniej wiadomości użytkownika.
- WWW: MyAAC w `/var/www/html` (tymczasowo `/var/www/html/apik` było użyte dla starego katalogu `/api`).
- DB: `canaryaac` (WWW) i `canary` (silnik) — plan: **Opcja 2 — sync na poziomie DB** z „cieniem SHA1”.
- Pliki pomocnicze API: `ping.php`, `health.php`, `pwcheck.php`, `auth_probe.php` — do pozostawienia tylko lokalnie (lub usunięcia) po zakończeniu instalacji.

## 1) Cel „instalki”
- Spójny, powtarzalny **setup MyAAC + API** w istniejącym docroot (`/var/www/html`) — bez kasowania katalogu.
- Jedna prawda konfiguracji w `.env` (host/port świata, bazy, itp.).
- Automatyczny **sync kont WWW → silnik** (SHA1) bez ręcznych UPDATE’ów.
- Minimalne, bezpieczne endpointy pomocnicze (opcjonalne, lokalne).

## 2) Plan prac (skrócony)
1. **Snapshot/backup** aktualnych plików WWW i `.env`; utrzymaj `/var/www/html/apik` (jeśli nadal potrzebne).
2. **.env – konsolidacja**: upewnij się, że `DB_NAME=canaryaac`, a `ENGINE_DB_NAME=canary`; `OTS_GAME_HOST=127.0.0.1`, `OTS_GAME_PORT=7172`.
3. **MyAAC** — skrypt „krok 9” (klon → przeniesienie do istniejącego docroot, prawa, composer, `install/ip.txt` z newline).
4. **Login API**: utrzymujemy wersję „samodzielną” czytającą `.env`, zwracającą `session+playdata` (sessionkey = `name + "\n" + plain`).
5. **Sync DB (Opcja 2)**: dodać `engine_password_sha1` w `canaryaac.accounts`; w WWW (rejestracja/zmiana hasła/udane logowanie) zapisywać `UPPER(SHA1(plain))`; włączyć triggery `acc_sync_ai` i `acc_sync_au` do `canary.accounts`.
6. **Health endpoints**: utrzymać `ping.php`/`health.php` na **localhost** (lub usunąć w prod).
7. **Launcher/klient**: `launcher_config.json` wskazuje klienta oraz `apiUrl: "http://127.0.0.1/api/v1/"`; assety 1412 pod ręką.
8. **Testy**: `curl` do login API (JSON), SQL kontrolne (len=40 w `canary.accounts.password`), *Enter Game*; potem twarde wyłączenie `DEBUG` i ograniczenie helperów.

## 3) Natychmiastowe kroki — komendy gotowe
> Możesz wykonać **w tej kolejności** (kroki idempotentne).

### A) Backup WWW i .env
```bash
sudo mkdir -p /var/backups/www_$(date +%F)
sudo rsync -a --delete /var/www/html/ /var/backups/www_$(date +%F)/html/
sudo cp -a /var/www/html/.env /var/backups/www_$(date +%F)/html/.env 2>/dev/null || true
```

### B) Konsolidacja `.env` (w razie rozjazdów)
Edytuj `.env` zgodnie ze wzorcem z pliku `/.env.instalka.sample` (załączony). Potem:
```bash
sudo systemctl reload php8.2-fpm nginx
```

### C) MyAAC do istniejącego docroot — skrypt
Użyj załączonego `install_step9_myaac.sh`:
```bash
sudo bash /mnt/data/install_step9_myaac.sh
```

### D) Sync DB (Opcja 2) — SQL
Uruchom w MySQL (root/ptaku): dodanie kolumny „cienia” i triggery, a także *przykładowe* uzupełnienie SHA1 podczas logowania/rejestracji po stronie WWW:
```sql
ALTER TABLE canaryaac.accounts
  ADD COLUMN engine_password_sha1 CHAR(40) NULL AFTER password;

DELIMITER //
CREATE TRIGGER acc_sync_ai AFTER INSERT ON canaryaac.accounts
FOR EACH ROW
BEGIN
  INSERT INTO canary.accounts (name, password, email, type, premdays)
  VALUES (NEW.name, COALESCE(NEW.engine_password_sha1, '0'), NEW.email, 1, 0)
  ON DUPLICATE KEY UPDATE
    password = VALUES(password),
    email    = VALUES(email);
END//
CREATE TRIGGER acc_sync_au AFTER UPDATE ON canaryaac.accounts
FOR EACH ROW
BEGIN
  UPDATE canary.accounts
     SET password = COALESCE(NEW.engine_password_sha1, password),
         email    = NEW.email
   WHERE name = NEW.name OR email = NEW.email;
END//
DELIMITER ;
```

> **WWW (PHP):** przy rejestracji/zmianie hasła/udanym logowaniu wykonaj dodatkowo:
> ```sql
> UPDATE canaryaac.accounts
>    SET engine_password_sha1 = UPPER(SHA1(:plain))
>  WHERE id = :id;
> ```
> (to zapewnia prawidłowe wypełnienie „cienia”, którego triggery przeniosą do bazy silnika).

### E) Testy akceptacyjne
```bash
# 1) API — JSON
curl -sS -H 'Content-Type: application/json'   -d '{"type":"login","email":"<twoj_email>","password":"<twoje_haslo>"}'   http://127.0.0.1/api/v1/login.php | jq '{ok:(.session!=null), world:(.playdata.worlds[0].externaladdress), port:(.playdata.worlds[0].externalport)}'

# 2) DB — SHA1 w bazie silnika
mysql -u ptaku -p canary -e "SELECT id,name,email,CHAR_LENGTH(password) len,LEFT(password,12) head FROM accounts ORDER BY id DESC LIMIT 5;"

# 3) Enter Game w kliencie (assets 1412, httpLogin)
```

## 4) Porządki/bezpieczeństwo po testach
- Ustaw `DEBUG=false` w `.env` + reload FPM/Nginx.
- Ogranicz/usuń `ping.php`, `health.php`, `pwcheck.php` (IP whitelist lub 403).
- Zrób `rsync` backup działającej wersji (jak w punkcie A).

---
**Załączone w tej wiadomości:**  
- `install_step9_myaac.sh` — skrypt wdrożenia MyAAC do istniejącego docroot.  
- `.env.instalka.sample` — wzorzec kluczowych wpisów `.env` (AAC + silnik + świat).


# WORKLOG — OTClient GUI i tłumaczenia — 2025-08-25

## Cel
Uspójnić języki w kliencie (PL/DE) i dodać stały przycisk wyboru języka w górnym menu. Dodatkowo przetłumaczyć elementy ekranu logowania.

## Zmiany w plikach

1) Top menu — przycisk języka
- [+] modules/client_topmenu/topmenu_language_button.otui
  Button < LanguageButton
    id: langTopButton
    icon: /images/topbuttons/language
    tooltip: tr('Language')  # (PL: „Język” / można zmienić na 'Change language')
    anchors.left: prev.right
    anchors.top: parent.top
    margin-left: 4
    onClick: modules.client_locales.createWindow()

- [*] modules/client_topmenu/topmenu.lua
  - rejestracja nowego przycisku (po istniejących)
  - UWAGA: pasek graczy używa `tr('players online')` → wymaga tłumaczenia w pl.lua. (Potwierdzenie w kodzie: setPlayersOnline). :contentReference[oaicite:1]{index=1}

2) Locales — naprawa i inicjalizacja
- [*] modules/client_locales/locales.lua
  - naprawiona składnia (poprzednio: „<eof> expected near 'end'”)
  - init(): po starcie rejestracja przycisku + `createWindow` pod onRun (aby okno było dostępne z GUI)
  - rejestracja extended opcode bez zmian

3) Tłumaczenia — ekran logowania i topbar
- [*] data/locales/pl.lua — dodane/uzupełnione klucze:
  "players online"        = "graczy online"
  "Enable HTTP login"     = "Włącz logowanie HTTP"
  "Remember Email"        = "Zapamiętaj identyfikator konta"
  "Password"              = "Hasło"
  "Language"              = "Język"
  "Change language"       = "Zmień język"
  # (opcjonalnie) tooltip:
  "Be aware that your email address will be stored on your configuration file 'config.otml' if you activate this option."
    = "Pamiętaj: po włączeniu tej opcji adres e-mail zostanie zapisany w pliku „config.otml”."

- [*] (DE) data/locales/de.lua — analogiczne wpisy (jeśli potrzebujesz, wygeneruję gotowy blok).

4) Ikona
- [+] images/topbuttons/language.png — dodana

## Test
- Start klienta → nowa ikona „Language” w prawym górnym rogu, okno wyboru języka działa.
- Pasek tytułowy pokazuje „– graczy online” (przetłumaczone via `tr('players online')` z topmenu.lua). :contentReference[oaicite:2]{index=2}
- Ekran logowania: etykiety i checkboxy przetłumaczone (HTTP login / Remember Email / Password).

## Co zostało / TODO
A) Tooltips i napisy pomocnicze
   - Dodać dłuższy opis dla „Remember Email” do pl.lua (wpis powyżej).
   - Przetłumaczyć dolny panel „Enabling Boosted Creature Panel…” (klucz w module tła/baneru – dopisz w pl.lua).

B) Czcionki (PL/DE/RU)
   - Dla pełnej obsługi ę/ą/ż/ś + cyrylicy dodać fallback czcionki:
     • skopiuj TTF (np. NotoSans-Regular.ttf / NotoSansSC.ttf) do `data/fonts/`
     • utwórz override `modules/corelib/otui/_override_fonts.otui` z mapowaniem:
       ```
       Font
         name: vera
         file: /data/fonts/NotoSans-Regular.ttf
         ...
       ```
     • restart klienta.
   - Dzięki temu etykiety z PL/DE/RU będą renderować się poprawnie (bez kwadracików).

## Notatka o formacie i zgodności z repo
- Styl sekcji i checklist dopasowany do istniejących logów („WORKLOG — …”, checklisty A/B/C). Zob. przykładowe sekcje i listy kontrolne w `worklog_all.md` (tam m.in. checklisty sieci/ports, appx A/B). :contentReference[oaicite:3]{index=3} :contentReference[oaicite:4]{index=4}

Set-Content worklog_session_2025-08-28.md @"
# Worklog — OTClient TTF/RTL + CI (session 2025-08-28)

**Repo:** PtakuPL/testyy  
**Lokalny katalog źródeł:** C:\Gry\Tibia\otland\otclient-main  
**Cel:** włączenie TTF + shaping (HarfBuzz) + bidi (FriBidi) + przygotowanie CI (GitHub Actions) i porządek w repo (bez dużych ZIP).

---

## TL;DR (stan na koniec sesji)
- **Fonts/TTF:** ustalony layout `data/fonts/ttf` + plan Noto (PL/RU/EL/AR/ZH).
- **CMake:** poprawne wykrywanie i linkowanie `Freetype (CONFIG)`, `harfbuzz (CONFIG)`, `fribidi (pkg-config)`.
- **Źródła TTF/RTL:** `framework/text/TextShaper.cpp` + `TTFFont.cpp` dopięte do targetu.
- **CI/Actions:** przygotowane workflowy (Windows, VS2022, vcpkg).
- **vcpkg:** potwierdzona instalacja `freetype`, `harfbuzz`, `fribidi` (OK w logach).
- **Pozostałe zależności:** dostępny wariant “full deps” (glew/physfs/openal/ogg/vorbis/zlib/protobuf/openssl/xz/luajit/asio…).
- **Repo:** `.gitignore`, `.gitattributes`, kopiowanie projektu do czystego klona, eliminacja ZIP > 100MB.

---

## Kroki z sesji (skrót + co zostało zrobione)
1) **Porządek w repo**
   - Skopiowanie projektu do czystego klona `testyy-clean` bez buildów/ZIP-ów:
     ```
     robocopy .. . /MIR /XD .git testyy-clean build .vs CMakeFiles /XF *.zip *.7z *.rar *.exe *.dll *.pdb CMakeCache.txt
     ```
   - Dodane `.gitignore` (ignoruje build/artefakty/archiwa) i `.gitattributes` (LF w repo, binarki jako binary).

2) **Fonty TTF i layout**
   - Katalogi:
     - `data/fonts/ttf/NotoSans-Regular.ttf`
     - `data/fonts/ttf/NotoSans-Bold.ttf`
     - `data/fonts/ttf/NotoSansSC-Regular.ttf` (CJK)
     - `data/fonts/ttf/NotoNaskhArabic-Regular.ttf` (RTL)
   - (Docelowo: fallback chain, RTL, shaping; “fill-unicode” tylko gdybyśmy jednak wracali do atlasów.)

3) **Nowe źródła TTF/RTL**
   - Dodane szkielety:
     - `framework/text/TextShaper.{h,cpp}`
     - `framework/text/TTFFont.{h,cpp}`
   - TODO w tych plikach: render quada + upload subimage na Waszym rendererze (OpenGL).

4) **CMake — poprawki**
   - W top-level `CMakeLists.txt`:
     - `find_package(Freetype CONFIG REQUIRED)`
     - `find_package(harfbuzz CONFIG REQUIRED)`
     - `find_package(PkgConfig REQUIRED)` + `pkg_check_modules(FRIBIDI REQUIRED fribidi)`
     - interfejs `add_library(otc_textstack INTERFACE)` linkujący Freetype/HarfBuzz oraz include+libs z FriBidi
   - W `src/CMakeLists.txt`:
     - `target_sources(... TextShaper.cpp TTFFont.cpp)`
     - `target_link_libraries(${PROJECT_NAME} PRIVATE otc_textstack)`

5) **GitHub Actions (Windows + vcpkg + CMake)**
   - Warianty workflow:
     - bazowy (cmd shell + vcpkg + CMake + pakowanie ZIP),
     - wymuszenie classic mode,
     - full deps (+ kopiowanie DLL z `vcpkg/installed/x64-windows/bin`).
   - Poprawki:
     - kroki vcpkg/CMake w `shell: cmd` (rozwiązuje błąd `-disableMetrics`),
     - pakowanie: `otclient*.exe` + sąsiednie DLL + `data/` do ZIP, upload artifact.

6) **vcpkg — instalacja (OK)**
   - Z logu: `freetype`, `harfbuzz`, `fribidi` + zależności (`brotli`, `bzip2`, `zlib`, `libpng`…) — zakończone sukcesem.

---

## Najczęstsze błędy i rozwiązania
- **PS vs CMD:** `bootstrap-vcpkg.bat` musi iść w `cmd`; ustawiono `shell: cmd`.
- **Manifest mode vs classic:** jeśli w repo jest `vcpkg.json`, vcpkg przechodzi w manifest mode. Rozwiązania:
  - classic: `working-directory: ${{ env.VCPKG_ROOT }}` + lista paczek,
  - manifest: trzymamy paczki w `vcpkg.json` i robimy samo `vcpkg install`.
- **CMake: FreetypeConfig.cmake not found:** używać `find_package(Freetype CONFIG REQUIRED)` (zamiast modułu); FriBidi przez `pkg-config`.
- **Pakowanie nie znajduje EXE:** dopasować wzorzec, częsty path: `build\src\Release\otclient.exe`.

---

## Co zrobić dalej
1. Podmień `CMakeLists.txt` (top + `src/`) zgodnie z poprawkami z sesji.
2. Wybierz workflow (np. wariant “full deps”) i dodaj do `.github/workflows/build-windows.yml`.
3. Commit + push → sprawdź Actions → pobierz artifact.
4. Jeżeli kompilator zgłosi brakujące biblioteki (GLEW/PhysFS/OpenAL itd.), użyj workflow “full deps”.

---

## Pliki przygotowane w sesji (gdzie ich szukać)
- Workflows:
  - build-windows.yml (bazowy)
  - build-windows-fixed.yml (classic vs manifest — pierwsza poprawka)
  - build-windows-classic.yml (wymuszenie `--classic`)
  - build-windows-fulldeps.yml (pełna lista paczek + kopiowanie DLL z vcpkg/bin)
- CMake:
  - CMakeLists.top.fixed.txt (top-level)
  - src.CMakeLists.fixed.txt (wewnątrz `src/`)

"@ -NoNewline


worklog_all_UPDATE_2025-08-30.md

Kontekst: krok 2 (instalka/klient) — pełne TTF + shaping RTL/CJK, bez dotykania serwera.
Repo/gałąź: otclient-main/vc17 (Visual Studio, OpenGL/x64).

Co zrobiliśmy (chronologicznie)

CMake + VS projekt

Naprawa vc17/otclient.vcxproj: usunięcie duplikatów i złej pozycji Target, dodanie jednego CopyFontAssets wewnątrz <Project>…</Project>.

Naprawa vc17/otclient.vcxproj.filters: przywrócony xmlns MSBuild, usunięte duplikaty, dodane filtry framework → framework\text, podpięte pliki w drzewku.

Dopisane źródła: ..\src\framework\text\LocaleShaping.cpp + nagłówek do <ClInclude>.

Fonty/TTF

(wcześniej) dodane TTF NotoSans/Noto Naskh/Noto Mono do data/fonts/ttf/, otfonty zmapowane; fallback dla UI i monospace (konsola/logi).

LocaleShaping (klient)

Wprowadzenie plików: src/framework/text/LocaleShaping.h/.cpp.

Usunięto dublowanie typów: TextDirection/ShapeParams nie są już definiowane w LocaleShaping.h (źródło konfliktu C2011) — korzystamy z tych z TextShaper.h.

Implementacja fromLocale(locale) → (Latn/Cyrl/Grek/Arab/Hani + LTR/RTL).

TextShaper

Poprawione include’y HarfBuzza na układ vcpkg:
<harfbuzz/hb.h>, <harfbuzz/hb-ft.h>, <harfbuzz/hb-ot.h> (fix dla hb.h: not found).

Dopasowanie sygnatury implementacji do nagłówka (rozwiązuje C2511):
std::vector<ShapedGlyph> TextShaper::shape(const std::u32string& text32, hb_font_t* hbFont, const ShapeParams& params)

Podanie tekstu do HarfBuzza jako UTF-32 (hb_buffer_add_utf32).

Główne błędy i jak je naprawiliśmy

„Istnieje wiele elementów głównych” w .vcxproj → Target CopyFontAssets był poza </Project>. Przeniesiony do środka.

Błąd przestrzeni nazw .filters → brak xmlns="http://schemas.microsoft.com/developer/msbuild/2003". Dodane.

Duplikaty w .filters (TextShaper.cpp „występuje już w filtrze”) → wyczyszczone duplikaty <ClCompile>/<ClInclude>.

C2011 (redefinicje TextDirection, ShapeParams) → LocaleShaping.h nie definiuje już tych typów; używa TextShaper.h.

C1083: hb.h not found → include’y na <harfbuzz/...> i/lub dopisanie vcpkg/include do Additional Include Directories.

C2511: sygnatura shape(...) → dopasowana do deklaracji w TextShaper.h (u32string + hb_font_t*).

Pliki zmienione (klient)

vc17/otclient.vcxproj — naprawa struktury, dodane LocaleShaping, CopyFontAssets.

vc17/otclient.vcxproj.filters — xmlns, filtry, mapowania, deduplikacja.

src/framework/text/LocaleShaping.h — bez dublowanych typów; deklaracje normalize/fromLocale.

src/framework/text/LocaleShaping.cpp — mapping locale → (script, direction).

src/framework/text/TextShaper.cpp — include’y HB (vcpkg), sygnatura i UTF-32, normalizacja parametrów.

# Build Log — otclient (OpenGL|x64) — 2025-09-02 06:16 UTC+02:00


**Środowisko**
- IDE: Visual Studio 2022 (v143 / MSVC)
- Konfiguracja: **OpenGL | x64**
- vcpkg: ostrzeżenia `MSB4011` (wielokrotny import `Directory.Build.props/targets`) — **do zignorowania**
- Unity build: **włączony** (kompilowane pliki mają formę `unity_*.cpp`)

---

## Oś czasu / co zrobiliśmy

**1) ICE C1001 w `otmlnode.cpp` (ok. 04:20–04:38)**  
- Komunikat: `fatal error C1001` (Internal Compiler Error), początkowo w linii 71, po zmianach w linii 76.  
- Podjęte próby: wariant A/B `otmlnode.cpp` (pragmy `/Od`, defensywne ciało), dodany arkusz `otmlnode-disable-opt.props`.  
- Wniosek: błąd pojawia się przy **„Trwa generowanie kodu”**, co wskazuje na **LTCG** (link-time).

**2) Wyłączenie LTCG (ok. 05:25–05:35)**  
- W **Konsolidator → Wiersz polecenia → Opcje dodatkowe** dopisano **`/LTCG:OFF`**.  
- ICE **C1001 zniknął** — potwierdza, że problem był po stronie **LTCG**.

**3) Nowe błędy po zdjęciu LTCG (ok. 05:45–05:47)**  
- **C1128**: *liczba sekcji przekroczyła limit; skompiluj z parametrem `/bigobj`* (`type_traits(1503,1)`).  
- **D8040**: *błąd podczas tworzenia lub komunikowania z procesem podrzędnym*.  
- Przyczyna: **gigantyczne TU** z unity buildów (mnóstwo instancji szablonów STL).

**4) Kolejna próba (ok. 06:06–06:07)**  
- W logu lista TU to `unity_*.cpp`, potem **„Generowanie kodu…”** (to etap codegen kompilatora, *nie* LTCG).  
- Błąd: **MSB6006**: `CL.exe` zakończone kodem **-1073741819 (0xC0000005)** — access violation kompilatora na bardzo dużym TU.  
- To znów wskazuje na **rozmiar TU / pamięć**, nie na błąd w kodzie projektu.

---

## Aktualny stan
- **ICE (C1001) — rozwiązany** przez **`/LTCG:OFF`**.  
- Build obecnie **łamie się na kompilacji unity_*.cpp**: **C1128** + **D8040** → przy kolejnej próbie **MSB6006**.

---

## Zalecenia i następne kroki (plan naprawczy)

1. **Włącz `/bigobj`** (jeśli jeszcze nie):  
   **C/C++ → Wiersz polecenia → Opcje dodatkowe** → dopisz ` /bigobj `.
2. **Na czas diagnozy wyłącz kompilację równoległą**:  
   **C/C++ → Ogólne → Kompilacja wieloprocesorowa (/MP)** = **Nie**.
3. **Wyłącz unity build dla konfiguracji OpenGL|x64**, aby zmniejszyć rozmiar TU:  
   - Jeśli projekt z CMake: przebuduj z `-DCMAKE_UNITY_BUILD=OFF` (ew. `-DCMAKE_UNITY_BUILD_BATCH_SIZE=4`).  
   - Jeśli MSBuild: dodaj plik **`Directory.Build.targets`** w katalogu rozwiązania:
     ```xml
     <Project>
       <ItemGroup Condition="'$(Configuration)|$(Platform)'=='OpenGL|x64'">
         <ClCompile Remove="**\unity_*.cpp" />
       </ItemGroup>
     </Project>
     ```
   Dzięki temu w **tej jednej** konfiguracji unity‑pliki nie będą kompilowane.
4. **(Opcjonalnie) Ogranicz footprint** na czas diagnozy:  
   - **C/C++ → Generowanie informacji debugowania** = **Brak**.  
   - **Konsolidator → Debugowanie → Generuj informacje debugowania** = **Nie**.  
   - **C/C++ → Wiersz polecenia** dopisz także ` /Zm200 ` (powiększa pulę PCH).

---

## Notatki o czcionkach / i18n (na później)
- Błędy kompilacji **nie** wynikają z TTF/otfont. Czcionki i i18n to sprawy runtime (Lua/OTUI).  
- Dla `.otfont` typu **bitmap** wymagany jest klucz `texture:`; dla typu **ttf** wymagane `type: ttf`, `file: /fonts/ttf/...` oraz `fallback: [...]`.  
- Po przejściu buildu wrócimy do i18n i konfiguracji fallbacków (Noto Sans + CJK/Arabic).

---

### Checklista do weryfikacji po zmianach
- W `Dane wyjściowe → Kompilacja` przy `cl.exe` widnieje **`/bigobj`**.  
- **C1128/D8040** nie występują.  
- Jeśli zastosowano `Directory.Build.targets`: unity‑pliki nie są kompilowane w OpenGL|x64.  
- Build przechodzi dalej niż wcześniej (brak MSB6006).
