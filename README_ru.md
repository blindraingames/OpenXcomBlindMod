# OpenXcom

[English version](README.md)

[Руководство по доступности Blind XCom](docs/accessibility_ru.md)

[Режим `--god`](docs/god-mode_ru.md)

OpenXcom — свободная реализация популярных игр MicroProse «UFO: Enemy Unknown»
(в американском издании — «X-COM: UFO Defense») и «X-COM: Terror From the
Deep». Проект распространяется по лицензии GPL и написан на C++ с использованием
SDL.

Дополнительная информация доступна на [сайте OpenXcom](https://openxcom.org) и
в [энциклопедии](https://www.ufopaedia.org/index.php/OpenXcom).

В проекте с разрешения автора используется изменённый код SDL_gfx,
распространяемый по лицензии LGPL.

## Установка

Для работы OpenXcom необходимы оригинальные, не изменённые ресурсы одной или
обеих игр X-COM. Windows-инсталлятор умеет автоматически находить игры,
установленные через Steam, и копировать их ресурсы.

При ручной установке каталоги игр Steam обычно находятся здесь:

```text
UFO:  Steam\SteamApps\common\XCom UFO Defense\XCOM
TFTD: Steam\SteamApps\common\X-COM Terror from the Deep\TFD
```

Не используйте изменённые версии ресурсов, например обработанные XcomUtil: это
может приводить к ошибкам и аварийному завершению игры. Скопируйте подкаталоги
UFO в каталог `UFO` внутри каталога данных или пользовательского каталога
OpenXcom. Аналогично скопируйте ресурсы TFTD в каталог `TFTD`. Расположение этих
каталогов описано ниже.

## Модификации

Модификации — важная часть OpenXcom. В комплект игры входит набор стандартных
модификаций, основанных на традиционных возможностях XcomUtil и UFOExtender.
Кроме того, на [портале модификаций](https://openxcom.mod.io/) доступны сотни
пользовательских проектов.

Чтобы установить модификацию, откройте подкаталог `mods` в пользовательском
каталоге игры и распакуйте модификацию в отдельный каталог с уникальным именем.
Некоторые архивы содержат лишние уровни вложенности. Например, после распаковки
может получиться такая структура:

```text
mods/LulzMod/data/TERRAIN/
mods/LulzMod/data/Rulesets/
```

В этом случае переместите содержимое на один уровень выше, чтобы структура
выглядела так:

```text
mods/LulzMod/TERRAIN/
mods/LulzMod/Rulesets/
```

После этого включите модификацию на странице «Настройки → Модификации» в игре.

## Расположение каталогов

OpenXcom использует три типа каталогов:

| Тип каталога | Содержимое |
|---|---|
| `user` | модификации, сохранения, снимки экрана |
| `config` | настройки игры |
| `data` | ресурсы UFO и TFTD, стандартные модификации и общие ресурсы |

Пути по умолчанию зависят от операционной системы. Для пользовательского
каталога и каталога настроек OpenXcom проверяет указанные ниже пути и использует
первый существующий. Если подходящего каталога нет, игра создаёт его. Ресурсы
ищутся во всех перечисленных каталогах данных, поэтому разные файлы могут
находиться в разных местах.

Путь к каждому каталогу можно явно указать в командной строке. Например:

```text
openxcom -data "$HOME/bin/OpenXcom/usr/share/openxcom"
```

Для полностью автономной установки:

```text
openxcom -data "$HOME/games/openxcom/data" -user "$HOME/games/openxcom/user" -config "$HOME/games/openxcom/config"
```

### Windows

Пользовательский каталог и каталог настроек:

- `C:\Documents and Settings\<пользователь>\My Documents\OpenXcom` — Windows 2000/XP;
- `C:\Users\<пользователь>\Documents\OpenXcom` — Windows Vista и новее;
- `<каталог игры>\user`;
- `.\user`.

Каталоги данных:

- `C:\Documents and Settings\<пользователь>\My Documents\OpenXcom\data` — Windows 2000/XP;
- путь, заданный параметром сборки `DATADIR`;
- `C:\Users\<пользователь>\Documents\OpenXcom\data` — Windows Vista и новее;
- `<каталог игры>`;
- текущий каталог `.`.

### macOS

Пользовательский каталог и каталог настроек:

- `$XDG_DATA_HOME/openxcom`, если переменная `XDG_DATA_HOME` задана;
- `$HOME/Library/Application Support/OpenXcom`;
- `$HOME/.openxcom`;
- `./user`.

Каталоги данных:

- `$XDG_DATA_HOME/openxcom`, если переменная `XDG_DATA_HOME` задана;
- `$HOME/Library/Application Support/OpenXcom`, если `XDG_DATA_HOME` не задана;
- путь, заданный параметром сборки `DATADIR`;
- `$XDG_DATA_DIRS/openxcom` для каждого пути из `XDG_DATA_DIRS`;
- `/Users/Shared/OpenXcom`, если `XDG_DATA_DIRS` не задана или пуста;
- текущий каталог `.`.

### Linux

Пользовательский каталог:

- `$XDG_DATA_HOME/openxcom`, если переменная `XDG_DATA_HOME` задана;
- `$HOME/.local/share/openxcom`, если `XDG_DATA_HOME` не задана;
- `$HOME/.openxcom`;
- `./user`.

Каталог настроек:

- `$XDG_CONFIG_HOME/openxcom`, если переменная `XDG_CONFIG_HOME` задана;
- `$HOME/.config/openxcom`, если `XDG_CONFIG_HOME` не задана.

Каталоги данных:

- `$XDG_DATA_HOME/openxcom`, если переменная `XDG_DATA_HOME` задана;
- `$HOME/.local/share/openxcom`, если `XDG_DATA_HOME` не задана;
- путь, заданный параметром сборки `DATADIR`;
- `$XDG_DATA_DIRS/openxcom` для каждого пути из `XDG_DATA_DIRS`;
- `/usr/local/share/openxcom`, если `XDG_DATA_DIRS` не задана или пуста;
- `/usr/share/openxcom`, если `XDG_DATA_DIRS` не задана или пуста;
- каталог, в который были установлены файлы данных;
- текущий каталог `.`.

## Настройка

OpenXcom предоставляет множество игровых параметров. Их можно изменять как в
игре, так и вне её. Параметры являются общими и действуют на новые и уже
существующие сохранения.

Подробное описание доступно в
[энциклопедии OpenXcom](https://ufopaedia.org/index.php/Options_(OpenXcom)).

## Разработка

Для сборки OpenXcom необходимы следующие библиотеки:

- [SDL](https://www.libsdl.org) — SDL 1.2;
- [SDL_mixer](https://www.libsdl.org/projects/SDL_mixer/) — SDL_mixer 1.2;
- [SDL_gfx](https://www.ferzkopp.net/wordpress/2016/01/02/sdl_gfx-sdl2_gfx/) версии 2.0.22 или новее;
- [SDL_image](https://www.libsdl.org/projects/SDL_image/) — SDL_image 1.2;
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) версии 0.5.3 или новее.

В исходный код включены файлы для следующих систем сборки:

- Microsoft Visual C++ 2010 или новее;
- Xcode;
- Make — см. `Makefile.simple`;
- CMake.

Проект также проверялся с другими инструментами в Windows, macOS и Linux.
Подробные инструкции по компиляции и готовые пакеты зависимостей доступны в
[энциклопедии OpenXcom](https://ufopaedia.org/index.php/Compiling_(OpenXcom)).
