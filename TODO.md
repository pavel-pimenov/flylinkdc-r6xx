# TODO

Все задачи и план сессии фиксируются здесь, чтобы можно было продолжить работу
после разрыва сессии (пропал интернет, закрыт терминал и т.п.).

## Обновление вендорных библиотек (vendor-update.py)

Скрипт `vendor-update.py` скачивает вендорные исходники с upstream-URL,
синхронизирует их в репозиторий, заново накладывает локальные патчи проекта
и делает git-commit. Запуск из корня репозитория:
`python vendor-update.py [--vendor NAME] [--dry-run] [--no-commit] [--list]`
Архивы кэшируются в `build-win/vendor-cache` (директория в .gitignore).

- [x] sqlite: 3.50.4 -> 3.54.0 (`client/sqlite`) — закоммичено `45872dc1`.
  Локальные патчи (`#include "sqlite_fly.h"`, pragma warning disable 4127/4244/4100
  в sqlite3.c, `#pragma once` + `sqlite_fly.h` в sqlite3.h) переносятся скриптом.
- [x] wtl: 10.0 -> 10.0 (10.0.10320) — сравнение с upstream и применение всех
  5 локальных патчей даёт 1:1 воспроизведение текущего дерева, commit не нужен.
- [ ] zlib-ng: пин develop f18efb822a2fc24a1209a836d51bb217476fc08e (HEAD).
  Сухие изменения (dry-run): add=326 update=84 delete=14. НЕ закоммичено:
  upstream перестроил дерево (zlib-ng.h/zconf-ng.h стали генерируемыми .in,
  файлы crc32.h/insert_string_roll.c переименованы), обновление ломает сборку
  без правки zlib-ng_2022.vcxproj. Перед коммитом: проверить сборку MSVC
  (Debug/Release x64), при необходимости добавить/убрать исходники в vcxproj,
  решить судьбу генерируемых заголовков.
- [ ] Остальные вендоры в конфиг скрипта (по аналогии): boost, openssl, bzip2,
  leveldb, libtorrent, lua, jsoncpp, maxminddb, snappy, selene, mediainfolib,
  Zenlib, unrar. У каждого проверить локальные патчи проекта перед заведением
  в конфиг.
- [ ] После обновлений — контрольная сборка Release|x64 (0 предупреждений/0
  ошибок) и Debug|x64, дымовой запуск exe на Windows.

## Сборка (правила — только Docker, см. AGENTS.md)

- [ ] `docker build --tag flylinkdc-build:ubuntu26.04 - < Dockerfile`
- [ ] Dockerfile: настроить сборку Windows-библиотек, решить совместимость
  ATL/MSVC для кросс-компиляции. Не подменять функционал заглушками.
- [ ] При успешной линковке: проверка импортов `x86_64-w64-mingw32-objdump -p`
  и проверка запуска EXE на Windows.

## История / статус прошлых сессий

- [x] WPO (/GL + /LTCG) для Release|x64, /analyze доведён до 0/0 (см. HISTORY.md).
- [x] Сборка Release|x64 в VS 18 Insiders: 0 предупреждений/0 ошибок
  (см. HISTORY.md).