<a id="english"></a>
**English** | [Русский](#russian)

# ReGameDLL_DOD [![Build Status](https://github.com/whamemer/ReGameDLL_DOD/actions/workflows/build.yml/badge.svg)](https://github.com/whamemer/ReGameDLL_DOD/actions)
Reverse-engineered server library of **Day of Defeat 1.3**.

The code base is [hlsdk-portable](https://github.com/FWGS/hlsdk-portable) with the Day of Defeat game code restored on top of it,
function by function, from the Steam version of the game (`dod.so` with debug information). It replaces `dod.dll` / `dod.so` of
the original game and works with the original client library as well as with [dod13-client](https://github.com/whamemer/dod13-client).

> **Status.** The library is playable and is being tested. It is developed and tested on **Windows x86** with the latest
> developer build of Xash3D FWGS. Windows x64, Linux (i386) and macOS (Apple Silicon and Intel) are built by the CI, but nobody
> has played them yet. The code of the game was compared with the decompiled original (the Half-Life monsters and the bots,
> which no map uses, only in part), and the bugs that were found in the game tests were fixed against it. Bug reports are
> welcome.

## What is restored
* **Game rules.** Rounds (pre-round, running, won, draw), reinforcement waves and infinite lives, teams and classes with the
  limits of weapons, scores and the log of scores, clan matches (warm-up, ready signal, restart), map change and time limit.
* **Objectives.** Control points and their master, capture areas, objects (dynamite, documents) and the places they are brought
  to, score entities, round timers, point relays, pre-round triggers, locations and sandbag zones.
* **Players.** Health and stamina, prone, the deployed bipod and the sandbag stance, the mortar, scopes, hints, team kills and
  the auto kick, spawn points, the observer and the spectator modes, map markers, voice commands.
* **Weapons.** All the weapons of the game, with the code that the client runs for the prediction (`dlls/wpn_shared`).
* **Entities of the maps.** The Day of Defeat entities and the entities of Half-Life (doors, trains, buttons, triggers, game_*,
  env_*), the ones that the maps of the game use.
* **Bug fixes of the original game**, optional and switchable at build time (see [Building](#building)).

The Half-Life monsters and the two bot classes (`bot_*`) are compiled as in the original, but no map of Day of Defeat uses them.

## Download
Builds are published on the [Releases](https://github.com/whamemer/ReGameDLL_DOD/releases) page: `windows` (32-bit),
`windows_x64`, `linux` (i386) and, if their build passed, `macos_arm64` and `macos_x64`. Each archive holds the `dod` folder
that goes next to the engine, with the server library (`dod/dlls/dod.dll`, `dod_amd64.dll`, `dod.so`, `dod_arm64.dylib` or
`dod_amd64.dylib`, see [Library names](#library-names)).

The build of the latest commit of `master` is in the pre-release
[continuous](https://github.com/whamemer/ReGameDLL_DOD/releases/tag/continuous), the CI replaces it after every push.

## Installation
1. Get the [latest developer build of Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous) and unpack it.
2. You have to own [Day of Defeat on Steam](https://store.steampowered.com/app/30/Day_of_Defeat/). Copy the `valve` and `dod`
   folders from your Steam installation into the Xash3D FWGS directory. The game files are not distributed with this project.
3. Unpack the build into the same directory, so that the library lands in `dod/dlls/`. Keep the original library under another
   name if you want to go back.
4. Run `xash3d.exe -game dod` (when the client part is installed) or a dedicated server, for example
   `xash3d.exe -DEDICATED -game dod -dlls dlls/dod.dll +maxplayers 32 +map dod_switch`.

### Notes
* **The view does not tilt when strafing.** The tilt comes from the engine cvar `sv_rollangle`. The default of GoldSrc is `2`,
  the default of Xash3D is `0`, so the library sets `sv_rollangle 2` at start when the value is still `0` (the option
  `DEFAULT_SV_ROLLANGLE` in `mod_options.txt`). The `server.cfg` of a dedicated server runs before the library is loaded, so to
  turn the tilt off set `sv_rollangle 0` after the `map` command, for example `+map dod_switch +sv_rollangle 0`.
* The server cvars and the mechanics are described in [dlls/README.md](dlls/README.md).

## Documentation
Everything about the server is in [dlls/README.md](dlls/README.md):
* [Layout](dlls/README.md#layout) of the source code;
* [Server cvars](dlls/README.md#server-cvars) with defaults and effects;
* [Mechanics that exist only in Day of Defeat](dlls/README.md#mechanics-that-exist-only-in-day-of-defeat);
* [Entities of the maps](dlls/README.md#entities-of-the-maps) with their keys;
* [Build options](dlls/README.md#build-options) and the
  [fixes of the bugs of the original game](dlls/README.md#fixes-of-the-bugs-of-the-original-game);
* [Differences from hlsdk-portable](dlls/README.md#differences-from-hlsdk-portable).

## Building
Clone the source code:
```
git clone https://github.com/whamemer/ReGameDLL_DOD --recursive
```
The build is 32-bit by default. The result is installed as `dod/dlls/<library>`. You need a C99 and C++11 compiler (MSVC,
GCC or Clang) and either Python 3 (waf) or CMake 3.10 or newer.

### Options
`mod_options.txt` holds the options of the build, both build systems read it. `GAMEDIR`, `SERVER_INSTALL_DIR` and
`SERVER_LIBRARY_NAME` say where the library is installed and how it is called (they must match `liblist.gam` / `gameinfo.txt`).
Every fix of a bug of the original game has its own macro (`FIX_ENTITY_HASH_OVERFLOW`, `FIX_CAPTURE_AREA_DIVIDE_BY_ZERO`), both
are `ON`. Set one to `OFF` to get the behavior of the original. They are described in
[dlls/README.md](dlls/README.md#fixes-of-the-bugs-of-the-original-game). `DEFAULT_SV_ROLLANGLE` (`ON`) makes the library set
the view tilt of the strafe, see the [Notes](#notes). waf reads the file at `./waf configure`, run it again after a change.

### Library names
The names follow the [library naming scheme](https://github.com/FWGS/xash3d-fwgs/blob/master/Documentation/extensions/library-naming.md)
of Xash3D FWGS, the engine finds the library by them:

| Platform | Library |
|----------|---------|
| Windows x86 | `dod.dll` |
| Windows x64 | `dod_amd64.dll` |
| Linux i386 | `dod.so` |
| macOS arm64 | `dod_arm64.dylib` |
| macOS x86_64 | `dod_amd64.dylib` |

The CI builds Windows x86 and x64, Linux i386 and macOS arm64 and x86_64, the same platforms as dod13-client without Android.

### waf
```
./waf configure
./waf build
./waf install --destdir=dist
```
On Windows use `waf.bat`. `-T debug` makes a debug build (the default, `humanrights`, is the optimized one), `-8` allows a
64-bit build, `--disable-werror` turns off the strict warnings of GCC and Clang. The other build types of waf (`fast`,
`fastnative`) use `-Ofast` with GCC and Clang, which changes the floating point math: do not take them for a game server.

### CMake
Windows x86:
```
cmake -A Win32 -S . -B build
cmake --build build --config Release
cmake --install build --config Release --prefix dist
```
Windows x64:
```
cmake -A x64 -D64BIT=ON -S . -B build64
cmake --build build64 --config Release
cmake --install build64 --config Release --prefix dist64
```
Linux and macOS:
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build
cmake --install build --prefix dist
```
* **Linux i386** on a 64-bit host: install `gcc-multilib` and `g++-multilib`, the build adds `-m32` by itself. For a library that
  starts on old distributions the CI builds in the [Steam Runtime](https://github.com/ValveSoftware/steam-runtime) i386
  environment, see [.github/workflows/build.yml](.github/workflows/build.yml).
* **macOS:** the build is 64-bit, on Apple Silicon it is arm64; `-DCMAKE_OSX_ARCHITECTURES=x86_64` makes the Intel build. A
  32-bit macOS build is not supported.
* The Ninja generator (`-GNinja`) works everywhere. In a Visual Studio prompt, pick the one of the architecture you want
  (`x86` or `x64`) before you run CMake with Ninja.

`-DLTO=ON` turns on the link time optimization.

## Credits
* **[jeefo](https://github.com/jeefo)** wrote the reverse-engineered source of the Day of Defeat 1.3 server library with which
  this repository started. The restoration was finished on top of that source.
* [FWGS](https://github.com/FWGS) for the engine, [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs), and for the SDK this code
  is based on, [hlsdk-portable](https://github.com/FWGS/hlsdk-portable).
* **Valve** for Day of Defeat.

## License
The code is based on the Half-Life SDK and is distributed under its license, see [LICENSE](LICENSE): it allows free,
non-commercial modifications of Valve games. Third-party parts keep their own licenses.

Day of Defeat and Half-Life are products of Valve Corporation. This project is not affiliated with or endorsed by Valve. It
contains no assets of the game, you need the original game to play.

---

<a id="russian"></a>
[English](#english) | **Русский**

# ReGameDLL_DOD [![Build Status](https://github.com/whamemer/ReGameDLL_DOD/actions/workflows/build.yml/badge.svg)](https://github.com/whamemer/ReGameDLL_DOD/actions)
Серверная библиотека **Day of Defeat 1.3**, восстановленная реверс-инжинирингом.

За основу взят [hlsdk-portable](https://github.com/FWGS/hlsdk-portable), поверх которого функция за функцией восстановлен
игровой код Day of Defeat из Steam-версии игры (`dod.so` с отладочной информацией). Библиотека заменяет `dod.dll` / `dod.so`
оригинальной игры и работает как с оригинальной клиентской библиотекой, так и с [dod13-client](https://github.com/whamemer/dod13-client).

> **Состояние.** Библиотека играбельна и проходит тестирование. Она разрабатывается и проверяется на **Windows x86** с последней
> developer-сборкой Xash3D FWGS. Windows x64, Linux (i386) и macOS (Apple Silicon и Intel) собирает CI, но в них пока никто не
> играл. Код игры сверен с декомпилированным оригиналом (монстры Half-Life и боты, которых не использует ни одна карта,
> только частично), а найденные в игровых тестах ошибки исправлялись по нему. Сообщения об ошибках приветствуются.

## Что восстановлено
* **Правила игры.** Раунды (подготовка, идёт, победа, ничья), волны подкреплений и бесконечные жизни, команды и классы с
  ограничениями по оружию, очки и журнал очков, клановые матчи (разминка, сигнал готовности, рестарт), смена карты и лимит времени.
* **Цели.** Контрольные точки и их мастер, зоны захвата, объекты (динамит, документы) и места, куда их несут, сущности очков,
  таймеры раунда, реле точек, триггеры подготовки, локации и зоны мешков с песком.
* **Игроки.** Здоровье и выносливость, положение лёжа, развёрнутые сошки и стойка с мешком песка, миномёт, прицелы, подсказки,
  убийства своих и автокик, точки появления, режимы наблюдателя и зрителя, метки на карте, голосовые команды.
* **Оружие.** Всё оружие игры вместе с кодом, который клиент запускает для предсказания (`dlls/wpn_shared`).
* **Сущности карт.** Сущности Day of Defeat и сущности Half-Life (двери, поезда, кнопки, триггеры, game_*, env_*), которые
  используют карты игры.
* **Исправления багов оригинальной игры**, необязательные и включаемые при сборке (см. [Сборка](#сборка)).

Монстры Half-Life и два класса ботов (`bot_*`) компилируются как в оригинале, но ни одна карта Day of Defeat их не использует.

## Загрузка
Сборки публикуются на странице [Releases](https://github.com/whamemer/ReGameDLL_DOD/releases): `windows` (32 бита),
`windows_x64`, `linux` (i386) и, если их сборка прошла, `macos_arm64` и `macos_x64`. В каждом архиве лежит папка `dod`, которая
кладётся рядом с движком, с серверной библиотекой (`dod/dlls/dod.dll`, `dod_amd64.dll`, `dod.so`, `dod_arm64.dylib` или
`dod_amd64.dylib`, см. [Имена библиотек](#имена-библиотек)).

Сборка последнего коммита ветки `master` лежит в пре-релизе
[continuous](https://github.com/whamemer/ReGameDLL_DOD/releases/tag/continuous), CI заменяет её после каждого пуша.

## Установка
1. Скачайте [последнюю developer-сборку Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous) и распакуйте её.
2. У вас должна быть [Day of Defeat в Steam](https://store.steampowered.com/app/30/Day_of_Defeat/). Скопируйте папки `valve` и
   `dod` из установки Steam в каталог Xash3D FWGS. Файлы игры вместе с проектом не распространяются.
3. Распакуйте сборку в тот же каталог так, чтобы библиотека оказалась в `dod/dlls/`. Сохраните оригинальную библиотеку под
   другим именем, если захотите вернуться.
4. Запустите `xash3d.exe -game dod` (при наличии клиентской части) или выделенный сервер, например
   `xash3d.exe -DEDICATED -game dod -dlls dlls/dod.dll +maxplayers 32 +map dod_switch`.

### Примечания
* **Вид не наклоняется при стрейфе.** Наклон задаёт переменная движка `sv_rollangle`. Умолчание GoldSrc равно `2`, умолчание
  Xash3D равно `0`, поэтому библиотека при старте ставит `sv_rollangle 2`, если значение всё ещё `0` (параметр
  `DEFAULT_SV_ROLLANGLE` в `mod_options.txt`). `server.cfg` выделенного сервера выполняется до загрузки библиотеки, поэтому
  чтобы отключить наклон, задайте `sv_rollangle 0` после команды `map`, например `+map dod_switch +sv_rollangle 0`.
* Серверные переменные и механики описаны в [dlls/README.md](dlls/README.md#russian).

## Документация
Всё о сервере описано в [dlls/README.md](dlls/README.md#russian):
* [Структура](dlls/README.md#структура) исходного кода;
* [Серверные переменные](dlls/README.md#серверные-переменные) с умолчаниями и действием;
* [Механики, которые есть только в Day of Defeat](dlls/README.md#механики-которые-есть-только-в-day-of-defeat);
* [Сущности карт](dlls/README.md#сущности-карт) с их ключами;
* [Параметры сборки](dlls/README.md#параметры-сборки) и
  [исправления багов оригинальной игры](dlls/README.md#исправления-багов-оригинальной-игры);
* [Отличия от hlsdk-portable](dlls/README.md#отличия-от-hlsdk-portable).

## Сборка
Склонируйте исходный код:
```
git clone https://github.com/whamemer/ReGameDLL_DOD --recursive
```
Сборка по умолчанию 32-битная. Результат устанавливается как `dod/dlls/<библиотека>`. Нужен компилятор C99 и C++11 (MSVC, GCC
или Clang) и либо Python 3 (waf), либо CMake 3.10 и новее.

### Параметры
`mod_options.txt` содержит параметры сборки, его читают обе системы сборки. `GAMEDIR`, `SERVER_INSTALL_DIR` и
`SERVER_LIBRARY_NAME` говорят, куда устанавливается библиотека и как она называется (они должны совпадать с `liblist.gam` /
`gameinfo.txt`). У каждого исправления бага оригинальной игры есть свой макрос (`FIX_ENTITY_HASH_OVERFLOW`,
`FIX_CAPTURE_AREA_DIVIDE_BY_ZERO`), оба `ON`. Поставьте `OFF`, чтобы получить поведение оригинала. Они описаны в
[dlls/README.md](dlls/README.md#исправления-багов-оригинальной-игры). `DEFAULT_SV_ROLLANGLE` (`ON`) заставляет библиотеку
включить наклон вида в стрейфе, см. [Примечания](#примечания). waf читает файл при `./waf configure`, после правки запустите
его снова.

### Имена библиотек
Имена следуют [схеме именования библиотек](https://github.com/FWGS/xash3d-fwgs/blob/master/Documentation/extensions/library-naming.md)
Xash3D FWGS, движок находит библиотеку по ним:

| Платформа | Библиотека |
|-----------|------------|
| Windows x86 | `dod.dll` |
| Windows x64 | `dod_amd64.dll` |
| Linux i386 | `dod.so` |
| macOS arm64 | `dod_arm64.dylib` |
| macOS x86_64 | `dod_amd64.dylib` |

CI собирает Windows x86 и x64, Linux i386 и macOS arm64 и x86_64, те же платформы, что и dod13-client без Android.

### waf
```
./waf configure
./waf build
./waf install --destdir=dist
```
В Windows используйте `waf.bat`. `-T debug` делает отладочную сборку (по умолчанию, `humanrights`, оптимизированная), `-8`
разрешает 64-битную сборку, `--disable-werror` отключает строгие предупреждения GCC и Clang. Остальные типы сборки waf (`fast`,
`fastnative`) используют `-Ofast` с GCC и Clang, который меняет арифметику с плавающей точкой: не берите их для игрового сервера.

### CMake
Windows x86:
```
cmake -A Win32 -S . -B build
cmake --build build --config Release
cmake --install build --config Release --prefix dist
```
Windows x64:
```
cmake -A x64 -D64BIT=ON -S . -B build64
cmake --build build64 --config Release
cmake --install build64 --config Release --prefix dist64
```
Linux и macOS:
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build
cmake --install build --prefix dist
```
* **Linux i386** на 64-битной системе: поставьте `gcc-multilib` и `g++-multilib`, `-m32` сборка добавит сама. Чтобы библиотека
  запускалась и на старых дистрибутивах, CI собирает в окружении [Steam Runtime](https://github.com/ValveSoftware/steam-runtime)
  i386, см. [.github/workflows/build.yml](.github/workflows/build.yml).
* **macOS:** сборка 64-битная, на Apple Silicon это arm64; `-DCMAKE_OSX_ARCHITECTURES=x86_64` делает сборку для Intel.
  32-битная сборка для macOS не поддерживается.
* Генератор Ninja (`-GNinja`) работает везде. В окне Visual Studio выберите нужную архитектуру (`x86` или `x64`) до запуска
  CMake с Ninja.

`-DLTO=ON` включает оптимизацию на этапе компоновки.

## Благодарности
* **[jeefo](https://github.com/jeefo)** написал реверс-инжиниринговый исходный код серверной библиотеки Day of Defeat 1.3, с которого начался этот
  репозиторий. Восстановление было завершено поверх этого исходного кода.
* [FWGS](https://github.com/FWGS) за движок, [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs), и за SDK, на котором основан
  этот код, [hlsdk-portable](https://github.com/FWGS/hlsdk-portable).
* **Valve** за Day of Defeat.

## Лицензия
Код основан на Half-Life SDK и распространяется под его лицензией, см. [LICENSE](LICENSE): она разрешает свободные
некоммерческие модификации игр Valve. Сторонние части сохраняют свои лицензии.

Day of Defeat и Half-Life — продукты Valve Corporation. Этот проект не связан с Valve и не одобрен ею. Он не содержит
материалов игры, для игры нужна оригинальная игра.
