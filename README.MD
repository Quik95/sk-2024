# Sprawozdanie z projektu - Szachy
##### Sebastian Bartoszewicz (151748)

Projekt jest zaimplementowany w architekturze klient-serwer. Server został napisany w języku C, natomiast klient w języku Typescript.
Komunikacja między klientem a serwerem odbywa się za pomocą protokołu TCP i HTTP. Dane wysyłane są w formacie JSON.
Serwer działa na jednym wątku i obsługuje wielu klientów jednocześnie. Wszystkie dane o grze przechowywane są w pamięci serwera.
W celu odebrania zmian klient wykorzystuje mechanizm pollingu, co sekundę wysyłając zapytanie do serwera o aktualny stan gry.
Serwer po otrzymaniu zapytania o stan gry sprawdza, czy gra się zakończyła, jeśli tak to wysyła odpowiedź z informacją o zwycięzcy.
W przeciwnym wypadku serwer przeprowadza walidację ruchu i jeśli jest on poprawny, aktualizuje stan gry i wysyła odpowiedź z nowym stanem gry.
W przypadku wykrycia niepoprawnego ruchu serwer wysyła odpowiedź z informacją o błędzie oraz poprawny stan szachownicy, dzięki czemu gracz może ponownie spróbować wykonać ruch.
W celu wykrycia rozłączenia przy każdej interakcji z serwerem zapisywany jest czas ostatniej aktywności klienta. Jeśli czas ten przekroczy 5 sekund, serwer uznaje, że klient się rozłączył i wysyła tę informację do drugiego klienta.

## Uruchamianie
### Serwer
W celu poprawnej kompilacji musi być zainstalowana biblioteka `cjson`
```bash
pacman -S cjson
```
Kompilacja:
```bash
cd server
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ../CMakeLists.txt
make
```
Uruchomienie:
```bash
./server
```

### Klient
W celu poprawnej kompilacji musi być zainstalowany `nodejs` oraz `npm`

W pierwszej kolejności należy zainstalować zależności:
```bash
npm install
```

#### Uruchamianie
```bash
npm run dev
```