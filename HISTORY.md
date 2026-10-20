# ╨Ш╤Б╤В╨╛╤А╨╕╤П ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╨╣ FlylinkDC++

## 2026-10-19
### Рефакторинг и поиск ошибок: собственный код `client/` + `windows/`
Цель: автономный аудит кода проекта (без вендоров) на реальные дефекты памяти, логики и
безопасности. Метод: MSVC `/analyze` (`build-win/analyze.ps1`, 255 находок, 49 — в
собственном коде) + параллельный ручной аудит по подсистемам + проверка каждого вывода по
исходникам. Сборка Debug|x64 и Release|x64 — 0 предупреждений / 0 ошибок.

#### Память и безопасность
- `windows/PublicHubsFrm.cpp:240` — **переполнение heap-буфера на 1 байт**: буфер распаковки
  bzip2 имел ровно `size*20` байт, `l_out[outSize] = 0` писал за его конец, если
  распакованные данные занимали весь буфер (реально для XML-списка хабов). Буфер +1 байт,
  индекс проверяется.
- `client/Encoder.cpp:80` — `fromBase32()` писал `dst[0]` при `len == 0`; соль ADC-хаба длиной
  1 символ давала `saltBytes = 0` → запись в 0-байтовый буфер. Гвард `dst == nullptr || len == 0`.
  `client/AdcHub.cpp:1463` — буфер соли теперь всегда непустой.
- `client/Encoder.cpp:120` — `isBase32()` индексировал таблицу знаковым `char` → отрицательный
  индекс (OOB-чтение) для не-ASCII magnet-хэша. Добавлен `(unsigned char)`-cast.
- `client/QueueItem.cpp:132` — деление на `getSize()` (размер файла задаёт удалённый пир)
  при `Size="0"` → `EXCEPTION_INT_DIVIDE_BY_ZERO` и падение клиента.
- `client/UploadManager.cpp:293,374` — проверка `(start + size) > sz` обходилась при
  `aBytes == -1` со `start > sz` (отрицательный размер в поток) и при переполнении `int64`
  (`aBytes` ≈ `INT64_MAX`). Старт и длина проверяются раздельно.
- `client/SearchManager.cpp:303` — UDP-парсер `$SR` искал `" ("` через `rfind` по всему буферу:
  в подделанном пакете позиция оказывалась меньше текущего курсора → `size_t`-underflow в
  `substr` и подстановка чужого TTH в результат поиска. Поля разбираются от конца сообщения.
- `client/iplist.cpp:291`, `client/PGLoader.cpp:63,141` — `substr(linestart, lineend - linestart - 1)`
  давал underflow на пустой строке (правилом становился весь остаток файла) и терял последний
  символ строки в LF-файлах. Разбор переписан с корректной обработкой CRLF/LF.
- `client/File.cpp:437,449` — `path[1]` на пустом пути (OOB-чтение).
- `client/Util.cpp:3170` — `GetTempPath` при нехватке буфера возвращает требуемый размер →
  чтение за пределами `LocalArray<TCHAR, MAX_PATH>`.
- `client/MD5Calc.cpp:20` — `CreateFile` не закрывался для пустых файлов и файлов > 4 ГБ
  (`GetFileSize` → `0xFFFFFFFF` → `int` → -1). Заменён на `GetFileSizeEx`.
- `client/FlyFeatures/flyServer.cpp:2351` — `l_dwBytesAvailable + 1` переполнял `DWORD`.
- `client/AdcHub.cpp:935` — `Util::toUInt32(...) * 8` считалось в 32 битах (переполнение).
- `client/AdcHub.cpp:289` — `*(short*)i->c_str()`: нарушение strict-aliasing и невыровненное
  чтение байтов от хаба. Заменено на чтение двух байт.
- `client/BufferedSocket.cpp:329` — underflow в `substr(8, l_marker_tth - 8)` на коротком
  `$Search`-пакете.
- `client/nmdchub.cpp:376` — тег `L:` из `$MyINFO` конвертировался знаковым `Util::toInt` и
  умножался на 1024 в 32 битах → произвольный лимит от хаба.
- `client/XMLParser/xmlParser.cpp:2291` — `(int)_ftelli64(f)` обрезал размер файла, результат
  `malloc` не проверялся → `fread(NULL, ...)` для файлов > 2 ГБ.
- `windows/WinUtil.cpp:2083` — в `RegSetValueEx` для `DCLST_SHELL_DESC` передавалась длина
  другой строки (`MAGNET_SHELL_DESC`).
- `windows/WinUtil.cpp:1991,1996,2000,2086,2090` — размер `REG_SZ` считался как
  `sizeof(TCHAR) * len + 1` → значение без терминального нуля.
- `windows/WinUtil.cpp:1785,1837,1879,1921,1969,2045` — `RegQueryValueEx` получал размер
  буфера в символах вместо байт → обрезанное значение команды в реестре.
- `windows/PropPage.cpp:202` — кисть `CreateSolidBrush` создавалась на каждое сообщение
  `WM_CTLCOLOR*` → утечка GDI-объектов (сотни в секунду). Кисть переиспользуется и
  пересоздаётся только при смене цвета темы.
- `windows/BarShader.h:209` — ключ кэша градиента не учитывал `bLight` (плоский и «объёмный»
  градиенты кэшировались под одним ключом).
- `windows/CheckTargetDlg.cpp:32,44`, `windows/SpyFrame.cpp:354` — `_snwprintf` без
  гарантированного NUL → чтение за пределами буфера.

#### Безопасность цепочки обновления
- `client/FlyFeatures/AutoUpdate.cpp` — **скачанный пакет обновления не проверялся по TTH**
  из подписанного манифеста и загружался по обычному HTTP: MITM (или скомпрометированное
  зеркало) мог подменить payload → произвольный код, устанавливаемый `FlyUpdate.exe`.
  Теперь TTH скачанного файла сверяется с подписанным манифестом (`Util::getTTH_MD5`),
  при несовпадении файл удаляется и обновление отклоняется.
- `client/FlyFeatures/AutoUpdate.cpp` — распаковка zip считала успехом флаг «есть ещё данные»
  (`UnZFilter` возвращает `Z_OK`, пока поток не завершён) → zip-обновления не устанавливались,
  а обрезанный payload принимался за корректный. Для bzip2 результат разжатия игнорировался
  вовсе. Буфер выделяется на 1 байт больше ожидаемого размера, проверяется полное
  потребление входа и точное совпадение размера.
- `client/FlyFeatures/AutoUpdate.cpp` — размер файла из манифеста не проверялся
  (`new byte[m_size]` с отрицательным значением → `bad_alloc` без обработки).
- `client/FlyFeatures/AutoUpdate.cpp` — путь и имя файла из манифеста не проверялись
  (path traversal → запись файла вне папки обновления). Добавлена проверка
  `isSafeRelativePath()`.
- `client/FlyFeatures/AutoUpdate.cpp:322` — инвертированный флаг `l_is_invalid_sign_update`
  при неуспешной проверке подписи второго манифеста.
- `client/FlyFeatures/flyServer.cpp` — конфиг fly-server приходит по обычному HTTP без подписи:
  невалидный `regex_find_ip` приводил к необработанному `std::regex_error` → падение клиента.
  Регулярка проверяется при загрузке конфига, в `windows/HubFrame.cpp` добавлен `try/catch`.
- `client/FlyFeatures/flyServer.cpp` — список расширений `mediainfo_ext` из того же конфига
  подставлялся в SQL без экранирования → SQL-инъекция в локальную БД. Значения экранируются.

#### COM и прочее
- `client/FlyFeatures/WinFirewall.cpp` — HRESULT `get_GloballyOpenPorts` не проверялся,
  указатель оставался неинициализированным → разыменование мусора (падение). Модуль
  переписан на RAII-обёртку `CFlyComPtr`: заодно устранена утечка 2..5 COM-ссылок на каждый
  запрос к брандмауэру.
- `client/Client.cpp:401` — при несовпадении keyprint TLS-соединение не закрывалось: сессия
  с недоверенным пиром оставалась открытой до авто-переподключения.

#### Продолжение сессии: память, безопасность, потоки

- `client/Util.cpp:1057` — буфер IDNA не оставался NUL-терминированным при длине
  хоста в ACP > 255 байт → чтение за пределами стекового буфера в
  `IDNA_convert_to_ACE()`. Хост приходит из URL хаба и из `Favorites.xml`.
- `client/SettingsManager.cpp` — настройки `*_COLUMNS_SORT` не проверялись и
  использовались как индекс колонки списка (значение обрезается до `uint8_t`) →
  выход за границу массива колонок. Добавлена проверка диапазона и защита в
  `windows/TypedListViewCtrl.h` (`setSortColumn`/`findColumn`/`getRealSortColumn`)
  и `windows/ExListViewCtrl.cpp` (`insert`).
- `client/FavoriteManager.cpp` — `HeaderSort` из `Favorites.xml` не проверялся
  (индекс колонки); разыменование `nullptr` в ветке ISP-синхронизации при
  `ISPDelete="1"`; утечка `RecentHubEntry` в `removeallRecent()`.
- `client/HashManager.cpp:644` — в fallback-ветке выделялся один буфер
  `g_HashBufferSize`, а `fastHash()` пишет во вторую половину → переполнение heap
  на 16 МБ. Буфер увеличен вдвое, добавлена проверка размера в `fastHash()`.
- `client/HashManager.cpp:84` — `blockSize` из NTFS-Alternate-Stream не проверялся →
  деление на ноль или гигантский `bad_alloc` в потоке хеширования.
- `client/SSLSocket.cpp:70,149` — при неудачном `SSL_new()` `checkSSL()` не бросал
  исключение → разыменование `nullptr` в `SSL_set_verify()`.
- `client/CFlylinkDBManager.cpp:4458` — размер blob'а дерева TTH обрезался до `int`
  (переполнение → запись NULL в БД), `getLeaves()[0]` — без проверки на пустоту.
- `client/SSL.h:73` — `scoped_handle::operator=(scoped_handle&&)` не освобождал
  прежний OpenSSL-ресурс.
- `client/DownloadManager.cpp:668` — `+ " Error = " = e.getError()`: присваивание
  вместо конкатенации; `:845` — отсутствующий `break` в `case SEV_RECOVERABLE`.
- `client/SettingsManager.cpp:1720` — переполнение `int` при проверке
  `COLOR_AVOIDING`; `client/SimpleXML.cpp:249` — «полусломанное» состояние объекта
  при исключении парсинга XML (итератор на освобождённом потомке).
- `client/CompatibilityManager.cpp` — `FreeLibrary()` после `GetModuleHandle()`
  (счётчик ссылок не увеличивался) и перебор дисков с `C:` при бите 0 = `A:`.
- `windows/SearchFrm.cpp:2994` — `wnds[-1]` (OOB) при Shift+Tab; `:3448` —
  пропущенная запятая в массиве разрешений.
- `windows/BarShader.cpp:520` — запись пикселя за правой границей DIB.
- `windows/TypedListViewCtrl.h:230` — `wcsncpy` без NUL-терминации в буфер
  `LVN_GETDISPINFO`.
- `windows/UsersFrame.cpp:307` — значение из конфига как индекс массива без проверки.
- `windows/PublicHubsFrm.cpp:385` — обрезание последнего символа строки и underflow
  при разборе ISP-списка; `windows/QueueFrame.cpp:1256` — `CheckMenuItem` без
  `MF_BYCOMMAND`.
- Потоки: `client/QueueManager.cpp:393` (`g_userQueueMap` без блокировки),
  `client/QueueItem.h` (`isWaiting`, `getDownloadsSegmentCount`),
  `client/QueueItem.cpp:329` (`isFinished`), `client/UploadManager.h:115`
  (`getUploadCount`), `client/CFlylinkDBManager.h:780` (`get_tth_cache_size`),
  `client/AdcHub.h:45` и `client/nmdchub.h:57` (`getUserCount`),
  `client/ShareManager.h:440` (размеры кэшей), `client/FlyFeatures/flyServer.cpp`
  (счётчики fly-server) — во всех местах размер контейнера читался без
  соответствующей блокировки.

#### Дымовой запуск (Release|x64, Windows)
- EXE запущен вручную: процесс стабильно живёт 20 с (32 потока, 119 МБ ОЗУ), новых
  дампов в `%LOCALAPPDATA%\CrashDumps` нет, клиент подключается к хабам
  (fly-server.ru, dchub.one, cifracom). В `Settings\Logs\System.log` — только
  безобидная запись об отсутствующей папке `Settings\Certificates\`; исключений,
  assert-ов и падений нет.

#### Продолжение сессии: клиент, окна, ресурсы

- `client/DownloadManager.cpp` — **удалённый DoS**: пир мог объявить дерево без leaf'ов,
  и цикл подбора размера блока крутился вечно (100 % CPU потока загрузки).
- `client/QueueManager.cpp` — `checkTarget()` читал `aTarget[1]`/`aTarget[2]` без проверки
  длины (пустой атрибут `Target` в файле очереди) → чтение за пределами строки.
- `client/UploadManager.cpp` — проверка сокета выполнялась после разыменования.
- `client/SettingsManager.cpp` — файл настроек заменялся неатомарно (delete + rename):
  падение оставляло клиент без настроек. Заменено на `MoveFileEx(MOVEFILE_REPLACE_EXISTING)`.
- `client/CFlylinkDBManager.cpp` — общая prepared-статмента без блокировки БД;
  `&p_resume[0]` на пустом векторе; имя таблицы в SQL конкатенацией; счётчик
  неотправленных статистик обнулялся при недоступном сервере.
- `client/CFlyThread.cpp` — handle потока терялся, если первая попытка создания потока
  не удалась, а вторая удалась → поток не присоединялся, handle утекал.
- `client/FlyFeatures/flyServer.h` — деструктор адаптера не присоединял поток запросов.
- `windows/flattabctrl.h` — `SwitchTo()` разыменовывал `m_active == nullptr`.
- `windows/DirectoryListingFrm.h` — `length() - 8` при коротком имени файла → underflow.
- `windows/MainFrm.cpp` — таймер окна уничтожался до подтверждения выхода; проверка
  `CB_ERR` сравнивалась с `uint32_t`; `_tcsncpy` без NUL в подсказке.
- `windows/DirectoryListingFrm.cpp` — пункт меню «избранные каталоги» индексировал
  чужой список; `dl->getUser()` мог быть `nullptr`.
- `windows/DCLSTPage.cpp` — индекс комбо не соответствовал значениям `DCLST_ACTION`.
- `windows/SearchFrm.h/.cpp` — гонка на `m_search` и потоки поиска, переживающие окно.
- `windows/ChatCtrl.cpp`, `windows/Players.cpp`, `windows/PopupManager.cpp`,
  `windows/TreePropertySheet.cpp` — утечки COM/GDI/модулей.
- `client/Text.h` — две перегрузки `tformat` заменены одной вариативной; опасные
  `buf.resize(_snwprintf(...))` заменены на `Text::tformat`.
- `client/Util.cpp` — `_snwprintf` писал строку в себя же (перекрывающиеся буферы).

#### Продолжение сессии: UI, протокол, потоки, рефакторинг

- `windows/FlyUpdateDlg.cpp` + `windows/AboutLogDlg.h` — **переполнение стека**:
  `EM_GETTEXTRANGE` пишет строку длины диапазона в буфер `LocalArray<TCHAR, MAX_PATH>`.
  В диалоге обновления текст приходит из скачанного по HTTP описания → длинная ссылка
  переполняет стек. Буфер выделяется по длине диапазона.
- `windows/PopupDlg.h` — кисть создавалась на каждое `WM_CTLCOLORSTATIC` (утечка GDI).
- `windows/SharePage.cpp` — сравнивался путь каталога вместо имени (`vName` не
  использовался) → переименование шары выполнялось всегда.
- `windows/FolderTree.cpp` — освобождённый буфер `NetShareEnum` не обнулялся → двойное
  освобождение.
- `windows/dclstGenDlg.cpp` — разыменование `_tth.get()` при нерассчитанном TTH.
- `windows/RangesPage.cpp` — `LB_GETTEXT` без терминатора → встроенный NUL в строке.
- `windows/MessagePanel.cpp` — `SHOW_EMOTIONS_BTN` проверялся дважды вместо
  `SHOW_MULTI_CHAT_BTN` → неверная ширина панели.
- `windows/EmoticonsDlg.cpp` — скрытый модальный диалог без эмотиконов → зависание клиента.
- `windows/SoundsPage.cpp` — `LVITEM` с `LVIF_TEXT` и `pszText == NULL` (запись по
  нулевому указателю) и `g_SoundsTable[GetSelectionMark()]` с индексом -1.
- `windows/PortalBrowser.cpp` — отрицательный код ошибки как индекс массива и двойной
  `FreeLibrary`.
- `windows/AdvancedPage.cpp` — результат `SetDlgItemText` присваивался строке → формат
  iTunes затирался.
- `windows/Players.cpp` — в «жанр» попадало название трека; `WideCharToMultiByte` в
  неинициализированный буфер; лишний `AddRef()` у COM-объекта.
- `windows/PropPageTextStyles.h/.cpp` — утечка GDI-кистей; имя шрифта писалось только в
  редактируемый стиль.
- `windows/JAControl.cpp` — часы делились на 360 вместо 3600.
- `windows/PrivateFrame.cpp` — ключ счётчика личных сообщений отличался от ключа
  увеличения/уменьшения → счётчик никогда не уменьшался и блокировал сообщения.
- `windows/PopupsPage.cpp` — выбранный в диалоге шрифт терялся (`LOGFONT` диалога — только
  входной параметр).
- `client/ConnectionManager.cpp` — **прослушивающий сокет пересоздавался каждые 250 мс**
  без `SO_REUSEADDR` → после отключения любого пира (порт в `TIME_WAIT`) `bind()` падал и
  слушатель не работал минутами. Пересоздание только после реальной ошибки.
- `client/CFlylinkDBManager.cpp` — инвертированный `memcmp` (данные возобновления торрента
  никогда не обновлялись) и 14 `bind(..., SQLITE_STATIC)` с `User::m_nick` → SQLite держит
  указатель на строку, которую меняет поток хаба → use-after-free. Заменено на
  `SQLITE_TRANSIENT`.
- `client/ClientManager.cpp` — `setErrors(0)` сбрасывало флаг «ошибка протокола» у всех
  файлов в очереди; порт из UDP-ответа обрезался до 16 бит; `dcassert` был инвертирован;
  сырой `Client*` использовался после освобождения блокировки списка клиентов (UAF) —
  добавлен `ClientManager::completeAutodetect()` под блокировкой.
- `client/OnlineUser.h`, `client/User.h` — ник пира менялся без блокировки, а геттер
  возвращал ссылку → use-after-free. Добавлены per-object мьютексы, геттеры возвращают
  копию; блокировка не удерживается при обращении к БД.
- `client/FlyFeatures/CProgressDlg.h` — рабочий поток переживал диалог (use-after-free) и
  терял handle. Добавлен heap-прокси прогресса с блокировкой и ожидание потока.
- `client/FlyFeatures/MiniHTML.h`, `client/FlyFeatures/PropertyItemImpl.h` — переполнения
  буферов, `ATLTRY`, проглатывавший `bad_alloc`, `_alloca` с длиной из удалённых данных.
- `client/CFlylinkDBManager.cpp` (`errorDB`) — модальный `MessageBox` под глобальной
  блокировкой БД: все операции с БД зависали до реакции пользователя.
- `client/StrUtil.h`, `client/TransferData.cpp`, `client/MerkleCheckOutputStream.h`,
  `client/Segment.h`, `client/HashValue.h`, `client/SettingsManager.cpp`,
  `client/MappingManager.cpp`, `client/WildcardsReg.cpp` — переполнение знакового типа,
  узкая строка в широком `%s`, деление на ноль, неверная семантика `contains()`,
  нетерминированная строка из реестра, потеря информации о порте, underflow в `substr`.
- **Рефакторинг**: четыре копии `registerXxxHandler()` в `windows/WinUtil.cpp` (~40 строк
  каждая) заменены одной общей функцией — именно из-за дублирования размер `REG_SZ`
  считался по-разному в копиях, и ошибка была исправлена только в одной.

#### Восстановление комментариев CP1251
- Коммит `32a9e4ed` («refactor: defensive _snprintf/strcpy/memcpy fixes») был записан
  инструментом, который читает файлы как UTF-8, а русские комментарии в исходниках
  хранятся в CP1251. Байты `0x80..0xFF` превратились в `U+FFFD` (EF BF BD) — в 7 файлах
  репозитория: `windows/MainFrm.cpp`, `client/ClientManager.cpp`, `client/OnlineUser.h`,
  `client/Socket.cpp`, `client/FinishedManager.cpp`, `client/idna/idna.cpp`,
  `client/ADLSearch.h`.
- Восстановлено скриптом `build-win/repair-cp1251.py`: строки сопоставляются по
  «скелету» (ASCII-часть + маркер не-ASCII-участка) с эталонными ревизиями
  (`32a9e4ed^`, `3f329713`, `58ebbbf8`) и оригинальные байты CP1251 возвращаются на место.
  Итог: **0 байт U+FFFD во всём репозитории**. Сборка после восстановления:
  Debug|x64 Rebuild — 0/0 (EXE=54883840), Release|x64 Rebuild — 0/0 (EXE=20748800).

#### Инструменты и документация
- `build-win/analyze.ps1` — прогон MSVC `/analyze` по решению с выгрузкой находок в
  `build-win/analysis/analyze_<name>_<config>.txt` (`file|line|rule|message`).
- `build-win/fix-todo-newlines.py` — восстановление переводов строк в `TODO.md`
  (файл был записан вообще без переводов строк).
- `build-win/update-todo.py`, `build-win/update-history.py` — обновление `TODO.md`/`HISTORY.md`.

## 2026-10-14
### ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨▓╨╡╨╜╨┤╨╛╤А╨░ boost 1.88.0 -> 1.92.0
- `vendor-manifests/boost.txt` тАФ ╨┐╨╗╨╛╤Б╨║╨╕╨╣ ╤Б╨┐╨╕╤Б╨╛╨║ ╤Д╨░╨╣╨╗╨╛╨▓; boost ╨▓╨╡╨╜╨┤╨╛╤А╨╕╤В╤Б╤П ╨┐╨╛ ╨╝╨╛╨┤╤Г╨╗╤П╨╝: ╨║╨░╤В╨░╨╗╨╛╨│ `boost/boost/` ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В ╨┐╨╛╨╗╨╜╤Л╨╡ ╨┤╨╡╤А╨╡╨▓╤М╤П ╨▓╤Л╨▒╤А╨░╨╜╨╜╤Л╤Е ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║ (╨▓╤Б╨╡ ╤Д╨░╨╣╨╗╤Л ╨║╨░╨╢╨┤╨╛╨│╨╛ ╨╕╨╖ 67 ╨║╨░╤В╨░╨╗╨╛╨│╨╛╨▓ ╨▓╨╡╤А╤Е╨╜╨╡╨│╨╛ ╤Г╤А╨╛╨▓╨╜╤П, 228 ╨║╨╛╨╝╨┐╨╛╨╜╨╡╨╜╤В╨╛╨▓ ╤Б ╤Г╤З╤С╤В╨╛╨╝ top-level ╤Д╨░╨╣╨╗╨╛╨▓), ╨░ ╨╜╨╡ include-╨╖╨░╨╝╤Л╨║╨░╨╜╨╕╨╡. ╨Я╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╛: `boost/algorithm` repo=upstream=87; ╨╡╨┤╨╕╨╜╤Б╤В╨▓╨╡╨╜╨╜╤Л╨╡ ╤Д╨░╨╣╨╗╤Л ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╤П, ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╤О╤Й╨╕╨╡ ╨▓ 1.92 тАФ 98 ╤А╨╡╨░╨╗╤М╨╜╨╛ ╤Г╨┤╨░╨╗╤С╨╜╨╜╤Л╤Е upstream (regex/v4 тАФ 49, smart_ptr/detail тАФ 33, asio/detail/impl тАФ 6, atomic тАФ 4 ╨╕ ╨┤╤А.). ╨з╨░╤Б╤В╤М `libs/` тАФ ╨║╤Г╤А╨╕╤А╤Г╨╡╨╝╤Л╨╣ ╤Б╨┐╨╕╤Б╨╛╨║ 37 ╤Д╨░╨╣╨╗╨╛╨▓ (`chrono`, `filesystem`, `iostreams`, `system`, `thread/src`).
- ╨Э╨╛╨▓╤Л╨╣ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В: 7713 ╨╖╨░╨┐╨╕╤Б╨╡╨╣ (7677 `boost/` + 36 `libs/`) = 6365 тИТ 98 (╤Г╨┤╨░╨╗╨╡╨╜╨╛ ╨▓ 1.92) + 1410 (╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╨╛ ╨▓ 1.92) + 36. ╨б╨│╨╡╨╜╨╡╤А╨╕╤А╨╛╨▓╨░╨╜ ╤Б╨┐╨╗╨╛╤И╨╜╤Л╨╝ ╨╛╨▒╤Е╨╛╨┤╨╛╨╝ upstream 1.92 ╨┐╨╛ ╤В╨╡╨╝ ╨╢╨╡ top-╨║╨╛╨╝╨┐╨╛╨╜╨╡╨╜╤В╨░╨╝; ╤Г╨┤╨░╨╗╨╡╨╜╤Л 99 ╤Г╤Б╤В╨░╤А╨╡╨▓╤И╨╕╤Е ╤Д╨░╨╣╨╗╨╛╨▓ (98 `boost/` + `libs/system/src/error_code.cpp`) ╨╕ 5 ╨┐╤Г╤Б╤В╤Л╤Е ╨║╨░╤В╨░╨╗╨╛╨│╨╛╨▓ (ASCII-╤Б╤В╤А╨╛╨║╨╕, LF, ╨╛╤В╤Б╨╛╤А╤В╨╕╤А╨╛╨▓╨░╨╜). ╨Я╤А╨╛╨▓╨╡╤А╨║╨░ ╨║╨╛╨╜╤Б╨╕╤Б╤В╨╡╨╜╤В╨╜╨╛╤Б╤В╨╕: 7713/7713 ╨┐╤А╨╕╤Б╤Г╤В╤Б╤В╨▓╤Г╤О╤В.
- Boost.System ╨▓ 1.92 ╤Б╤В╨░╨╗ header-only: `libs/system/src/` ╤Г╨┤╨░╨╗╤С╨╜ ╤Ж╨╡╨╗╨╕╨║╨╛╨╝. ╨Ш╨╖ ╨┐╤А╨╛╨╡╨║╤В╨╛╨▓ ╤Г╨▒╤А╨░╨╜╤Л ╤Б╤Б╤Л╨╗╨║╨╕ ╨╜╨░ ╨╕╤Б╤З╨╡╨╖╨╜╤Г╨▓╤И╨╕╨╣ `libs/system/src/error_code.cpp` (╨▒╤Л╨╗ ╤Б╨╛╨▓╨╝╨╡╤Б╤В╨╕╨╝╨╛╤Б╤В╨╜╨╛╨╣ ╨╖╨░╨│╨╗╤Г╤И╨║╨╛╨╣ `dummy_exported_function`, ╨╜╨╕╤З╨╡╨│╨╛ ╨╜╨╡ ╨▓╤Л╨╖╤Л╨▓╨░╨╡╤В): `boost-regex_2022.vcxproj` (+`.filters`), `MakeDefs.vcxproj` (+`.filters`, ╨░ ╤В╨░╨║╨╢╨╡ stale `local_free_on_destruction.hpp`, ╨║╨╛╤В╨╛╤А╨╛╨│╨╛ ╨▓ 1.88 ╤Г╨╢╨╡ ╨╜╨╡ ╨▒╤Л╨╗╨╛). ╨в╨╡ ╨╢╨╡ ╤Б╤Б╤Л╨╗╨║╨╕ ╤Г╨▒╤А╨░╨╜╤Л ╨▓ ╨╛╤В╨┤╨╡╨╗╤М╨╜╤Л╤Е ╤А╨╡╤И╨╡╨╜╨╕╤П╤Е `libtorrent/examples/client_test.vcxproj` ╨╕ `test/test-console.vcxproj` (+`.filters`). ╨Э╨╛╨▓╤Л╨╣ `libs/thread/src/tss_null.cpp` ╨╜╨╡ ╨▓╨╡╨╜╨┤╨╛╤А╨╕╤В╤Б╤П (Win32 ╤Б╨╛╨▒╨╕╤А╨░╨╡╤В `tss_dll.cpp`/`tss_pe.cpp`).
- `vendor-patches/boost.patch`: ╤Д╨╕╨║╤Б `std::strtoll`тЖТ`strtoll` ╨▓ `boost/asio/impl/config.hpp` ╨░╨║╤В╤Г╨░╨╗╨╡╨╜ ╨╕ ╨┤╨╗╤П 1.92 (2 ╨╝╨╡╤Б╤В╨░); ╤Е╤Г╨╜╨║ ╤Б ╤Г╨┤╨░╨╗╨╡╨╜╨╕╨╡╨╝ ╨┐╨╡╤А╨╡╨╜╨╛╤Б╨░ ╤Б╤В╤А╨╛╨║╨╕ ╨▓ `boost/detail/lightweight_test_report.hpp` ╤В╨╛╨╢╨╡ ╨┐╤А╨╕╨╝╨╡╨╜╤П╨╡╤В╤Б╤П. ╨Ю╨▒╨╜╨░╤А╤Г╨╢╨╡╨╜ ╨▒╨░╨│ ╨┐╨░╨╣╨┐╨╗╨░╨╣╨╜╨░: `git apply` ╨╖╨░╨┐╤Г╤Б╨║╨░╨╡╤В╤Б╤П ╤Б `core.autocrlf=false`, ╨░ ╤А╨░╨▒╨╛╤З╨░╤П ╨║╨╛╨┐╨╕╤П ╨┐╨░╤В╤З╨░ ╨╕╨╝╨╡╨╗╨░ CRLF-╨┐╨╡╤А╨╡╨▓╨╛╨┤╤Л ╤Б╤В╤А╨╛╨║ (╨▓ git-blob тАФ LF), ╨╕╨╖-╨╖╨░ ╤З╨╡╨│╨╛ ╨║╨╛╨╜╤В╨╡╨║╤Б╤В ╨╜╨╡ ╤Б╨╛╨▓╨┐╨░╨┤╨░╨╗. ╨Я╨░╤В╤З ╨╜╨╛╤А╨╝╨░╨╗╨╕╨╖╨╛╨▓╨░╨╜ ╨▓ LF; ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜ `.gitattributes` (`vendor-patches/*.patch text eol=lf`) ╨┤╨╗╤П ╨▓╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╛╨┤╨╕╨╝╨╛╤Б╤В╨╕ ╨╜╨░ ╤Б╨▓╨╡╨╢╨╡╨╝ checkout.
- ╨Я╤А╨╛╨▓╨╡╤А╨║╨░ ╨┐╨░╨╣╨┐╨╗╨░╨╣╨╜╨░: `vendor-update.py --vendor boost --dry-run` тАФ add=1410 update=4283 unchanged=2020; ╤А╨╡╨░╨╗╤М╨╜╤Л╨╣ ╤Б╨╕╨╜╨║ (`--no-commit`) ╤Б╨║╨╛╨┐╨╕╤А╨╛╨▓╨░╨╗ ╤Д╨░╨╣╨╗╤Л ╨╕ ╨┐╤А╨╕╨╝╨╡╨╜╨╕╨╗ ╨┐╨░╤В╤З; ╨┐╨╛╨▓╤В╨╛╤А╨╜╤Л╨╣ dry-run тАФ add=0 update=2 delete=0 unchanged=7711 (╤Б╤В╨░╨▒╨╕╨╗╤М╨╜╨╛: ╤А╨╛╨▓╨╜╨╛ 2 ╨┐╨░╤В╤З╨╡╨╜╨╜╤Л╤Е ╤Д╨░╨╣╨╗╨░). ╨Т ╨┤╨╡╤А╨╡╨▓╨╡ `BOOST_VERSION 109200` / `BOOST_LIB_VERSION "1_92"`.
- ╨б╨▒╨╛╤А╨║╨░: Debug|x64 Rebuild тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║, exit=0 (EXE=54560768); Release|x64 Rebuild тАФ 0/0, exit=0 (EXE=20656640). libtorrent 2.1.2 ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╤В ╤В╨╛╨╗╤М╨║╨╛ ╨┐╤Г╨▒╨╗╨╕╤З╨╜╤Л╨╣ API asio/beast (`make_work_guard`/`executor_work_guard`) ╨╕ ╤Г╨╢╨╡ ╨╕╨╝╨╡╨╡╤В guards `BOOST_VERSION >=`; ╨┐╤А╤П╨╝╨╛╨╣ ╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛╤Б╤В╨╕ ╨╛╤В ╤Г╨┤╨░╨╗╤С╨╜╨╜╤Л╤Е detail-╤Д╨░╨╣╨╗╨╛╨▓ ╨╜╨╡╤В, ╨╜╨╡╤Б╨╛╨▓╨╝╨╡╤Б╤В╨╕╨╝╨╛╤Б╤В╨╕ ╨╜╨╡ ╨▓╤Л╤П╨▓╨╗╨╡╨╜╨╛.
- ╨Ъ╨╛╨╝╨╝╨╕╤В ╨╜╨╡ ╤Б╨╛╨╖╨┤╨░╨▓╨░╨╗╤Б╤П (╨┐╤А╨░╨▓╨╕╨╗╨╛: ╤В╨╛╨╗╤М╨║╨╛ ╨┐╨╛ ╨╖╨░╨┐╤А╨╛╤Б╤Г). ╨а╨░╨▒╨╛╤З╨╡╨╡ ╨┤╨╡╤А╨╡╨▓╨╛: ╨╛╨▒╨╜╨╛╨▓╨╗╤С╨╜╨╜╤Л╨╣ boost (╨▓ ╤В.╤З. +1410 ╨╜╨╛╨▓╤Л╤Е ╤Д╨░╨╣╨╗╨╛╨▓ ╨║╨░╨║ untracked), ╨╜╨╛╨▓╤Л╨╣ `.gitattributes`, ╨┐╤А╨░╨▓╨║╨╕ vcxproj/filters, ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В╨░ ╨╕ ╨┐╨░╤В╤З╨░.

## 2026-10-13
### ╨Р╤Г╨┤╨╕╤В PCH ╨╕ ╨┤╤Г╨▒╨╗╨╕╤А╤Г╤О╤Й╨╕╤Е╤Б╤П ╨▓╨║╨╗╤О╤З╨╡╨╜╨╕╨╣ (Release|x64)
- ╨Я╤А╨╛╨▓╨╡╤А╨╡╨╜╨░ ╤Б╤Е╨╡╨╝╨░ ╨┐╤А╨╡╨┤╨║╨╛╨╝╨┐╨╕╨╗╤П╤Ж╨╕╨╕ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╛╨▓ ╨▓╤Б╨╡╤Е 13 ╨┐╤А╨╛╨╡╨║╤В╨╛╨▓: PCH ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╤В╤Б╤П ╨╛╨┐╤В╨╕╨╝╨░╨╗╤М╨╜╨╛ ╨▓╨╡╨╖╨┤╨╡, ╨│╨┤╨╡ ╤Н╤В╨╛ ╨┤╨░╤С╤В ╨▓╤Л╨╕╨│╤А╤Л╤И: client (`stdinc.h`), flylinkdc (`windows/stdafx.h`, ╨║╨╛╤В╨╛╤А╤Л╨╣ ╤В╤П╨╜╨╡╤В `../client/stdinc.h`), openssl (`openssl_pch.h`), libtorrent (`libtorrent_pch.hpp`), ZenLib/MediaInfoLib (`PreComp.h`), leveldb (`stdinc.h`), GdiOle (`stdafx.h`), UnRAR (`rar.hpp`). ╨Ъ╨░╨╢╨┤╤Л╨╣ .cpp (204 ╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜╨╜╤Л╤Е) ╨┐╨╛╨┤╤Е╨▓╨░╤В╤Л╨▓╨░╨╡╤В ╤Б╨▓╨╛╨╣ PCH ╨┐╨╡╤А╨▓╤Л╨╝ ╨▓╨║╨╗╤О╤З╨╡╨╜╨╕╨╡╨╝; ╨╕╤Б╨║╨╗╤О╤З╨╡╨╜╨╕╨╡ тАФ `client/XMLParser/xmlParser.cpp` (╨▓╨║╨╗╤О╤З╨░╨╡╤В `stdinc.h` ╨╜╨░ ╤Б╤В╤А╨╛╨║╨╡ 84, ╨┐╤А╨░╨▓╨╕╨╗╤М╨╜╨╛ ╨┤╨╗╤П ╤Б╨▓╨╛╨╡╨╣ ╨╡╨┤╨╕╨╜╨╕╤Ж╤Л).
- ╨г╨┤╨░╨╗╨╡╨╜╤Л ╤Д╨░╨║╤В╨╕╤З╨╡╤Б╨║╨╕╨╡ ╨┤╤Г╨▒╨╗╨╕╨║╨░╤В╤Л ╨▓╨║╨╗╤О╤З╨╡╨╜╨╕╨╣ ╨▓ `windows/MainFrm.cpp`: ╨┐╨╛╨▓╤В╨╛╤А╨╜╤Л╨╡ `#include "Resource.h"` ╨╕ `#include "PrivateFrame.h"`. ╨Ю╤Б╤В╨░╨╗╤М╨╜╤Л╨╡ ┬л╨┤╤Г╨▒╨╗╨╕┬╗ ╨▓ ╨┤╨╡╤А╨╡╨▓╨╡ (`sqlite3.c`/`shell.c`, zlib-ng `*_tpl.h`, libtorrent, mediainfolib, openssl, zenlib) тАФ ╤Н╤В╨╛ ╨╗╨╕╨▒╨╛ ╤А╨░╨╖╨╜╤Л╨╡ `#ifdef`-╨▓╨╡╤В╨║╨╕, ╨╗╨╕╨▒╨╛ upstream-╤В╤А╨╡╨▒╨╛╨▓╨░╨╜╨╕╤П, ╨▓ ╤Б╨║╨╛╨╝╨┐╨╕╨╗╨╕╤А╨╛╨▓╨░╨╜╨╜╨╛╨╝ ╨▓╨╕╨┤╨╡ ╨╜╨╡ ╨┐╨░╤А╤Б╤П╤В╤Б╤П ╨┐╨╛╨▓╤В╨╛╤А╨╜╨╛ (include-guards). ╨Я╤А╨░╨▓╨║╨░ ╨▓╨╜╨╡╤Б╨╡╨╜╨░ ╨▒╨░╨╣╤В╨╛╨▓╨╛-╨▒╨╡╨╖╨╛╨┐╨░╤Б╨╜╨╛: ╤А╨╡╨┤╨░╨║╤В╨╛╤А ╨┐╤А╨╕ ╨┐╨╡╤А╨▓╨╛╨╝ ╨┐╤А╨╛╤Е╨╛╨┤╨╡ ╨┐╨╡╤А╨╡╨║╨╛╨┤╨╕╤А╨╛╨▓╨░╨╗ ╤Д╨░╨╣╨╗ ╨╕╨╖ CP1251 ╨▓ UTF-8 (1238 U+FFFD); ╤Д╨░╨╣╨╗ ╨▓╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜ ╨╕╨╖ HEAD ╨╕ ╨╛╤В╤А╨╡╨┤╨░╨║╤В╨╕╤А╨╛╨▓╨░╨╜ ╨╜╨░ ╤Г╤А╨╛╨▓╨╜╨╡ ╨▒╨░╨╣╤В, CP1251 ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╨░ (diff: тИТ2 ╤Б╤В╤А╨╛╨║╨╕).
- ╨Т╤Л╨▓╨╛╨┤: ╨▓╤А╨╡╨╝╤П ClCompile (84.6 ╤Б ╤Б╤Г╨╝╨╝╨░╤А╨╜╨╛ ╨┐╨╛ 13 ╨┐╤А╨╛╨╡╨║╤В╨░╨╝ ╨┐╤А╨╕ `/MP`+`/m`) ╨╜╨╡ ╤П╨▓╨╗╤П╨╡╤В╤Б╤П ╤Г╨╖╨║╨╕╨╝ ╨╝╨╡╤Б╤В╨╛╨╝ ╨╕ ╨╜╨╡ ╨╝╨╛╨╢╨╡╤В ╨▒╤Л╤В╤М ╤Б╤Г╤Й╨╡╤Б╤В╨▓╨╡╨╜╨╜╨╛ ╤Б╨╛╨║╤А╨░╤Й╨╡╨╜╨╛ ╨╖╨░ ╤Б╤З╤С╤В PCH/╨╕╨╜╨║╨╗╤Г╨┤╨╛╨▓ тАФ ╤А╨░╤Б╤И╨╕╤А╨╡╨╜╨╕╨╡ PCH ╤Г╨╢╨╡ ╨┐╨╛╨║╤А╤Л╨▓╨░╨╡╤В ╨▓╤Б╨╡ ╤В╤П╨╢╤С╨╗╤Л╨╡ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╨╡/╤Б╤В╨╛╤А╨╛╨╜╨╜╨╕╨╡ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╕, ╨┤╤Г╨▒╨╗╨╡╨╣ ╨┐╤А╨╕ ╨║╨╛╨╝╨┐╨╕╨╗╤П╤Ж╨╕╨╕ ╨╜╨╡╤В. ╨Ф╨╛╨╝╨╕╨╜╨╕╤А╤Г╤О╤Й╨░╤П ╤Б╤В╨░╤В╤М╤П ╨┐╨╛╨╗╨╜╨╛╨╣ ╨┐╨╡╤А╨╡╤Б╨▒╨╛╤А╨║╨╕ тАФ LTCG-╨╗╨╕╨╜╨║╨╛╨▓╨║╨░ (Link ~92.6 ╤Б), ╨║╨╛╤В╨╛╤А╨░╤П ╤Г╨╢╨╡ ╨╕╤Б╤Б╨╗╨╡╨┤╨╛╨▓╨░╨╜╨░ ╨╛╤В╨┤╨╡╨╗╤М╨╜╨╛. ╨д╨╕╨╜╨░╨╗╤М╨╜╨░╤П ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ ╨┐╨╛╤Б╨╗╨╡ ╨┐╤А╨░╨▓╨╛╨║: release ClCompile flylinkdc тАФ 0/0, exit=0 (╨╕╨╜╨║╤А╨╡╨╝╨╡╨╜╤В╨░╨╗╤М╨╜╨╛, 2.05 ╤Б).

### ╨г╤Б╨║╨╛╤А╨╡╨╜╨╕╨╡ ╤Б╨▒╨╛╤А╨║╨╕ MSVC (Release|x64): ╤Н╨║╤Б╨┐╨╡╤А╨╕╨╝╨╡╨╜╤В╤Л ╨┐╨╛ ╨┐╨░╤А╨░╨╗╨╗╨╡╨╗╨╕╨╖╨╝╤Г MSBuild/LTCG
- ╨ж╨╡╨╗╤М: ╤Б╨╛╨║╤А╨░╤В╨╕╤В╤М ╨▓╤А╨╡╨╝╤П ╨┐╨╛╨╗╨╜╨╛╨│╨╛ Release Rebuild solution (╨▒╨░╨╖╨░ ~2.2 ╨╝╨╕╨╜). ╨Ь╨╡╤В╨╛╨┤╨╕╨║╨░: ╨║╨░╨╢╨┤╤Л╨╣ ╨▓╨░╤А╨╕╨░╨╜╤В ╨╖╨░╨┐╤Г╤Б╨║╨░╨╗╤Б╤П ╤З╨╡╤А╨╡╨╖ `build-win/build-round.ps1` (`/m`, `/v:m`, `/clp:PerformanceSummary`), ╨╝╨╡╤В╤А╨╕╨║╨╕ ╨╖╨░╨┐╨╕╤Б╤Л╨▓╨░╨╗╨╕╤Б╤М ╨▓ `build-win/build-stats.csv`; ╨╛╤В╨║╨░╤В ╨┐╤А╨╕ ╤А╨╡╨│╤А╨╡╤Б╤Б╨╡.
- ╨Я╤А╨╛╤Д╨╕╨╗╤М ╤Г╨╖╨║╨╕╤Е ╨╝╨╡╤Б╤В ╨▒╨░╨╖╤Л (Release Rebuild): LTCG `Link` = 92.6 ╤Б (2 ╨▓╤Л╨╖╨╛╨▓╨░; "All 152382 functions were compiled because no usable IPDB/IOBJ" тАФ Rebuild ╤Г╨┤╨░╨╗╤П╨╡╤В IPDB, ╨┐╨╛╤Н╤В╨╛╨╝╤Г ╨╕╨╜╨║╤А╨╡╨╝╨╡╨╜╤В╨░╨╗╤М╨╜╨░╤П LTCG-╨║╨╛╨┤╨╛╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╤П ╨║╨░╨╢╨┤╤Л╨╣ ╤А╨░╨╖ ╨┐╨╛╨╗╨╜╨░╤П), `ClCompile` = 84.6 ╤Б (13 ╨▓╤Л╨╖╨╛╨▓╨╛╨▓), `Lib` = 16.5 ╤Б, `Midl` = 3.9 ╤Б. ╨е╨╛╤Б╤В: i7-11700KF, 8 ╤П╨┤╨╡╤А / 16 ╨┐╨╛╤В╨╛╨║╨╛╨▓.
- ╨а╨╡╨╖╤Г╨╗╤М╤В╨░╤В╤Л ╤Н╨║╤Б╨┐╨╡╤А╨╕╨╝╨╡╨╜╤В╨╛╨▓ (╨▓╤Б╨╡ W=0, E=0, exit=0, EXE 20656640 ╨▒ тАФ ╤Б╤А╨░╨▓╨╜╨╕╨╝╨╛ ╤Б ╨▒╨░╨╖╨╛╨╣ 2.16-2.21 ╨╝╨╕╨╜):
  - MTT (`UseMultiToolTask=true` + `EnforceProcessCountAcrossBuilds=true`): 2.66 ╨╝╨╕╨╜ тАФ ╨╝╨╡╨┤╨╗╨╡╨╜╨╜╨╡╨╡, ╨╛╤В╨║╨╗╨╛╨╜╨╡╨╜╨╛ ╨╕ ╨╛╤В╨║╨░╤З╨╡╨╜╨╛.
  - `EnableClServerMode=true`: 2.61 ╨╝╨╕╨╜ тАФ ╨╝╨╡╨┤╨╗╨╡╨╜╨╜╨╡╨╡, ╨╛╤В╨║╨╗╨╛╨╜╨╡╨╜╨╛.
  - `BuildPassReferences=true`: 2.68 ╨╝╨╕╨╜ тАФ ╨╝╨╡╨┤╨╗╨╡╨╜╨╜╨╡╨╡, ╨╛╤В╨║╨╗╨╛╨╜╨╡╨╜╨╛.
  - `/CGTHREADS:8` ╨▓ Release Link flylinkdc (LTCG-╨║╨╛╨┤╨╛╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╤П): 2.39 ╨╝╨╕╨╜; Link ╨▓╤Л╤А╨╛╤Б ╨┤╨╛ 105.7 ╤Б ╨┐╤А╨╛╤В╨╕╨▓ 92.6 ╤Б тАФ ╨┤╨╡╤Д╨╛╨╗╤В╨╜╤Л╨╡ 4 ╨┐╨╛╤В╨╛╨║╨░ ╨╛╨┐╤В╨╕╨╝╨░╨╗╤М╨╜╤Л, ╨╛╤В╨║╨╗╨╛╨╜╨╡╨╜╨╛ ╨╕ ╨╛╤В╨║╨░╤З╨╡╨╜╨╛.
  - ╨б╨╜╤П╤В╨╕╨╡ `<Profile>true</Profile>` ╨╕╨╖ Release Link flylinkdc (╤Д╨╗╨░╨│ `/PROFILE` ╨▓╨║╨╗╤О╤З╨░╨╡╤В `/DEBUG:FULL` ╨╕ `/INCREMENTAL:NO`): 2.14 ╨╝╨╕╨╜ тАФ ╨▒╨╡╨╖ ╨╖╨╜╨░╤З╨╕╨╝╨╛╨│╨╛ ╨▓╤Л╨╕╨│╤А╤Л╤И╨░ (Link 91.5 ╤Б vs 92.6 ╤Б), ╨╛╤В╨║╨╗╨╛╨╜╨╡╨╜╨╛ ╨╕ ╨╛╤В╨║╨░╤З╨╡╨╜╨╛, ╤З╤В╨╛╨▒╤Л ╨╜╨╡ ╨╝╨╡╨╜╤П╤В╤М ╤Б╨╡╨╝╨░╨╜╤В╨╕╨║╤Г ╨▒╨╕╨╜╨░╤А╨╜╨╕╨║╨░.
- ╨Т╤Л╨▓╨╛╨┤: ╤Б╤В╨░╨╜╨┤╨░╤А╤В╨╜╤Л╨╡ ╨╜╨░╤Б╤В╤А╨╛╨╣╨║╨╕ (`/m` + ╤Г╨╢╨╡ ╨▓╨║╨╗╤О╤З╤С╨╜╨╜╤Л╨╡ `/MP` ╨╕ `/LTCG:incremental`) ╨╛╨┐╤В╨╕╨╝╨░╨╗╤М╨╜╤Л ╨┤╨╗╤П ╤Н╤В╨╛╨│╨╛ ╨╜╨░╨▒╨╛╤А╨░ ╨┐╤А╨╛╨╡╨║╤В╨╛╨▓; ╤Г╨┐╤А╨░╨▓╨╗╤П╨╡╨╝╤Л╨╡ MSBuild/LTCG-╨┐╨░╤А╨░╨╗╨╗╨╡╨╗╨╕╨╖╨╝ ╨╜╨╡ ╨┤╨░╤С╤В ╤Г╤Б╨║╨╛╤А╨╡╨╜╨╕╨╣ (╨╝╨╡╤Е╨░╨╜╨╕╨╖╨╝╤Л ╨║╨╛╨╜╨║╤Г╤А╨╕╤А╤Г╤О╤В ╨╖╨░ ╤В╨╡ ╨╢╨╡ 8 ╤Д╨╕╨╖╨╕╤З╨╡╤Б╨║╨╕╤Е ╤П╨┤╨╡╤А, ╤З╤В╨╛ ╨╕ `/MP`). ╨Ю╤Б╨╜╨╛╨▓╨╜╨╛╨╣ ╨╛╤Б╤В╨░╨▓╤И╨╕╨╣╤Б╤П ╨▓╤Л╨╕╨│╤А╤Л╤И ╨▓╨╛╨╖╨╝╨╛╨╢╨╡╨╜ ╤В╨╛╨╗╤М╨║╨╛ ╨╖╨░ ╤Б╤З╤С╤В ╨╕╨╜╨║╤А╨╡╨╝╨╡╨╜╤В╨░╨╗╤М╨╜╨╛╨╣ ╤Ж╨╡╨╗╨╕ `Build` (╨╜╨╡ `Rebuild`), ╨│╨┤╨╡ IPDB LTCG ╨┐╨╡╤А╨╡╨╢╨╕╨▓╨░╨╡╤В ╨┐╨╡╤А╨╡╤Б╨▒╨╛╤А╨║╤Г.
- ╨Я╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╛ ╨╕╨╖╨╝╨╡╤А╨╡╨╜╨╕╨╡╨╝ (2026-10-13): ╨╕╨╜╨║╤А╨╡╨╝╨╡╨╜╤В╨░╨╗╤М╨╜╨░╤П ╤Ж╨╡╨╗╤М `Build` ╨▓╨╝╨╡╤Б╤В╨╛ `Rebuild` ╤Б╨╛╨║╤А╨░╤Й╨░╨╡╤В ╨┐╨╛╨╗╨╜╤Л╨╣ ╤Ж╨╕╨║╨╗ ╤Б ~2.2 ╨╝╨╕╨╜ ╨┤╨╛ 0.47 ╨╝╨╕╨╜ (╨┐╤А╨░╨▓╨║╨░ 1 ╤Д╨░╨╣╨╗╨░) ╨╕ ╨┤╨╛ 0.01 ╨╝╨╕╨╜ (no-op). ╨Ъ╨╗╤О╤З╨╡╨▓╨░╤П ╤Б╤В╤А╨╛╨║╨░ ╨╗╨╕╤Б╨░ LTCG-╨╗╨╕╨╜╨║╨╛╨▓╨║╨╕: ┬л1768 of 152382 functions (1.2%) were compiled, the rest were copied from previous compilation┬╗ тАФ ╨┐╤А╨╕ `Build` IPDB ╨┐╨╡╤А╨╡╨╢╨╕╨▓╨░╨╡╤В ╨┐╨╡╤А╨╡╤Б╨▒╨╛╤А╨║╤Г ╨╕ ╨║╨╛╨┤╨╛╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╤П ╨╕╨┤╤С╤В ╤В╨╛╨╗╤М╨║╨╛ ╨┐╨╛ ╨╕╨╖╨╝╨╡╨╜╨╕╨▓╤И╨╕╨╝╤Б╤П ╨╝╨╛╨┤╤Г╨╗╤П╨╝, ╤В╨╛╨│╨┤╨░ ╨║╨░╨║ `Rebuild` ╤Г╨┤╨░╨╗╤П╨╡╤В IPDB ╨╕ ╤Д╨╛╤А╤Б╨╕╤А╤Г╨╡╤В ╨┐╨╛╨╗╨╜╤Г╤О ╨║╨╛╨┤╨╛╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╤О (2002.C ╤Б Link ~92.6 ╤Б). ╨а╨╡╨║╨╛╨╝╨╡╨╜╨┤╨░╤Ж╨╕╤П: ╨┤╨╗╤П ╨┐╨╛╨▓╤Б╨╡╨┤╨╜╨╡╨▓╨╜╨╛╨╣ ╨╕╤В╨╡╤А╨░╤В╨╕╨▓╨╜╨╛╨╣ ╤А╨░╨╖╤А╨░╨▒╨╛╤В╨║╨╕ ╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╤В╤М ╤Ж╨╡╨╗╤М `Build`; `Rebuild` тАФ ╤В╨╛╨╗╤М╨║╨╛ ╨┤╨╗╤П ╤З╨╕╤Б╤В╤Л╤Е/╨║╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╤Л╤Е ╨┐╤А╨╛╨▓╨╡╤А╨╛╨║.
- ╨д╨╕╨╜╨░╨╗╤М╨╜╨░╤П ╨║╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ Release|x64 Rebuild: 2.14 ╨╝╨╕╨╜, 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║, exit=0.
### ╨г╨┤╨░╨╗╨╡╨╜╨╕╨╡ ╨╜╨╡╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╨╝╨╛╨│╨╛ ╨▓╨╡╨╜╨┤╨╛╤А╨░ unrar
- ╨Я╤А╨╛╨▓╨╡╤А╨╡╨╜╨╛: ╨║╨░╤В╨░╨╗╨╛╨│ `unrar/` (153 ╤Д╨░╨╣╨╗╨░, ╨┐╤А╨╛╨╡╨║╤В `unrar/UnRAR.vcxproj`) ╨╜╨╡ ╨▓╤Е╨╛╨┤╨╕╤В ╨╜╨╕ ╨▓ ╨╛╨┤╨╜╨╛ ╤А╨╡╤И╨╡╨╜╨╕╨╡ (╤Б╨┐╨╕╤Б╨╛╨║ ╨┐╤А╨╛╨╡╨║╤В╨╛╨▓ `FlyLinkDC_2022.sln`: flylinkdc, bzip2, client, zlib-ng, GdiOle, boost, MediaInfoLib, ZenLib, openssl, libtorrent, lua, LevelDB, MakeDefs), ╨╜╨╡ ╨╗╨╕╨╜╨║╤Г╨╡╤В╤Б╤П ╨╜╨╕ ╨╛╨┤╨╜╨╕╨╝ ╨┐╤А╨╛╨╡╨║╤В╨╛╨╝ (`AdditionalDependencies`) ╨╕ ╨╜╨╡ ╨▓╤Л╨╖╤Л╨▓╨░╨╡╤В╤Б╤П ╨╕╨╖ ╨║╨╛╨┤╨░ (╨╜╨╡╤В `unrar.dll`/`RAROpenArchive`/`RAR_*` ╨▓ client/windows; ╤Б╨╛╨▓╨┐╨░╨┤╨╡╨╜╨╕╤П `LoadLibraryA` тАФ ╨╕╨╖ libtorrent `aux_/win_util.hpp`, `CMultiFileDialog` тАФ ╨╕╨╖ wtl `atldlgs.h`). ╨Ъ╨╛╨┤ ╨┐╨╛╨┤╨┤╨╡╤А╨╢╨║╨╕ RAR ╨▓ ╨┐╤А╨╕╨╗╨╛╨╢╨╡╨╜╨╕╨╕ ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В.
- ╨г╨┤╨░╨╗╨╡╨╜╨╛: ╨▓╨╡╤Б╤М ╨║╨░╤В╨░╨╗╨╛╨│ `unrar/`, `vendor-manifests/unrar.txt`, `vendor-patches/unrar.patch`, ╨╖╨░╨┐╨╕╤Б╤М `unrar` ╨╕╨╖ `vendor-update.py` (╨╛╤Б╤В╨░╨╗╨╛╤Б╤М 14 ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓), ╨║╨╡╤И `build-win/vendor-cache/unrar-*.tar.gz`, ╨░╤А╤В╨╡╤Д╨░╨║╤В╤Л `vendor-patches/unrar.gen` ╨╕ `vendor-patches/base`. ╨б╨╕╨╜╤В╨░╨║╤Б╨╕╤Б ╤Б╨║╤А╨╕╨┐╤В╨░ ╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜ (`ast.parse`), `--list` ╨║╨╛╤А╤А╨╡╨║╤В╨╡╨╜.
- ╨Я╤А╨╛╨▓╨╡╤А╨║╨░: Release|x64 Build тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║, exit=0, EXE=20656640 (╨▒╨╡╨╖ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╨╣).

### ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╤Е ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║: libtorrent 2.1.2 ╨╕ mediainfolib 26.10 (╤А╨╡╨▒╨░╨╖ ╤Д╨╛╤А╨║╨╛╨▓)
- libtorrent 2.1.0 -> 2.1.2. ╨С╨░╨╖╨░ ╤Д╨╛╤А╨║╨░ ╨╛╨┐╤А╨╡╨┤╨╡╨╗╨╡╨╜╨░ ╨║╨░╨║ upstream `e2b12e72` (RC_2_1, 2025-07-26) тАФ ╤А╨╡╨▒╨░╨╖ ╨┐╨╛ ╨╜╨╡╨╣ ╨┤╨░╨╗ ╨▓╤Б╨╡╨│╨╛ 2 ╨║╨╛╨╜╤Д╨╗╨╕╨║╤В╨░ (╨▓╨╝╨╡╤Б╤В╨╛ 27 ╨┐╤А╨╕ ╨╜╨╡╨▓╨╡╤А╨╜╨╛╨╣ ╨▒╨░╨╖╨╡): `src/http_parser.cpp` (╨║╨▓╨░╨╗╨╕╤Д╨╕╨║╨░╤Ж╨╕╤П 5 ╨▓╤Л╨╖╨╛╨▓╨╛╨▓ `libtorrent::read_until`) ╨╕ `src/identify_client.cpp` (╨┐╨╡╤А╨╡╨╜╨╛╤Б `-MG`/`Media Get` ╨▓ ╨╜╨░╤З╨░╨╗╨╛ ╤В╨░╨▒╨╗╨╕╤Ж╤Л `std::array<generic_map_entry, 37>`). ╨Я╨░╤В╤З ╨┐╨╡╤А╨╡╨│╨╡╨╜╨╡╤А╨╕╤А╨╛╨▓╨░╨╜ (`vendor-patches/libtorrent.patch`, 16081 ╨▒, 12 ╤Д╨░╨╣╨╗╨╛╨▓); verify 499/0 ╤А╨░╤Б╤Е╨╛╨╢╨┤╨╡╨╜╨╕╨╣.
- libtorrent 2.1.2 ╨┐╤А╨╕╨╜╤С╤Б ╨╜╨╛╨▓╤Л╨╣ disk-I/O ╨▒╤Н╨║╨╡╨╜╨┤ pread: 14 ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╨╛╨▓╨░╨▓╤И╨╕╤Е ╨▓ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В╨╡ ╤Д╨░╨╣╨╗╨╛╨▓ (`src/{pread_disk_io,pread_storage,disk_cache,back_pressure,precomputed_block_hashes}.cpp`, `include/libtorrent/pread_disk_io.hpp`, `include/libtorrent/aux_/{pread_disk_job,pread_storage,disk_cache,back_pressure,precomputed_block_hashes,merge_block_requests,visit_block_iovecs}.hpp`, `include/libtorrent/random.hpp`) ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л ╨▓ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В (485->499), ╨┤╨╡╤А╨╡╨▓╨╛, `libtorrent_2022.vcxproj` (+8 ClInclude/+5 ClCompile) ╨╕ `.filters`. `vendor-update.py --vendor libtorrent` ╨▓╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╛╨┤╨╕╤В: add=0 update=12 delete=0 unchanged=487.
- mediainfolib 25.07 -> 26.10. ╨С╨░╨╖╨░ ╤Д╨╛╤А╨║╨░ ╨╛╨┐╤А╨╡╨┤╨╡╨╗╨╡╨╜╨░ ╨║╨░╨║ ╤В╨╡╨│ v25.07 (2025-07-29, `History_DLL.txt` ╤Б╨╛╨▓╨┐╨░╨╗); ╤А╨╡╨▒╨░╨╖ 26.10 -> 17 ╨║╨╛╨╜╤Д╨╗╨╕╨║╤В╨╛╨▓, ╤А╨░╨╖╤А╨╡╤И╨╡╨╜╤Л ╨▓╤А╤Г╤З╨╜╤Г╤О best-effort:
  - ╨▓╨╖╤П╤В╤Л upstream: ╤З╨╕╤Б╤В╨║╨╕ ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ (╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨░╤Ж╨╕╤П/╤Г╨┤╨░╨╗╨╡╨╜╨╕╨╡ ╨╜╨╡╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╨╝╤Л╤Е ╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╤Е ╨▓ File_Ac4/File_Dts/File_Usac/File_Pcm_Vob/File_DolbyE), ╨╜╨╛╨▓╤Л╨╡ ╤Б╨╕╨│╨╜╨░╤В╤Г╤А╤Л File__Analyze.h, File_Jpeg.h (+ File_Jpeg.cpp), ╤А╨╡╤Д╨░╨║╤В╨╛╤А╨╕╨╜╨│ File_Mk.cpp/File_Mk.h (unique_ptr/BlockAdditions), File_Mpeg_Descriptors.cpp, File_Avc.cpp, ╨░ ╤В╨░╨║╨╢╨╡ ╨║╤А╤Г╨┐╨╜╨╛╨╡ ╤А╨░╤Б╤И╨╕╤А╨╡╨╜╨╕╨╡ File_Av1.{cpp,h} (metadata_itu_t_t35_B5_5890, metadata_scalability, scalability_structure, metadata_timecode).
  - ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╤Л ╨┐╤А╨░╨▓╨║╨╕ ╤Д╨╛╤А╨║╨░: zlib-ng (`zng_stream`/`zng_inflate`/`zng_inflateEnd` + guard `MEDIAINFO_ICC_YES`) ╨▓ File_Png.cpp; `zng_inflate` ╨▓ File_Mpeg4_Elements.cpp (+ upstream-╨║╨░╤Б╤В `(uInt)`); ╨┤╨╛╨┐╨╛╨╗╨╜╨╕╤В╨╡╨╗╤М╨╜╤Л╨╡ `#include` ╨▓ PreComp.h; ╨▓╤Л╤З╨╕╤В╨░╨╜╨╕╨╡ `Offset` ╨▓ File_Pac.cpp; ╤Д╨╛╤А╨║-╨║╨╗╨░╤Б╤Б `File__Base_Avc_Hevc` (File__Analyze.h, ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╤В╤Б╤П ╨▓ File_Avc.h/File_Hevc.h).
- mediainfolib 26.10 ╨┤╨╛╨▒╨░╨▓╨╕╨╗ 3 ╨╜╨╛╨▓╤Л╤Е ╨┐╨░╤А╤Б╨╡╤А╨░: `Source/MediaInfo/Image/File_AmigaIcon.{cpp,h}`, `Source/MediaInfo/Video/File_Apv.{cpp,h}`, `Source/MediaInfo/Video/File_Av2.{cpp,h}` тАФ ╨╜╨░ ╨╜╨╕╤Е ╤Б╤Б╤Л╨╗╨░╤О╤В╤Б╤П File_Ivf.cpp/File_Mk.cpp/File_Mpeg4_Elements.cpp/File__MultipleParsing.cpp/MediaInfo_File.cpp (╨╝╨░╨║╤А╨╛╤Б╤Л `MEDIAINFO_AV2_YES`/`APV_YES`/`AMIGAICON_YES` ╨▓╨║╨╗╤О╤З╨╡╨╜╤Л ╨┐╨╛ ╤Г╨╝╨╛╨╗╤З╨░╨╜╨╕╤О ╨▓ Setup.h 26.10). ╨д╨░╨╣╨╗╤Л ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л ╨▓ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В (509->515), ╨┤╨╡╤А╨╡╨▓╨╛, `MediaInfoLib_2022.vcxproj` (+3 ClInclude/+3 ClCompile) ╨╕ `.filters`. ╨Я╤А╨╛╨▓╨╡╤А╨╡╨╜╨╛: ╨▓╤Б╨╡ `#include "MediaInfo/..."` ╤А╨╡╨╖╨╛╨╗╨▓╤П╤В╤Б╤П (0 ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╤О╤Й╨╕╤Е). SHA1/SHA2/AES ╨╛╤В╨║╨╗╤О╤З╨╡╨╜╤Л (Setup.h -> `MEDIAINFO_SHA*_NO`/`MEDIAINFO_AES_NO`; ╨▓ vcxproj `MEDIAINFO_AES_NO`), libcurl ╨╛╤В╨║╨╗╤О╤З╤С╨╜ тАФ aes/sha/hmac-╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╕ ╨╜╨╡ ╤В╤А╨╡╨▒╤Г╤О╤В╤Б╤П.
  - ╨Я╨╡╤А╨╡╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╤П: `vendor-patches/mediainfolib.patch` (172771 ╨▒, 110 ╤Д╨░╨╣╨╗╨╛╨▓; 6 ╨╜╨╛╨▓╤Л╤Е ╤Д╨░╨╣╨╗╨╛╨▓ ╨╕╨┤╨╡╨╜╤В╨╕╤З╨╜╤Л upstream ╨╕ ╨▓ ╨┐╨░╤В╤З ╨╜╨╡ ╨▓╤Е╨╛╨┤╤П╤В) тАФ ╨▓╤Л╨┐╨╛╨╗╨╜╨╡╨╜╨░ ╨┐╨╛╤Б╨╗╨╡ ╨▓╨╜╨╡╤Б╨╡╨╜╨╕╤П ╨┐╤А╨░╨▓╨╛╨║ ╤Б╨▒╨╛╤А╨║╨╕; verify 515/0; `vendor-update.py --vendor mediainfolib` -> add=0 update=110 delete=0 unchanged=405.
- ╨Ч╨░╨║╨╛╨╝╨╝╨╕╤З╨╡╨╜╨╛: libtorrent тАФ `59ea1ac3`, mediainfolib тАФ `9d541b1f`, ╨┐╤А╨░╨▓╨║╨╕ ╤Б╨▒╨╛╤А╨╛╤З╨╜╤Л╤Е ╨╛╤И╨╕╨▒╨╛╨║ mediainfolib 26.10 тАФ `47f48519`.
- ╨Ъ╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ MSVC (VS 18 Insiders) Debug x64: ╨┐╨╛╨╗╨╜╨░╤П solution тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║ (exe 54387200 ╨▒). ╨б╨▒╨╛╤А╨╛╤З╨╜╤Л╨╡ ╨╛╤И╨╕╨▒╨║╨╕ mediainfolib ╨╕ ╨╕╤Е ╨┐╤А╨░╨▓╨║╨╕: `File__Analyze.h` (╨▓╨╛╨╖╨▓╤А╨░╤Й╨╡╨╜╤Л ╤Д╨╛╤А╨║╨╛╨▓╤Л╨╡ reason-╤Б╤В╤А╨╛╨║╨╕ `Trusted_IsNot("RanOutOfData")`/`"SynchLost"` ╨┐╤А╨╕ upstream-╤Б╨╕╨│╨╜╨░╤В╤Г╤А╨╡ `SynchLost` ╨╕╨╖ 3 ╨░╤А╨│╤Г╨╝╨╡╨╜╤В╨╛╨▓), `Image/File_Png.cpp` (╤Г╨▒╤А╨░╨╜ ╨┤╤Г╨▒╨╗╨╕╤А╤Г╤О╤Й╨╕╨╣ ╨▓╨╜╤Г╤В╤А╨╡╨╜╨╜╨╕╨╣ guard `MEDIAINFO_ICC_YES`, ╨╛╤Б╤В╨░╨▓╨╗╨╡╨╜ ╨▓╨╜╨╡╤И╨╜╨╕╨╣ upstream-guard), `Multiple/File_Mk.cpp` (`to_string` -> `std::to_string`, 2 ╨╝╨╡╤Б╤В╨░), `Image/File_Jpeg.cpp` ╨╕ `Video/File_Av1.cpp` (`#define FMT_UNICODE 0` ╨┐╨╡╤А╨╡╨┤ `ThirdParty/fmt/format.h`). Release x64 (Build ╨╕ ╤З╨╕╤Б╤В╤Л╨╣ Rebuild solution) тАФ ╤В╨╛╨╢╨╡ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║ (exe 20661760 ╨▒).
- ╨Я╨╛╨▒╨╛╤З╨╜╨╛ ╨╕╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜╨░ ╤Б╨▒╨╛╤А╨║╨░ phase D ╤В╤С╨╝╨╜╨╛╨╣ ╤В╨╡╨╝╤Л (`e543387e`): `client/FlyFeatures/PropertyList.h` (`Colors::getSysColor` -> `FlyTheme::getSysColor` + `#include "Colors.h"`, ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╣ `CBrush::CreateSolidBrush` ╨▓╨╝╨╡╤Б╤В╨╛ `CBrush(COLORREF)`) ╨╕ `windows/AppearancePage.cpp` (combo `AddString` ╤Б `const std::wstring` -> `.c_str()`). ╨Т `.gitignore` ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜ `vc17/` (╨░╤А╤В╨╡╤Д╨░╨║╤В╤Л MSBuild ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓).

### ╨в╤С╨╝╨╜╤Л╨╣ ╤А╨╡╨╢╨╕╨╝ ╨╕╨╜╤В╨╡╤А╤Д╨╡╨╣╤Б╨░: ╨╝╨╡╨╜╤О ╨╕ ╨╛╨▒╤Й╨╕╨╡ ╨║╨╛╨╜╤В╤А╨╛╨╗╤Л (uxtheme)
- `WinUtil::initDarkMode()` тАФ ╤В╤С╨╝╨╜╤Л╨╡ ╨╝╨╡╨╜╤О: ╨┤╨╕╨╜╨░╨╝╨╕╤З╨╡╤Б╨║╨╕ ╨╖╨░╨│╤А╤Г╨╢╨░╨╡╤В ╨╕╨╖ `uxtheme.dll` ╨╜╨╡╨┤╨╛╨║╤Г╨╝╨╡╨╜╤В╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╨╡ `SetPreferredAppMode` (ordinal 135) ╨╕ `FlushMenuThemes` (ordinal 136); ╨▓╤Л╨┐╨╛╨╗╨╜╤П╨╡╤В╤Б╤П ╤В╨╛╨╗╤М╨║╨╛ ╨┐╤А╨╕ ╤В╤С╨╝╨╜╨╛╨╣ ╤В╨╡╨╝╨╡ ╨╕ Windows 10 build >= 17763 (╨╕╨╜╨░╤З╨╡ no-op).
- `WinUtil::applyDarkMode(HWND)` тАФ ╤В╤С╨╝╨╜╤Л╨╣ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨╛╨║ ╨╛╨║╨╜╨░ (`DwmSetWindowAttribute`) + `AllowDarkModeForWindow` (ordinal 133) + ╤А╨╡╨║╤Г╤А╤Б╨╕╨▓╨╜╤Л╨╣ `SetWindowTheme(L"DarkMode_Explorer")` ╨┤╨╗╤П ╨╛╨▒╤Й╨╕╤Е ╨║╨╛╨╜╤В╤А╨╛╨╗╨╛╨▓ (listview/treeview/header/scrollbar/edit/combo/statusbar/tooltip) тАФ ╤В╤С╨╝╨╜╤Л╨╡ ╤Б╨║╤А╨╛╨╗╨╗╨▒╨░╤А╤Л ╨╕ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╕ ╨║╨╛╨╗╨╛╨╜╨╛╨║; ╨┐╤А╨╕ ╤Б╨▓╨╡╤В╨╗╨╛╨╣ ╤В╨╡╨╝╨╡ ╤В╨╡╨╝╨░ ╨║╨╛╨╜╤В╤А╨╛╨╗╨╛╨▓ ╨▓╨╛╨╖╨▓╤А╨░╤Й╨░╨╡╤В╤Б╤П (`SetWindowTheme(..., nullptr, ...)`). `WinUtil::applyDarkModeToAllWindows()` ╨┐╤А╨╕╨╝╨╡╨╜╤П╨╡╤В ╨║╨╛ ╨▓╤Б╨╡╨╝ top-level ╨╛╨║╨╜╨░╨╝ ╨┐╤А╨╛╤Ж╨╡╤Б╤Б╨░ (`EnumWindows`).
- ╨в╨╛╤З╨║╨╕ ╨▓╤Л╨╖╨╛╨▓╨░: `MainFrame::OnCreate` (`initDarkMode` + `applyDarkMode`) ╨╕ `Colors::init()` (╨┐╨╡╤А╨╡╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╨╕╨╡ ╨┐╤А╨╕ ╤Б╨╝╨╡╨╜╨╡ ╤В╨╡╨╝╤Л ╨╕ ╨║╨╛ ╨▓╤Б╨╡╨╝ ╨╛╤В╨║╤А╤Л╤В╤Л╨╝ ╨╛╨║╨╜╨░╨╝).
- ╨Т╤Б╨╡ ╨▓╤Л╨╖╨╛╨▓╤Л uxtheme ╨╖╨░╤Й╨╕╤Й╨╡╨╜╤Л ╨┐╤А╨╛╨▓╨╡╤А╨║╨╛╨╣ ╨▓╨╡╤А╤Б╨╕╨╕ ╨Ю╨б ╨╕ ╨┤╨╕╨╜╨░╨╝╨╕╤З╨╡╤Б╨║╨╛╨╣ ╨╖╨░╨│╤А╤Г╨╖╨║╨╛╨╣; ╨╜╨░ ╤Б╤В╨░╤А╤Л╤Е ╨Ю╨б ╨┐╨╛╨▓╨╡╨┤╨╡╨╜╨╕╨╡ ╨┐╤А╨╡╨╢╨╜╨╡╨╡ (╤В╨╛╨╗╤М╨║╨╛ ╤В╤С╨╝╨╜╤Л╨╣ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨╛╨║).
- ╨в╤С╨╝╨╜╤Л╨╣ ╤А╨╡╨╢╨╕╨╝ ╨┐╤А╨╕╨╝╨╡╨╜╤П╨╡╤В╤Б╤П ╨╕ ╨║ ╨▓╨╜╨╛╨▓╤М ╨╛╤В╨║╤А╤Л╨▓╨░╨╡╨╝╤Л╨╝ MDI-╤Д╤А╨╡╨╣╨╝╨░╨╝: ╨▓ `MDITabChildWindowImpl::Create` (`windows/flattabctrl.h`) ╨┐╨╛╤Б╨╗╨╡ ╤Б╨╛╨╖╨┤╨░╨╜╨╕╤П ╨╛╨║╨╜╨░ (╨┤╨╛╤З╨╡╤А╨╜╨╕╨╡ ╨║╨╛╨╜╤В╤А╨╛╨╗╤Л ╤Г╨╢╨╡ ╤Б╤Г╤Й╨╡╤Б╤В╨▓╤Г╤О╤В) ╨▓╤Л╨╖╤Л╨▓╨░╨╡╤В╤Б╤П `WinUtil::applyDarkMode`.
- ╨а╨░╨╖╨┤╨╡╨╗╨╕╤В╨╡╨╗╨╕ ╤Б╨┐╨╗╨╕╤В╤В╨╡╤А╨╛╨▓ ╤В╨╡╨╝╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╤Л: ╨▓ `windows/SearchFrm.h` ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜ `CFlySplitterWindowT` (╨┐╨╡╤А╨╡╨╛╨┐╤А╨╡╨┤╨╡╨╗╤П╨╡╤В `DrawSplitterBar`/`DrawSplitterPane` ╤З╨╡╤А╨╡╨╖ `FlyTheme::getSysColor`), ╨┐╤А╨╕╨╝╨╡╨╜╤С╨╜ ╨║ `m_vSplit`/`m_hzSplit`.
- ╨Р╤Г╨┤╨╕╤В ╨╛╤В╤А╨╕╤Б╨╛╨▓╨║╨╕: ╨▓ ╨║╨╛╨┤╨╡ ╨┐╤А╨╕╨╗╨╛╨╢╨╡╨╜╨╕╤П ╨╜╨╡ ╨╛╤Б╤В╨░╨╗╨╛╤Б╤М ╨┐╤А╤П╨╝╤Л╤Е ╨▓╤Л╨╖╨╛╨▓╨╛╨▓ `::GetSysColor`/`FillRect(COLOR_*)` тАФ ╨▓╤Б╨╡ ╤З╨╡╤А╨╡╨╖ ╤Д╨░╤Б╨░╨┤ `Colors::getSysColor`/`FlyTheme::getSysColor`; owner-draw ╨╕ ╤В╨░╨▒-╨▒╨░╤А ╤В╨╡╨╝╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╤Л.
- ╨б╨▒╨╛╤А╨║╨░ MSVC Debug ╨╕ Release x64 ╨┐╨╛╤Б╨╗╨╡ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╨╣ тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║.

### ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ zlib-ng: develop-╨┐╨╕╨╜ (2.3.90), ╨┐╨╡╤А╨╡╤Е╨╛╨┤ ╨╜╨░ ╨║╤Г╤А╨╕╤А╤Г╨╡╨╝╤Л╨╣ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В
- zlib-ng ╨╛╨▒╨╜╨╛╨▓╨╗╤С╨╜ ╨┤╨╛ ╨┐╨╕╨╜╨░ `f18efb82` (╨▓╨╡╤А╤Б╨╕╤П 2.3.90). upstream ╤А╨╡╤Б╤В╤А╤Г╨║╤В╤Г╤А╨╕╤А╨╛╨▓╨░╨╗ ╨┤╨╡╤А╨╡╨▓╨╛:
  `zlib-ng.h`/`zconf-ng.h` ╤Б╤В╨░╨╗╨╕ ╨│╨╡╨╜╨╡╤А╨╕╤А╤Г╨╡╨╝╤Л╨╝╨╕ ╨╕╨╖ `*.h.in`, ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜
  `zlib_name_mangling-ng.h`; ╤Г╨┤╨░╨╗╨╡╨╜╤Л `crc32.h`, `crc32_fold*`, `insert_string_roll.c`,
  `insert_string_tpl.h`, `arch/generic/{adler32_fold_c,crc32_c,crc32_fold_c}.c`,
  `arch/x86/{chorba_sse2,chorba_sse41,crc32_vpclmulqdq}.c`. ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л
  `zconf-ng.h.in`/`zlib-ng.h.in`/`zlib_name_mangling-ng.h(.in)`, `gzread.c`,
  `zarch.h`, `zsanitizer.h`, `arch_natives.h`, `arch/shared/*`,
  `arch/x86/{adler32_avx2_vnni,crc32_chorba_sse2,crc32_chorba_sse41,
  crc32_vpclmulqdq_avx2,crc32_vpclmulqdq_avx512}.c`. ╨Ч╨░╨│╨╛╨╗╨╛╨▓╨║╨╕ ╤Б╨│╨╡╨╜╨╡╤А╨╕╤А╨╛╨▓╨░╨╜╤Л CMake
  (`-G "Visual Studio 18 2026" -DZLIB_COMPAT=OFF`) ╨╕ ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╤Л ╨▓ LF.
- `zlib-ng_2022.vcxproj` (+`.filters`) ╨┐╨╡╤А╨╡╤Б╨╛╨▒╤А╨░╨╜ ╨┐╨╛╨┤ ╨╜╨╛╨▓╤Л╨╣ ╨╜╨░╨▒╨╛╤А ╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╛╨▓
  (ClCompile 48, ClInclude 52).
- ╨Т╨▓╨╡╨┤╤С╨╜ ╨║╤Г╤А╨╕╤А╤Г╨╡╨╝╤Л╨╣ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В `vendor-manifests/zlib-ng.txt` (╤В╨╛╨╗╤М╨║╨╛ library-╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╕,
  ╨▒╨╡╨╖ test/tools/CMakeLists/╤З╤Г╨╢╨╕╤Е ╨░╤А╤Е╨╕╤В╨╡╨║╤В╤Г╤А); `vendor-update.py` ╨┤╨╗╤П zlib-ng ╨┐╨╡╤А╨╡╨▓╨╡╨┤╤С╨╜
  ╤Б `'tree': True` ╨╜╨░ `'select'`. ╨Т╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╡╨┤╨╡╨╜╨╕╨╡: add=0 update=0 delete=0 unchanged=106.
- ╨б╨▒╨╛╤А╨║╨░ MSVC (VS 18 Insiders): Debug x64 Rebuild тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║
  (exe 54457856); Release x64 Rebuild тАФ 0/0 (exe 20656640).

### ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨▓╨╡╨╜╨┤╨╛╤А╨░ maxminddb 1.7.1 -> 1.14.1
- ╨Р╤Г╨┤╨╕╤В upstream-╤А╨╡╨╗╨╕╨╖╨╛╨▓: ╨▒╨╛╨╗╤М╤И╨╕╨╜╤Б╤В╨▓╨╛ ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓ ╨░╨║╤В╤Г╨░╨╗╤М╨╜╤Л; ╨╡╨┤╨╕╨╜╤Б╤В╨▓╨╡╨╜╨╜╨╛╨╡ ╨┐╤А╨╕╨╝╨╡╨╜╤С╨╜╨╜╨╛╨╡
  ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ тАФ maxminddb. boost 1.92 / lua 5.5 / unrar 7.x ╨┤╨╛╤Б╤В╤Г╨┐╨╜╤Л, ╨╜╨╛ ╨╛╤В╨╗╨╛╨╢╨╡╨╜╤Л (╨╝╨░╨╢╨╛╤А/╨║╤А╤Г╨┐╨╜╨╛).
- maxminddb 1.7.1 -> 1.14.1. upstream ╤Г╨╢╨╡ ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В ╨┐╨╛╤З╤В╨╕ ╨▓╨╡╤Б╤М ╤Д╨╛╤А╨║-╤Е╨░╤А╨┤╨╡╨╜╨╕╨╜╨│:
  `can_multiply` (╨╜╨╡-╤Б╤В╨░╤В╨╕╤З╨╡╤Б╨║╨╕╨╣ + ╨┤╨╡╨║╨╗╨░╤А╨░╤Ж╨╕╤П), `SSIZE_MAX`, `sa_family_t` = ADDRESS_FAMILY,
  ╨┐╤А╨╛╨▓╨╡╤А╨║╨╕ overflow (`data_section_size`, `uint64_t value`, `right_record_offset`), clang-pragma.
  ╨Я╨╛╤Н╤В╨╛╨╝╤Г ╨╜╨╛╨▓╤Л╨╣ ╨┐╨░╤В╤З ╤Б╨▓╤С╨╗╤Б╤П ╨║ ╨╝╨╕╨╜╨╕╨╝╤Г╨╝╤Г: `#pragma warning(push/disable:4201/pop)`
  ╨▓ `maxminddb.h` ╨┤╨╗╤П ╨░╨╜╨╛╨╜╨╕╨╝╨╜╨╛╨│╨╛ union (MSVC).
- upstream ╤Г╨┤╨░╨╗╨╕╨╗ `projects/VS12/maxminddb_config.h`; ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╣ `maxminddb/maxminddb_config.h`
  ╨▒╨╛╨╗╤М╤И╨╡ ╨╜╨╡ ╤Б╨╕╨╜╨║╨░╨╡╤В╤Б╤П (extra-╤Д╨░╨╣╨╗) ╨╕ ╨╖╨░╨┤╨░╤С╤В `MMDB_UINT128_IS_BYTE_ARRAY=1` ╨╕
  `PACKAGE_VERSION "1.14.1"` (╨▓ upstream ╨┐╨╛╤Б╨╗╨╡╨┤╨╜╨╕╨╣ ╨▒╨╡╤А╤С╤В╤Б╤П ╨╕╨╖ autoconf config.h). ╨Т `vendor-update.py` select
  ╨╛╨▒╨╜╨╛╨▓╨╗╤С╨╜ (╤Г╨▒╤А╨░╨╜ projects/VS12), URL -> 1.14.1, version-╨┐╤Г╤В╤М -> maxminddb_config.h.
- ╨б╨▒╨╛╤А╨║╨░ MSVC (VS 18 Insiders): Debug x64 Rebuild тАФ 0/0 (exe 54460416);
  Release x64 Rebuild тАФ 0/0 (exe 20656640). maxminddb ╨║╨╛╨╝╨┐╨╕╨╗╨╕╤А╤Г╨╡╤В╤Б╤П ╨▓ client_2022.vcxproj.

## 2026-10-12
### ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╤Е ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║ ╨┤╨╛ ╨░╨║╤В╤Г╨░╨╗╤М╨╜╤Л╤Е ╨▓╨╡╤А╤Б╨╕╨╣ (vendor-update.py)
- ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╤Л ╨╕ ╨▓╤Л╨▓╨╡╤А╨╡╨╜╤Л 1:1 (upstream + ╨┐╨░╤В╤З == ╨┤╨╡╤А╨╡╨▓╨╛, 0 ╤А╨░╤Б╤Е╨╛╨╢╨┤╨╡╨╜╨╕╨╣): lua 5.4.6 -> 5.4.9 (╨┐╨░╤В╤З тАФ ╤В╨╛╨╗╤М╨║╨╛ ╨▓╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ $Id: ╨▓ lzio.c/lzio.h), unrar 6.23 -> 6.24 (unrarsrc-6.2.12; ╨┐╨░╤В╤З тАФ UnRAR.vcxproj, win32stm.cpp), jsoncpp 1.9.6 -> 1.9.8, snappy 1.2.0 -> 1.3.1.
- jsoncpp: ╨┐╨░╤В╤З ╤А╨╡╨╖╨║╨╛ ╤Б╨╛╨║╤А╨░╤Й╤С╨╜ тАФ ╨▒╨╛╨╗╤М╤И╨░╤П ╤З╨░╤Б╤В╤М ╤Б╤В╨░╤А╨╛╨│╨╛ ╨┐╨░╤В╤З╨░ ╨╛╨║╨░╨╖╨░╨╗╨░╤Б╤М upstream-╨┤╤А╨╡╨╣╤Д╨╛╨╝ ╨╝╨╡╨╢╨┤╤Г 1.9.6 ╨╕ ╨▒╨░╨╖╨╛╨╣ ╤Д╨╛╤А╨║╨░ (╨▓ 1.9.8 ╤Н╤В╨╕ ╨┐╤А╨░╨▓╨║╨╕ ╤Г╨╢╨╡ ╨╡╤Б╤В╤М); ╨╛╤Б╤В╨░╨╗╨╕╤Б╤М ╤А╨╡╨░╨╗╤М╨╜╤Л╨╡ ╨┐╤А╨░╨▓╨║╨╕ FlylinkDC: #include "stdinc.h", toStyledString(bool), ╨┐╤Г╨▒╨╗╨╕╤З╨╜╤Л╨╣ use_end_line_, #pragma warning(disable:4389) ╨▓╨╛╨║╤А╤Г╨│ InRange.
- snappy: ╨┐╨░╤В╤З ╤Г╨┤╨░╨╗╤С╨╜ ╤Ж╨╡╨╗╨╕╨║╨╛╨╝ тАФ upstream 1.3.1 ╤Б╨░╨╝ ╨┐╤А╨╡╨┤╨╛╤Б╤В╨░╨▓╨╗╤П╨╡╤В ╤В╨╡ ╨╢╨╡ overload-╤Л ╨▒╨╡╨╖ CompressionOptions, ╨║╨╛╤В╨╛╤А╤Л╨╡ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╣ ╨┐╨░╤В╤З ╨┤╨╛╨▒╨░╨▓╨╗╤П╨╗ ╨▓╤А╤Г╤З╨╜╤Г╤О.
- ╨Ш╨╜╤Б╤В╤А╤Г╨╝╨╡╨╜╤В╤Л: genpatch.py (LF-╨╜╨╛╤А╨╝╨░╨╗╨╕╨╖╨░╤Ж╨╕╤П ╨╛╨▒╨╡╨╕╤Е ╤Б╤В╨╛╤А╨╛╨╜, ╨╝╨░╨┐╨┐╨╕╨╜╨│ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В╨░ src->out) ╨╕ verify.py (upstream+╨┐╨░╤В╤З == ╨┤╨╡╤А╨╡╨▓╨╛); ╤А╨╡╨▒╨░╨╖ ╤Д╨╛╤А╨║╨╛╨▓ тАФ 3-way ╤З╨╡╤А╨╡╨╖ git merge-file.
- ╨Ю╤В╨╗╨╛╨╢╨╡╨╜╨╛ (╤Б╨╝. TODO.md): libtorrent 2.1.0 -> 2.1.2 (╤А╨╡╨░╨╗╤М╨╜╤Л╨╣ ╤Д╨╛╤А╨║, 196 ╤Д╨░╨╣╨╗╨╛╨▓, 27 ╨║╨╛╨╜╤Д╨╗╨╕╨║╤В╨╛╨▓ ╤А╨╡╨▒╨░╨╖╨░) ╨╕ mediainfolib 25.07 -> 26.10 (116 ╤Д╨░╨╣╨╗╨╛╨▓, 17 ╨║╨╛╨╜╤Д╨╗╨╕╨║╤В╨╛╨▓) тАФ ╨╜╤Г╨╢╨╡╨╜ ╤А╤Г╤З╨╜╨╛╨╣ ╨┐╨╛╤А╤В ╨╕ ╨║╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ MSVC; ╨┤╨╡╤А╨╡╨▓╤М╤П ╨▓╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜╤Л ╨║ ╤В╨╡╨║╤Г╤Й╨╕╨╝ ╨▓╨╡╤А╤Б╨╕╤П╨╝.
### ╨б╨║╤А╨╕╨┐╤В ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╤П ╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╤Е ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║ vendor-update.py
- ╨а╨╡╨░╨╗╨╕╨╖╨╛╨▓╨░╨╜ `vendor-update.py`: ╤Б╨║╨░╤З╨╕╨▓╨░╨╡╤В ╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╨╡ ╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╕ ╤Б upstream-URL, ╤Б╨╕╨╜╤Е╤А╨╛╨╜╨╕╨╖╨╕╤А╤Г╨╡╤В ╨╕╤Е ╨▓ ╨┤╨╡╤А╨╡╨▓╨╛ ╨┐╤А╨╛╨╡╨║╤В╨░, ╨╖╨░╨╜╨╛╨▓╨╛ ╨╜╨░╨║╨╗╨░╨┤╤Л╨▓╨░╨╡╤В ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╨┐╨░╤В╤З╨╕ ╨╕ ╨┤╨╡╨╗╨░╨╡╤В git-commit. ╨Ъ╨╛╨╜╤Д╨╕╨│ тАФ ╤В╨░╨▒╨╗╨╕╤Ж╨░ `VENDORS` (url/pin, ╨╗╨╛╨║╨░╨╗╤М╨╜╨░╤П ╨┤╨╕╤А╨╡╨║╤В╨╛╤А╨╕╤П, ╤А╨╡╨╢╨╕╨╝ ╨╕╨╝╨┐╨╛╤А╤В╨░ ╤Д╨░╨╣╨╗╨╛╨▓, ╤Б╨┐╨╕╤Б╨╛╨║ ╨┐╨░╤В╤З╨╡╨╣, ╨╝╨░╤А╨║╨╡╤А ╨▓╨╡╤А╤Б╨╕╨╕). ╨а╨╡╨╢╨╕╨╝╤Л: `--list`, `--dry-run`, `--no-commit`, `--vendor NAME`. ╨Р╤А╤В╨╡╤Д╨░╨║╤В╤Л ╨░╤А╤Е╨╕╨▓╨╕╤А╤Г╤О╤В╤Б╤П ╨▓ `build-win/vendor-cache` (╨▓ .gitignore).
- SQLite: `client/sqlite` ╨╛╨▒╨╜╨╛╨▓╨╗╤С╨╜ ╤Б 3.50.4 ╨┤╨╛ 3.54.0 (╨░╨╝╨╛╨╗╨│╨░╨╝╨░╤Ж╨╕╤П `sqlite-amalgamation-3540000.zip`), ╨╖╨░╨║╨╛╨╝╨╝╨╕╤З╨╡╨╜╨╛ `45872dc1`. ╨Ы╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╨┐╨░╤В╤З╨╕ ╨┐╤А╨╛╨╡╨║╤В╨░ (╨▓ sqlite3.c: `#include "sqlite_fly.h"` ╨╕ `#pragma warning(disable: 4127/4244/4100)`; ╨▓ sqlite3.h: `#pragma once` ╨╕ `#include "sqlite_fly.h"`) ╨╛╨┐╨╕╤Б╨░╨╜╤Л ╨▓ ╨║╨╛╨╜╤Д╨╕╨│╨╡ ╨╕ ╨╖╨░╨╜╨╛╨▓╨╛ ╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╤Л ╨║ ╨╜╨╛╨▓╨╛╨╣ ╨▓╨╡╤А╤Б╨╕╨╕.
- WTL: ╨▓╤Л╨▓╨╡╤А╨╡╨╜╨╛, ╤З╤В╨╛ ╨▓╨╡╤В╨║╨░ 10.0.10320 ╤Б 5 ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╝╨╕ ╨┐╨░╤В╤З╨░╨╝╨╕ (atlcrack.h тАФ ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨░╤Ж╨╕╤П `m_bMsgHandled`; atldlgs.h тАФ default-╨░╤А╨│╤Г╨╝╨╡╨╜╤В `0U` ╤Г `CPropertyPageImpl`/`CPropertyPage`; atlmisc.h тАФ `_T("file://")`; atlribbon.h тАФ include ╤З╨╡╤А╨╡╨╖ ╨║╨░╨▓╤Л╤З╨║╨╕; atlsplit.h тАФ `m_nPropMax = 10000` ╨╕ ╤А╨░╨╖╨▒╨╕╨╡╨╜╨╕╨╡ ╤А╨░╤Б╤З╤С╤В╨░ ╨┐╨╛╨╖╨╕╤Ж╨╕╨╕) ╨┤╨░╤С╤В 1:1 ╨▓╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╡╨┤╨╡╨╜╨╕╨╡ ╨┤╨╡╤А╨╡╨▓╨░, ╨║╨╛╨╝╨╝╨╕╤В ╨╜╨╡ ╤В╤А╨╡╨▒╤Г╨╡╤В╤Б╤П.
- zlib-ng: ╨┐╨╕╨╜╨╕╤А╨╛╨▓╨░╨╜ ╨╜╨░ develop HEAD `f18efb822a2fc24a1209a836d51bb217476fc08e` (╨▓ ╨║╨╛╨╜╤Д╨╕╨│╨╡, dry-run: 326 ╨╜╨╛╨▓╤Л╤Е / 84 ╨╕╨╖╨╝╨╡╨╜╤С╨╜╨╜╤Л╤Е / 14 ╤Г╨┤╨░╨╗╤С╨╜╨╜╤Л╤Е ╤Д╨░╨╣╨╗╨╛╨▓). ╨Ъ╨╛╨╝╨╝╨╕╤В ╨╛╤В╨╗╨╛╨╢╨╡╨╜: upstream ╨┐╨╡╤А╨╡╤Б╤В╤А╨╛╨╕╨╗ ╨┤╨╡╤А╨╡╨▓╨╛ (zlib-ng.h/zconf-ng.h ╤Б╤В╨░╨╗╨╕ ╨│╨╡╨╜╨╡╤А╨╕╤А╤Г╨╡╨╝╤Л╨╝╨╕ `.in`, ╨┐╨╡╤А╨╡╨╕╨╝╨╡╨╜╨╛╨▓╨░╨╜╤Л crc32.h/insert_string_roll.c ╨╕ ╨┤╤А.), ╤В╤А╨╡╨▒╤Г╨╡╤В╤Б╤П ╨┐╤А╨░╨▓╨║╨░ zlib-ng_2022.vcxproj ╨╕ ╨║╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ тАФ ╤Б╨╝. TODO.md.
- ╨Т AGENTS.md ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╨╛ ╨┐╤А╨░╨▓╨╕╨╗╨╛ ╤Д╨╕╨║╤Б╨╕╤А╨╛╨▓╨░╤В╤М ╨╖╨░╨┤╨░╤З╨╕ ╨╕ ╨┐╨╗╨░╨╜ ╤Б╨╡╤Б╤Б╨╕╨╕ ╨▓ TODO.md ╨┤╨╗╤П ╨▓╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜╨╕╤П ╨║╨╛╨╜╤В╨╡╨║╤Б╤В╨░ ╨┐╨╛╤Б╨╗╨╡ ╤А╨░╨╖╤А╤Л╨▓╨░ ╤Б╨╡╤Б╤Б╨╕╨╕.
### ╨в╤С╨╝╨╜╨░╤П ╤В╨╡╨╝╨░ ╨╕╨╜╤В╨╡╤А╤Д╨╡╨╣╤Б╨░ WTL (Light/Dark)
- ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜╨░ ╨╜╨░╤Б╤В╤А╨╛╨╣╨║╨░ `THEME` (`SettingsManager::IntSetting`) + enum `ThemeMode { THEME_LIGHT, THEME_DARK }` ╨╕ `static bool isDarkTheme()` (client/SettingsManager.h/.cpp). ╨в╨╡╨│ `"Theme"`, ╨┤╨╡╤Д╨╛╨╗╤В `THEME_LIGHT` ╨▓ `setDefaults()`, ╨╕╨╝╨┐╨╛╤А╤В/╤Н╨║╤Б╨┐╨╛╤А╤В ╨▓ dctheme (`importData`/`exportData("Theme", THEME)`).
- ╨в╤С╨╝╨╜╨░╤П ╨┐╨░╨╗╨╕╤В╤А╨░ ╨┐╨╡╤А╨╡╨╛╨┐╤А╨╡╨┤╨╡╨╗╤П╨╡╤В ╤В╨╛╨╗╤М╨║╨╛ ╨┤╨╡╤Д╨╛╨╗╤В╤Л: ╨▓ `SettingsManager::get(IntSetting)` ╨┐╤А╨╕ `!isSet[key]` ╨╕ ╨▓╨║╨╗╤О╤З╤С╨╜╨╜╨╛╨╣ ╤В╤С╨╝╨╜╨╛╨╣ ╤В╨╡╨╝╨╡ ╨┐╤А╨╕╨╝╨╡╨╜╤П╨╡╤В╤Б╤П `getDarkThemeColor(key, value)` (~70 ╤Ж╨▓╨╡╤В╨╛╨▓╤Л╤Е ╨║╨╗╤О╤З╨╡╨╣ ╤З╨░╤В╨░, ╤Б╨┐╨╕╤Б╨║╨╛╨▓, ╨┐╤А╨╛╨│╤А╨╡╤Б╤Б╨░, ╨┐╨╛╨┐╨░╨┐╨╛╨▓, ╤В╨░╨▒╨╛╨▓, userlist). ╨Я╨╛╨╗╤М╨╖╨╛╨▓╨░╤В╨╡╨╗╤М╤Б╨║╨╕╨╡ ╨║╨░╤Б╤В╨╛╨╝╨╕╨╖╨░╤Ж╨╕╨╕ ╤Б╨╛╤Е╤А╨░╨╜╤П╤О╤В╤Б╤П тАФ ╨┐╨╡╤А╨╡╨║╨╗╤О╤З╨╡╨╜╨╕╨╡ ╨╛╨▒╤А╨░╤В╨╜╨╛ ╨╜╨░ ╤Б╨▓╨╡╤В╨╗╤Г╤О ╨▓╨╛╨╖╨▓╤А╨░╤Й╨░╨╡╤В ╨╕╤Б╤Е╨╛╨┤╨╜╤Л╨╡ ╤Ж╨▓╨╡╤В╨░. ╨Ю╤В ╤А╨╡╨║╤Г╤А╤Б╨╕╨╕ ╨╖╨░╤Й╨╕╤Й╨░╨╡╤В ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ `key != THEME`.
- `Colors::getSysColor(int)` (windows/WinUtil.cpp) тАФ ╤В╤С╨╝╨╜╨░╤П ╨┐╨░╨╗╨╕╤В╤А╨░ ╨┤╨╗╤П ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╤Е `COLOR_*` (╨╛╨║╨╜╨░/╤Д╨╛╨╜/╤В╨╡╨║╤Б╤В/╨▓╤Л╨┤╨╡╨╗╨╡╨╜╨╕╨╡/╤Б╨╡╤А╤Л╨╣), ╨╕╨╜╨░╤З╨╡ `::GetSysColor`.
- `WinUtil::applyDarkTitleBar(HWND)` тАФ ╤В╤С╨╝╨╜╤Л╨╣ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨╛╨║ ╨╛╨║╨╜╨░ ╤З╨╡╤А╨╡╨╖ `dwmapi.dll`/`DwmSetWindowAttribute` (`DWMWA_USE_IMMERSIVE_DARK_MODE = 20`), ╨╖╨░╨│╤А╤Г╨╢╨░╨╡╤В╤Б╤П ╨┤╨╕╨╜╨░╨╝╨╕╤З╨╡╤Б╨║╨╕ (dwmapi.lib ╨╜╨╡ ╨╗╨╕╨╜╨║╤Г╨╡╤В╤Б╤П). ╨Т╤Л╨╖╤Л╨▓╨░╨╡╤В╤Б╤П ╨▓ `MainFrame::OnCreate` ╨╕ ╨┐╨╡╤А╨╡╨┐╤А╨╕╨╝╨╡╨╜╤П╨╡╤В╤Б╤П ╨▓ `Colors::init()`.
- `Colors::init()`: ╤Г╤Б╤В╤А╨░╨╜╨╡╨╜╨░ ╤Г╤В╨╡╤З╨║╨░ ╨║╨╕╤Б╤В╨╕ (╨╛╤Б╨▓╨╛╨▒╨╛╨╢╨┤╨╡╨╜╨╕╨╡ ╤Б╤В╨░╤А╨╛╨│╨╛ `g_bgBrush`), ╨▓ ╨║╨╛╨╜╤Ж╨╡ тАФ ╨┐╨╡╤А╨╡╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╨╕╨╡ ╤В╤С╨╝╨╜╨╛╨│╨╛ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨░ ╨╕ `RedrawWindow(RDW_ALLCHILDREN)` ╨│╨╗╨░╨▓╨╜╨╛╨│╨╛ ╨╛╨║╨╜╨░ ╨┤╨╗╤П ╨╢╨╕╨▓╨╛╨│╨╛ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╤П ╨┐╤А╨╕ ╨┐╨╡╤А╨╡╨║╨╗╤О╤З╨╡╨╜╨╕╨╕ ╤В╨╡╨╝╤Л.
- ╨Т╤Л╨▒╨╛╤А ╤В╨╡╨╝╤Л ╨▓ ╨╜╨░╤Б╤В╤А╨╛╨╣╨║╨░╤Е: ╨║╨╛╨╝╨▒╨╛ `IDC_THEME_COMBO` ╨╜╨░ `AppearancePage` (╨┐╨╡╤А╨╡╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜ ╨╝╤С╤А╤В╨▓╤Л╨╣ UI ╤В╨╡╨╝╨░-╨╝╨╡╨╜╨╡╨┤╨╢╨╡╤А╨░, `FLYLINKDC_USE_THEME_MANAGER` ╨▓╤Л╨║╨╗╤О╤З╨╡╨╜) тАФ `TSTRING(THEME_LIGHT)`/`TSTRING(THEME_DARK)`; ╨▓ `write()` ╨╖╨░╨┐╨╕╤Б╤М `THEME` + `Colors::init()`. ╨б╤В╤А╨╛╨║╨╕ `THEME_LIGHT/THEME_DARK` ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л ╨▓ client/StringDefs.h.
- ╨Ф╨╕╨░╨╗╨╛╨│╨╕ ╨╜╨░╤Б╤В╤А╨╛╨╡╨║ ╨║╤А╨░╤Б╤П╤В╤Б╤П ╨▓ ╤В╤С╨╝╨╜╨╛╨╝ ╤А╨╡╨╢╨╕╨╝╨╡ ╨╜╨╡╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛ ╨╛╤В `SETTINGS_WINDOW_COLORIZE` (`PropPage::OnCtlColorDlg/OnCtlColorStatic`).
- ╨Ь╨╕╨│╤А╨░╤Ж╨╕╤П `GetSysColor` тЖТ `Colors::getSysColor` ╨▓ ╨▓╨╕╨┤╨╕╨╝╤Л╤Е ╨║╨╛╨╝╨┐╨╛╨╜╨╡╨╜╤В╨░╤Е: flattabctrl.h, TypedListViewCtrl.h, TransferView.cpp, SearchFrm.cpp (╨▓ ╤В.╤З. ╤Д╨╛╨╜ ╤Б╤В╨░╤В╨╕╨║╨╛╨▓), EmoticonsDlg.cpp, OperaColorsPage.cpp, PropPageTextStyles.cpp, PublicHubsFrm.cpp, WinUtil.h (╤З╨╡╤А╨╡╨┤╨╛╨▓╨░╨╜╨╕╨╡ ╤Б╤В╤А╨╛╨║), FlyFeatures/GradientLabel.h.
- ╨Т╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨║╨╛╨┤╨╕╤А╨╛╨▓╨║╨╕ CP1251 ╨╖╨░╤В╤А╨╛╨╜╤Г╤В╤Л╤Е ╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╛╨▓ ╨┐╨╛╤Б╨╗╨╡ ╨┐╨╡╤А╨╡╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╨╕╤П ╨▓ UTF-8 (U+FFFD) тАФ ╨┐╤А╨░╨▓╨║╨╕ ╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╤Л ╨╖╨░╨╜╨╛╨▓╨╛ ╨▒╨░╨╣╤В-╨▒╨╡╨╖╨╛╨┐╨░╤Б╨╜╨╛ (╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜╨╛: U+FFFD ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В, diff ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В ╤В╨╛╨╗╤М╨║╨╛ ╤Ж╨╡╨╗╨╡╨▓╤Л╨╡ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╤П).
- ╨Э╨╡ ╤В╤А╨╛╨╜╤Г╤В╨╛ (╨╛╤Б╨╛╨╖╨╜╨░╨╜╨╜╨╛): ╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╨╣ `windows/grid/PropertyGrid.h`, `wtl_flylinkdc.h` (╤Ж╨╕╨║╨╗╨╕╤З╨╡╤Б╨║╨░╤П ╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛╤Б╤В╤М ╨╛╤В WinUtil.h), `DimEdit.h`, `client/FlyFeatures/MiniHTML.h|PropertyList.h|PropertyView.h` тАФ ╨╛╤В╨╝╨╡╤З╨╡╨╜╨╛ ╨▓ TODO.md (Phase D).

### ╨Т╨╡╨╜╨┤╨╛╤А╨╜╤Л╨╡ ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║╨╕ ╨▓ vendor-update.py: jsoncpp, maxminddb, boost, leveldb, ZenLib
- ╨б╨║╤А╨╕╨┐╤В ╤А╨░╤Б╤И╨╕╤А╨╡╨╜: ╤А╨╡╨╢╨╕╨╝ `select` (╨╝╨░╨┐╨┐╨╕╨╜╨│ `{src_rel: out_rel}`; ╤Б╤В╤А╨╛╨║╨░ тАФ ╤Д╨░╨╣╨╗-╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В ╤Б╨╛ ╤Б╨┐╨╕╤Б╨║╨╛╨╝ ╨┐╤Г╤В╨╡╨╣), ╤Е╤Г╨║ `generate` (╨║╨╛╨╝╨░╨╜╨┤╨░ ╨┐╨╡╤А╨╡╨┤ ╨▓╤Л╨▒╨╛╤А╨╛╨╝ ╤Д╨░╨╣╨╗╨╛╨▓ тАФ ╨┤╨╗╤П jsoncpp `amalgamate.py`), ╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╨╕╨╡ ╨▓╨╜╨╡╤И╨╜╨╕╤Е unified-╨┤╨╕╤Д╨╛╨▓ `diff` ╤З╨╡╤А╨╡╨╖ `git apply -p1` ╨▓ ╨┤╨╕╤А╨╡╨║╤В╨╛╤А╨╕╨╕ ╨▓╨╡╨╜╨┤╨╛╤А╨░. ╨Ы╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╨┐╨░╤В╤З╨╕ ╤Е╤А╨░╨╜╤П╤В╤Б╤П ╨▓ `vendor-patches/*.patch`, ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В╤Л ╨┐╨╛╨┤╨╝╨╜╨╛╨╢╨╡╤Б╤В╨▓ тАФ ╨▓ `vendor-manifests/*.txt`; ╨║╨╛╨╝╨╝╨╕╤В ╨▓╨║╨╗╤О╤З╨░╨╡╤В ╨║╨░╤В╨░╨╗╨╛╨│ ╨▓╨╡╨╜╨┤╨╛╤А╨░ + ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В + ╨┐╨░╤В╤З╨╕.
- ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л 5 ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓; ╨┤╨╗╤П ╨║╨░╨╢╨┤╨╛╨│╨╛ 1:1 ╨▓╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╡╨┤╨╡╨╜╨╕╨╡ ╤В╨╡╨║╤Г╤Й╨╡╨│╨╛ ╨┤╨╡╤А╨╡╨▓╨░ (upstream тЖТ select/generate тЖТ ╨┐╨░╤В╤З == ╨║╨░╤В╨░╨╗╨╛╨│ ╨▓ ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╨╕, 0 ╤А╨░╤Б╤Е╨╛╨╢╨┤╨╡╨╜╨╕╨╣):
  - jsoncpp тАФ ╨▒╨░╨╖╨░ 1.9.6, ╨░╨╝╨░╨╗╤М╨│╨░╨╝╨░╤Ж╨╕╤П `amalgamate.py`; `select` dist/jsoncpp.cppтЖТjsoncpp.cpp ╨╕ dist/json/json.hтЖТinclude/json/json.h; ╨┐╨░╤В╤З тАФ ╨┐╤А╨░╨▓╨║╨╕ FlylinkDC (╨▓╨╡╤А╤Б╨╕╨╣╨╜╨░╤П ╤Б╤В╤А╨╛╨║╨░ 1.9.7, `JSONCPP_USE_SECURE_MEMORY`, MSC-╤Б╨╛╨▓╨╝╨╡╤Б╤В╨╕╨╝╤Л╨╡ ╤И╨╕╨╝╤Л).
  - maxminddb 1.7.1 тАФ 6 ╤Д╨░╨╣╨╗╨╛╨▓; ╨┐╨░╤В╤З╨╕ (POSIX-╨╖╨░╨│╨╗╤Г╤И╨║╨░ ╨▓ data-pool.c, ╨┐╤А╨░╨│╨╝╤Л MSVC, guard `PACKAGE_VERSION` ╨▓ maxminddb.h); project-╤Д╨░╨╣╨╗╤Л `libmaxminddb.vcxproj*` ╨╛╤Б╤В╨░╤О╤В╤Б╤П ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╝╨╕.
  - boost 1.88 тАФ ╨┐╨╛╨┤╨╝╨╜╨╛╨╢╨╡╤Б╤В╨▓╨╛ `boost/` + `libs/{chrono,filesystem,iostreams,system,thread}` (╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В 6402 ╤Д╨░╨╣╨╗╨╛╨▓); ╨╡╨┤╨╕╨╜╤Б╤В╨▓╨╡╨╜╨╜╤Л╨╣ ╨┐╨░╤В╤З тАФ `boost/asio/impl/config.hpp`: `std::strtoll`тЖТ`strtoll` (+ ╨╝╨░╤А╨║╨╡╤А ╨║╨╛╨╜╤Ж╨░ ╤Б╤В╤А╨╛╨║╨╕ ╨▓ `lightweight_test_report.hpp`).
  - leveldb 1.23 тАФ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В 100 ╤Д╨░╨╣╨╗╨╛╨▓ (upstream ╨╝╨╕╨╜╤Г╤Б ╤В╨╡╤Б╤В╤Л); 55 ╤Д╨░╨╣╨╗╨╛╨▓ ╤Б ╨┐╨░╤В╤З╨░╨╝╨╕ (`LEVELDB_VER "1.23"`, c.cc, env.h, cache.cc, env_windows.cc ╨╕ ╨┤╤А.).
  - ZenLib 0.4.41 тАФ ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В 59 ╤Д╨░╨╣╨╗╨╛╨▓ (`Source/` + History/License/ReadMe); 30 ╤Д╨░╨╣╨╗╨╛╨▓ ╤Б ╨┐╨░╤В╤З╨░╨╝╨╕ (Conf.h `ZENLIB_NO_WIN9X_SUPPORT`, CriticalSection/Thread, PreComp.h, Ztring*, int128* ╨╕ ╨┤╤А.).
- ╨Я╤А╨╛╨▓╨╡╤А╨║╨╕: ╨▓╤Б╨╡ 5 ╤Б╤Г╤Е╨╕╤Е ╨┐╤А╨╛╨│╨╛╨╜╨╛╨▓ `--dry-run` ╨┐╤А╨╛╤Е╨╛╨┤╤П╤В end-to-end (download тЖТ extract тЖТ generate тЖТ select тЖТ plan); `git apply` ╨▓╨╜╤Г╤В╤А╨╕ ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╤П ╨▓╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╛╨┤╨╕╤В ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╤Д╨░╨╣╨╗╤Л ╨▒╨╡╨╖ ╤А╨░╤Б╤Е╨╛╨╢╨┤╨╡╨╜╨╕╨╣ (╤В╨╡╤Б╤В ╨▓ gitignored `build-win`). ╨Т╨╡╤А╤Б╨╕╨╕ ╨┤╨╡╤В╨╡╨║╤В╨╕╤А╤Г╤О╤В╤Б╤П ╨┐╨╛ ╨╝╨░╤А╨║╨╡╤А╨░╨╝ (json.h/configure-╨▓╨╡╤А╤Б╨╕╨╕/maxminddb.h/db.h/History.txt/version.hpp).
- ╨в╨╡╤Е╨╜╨╕╨║╨░: ╨┤╨╗╤П ╨▒╨╛╨╗╤М╤И╨╕╤Е ╤З╨╡╤А╨╡╨┤╤Г╤О╤Й╨╕╤Е╤Б╤П ╨┐╨░╤В╤З╨╡╨╣ ╨▓╤Л╨▒╤А╨░╨╜ ╨▓╨╜╨╡╤И╨╜╨╕╨╣ unified-diff ╨▓╨╝╨╡╤Б╤В╨╛ ╤Б╤В╤А╨╛╨║╨╛╨▓╤Л╤Е ╨╖╨░╨╝╨╡╨╜; ╨┐╨╛╨┤╨┤╨╡╤А╨╢╨░╨╜ ╤Б╨╕╨╜╤В╨╡╨╖ ┬л╨▒╨╡╨╖ ╨┐╨╡╤А╨╡╨▓╨╛╨┤╨░ ╤Б╤В╤А╨╛╨║╨╕ ╨▓ ╨║╨╛╨╜╤Ж╨╡ ╤Д╨░╨╣╨╗╨░┬╗ (`\ No newline at end of file`). ╨У╨╡╨╜╨╡╤А╨░╤В╨╛╤А `vendor-patches` ╨╕ ╨▓╨░╨╗╨╕╨┤╨░╤В╨╛╤А ╨┐╤А╨╕╨╝╨╡╨╜╤П╨╗╨╕╤Б╤М ╤А╨░╨╖╨╛╨▓╨╛ ╨╕╨╖ ╨▓╤А╨╡╨╝╨╡╨╜╨╜╤Л╤Е ╤Б╨║╤А╨╕╨┐╤В╨╛╨▓.

### ╨Т╨╡╨╜╨┤╨╛╤А╤Л ╨▓╤В╨╛╤А╨╛╨╣ ╨▓╨╛╨╗╨╜╤Л: bzip2, lua, unrar, libtorrent, mediainfolib, snappy, selene, openssl
- ╨Ч╨░╨▓╨╡╨┤╨╡╨╜╤Л 8 ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓; ╨┤╨╗╤П ╨║╨░╨╢╨┤╨╛╨│╨╛ ╨┐╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╛ 1:1-╨▓╨╛╤Б╨┐╤А╨╛╨╕╨╖╨▓╨╡╨┤╨╡╨╜╨╕╨╡ ╤В╨╡╨║╤Г╤Й╨╡╨│╨╛ ╨┤╨╡╤А╨╡╨▓╨░ (╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В + ╨┐╨░╤В╤З == ╨║╨░╤В╨░╨╗╨╛╨│ ╨▓ ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╨╕, 0 ╤А╨░╤Б╤Е╨╛╨╢╨┤╨╡╨╜╨╕╨╣):
  - bzip2 1.0.8 (`files`, 10 ╤Д╨░╨╣╨╗╨╛╨▓; ╨┐╨░╤В╤З тАФ `nGroups`тЖТ`BZ_N_GROUPS` ╨▓ `compress.c`).
  - lua 5.4.6 (`select` ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В 59 ╤Д╨░╨╣╨╗╨╛╨▓ ╤Б ╨╝╨░╨┐╨┐╨╕╨╜╨│╨╛╨╝ `src/x x`; ╨┐╨░╤В╤З тАФ ╤Б╤В╤А╨╛╨║╨╕ `$Id:` ╨▓ `lzio.c/.h`).
  - unrar 6.23 (`select` 153 ╤Д╨░╨╣╨╗╨░; ╨┐╨░╤В╤З тАФ 1 ╤Д╨░╨╣╨╗ `UnRAR.vcxproj`). ╨Т╤Л╤П╤Б╨╜╨╡╨╜╨╛, ╤З╤В╨╛ ╤Г rarlab ╨╜╨╡╤Б╨╛╨│╨╗╨░╤Б╨╛╨▓╨░╨╜╨╜╨░╤П ╨╜╤Г╨╝╨╡╤А╨░╤Ж╨╕╤П: `unrarsrc-6.2.9.tar.gz` = 6.23 beta 1, `unrarsrc-6.2.10.tar.gz` = 6.23 final; `unrarsrc-6.2.3.tar.gz` = 6.20 beta 3 (╨╜╨╡ 6.23). ╨Т╨╡╤А╨╜╤Л╨╣ ╨░╤А╤Е╨╕╨▓ тАФ `unrarsrc-6.2.10`, ╨▓╨╡╤А╤Б╨╕╤П ╤З╨╕╤В╨░╨╡╤В╤Б╤П ╨║╨░╨║ 6.23 ╤Б ╨╛╨▒╨╡╨╕╤Е ╤Б╤В╨╛╤А╨╛╨╜.
  - libtorrent 2.1.0 (release tarball, `select` 485 ╤Д╨░╨╣╨╗╨╛╨▓; ╨┐╨░╤В╤З 196 ╤Д╨░╨╣╨╗╨╛╨▓ тАФ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╨┐╤А╨░╨▓╨║╨╕ ╨┐╤А╨╛╨╡╨║╤В╨░, ╨╜╨░╨┐╤А. `config.hpp`, `aux_/export.hpp`, `alert.hpp`).
  - mediainfolib 25.07 (github tag v25.07, `select` 509 ╤Д╨░╨╣╨╗╨╛╨▓; ╨┐╨░╤В╤З 116 ╤Д╨░╨╣╨╗╨╛╨▓; ╨▓╨╡╤А╤Б╨╕╤П ╨╕╨╖ `History_DLL.txt`).
  - snappy 1.2.0 (`select` 9 ╤Д╨░╨╣╨╗╨╛╨▓; ╨┐╨░╤В╤З тАФ `snappy.cc`/`snappy.h`; ╨▓╨╡╤А╤Б╨╕╨╕ ╨╗╨╛╨║╨░╨╗╤М╨╜╨╛ ╨╜╨╡╤В, ╨║╨╗╤О╤З `version` ╨╛╨┐╤Г╤Й╨╡╨╜). ╨Ы╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ `snappy-stubs-public-{linux,windows}.h` ╨╕ `Snappy.vcxproj` тАФ ╨┐╤А╨╛╨╡╨║╤В╨╜╤Л╨╡ ╤Д╨░╨╣╨╗╤Л (╨▓ upstream ╨╕╤Е ╨╜╨╡╤В). ╨Т ╤А╨╡╤И╨╡╨╜╨╕╨╕ `.sln` snappy ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В (╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╤В╤Б╤П leveldb ╤В╨╛╨╗╤М╨║╨╛ ╨║╨░╨║ include-╨┐╤Г╤В╤М).
  - selene: ╨┐╨╕╨╜ ╨╜╨░ ╨║╨╛╨╝╨╝╨╕╤В `5cde61c6c021ad1c096b03cc41c7d2325e0ca4eb` (master; 24/26 ╤Д╨░╨╣╨╗╨╛╨▓ ╤Б╨╛╨▓╨┐╨░╨┤╨░╤О╤В, ╨┐╨░╤В╤З тАФ `Selector.h`/`State.h`).
  - openssl: ╨┐╨╕╨╜ ╨╜╨░ ╨║╨╛╨╝╨╝╨╕╤В ╨▓╨╡╤В╨║╨╕ OpenSSL_1_1_1-stable `b372b1f76450acdfed1e2301a39810146e28b02c` (`1.1.1x-dev`); `select` 953 ╤Д╨░╨╣╨╗╨░, ╨┐╨░╤В╤З 15 ╤Д╨░╨╣╨╗╨╛╨▓ (╨▓ ╨╛╤Б╨╜╨╛╨▓╨╜╨╛╨╝ ╤Б╨│╨╡╨╜╨╡╤А╨╕╤А╨╛╨▓╨░╨╜╨╜╨░╤П ╤В╨░╨▒╨╗╨╕╤Ж╨░ `crypto/ec/ecp_nistz256_table.c` ~1.25 ╨Ь╨С). ╨Я╤А╨╛╨╡╨║╤В╨╜╤Л╨╡/╤Б╨│╨╡╨╜╨╡╤А╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╨╡ ╤Д╨░╨╣╨╗╤Л тАФ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╡-only: `build-for-windows/`, `vc17/`, `openssl_2022.vcxproj*`, `openssl_pch.*`, `crypto/buildinf.h`, `include/crypto/{bn_conf,dso_conf}.h`, `include/openssl/opensslconf.h`.
- ╨Ф╨╛╤А╨░╨▒╨╛╤В╨║╨╕ `vendor-update.py`:
  - ╨╝╨░╨╜╨╕╤Д╨╡╤Б╤В `select` ╤В╨╡╨┐╨╡╤А╤М ╨┤╨╛╨┐╤Г╤Б╨║╨░╨╡╤В ╨╛╨┐╤Ж╨╕╨╛╨╜╨░╨╗╤М╨╜╤Л╨╣ ╨▓╤В╨╛╤А╨╛╨╣ ╤Б╤В╨╛╨╗╨▒╨╡╤Ж `src_rel out_rel` (╨╛╨▒╤А╨░╤В╨╜╨╛ ╤Б╨╛╨▓╨╝╨╡╤Б╤В╨╕╨╝╨╛ ╤Б╨╛ ╤Б╨┐╨╕╤Б╨║╨╛╨╝ ╨╛╨┤╨╕╨╜╨╛╤З╨╜╤Л╤Е ╨┐╤Г╤В╨╡╨╣);
  - `read_version` ╤Б╨╛╨╡╨┤╨╕╨╜╤П╨╡╤В ╨╜╨╡╤Б╨║╨╛╨╗╤М╨║╨╛ ╨│╤А╤Г╨┐╨┐ ╤З╨╡╤А╨╡╨╖ `.` ╨╕ ╨╕╤Й╨╡╤В ╤Б `re.MULTILINE` (╨┤╨╗╤П ╨╝╨░╤А╨║╨╡╤А╨╛╨▓ ╨▓╨╕╨┤╨░ `^Version тАж`);
  - ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╨░ ╨╜╨╛╤А╨╝╨░╨╗╨╕╨╖╨░╤Ж╨╕╤П ╨┐╨╡╤А╨╡╨▓╨╛╨┤╨╛╨▓ ╤Б╤В╤А╨╛╨║ `normalize_lf()` ╤А╨░╤Б╨┐╨░╨║╨╛╨▓╨░╨╜╨╜╨╛╨│╨╛ upstream ╨▓ LF ╨┐╨╡╤А╨╡╨┤ ╨┐╨╗╨░╨╜╨╕╤А╨╛╨▓╨░╨╜╨╕╨╡╨╝/╨║╨╛╨┐╨╕╤А╨╛╨▓╨░╨╜╨╕╨╡╨╝ тАФ ╨╛╨▒╤П╨╖╨░╤В╨╡╨╗╤М╨╜╨╛ ╨┤╨╗╤П ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓, ╨╛╤В╨┤╨░╤О╤Й╨╕╤Е CRLF (mediainfolib, snappy), ╨╕╨╜╨░╤З╨╡ ╨▒╨░╨╣╤В╨╛╨▓╨╛╨╡ ╤Б╤А╨░╨▓╨╜╨╡╨╜╨╕╨╡ ╨╕ `git apply` ╨╗╨╛╨╝╨░╤О╤В╤Б╤П;
  - ╨╕╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜╨╛ ╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╨╕╨╡ `diff`-╨┐╨░╤В╤З╨╡╨╣: `git apply` ╨╖╨░╨┐╤Г╤Б╨║╨░╨╡╤В╤Б╤П ╨╕╨╖ ╨║╨╛╤А╨╜╤П ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╤П ╤Б `--directory=<local>` ╨╕ `-c core.autocrlf=false -c core.safecrlf=false` (╨╕╨╜╨░╤З╨╡ ╨╕╨╖ ╨┐╨╛╨┤╨║╨░╤В╨░╨╗╨╛╨│╨░ ╤А╨╡╨╖╨╛╨╗╨▓╨╕╨╜╨│ ╨┐╤Г╤В╨╡╨╣ ╨╕╨┤╤С╤В ╨╛╤В ╨║╨╛╤А╨╜╤П ╤А╨╡╨┐╨╛ ╨╕ ╤Д╨░╨╣╨╗╤Л ╨╝╨╛╨╗╤З╨░ ╨┐╤А╨╛╨┐╤Г╤Б╨║╨░╤О╤В╤Б╤П).
- ╨Я╤А╨░╨▓╨╕╨╗╨╛ ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╤П: ╨║╨░╨╜╨╛╨╜╨╕╤З╨╡╤Б╨║╨╕╨╣ EOL тАФ LF (╨╕╨╜╨┤╨╡╨║╤Б ╤Е╤А╨░╨╜╨╕╤В LF ╨┐╤А╨╕ `core.autocrlf=true`). ╨Я╨╛╤Н╤В╨╛╨╝╤Г ╨╕ upstream, ╨╕ ╨╗╨╛╨║╨░╨╗╤М╨╜╨╛╨╡ ╨┤╨╡╤А╨╡╨▓╨╛ ╨┐╤А╨╕ ╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╨╕ ╨┐╨░╤В╤З╨░ ╨╜╨╛╤А╨╝╨░╨╗╨╕╨╖╤Г╤О╤В╤Б╤П ╨▓ LF (CRLFтЖТLF ╨┤╨╗╤П ╨╜╨╡-╨▒╨╕╨╜╨░╤А╨╜╤Л╤Е ╤Д╨░╨╣╨╗╨╛╨▓); ╨╕╤В╨╛╨│╨╛╨▓╤Л╨╣ ╨┐╨░╤В╤З EOL-╨╜╨╡╨╖╨░╨▓╨╕╤Б╨╕╨╝ ╨╕ ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В LF-╨║╨╛╨╜╤В╨╡╨║╤Б╤В. ╨н╤В╨╛ ╤Г╤Б╤В╤А╨░╨╜╨╕╨╗╨╛ ╨╛╤В╨║╨░╨╖ `git apply` ╨╜╨░ CRLF-╨┐╨░╤В╤З╨╡ unrar.
- ╨Ш╨┤╨╡╨╝╨┐╨╛╤В╨╡╨╜╤В╨╜╨╛╤Б╤В╤М ╨┐╤А╨╛╨▓╨╡╤А╤П╨╡╤В╤Б╤П ╤В╨░╨║: `vendor-update.py --vendor N --no-commit` тЖТ `git add N` тЖТ `git diff --cached --stat` (╨┤╨╛╨╗╨╢╨╡╨╜ ╨▒╤Л╤В╤М ╨┐╤Г╤Б╤В) тЖТ `git reset`. ╨Я╤А╨╛╨╝╨╡╨╢╤Г╤В╨╛╤З╨╜╤Л╨╣ `git status` ╨┐╨╛╨║╨░╨╖╤Л╨▓╨░╨╡╤В ╤Д╨░╨╣╨╗╤Л ╨║╨░╨║ Modified/┬лLF will be replaced by CRLF┬╗ тАФ ╤Н╤В╨╛ stat-╨░╤А╤В╨╡╤Д╨░╨║╤В `core.autocrlf=true` (╤А╨░╨▒╨╛╤З╨╕╨╣ LF ╨┐╤А╨╛╤В╨╕╨▓ ╨╛╨╢╨╕╨┤╨░╨╡╨╝╨╛╨│╨╛ CRLF), ╨▒╨╗╨╛╨▒ ╨┐╤А╨╕ `git add` ╨╜╨╡ ╨╝╨╡╨╜╤П╨╡╤В╤Б╤П. ╨Т╤Б╨╡ 8 ╨╜╨╛╨▓╤Л╤Е ╨▓╨╡╨╜╨┤╨╛╤А╨╛╨▓ (bzip2, lua, unrar, libtorrent, mediainfolib, snappy, selene, openssl) ╨┐╤А╨╛╤Е╨╛╨┤╤П╤В ╨┐╤А╨╛╨▓╨╡╤А╨║╤Г: 0 ╨╛╤И╨╕╨▒╨╛╨║ ╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╨╕╤П ╨┐╨░╤В╤З╨░, ╨┐╤Г╤Б╤В╨╛╨╣ `diff --cached`.

### ╨д╨░╨╖╨░ D: ╨▓╤Л╨╜╨╛╤Б ╨┐╨░╨╗╨╕╤В╤А╤Л ╨▓ ╨╗╤С╨│╨║╨╕╨╣ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨╛╨║ ╨╕ ╨║╨╛╨╜╤Б╨╕╤Б╤В╨╡╨╜╤В╨╜╨╛╤Б╤В╤М
- ╨Э╨╛╨▓╤Л╨╣ `windows/Colors.h`: header-only `FlyTheme::getSysColor(int)` тАФ ╤В╨╛╤З╨╜╨░╤П ╨║╨╛╨┐╨╕╤П ╨┐╨░╨╗╨╕╤В╤А╤Л ╨╕╨╖ `Colors::getSysColor`; ╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛╤Б╤В╨╕ ╤В╨╛╨╗╤М╨║╨╛ `<windows.h>` ╨╕ `../client/SettingsManager.h`. ╨а╨░╨╖╤А╤Л╨▓╨░╨╡╤В ╤Ж╨╕╨║╨╗╨╕╤З╨╡╤Б╨║╨╕╨╣ include WinUtil.h тЖР wtl_flylinkdc.h (╤Н╤В╨╛ ╨┐╨╛╨╖╨▓╨╛╨╗╤П╨╗╨╛ ╨▓╨╖╤П╤В╤М ╤Б╤Б╤Л╨╗╨║╨╕ `COLOR_HOTLIGHT` ╨▓ `wtl_flylinkdc.h:241`).
- `Colors::getSysColor` (windows/WinUtil.cpp, ~3858) ╤В╨╡╨┐╨╡╤А╤М ╨┤╨╡╨╗╨╡╨│╨╕╤А╤Г╨╡╤В ╨▓ `FlyTheme::getSysColor`.
- `wtl_flylinkdc.h:241`: ╤Ж╨▓╨╡╤В ╤Б╤Б╤Л╨╗╨╛╨║ `CFlyHyperLink` тАФ `FlyTheme::getSysColor(COLOR_HOTLIGHT)`.
- `flattabctrl.h:720`: ╤Д╨╛╨╜ ╨▓╨║╨╗╨░╨┤╨╛╨║ тАФ `CreateSolidBrush(Colors::getSysColor(COLOR_BTNFACE))` + ╨▓╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨┐╤А╨╡╨╢╨╜╨╡╨╣ ╨║╨╕╤Б╤В╨╕ ╨╕ `DeleteObject`.
- `PopupDlg.h`: ╤Д╨╛╨╜ PopupWnd ╨╕╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜ ╤Б `COLOR_INFOTEXT` (╤З╤С╤А╨╜╤Л╨╣, ╨▒╨░╨│) ╨╜╨░ `COLOR_INFOBK`, ╨║╨╕╤Б╤В╤М ╤В╨╡╨┐╨╡╤А╤М ╤Б╨╛╨▒╤Б╤В╨▓╨╡╨╜╨╜╨░╤П (`m_bgBrush`) ╤Б ╨╛╤З╨╕╤Б╤В╨║╨╛╨╣ ╨▓ ╨┤╨╡╤Б╤В╤А╤Г╨║╤В╨╛╤А╨╡.
- ╨Ь╤С╤А╤В╨▓╤Л╨╣/╨╜╨╡╨░╨║╤В╨╕╨▓╨╜╤Л╨╣ ╨║╨╛╨┤ ╨┐╤А╨╕╨▓╨╡╨┤╤С╨╜ ╨║ ╨┐╨░╨╗╨╕╤В╤А╨╡ ╨▒╨╡╨╖ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╤П ╨┐╨╛╨▓╨╡╨┤╨╡╨╜╨╕╤П: `windows/DimEdit.h` (╨▓╨▓╨╡╨┤╤С╨╜ include WinUtil.h; `COLOR_WINDOW`/`COLOR_BTNTEXT`), ╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╨╣ `windows/grid/PropertyGrid.h` (`OnSettingChange`), `client/FlyFeatures/MiniHTML.h`, `PropertyList.h` (╨▓ ╤В.╤З. 2 `GetSysColorBrush(COLOR_3DFACE)` тЖТ `CBrush(...)`), `PropertyView.h`.
- ╨Ъ╨╛╨┤╨╕╤А╨╛╨▓╨║╨╕ ╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜╤Л ╨▒╨░╨╣╤В-╨▒╨╡╨╖╨╛╨┐╨░╤Б╨╜╨╛: CP1251 ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╨░ (non-ASCII ╨▒╨░╨╣╤В╤Л ╨▒╨╡╨╖ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╨╣), U+FFFD ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В, diff ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В ╤В╨╛╨╗╤М╨║╨╛ ╤Ж╨╡╨╗╨╡╨▓╤Л╨╡ ╨┐╤А╨░╨▓╨║╨╕.

## 2026-10-11
### Release|x64 warning-clean ╨╕ ╤З╨╕╤Б╤В╤Л╨╡ ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╜╤Л╨╡ ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╤П Debug|x64
- Release|x64 ╤Б╨╛╨▒╤А╨░╨╜ ╨╕╨╖ FlylinkDC_2022.sln: 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ ╨╕ 0 ╨╛╤И╨╕╨▒╨╛╨║; EXE ╨╗╨╕╨╜╨║╤Г╨╡╤В╤Б╤П, ╨╕╨╝╨┐╨╛╤А╤В╤Л тАФ ╤В╨╛╨╗╤М╨║╨╛ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╨╡ DLL Windows, ╨┤╤Л╨╝╨╛╨▓╨╛╨╣ ╨╖╨░╨┐╤Г╤Б╨║ ╨╜╨░ Windows ╤Г╤Б╨┐╨╡╤И╨╡╨╜ (╨┐╤А╨╛╤Ж╨╡╤Б╤Б ╤Б╤В╨░╤А╤В╤Г╨╡╤В ╨╕ ╨╢╨╕╨▓╤С╤В).
- Debug|x64: 0 ╨╛╤И╨╕╨▒╨╛╨║ ╨╕ 0 ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╜╤Л╤Е ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ C4xxx; ╨╛╤Б╤В╨░╤О╤В╤Б╤П ╤В╨╛╨╗╤М╨║╨╛ ╤Б╤В╨░╤В╨╕╤З╨╡╤Б╨║╨╕╨╡ C26xx ╨╛╤В /analyze, ╨▓╨║╨╗╤О╤З╤С╨╜╨╜╨╛╨│╨╛ ╨▓ ╤Н╤В╨╛╨╣ ╨║╨╛╨╜╤Д╨╕╨│╤Г╤А╨░╤Ж╨╕╨╕.
- C2668 (╨┤╨▓╤Г╤Б╨╝╤Л╤Б╨╗╨╡╨╜╨╜╤Л╨╣ SET_SETTING): windows/TreePropertySheet.cpp тАФ `(INT_PTR)next` ╨╖╨░╨╝╨╡╨╜╨╡╨╜╨╛ ╨╜╨░ `static_cast<int>(ctrlTree.GetItemData(next))`.
- C4456 (╨╖╨░╤В╨╡╨╜╨╡╨╜╨╕╨╡ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╤Е): ╨╝╨░╨║╤А╨╛╤Б╤Л ╨▒╨╗╨╛╨║╨╕╤А╨╛╨▓╨╛╨║ ╨▒╨╛╨╗╤М╤И╨╡ ╨╜╨╡ ╨┐╨╡╤А╨╡╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╤О╤В ╨╕╨╝╤П `l_lock`; ╨▓ client/CFlyThread.h ╨╕ client/webrtc/.../rw_lock_wrapper.h ╨╕╨╝╨╡╨╜╨░ guard-╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╤Е ╤Б╨┤╨╡╨╗╨░╨╜╤Л ╤Г╨╜╨╕╨║╨░╨╗╤М╨╜╤Л╨╝╨╕ ╤З╨╡╤А╨╡╨╖ `FLY_LOCK_CONCAT(l_fly_lock_, __COUNTER__)`.
- C4189 (╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╨╡, ╨╜╤Г╨╢╨╜╤Л╨╡ ╤В╨╛╨╗╤М╨║╨╛ ╨▓ dcassert, ╨┐╤Г╤Б╤В╨╛╨╝ ╨▓ Release): ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л `(void)var;` (File.cpp, HashManager.cpp, GdiOle/GDIImage.cpp, flyServer.cpp, DownloadManager.cpp, ConnectionManager.cpp, windows/flattabctrl.h, EmoticonsDlg.cpp, HIconWrapper.cpp, FlyFeatures/AutoUpdate.cpp, Util.cpp ╨╕ ╨┤╤А.).
- ╨Я╤А╨╛╤З╨╕╨╡ ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╜╤Л╨╡ ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╤П: C4267/C4244/C4311 тАФ ╤П╨▓╨╜╤Л╨╡ static_cast (AdcHub.cpp, HashManager.cpp, SearchManager.cpp, ShareManager.cpp, AutoUpdate.cpp); C4702 тАФ ╤Г╨┤╨░╨╗╤С╨╜ ╨╜╨╡╨┤╨╛╤Б╤В╨╕╨╢╨╕╨╝╤Л╨╣ ╨║╨╛╨┤ (flyServer.cpp:getMediaInfo, flattabctrl.h::setColor); C4541 (typeid ╨┐╤А╨╕ /GR-) тАФ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╣ `#pragma warning(disable:4541)` ╨▓ DownloadManager.cpp (Debug-only ╨┤╨╕╨░╨│╨╜╨╛╤Б╤В╨╕╨║╨░); C4066 тАФ ╨╝╨╜╨╛╨│╨╛╨▒╨░╨╣╤В╨╛╨▓╤Л╨╣ ╤Б╨╕╨╝╨▓╨╛╨╗╤М╨╜╤Л╨╣ ╨╗╨╕╤В╨╡╤А╨░╨╗ ╨╖╨░╨╝╨╡╨╜╤С╨╜ ╨╜╨░ `_T('\x2026')`.
- ╨Т╨╛╤Б╤Б╤В╨░╨╜╨╛╨▓╨╗╨╡╨╜╨░ ╨║╨╛╨┤╨╕╤А╨╛╨▓╨║╨░ ╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╛╨▓ CP1251: ╤А╤Г╤Б╤Б╨║╨╕╨╡ ╨║╨╛╨╝╨╝╨╡╨╜╤В╨░╤А╨╕╨╕, ╤А╨░╨╜╨╡╨╡ ╨╖╨░╨╝╨╡╨╜╤С╨╜╨╜╤Л╨╡ ╨╜╨░ U+FFFD ╨┐╤А╨╕ ╨┐╨╡╤А╨╡╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╨╕╨╕ ╨▓ UTF-8, ╨▓╨╛╨╖╨▓╤А╨░╤Й╨╡╨╜╤Л ╨╕╨╖ git, ╤Ж╨╡╨╗╨╡╨▓╤Л╨╡ ╨┐╤А╨░╨▓╨║╨╕ ╨┐╤А╨╕╨╝╨╡╨╜╨╡╨╜╤Л ╨╖╨░╨╜╨╛╨▓╨╛ ╨▒╨╡╨╖ ╨┐╨╛╤В╨╡╤А╨╕ ╨║╨╛╨┤╨╕╤А╨╛╨▓╨║╨╕ (╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜╨╛: U+FFFD ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В, diff ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В ╤В╨╛╨╗╤М╨║╨╛ ╤Ж╨╡╨╗╨╡╨▓╤Л╨╡ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╤П).
### ╨Ю╨│╤А╨░╨╜╨╕╤З╨╡╨╜╨╕╨╡ ╨░╨╜╨░╨╗╨╕╨╖╨░ ╤Б╨╛╨▒╤Б╤В╨▓╨╡╨╜╨╜╤Л╨╝ ╨║╨╛╨┤╨╛╨╝ ╨╕ ╨╕╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜╨╕╨╡ ╤А╨╡╨░╨╗╤М╨╜╤Л╤Е ╨┤╨╡╤Д╨╡╨║╤В╨╛╨▓ /analyze
- ╨б╤В╨░╤В╨╕╤З╨╡╤Б╨║╨╕╨╣ ╨░╨╜╨░╨╗╨╕╨╖ (Debug|x64) ╨╛╨│╤А╨░╨╜╨╕╤З╨╡╨╜ ╤Б╨╛╨▒╤Б╤В╨▓╨╡╨╜╨╜╤Л╨╝ ╨║╨╛╨┤╨╛╨╝: ╨▓ flylinkdc_2022.vcxproj ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л `ExternalIncludePath` (wtl, boost, openssl, zlib-ng, bzip2, leveldb, libtorrent, selene, lua, jsoncpp, mediainfolib, zenlib, maxminddb, client/webrtc) ╨╕ `<DisableAnalyzeExternal>true</DisableAnalyzeExternal>` (Debug|x64); ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╤П ╨░╨╜╨░╨╗╨╕╨╖╨░╤В╨╛╤А╨░ ╨┐╨╛ ╤Б╤В╨╛╤А╨╛╨╜╨╜╨╕╨╝ ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨░╨╝ (╨▒╤Л╨╗╨╛ ~13500: wtl/boost/openssl/libtorrent/leveldb/jsoncpp) ╨┐╨╛╨╗╨╜╨╛╤Б╤В╤М╤О ╨╕╤Б╤З╨╡╨╖╨╗╨╕, ╨╛╤Б╤В╨░╨╗╨╕╤Б╤М ╤В╨╛╨╗╤М╨║╨╛ ╨┐╨╛ ╤Б╨▓╨╛╨╡╨╝╤Г ╨║╨╛╨┤╤Г.
- C6001 (╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜╨╕╨╡ ╨╜╨╡╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╤Е lLow/lHigh ╨┐╨╛╤Б╨╗╨╡ EM_GETSEL): client/FlyFeatures/PropertyItemEditors.h тАФ ╨╛╨▒╤К╤П╨▓╨╗╨╡╨╜╨╕╨╡ ╨┐╨╡╤А╨╡╨╜╨╡╤Б╨╡╨╜╨╛ ╨┤╨╛ switch ╤Б ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨░╤Ж╨╕╨╡╨╣ `int lLow = 0, lHigh = 0;`.
- C6385 (╤З╤В╨╡╨╜╨╕╨╡ ╨╖╨░ ╨│╤А╨░╨╜╨╕╤Ж╨╡╨╣ `dDepths[5]` ╨▓ CBarShader::BuildModifiers): windows/BarShader.cpp тАФ ╨▓╤Л╤З╨╕╤Б╨╗╨╡╨╜╨╜╤Л╨╣ ╨╕╨╜╨┤╨╡╨║╤Б ╤Г╤А╨╛╨▓╨╜╤П ╨╖╨░╨╢╨░╤В ╨▓ ╨┤╨╕╨░╨┐╨░╨╖╨╛╨╜ [1..5] ╨┐╨╡╤А╨╡╨┤ ╨╛╨▒╤А╨░╤Й╨╡╨╜╨╕╨╡╨╝ ╨║ ╨╝╨░╤Б╤Б╨╕╨▓╤Г.
- C6308/C28182 (╤Г╤В╨╡╤З╨║╨░ ╨╕ ╤А╨░╨╖╤Л╨╝╨╡╨╜╨╛╨▓╨░╨╜╨╕╨╡ ╨┐╤А╨╕ realloc ╨▓ CShellContextMenu::SetPath): windows/ShellContextMenu.cpp тАФ ╤А╨╡╨╖╤Г╨╗╤М╤В╨░╤В realloc ╤Б╨╛╤Е╤А╨░╨╜╤П╨╡╤В╤Б╤П ╨▓╨╛ ╨▓╤А╨╡╨╝╨╡╨╜╨╜╤Г╤О ╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Г╤О ╤Б ╨┐╤А╨╛╨▓╨╡╤А╨║╨╛╨╣, ╤А╨╡╨╖╤Г╨╗╤М╤В╨░╤В calloc ╤В╨░╨║╨╢╨╡ ╨┐╤А╨╛╨▓╨╡╤А╤П╨╡╤В╤Б╤П; ╨┐╤А╨╕ ╨╛╤И╨╕╨▒╨║╨╡ ╨║╨╛╤А╤А╨╡╨║╤В╨╜╨╛ ╨╛╤Б╨▓╨╛╨▒╨╛╨╢╨┤╨░╤О╤В╤Б╤П pidl ╨╕ COM-╨╕╨╜╤В╨╡╤А╤Д╨╡╨╣╤Б╤Л.
- C6387 (LoadLibrary ╨╝╨╛╨│ ╨▓╨╡╤А╨╜╤Г╤В╤М 0, ╨┤╨░╨╗╨╡╨╡ GetProcAddress): windows/ShellContextMenu.cpp, PopupManager.cpp, TreePropertySheet.cpp, WinUtil.cpp (LockWorkStation) тАФ ╨┤╨╡╤Б╨║╤А╨╕╨┐╤В╨╛╤А ╨╝╨╛╨┤╤Г╨╗╤П ╨┐╤А╨╛╨▓╨╡╤А╤П╨╡╤В╤Б╤П ╨┐╨╡╤А╨╡╨┤ GetProcAddress.
- C6387 ╨▓ windows/WinUtil.cpp: ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ ╤А╨╡╨╖╤Г╨╗╤М╤В╨░╤В╨░ GlobalLock ╨┐╨╡╤А╨╡╨┤ wcscpy; ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ fi.hIcon ╨┐╨╡╤А╨╡╨┤ DestroyIcon.
- C6053 (╨▓╨╛╨╖╨╝╨╛╨╢╨╜╨╛╨╡ ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╨╕╨╡ ╨╜╤Г╨╗╤М-╤В╨╡╤А╨╝╨╕╨╜╨░╤В╨╛╤А╨░ ╤Г _snprintf): client/Util.h (17 ╨╝╨╡╤Б╤В), client/CFlyMediaInfo.h, windows/HTMLColors.h тАФ ╨┐╨╡╤А╨╡╤Е╨╛╨┤ ╨╜╨░ ╨▒╨╡╨╖╨╛╨┐╨░╤Б╨╜╤Л╨╡ `_snprintf_s`/`_snwprintf_s` ╤Б `_TRUNCATE`.
- C6386 ╨▓ windows/PropertiesDlg.cpp: ╤З╨╕╤Б╨╗╨╛ ╤Б╤В╤А╨░╨╜╨╕╤Ж g_numPages ╤Б╨┤╨╡╨╗╨░╨╜╨╛ ╤Г╤Б╨╗╨╛╨▓╨╜╤Л╨╝ (37/38) ╨▓ ╤Б╨╛╨╛╤В╨▓╨╡╤В╤Б╤В╨▓╨╕╨╕ ╤Б `SSA_INCLUDE_FILE_SHARE_PAGE`, ╤Г╤Б╤В╤А╨░╨╜╤С╨╜ ╨╗╨░╤В╨╡╨╜╤В╨╜╤Л╨╣ ╨▓╤Л╤Е╨╛╨┤ ╨╖╨░ ╨│╤А╨░╨╜╨╕╤Ж╤Г ╨╝╨░╤Б╤Б╨╕╨▓╨░; ╨╛╤Б╤В╨░╨▓╤И╨╡╨╡╤Б╤П ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╡ ╨░╨╜╨░╨╗╨╕╨╖╨░╤В╨╛╤А╨░ ╨┤╨╗╤П `pages[n++]` тАФ ╨╗╨╛╨╢╨╜╨╛╨╡ (╤Н╨▓╤А╨╕╤Б╤В╨╕╨║╨░ ╨▓╤Б╨╡╨│╨┤╨░ ╤Б╨╛╨╛╨▒╤Й╨░╨╡╤В ╨┐╨╡╤А╨╡╨┐╨╛╨╗╨╜╨╡╨╜╨╕╨╡ ╨╜╨░ ╨╛╨┤╨╕╨╜ ╤Н╨╗╨╡╨╝╨╡╨╜╤В ╨╜╨╡╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛ ╨╛╤В ╤А╨░╨╖╨╝╨╡╤А╨░ ╨╝╨░╤Б╤Б╨╕╨▓╨░).
- ╨Э╨╡ ╤В╤А╨╛╨╜╤Г╤В╤Л (╨╗╨╛╨╢╨╜╤Л╨╡/╨▒╨╡╨╖╨▓╤А╨╡╨┤╨╜╤Л╨╡): C6387 ╨┤╨╗╤П ATL `_U_STRINGorID` ╨▓ ╤Б╤В╤А╨░╨╜╨╕╤Ж╨░╤Е ╨╜╨░╤Б╤В╤А╨╛╨╡╨║, C6387 ╨▓ ChatCtrl.cpp (GetWindowContext ╤Б NULL) ╨╕ MainFrm.cpp (SetMenu/MapWindowPoints), C6011 ╨▓ QueueFrame.cpp:1394 (`ii` ╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜ ╨▓╤Л╤И╨╡).
- ╨Ъ╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ ╨┐╨╛╤Б╨╗╨╡ ╨┐╤А╨░╨▓╨╛╨║: Release|x64 тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║; Debug|x64 тАФ 0 ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╜╤Л╤Е C4xxx/0 ╨╛╤И╨╕╨▒╨╛╨║; ╨┤╤Л╨╝╨╛╨▓╨╛╨╣ ╨╖╨░╨┐╤Г╤Б╨║ Release EXE ╤Г╤Б╨┐╨╡╤И╨╡╨╜.
### ╨б╨╜╨╕╨╢╨╡╨╜╨╕╨╡ ╤И╤Г╨╝╨░ ╤Б╤В╨░╤В╨╕╤З╨╡╤Б╨║╨╛╨│╨╛ ╨░╨╜╨░╨╗╨╕╨╖╨░╤В╨╛╤А╨░: 14456 тЖТ 98 ╨╕ ╨╕╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜╨╕╨╡ ╤А╨╡╨░╨╗╤М╨╜╤Л╤Е ╨┤╨╡╤Д╨╡╨║╤В╨╛╨▓
- `CodeAnalysisRuleSet` ╨┐╨╡╤А╨╡╨║╨╗╤О╤З╤С╨╜ ╤Б `AllRules.ruleset` (╤Б `<IncludeAll>`) ╨╜╨░ `NativeRecommendedRules.ruleset` ╨┤╨╗╤П Debug|x64 ╨╕ Release|x64 ╨▓ flylinkdc_2022.vcxproj: ╤Г╤Б╤В╤А╨░╨╜╤С╨╜ ╨╝╨░╤Б╤Б╨╛╨▓╤Л╨╣ ╤Б╤В╨╕╨╗╨╡╨▓╨╛╨╣ ╤И╤Г╨╝ (C26440/26446/26447/26481/26482/26485/26493/26496 ╨╕ ╤В.╨┐.), ╨╜╨░╨▒╨╛╤А ╨╛╤Б╤В╨░╨▓╨╗╨╡╨╜╨╜╤Л╤Е ╨┐╤А╨░╨▓╨╕╨╗ тАФ NativeMinimumRules ╨┐╨╗╤О╤Б ╨║╤Г╤А╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╨╡ C6031/C6287/C26437/C26439/C26478/C26819 ╨╕ ╨┤╤А. ╨з╨╕╤Б╨╗╨╛ ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ ╨░╨╜╨░╨╗╨╕╨╖╨░ ╨▓ Debug|x64 ╤Б╨╜╨╕╨╢╨╡╨╜╨╛ ╤Б ~14456 ╨┤╨╛ 273.
- C6287 (╨╕╨╖╨▒╤Л╤В╨╛╤З╨╜╨╛╨╡ ╤Б╤А╨░╨▓╨╜╨╡╨╜╨╕╨╡ ╤Б ╨║╨╛╨╜╤Б╤В╨░╨╜╤В╨╛╨╣): windows/HubFrame.cpp:2398 тАФ ╤Г╨┤╨░╨╗╨╡╨╜╨╛ `|| wp.showCmd == SW_MAXIMIZE` (╨┤╤Г╨▒╨╗╨╕╨║╨░╤В SW_SHOWMAXIMIZED, ╨╛╨▒╨╡ ╨║╨╛╨╜╤Б╤В╨░╨╜╤В╤Л ╤А╨░╨▓╨╜╤Л 3).
- C6031 (╨┐╤А╨╛╨╕╨│╨╜╨╛╤А╨╕╤А╨╛╨▓╨░╨╜╨╜╨╛╨╡ ╨▓╨╛╨╖╨▓╤А╨░╤Й╨░╨╡╨╝╨╛╨╡ ╨╖╨╜╨░╤З╨╡╨╜╨╕╨╡): ╤П╨▓╨╜╤Л╨╡ ╨┐╤А╨╕╨▓╨╡╨┤╨╡╨╜╨╕╤П `(void)` ╨▓ 7 ╨╝╨╡╤Б╤В╨░╤Е тАФ windows/ChatCtrl.cpp (StgCreateDocfileOnILockBytes), FolderTree.cpp (SHGetDesktopFolder, WNetCloseEnum), MainFrm.cpp (CoInitialize), Players.cpp (CoInitialize ├Ч2), ShellContextMenu.cpp (SHGetDesktopFolder).
- C6255 (`_alloca` ╨╝╨╛╨╢╨╡╤В ╨┐╨╡╤А╨╡╨┐╨╛╨╗╨╜╨╕╤В╤М ╤Б╤В╨╡╨║): ╨┐╨╛╨┤╨░╨▓╨╗╨╡╨╜╨╛ ╨╗╨╛╨║╨░╨╗╤М╨╜╨╛ ╨▓ client/FlyFeatures/GradientLabel.h, PropertyItemEditors.h, PropertyItemImpl.h ╤З╨╡╤А╨╡╨╖ `#pragma warning(push/pop)`.
- C26495 (╨╜╨╡╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╨╡ ╤З╨╗╨╡╨╜╤Л): 71 ╨┐╤А╨░╨▓╨║╨░ ╨▓ 15 ╤Д╨░╨╣╨╗╨░╤Е тАФ ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л NSDMI (client/ClientManager.h, CFlylinkDBManager.h, TigerHash.h, FlyFeatures/CProgressDlg.h, PropertyItemEditors.h; windows/UserlistColours.h, EmoticonsDlg.h, BarShader.h, OperaColorsPage.h, PropPageTextStyles.h, ChatCtrl.h, SearchFrm.h, MainFrm.h, FinishedFrameBase.h).
- C26819 (╨╜╨╡╨░╨╜╨╜╨╛╤В╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╨╣ fallthrough): 8 ╨╝╨╡╤Б╤В ╨┐╨╛╨╝╨╡╤З╨╡╨╜╤Л `[[fallthrough]];` (C++17) тАФ client/FlyFeatures/PropertyItemEditors.h, windows/BaseChatFrame.cpp, QueueFrame.cpp ├Ч2, ShellContextMenu.cpp, WaitingUsersFrame.cpp, TransferView.cpp ├Ч2; ╨╜╨░╨╝╨╡╤А╨╡╨╜╨╜╤Л╨╡ ╨┐╨╡╤А╨╡╤Е╨╛╨┤╤Л ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╤Л.
- C26478 (std::move ╨╛╤В ╨║╨╛╨╜╤Б╤В╨░╨╜╤В╤Л) тАФ 86 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ ╤Г╤Б╤В╤А╨░╨╜╨╡╨╜╤Л: ╨▓ ╨╝╨░╨║╤А╨╛╤Б╨╡ GETSET (client/BaseUtil.h) ╤Г╨▒╤А╨░╨╜ no-op `std::move` (ParameterType ╨┤╨╗╤П ╨║╨╗╨░╤Б╤Б╨╛╨▓╤Л╤Е ╤В╨╕╨┐╨╛╨▓ ╨▓╤Б╨╡╨│╨┤╨░ `const T&`), ╨░ ╤В╨░╨║╨╢╨╡ ╨▓ client/CFlySearchItemTTH.h ╨┤╨╗╤П ╨┐╨░╤А╨░╨╝╨╡╤В╤А╨╛╨▓-╨║╨╛╨╜╤Б╤В╨░╨╜╤В. ╨Я╨╛╨▓╨╡╨┤╨╡╨╜╨╕╨╡ ╨╜╨╡ ╨╕╨╖╨╝╨╡╨╜╨╕╨╗╨╛╤Б╤М (╨┐╨╡╤А╨╡╨╝╨╡╤Й╨╡╨╜╨╕╨╡ ╨╛╤В const ╨╕ ╤В╨░╨║ ╨▒╤Л╨╗╨╛ ╨║╨╛╨┐╨╕╤А╨╛╨▓╨░╨╜╨╕╨╡╨╝).
- C26439: ╨┐╨╡╤А╨╡╨╝╨╡╤Й╨░╤О╤Й╨╕╨╣ ╨║╨╛╨╜╤Б╤В╤А╤Г╨║╤В╨╛╤А CFlySearchItemTTH ╨╛╨▒╤К╤П╨▓╨╗╨╡╨╜ `noexcept`.
- ╨Ш╤В╨╛╨│╨╛╨▓╨░╤П ╨║╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░: Debug|x64 тАФ 98 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║ (╨╛╤Б╤В╨░╨╗╨╛╤Б╤М: C6387 42 тАФ ╨╗╨╛╨╢╨╜╤Л╨╡ ╤Б╤А╨░╨▒╨░╤В╤Л╨▓╨░╨╜╨╕╤П ATL/COM; C26495 28 тАФ `m_bMsgHandled` ╨╕╨╖ ╨╝╨░╨║╤А╨╛╤Б╨░ WTL DECLARE_MSG_MAP; C28251 18 тАФ ╨░╨╜╨╜╨╛╤В╨░╤Ж╨╕╨╕ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╤Е/╤Б╤В╨╛╤А╨╛╨╜╨╜╨╕╤Е ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╛╨▓; C28159 7 тАФ ╤Б╨╛╨▓╨╡╤В╤Л ╨╖╨░╨╝╨╡╨╜╨╕╤В╤М GetTickCount/ExitWindowsEx; C6011/C26437/C6386 ╨┐╨╛ 1 тАФ ╨╗╨╛╨╢╨╜╤Л╨╡/╨╜╨░╨╝╨╡╤А╨╡╨╜╨╜╤Л╨╡); Release|x64 тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║.
### ╨Р╨╜╨░╨╗╨╕╨╖ /analyze ╨┤╨╛╨▓╨╡╨┤╤С╨╜ ╨┤╨╛ ╨╜╤Г╨╗╤П: 98 тЖТ 0, ╨╗╨╛╨╢╨╜╤Л╨╡ ╤Б╤А╨░╨▒╨░╤В╤Л╨▓╨░╨╜╨╕╤П ╤Г╤Б╤В╤А╨░╨╜╨╡╨╜╤Л ╨▓ ╨║╨╛╤А╨╜╨╡
- WTL (╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╨╣ ╨║╨╛╨┤ ╨▓ wtl/): `BOOL m_bMsgHandled` ╨▓ ╨╝╨░╨║╤А╨╛╤Б╨╡ `BEGIN_MSG_MAP_EX` (wtl/atlcrack.h) ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜ ╨╖╨╜╨░╤З╨╡╨╜╨╕╨╡╨╝ `FALSE` тАФ ╤Г╤Б╤В╤А╨░╨╜╨╡╨╜╤Л 28 C26495 ┬л╨╜╨╡╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╨╜╤Л╨╣ ╤З╨╗╨╡╨╜┬╗ (╤А╨╡╨░╨╗╤М╨╜╨░╤П ╨╛╤И╨╕╨▒╨║╨░: ╤З╤В╨╡╨╜╨╕╨╡ ╨╜╨╡╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╨╜╨╛╨│╨╛ ╤Д╨╗╨░╨│╨░ ╨▓ ╨╛╨▒╤А╨░╨▒╨╛╤В╤З╨╕╨║╨░╤Е ╤Б╨╛╨╛╨▒╤Й╨╡╨╜╨╕╨╣).
- WTL (╨▓╨╡╨╜╨┤╨╛╤А╨╜╤Л╨╣ ╨║╨╛╨┤ ╨▓ wtl/atldlgs.h): default-╨░╤А╨│╤Г╨╝╨╡╨╜╤В `ATL::_U_STRINGorID title` ╤Г `CPropertyPageImpl` ╨╕ `CPropertyPage` ╨╖╨░╨╝╨╡╨╜╤С╨╜ ╤Б `(LPCTSTR)NULL` ╨╜╨░ `0U` тАФ ╤Г╤Б╤В╤А╨░╨╜╨╡╨╜╤Л 37 ╨╗╨╛╨╢╨╜╤Л╤Е C6387 (╨╜╤Г╨╗╤М ╨▓ ╨░╨┤╤А╨╡╤Б╨╛╨▓╨░╨╜╨╜╤Л╨╣ `_In_z_` LPCTSTR-╨┐╨╡╤А╨╡╨│╤А╤Г╨╖╨╡ `_U_STRINGorID`). ╨Я╨╛╨▓╨╡╨┤╨╡╨╜╨╕╨╡ ╨╕╨┤╨╡╨╜╤В╨╕╤З╨╜╨╛ (`MAKEINTRESOURCE(0) == NULL`), ╤В╨░╨║ ╨╢╨╡ ╨┐╨╛╤Б╤В╤Г╨┐╨░╨╡╤В ╨╕ ╤Б╨░╨╝ WTL ╨▓ ╨░╨╜╨░╨╗╨╛╨│╨╕╤З╨╜╤Л╤Е ╨╝╨╡╤Б╤В╨░╤Е (Menu/ClassName).
- ╨Ю╤Б╤В╨░╨▓╤И╨╕╨╡╤Б╤П ╨╗╨╛╨╢╨╜╤Л╨╡/╨╜╨░╨╝╨╡╤А╨╡╨╜╨╜╤Л╨╡ ╤Б╤А╨░╨▒╨░╤В╤Л╨▓╨░╨╜╨╕╤П ╨┐╨╛╨┤╨░╨▓╨╗╨╡╨╜╤Л ╤В╨╛╤З╨╡╤З╨╜╨╛: C6387 ╨▓ windows/ChatCtrl.cpp (`GetWindowContext` ╤Б NULL тАФ `#pragma warning(push/disable/pop)`) ╨╕ windows/MainFrm.cpp (SetMenu/MapWindowPoints тАФ `suppress`), C6011 ╨▓ windows/QueueFrame.cpp (`ii` ╨┐╤А╨╛╨▓╨╡╤А╨╡╨╜ ╨▓╤Л╤И╨╡), C6386 ╨▓ windows/PropertiesDlg.cpp (╤Н╨▓╤А╨╕╤Б╤В╨╕╨║╨░ ╨┐╨╡╤А╨╡╨┐╨╛╨╗╨╜╨╡╨╜╨╕╤П ╨╜╨░ ╨╛╨┤╨╕╨╜ ╤Н╨╗╨╡╨╝╨╡╨╜╤В), C26437 ╨▓ windows/PropPageTextStyles.cpp (╨╜╨░╨╝╨╡╤А╨╡╨╜╨╜╤Л╨╣ ╤Б╤А╨╡╨╖ `TextStyleSettings` тЖТ `CHARFORMAT2`, ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜ ╤П╨▓╨╜╤Л╨╣ `static_cast` ╨║ ╨▒╨░╨╖╨╛╨▓╨╛╨╣ ╤Б╤Б╤Л╨╗╨║╨╡).
- ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜ ╤Б╨╛╨▒╤Б╤В╨▓╨╡╨╜╨╜╤Л╨╣ ╨╜╨░╨▒╨╛╤А ╨┐╤А╨░╨▓╨╕╨╗ flylinkdc.ruleset: ╨▓╨║╨╗╤О╤З╨░╨╡╤В `NativeRecommendedRules` ╨╕ ╨╛╤В╨║╨╗╤О╤З╨░╨╡╤В ╤В╨╛╨╗╤М╨║╨╛ C28159 (╤Б╨╛╨▓╨╡╤В╤Л ╨╖╨░╨╝╨╡╨╜╨╕╤В╤М GetTickCount/ExitWindowsEx; ╨┤╨╡╨╗╤М╤В╤Л GetTickCount ╨║╨╛╤А╤А╨╡╨║╤В╨╜╤Л ╨┐╨╛ ╨╝╨╛╨┤╤Г╨╗╤О 2^32) ╨╕ C28251 (╨╜╨╡╤Б╨╛╨│╨╗╨░╤Б╨╛╨▓╨░╨╜╨╜╨╛╤Б╤В╤М SAL-╨░╨╜╨╜╨╛╤В╨░╤Ж╨╕╨╣ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╤Е/MIDL-╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╛╨▓ wmp.h/iTunesCOMInterface.h ╨╕ `_tWinMain`). `CodeAnalysisRuleSet` ╨▓ flylinkdc_2022.vcxproj ╨┐╨╡╤А╨╡╨▓╨╡╨┤╤С╨╜ ╨╜╨░ ╨╜╨╡╨│╨╛ (Debug|x64 ╨╕ Release|x64).
- ╨Ш╤В╨╛╨│╨╛╨▓╨░╤П ╨║╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░: Debug|x64 тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║ ╨┐╤А╨╕ ╨▓╨║╨╗╤О╤З╤С╨╜╨╜╨╛╨╝ /analyze (╨┐╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╛ ╨┐╨╛ ╨║╨╛╨╝╨░╨╜╨┤╨╜╨╛╨╣ ╤Б╤В╤А╨╛╨║╨╡ CL: `/analyze:ruleset flylinkdc.ruleset`); Release|x64 тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║. ╨Ъ╨╛╨┤╨╕╤А╨╛╨▓╨║╨╕ ╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╛╨▓ (CP1251 ╨▒╨╡╨╖ BOM, .vcxproj UTF-8 ╤Б BOM) ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╤Л, U+FFFD ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В, `git diff --check` ╤З╨╕╤Б╤В.

### C26494 (ES.20, ┬л╨▓╤Б╨╡╨│╨┤╨░ ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╤Г╨╣ ╨╛╨▒╤К╨╡╨║╤В┬╗): ╨┐╤А╨░╨▓╨╕╨╗╨╛ ╨▓╨║╨╗╤О╤З╨╡╨╜╨╛ ╨╕ ╨┐╤А╨╕╨▓╨╡╨┤╨╡╨╜╨╛ ╨║ ╨╜╤Г╨╗╤О
- ╨Т flylinkdc.ruleset ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╨╛ `<Rule Id="C26494" Action="Warning" />` тАФ ╤В╤А╨╡╨▒╨╛╨▓╨░╨╜╨╕╨╡ ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╤В╤М ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╨╡ ╨┐╤А╨╕ ╨╛╨▒╤К╤П╨▓╨╗╨╡╨╜╨╕╨╕ ╤В╨╡╨┐╨╡╤А╤М ╨┐╤А╨╛╨▓╨╡╤А╤П╨╡╤В╤Б╤П /analyze ╨╜╨░╤А╨░╨▓╨╜╨╡ ╤Б ╨╛╤Б╤В╨░╨╗╤М╨╜╤Л╨╝╨╕ ╤А╨╡╨║╨╛╨╝╨╡╨╜╨┤╨╛╨▓╨░╨╜╨╜╤Л╨╝╨╕ ╨┐╤А╨░╨▓╨╕╨╗╨░╨╝╨╕.
- ╨Ш╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╨╛╨▓╨░╨╜╤Л ╨▓╤Б╨╡ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╨╡ ╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╨╡, ╨╛╤В╨╝╨╡╤З╨╡╨╜╨╜╤Л╨╡ ╨┐╤А╨░╨▓╨╕╨╗╨╛╨╝ (77 ╤Б╤А╨░╨▒╨░╤В╤Л╨▓╨░╨╜╨╕╨╣, 69 ╤Г╨╜╨╕╨║╨░╨╗╤М╨╜╤Л╤Е ╨╝╨╡╤Б╤В) ╨▓ 31 ╤Д╨░╨╣╨╗╨╡: ╤П╨▓╨╜╤Л╨╡ (`= 0`, `= false`, `= nullptr`, `= {}`) ╨╕ ╤Б╨╡╨╝╨░╨╜╤В╨╕╤З╨╡╤Б╨║╨╕╨╡ ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨░╤В╨╛╤А╤Л (enum: `QueueItem::DEFAULT`, `FavoriteManager::NOT_CHANGE`, `SettingsManager::INT_FIRST`; `TASK_TYPE next = TASK_TYPE();`).
- ╨а╨╡╨░╨╗╤М╨╜╤Л╤Е ╨┤╨╡╤Д╨╡╨║╤В╨╛╨▓ ┬л╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜╨╕╨╡ ╨┤╨╛ ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨░╤Ж╨╕╨╕┬╗ ╨╜╨╡ ╨▒╤Л╨╗╨╛ (C6001 = 0), ╨┐╤А╨░╨▓╨╕╨╗╨╛ ╨▒╤Л╨╗╨╛ ╤З╨╕╤Б╤В╨╛ ╤Б╤В╨╕╨╗╨╡╨▓╤Л╨╝ (ES.20); ╨╡╨│╨╛ ╨▓╨║╨╗╤О╤З╨╡╨╜╨╕╨╡ ╨╖╨░╨║╤А╤Л╨▓╨░╨╡╤В ╨║╨╗╨░╤Б╤Б ╨╛╤И╨╕╨▒╨╛╨║ ╨╜╨░ ╨▒╤Г╨┤╤Г╤Й╨╡╨╡.
- ╨Ъ╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░: Debug|x64 тАФ 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║ ╨┐╤А╨╕ ╨░╨║╤В╨╕╨▓╨╜╨╛╨╝ C26494 (╨┐╤А╨╛╨┐╤Г╤Й╨╡╨╜╨╜╤Л╨╡ ╨╝╨╡╤Б╤В╨░, ╨▓╤Л╤П╨▓╨╗╨╡╨╜╨╜╤Л╨╡ ╤Б╨░╨╝╨╛╨╣ ╤Б╨▒╨╛╤А╨║╨╛╨╣, ╨╕╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜╤Л: windows/ChatCtrl.cpp:1425 `OLEINPLACEFRAMEINFO frameInfo`, windows/WinUtil.cpp:3622 `HRESULT hRes`); Release|x64 тАФ 0/0. ╨Ъ╨╛╨┤╨╕╤А╨╛╨▓╨║╨╕ ╨╕ `git diff --check` ╤З╨╕╤Б╤В╤Л╨╡.

### ╨Ю╨┐╤В╨╕╨╝╨╕╨╖╨░╤Ж╨╕╤П ╨║╨╛╨┤╨╛╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╨╕: ╨▓╨║╨╗╤О╤З╤С╨╜ Whole Program Optimization (/GL + /LTCG) ╨┤╨╗╤П Release|x64
- ╨Я╤А╨╕╤З╨╕╨╜╨░: ╨▓╨╛ ╨▓╤Б╨╡╤Е ╨┐╤А╨╛╨╡╨║╤В╨░╤Е ╨╜╨░╨▒╨╛╤А ╨┐╤А╨░╨▓╨╕╨╗ ╨╖╨░╨┤╨░╨▓╨░╨╗ ╨┐╤А╨╛╨╡╨║╤В╨╜╤Л╨╣ `WholeProgramOptimization=true` (╨║╤А╨╛╨╝╨╡ flylinkdc) ╨╕ `LIB.LinkTimeCodeGeneration=true`, ╨╜╨╛ ╨▓ ╨║╨░╨╢╨┤╨╛╨╝ ItemDefinitionGroup `Release|x64` ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╜╤Л╨╣ `ClCompile.WholeProgramOptimization` ╨▒╤Л╨╗ ╤П╨▓╨╜╨╛ `false` тАФ ╨╕╨╖-╨╖╨░ ╤Н╤В╨╛╨│╨╛ `/GL` ╨╜╨╡ ╨▓╤Л╨┤╨░╨▓╨░╨╗╤Б╤П, ╨╕ ╨╗╨╕╨╜╨║╨╛╨▓╨║╨░ ╤Б `/LTCG` ╤Д╨░╨║╤В╨╕╤З╨╡╤Б╨║╨╕ ╨▒╤Л╨╗╨░ no-op (╨┐╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╛: `/GL` ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╨╛╨▓╨░╨╗ ╨▓ CL-╨║╨╛╨╝╨░╨╜╨┤╨╜╨╛╨╣ ╤Б╤В╤А╨╛╨║╨╡ ╨▓╤Б╨╡╤Е 13 ╨┐╤А╨╛╨╡╨║╤В╨╛╨▓).
- ╨г╨▒╤А╨░╨╜╨╛ ╨┐╨╛╨┤╨░╨▓╨╗╨╡╨╜╨╕╨╡ `/GL`: `ClCompile.WholeProgramOptimization` ╨╕╨╖╨╝╨╡╨╜╤С╨╜ ╤Б `false` ╨╜╨░ `true` ╨▓ `Release|x64` ╨▓╤Б╨╡╤Е ╤Г╤З╨░╤Б╤В╨▓╤Г╤О╤Й╨╕╤Е ╨┐╤А╨╛╨╡╨║╤В╨╛╨▓ (client, bzip2, zlib-ng, GdiOle, boost, MediaInfoLib, ZenLib, openssl, libtorrent, lua, LevelDB); flylinkdc ╤В╨░╨║╨╢╨╡ ╨┐╨╛╨╗╤Г╤З╨╕╨╗ ╨┐╤А╨╛╨╡╨║╤В╨╜╤Л╨╣ `WholeProgramOptimization=true`. ╨б╨╗╤Г╨╢╨╡╨▒╨╜╤Л╨╣ ╨│╨╡╨╜╨╡╤А╨░╤В╨╛╤А MakeDefs ╨╛╤Б╤В╨░╨▓╨╗╨╡╨╜ ╨▒╨╡╨╖ `/GL`. ╨в╨╡╨┐╨╡╤А╤М `/GL` ╨┐╤А╨╕╤Б╤Г╤В╤Б╤В╨▓╤Г╨╡╤В ╨▓╨╛ ╨▓╤Б╨╡╤Е Release-╨╛╨▒╤К╨╡╨║╤В╨╜╤Л╤Е ╤Д╨░╨╣╨╗╨░╤Е, ╨░ ╨╗╨╕╨╜╨║╨╛╨▓╨║╨░ flylinkdc ╨▓╤Л╨┐╨╛╨╗╨╜╤П╨╡╤В╤Б╤П ╤Б `/LTCG:incremental` ╨╕ `/LTCGOUT`.
- ╨г╤Б╤В╤А╨░╨╜╤С╨╜ ╨║╨╛╨╜╤Д╨╗╨╕╨║╤В ╨╕╨╜╤Б╤В╤А╤Г╨╝╨╡╨╜╤В╨░╤А╨╕╨╡╨▓, ╨▓╤Л╤П╨▓╨╗╨╡╨╜╨╜╤Л╨╣ ╨╗╨╕╨╜╨║╨╛╨▓╨║╨╛╨╣ (C1047/LNK1257): ╨▓ ╤А╨░╨▒╨╛╤З╨╡╨╣ ╨║╨╛╨┐╨╕╨╕ `Release|x64` ╨┐╤А╨╛╨╡╨║╤В╨░ flylinkdc ╨▒╤Л╨╗ ╨╜╨░ v145 (MSVC 14.51.36231), ╨░ ╨▓╤Б╨╡ ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║╨╕ тАФ ╨╜╨░ v143 (MSVC 14.44.35207); ╨┐╤А╨╕ `/GL` ╤Б╨║╨▓╨╛╨╖╨╜╨░╤П ╨╛╨┐╤В╨╕╨╝╨╕╨╖╨░╤Ж╨╕╤П ╨╛╨▒╤К╨╡╨║╤В╨╛╨▓, ╤Б╨╛╨▒╤А╨░╨╜╨╜╤Л╤Е ╤А╨░╨╖╨╜╤Л╨╝╨╕ ╨▓╨╡╤А╤Б╨╕╤П╨╝╨╕ ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨░, ╨╜╨╡╨▓╨╛╨╖╨╝╨╛╨╢╨╜╨░. `Release|x64` flylinkdc ╨▓╨╛╨╖╨▓╤А╨░╤Й╤С╨╜ ╨║ v143 (╨║╨░╨║ ╨▓ HEAD), ╤З╤В╨╛╨▒╤Л ╨▓╨╡╤Б╤М Release-╨╜╨░╨▒╨╛╤А `/GL`-╨╛╨▒╤К╨╡╨║╤В╨╛╨▓ ╤Б╨╛╨▒╨╕╤А╨░╨╗╤Б╤П ╨╛╨┤╨╜╨╕╨╝ ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╛╨╝ 14.44.35207. Debug ╨╜╨╡ ╨╕╨╖╨╝╨╡╨╜╤С╨╜.
- ╨а╨╡╨╖╤Г╨╗╤М╤В╨░╤В: Release|x64 тАФ EXE 20 198 400 ╨▒╨░╨╣╤В (╨▒╤Л╨╗╨╛ 19 981 824; +1.1% ╨╕╨╖-╨╖╨░ ╨░╨│╤А╨╡╤Б╤Б╨╕╨▓╨╜╨╛╨│╨╛ ╨╕╨╜╨╗╨░╨╣╨╜╨░ ╨┐╤А╨╕ LTCG), ╨┐╨╛╨╗╨╜╨░╤П ╨┐╨╡╤А╨╡╤Б╨▒╨╛╤А╨║╨░ 2.08 ╨╝╨╕╨╜ (╨▒╤Л╨╗╨╛ 2.33 ╨╝╨╕╨╜), 0 ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣/0 ╨╛╤И╨╕╨▒╨╛╨║; ╨╕╨╝╨┐╨╛╤А╤В╤Л тАФ ╤В╨╛╨╗╤М╨║╨╛ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╨╡ DLL Windows; ╨┤╤Л╨╝╨╛╨▓╨╛╨╣ ╨╖╨░╨┐╤Г╤Б╨║ ╤Г╤Б╨┐╨╡╤И╨╡╨╜. Debug|x64 тАФ ╨▒╨╡╨╖ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╨╣ (WPO ╨▓ Debug ╨▓╤Л╨║╨╗╤О╤З╨╡╨╜): 0/0, C26494=0.
- ╨Я╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╛ ╨┐╨╛ `.tlog`: `/GL` ╨▓ CL-╨║╨╛╨╝╨░╨╜╨┤╨░╤Е client (104), flylinkdc (102), libtorrent (173), MediaInfoLib (250), openssl (678) ╨╕ ╨┤╤А.; `link.command` ╤Б╨╛╨┤╨╡╤А╨╢╨╕╤В `/LTCG:incremental` ╨╕ `/LTCGOUT`.

## 2026-10-10
### ╨б╨▒╨╛╤А╨║╨░ Release|x64 ╨▓ Visual Studio 18 Insiders (MSVC)
- ╨а╨╡╤И╨╡╨╜╨╕╨╡ FlylinkDC_2022.sln ╤Б╨╛╨▒╤А╨░╨╜╨╛ ╨╜╨░ ╤Г╨╖╨╗╨╡ ╤З╨╡╤А╨╡╨╖ MSBuild (VS 18 Insiders, MSVC 14.51.36231/v145 ╨╕ 14.44.35207/v143, Windows SDK 10.0.26100) ╨▓ ╨║╨╛╨╜╤Д╨╕╨│╤Г╤А╨░╤Ж╨╕╨╕ Release|x64; ╨║╨╛╨┤ ╨▓╨╛╨╖╨▓╤А╨░╤В╨░ 0.
- ╨Р╤А╤В╨╡╤Д╨░╨║╤В: compiled/flylinkdc_x64.exe (~19 ╨Ь╨С). ╨Я╤А╨╛╨▓╨╡╤А╨║╨░ `dumpbin /dependents` ╨┐╨╛╨║╨░╨╖╤Л╨▓╨░╨╡╤В ╤В╨╛╨╗╤М╨║╨╛ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╨╡ DLL Windows (WS2_32, KERNEL32, USER32, GDI32, gdiplus, SHELL32 ╨╕ ╤В.╨┐.); ╤Б╤В╨╛╤А╨╛╨╜╨╜╨╕╤Е ╨╕ runtime-DLL (vcruntime/msvcp) ╨╜╨╡╤В тАФ ╤Б╤В╨░╤В╨╕╤З╨╡╤Б╨║╨░╤П ╨╗╨╕╨╜╨║╨╛╨▓╨║╨░.
- ╨Ф╤Л╨╝╨╛╨▓╨╛╨╣ ╨╖╨░╨┐╤Г╤Б╨║ EXE ╨╜╨░ Windows: ╨┐╤А╨╛╤Ж╨╡╤Б╤Б ╤Б╤В╨░╤А╤В╤Г╨╡╤В ╨╕ ╨╢╨╕╨▓╤С╤В, ╨┐╨░╨┤╨╡╨╜╨╕╤П ╨╜╨╡╤В.
- ╨б╨▒╨╛╤А╨║╨░ ╨╜╨╡ warning-clean ╨┐╤А╨╕ /W4: ~3000+ ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ (╨▓ ╨╛╤Б╨╜╨╛╨▓╨╜╨╛╨╝ C4100, C4189, C4057; ╨╡╤Б╤В╤М C4244/C4267/C4311/C4302 тАФ ╤Г╤Б╨╡╤З╨╡╨╜╨╕╨╡ ╨╜╨░ x64). ╨Ф╨╕╨░╨│╨╜╨╛╤Б╤В╨╕╨║╨░: LNK4075 (/INCREMENTAL ╨╕╨│╨╜╨╛╤А╨╕╤А╤Г╨╡╤В╤Б╤П ╨╕╨╖-╨╖╨░ /LTCG), LNK4042 (winfirewall.obj ╨┤╨▓╨░╨╢╨┤╤Л ╨▓ client), MSB8012 (MakeDefs OutputFile ╨╜╨╡ ╤Б╨╛╨▓╨┐╨░╨┤╨░╨╡╤В ╤Б OutDir).
- CMake/minGW-╨┐╤Г╤В╤М (build-win.sh, build-with-mxe.sh) ╨┐╨╛-╨┐╤А╨╡╨╢╨╜╨╡╨╝╤Г ╨╜╨╡╤А╨░╨▒╨╛╤З╨╕╨╣: ╨╜╨╡╤В ╤Б╨╛╨▒╤А╨░╨╜╨╜╤Л╤Е MinGW-╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛╤Б╤В╨╡╨╣ ╨╕ WTL/ATL.
### ╨Ш╤Б╨┐╤А╨░╨▓╨╗╨╡╨╜╨╕╨╡ ╨┤╨╕╨░╨│╨╜╨╛╤Б╤В╨╕╨║╨╕ ╨╕ ╤Б╨╜╨╕╨╢╨╡╨╜╨╕╨╡ ╤И╤Г╨╝╨░ ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣
- LNK4075: ╨▓ flylinkdc_2022.vcxproj (Release|x64) LinkIncremental=false.
- LNK4042: ╨▓ client_2022.vcxproj ╨┤╨╗╤П client\webrtc\talk\base\winfirewall.cc ╨╖╨░╨┤╨░╨╜ ╨╛╤В╨┤╨╡╨╗╤М╨╜╤Л╨╣ ObjectFileName (winfirewall_webrtc.obj), ╤З╤В╨╛╨▒╤Л ╨╜╨╡ ╨║╨╛╨╜╤Д╨╗╨╕╨║╤В╨╛╨▓╨░╤В╤М ╤Б FlyFeatures\WinFirewall.cpp.
- MSB8012: ╨▓ MakeDefs\MakeDefs.vcxproj OutDir ╨┐╤А╨╕╨▓╨╡╨┤╤С╨╜ ╨║ $(SolutionDir)MakeDefs\ (╤Б╨╛╨▓╨┐╨░╨┤╨░╨╡╤В ╤Б Link.OutputFile, ╨║╨╛╤В╨╛╤А╤Л╨╣ ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╤В pre-build ╤И╨░╨│ client).
- ╨б╨╜╨╕╨╢╨╡╨╜╨╕╨╡ ╤И╤Г╨╝╨░: ╤Б╤В╨╛╤А╨╛╨╜╨╜╨╕╨╡ ╨┐╤А╨╛╨╡╨║╤В╤Л (openssl, MediaInfoLib, libtorrent, lua, LevelDB, zlib-ng, bzip2, boost-regex, ZenLib) ╨┐╨╡╤А╨╡╨▓╨╡╨┤╨╡╨╜╤Л ╨╜╨░ TurnOffAllWarnings; ╨┤╨╗╤П ╤Б╨▓╨╛╨╡╨│╨╛ ╨║╨╛╨┤╨░ ╨╛╤Б╤В╨░╨▓╨╗╨╡╨╜ /W4 ╨╕ ╨▓╨║╨╗╤О╤З╨╡╨╜╨╛ ╨╕╤Б╨║╨╗╤О╤З╨╡╨╜╨╕╨╡ ╨▓╨╜╨╡╤И╨╜╨╕╤Е ╨╖╨░╨│╨╛╨╗╨╛╨▓╨║╨╛╨▓ (/external:anglebrackets, ExternalWarningLevel=TurnOffAllWarnings) ╨▓ flylinkdc ╨╕ client.
- C4189 (╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╨╡, ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╨╝╤Л╨╡ ╤В╨╛╨╗╤М╨║╨╛ ╨▓ dcassert, ╤З╤В╨╛ ╨▓ Release ╨┐╤Г╤Б╤В): ╨┤╨╛╨▒╨░╨▓╨╗╨╡╨╜╤Л ╨┐╨╛╨┤╨░╨▓╨╗╨╡╨╜╨╕╤П `(void)var;` ╨▓ ╨┐╤А╨╛╨▒╨╗╨╡╨╝╨╜╤Л╤Е ╨╝╨╡╤Б╤В╨░╤Е (Bitmap.cpp, main.cpp, WinUtil.h, flattabctrl.h, WinUtil.cpp, TypedListViewCtrl.h, Util.cpp, CFlylinkDBManager.h/.cpp, SharedFileStream.cpp, File.cpp, ConnectionManager.cpp, flyServer.cpp, AutoUpdate.cpp).
- ╨Я╨╛╨╗╨╜╨╛╤Б╤В╤М╤О ╤Г╨┤╨░╨╗╤С╨╜ Visual Leak Detector (VLD): include-╨┐╤Г╤В╨╕ ╨▓ zlib-ng_2022.vcxproj, ╨║╨╛╨┤/╨╝╨░╨║╤А╨╛╤Б╤Л ╨▓ DCPlusPlus.cpp, main.cpp, stdinc.h; ╨╛╤В╨╗╨░╨┤╨╛╤З╨╜╤Л╨╣ dcdebug ╨▓ HIconWrapper.cpp ╨╖╨░╨║╨╗╤О╤З╤С╨╜ ╨▓ #ifdef _DEBUG.
- ╨Ъ╨╛╨╜╤В╤А╨╛╨╗╤М╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ ╨┐╨╛╤Б╨╗╨╡ ╨┐╤А╨░╨▓╨╛╨║ (Release|x64, exit 0): ╨╛╤И╨╕╨▒╨╛╨║ ╨╕ ╨┤╨╕╨░╨│╨╜╨╛╤Б╤В╨╕╨║ LNK/MSB ╨╜╨╡╤В, ╨┐╤А╨╡╨┤╤Г╨┐╤А╨╡╨╢╨┤╨╡╨╜╨╕╨╣ ~60 (╨▓ ╨╛╤Б╨╜╨╛╨▓╨╜╨╛╨╝ C4456 тАФ ╨╖╨░╤В╨╡╨╜╨╡╨╜╨╕╨╡ ╨╗╨╛╨║╨░╨╗╤М╨╜╤Л╤Е ╨┐╨╡╤А╨╡╨╝╨╡╨╜╨╜╤Л╤Е ╨▓ ╤Б╨▓╨╛╤С╨╝ ╨║╨╛╨┤╨╡; ╨╛╤В╨┤╨╡╨╗╤М╨╜╨╛ C4302/C4311 тАФ ╤Г╤Б╨╡╤З╨╡╨╜╨╕╨╡ ╤Г╨║╨░╨╖╨░╤В╨╡╨╗╨╡╨╣ ╨┐╤А╨╕ ╨║╨░╤Б╤В ╨║ int/UINT ╨╜╨░ x64, ╨┐╨╛╤В╨╡╨╜╤Ж╨╕╨░╨╗╤М╨╜╤Л╨╡ ╨▒╨░╨│╨╕: WinUtil.cpp:3889, TreePropertySheet.cpp:459, PropertyList.h:858). ╨Ш╨╝╨┐╨╛╤А╤В╤Л EXE тАФ ╤В╨╛╨╗╤М╨║╨╛ ╤Б╨╕╤Б╤В╨╡╨╝╨╜╤Л╨╡ DLL; ╨┤╤Л╨╝╨╛╨▓╨╛╨╣ ╨╖╨░╨┐╤Г╤Б╨║ ╨╜╨░ Windows ╤Г╤Б╨┐╨╡╤И╨╡╨╜.

## 2026-09-17
### ╨б╨▒╨╛╤А╨╛╤З╨╜╨╛╨╡ ╨╛╨║╤А╤Г╨╢╨╡╨╜╨╕╨╡ Docker
- ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜ Dockerfile ╨╜╨░ ╨▒╨░╨╖╨╡ Ubuntu 26.04 ╤Б MinGW-w64, CMake, Ninja, Make, ╨╕╨╜╤Б╤В╤А╤Г╨╝╨╡╨╜╤В╨░╨╝╨╕ ╤Б╨▒╨╛╤А╨║╨╕ ╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛╤Б╤В╨╡╨╣ ╨╕ ShellCheck; ╨╛╨▒╤А╨░╨╖ ╤Г╤Б╨┐╨╡╤И╨╜╨╛ ╤Б╨╛╨▒╤А╨░╨╜.
- ╨Т AGENTS.md ╨╖╨░╨║╤А╨╡╨┐╨╗╨╡╨╜╤Л ╤Б╨▒╨╛╤А╨║╨░ ╨╕ ╨┐╤А╨╛╨▓╨╡╤А╨║╨╕ ╤В╨╛╨╗╤М╨║╨╛ ╨▓ Docker, ╨╖╨░╨┐╤А╨╡╤В ╤Г╤Б╤В╨░╨╜╨╛╨▓╨║╨╕ ╤Б╨▒╨╛╤А╨╛╤З╨╜╤Л╤Е ╨┐╨░╨║╨╡╤В╨╛╨▓ ╨╜╨░ ╤Е╨╛╤Б╤В, ╨║╨╛╨╝╨░╨╜╨┤╤Л ╨╖╨░╨┐╤Г╤Б╨║╨░ ╤Б UID/GID ╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╤В╨╡╨╗╤П ╨╕ ╤Б╨╛╤Е╤А╨░╨╜╨╡╨╜╨╕╨╡ ╨░╤А╤В╨╡╤Д╨░╨║╤В╨╛╨▓ ╨▓ build-win.
- ╨Ч╨░╤Д╨╕╨║╤Б╨╕╤А╨╛╨▓╨░╨╜╨░ ╤Ж╨╡╨╗╤М Windows x64 ╤Б╨╛ ╤Б╤В╨░╤В╨╕╤З╨╡╤Б╨║╨╕╨╝╨╕ ╤Б╤В╨╛╤А╨╛╨╜╨╜╨╕╨╝╨╕ ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║╨░╨╝╨╕ ╨╕ runtime ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨░; ╤В╤А╨╡╨▒╤Г╨╡╤В╤Б╤П ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ ╨╕╨╝╨┐╨╛╤А╤В╨╛╨▓ EXE ╨╕ ╨╛╤В╨┤╨╡╨╗╤М╨╜╤Л╨╣ ╨╖╨░╨┐╤Г╤Б╨║ ╨╜╨░ Windows.
- build-win.sh ╨┐╨╡╤А╨╡╨▓╨╡╨┤╤С╨╜ ╨╜╨░ CMake -S/-B ╤Б ╨░╨▒╤Б╨╛╨╗╤О╤В╨╜╤Л╨╝╨╕ ╨┐╤Г╤В╤П╨╝╨╕, Release, BUILD_SHARED_LIBS=OFF, ╤Б╤В╤А╨╛╨│╨╛╨╣ ╨╛╨▒╤А╨░╨▒╨╛╤В╨║╨╛╨╣ ╨╛╤И╨╕╨▒╨╛╨║ ╨╕ ╨╜╨░╤Б╤В╤А╨╛╨╣╨║╨╛╨╣ ╨┐╨░╤А╨░╨╗╨╗╨╡╨╗╨╕╨╖╨╝╨░ ╤З╨╡╤А╨╡╨╖ JOBS; ShellCheck ╨▓ Docker ╨┐╤А╨╛╤Е╨╛╨┤╨╕╤В.
- ╨Я╤А╨╛╨▒╨╜╨░╤П ╤Б╨▒╨╛╤А╨║╨░ ╨▓ Docker ╨╛╤Б╤В╨░╨╜╨╛╨▓╨╕╨╗╨░╤Б╤М ╨┐╤А╨╕ ╨│╨╡╨╜╨╡╤А╨░╤Ж╨╕╨╕ CMake: ╨╜╨╡ ╨╜╨░╨╣╨┤╨╡╨╜╤Л Windows-╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║╨╕ OpenSSL, zlib, bzip2, libtorrent, leveldb, Lua, jsoncpp, maxminddb, MediaInfo ╨╕ ZenLib. ╨Ш╤Е ╤Б╨▒╨╛╤А╨║╨░ ╨╕╨╖ ╨╕╤Б╤Е╨╛╨┤╨╜╨╕╨║╨╛╨▓ ╨┐╨╛╨║╨░ ╨╜╨╡ ╨╜╨░╤Б╤В╤А╨╛╨╡╨╜╨░; BUILD_SHARED_LIBS=OFF ╤Б╨░╨╝ ╨┐╨╛ ╤Б╨╡╨▒╨╡ ╨╜╨╡ ╨╛╨▒╨╡╤Б╨┐╨╡╤З╨╕╨▓╨░╨╡╤В ╤Б╤В╨░╤В╨╕╤З╨╡╤Б╨║╤Г╤О ╨╗╨╕╨╜╨║╨╛╨▓╨║╤Г runtime ╨╕ ╨▓╨╜╨╡╤И╨╜╨╕╤Е ╨▒╨╕╨▒╨╗╨╕╨╛╤В╨╡╨║.
- ╨Я╤А╨╛╨▓╨╡╤А╨║╨░ client/Util.h ╨║╨╛╨╝╨┐╨╕╨╗╤П╤В╨╛╤А╨╛╨╝ MinGW-w64 ╨▓ Docker ╨┐╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨░╨╡╤В ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╨╕╨╡ atlcomtime.h; GUI ╤В╨░╨║╨╢╨╡ ╤В╤А╨╡╨▒╤Г╨╡╤В ATL. ╨б╨╛╨▓╨╝╨╡╤Б╤В╨╕╨╝╨╛╤Б╤В╤М ATL/MSVC ╨╛╤Б╤В╨░╤С╤В╤Б╤П ╨▒╨╗╨╛╨║╨╡╤А╨╛╨╝; ╨│╨╛╤В╨╛╨▓╤Л╨╣ EXE ╨╜╨╡ ╨┐╨╛╨╗╤Г╤З╨╡╨╜, ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ ╨╕╨╝╨┐╨╛╤А╤В╨╛╨▓ ╨╕ ╨╖╨░╨┐╤Г╤Б╨║ ╨╜╨░ Windows ╨╜╨╡ ╨▓╤Л╨┐╨╛╨╗╨╜╨╡╨╜╤Л.

## 2026-03-14
### ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜╨░ ╨┐╨╛╨┤╨┤╨╡╤А╨╢╨║╨░ ╨║╤А╨╛╤Б╤Б-╨┐╨╗╨░╤В╤Д╨╛╤А╨╝╨╡╨╜╨╜╨╛╨╣ ╤Б╨▒╨╛╤А╨║╨╕
- ╨б╨╛╨╖╨┤╨░╨╜ CMakeLists.txt ╨┤╨╗╤П ╤Б╨▒╨╛╤А╨║╨╕ ╨┐╤А╨╛╨╡╨║╤В╨░ ╤Б ╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜╨╕╨╡╨╝ CMake.
- ╨Ф╨╛╨▒╨░╨▓╨╗╨╡╨╜╨░ ╨▓╨╛╨╖╨╝╨╛╨╢╨╜╨╛╤Б╤В╤М ╨║╤А╨╛╤Б╤Б-╨║╨╛╨╝╨┐╨╕╨╗╤П╤Ж╨╕╨╕ ╨┐╨╛╨┤ Windows (x64) ╨╕╨╖-╨┐╨╛╨┤ Linux ╤Б ╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜╨╕╨╡╨╝ mingw-w64.
- ╨Ш╨╜╤В╨╡╨│╤А╨░╤Ж╨╕╤П ╤Б ╤Б╤Г╤Й╨╡╤Б╤В╨▓╤Г╤О╤Й╨╕╨╝╨╕ ╨╖╨░╨▓╨╕╤Б╨╕╨╝╨╛╤Б╤В╤П╨╝╨╕ (Boost, OpenSSL, libtorrent, zlib, bzip2, leveldb, lua, jsoncpp, maxminddb, mediainfolib, Zenlib).
- ╨Ю╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨░ ╨┤╨╛╨║╤Г╨╝╨╡╨╜╤В╨░╤Ж╨╕╤П ╨┐╨╛ ╤Б╨▒╨╛╤А╨║╨╡.

### ╨Ш╨╖╨╝╨╡╨╜╨╡╨╜╨╕╤П ╨▓ ╤Б╤В╤А╤Г╨║╤В╤Г╤А╨╡ ╨┐╤А╨╛╨╡╨║╤В╨░
- ╨д╨░╨╣╨╗╤Л Visual Studio (.vcxproj, .sln) ╨╛╤Б╤В╨░╤О╤В╤Б╤П ╨┤╨╗╤П ╤Б╨▒╨╛╤А╨║╨╕ ╨┐╨╛╨┤ Windows ╤Б MSVC.
- CMake ╨┐╨╛╨╖╨▓╨╛╨╗╤П╨╡╤В ╨░╨╗╤М╤В╨╡╤А╨╜╨░╤В╨╕╨▓╨╜╤Г╤О ╤Б╨▒╨╛╤А╨║╤Г ╨▒╨╡╨╖ Visual C++.

## ╨Я╤А╨╡╨┤╤Л╨┤╤Г╤Й╨╕╨╡ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╤П
(╤Б╨╝. ╨╕╤Б╤В╨╛╤А╨╕╤О ╨║╨╛╨╝╨╝╨╕╤В╨╛╨▓ ╤А╨╡╨┐╨╛╨╖╨╕╤В╨╛╤А╨╕╤П)

## 2026-10-16
### Фиксы _snprintf (защита от усечения без NUL)
 - client/FinishedManager.cpp: добавлена явная NUL-терминация буфера после _snprintf (предотвращает heap over-read при усечении).
 - client/ClientManager.cpp: NUL-терминация buf после _snprintf в checkCheating.
 - client/Socket.cpp: NUL-терминация tmp в SocketException::errorToString.
 - client/UploadManager.cpp: NUL-терминация трёх буферов (bufNewLeacherShare/bufUploadUserShare/bufShareDelta) после _snprintf.
 - windows/MainFrm.cpp: NUL-терминация l_buf после _snprintf.
 - client/CFlylinkDBManager.cpp, client/CFlyProfiler.cpp, client/MD5Calc.cpp, client/TraceManager.cpp, client/Util.cpp, windows/QueueFrame.cpp, windows/SearchFrm.cpp: замена sprintf на _snprintf + NUL-терминация там, где это минимально и безопасно.
 - client/XMLParser/xmlParser.cpp: sprintf -> _snprintf с защитой NUL.
 - client/idna/idna.cpp: sprintf -> _snprintf/sprintf_s с дополнительными проверками границ.
 - client/FlyFeatures/flyServer.cpp: отладочный sprintf -> _snprintf.

### Сканы и мелкие правки
 - Продолжён скан потенциально небезопасных паттернов (size()-N, memcpy/strcpy, sprintf/_snprintf) в client/ и windows/; большинство кандидатов либо безопасны, либо уже огорожены. Фиксились только реальные риски с сохранением поведения (W=0,E=0).
 - client/ShareManager.cpp, client/FlyFeatures/VideoPreview.cpp, windows/wtlbuilder/GDIUtil.cpp — выявлены только закомментированные/мёртвые sprintf или безопасные места; вмешательства минимальны.

#### Контрольная сборка (финал)
- Debug|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=54883840).
- Release|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=20748800).
- Итог: исправлено 53 дефекта в собственном коде; количество находок /analyze в собственном коде сократилось с 49 до 26.
- Отдельно: русские комментарии в исходниках хранятся в CP1251 — инструмент, который читат файл как UTF-8, портит комментарии
  (uild-win/repair-cp1251.py — восстановлено оригинальные байты из git show HEAD:<file>).

#### Отложено (требует архитектурной переработки, описано в TODO.md)
- `Identity::setNick()` без блокировки + `getNick()` возвращает ссылку (UAF).
- Модальный `MessageBox` и HTTP-POST под глобальной блокировкой БД.
- Сканирование ФС под эксклюзивной блокировкой шары.
- Сырой `Client*` после освобождения блокировки списка клиентов.

#### Контрольная сборка (финальная)
- [x] **Финальная контрольная сборка** (2026-10-20 02:31-02:36):
  Debug|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=54908928);
  Release|x64 Rebuild — 0 предупреждений / 0 ошибок (EXE=20761600).
- [x] **Финальный дымовой запуск** (Release|x64): процесс жив 30 с, 32 потока, 124 МБ ОЗУ,
  подключение к хабам, проброс портов через UPnP, новых дампов нет, в `System.log` нет
  исключений и assert-ов.
- [x] **В репозитории не осталось ни одного повреждённого байта U+FFFD** (проверены все
  отслеживаемые файлы).
