# Pac-Man 3D

## Streszczenie

Pac-Man 3D to desktopowa gra napisana w C++ z wykorzystaniem OpenGL. Projekt przenosi klasyczną mechanikę Pac-Mana do prostej przestrzeni trójwymiarowej: gracz porusza się po labiryncie, zbiera kryształy, unika duchów, a po zebraniu wszystkich kryształów przechodzi w tryb polowania, w którym może zjadać przeciwników.

Gra korzysta z proceduralnie generowanej mapy, modeli OBJ, tekstur, shaderów GLSL, oświetlenia, prostych cieni, HUD-u oraz efektów dźwiękowych. Projekt jest przygotowany głównie pod Windows i może być budowany przez Visual Studio albo CMake.

## Najważniejsze funkcje

- Trójwymiarowy labirynt generowany proceduralnie.
- Sterowanie Pac-Manem za pomocą klawiszy `W`, `A`, `S`, `D` oraz myszy.
- Kamera trzecioosobowa podążająca za graczem.
- Kolizje gracza, duchów i kamery ze ścianami labiryntu.
- Zbieranie kryształów rozmieszczonych w przejściach mapy.
- Modele OBJ Pac-Mana i duchów.
- Animacja Pac-Mana przez przełączanie modelu z otwartą i zamkniętą buzią.
- Duchy z prostą sztuczną inteligencją: patrol, pościg i dojście do ostatniej znanej pozycji gracza.
- Tryb polowania `huntMode` aktywowany po zebraniu wszystkich kryształów.
- Warunki przegranej i zwycięstwa.
- Tryb `God Mode`, który pozwala testować grę bez przegranej po kontakcie z duchem.
- Teksturowane ściany labiryntu.
- Oświetlenie kierunkowe oraz pięć lamp sufitowych rozmieszczanych w labiryncie.
- Półprzezroczyste cienie typu blob shadow pod obiektami.
- Prosty HUD z licznikami i komunikatami gry.
- Efekty dźwiękowe WAV dla najważniejszych zdarzeń.

## Sterowanie

| Klawisz / akcja | Działanie |
| --- | --- |
| `W` | Ruch do przodu względem kierunku Pac-Mana |
| `S` | Ruch do tyłu |
| `A` | Ruch w lewo |
| `D` | Ruch w prawo |
| Mysz | Obracanie Pac-Mana i kamery |
| `R` | Reset gry i wygenerowanie nowej mapy |
| `G` | Włączenie lub wyłączenie `God Mode` |

## Zasady gry

Gracz startuje w labiryncie jako Pac-Man. Celem pierwszej fazy jest zebranie wszystkich kryształów znajdujących się na mapie. Duchy poruszają się po korytarzach i mogą wykrywać gracza, jeżeli mają z nim linię widzenia. Po wykryciu przechodzą w stan pościgu.

Kontakt z duchem w normalnym trybie kończy grę porażką. Po zebraniu wszystkich kryształów aktywuje się tryb `huntMode`: duchy zmieniają kolor na niebieski i zaczynają uciekać od gracza. W tym trybie Pac-Man może zjadać duchy. Zjedzenie wszystkich duchów kończy grę zwycięstwem.

## Technologie i zależności

Projekt wykorzystuje:

- C++17,
- OpenGL,
- GLFW do tworzenia okna i obsługi wejścia,
- GLEW do ładowania funkcji OpenGL,
- GLM do obliczeń na wektorach i macierzach,
- GLSL do programów cieniujących,
- LodePNG do ładowania tekstur PNG,
- WinAPI `PlaySound` i bibliotekę `winmm` do odtwarzania dźwięków,
- modele OBJ dla Pac-Mana i duchów.

Biblioteki GLFW, GLEW i GLM znajdują się lokalnie w katalogu projektu, więc projekt nie wymaga ich osobnego pobierania.

## Struktura projektu

| Ścieżka | Opis |
| --- | --- |
| `main.cpp` | Punkt wejścia programu, inicjalizacja GLFW/GLEW, okna i pętli gry. |
| `game/game.cpp` | Główna logika gry, sterowanie, AI duchów, kolizje, renderowanie, światła, cienie i HUD. |
| `game/game.h` | Deklaracje głównych funkcji gry. |
| `game/cleanup.h` | Reset gry, generowanie mapy, liczenie kryształów i rozmieszczanie duchów. |
| `map/map_settings.h` | Rozmiar mapy oraz typy pól: puste, kryształ i ściana. |
| `map/map_generate.h` | Proceduralne generowanie labiryntu metodą wycinania korytarzy. |
| `pacman.h` | Struktura danych Pac-Mana. |
| `ghost.h` | Struktura ducha i stany AI: `PATROL`, `CHASE`, `FINDING`. |
| `models/` | Figury bazowe, klasa modelu i loader OBJ. |
| `pacman_models/` | Modele `.obj` i `.mtl` Pac-Mana oraz ducha. |
| `shaders/` | Pliki shaderów GLSL. |
| `images/` | Tekstury oraz loader LodePNG. |
| `sounds/` | Efekty dźwiękowe gry. |
| `utils/fonts.h` | Prosty font używany do rysowania HUD-u. |
| `CMakeLists.txt` | Konfiguracja budowania przez CMake. |
| `pacman_3d.sln` | Rozwiązanie Visual Studio. |

## Architektura działania

Aplikacja zaczyna działanie w `main.cpp`. Program tworzy okno 1280x720, inicjalizuje OpenGL, rejestruje callback myszy i uruchamia pętlę gry.

W każdej klatce wykonywane są następujące kroki:

1. Obliczany jest czas od poprzedniej klatki `deltaTime`.
2. Aktualizowana jest logika gry.
3. Aktualizowane są duchy i ich zachowanie.
4. Rysowana jest scena 3D oraz HUD.
5. Bufory obrazu są zamieniane.
6. GLFW przetwarza zdarzenia wejścia.

Najważniejsze funkcje znajdują się w `game/game.cpp`:

- `processInput()` obsługuje klawiaturę, ruch Pac-Mana, reset i `God Mode`,
- `updateGhosts()` aktualizuje AI duchów, ruch i kolizje z graczem,
- `drawScene()` renderuje labirynt, obiekty, światła, cienie i HUD,
- `loadTexture()` ładuje teksturę PNG do OpenGL,
- `updateCeilingLampPositions()` ustawia lampy sufitowe na dostępnych polach mapy.

## Mapa i generowanie labiryntu

Mapa ma rozmiar 10x10 pól i jest przechowywana w tablicy `levelMap`. Każde pole może mieć jeden z typów:

- `FIELD_EMPTY` - puste przejście,
- `FIELD_CRYSTAL` - pole z kryształem,
- `FIELD_WALL` - ściana.

Generowanie mapy działa w kilku etapach:

1. Cała mapa jest wypełniana ścianami.
2. Funkcja `carveMaze()` rekurencyjnie wycina korytarze.
3. Dodawane są dodatkowe przejścia, aby labirynt nie był zbyt liniowy.
4. Puste pola są zamieniane na kryształy.
5. Pole startowe gracza zostaje wyczyszczone.
6. Duchy są rozmieszczane na bezpiecznych polach oddalonych od startu.

## Kolizje

Kolizje Pac-Mana ze ścianami są sprawdzane przed zatwierdzeniem ruchu. Program najpierw oblicza próbną pozycję `tryX` i `tryZ`, a następnie sprawdza pola mapy wokół tej pozycji z niewielkim marginesem kolizji. Jeżeli któreś z tych pól jest ścianą (`FIELD_WALL`) albo znajduje się poza mapą, ruch w danej osi nie zostaje wykonany.

Ruch jest sprawdzany osobno dla osi X i Z. Dzięki temu Pac-Man może płynnie przesuwać się wzdłuż ścian, zamiast całkowicie zatrzymywać się przy lekkim kontakcie z przeszkodą.

Podobny mechanizm jest stosowany dla duchów oraz dla kamery. Kamera sprawdza, czy między graczem a planowaną pozycją kamery nie znajduje się ściana. Jeżeli jest przeszkoda, kamera zostaje przesunięta bliżej Pac-Mana.

## AI duchów

Duchy mają trzy stany działania:

- `PATROL` - losowe poruszanie się po dostępnych korytarzach,
- `CHASE` - pościg za Pac-Manem, gdy duch ma z nim linię widzenia,
- `FINDING` - ruch do ostatniej znanej pozycji Pac-Mana po utracie widoczności.

Widoczność gracza jest sprawdzana przez przejście po punktach między duchem a Pac-Manem i sprawdzenie, czy po drodze nie ma ściany. W normalnym trybie duchy wybierają kierunek zmniejszający odległość do celu. W trybie polowania cel jest ustawiany w przeciwną stronę, więc duchy próbują oddalić się od gracza.

## Renderowanie i shadery

Scena jest renderowana w OpenGL. Labirynt składa się z sześcianów reprezentujących podłogę, sufit i ściany. Ściany używają tekstury `images/bricks_diffuse.png`. Kryształy są rysowane jako małe żółte kule. Pac-Man i duchy korzystają z modeli OBJ z katalogu `pacman_models`.

Projekt używa shaderów GLSL z katalogu `shaders`. Programy cieniujące odpowiadają za przekształcanie wierzchołków, nakładanie kolorów i tekstur oraz obliczanie oświetlenia. Do shaderów przekazywane są macierze:

- `M` - macierz modelu,
- `V` - macierz widoku kamery,
- `P` - macierz projekcji perspektywicznej.

Najważniejszy shader obsługuje kolor bazowy, teksturę, oświetlenie kierunkowe i wpływ lamp sufitowych.

## Oświetlenie i cienie

Projekt zawiera więcej niż jedno źródło światła:

- globalne światło kierunkowe opisane wektorem `lightDirection`,
- pięć lamp sufitowych opisanych strukturą `CeilingLamp`.

Każda lampa sufitowa ma pozycję, kolor, promień działania i intensywność. Lampy mają ciepły żółty kolor i są rozmieszczane w pobliżu narożników oraz środka labiryntu. Po wygenerowaniu mapy ich pozycje są dopasowywane do najbliższych dostępnych pól, aby nie znajdowały się wewnątrz ścian.

Kod sprawdza też widoczność między lampą a oświetlanym punktem. Jeżeli pomiędzy nimi znajduje się ściana, światło danej lampy nie wpływa na ten punkt. Dzięki temu oświetlenie zależy od układu labiryntu.

Pod Pac-Manem, duchami i kryształami rysowane są półprzezroczyste cienie typu blob shadow. W kodzie znajduje się również przygotowanie tekstury głębi i framebufferów dla shadow mappingu.

## HUD i dźwięki

HUD jest rysowany w OpenGL za pomocą prostego fontu zbudowanego z małych brył. Na ekranie wyświetlane są:

- licznik zebranych kryształów,
- licznik pozostałych duchów,
- komunikat `ZJEDZ DUCHY` w trybie polowania,
- komunikat `GOD MODE`,
- ekran `KONIEC GRY`,
- ekran `WYGRANA!`.

Projekt korzysta z plików WAV:

- `sounds/crystal_eaten.wav` - zebranie kryształu,
- `sounds/pacman_death.wav` - przegrana,
- `sounds/pacman_ghost_eating.wav` - zjedzenie ducha,
- `sounds/victory.wav` - zwycięstwo.

Dźwięki są odtwarzane asynchronicznie przez `PlaySound`.

## Budowanie i uruchamianie

Projekt można uruchomić na Windowsie przez Visual Studio, otwierając `pacman_3d.sln`, albo przez CMake.

Przykładowe budowanie przez CMake:

```powershell
cmake -S . -B cmake-build-debug-visual-studio
cmake --build cmake-build-debug-visual-studio
```

Po zbudowaniu program powinien być uruchamiany z katalogu projektu albo z katalogiem roboczym ustawionym na folder projektu. Ścieżki do modeli, tekstur, shaderów i dźwięków są względne, np. `pacman_models/ghost.obj`, `images/bricks_diffuse.png`, `shaders/f_constant.glsl` i `sounds/victory.wav`.

## Stan projektu

Projekt jest grywalnym prototypem trójwymiarowej gry zręcznościowej. Zawiera kompletną pętlę rozgrywki: start, eksplorację labiryntu, zbieranie kryształów, pościg duchów, przegraną, tryb polowania, zjadanie duchów, zwycięstwo i reset gry.

Możliwe dalsze usprawnienia:

- dodanie menu startowego,
- dodanie pauzy,
- dodanie punktacji i najlepszych wyników,
- dodanie liczby żyć,
- rozbudowanie mapy i poziomów trudności,
- ulepszenie AI duchów, np. przez algorytm A*,
- dalsze rozwinięcie efektów świetlnych i cieni,
- uporządkowanie kodu w osobne klasy dla mapy, gracza, duchów i renderera.