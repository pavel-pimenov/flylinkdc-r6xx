# История изменений FlylinkDC++

## 2026-10-12
### Скрипт обновления вендорных библиотек vendor-update.py
- Реализован `vendor-update.py`: скачивает вендорные исходники с upstream-URL, синхронизирует их в дерево проекта, заново накладывает локальные патчи и делает git-commit. Конфиг — таблица `VENDORS` (url/pin, локальная директория, режим импорта файлов, список патчей, маркер версии). Режимы: `--list`, `--dry-run`, `--no-commit`, `--vendor NAME`. Артефакты архивируются в `build-win/vendor-cache` (в .gitignore).
- SQLite: `client/sqlite` обновлён с 3.50.4 до 3.54.0 (амолгамация `sqlite-amalgamation-3540000.zip`), закоммичено `45872dc1`. Локальные патчи проекта (в sqlite3.c: `#include "sqlite_fly.h"` и `#pragma warning(disable: 4127/4244/4100)`; в sqlite3.h: `#pragma once` и `#include "sqlite_fly.h"`) описаны в конфиге и заново применены к новой версии.
- WTL: выверено, что ветка 10.0.10320 с 5 локальными патчами (atlcrack.h — инициализация `m_bMsgHandled`; atldlgs.h — default-аргумент `0U` у `CPropertyPageImpl`/`CPropertyPage`; atlmisc.h — `_T("file://")`; atlribbon.h — include через кавычки; atlsplit.h — `m_nPropMax = 10000` и разбиение расчёта позиции) даёт 1:1 воспроизведение дерева, коммит не требуется.
- zlib-ng: пинирован на develop HEAD `f18efb822a2fc24a1209a836d51bb217476fc08e` (в конфиге, dry-run: 326 новых / 84 изменённых / 14 удалённых файлов). Коммит отложен: upstream перестроил дерево (zlib-ng.h/zconf-ng.h стали генерируемыми `.in`, переименованы crc32.h/insert_string_roll.c и др.), требуется правка zlib-ng_2022.vcxproj и контрольная сборка — см. TODO.md.
- В AGENTS.md добавлено правило фиксировать задачи и план сессии в TODO.md для восстановления контекста после разрыва сессии.

## 2026-10-11
### Release|x64 warning-clean и чистые компиляторные предупреждения Debug|x64
- Release|x64 собран из FlylinkDC_2022.sln: 0 предупреждений и 0 ошибок; EXE линкуется, импорты — только системные DLL Windows, дымовой запуск на Windows успешен (процесс стартует и живёт).
- Debug|x64: 0 ошибок и 0 компиляторных предупреждений C4xxx; остаются только статические C26xx от /analyze, включённого в этой конфигурации.
- C2668 (двусмысленный SET_SETTING): windows/TreePropertySheet.cpp — `(INT_PTR)next` заменено на `static_cast<int>(ctrlTree.GetItemData(next))`.
- C4456 (затенение локальных): макросы блокировок больше не переиспользуют имя `l_lock`; в client/CFlyThread.h и client/webrtc/.../rw_lock_wrapper.h имена guard-переменных сделаны уникальными через `FLY_LOCK_CONCAT(l_fly_lock_, __COUNTER__)`.
- C4189 (переменные, нужные только в dcassert, пустом в Release): добавлены `(void)var;` (File.cpp, HashManager.cpp, GdiOle/GDIImage.cpp, flyServer.cpp, DownloadManager.cpp, ConnectionManager.cpp, windows/flattabctrl.h, EmoticonsDlg.cpp, HIconWrapper.cpp, FlyFeatures/AutoUpdate.cpp, Util.cpp и др.).
- Прочие компиляторные предупреждения: C4267/C4244/C4311 — явные static_cast (AdcHub.cpp, HashManager.cpp, SearchManager.cpp, ShareManager.cpp, AutoUpdate.cpp); C4702 — удалён недостижимый код (flyServer.cpp:getMediaInfo, flattabctrl.h::setColor); C4541 (typeid при /GR-) — локальный `#pragma warning(disable:4541)` в DownloadManager.cpp (Debug-only диагностика); C4066 — многобайтовый символьный литерал заменён на `_T('\x2026')`.
- Восстановлена кодировка исходников CP1251: русские комментарии, ранее заменённые на U+FFFD при пересохранении в UTF-8, возвращены из git, целевые правки применены заново без потери кодировки (проверено: U+FFFD отсутствует, diff содержит только целевые изменения).
### Ограничение анализа собственным кодом и исправление реальных дефектов /analyze
- Статический анализ (Debug|x64) ограничен собственным кодом: в flylinkdc_2022.vcxproj добавлены `ExternalIncludePath` (wtl, boost, openssl, zlib-ng, bzip2, leveldb, libtorrent, selene, lua, jsoncpp, mediainfolib, zenlib, maxminddb, client/webrtc) и `<DisableAnalyzeExternal>true</DisableAnalyzeExternal>` (Debug|x64); предупреждения анализатора по сторонним заголовкам (было ~13500: wtl/boost/openssl/libtorrent/leveldb/jsoncpp) полностью исчезли, остались только по своему коду.
- C6001 (использование неинициализированных lLow/lHigh после EM_GETSEL): client/FlyFeatures/PropertyItemEditors.h — объявление перенесено до switch с инициализацией `int lLow = 0, lHigh = 0;`.
- C6385 (чтение за границей `dDepths[5]` в CBarShader::BuildModifiers): windows/BarShader.cpp — вычисленный индекс уровня зажат в диапазон [1..5] перед обращением к массиву.
- C6308/C28182 (утечка и разыменование при realloc в CShellContextMenu::SetPath): windows/ShellContextMenu.cpp — результат realloc сохраняется во временную переменную с проверкой, результат calloc также проверяется; при ошибке корректно освобождаются pidl и COM-интерфейсы.
- C6387 (LoadLibrary мог вернуть 0, далее GetProcAddress): windows/ShellContextMenu.cpp, PopupManager.cpp, TreePropertySheet.cpp, WinUtil.cpp (LockWorkStation) — дескриптор модуля проверяется перед GetProcAddress.
- C6387 в windows/WinUtil.cpp: проверка результата GlobalLock перед wcscpy; проверка fi.hIcon перед DestroyIcon.
- C6053 (возможное отсутствие нуль-терминатора у _snprintf): client/Util.h (17 мест), client/CFlyMediaInfo.h, windows/HTMLColors.h — переход на безопасные `_snprintf_s`/`_snwprintf_s` с `_TRUNCATE`.
- C6386 в windows/PropertiesDlg.cpp: число страниц g_numPages сделано условным (37/38) в соответствии с `SSA_INCLUDE_FILE_SHARE_PAGE`, устранён латентный выход за границу массива; оставшееся предупреждение анализатора для `pages[n++]` — ложное (эвристика всегда сообщает переполнение на один элемент независимо от размера массива).
- Не тронуты (ложные/безвредные): C6387 для ATL `_U_STRINGorID` в страницах настроек, C6387 в ChatCtrl.cpp (GetWindowContext с NULL) и MainFrm.cpp (SetMenu/MapWindowPoints), C6011 в QueueFrame.cpp:1394 (`ii` проверен выше).
- Контрольная сборка после правок: Release|x64 — 0 предупреждений/0 ошибок; Debug|x64 — 0 компиляторных C4xxx/0 ошибок; дымовой запуск Release EXE успешен.
### Снижение шума статического анализатора: 14456 → 98 и исправление реальных дефектов
- `CodeAnalysisRuleSet` переключён с `AllRules.ruleset` (с `<IncludeAll>`) на `NativeRecommendedRules.ruleset` для Debug|x64 и Release|x64 в flylinkdc_2022.vcxproj: устранён массовый стилевой шум (C26440/26446/26447/26481/26482/26485/26493/26496 и т.п.), набор оставленных правил — NativeMinimumRules плюс курированные C6031/C6287/C26437/C26439/C26478/C26819 и др. Число предупреждений анализа в Debug|x64 снижено с ~14456 до 273.
- C6287 (избыточное сравнение с константой): windows/HubFrame.cpp:2398 — удалено `|| wp.showCmd == SW_MAXIMIZE` (дубликат SW_SHOWMAXIMIZED, обе константы равны 3).
- C6031 (проигнорированное возвращаемое значение): явные приведения `(void)` в 7 местах — windows/ChatCtrl.cpp (StgCreateDocfileOnILockBytes), FolderTree.cpp (SHGetDesktopFolder, WNetCloseEnum), MainFrm.cpp (CoInitialize), Players.cpp (CoInitialize ×2), ShellContextMenu.cpp (SHGetDesktopFolder).
- C6255 (`_alloca` может переполнить стек): подавлено локально в client/FlyFeatures/GradientLabel.h, PropertyItemEditors.h, PropertyItemImpl.h через `#pragma warning(push/pop)`.
- C26495 (неинициализированные члены): 71 правка в 15 файлах — добавлены NSDMI (client/ClientManager.h, CFlylinkDBManager.h, TigerHash.h, FlyFeatures/CProgressDlg.h, PropertyItemEditors.h; windows/UserlistColours.h, EmoticonsDlg.h, BarShader.h, OperaColorsPage.h, PropPageTextStyles.h, ChatCtrl.h, SearchFrm.h, MainFrm.h, FinishedFrameBase.h).
- C26819 (неаннотированный fallthrough): 8 мест помечены `[[fallthrough]];` (C++17) — client/FlyFeatures/PropertyItemEditors.h, windows/BaseChatFrame.cpp, QueueFrame.cpp ×2, ShellContextMenu.cpp, WaitingUsersFrame.cpp, TransferView.cpp ×2; намеренные переходы сохранены.
- C26478 (std::move от константы) — 86 предупреждений устранены: в макросе GETSET (client/BaseUtil.h) убран no-op `std::move` (ParameterType для классовых типов всегда `const T&`), а также в client/CFlySearchItemTTH.h для параметров-констант. Поведение не изменилось (перемещение от const и так было копированием).
- C26439: перемещающий конструктор CFlySearchItemTTH объявлен `noexcept`.
- Итоговая контрольная сборка: Debug|x64 — 98 предупреждений/0 ошибок (осталось: C6387 42 — ложные срабатывания ATL/COM; C26495 28 — `m_bMsgHandled` из макроса WTL DECLARE_MSG_MAP; C28251 18 — аннотации системных/сторонних заголовков; C28159 7 — советы заменить GetTickCount/ExitWindowsEx; C6011/C26437/C6386 по 1 — ложные/намеренные); Release|x64 — 0 предупреждений/0 ошибок.
### Анализ /analyze доведён до нуля: 98 → 0, ложные срабатывания устранены в корне
- WTL (вендорный код в wtl/): `BOOL m_bMsgHandled` в макросе `BEGIN_MSG_MAP_EX` (wtl/atlcrack.h) инициализирован значением `FALSE` — устранены 28 C26495 «неинициализированный член» (реальная ошибка: чтение неинициализированного флага в обработчиках сообщений).
- WTL (вендорный код в wtl/atldlgs.h): default-аргумент `ATL::_U_STRINGorID title` у `CPropertyPageImpl` и `CPropertyPage` заменён с `(LPCTSTR)NULL` на `0U` — устранены 37 ложных C6387 (нуль в адресованный `_In_z_` LPCTSTR-перегрузе `_U_STRINGorID`). Поведение идентично (`MAKEINTRESOURCE(0) == NULL`), так же поступает и сам WTL в аналогичных местах (Menu/ClassName).
- Оставшиеся ложные/намеренные срабатывания подавлены точечно: C6387 в windows/ChatCtrl.cpp (`GetWindowContext` с NULL — `#pragma warning(push/disable/pop)`) и windows/MainFrm.cpp (SetMenu/MapWindowPoints — `suppress`), C6011 в windows/QueueFrame.cpp (`ii` проверен выше), C6386 в windows/PropertiesDlg.cpp (эвристика переполнения на один элемент), C26437 в windows/PropPageTextStyles.cpp (намеренный срез `TextStyleSettings` → `CHARFORMAT2`, добавлен явный `static_cast` к базовой ссылке).
- Добавлен собственный набор правил flylinkdc.ruleset: включает `NativeRecommendedRules` и отключает только C28159 (советы заменить GetTickCount/ExitWindowsEx; дельты GetTickCount корректны по модулю 2^32) и C28251 (несогласованность SAL-аннотаций системных/MIDL-заголовков wmp.h/iTunesCOMInterface.h и `_tWinMain`). `CodeAnalysisRuleSet` в flylinkdc_2022.vcxproj переведён на него (Debug|x64 и Release|x64).
- Итоговая контрольная сборка: Debug|x64 — 0 предупреждений/0 ошибок при включённом /analyze (подтверждено по командной строке CL: `/analyze:ruleset flylinkdc.ruleset`); Release|x64 — 0 предупреждений/0 ошибок. Кодировки исходников (CP1251 без BOM, .vcxproj UTF-8 с BOM) сохранены, U+FFFD отсутствует, `git diff --check` чист.

### C26494 (ES.20, «всегда инициализируй объект»): правило включено и приведено к нулю
- В flylinkdc.ruleset добавлено `<Rule Id="C26494" Action="Warning" />` — требование инициализировать локальные переменные при объявлении теперь проверяется /analyze наравне с остальными рекомендованными правилами.
- Инициализированы все локальные переменные, отмеченные правилом (77 срабатываний, 69 уникальных мест) в 31 файле: явные (`= 0`, `= false`, `= nullptr`, `= {}`) и семантические инициализаторы (enum: `QueueItem::DEFAULT`, `FavoriteManager::NOT_CHANGE`, `SettingsManager::INT_FIRST`; `TASK_TYPE next = TASK_TYPE();`).
- Реальных дефектов «использование до инициализации» не было (C6001 = 0), правило было чисто стилевым (ES.20); его включение закрывает класс ошибок на будущее.
- Контрольная сборка: Debug|x64 — 0 предупреждений/0 ошибок при активном C26494 (пропущенные места, выявленные самой сборкой, исправлены: windows/ChatCtrl.cpp:1425 `OLEINPLACEFRAMEINFO frameInfo`, windows/WinUtil.cpp:3622 `HRESULT hRes`); Release|x64 — 0/0. Кодировки и `git diff --check` чистые.

### Оптимизация кодогенерации: включён Whole Program Optimization (/GL + /LTCG) для Release|x64
- Причина: во всех проектах набор правил задавал проектный `WholeProgramOptimization=true` (кроме flylinkdc) и `LIB.LinkTimeCodeGeneration=true`, но в каждом ItemDefinitionGroup `Release|x64` компиляторный `ClCompile.WholeProgramOptimization` был явно `false` — из-за этого `/GL` не выдавался, и линковка с `/LTCG` фактически была no-op (подтверждено: `/GL` отсутствовал в CL-командной строке всех 13 проектов).
- Убрано подавление `/GL`: `ClCompile.WholeProgramOptimization` изменён с `false` на `true` в `Release|x64` всех участвующих проектов (client, bzip2, zlib-ng, GdiOle, boost, MediaInfoLib, ZenLib, openssl, libtorrent, lua, LevelDB); flylinkdc также получил проектный `WholeProgramOptimization=true`. Служебный генератор MakeDefs оставлен без `/GL`. Теперь `/GL` присутствует во всех Release-объектных файлах, а линковка flylinkdc выполняется с `/LTCG:incremental` и `/LTCGOUT`.
- Устранён конфликт инструментариев, выявленный линковкой (C1047/LNK1257): в рабочей копии `Release|x64` проекта flylinkdc был на v145 (MSVC 14.51.36231), а все библиотеки — на v143 (MSVC 14.44.35207); при `/GL` сквозная оптимизация объектов, собранных разными версиями компилятора, невозможна. `Release|x64` flylinkdc возвращён к v143 (как в HEAD), чтобы весь Release-набор `/GL`-объектов собирался одним компилятором 14.44.35207. Debug не изменён.
- Результат: Release|x64 — EXE 20 198 400 байт (было 19 981 824; +1.1% из-за агрессивного инлайна при LTCG), полная пересборка 2.08 мин (было 2.33 мин), 0 предупреждений/0 ошибок; импорты — только системные DLL Windows; дымовой запуск успешен. Debug|x64 — без изменений (WPO в Debug выключен): 0/0, C26494=0.
- Подтверждено по `.tlog`: `/GL` в CL-командах client (104), flylinkdc (102), libtorrent (173), MediaInfoLib (250), openssl (678) и др.; `link.command` содержит `/LTCG:incremental` и `/LTCGOUT`.

## 2026-10-10
### Сборка Release|x64 в Visual Studio 18 Insiders (MSVC)
- Решение FlylinkDC_2022.sln собрано на узле через MSBuild (VS 18 Insiders, MSVC 14.51.36231/v145 и 14.44.35207/v143, Windows SDK 10.0.26100) в конфигурации Release|x64; код возврата 0.
- Артефакт: compiled/flylinkdc_x64.exe (~19 МБ). Проверка `dumpbin /dependents` показывает только системные DLL Windows (WS2_32, KERNEL32, USER32, GDI32, gdiplus, SHELL32 и т.п.); сторонних и runtime-DLL (vcruntime/msvcp) нет — статическая линковка.
- Дымовой запуск EXE на Windows: процесс стартует и живёт, падения нет.
- Сборка не warning-clean при /W4: ~3000+ предупреждений (в основном C4100, C4189, C4057; есть C4244/C4267/C4311/C4302 — усечение на x64). Диагностика: LNK4075 (/INCREMENTAL игнорируется из-за /LTCG), LNK4042 (winfirewall.obj дважды в client), MSB8012 (MakeDefs OutputFile не совпадает с OutDir).
- CMake/minGW-путь (build-win.sh, build-with-mxe.sh) по-прежнему нерабочий: нет собранных MinGW-зависимостей и WTL/ATL.
### Исправление диагностики и снижение шума предупреждений
- LNK4075: в flylinkdc_2022.vcxproj (Release|x64) LinkIncremental=false.
- LNK4042: в client_2022.vcxproj для client\webrtc\talk\base\winfirewall.cc задан отдельный ObjectFileName (winfirewall_webrtc.obj), чтобы не конфликтовать с FlyFeatures\WinFirewall.cpp.
- MSB8012: в MakeDefs\MakeDefs.vcxproj OutDir приведён к $(SolutionDir)MakeDefs\ (совпадает с Link.OutputFile, который использует pre-build шаг client).
- Снижение шума: сторонние проекты (openssl, MediaInfoLib, libtorrent, lua, LevelDB, zlib-ng, bzip2, boost-regex, ZenLib) переведены на TurnOffAllWarnings; для своего кода оставлен /W4 и включено исключение внешних заголовков (/external:anglebrackets, ExternalWarningLevel=TurnOffAllWarnings) в flylinkdc и client.
- C4189 (переменные, используемые только в dcassert, что в Release пуст): добавлены подавления `(void)var;` в проблемных местах (Bitmap.cpp, main.cpp, WinUtil.h, flattabctrl.h, WinUtil.cpp, TypedListViewCtrl.h, Util.cpp, CFlylinkDBManager.h/.cpp, SharedFileStream.cpp, File.cpp, ConnectionManager.cpp, flyServer.cpp, AutoUpdate.cpp).
- Полностью удалён Visual Leak Detector (VLD): include-пути в zlib-ng_2022.vcxproj, код/макросы в DCPlusPlus.cpp, main.cpp, stdinc.h; отладочный dcdebug в HIconWrapper.cpp заключён в #ifdef _DEBUG.
- Контрольная сборка после правок (Release|x64, exit 0): ошибок и диагностик LNK/MSB нет, предупреждений ~60 (в основном C4456 — затенение локальных переменных в своём коде; отдельно C4302/C4311 — усечение указателей при каст к int/UINT на x64, потенциальные баги: WinUtil.cpp:3889, TreePropertySheet.cpp:459, PropertyList.h:858). Импорты EXE — только системные DLL; дымовой запуск на Windows успешен.

## 2026-09-17
### Сборочное окружение Docker
- Добавлен Dockerfile на базе Ubuntu 26.04 с MinGW-w64, CMake, Ninja, Make, инструментами сборки зависимостей и ShellCheck; образ успешно собран.
- В AGENTS.md закреплены сборка и проверки только в Docker, запрет установки сборочных пакетов на хост, команды запуска с UID/GID пользователя и сохранение артефактов в build-win.
- Зафиксирована цель Windows x64 со статическими сторонними библиотеками и runtime компилятора; требуется проверка импортов EXE и отдельный запуск на Windows.
- build-win.sh переведён на CMake -S/-B с абсолютными путями, Release, BUILD_SHARED_LIBS=OFF, строгой обработкой ошибок и настройкой параллелизма через JOBS; ShellCheck в Docker проходит.
- Пробная сборка в Docker остановилась при генерации CMake: не найдены Windows-библиотеки OpenSSL, zlib, bzip2, libtorrent, leveldb, Lua, jsoncpp, maxminddb, MediaInfo и ZenLib. Их сборка из исходников пока не настроена; BUILD_SHARED_LIBS=OFF сам по себе не обеспечивает статическую линковку runtime и внешних библиотек.
- Проверка client/Util.h компилятором MinGW-w64 в Docker подтверждает отсутствие atlcomtime.h; GUI также требует ATL. Совместимость ATL/MSVC остаётся блокером; готовый EXE не получен, проверка импортов и запуск на Windows не выполнены.

## 2026-03-14
### Добавлена поддержка кросс-платформенной сборки
- Создан CMakeLists.txt для сборки проекта с использованием CMake.
- Добавлена возможность кросс-компиляции под Windows (x64) из-под Linux с использованием mingw-w64.
- Интеграция с существующими зависимостями (Boost, OpenSSL, libtorrent, zlib, bzip2, leveldb, lua, jsoncpp, maxminddb, mediainfolib, Zenlib).
- Обновлена документация по сборке.

### Изменения в структуре проекта
- Файлы Visual Studio (.vcxproj, .sln) остаются для сборки под Windows с MSVC.
- CMake позволяет альтернативную сборку без Visual C++.

## Предыдущие изменения
(см. историю коммитов репозитория)