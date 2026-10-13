# TODO

Все задачи и план сессии фиксируются здесь, чтобы можно было продолжить работу
после разрыва сессии (пропал интернет, закрыт терминал и т.п.).

## Тёмная тема интерфейса WTL (Light/Dark)

Цель: полный тёмный UI с опцией возврата к светлой теме.

- [x] Phase A — инфраструктура: настройка `THEME` + `ThemeMode`, `isDarkTheme()`,
  тёмная палитра-переопределение дефолтов в `SettingsManager::get(IntSetting)`,
  тег/дефолт/import-export dctheme, строки `THEME_LIGHT/DARK`.
- [x] Phase B — `Colors::getSysColor` + `WinUtil::applyDarkTitleBar` (DWM),
  миграция `GetSysColor` в видимых компонентах (табы, списки/выделение,
  transfers, search, emoticons, colors pages, public hubs, GradientLabel).
- [x] Phase C — комбо выбора темы в `AppearancePage` (`IDC_THEME_COMBO`),
  live re-apply (`Colors::init()` в `write()` + заголовок/RedrawWindow),
  форс краски PropPage-диалогов в тёмном режиме.
- [x] Phase D — каркас: лёгкий `windows/Colors.h` (`FlyTheme::getSysColor`,
  разрыв цикл. include WinUtil.h ← wtl_flylinkdc.h); `Colors::getSysColor`
  (WinUtil.cpp) делегирует в него; ссылки `wtl_flylinkdc.h:241`, `flattabctrl.h`,
  `PopupDlg.h` (фон INFOBK + owned brush), включая мёртвые `DimEdit.h`,
  `grid/PropertyGrid.h`, `MiniHTML.h`, `PropertyList.h`, `PropertyView.h`.
- [ ] Phase D — остаточный проход по окнам/компонентам:
  - [x] меню и общие контролы (скроллбары, заголовки колонок, тултипы) —
    uxtheme (`WinUtil::initDarkMode`/`applyDarkMode`, DarkMode_Explorer).
  - [x] все MDI-фреймы (применение в `MDITabChildWindowImpl::Create`).
  - [x] сплиттеры (`windows/SearchFrm.h`, `CFlySplitterWindowT::DrawSplitterBar`).
  - [x] owner-draw элементы: аудит — прямых системных цветов не осталось (всё через `Colors::getSysColor`).
  - [ ] проверка на светлой теме (регрессии) и дымовой запуск exe — требует GUI, вручную.
- [x] Phase D — сборочные фиксы Debug x64 (MSVC доступен в текущем окружении,
  VS 18 Insiders): `PropertyList.h` (`FlyTheme::getSysColor`, `CBrush::CreateSolidBrush`),
  `AppearancePage.cpp` (combo `AddString`).

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
- [x] zlib-ng: пин develop f18efb822a2fc24a1209a836d51bb217476fc08e (2.3.90).
  Обновление выполнено. upstream перестроил дерево: `zlib-ng.h`/`zconf-ng.h` стали
  генерируемыми из `*.h.in`, добавлен `zlib_name_mangling-ng.h`; `crc32.h`,
  `crc32_fold*`, `insert_string_roll.c`/`insert_string_tpl.h` удалены/переименованы.
  Заголовки сгенерированы CMake (`-DZLIB_COMPAT=OFF`) и положены в LF.
  `zlib-ng_2022.vcxproj` (+`.filters`) пересобран: ClCompile 48 / ClInclude 52.
  Введён курируемый манифест `vendor-manifests/zlib-ng.txt` (только library-исходники,
  без test/tools/CMakeLists/чужих архитектур) — `vendor-update.py` переведён с
  `'tree': True` на `'select'`. Воспроизведение: add=0 update=0 delete=0 unchanged=106.
  Debug x64 Rebuild 0/0 (exe 54457856); Release x64 Rebuild 0/0 (exe 20656640).
- [x] Остальные вендоры в конфиг скрипта (по аналогии).
- [x] jsoncpp (база 1.9.6, амальгамация `amalgamate.py`), maxminddb 1.7.1,
  boost 1.88, leveldb 1.23, ZenLib 0.4.41 — заведены в конфиг (`select` +
  `generate` + внешние патчи `vendor-patches/*.patch` через `git apply`,
  манифесты `vendor-manifests/*.txt`). Для каждого проверено 1:1
  воспроизведение дерева, `--dry-run` проходит. Коммит пока НЕ создан
  (вся работа в working tree: vendor-update.py, vendor-patches/, vendor-manifests/).
- [x] Вендоры второй волны заведены и проверены 1:1 (манифест + патч == дерево):
  bzip2 1.0.8, lua 5.4.6, unrar 6.23 (`unrarsrc-6.2.10`; у rarlab несогласованная
  нумерация — 6.2.3 = 6.20 beta 3), libtorrent 2.1.0, mediainfolib 25.07,
  snappy 1.2.0, selene (pin `5cde61c6…`), openssl (pin `b372b1f7…`,
  OpenSSL_1_1_1-stable `1.1.1x-dev`). Скрипт: добавлена EOL-нормализация
  распакованного upstream в LF (`normalize_lf`), `re.MULTILINE` для версий,
  опциональный второй столбец в манифесте; фикс `git apply` (из корня репо с
  `--directory` и `autocrlf=false`). Патчи генерируются против LF-нормализованных
  деревьев (mediainfolib/unrar с CRLF-upstream). Проверка всех 8: sync →
  `git add` → пустой `git diff --cached` → 0 ошибок `git apply`. Артефакты
  (`vendor-manifests/*.txt`, `vendor-patches/*.patch`, правки `vendor-update.py`)
  пока в working tree, коммит по запросу.
- [x] Обновления до актуальных версий (безопасный набор), выверено 1:1:
  lua 5.4.6->5.4.9, unrar 6.23->6.24, jsoncpp 1.9.6->1.9.8, snappy 1.2.0->1.3.1.
  Патчи: lua — только $Id:; unrar — UnRAR.vcxproj + win32stm.cpp; jsoncpp — реальные
  правки FlylinkDC (остальное было upstream-дрейфом); snappy — патч удалён
  (upstream 1.3.1 уже имеет эти overload-ы). URL в vendor-update.py обновлены,
  diff у snappy убран. Закоммичено (`7f623158`).
- [x] libtorrent 2.1.0->2.1.2: база форка — upstream `e2b12e72` (RC_2_1, 2025-07-26).
  Ребаз дал 2 конфликта (`src/http_parser.cpp` — квалификация `libtorrent::read_until`;
  `src/identify_client.cpp` — перенос `-MG` в начало), разрешены вручную. Патч
  перегенерирован (16081 б, 12 файлов), verify 499/0. Новый disk-I/O бэкенд pread
  (14 файлов: `src/{pread_disk_io,pread_storage,disk_cache,back_pressure,precomputed_block_hashes}.cpp`,
  `include/libtorrent/pread_disk_io.hpp`, `include/libtorrent/aux_/{pread_disk_job,pread_storage,disk_cache,back_pressure,precomputed_block_hashes,merge_block_requests,visit_block_iovecs}.hpp`,
  `include/libtorrent/random.hpp`) добавлен в манифест (485->499), дерево и
  `libtorrent_2022.vcxproj` (+8 ClInclude/+5 ClCompile) с `.filters`. `vendor-update.py
  --vendor libtorrent` воспроизводит: add=0 update=12 delete=0 unchanged=487. Закоммичено `59ea1ac3` (Debug x64 собран 0/0).
- [x] mediainfolib 25.07->26.10: база форка — тег v25.07 (2025-07-29, `History_DLL.txt`
  совпал). 17 конфликтов ребаза разрешены вручную (upstream — чистки warnings и новые
  сигнатуры/рефакторинг; сохранены правки форка: zlib-ng `zng_*`, доп. include в PreComp.h,
  `Offset` в File_Pac.cpp, класс `File__Base_Avc_Hevc`). Добавлены 3 новых парсера
  (`Image/File_AmigaIcon`, `Video/File_Apv`, `Video/File_Av2`) в манифест (509->515),
  дерево и `MediaInfoLib_2022.vcxproj` (+.filters). Патч перегенерирован (172771 б, 110 файлов),
  verify 515/0; `vendor-update.py --vendor mediainfolib` -> add=0 update=110 delete=0 unchanged=405.
  Закоммичено `9d541b1f` (правки сборочных ошибок + патч — `47f48519`); Debug x64 собран 0/0.
- [ ] При первом реальном прогоне новых вендоров создаются коммиты через
  `commit()` (git add → porcelain → commit). Нюанс на Windows при
  core.autocrlf=true: у LF-исходников (tar.gz с GitHub) после синка
  `git status` может показывать файлы как Modified — это артефакт stat/EOL-представления
  (контент байт-в-байт совпадает, проверено hash-object); после `git add`
  porcelain чистый и повторный синк даёт «no changes».
  Поэтому перед коммитом дерева достаточно `git add` (скрипт делает сам);
  уровень безопасной проверки — `git diff --stat` пустой.
- [x] Debug|x64 и Release|x64 (Build + чистый Rebuild solution) — 0 предупреждений/0 ошибок.
- [ ] Дымовой запуск exe на Windows.

## Сборка (правила — только Docker, см. AGENTS.md)

- [ ] `docker build --tag flylinkdc-build:ubuntu26.04 - < Dockerfile`
- [ ] Dockerfile: настроить сборку Windows-библиотек, решить совместимость
  ATL/MSVC для кросс-компиляции. Не подменять функционал заглушками.
- [ ] При успешной линковке: проверка импортов `x86_64-w64-mingw32-objdump -p`
  и проверка запуска EXE на Windows.

## Аудит апгрейдов вендоров (2026-10-13)

- [x] Проверены upstream-релизы всех вендоров. Актуальны: sqlite 3.54.0, zlib-ng 2.3.90 (develop-pin),
  jsoncpp 1.9.8, libtorrent 2.1.2, mediainfolib 26.10, snappy 1.3.1, wtl 10.0.10320,
  bzip2 1.0.8, leveldb 1.23, ZenLib 0.4.41. Запинены: selene/openssl.
- [x] maxminddb 1.7.1 -> 1.14.1. upstream уже содержит большинство форк-харденинга
  (can_multiply, SSIZE_MAX, sa_family_t, проверки overflow, clang-pragma). Новый патч минимальный:
  `#pragma warning(4201)` для MSVC в maxminddb.h; локальный `maxminddb_config.h` (не синкается)
  задаёт MMDB_UINT128_IS_BYTE_ARRAY=1 и PACKAGE_VERSION "1.14.1". upstream убрал `projects/VS12/maxminddb_config.h`.
  Версия читается теперь из maxminddb_config.h. Debug x64 Rebuild 0/0 (exe 54460416); Release x64 Rebuild 0/0 (exe 20656640).
  Закоммичено.
- [x] Удалён неиспользуемый вендор unrar (153 файла + проект UnRAR.vcxproj): не входит ни в
  одно решение, не линкуется и не вызывается из кода. Убраны каталог, манифест, патч, запись
  в vendor-update.py (14 вендоров), кеш и артефакты. Release x64 Build 0/0. См. HISTORY.md.
- [x] boost 1.88.0 -> 1.92.0. Манифест — по модулям (полные деревья 67 каталогов); новый
  `vendor-manifests/boost.txt` = 7713 записей (7677 `boost/` + 36 `libs/`), удалены 98 файлов,
  пропавших в 1.92 (regex/v4, smart_ptr/detail, asio/detail и т.п.), + `libs/system/src/error_code.cpp`
  (System стал header-only в 1.92). Убраны ссылки на `error_code.cpp` в `boost-regex_2022`,
  `MakeDefs`, `client_test`, `test-console` (+.filters/`local_free_on_destruction.hpp`). Патч
  `boost.patch` (strtoll) актуализирован; исправлен баг CRLF в рабочей копии патча + добавлен
  `.gitattributes` (`vendor-patches/*.patch text eol=lf`). Проверка: dry-run add=1410 update=4283;
  повторный — update=2 unchanged=7711 (стабильно). Debug|x64 и Release|x64 Rebuild — 0/0, exit=0.
  См. HISTORY.md. Коммит НЕ создан (правило: только по запросу).
- [ ] lua 5.4.9 -> 5.5.1 (мажор 5.5, отложено).
- [ ] Коммит обновления boost (boost/ + манифест/патч/vcxproj/.gitattributes) — по запросу пользователя.

## История / статус прошлых сессий

- [x] Аудит PCH/дублей инклудов: PCH уже оптимален во всех проектах (Create+Use),
  дублей фактически нет; убраны 2 повтора в windows/MainFrm.cpp. ClCompile не узкое
  место (см. запись в HISTORY.md).
- [x] Ускорение сборки MSVC (Release Rebuild): протестированы MTT, EnableClServerMode,
  BuildPassReferences, /CGTHREADS:8 и снятие /PROFILE — все медленнее или без выигрыша
  относительно базы 2.16-2.21 мин; стандартные /m + /MP + /LTCG:incremental оптимальны.
  Метрики в build-win/build-stats.csv, запись в HISTORY.md.
- [x] Инкрементальная Build vs Rebuild: Build перелинковывает только затронутое (LTCG
  «1768 of 152382 functions (1.2%) were compiled»), что даёт 0.47 мин на правку 1 файла
  и 0.01 мин на no-op против ~2.2 мин Rebuild. Рекомендация: в повседневной работе
  использовать цель Build, Rebuild только для чистых/контрольных прогонов (HISTORY.md).
- [x] WPO (/GL + /LTCG) для Release|x64, /analyze доведён до 0/0 (см. HISTORY.md).
- [x] Сборка Release|x64 в VS 18 Insiders: 0 предупреждений/0 ошибок
  (см. HISTORY.md).