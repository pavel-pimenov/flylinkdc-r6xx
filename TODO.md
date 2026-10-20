
# TODO

Все задачи и план сессии фиксируются здесь, чтобы можно было продолжить работупосле разрыва сессии (пропал интернет, закрыт терминал и т.п.).
## Интеграция Sentry (crash reporting)
Цель: отправлять стек падения в Sentry при краше приложения (Windows x64).
- [x] Вендорирован `sentry-native` 0.7.20 (117 файлов, манифест + запись в `vendor-update.py`).
- [x] Проект `sentry-native/sentry-native_2022.vcxproj` (static lib, backend **inproc**, `SENTRY_BUILD_STATIC`).
- [x] Подключение к `flylinkdc_2022.vcxproj` + `FlyLinkDC_2022.sln` (ProjectReference, include, libs dbghelp/Winhttp/version).
- [x] Модуль `windows/CrashReporter.{h,cpp}` (init/shutdown/addBreadcrumb, fatal-обработчики terminate/purecall/invalid_parameter).
- [x] Вызовы в `windows/main.cpp` (init после `SettingsManager::load()`; shutdown на выходе).
- [x] Release|x64 Build — 0 предупреждений/0 ошибок; `dumpbin /dependents` показывает `dbghelp.dll`, `WINHTTP.dll`, `VERSION.dll` (только системные DLL); смоук-тест запуска EXE.
- [ ] **BLOCKER**: подставить реальный Sentry DSN в `windows/CrashReporter.cpp` (`g_sentry_dsn_default`). Сейчас пустой — `sentry_init` вызывается, но события не отправляются.
- [ ] Загрузить PDB (`compiled/flylinkdc_x64.pdb`, Debug+Release) в Sentry для символизации стека под релиз `A_VERSIONSTRING`.
- [ ] Проверить реальную отправку события при падении (тестовый crash-триггер или ручной `__debugbreak`/`abort`).
- [ ] Debug|x64 сборка sentry-native и всего решения (сейчас проверен только Release).
## AddressSanitizer (нагрузочное тестирование)
Цель: найти и исправить нарушения памяти под нагрузкой. Пользователь запускает ASan-сборку,ASan при нарушении пишет отчёты в `build-win\asan-logs\asan_errors.log`; агент затеманализирует лог и правит код.
- [x] Опциональный ASan-режим: корневой `Directory.Build.targets` (`/p:FlyASan=true`;  инструментирует все проекты, включая вендоров; объекты в `asan\`, exe  `compiled\flylinkdc_x64_asan.exe`, копия рантайм-DLL). Без флага — no-op.
- [x] Скрипты `build-win/build-asan.ps1` и `build-win/run-asan.cmd`  (`continue_on_error=1` + `COE_LOG_FILE`, продолжает работу и пишет все ошибки в лог).
- [x] ASan-сборка Release|x64 собрана (W=3 E=0, EXE=37776384).
- [x] Первый прогон: `stack-use-after-scope` в `SimpleXML::getBoolChildAttrib` /  `FavoriteManager::load` — ИСПРАВЛЕНО (статическая пустая строка вместо  default-временного в `client/SimpleXML.h`); повторный 30-секундный прогон чист.
- [ ] Пользователь: прогнать нагрузочный тест на ASan-сборке, передать  `asan_errors.log` для анализа.
- [x] Прогон 2026-10-15: пользователь запустил `FlylinkDC_Debug_x64.exe` (это  **Debug**, не ASan — в импортах нет `clang_rt.asan_dynamic-x86_64.dll`),  ASan-лог пуст. Падение — срабатывание `dcassert` в  `CFlyServerJSON::postQuery` (`flyServer.cpp:2417`, `l_un_compress_result == Z_OK`):  ответ на `fly-zget` (93 байта) не распаковывается (`Z_DATA_ERROR`, -3). Разбор  WER-дампа `%LOCALAPPDATA%\CrashDumps\FlylinkDC_Debug_x64.exe.3668.dmp`,  `compiled\debug-trace.log`, `System.log`, `flylinkdc-Exception-error.log`.  **ИСПРАВЛЕНО**: авто-детект zlib/gzip, потоковый `zng_inflate`,  fallback на необработанное тело, `dcassert` убран. Debug|x64 и ASan Release|x64  собраны без ошибок. См. HISTORY.md (2026-10-15).
- [x] Повторный ASan-прогон (2026-10-15 19:35–19:41) — **чисто**: `asan_errors.log`  не создан (0 ошибок памяти), новых дампов нет. Причина падения подтверждена в  `System.log`: сервер вернул `Error fly-server-test-port` HTML (94 байта = тело из  дампа) вместо zlib; новый код обрабатывает это без краха.
- [ ] Разобрать и исправить все последующие находки ASan.
- [ ] По завершении решить судьбу ASan-инфраструктуры (оставить в репо / вынести в docs).
## Тёмная тема интерфейса WTL (Light/Dark)
Цель: полный тёмный UI с опцией возврата к светлой теме.
- [x] Phase A — инфраструктура: настройка `THEME` + `ThemeMode`, `isDarkTheme()`,  тёмная палитра-переопределение дефолтов в `SettingsManager::get(IntSetting)`,  тег/дефолт/import-export dctheme, строки `THEME_LIGHT/DARK`.
- [x] Phase B — `Colors::getSysColor` + `WinUtil::applyDarkTitleBar` (DWM),  миграция `GetSysColor` в видимых компонентах (табы, списки/выделение,  transfers, search, emoticons, colors pages, public hubs, GradientLabel).
- [x] Phase C — комбо выбора темы в `AppearancePage` (`IDC_THEME_COMBO`),  live re-apply (`Colors::init()` в `write()` + заголовок/RedrawWindow),  форс краски PropPage-диалогов в тёмном режиме.
- [x] Phase D — каркас: лёгкий `windows/Colors.h` (`FlyTheme::getSysColor`,  разрыв цикл. include WinUtil.h ← wtl_flylinkdc.h); `Colors::getSysColor`  (WinUtil.cpp) делегирует в него; ссылки `wtl_flylinkdc.h:241`, `flattabctrl.h`,  `PopupDlg.h` (фон INFOBK + owned brush), включая мёртвые `DimEdit.h`,  `grid/PropertyGrid.h`, `MiniHTML.h`, `PropertyList.h`, `PropertyView.h`.
- [ ] Phase D — остаточный проход по окнам/компонентам:
- [x] меню и общие контролы (скроллбары, заголовки колонок, тултипы) —    uxtheme (`WinUtil::initDarkMode`/`applyDarkMode`, DarkMode_Explorer).
- [x] все MDI-фреймы (применение в `MDITabChildWindowImpl::Create`).
- [x] сплиттеры (`windows/SearchFrm.h`, `CFlySplitterWindowT::DrawSplitterBar`).
- [x] owner-draw элементы: аудит — прямых системных цветов не осталось (всё через `Colors::getSysColor`).
- [ ] проверка на светлой теме (регрессии) и дымовой запуск exe — требует GUI, вручную.
- [x] Phase D — сборочные фиксы Debug x64 (MSVC доступен в текущем окружении,  VS 18 Insiders): `PropertyList.h` (`FlyTheme::getSysColor`, `CBrush::CreateSolidBrush`),  `AppearancePage.cpp` (combo `AddString`).
- [x] Тёмная тема по умолчанию — ОТКАЧЕНО (решение пользователя): тёмная тема  выглядит плохо, переведены не все элементы, в диалогах чёрный текст на сером  не виден. Дефолт возвращён на `THEME_LIGHT` (`client/SettingsManager.cpp:676`).  ru-локализация комбо оставлена (`ThemeDark`/`ThemeLight` в  `compiled/Lang/ru-RU.xml`), переключение: `Файл → Settings (F12) →  «UI Settings» → «FlylinkDC++ Themes» → Light/Dark`.
- [ ] BLOCKER перед повторным включением тёмной по умолчанию: не перекрашены  все диалоги/контролы настроек (чёрный текст на сером фоне). Нужен GUI-проход  по `PropPage`-диалогам (`WM_CTLCOLOR*`, чекбоксы/списки/эдиты внутри страниц)  и дымовой запуск exe на Windows; после этого можно вернуть дефолт на Dark.
## Обновление вендорных библиотек (vendor-update.py)
Скрипт `vendor-update.py` скачивает вендорные исходники с upstream-URL,синхронизирует их в репозиторий, заново накладывает локальные патчи проектаи делает git-commit. Запуск из корня репозитория:`python vendor-update.py [--vendor NAME] [--dry-run] [--no-commit] [--list]`Архивы кэшируются в `build-win/vendor-cache` (директория в .gitignore).
- [x] sqlite: 3.50.4 -> 3.54.0 (`client/sqlite`) — закоммичено `45872dc1`.  Локальные патчи (`#include "sqlite_fly.h"`, pragma warning disable 4127/4244/4100  в sqlite3.c, `#pragma once` + `sqlite_fly.h` в sqlite3.h) переносятся скриптом.
- [x] wtl: 10.0 -> 10.0 (10.0.10320) — сравнение с upstream и применение всех  5 локальных патчей даёт 1:1 воспроизведение текущего дерева, commit не нужен.
- [x] zlib-ng: пин develop f18efb822a2fc24a1209a836d51bb217476fc08e (2.3.90).  Обновление выполнено. upstream перестроил дерево: `zlib-ng.h`/`zconf-ng.h` стали  генерируемыми из `*.h.in`, добавлен `zlib_name_mangling-ng.h`; `crc32.h`,  `crc32_fold*`, `insert_string_roll.c`/`insert_string_tpl.h` удалены/переименованы.  Заголовки сгенерированы CMake (`-DZLIB_COMPAT=OFF`) и положены в LF.  `zlib-ng_2022.vcxproj` (+`.filters`) пересобран: ClCompile 48 / ClInclude 52.  Введён курируемый манифест `vendor-manifests/zlib-ng.txt` (только library-исходники,  без test/tools/CMakeLists/чужих архитектур) — `vendor-update.py` переведён с  `'tree': True` на `'select'`. Воспроизведение: add=0 update=0 delete=0 unchanged=106.  Debug x64 Rebuild 0/0 (exe 54457856); Release x64 Rebuild 0/0 (exe 20656640).
- [x] Остальные вендоры в конфиг скрипта (по аналогии).
- [x] jsoncpp (база 1.9.6, амальгамация `amalgamate.py`), maxminddb 1.7.1,  boost 1.88, leveldb 1.23, ZenLib 0.4.41 — заведены в конфиг (`select` +  `generate` + внешние патчи `vendor-patches/*.patch` через `git apply`,  манифесты `vendor-manifests/*.txt`). Для каждого проверено 1:1  воспроизведение дерева, `--dry-run` проходит. Коммит пока НЕ создан  (вся работа в working tree: vendor-update.py, vendor-patches/, vendor-manifests/).
- [x] Вендоры второй волны заведены и проверены 1:1 (манифест + патч == дерево):  bzip2 1.0.8, lua 5.4.6, unrar 6.23 (`unrarsrc-6.2.10`; у rarlab несогласованная  нумерация — 6.2.3 = 6.20 beta 3), libtorrent 2.1.0, mediainfolib 25.07,  snappy 1.2.0, selene (pin `5cde61c6…`), openssl (pin `b372b1f7…`,  OpenSSL_1_1_1-stable `1.1.1x-dev`). Скрипт: добавлена EOL-нормализация  распакованного upstream в LF (`normalize_lf`), `re.MULTILINE` для версий,  опциональный второй столбец в манифесте; фикс `git apply` (из корня репо с  `--directory` и `autocrlf=false`). Патчи генерируются против LF-нормализованных  деревьев (mediainfolib/unrar с CRLF-upstream). Проверка всех 8: sync →  `git add` → пустой `git diff --cached` → 0 ошибок `git apply`. Артефакты  (`vendor-manifests/*.txt`, `vendor-patches/*.patch`, правки `vendor-update.py`)  пока в working tree, коммит по запросу.
- [x] Обновления до актуальных версий (безопасный набор), выверено 1:1:  lua 5.4.6->5.4.9, unrar 6.23->6.24, jsoncpp 1.9.6->1.9.8, snappy 1.2.0->1.3.1.  Патчи: lua — только $Id:; unrar — UnRAR.vcxproj + win32stm.cpp; jsoncpp — реальные  правки FlylinkDC (остальное было upstream-дрейфом); snappy — патч удалён  (upstream 1.3.1 уже имеет эти overload-ы). URL в vendor-update.py обновлены,  diff у snappy убран. Закоммичено (`7f623158`).
- [x] libtorrent 2.1.0->2.1.2: база форка — upstream `e2b12e72` (RC_2_1, 2025-07-26).  Ребаз дал 2 конфликта (`src/http_parser.cpp` — квалификация `libtorrent::read_until`;  `src/identify_client.cpp` — перенос `-MG` в начало), разрешены вручную. Патч  перегенерирован (16081 б, 12 файлов), verify 499/0. Новый disk-I/O бэкенд pread  (14 файлов: `src/{pread_disk_io,pread_storage,disk_cache,back_pressure,precomputed_block_hashes}.cpp`,  `include/libtorrent/pread_disk_io.hpp`, `include/libtorrent/aux_/{pread_disk_job,pread_storage,disk_cache,back_pressure,precomputed_block_hashes,merge_block_requests,visit_block_iovecs}.hpp`,  `include/libtorrent/random.hpp`) добавлен в манифест (485->499), дерево и  `libtorrent_2022.vcxproj` (+8 ClInclude/+5 ClCompile) с `.filters`. `vendor-update.py  --vendor libtorrent` воспроизводит: add=0 update=12 delete=0 unchanged=487. Закоммичено `59ea1ac3` (Debug x64 собран 0/0).
- [x] mediainfolib 25.07->26.10: база форка — тег v25.07 (2025-07-29, `History_DLL.txt`  совпал). 17 конфликтов ребаза разрешены вручную (upstream — чистки warnings и новые  сигнатуры/рефакторинг; сохранены правки форка: zlib-ng `zng_*`, доп. include в PreComp.h,  `Offset` в File_Pac.cpp, класс `File__Base_Avc_Hevc`). Добавлены 3 новых парсера  (`Image/File_AmigaIcon`, `Video/File_Apv`, `Video/File_Av2`) в манифест (509->515),  дерево и `MediaInfoLib_2022.vcxproj` (+.filters). Патч перегенерирован (172771 б, 110 файлов),  verify 515/0; `vendor-update.py --vendor mediainfolib` -> add=0 update=110 delete=0 unchanged=405.  Закоммичено `9d541b1f` (правки сборочных ошибок + патч — `47f48519`); Debug x64 собран 0/0.
- [ ] При первом реальном прогоне новых вендоров создаются коммиты через  `commit()` (git add → porcelain → commit). Нюанс на Windows при  core.autocrlf=true: у LF-исходников (tar.gz с GitHub) после синка  `git status` может показывать файлы как Modified — это артефакт stat/EOL-представления  (контент байт-в-байт совпадает, проверено hash-object); после `git add`  porcelain чистый и повторный синк даёт «no changes».  Поэтому перед коммитом дерева достаточно `git add` (скрипт делает сам);  уровень безопасной проверки — `git diff --stat` пустой.
- [x] Debug|x64 и Release|x64 (Build + чистый Rebuild solution) — 0 предупреждений/0 ошибок.
- [ ] Дымовой запуск exe на Windows.
## Сборка (правила — только Docker, см. AGENTS.md)

- [ ] `docker build --tag flylinkdc-build:ubuntu26.04 - < Dockerfile`
- [ ] Dockerfile: настроить сборку Windows-библиотек, решить совместимость  ATL/MSVC для кросс-компиляции. Не подменять функционал заглушками.
- [ ] При успешной линковке: проверка импортов `x86_64-w64-mingw32-objdump -p`  и проверка запуска EXE на Windows.
## Аудит апгрейдов вендоров (2026-10-13)

- [x] Проверены upstream-релизы всех вендоров. Актуальны: sqlite 3.54.0, zlib-ng 2.3.90 (develop-pin),  jsoncpp 1.9.8, libtorrent 2.1.2, mediainfolib 26.10, snappy 1.3.1, wtl 10.0.10320,  bzip2 1.0.8, leveldb 1.23, ZenLib 0.4.41. Запинены: selene/openssl.
- [x] maxminddb 1.7.1 -> 1.14.1. upstream уже содержит большинство форк-харденинга  (can_multiply, SSIZE_MAX, sa_family_t, проверки overflow, clang-pragma). Новый патч минимальный:  `#pragma warning(4201)` для MSVC в maxminddb.h; локальный `maxminddb_config.h` (не синкается)  задаёт MMDB_UINT128_IS_BYTE_ARRAY=1 и PACKAGE_VERSION "1.14.1". upstream убрал `projects/VS12/maxminddb_config.h`.  Версия читается теперь из maxminddb_config.h. Debug x64 Rebuild 0/0 (exe 54460416); Release x64 Rebuild 0/0 (exe 20656640).  Закоммичено.
- [x] Удалён неиспользуемый вендор unrar (153 файла + проект UnRAR.vcxproj): не входит ни в  одно решение, не линкуется и не вызывается из кода. Убраны каталог, манифест, патч, запись  в vendor-update.py (14 вендоров), кеш и артефакты. Release x64 Build 0/0. См. HISTORY.md.
- [x] boost 1.88.0 -> 1.92.0. Манифест — по модулям (полные деревья 67 каталогов); новый  `vendor-manifests/boost.txt` = 7713 записей (7677 `boost/` + 36 `libs/`), удалены 98 файлов,  пропавших в 1.92 (regex/v4, smart_ptr/detail, asio/detail и т.п.), + `libs/system/src/error_code.cpp`  (System стал header-only в 1.92). Убраны ссылки на `error_code.cpp` в `boost-regex_2022`,  `MakeDefs`, `client_test`, `test-console` (+.filters/`local_free_on_destruction.hpp`). Патч  `boost.patch` (strtoll) актуализирован; исправлен баг CRLF в рабочей копии патча + добавлен  `.gitattributes` (`vendor-patches/*.patch text eol=lf`). Проверка: dry-run add=1410 update=4283;  повторный — update=2 unchanged=7711 (стабильно). Debug|x64 и Release|x64 Rebuild — 0/0, exit=0.  См. HISTORY.md. Коммит НЕ создан (правило: только по запросу).
- [ ] lua 5.4.9 -> 5.5.1 (мажор 5.5, отложено).
- [ ] Коммит обновления boost (boost/ + манифест/патч/vcxproj/.gitattributes) — по запросу пользователя.
## Рефакторинг client (2026-10-15)
Цель: автономный поиск и исправление реальных дефектов (переполнения, OOB,утечки) в коде `client` без изменения вендорных библиотек. Сборка обязанаоставаться зелёной (W=0 E=0).
- [x] `client\OnlineUser.h`, `client\FavoriteUser.h`, `client\FlyFeatures\GradientLabel.h`  — 32-битные переполнения при переводе в байты (int64) / `wcsncpy` в `GradientLabel`.
- [x] `client\SimpleXMLReader.cpp:398` — `inRange(c, 0, 9)` -> `inRange(c, '0', '9')`  (проверка цифр в XML-объявлении всегда падала).
- [x] `client\MD5Calc.cpp:29` — инвертированное условие `h_File == INVALID_HANDLE_VALUE`  -> `!=` (функция всегда возвращала "").
- [x] `client\HashManager.cpp:425-428` — утечка handle `h` при `GetFileSizeEx == FALSE`.
- [x] `client\FlyFeatures\AutoUpdate.cpp:629` — `outSize = file.m_size + 1` при буфере  `file.m_size`: 1-байтный heap over-read/overflow -> `outSize = file.m_size`.
- [x] `client\FlyFeatures\VideoPreview.cpp` — `string header = buff.data()` читал за  границей буфера при полном recv (без NUL); утечка сокета на ошибке recv.
- [x] `client\nmdchub.cpp` — `connectToMeParse`: OOB/`erase` на пустом `port`;  `searchParse` (и `client\SearchQueue.cpp:251`): `atoi` из недоверенного ответа  хаба -> `TypeModes` вне диапазона (UB в `hasType`/`1 << type`) -> кламп в `TYPE_ANY`.
- [x] `client\AdcCommand.h:232` — `toSID`: `*reinterpret_cast<const uint32_t*>(aSID.data())`  (unaligned-чтение + OOB на строке < 4). Гвард `aSID.size() < sizeof(uint32_t) -> 0` + `memcpy`.  Вызывающие `AdcHub.cpp:500/536/566/580` передают недоверенные ADC-параметры → реальный OOB.
- [x] `client\User.cpp:565` — `Identity::getTag`: `char l_tagItem[128]` + `_snprintf` (не  терминирует при усечении) + `return l_tagItem` → чтение за стеком; `l_version` берётся из  недоверенного тега (`AP`/`VE`) удалённого пользователя. Добавлена явная терминация.
- [x] `client\FlyFeatures\VideoPreview.cpp` — 8 мест `_snprintf(buff.data(), buff.size(), ...)`  в `vector<char>(8192)`/`LocalArray<CHAR,256>` и последующее `buff.data()` как C-строка  (в т.ч. `User-Agent` от клиента) → heap/stack over-read при усечении. Добавлена терминация.
- [x] `client\Client.cpp:842` (`LocalArray<char,200>`) и `:976` (`AutoArray<char>(512)`) —  тот же `_snprintf`-без-`NUL` в логах (ник/hub name/URL недоверенные). Добавлена терминация.
- [x] `client\ConnectivityManager.cpp:86` — `AutoArray<char> buf(512)` + `_snprintf` → терминация.
- [x] `client\CompatibilityManager.cpp:246` — `sprintf_s` возвращает `-1` при усечении →  `temp.resize((size_t)-1)` бросал `std::length_error`; результат проверяется.
- [x] Сборка Debug|x64 (`build_dbg_refactor5.log`) и Release|x64 (`build_rel_refactor5.log`)  — W=0 E=0, exit=0 (локальный MSBuild VS 18 Insiders).
- [x] Отложенные кандидаты проверены — в текущей конфигурации это мёртвый код (фикс не даёт  runtime-эффекта), поэтому не менялись: `HttpConnection::substr` (файл под  `FLYLINKDC_USE_CROOKED_HTTP_CONNECTION`, макрос нигде не определён), `CFlyProfiler`  (только `_M_IX86` + закомментированный `__PROFILER_ENABLED__`), `WinFirewall` COM leaks  (вызывающие под отключённым `FLYLINKDC_USE_SSA_WINFIREWALL`). `PropertyItemImpl.h` `wcsncpy`  признано безопасным (zero-init буферы / cchMax = len+1).
- [x] `C26451` в `flylinkdc.ruleset` — пробовалось, откачено (52 warning в `windows/`, вне scope).
## История / статус прошлых сессий

- [x] Аудит PCH/дублей инклудов: PCH уже оптимален во всех проектах (Create+Use),  дублей фактически нет; убраны 2 повтора в windows/MainFrm.cpp. ClCompile не узкое  место (см. запись в HISTORY.md).
- [x] Ускорение сборки MSVC (Release Rebuild): протестированы MTT, EnableClServerMode,  BuildPassReferences, /CGTHREADS:8 и снятие /PROFILE — все медленнее или без выигрыша  относительно базы 2.16-2.21 мин; стандартные /m + /MP + /LTCG:incremental оптимальны.  Метрики в build-win/build-stats.csv, запись в HISTORY.md.
- [x] Инкрементальная Build vs Rebuild: Build перелинковывает только затронутое (LTCG  «1768 of 152382 functions (1.2%) were compiled»), что даёт 0.47 мин на правку 1 файла  и 0.01 мин на no-op против ~2.2 мин Rebuild. Рекомендация: в повседневной работе  использовать цель Build, Rebuild только для чистых/контрольных прогонов (HISTORY.md).
- [x] WPO (/GL + /LTCG) для Release|x64, /analyze доведён до 0/0 (см. HISTORY.md).
- [x] Сборка Release|x64 в VS 18 Insiders: 0 предупреждений/0 ошибок  (см. HISTORY.md).
## 2026-10-16 (продолжение сессии)
- [x] Найдены и исправлены все небезопасные `_snprintf` (FinishedManager.cpp, ClientManager.cpp, Socket.cpp, UploadManager.cpp, windows/MainFrm.cpp) — добавлена гарантия NUL-терминации в логах.
- [x] Сделан полный аудит всех потенциально опасных мест: `size()-N`, `memcpy`/`strcpy`, `_snprintf`; найден 1 реальный дефект — исправлен в `FinishedManager.cpp`.

## Рефакторинг и поиск ошибок (2026-10-19)
Цель: автономный аудит собственного кода (не вендоров) на реальные дефекты памяти,
логики и безопасности; сборка Debug|x64 и Release|x64 должна остаться зелёной (W=0 E=0).

### Инструменты
- [x] `build-win/analyze.ps1` — прогон MSVC `/analyze` (`RunCodeAnalysis=true`) по решению
  с выгрузкой находок в `build-win/analysis/analyze_<name>_<config>.txt`
  (формат `file|line|rule|message`). Прогон Debug|x64: 255 находок, из них 49 в
  собственном коде `client/` + `windows/`.
- [x] `build-win/fix-todo-newlines.py` — восстановление переводов строк в `TODO.md`
  (файл был записан вообще без переводов строк).

### Найдено и исправлено (память / безопасность)
- [x] `windows/PublicHubsFrm.cpp:240` — **heap overflow на 1 байт**: буфер распаковки bzip2
  имел ровно `size*20` байт, а `l_out[outSize] = 0` писал за его конец, если распакованные
  данные занимали весь буфер. Буфер увеличен на 1 байт + проверка индекса. (HIGH)
- [x] `client/Encoder.cpp:80` (`fromBase32`) — запись `dst[offset]` при `len == 0`
  (соль ADC-хаба длиной 1 символ → `saltBytes = 0` → OOB-запись в 0-байтовый буфер).
  Добавлен гвард `dst == nullptr || len == 0`; `client/AdcHub.cpp:1463` — буфер не нулевой.
- [x] `client/Encoder.cpp:120` (`isBase32`) — `g_base32Table[src[i]]` с `signed char`
  → отрицательный индекс (OOB-чтение) для не-ASCII magnet-хэша; добавлен
  `(unsigned char)`-cast.
- [x] `client/QueueItem.cpp:132` (`calculateAutoPriority`) — деление на `getSize()`,
  которое может быть 0 (размер файла задаёт удалённый пир) → `EXCEPTION_INT_DIVIDE_BY_ZERO`.
  Добавлена проверка `getSize() <= 0`.
- [x] `client/UploadManager.cpp:293` и `:374` — проверка `(start + size) > sz` обходилась
  при `aBytes == -1` и `aStartPos > sz` (отрицательный размер) и при переполнении
  `int64` (`aBytes` близко к `INT64_MAX`). Размер и старт проверяются раздельно.
- [x] `client/SearchManager.cpp:303` — UDP-парсер `$SR` искал `" ("` через `rfind` по всему
  буферу: при подделанном пакете `j < i` → `size_t`-underflow в `substr` и подстановка
  чужого TTH в результат поиска. Поля разбираются от конца сообщения.
- [x] `client/iplist.cpp:291`, `client/PGLoader.cpp:63,141` — `substr(linestart, lineend - linestart - 1)`
  → underflow на пустой строке (весь остаток файла становился одной «строкой» правила);
  при LF-файлах терялся последний символ строки. Разбор переписан (CRLF/LF).
- [x] `client/File.cpp:437,449` (`formatPath`) — `path[1]` на пустой строке (OOB-чтение).
- [x] `client/Util.cpp:3170` (`getTempPath`) — `GetTempPath` возвращает требуемый размер
  буфера при переполнении → чтение за пределами `LocalArray<TCHAR, MAX_PATH>`.
- [x] `client/MD5Calc.cpp:20` (`CalcMD5FromFile`) — handle `CreateFile` не закрывался
  для пустых файлов и файлов > 4 ГБ (`GetFileSize` → `0xFFFFFFFF` → `int` → -1);
  заменён на `GetFileSizeEx`.
- [x] `client/FlyFeatures/flyServer.cpp:2351` — `l_dwBytesAvailable + 1` переполнял `DWORD`
  (0xFFFFFFFF → 0) при размере ответа от сервера.
- [x] `client/AdcHub.cpp:935` — `Util::toUInt32(...) * 8` считалось в 32 битах
  (переполнение → `m == 0`).
- [x] `client/AdcHub.cpp:289` — `*(short*)i->c_str()` (нарушение strict-aliasing +
  невыровненное чтение) заменено на чтение двух байт.
- [x] `client/BufferedSocket.cpp:329` — `substr(8, l_marker_tth - 8)` → underflow
  при коротком `$Search`-пакете; добавлена проверка длины префикса.
- [x] `client/nmdchub.cpp:376` — тег `L:` (`$MyINFO`) конвертировался через `Util::toInt`
  (знаковый) и умножался на 1024 в 32 битах → произвольный лимит от хаба.
- [x] `client/XMLParser/xmlParser.cpp:2291` (`parseFile`) — `(int)_ftelli64(f)` обрезал
  размер файла, `malloc` не проверялся → `fread(NULL, ...)` для файлов > 2 ГБ.
- [x] `windows/WinUtil.cpp:2083` — в `RegSetValueEx` для `DCLST_SHELL_DESC` передавалась
  длина **другой** строки (`MAGNET_SHELL_DESC`).
- [x] `windows/WinUtil.cpp:1991,1996,2000,2086,2090` — размер `REG_SZ` считался как
  `sizeof(TCHAR) * len + 1` вместо `sizeof(TCHAR) * (len + 1)` → значение без
  терминального нуля (нечётная длина).
- [x] `windows/WinUtil.cpp:1785,1837,1879,1921,1969,2045` — `RegQueryValueEx` получал
  размер буфера в символах вместо байт → обрезанное значение команды.
- [x] `windows/PropPage.cpp:202` — кисть `CreateSolidBrush` создавалась на каждое
  сообщение `WM_CTLCOLOR*` → утечка GDI-объектов (сотни в секунду). Кисть переиспользуется.
- [x] `windows/BarShader.h:209` — ключ кэша градиента не учитывал флаг `bLight`
  (плоский градиент и «объёмный» кэшировались под одним ключом).
- [x] `windows/CheckTargetDlg.cpp:32,44`, `windows/SpyFrame.cpp:354` — `_snwprintf` без
  гарантированного NUL → чтение за пределами буфера.

### Найдено и исправлено (безопасность цепочки обновления)
- [x] `client/FlyFeatures/AutoUpdate.cpp` — загруженный пакет обновления **не проверялся
  по TTH** из подписанного манифеста, а скачивался по обычному HTTP → MITM мог подменить
  payload (произвольный код, устанавливаемый `FlyUpdate.exe`). Добавлена проверка TTH
  скачанного файла (`Util::getTTH_MD5`) до записи на диск; при несовпадении файл удаляется.
- [x] `client/FlyFeatures/AutoUpdate.cpp` — распаковка zip считала признаком успеха флаг
  «есть ещё данные» (`UnZFilter` возвращает `Z_OK` при незавершённом потоке) → zip-обновления
  не устанавливались, а обрезанный payload принимался за корректный. То же для bzip2
  (результат разжатия игнорировался). Буфер на 1 байт больше ожидаемого размера.
- [x] `client/FlyFeatures/AutoUpdate.cpp` — размер файла из манифеста не проверялся
  (`new byte[m_size]` с отрицательным/огромным значением → `bad_alloc` без обработки).
- [x] `client/FlyFeatures/AutoUpdate.cpp` — путь и имя файла из манифеста не проверялись
  (path traversal → запись файла вне папки обновления). Добавлена проверка
  `isSafeRelativePath()`.
- [x] `client/FlyFeatures/AutoUpdate.cpp:322` — инвертированный флаг
  `l_is_invalid_sign_update = false` при неуспешной проверке подписи второго манифеста.
- [x] `client/FlyFeatures/flyServer.cpp` — конфиг fly-server (обычный HTTP, без подписи):
  невалидный `regex_find_ip` приводил к необработанному `std::regex_error` → падение.
  Добавлена валидация регулярки при загрузке конфига и `try/catch` в `windows/HubFrame.cpp`.
- [x] `client/FlyFeatures/flyServer.cpp` — список расширений `mediainfo_ext` из того же
  конфига подставлялся в SQL без экранирования → SQL-инъекция в локальную БД.

### Найдено и исправлено (COM / прочее)
- [x] `client/FlyFeatures/WinFirewall.cpp` — HRESULT `get_GloballyOpenPorts` не проверялся,
  указатель оставался неинициализированным → разыменование мусора (падение).
  Переписано на RAII-обёртку `CFlyComPtr` — заодно устранена утечка COM-ссылок
  (2..5 ссылок на каждый запрос).
- [x] `client/Client.cpp:401` — при несовпадении keyprint TLS-соединение не закрывалось:
  сессия с недоверенным пиром оставалась открытой.

### Проверка сборки
- [x] Debug|x64 Build — 0 предупреждений / 0 ошибок после каждого пакета правок.
- [ ] Финальный контрольный прогон: Debug|x64 **Rebuild** + Release|x64 **Rebuild** (0/0).
- [ ] Дымовой запуск EXE на Windows (требует GUI, вручную).
- [ ] Проверить обновление с реального сервера (исправленный путь проверки TTH).

### Отложено
- [ ] `windows/TreePropertySheet.cpp:212` (`doCamShoot`) — утечка GDI-объектов и
  `DeleteDC()` вместо `ReleaseDC()`. Код под неактивным макросом
  `SCALOLAZ_PROPPAGE_CAMSHOOT` → не компилируется, правка без проверки сборкой.
- [ ] Подпись манифеста обновления — SHA-1 + RSA-1024 (`client/FlyFeatures/AutoUpdate.cpp:780`).
  Требуется перевыпуск ключа на стороне сервера, менять только вместе с ним.
- [ ] `client/FlyFeatures/flyServer.cpp` — конфиг и обновления по HTTP: переход на HTTPS.

### Продолжение сессии 2026-10-19 (вторая половина)

#### Найдено и исправлено (память / безопасность)
- [x] `client/Util.cpp:1057` — буфер IDNA (`LocalArray<char, MAX_HOST_LEN>`) не оставался
  NUL-терминированным, если хост в ACP-конвертации был длиннее 255 байт → чтение за
  пределами стекового буфера в `IDNA_convert_to_ACE()`. Хост берётся из URL хаба
  (сети) и из `Favorites.xml`.
- [x] `client/SettingsManager.cpp` — настройки `*_COLUMNS_SORT` (`HUBFRAME_`,
  `SEARCH_`, `QUEUE_`, `FINISHED_DL/UL_`, `UPLOAD_QUEUE_`, `USERS_`,
  `HUBS_PUBLIC/FAVORITES/RECENTS_`, `DIRLIST_`, `TRANSFERS_`) не проверялись и
  использовались как индекс колонки (обрезается до `uint8_t`) → выход за границу
  массива колонок окна. Добавлена проверка диапазона `-1..31` + проверка в
  `windows/TypedListViewCtrl.h` (`setSortColumn`, `findColumn`, `getRealSortColumn`)
  и в `windows/ExListViewCtrl.cpp` (`insert`).
- [x] `client/FavoriteManager.cpp:1368` — `HeaderSort` из `Favorites.xml` (файл
  приходит и с ISP-сервера) не проверялся и использовался как индекс колонки.
- [x] `client/FavoriteManager.cpp:1077` — разыменование `nullptr`: хабы, помеченные
  в ISP-файле как `ISPDelete="1"`, отсутствуют в `g_favoriteHubs`.
- [x] `client/FavoriteManager.cpp:1982` (`removeallRecent`) — утечка всех
  `RecentHubEntry` (в векторе хранятся указатели).
- [x] `client/HashManager.cpp:644` — в fallback-ветке (когда `VirtualAlloc` не
  удался) выделялся **один** буфер `g_HashBufferSize`, а `fastHash()` пишет во вторую
  половину → переполнение heap на 16 МБ. Буфер теперь `g_HashBufferSize * 2`,
  добавлена проверка в `fastHash()`.
- [x] `client/HashManager.cpp:84` — `blockSize` из NTFS-Alternate-Stream (`.gltth`)
  не проверялся → деление на ноль или гигантский `AutoArray` → `bad_alloc` без
  обработки. Добавлена проверка + защита в `client/MerkleTree.h` (`calcBlocks`).
- [x] `client/SSLSocket.cpp:70,149` — при неудачном `SSL_new()` `checkSSL()` не
  бросает исключение (пустой handle) → разыменование `nullptr` в `SSL_set_verify()`.
- [x] `client/CFlylinkDBManager.cpp:4458` — размер blob'а дерева TTH обрезался до
  `int` (переполнение → `NULL` в БД) + `getLeaves()[0]` без проверки на пустоту.
- [x] `client/SSL.h:73` — `scoped_handle::operator=(scoped_handle&&)` не освобождал
  прежний ресурс → утечка OpenSSL-объектов.
- [x] `client/DownloadManager.cpp:668` — `+ " Error = " = e.getError()`: присваивание
  вместо конкатенации → терялся весь контекст ошибки.
- [x] `client/DownloadManager.cpp:845` — отсутствующий `break` в
  `case SEV_RECOVERABLE` → неизвестная восстановимая ошибка пира обрабатывалась как
  «успех» и соединение не закрывалось.
- [x] `client/SettingsManager.cpp:1720` (`COLOR_AVOIDING`) — сложение `int` из
  файла настроек переполнялось (UB) и отрицательное значение принималось.
- [x] `client/SimpleXML.cpp:249` (`fromXML`) — при исключении парсинга объект
  оставался с итератором на освобождённом потомке (use-after-free при
  последующем `getChildData()`).
- [x] `client/CompatibilityManager.cpp:172` — `FreeLibrary()` для модуля, полученного
  через `GetModuleHandle()` (счётчик ссылок не увеличивался) → риск выгрузки
  `ntdll.dll`.
- [x] `client/CompatibilityManager.cpp:1082,1166` — перебор дисков начинался с
  `C:` при бите 0 = `A:` → неверный учёт сетевых дисков в статистике.

#### Найдено и исправлено (windows/UI)
- [x] `windows/SearchFrm.cpp:2994` — `(i - 1) % size` даёт `-1` при `i == 0`
  → чтение `wnds[-1]` и `SetFocus()` с мусорным HWND (обычное Shift+Tab из строки
  поиска).
- [x] `windows/BarShader.cpp:520` — цикл `_x <= w` писал пиксель за правой границей
  DIB (в соседней ветке — `_x < w`).
- [x] `windows/TypedListViewCtrl.h:230` — `wcsncpy` без NUL-терминации в буфер
  `LVN_GETDISPINFO` (размер буфера задаёт common control) → чтение за его границей.
- [x] `windows/UsersFrame.cpp:307` — `SETTING(FAVUSERLIST_DBLCLICK)` из
  пользовательского конфига использовался как индекс массива без проверки.
- [x] `windows/PublicHubsFrm.cpp:385` — `-1` при разборе строк ISP-списка: терялся
  последний символ строки и происходил underflow на пустой строке.
- [x] `windows/QueueFrame.cpp:1256` — `CheckMenuItem()` с id-команды, но без
  `MF_BYCOMMAND` (трактовалось как позиция) → текущее значение не отмечалось.
- [x] `windows/SearchFrm.cpp:3448` — пропущена запятая: `"720p" "x264"` склеивалось в
  `"720px264"`, категория `x264` не создавалась.

#### Найдено и исправлено (потоки)
- [x] `client/QueueManager.cpp:393` (`userIsDownloadedFiles`) — `g_userQueueMap`
  читалась без блокировки, хотя писатели держат `QueueItem::g_cs` (рекурсивный
  мьютекс) → гонка с `insert()` (перехеширование карты).
- [x] `client/QueueItem.h` — `isWaiting()` и `getDownloadsSegmentCount()` читали
  `m_downloads` без `m_fcs_download`.
- [x] `client/QueueItem.cpp:329` (`isFinished`) — `m_done_segment.size()` читалось
  до захвата `m_fcs_segment`.
- [x] `client/UploadManager.h:115` (`getUploadCount`) и `client/UploadManager.cpp:1215`
  — `g_uploads.size()` читалось без `g_csUploadsDelay`.
- [x] `client/CFlylinkDBManager.h:780` (`get_tth_cache_size`) — размер кэша читался
  без `g_tth_cache_cs`.
- [x] `client/AdcHub.h:45`, `client/nmdchub.h:57` (`getUserCount`) — размер списка
  пользователей читался без `m_cs`.
- [x] `client/ShareManager.h:440` — размеры кэшей читались без своих блокировок.
- [x] `client/FlyFeatures/flyServer.cpp:2532,2599,2676` — `.empty()` на
  счётчиках fly-server читался без блокировки (писатели — потоки хабов).

#### Проверка сборки
- [x] Debug|x64 **Rebuild** — 0 предупреждений / 0 ошибок, exit=0 (EXE=54887936).
- [x] Release|x64 **Rebuild** — 0 предупреждений / 0 ошибок, exit=0 (EXE=20752896).
- [ ] Дымовой запуск EXE на Windows (требует GUI, вручную).
- [ ] Проверить обновление с реального сервера (исправленный путь проверки TTH).

#### Отложено (требует архитектурной переработки)
- [ ] `client/OnlineUser.h:162` (`Identity::setNick`) — ник пира пишется без
  блокировки, а `getNick()` возвращает `const string&` → use-after-free при смене
  ника. Исправление требует возврата значения по копии (97 мест вызова,
  `Client::getMyNick()` тоже возвращает ссылку) — риск регрессий слишком велик
  для автоматического применения.
- [ ] `client/CFlylinkDBManager.cpp:227` (`errorDB`) — модальный `MessageBox` и
  HTTP-POST выполняются под глобальной блокировкой БД → блокировка всех операций с
  БД до реакции пользователя. Требуется вынести отчёт за пределы блокировки.
- [ ] `client/ShareManager.cpp:985,1727` — полное сканирование ФС выполняется под
  эксклюзивной блокировкой шары → хабы перестают отвечать на `$Search` на время
  сканирования. Требуется разделить «сбор дерева» и «публикацию».
- [ ] `client/ConnectionManager.cpp:1369` — сырой `Client*` используется после
  освобождения блокировки списка клиентов → UAF при отключении хаба.
- [ ] `windows/TreePropertySheet.cpp:212` (`doCamShoot`) — утечка GDI-объектов и
  `DeleteDC()` вместо `ReleaseDC()`. Код под неактивным макросом
  `SCALOLAZ_PROPPAGE_CAMSHOOT` → не компилируется.
- [ ] Подпись манифеста обновления — SHA-1 + RSA-1024
  (`client/FlyFeatures/AutoUpdate.cpp:780`). Требуется перевыпуск ключа на стороне
  сервера, менять только вместе с ним.
- [ ] `client/FlyFeatures/flyServer.cpp` — конфиг и обновления по HTTP: переход на
  HTTPS.

### ВАЖНО: кодировка исходников (CP1251) и инструменты правки
- [x] Русские комментарии в `client/` и `windows/` хранятся в **CP1251** (это не валидный
  UTF-8!). Любой инструмент, который читает файл как UTF-8 и записывает его обратно,
  заменяет байты `0x80..0xFF` на `U+FFFD` (EF BF BD) — комментарии портятся, а
  `git diff` показывает «невидимые» изменения в сотнях строк.
- [x] Восстановлено: `build-win/repair-cp1251.py` — по pristine-копии из git
  (`git show HEAD:<file>`) восстанавливает оригинальные байты всех повреждённых строк
  (сопоставление по «скелету» строки: ASCII-часть + маркер не-ASCII-участка).
  Прогон после правок: 654 строки восстановлены, 1 строка — вручную
  (`client/CompatibilityManager.cpp`, там менялась буква диска рядом с русским комментарием).
- [ ] **Правило на будущее**: правки исходников выполнять так, чтобы байты CP1251
  сохранялись — например, скриптом на Python в режиме `rb`/`wb` (замена ASCII-строк
  побайтово) либо `git apply` с патчем, в котором только ASCII-строки. После правок
  проверять `git diff --stat`: изменённых строк должно быть столько же, сколько
  реально затронуто.
- [x] Проверено: `git diff --stat` после восстановления — 1066 добавлений / 187
  удалений в 51 файле (только реальные правки), сборка Debug|x64 и Release|x64 — 0/0.

### Итог сессии 2026-10-19
- [x] Инструменты: `build-win/analyze.ps1` (MSVC `/analyze`), `build-win/repair-cp1251.py`
  (восстановление комментариев CP1251), `build-win/update-todo*.py`,
  `build-win/update-history*.py`, `build-win/fix-todo-newlines.py`.
- [x] Найдено и исправлено **53 дефекта** в собственном коде `client/` и `windows/`
  (память, безопасность цепочки обновления, гонки потоков, логика, реестр, GDI).
  Полный список — в `HISTORY.md`, раздел «2026-10-19».
- [x] Количество находок `/analyze` в собственном коде: 49 → 26 (оставшиеся —
  теоретические `_snprintf` без терминации и стилистические замечания).
- [x] **Контрольная сборка**: Debug|x64 Rebuild — 0 предупреждений / 0 ошибок
  (EXE=54883840); Release|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=20748800).
- [x] `git diff --stat`: 51 файл, изменения только по существу (комментарии CP1251
  восстановлены, `git diff` не показывает «мусорных» строк).
- [x] Дымовой запуск EXE на Windows (Release|x64, 2026-10-19 20:58 и 21:12): процесс жив 20 с,
  32 потока, 119 МБ ОЗУ, подключение к хабам (хабы fly-server.ru, dchub.one, cifracom),
  новых дампов нет. В `System.log` — только безобидная ошибка (несуществующая папка
  `Settings\Certificates\`, создаётся по мере надобности), без исключений и assert-ов.
- [x] В повторном запуске (после восстановления компиляции) клиент скачал файл с хаба («1976-06.djvu») — проверка протокола работает в рабочем режиме.
- [ ] Не выполнено (нужен GUI/окружение): реальная проверка автообновления с сервера
  (с исправленной проверкой TTH), тест в тёмной теме.

- [x] **Финальная контрольная сборка** (2026-10-20 02:31-02:36):
  Debug|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=54908928);
  Release|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=20761600).
- [x] **Финальный дымовой запуск** (Release|x64): процесс жив 30 с, 32 потока, 124 МБ ОЗУ,
  подключение к хабам, проброс портов через UPnP, новых дампов нет, в `System.log` нет
  исключений и assert-ов.
- [x] **В репозитории не осталось ни одного повреждённого байта U+FFFD** (проверены все
  отслеживаемые файлы).

### Восстановление комментариев CP1251 по всему репозиторию (2026-10-19)
- [x] Проверено: коммит `32a9e4ed` («refactor: defensive _snprintf/strcpy/memcpy fixes»)
  записал 7 файлов с уже повреждёнными байтами (U+FFFD вместо CP1251):
  `windows/MainFrm.cpp`, `client/ClientManager.cpp`, `client/OnlineUser.h`,
  `client/Socket.cpp`, `client/FinishedManager.cpp`, `client/idna/idna.cpp`,
  `client/ADLSearch.h`. Повреждение попало в репозиторий (коммит), а не появилось
  в этой сессии.
- [x] Восстановлено через `build-win/repair-cp1251.py`: строки сопоставляются по
  «скелету» (ASCII-часть + маркер не-ASCII-участка) с эталонными ревизиями
  (`32a9e4ed^`, `3f329713`, `58ebbbf8`), оригинальные байты CP1251 возвращаются.
  Итог: **0 байт U+FFFD во всём репозитории** (проверены все отслеживаемые
  `.cpp/.h/.rc/.xml/.txt/.md`).
- [x] Сборка после восстановления: Debug|x64 Rebuild — 0/0 (EXE=54883840),
  Release|x64 Rebuild — 0/0 (EXE=20748800).
- [x] `client/ADLSearch.h:21` — единственная строка, которую не удалось сопоставить
  автоматически (имя автора с латинской «ö», испорчено ещё в 2024 г.):
  восстановлена вручную (0xF6 в CP1251).

### Продолжение сессии 2026-10-19 (третья часть, ~4 ч)

#### Найдено и исправлено (windows/UI)
- [x] `windows/FlyUpdateDlg.cpp:90` и `windows/AboutLogDlg.h:53` — **переполнение стека**:
  `EM_GETTEXTRANGE` пишет в буфер `LocalArray<TCHAR, MAX_PATH>` строку произвольной длины
  (длина берётся из диапазона, а буфер — 260 символов). В `FlyUpdateDlg` текст приходит из
  скачанного по HTTP описания обновления → удалённый сервер (или MITM) присылает ссылку
  длиннее 259 символов → переполнение стека. Буфер выделяется по длине диапазона.
- [x] `windows/PopupDlg.h:149` — кисть `CreateSolidBrush` создавалась на каждое сообщение
  `WM_CTLCOLORSTATIC` → утечка GDI-объектов (всплывающие окна создаются постоянно).
- [x] `windows/SharePage.cpp:234` — сравнивался буфер с путём каталога вместо его имени
  (`vName` не использовался) → переименование шары выполнялось всегда, даже если имя
  не изменилось.
- [x] `windows/FolderTree.cpp:129` — после `NetApiBufferFree` указатель не обнулялся →
  **двойное освобождение** при неудачном `NetShareEnum`.
- [x] `windows/dclstGenDlg.cpp:366` — разыменование `_tth.get()`, когда TTH не рассчитан
  (файл не удалось прочитать) → падение.
- [x] `windows/RangesPage.cpp:105` — `LB_GETTEXT` не пишет терминатор → в строке оставался
  встроенный NUL → адрес не разбирался и кнопка «удалить» ничего не делала.
- [x] `windows/MessagePanel.cpp:271` — два раза проверялся `SHOW_EMOTIONS_BTN` вместо
  `SHOW_MULTI_CHAT_BTN` → панель сообщений рассчитывалась неправильной ширины.
- [x] `windows/EmoticonsDlg.cpp:118` — скрытый модальный диалог (нет эмотиконов) →
  **зависание всего клиента**.
- [x] `windows/SoundsPage.cpp:173` — `LVITEM` с `LVIF_TEXT` и `pszText == NULL` →
  запись по нулевому указателю (кнопка «Play»).
- [x] `windows/SoundsPage.cpp:212` — `GetSelectionMark()` возвращает -1, если элемент не
  помечен → `g_SoundsTable[-1]` (OOB).
- [x] `windows/PortalBrowser.cpp:456` — отрицательный код ошибки от DLL использовался как
  индекс массива → OOB-чтение.
- [x] `windows/PortalBrowser.cpp:213` — `FreeLibrary` вызывался дважды (деструктор и
  обработчик ошибки) → двойное выгрузка модуля.
- [x] `windows/AdvancedPage.cpp:101` — результат `SetDlgItemText` (BOOL) присваивался
  строке → формат iTunes затирался одним символом.
- [x] `windows/Players.cpp:96` — в поле «жанр» попадало название трека (copy-paste) и
  BSTR жанра не освобождался.
- [x] `windows/Players.cpp:266` — `WideCharToMultiByte` в неинициализированный буфер →
  чтение за пределами кучи при ошибке конвертации.
- [x] `windows/Players.cpp:466` — лишний `AddRef()` у COM-объекта → утечка на каждый
  «now playing».
- [x] `windows/PropPageTextStyles.h:79` — кисть сохранялась в свойстве окна, старая
  не удалялась → утечка GDI; на `WM_DESTROY` свойство тоже не удалялось.
- [x] `windows/PropPageTextStyles.cpp:298` — имя шрифта писалось только в редактируемый
  стиль, а charset/размер — во все → стили чата становились непоследовательными.
- [x] `windows/JAControl.cpp:150` — часы делились на 360 вместо 3600 → «10:00:00» вместо
  «1:00:00» в спаме Winamp.
- [x] `windows/PrivateFrame.cpp:86` — ключ счётчика личных сообщений отличался от ключа
  увеличения/уменьшения → счётчик никогда не уменьшался, и пользователю блокировались
  личные сообщения (>10) навсегда. Ключи унифицированы, уменьшение проверяется.
- [x] `windows/PopupsPage.cpp:167,183` — шрифт, выбранный в диалоге, терялся (сохранялся
  исходный `LOGFONT`) → шрифт всплывающих окон нельзя было изменить.

#### Найдено и исправлено (client/)
- [x] `client/ConnectionManager.cpp:808` — **прослушивающий сокет пересоздавался каждые
  250 мс**: новый сокет не получал `SO_REUSEADDR`, поэтому после отключения любого пира
  (порт в `TIME_WAIT`) `bind()` падал и слушатель не работал минутами. Сокет
  пересоздаётся только после реальной ошибки.
- [x] `client/CFlylinkDBManager.cpp:2142` — инвертированный `memcmp`: `l_is_need_update`
  становилось истинным, когда данные **не** изменились → данные возобновления торрента
  никогда не обновлялись в БД.
- [x] `client/CFlylinkDBManager.cpp` — 14绑定 `p_nick` (`User::m_nick`) со `SQLITE_STATIC`:
  SQLite не копирует строку, а ник меняется потоком хаба → **use-after-free** в БД.
  Заменено на `SQLITE_TRANSIENT`.
- [x] `client/ClientManager.cpp:1311` — инвертированный `dcassert` (срабатывал при каждом
  обновлении ника) → падение отладочной сборки.
- [x] `client/ConnectionManager.cpp:1421` — `setErrors(0)` выполнялось для **всех** файлов в
  очереди → сбрасывался флаг «ошибка протокола» (-1) у других пиров.
- [x] `client/ClientManager.cpp:481` (`findHub`) — порт из недоверенного ответа UDP-поиска
  обрезался до 16 бит (порт 0 → поиск ника во всех хабах). Добавлена проверка диапазона.
- [x] `client/ClientManager.cpp` — сырой `Client*` использовался после освобождения
  блокировки списка клиентов → UAF. Добавлен `ClientManager::completeAutodetect()`,
  который работает под блокировкой списка.
- [x] `client/SettingsManager.cpp:1338` — `RegQueryValueEx` без проверки результата →
  буфер без NUL → чтение за пределами стека.
- [x] `client/MappingManager.cpp:89` — при неизвестном статусе порта терялась вся
  информация о порте.
- [x] `client/WildcardsReg.cpp:180` — `size_t`-underflow в `substr` → `std::out_of_range`.
- [x] `client/StrUtil.h` — переполнение знакового типа в `stringToInt` (UB) → насыщение.
- [x] `client/TransferData.cpp:40` — узкая строка `""` передавалась в `%s` широкого
  `_snwprintf` → чтение за пределами литерала.
- [x] `client/MerkleCheckOutputStream.h:35` — деление на размер блока дерева, который может
  быть 0 → `EXCEPTION_INT_DIVIDE_BY_ZERO`.
- [x] `client/Segment.h:105` — `contains()` сравнивал концы через `==` вместо `>=`.
- [x] `client/HashValue.h:118` — `hash<HashValue<T>*>` хешировал данные за указателем, а
  `equal_to` сравнивал значения → несогласованная пара для контейнера.
- [x] `client/OnlineUser.h` / `client/User.h` — ник пира (`Identity::m_user_nick`,
  `User::m_nick`) изменялся без блокировки, а `getNick()` возвращал ссылку → **use-after-free**
  при смене ника. Добавлены per-object рекурсивные мьютексы, геттеры возвращают копию.
  Блокировка ника не удерживается при обращении к БД (порядок блокировок не инвертирован).
- [x] `client/FlyFeatures/CProgressDlg.h` — рабочий поток мог пережить диалог (use-after-free)
  и потерять handle потока. Добавлены heap-прокси интерфейса прогресса с блокировкой и
  ожидание потока в деструкторе.
- [x] `client/FlyFeatures/MiniHTML.h` — переполнение `szTag[16]` длинным тегом, `lstrcpy` в
  `lfFaceName[32]`, чтение за пределами стека при закрывающем теге без открывающего и
  бесконечный цикл на неизвестном HTML-entity.
- [x] `client/FlyFeatures/PropertyItemImpl.h` — `ATLTRY` проглатывал `bad_alloc` (указатель
  оставался неинициализированным), `wcsncpy` без NUL, `_alloca` с длиной из удалённых
  данных (переполнение стека UI-потока).
- [x] `client/CFlylinkDBManager.cpp` (`errorDB`) — модальный `MessageBox` показывался под
  глобальной блокировкой БД → все операции с БД зависали до реакции пользователя. Диалог
  теперь показывается только когда БД неработоспособна, остальные ошибки пишутся в лог.

#### Рефакторинг
- [x] `windows/WinUtil.cpp` — четыре почти одинаковые функции регистрации обработчиков
  протоколов (`dchub`, `nmdcs`, `adc`, `adcs`, ~40 строк каждая) заменены одной общей
  `internalRegisterUrlProtocolHandler()` (~120 строк дублирования удалено). Именно из-за
  дублирования размер `REG_SZ` считался по-разному в копиях, и ошибка была исправлена
  только в одной из них.
- [x] `client/StrUtil.h`, `client/OnlineUser.h`, `client/User.h` — имена локальных переменных
  и параметров приведены к соглашениям из `AGENTS.md` (`l_`, `p_`, `m_`).

#### Инструменты
- [x] `build-win/patch.py`, `build-win/apply-patches.py` + `build-win/patches-*.json` —
  побайтовое применение правок (обычный редактор портит комментарии CP1251).
- [x] `build-win/refactor-winutil-registry.py` — рефакторинг регистрации протоколов.
- [x] `build-win/repair-cp1251.py` — восстановление комментариев CP1251 (см. выше).

### Продолжение сессии 2026-10-20 (четвёртая часть)

#### Найдено и исправлено (клиент)
- [x] `client/DownloadManager.cpp:607` — **удалённый DoS**: пир мог объявить дерево без
  единого leaf'а (`bytes == 0`), тогда `bl * 0 < fileSize` всегда истинно и цикл
  `bl *= 2` крутился вечно (100 % CPU в потоке загрузки). Добавлена проверка пустого
  дерева с корректным завершением загрузки.
- [x] `client/QueueManager.cpp:1320` (`checkTarget`) — `aTarget[1]`/`aTarget[2]` без
  проверки длины → чтение за пределами строки (пустой атрибут `Target` в файле очереди).
- [x] `client/UploadManager.cpp:92` — проверка `getSocket()` выполнялась **после**
  разыменования `getSocket()->getIp()`.
- [x] `client/SettingsManager.cpp:1912` — файл настроек заменялся неатомарно
  (`delete` + `rename`): падение между ними оставляло клиент вообще без настроек.
  Заменено на `MoveFileEx(MOVEFILE_REPLACE_EXISTING)`.
- [x] `client/CFlylinkDBManager.cpp:1843` (`is_download_tth`) — общая подготовленная
  SQLite-статмента использовалась из нескольких потоков **без** блокировки БД.
- [x] `client/CFlylinkDBManager.cpp:2155` — `&p_resume[0]` на пустом векторе (UB).
- [x] `client/CFlylinkDBManager.cpp:3562` (`is_table_exists`) — имя таблицы
  подставлялось в SQL конкатенацией → потенциальная SQL-инъекция; заменено на
  параметр.
- [x] `client/CFlylinkDBManager.cpp:1333` — счётчик неотправленных статистик
  обнулялся и при недоступном сервере → записи терялись безвозвратно.
- [x] `client/HashManager.cpp:136` — `getLeaves()[0]` на пустом векторе (файл меньше
  размера блока).
- [x] `client/CFlyThread.cpp:121` (`Thread::start`) — handle потока сохранялся только
  при успехе первой попытки создания; при успехе второй handle терялся → поток не
  присоединялся, handle утекал.
- [x] `client/FlyFeatures/flyServer.h:436` — деструктор `CFlyServerAdapter` не
  присоединял поток запросов → поток жил дольше адаптера (use-after-free).
- [x] `client/FlyFeatures/fuSearch.cpp:34` — `Scan()` всегда возвращал `false`,
  игнорируя результат `ProcessFolder()`.

#### Найдено и исправлено (windows)
- [x] `windows/flattabctrl.h:812` (`SwitchTo`) — `m_active` может быть `nullptr`
  (после закрытия последней вкладки) → разыменование `nullptr` по кнопкам «назад/вперёд».
- [x] `windows/DirectoryListingFrm.h:627` — `l_filename.length() - 8` при длине имени
  меньше 8 → underflow и чтение за пределами кучи (файл с именем `files`).
- [x] `windows/MainFrm.cpp:2584` — таймер главного окна уничтожался до подтверждения
  выхода: если пользователь отменял выход, таймер больше не работал (не обновлялись
  скорость/часы/подсказка трея).
- [x] `windows/MainFrm.cpp:1519` — `CB_ERR` (-1) сравнивался с `uint32_t` → проверка
  ошибки всегда истинна; при ошибке `new _TCHAR[0]` и переполнение кучи.
- [x] `windows/MainFrm.cpp:2271` — `_tcsncpy` без NUL-терминации в буфер подсказки.
- [x] `windows/DirectoryListingFrm.cpp:1485` — пункт меню «избранные каталоги»
  индексировал список целей очереди (другая база id) → файл скачивался не туда.
- [x] `windows/DirectoryListingFrm.cpp:1307` (и 585, 1800) — `dl->getUser()` может быть
  `nullptr` (локальный список) → разыменование `nullptr`.
- [x] `windows/DCLSTPage.cpp:66` — индекс комбо «Ask/Search/Download/Open» не
  соответствовал значениям настройки `DCLST_ACTION` (сдвиг на единицу) → выбор
  «Download» сохранялся как «скачать и открыть».
- [x] `windows/TreePropertySheet.cpp:269` — `LoadLibrary("user32")` при каждом движении
  ползунка прозрачности без `FreeLibrary` → рост счётчика ссылок модуля.
- [x] `windows/Players.cpp` — четыре `BSTR` от iTunes (`album`, `artist`, `name`,
  `version`) не освобождались → утечка на каждый `/itunes`.
- [x] `windows/PopupManager.cpp` — `LoadLibrary("user32")` без `FreeLibrary` на каждое
  всплывающее окно; старый битмап заменялся без удаления.
- [x] `windows/ChatCtrl.cpp` — `IStorage`/`ILockBytes` перезаписывались без
  `Release()` на каждом смайлике; `IOleClientSite`/`IRichEditOle` не освобождались
  при неудачной вставке смайлика.
- [x] `windows/SearchFrm.h:956` — `m_search`/`m_wnd` перезаписывались, пока
  предыдущий поток поиска их читал (гонка данных).
- [x] `windows/SearchFrm.cpp:218` — потоки поиска торрентов не присоединялись в
  деструкторе → работали после уничтожения окна (use-after-free).
- [x] `windows/WaitingUsersFrame.cpp:547` — граница цикла обновления видимых строк
  не учитывала `l_top_index` → часть строк не обновлялась.
- [x] `windows/FinishedFrameBase.h:1039` — массив ширин колонок был на один элемент
  короче → колонка «сетевой трафик» имела ширину 10 пикселей.

#### Рефакторинг
- [x] `client/Text.h` — две перегрузки `tformat` (на 1 и на 3 аргумента) заменены
  одной вариативной: `Text::tformat` теперь поддерживает любое число аргументов, а
  вызовы вида `buf.resize(_snwprintf(...))` (при усечении `resize(-1)` → исключение)
  заменены на безопасный `Text::tformat` в `client/User.cpp` и `windows/QueueFrame.cpp`.
- [x] `windows/WinUtil.cpp` — регистрация обработчиков протоколов (4 копии) заменена
  одной общей функцией (см. выше).
- [x] `client/Util.cpp` (`formatExactSize`) — `_snwprintf` писал строку в себя же
  (перекрывающиеся буферы, UB) → отдельный буфер результата.
- [x] `client/DirectoryListing.cpp` — оператор «запятая» в предикате заменён явным
  кодом.

#### Инструменты
- [x] `build-win/patches-*.json` + `build-win/apply-patches.py` — набор побайтовых
  правок (по 36 наборов за сессию).

### Исправление насыщения при разборе чисел (2026-10-20, перед пушем)
- [x] `client/Util.h` — `Util::toInt()`/`toInt64()` использовали `atoi()`/`_atoi64()`:
  значение строки вне диапазона типа — это **неопределённое поведение** (переполнение
  знакового типа). Теперь значение **насыщается** на границах диапазона
  (`INT_MAX`/`INT_MIN`/`INT64_MAX`/`INT64_MIN`). Проверено отдельным тестом
  (`build-win/check-strutil.cpp`).
- [x] Найдена и исправлена ошибка в первом варианте исправления: знаковый бит типа
  вычислялся сдвигом вправо (`>> 31` → 1 вместо 0x80000000), из-за чего **любое**
  значение насыщалось до нуля. Ошибка поймана модульным тестом, а не сборкой.
  Итог: `build-win/check-strutil.cpp` — все проверки проходят; значения в пределах
  диапазона не изменились.
- [x] `client/StrUtil.h` — тот же алгоритм (файл сейчас никем не подключается, но
  приведён в соответствие).
- [x] `client/Text.h` — новая вариативная `tformat()` проверена отдельным тестом
  (`build-win/check-tformat.cpp`): 1..4 аргумента, усечение результата.
- [x] Сборка: Debug|x64 — 0/0, Release|x64 — 0/0.
