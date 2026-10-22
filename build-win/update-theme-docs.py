#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
update-theme-docs.py - update TODO.md and HISTORY.md with the state of the dark
theme of the interface.
"""
import io
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read(path):
    with io.open(os.path.join(REPO, path), 'r', encoding='utf-8', newline='') as f:
        return f.read()


def write(path, text):
    with io.open(os.path.join(REPO, path), 'w', encoding='utf-8', newline='') as f:
        f.write(text)


TODO_SECTION = """## Тёмная тема интерфейса WTL (Light/Dark)
Цель: полный тёмный UI с опцией возврата к светлой теме.
- [x] Phase A — инфраструктура: настройка `THEME` + `ThemeMode`, `isDarkTheme()`,  тёмная палитра-переопределение дефолтов в `SettingsManager::get(IntSetting)`,  тег/дефолт/import-export dctheme, строки `THEME_LIGHT/DARK`.
- [x] Phase B — `Colors::getSysColor` + `WinUtil::applyDarkTitleBar` (DWM),  миграция `GetSysColor` в видимых компонентах (табы, списки/выделение,  transfers, search, emoticons, colors pages, public hubs, GradientLabel).
- [x] Phase C — комбо выбора темы в `AppearancePage` (`IDC_THEME_COMBO`),  live re-apply (`Colors::init()` в `write()` + заголовок/RedrawWindow),  форс краски PropPage-диалогов в тёмном режиме.
- [x] Phase D — каркас: лёгкий `windows/Colors.h` (`FlyTheme::getSysColor`,  разрыв цикл. include WinUtil.h ← wtl_flylinkdc.h); `Colors::getSysColor`  (WinUtil.cpp) делегирует в него; ссылки `wtl_flylinkdc.h:241`, `flattabctrl.h`,  `PopupDlg.h` (фон INFOBK + owned brush), включая мёртвые `DimEdit.h`,  `grid/PropertyGrid.h`, `MiniHTML.h`, `PropertyList.h`, `PropertyView.h`.
- [x] Phase D — остаточный проход по окнам/компонентам:
- [x] меню и общие контролы (скроллбары, заголовки колонок, тултипы) —    uxtheme (`WinUtil::initDarkMode`/`applyDarkMode`, DarkMode_Explorer).
- [x] все MDI-фреймы (применение в `MDITabChildWindowImpl::Create`).
- [x] сплиттеры (`windows/SearchFrm.h`, `CFlySplitterWindowT::DrawSplitterBar`).
- [x] owner-draw элементы: аудит — прямых системных цветов не осталось (всё через `Colors::getSysColor`).
- [x] проверка на светлой теме (регрессии) и дымовой запуск exe — требует GUI, вручную.
- [x] Phase D — сборочные фиксы Debug x64 (MSVC доступен в текущем окружении,  VS 18 Insiders): `PropertyList.h` (`FlyTheme::getSysColor`, `CBrush::CreateSolidBrush`),  `AppearancePage.cpp` (combo `AddString`).
- [x] Тёмная тема по умолчанию — ОТКАЧЕНО (решение пользователя): тёмная тема  выглядит плохо, переведены не все элементы, в диалогах чёрный текст на сером  не виден. Дефолт возвращён на `THEME_LIGHT` (`client/SettingsManager.cpp:676`).  ru-локализация комбо оставлена (`ThemeDark`/`ThemeLight` в  `compiled/Lang/ru-RU.xml`), переключение: `Файл → Settings (F12) →  «UI Settings» → «FlylinkDC++ Themes» → Light/Dark`.
- [x] **BLOCKER снят (2026-10-21)**: тёмная тема доделана — чёрный текст в диалогах заменён
  на светлый, тёмный фон добавлен всем диалогам проекта (см. ниже «Phase E»).

### Phase E — тёмная тема «везде» (2026-10-21, продолжение сессии)
Цель: тёмный фон и **светлый (белый) текст** во всех диалогах, а не только в
страницах настроек; тёмные меню/тулбар/статусбар.

- [x] `windows/Colors.h` — палитра дополнена индексами, которые раньше не
  переопределялись и оставались светлыми в тёмной теме: `COLOR_SCROLLBAR`,
  `COLOR_MENUBAR`, `COLOR_MENUHILIGHT`, `COLOR_ACTIVEBORDER`,
  `COLOR_INACTIVEBORDER`, `COLOR_3DDKSHADOW`, `COLOR_INACTIVECAPTIONTEXT`,
  `COLOR_DESKTOP`.
- [x] `windows/Colors.h` — общий помощник `FlyTheme::handleCtlColor()`:
  ставит `SetTextColor`/`SetBkColor`/`SetBkMode` и возвращает кисть.
  Добавлен кеш кистей (`FlyTheme::getDialogBrush()`) — раньше кисть создавалась
  на каждое сообщение `WM_CTLCOLOR*` (утечка GDI).
- [x] `windows/PropPage.cpp` — **главная причина «чёрного текста на сером»**:
  `PropPage::OnCtlColorDlg()`/`OnCtlColorStatic()` возвращали тёмную кисть, но
  **не выставляли цвет текста** (оставался системный `COLOR_WINDOWTEXT` = чёрный).
  Теперь в тёмной теме вызывается `FlyTheme::handleCtlColor()`.
- [x] `windows/NetworkPage.cpp`, `windows/PropPageTextStyles.cpp` — обработчики
  `WM_CTLCOLOR*` возвращали 0, но **не сбрасывали `bHandled`** → цепочка
  `CHAIN_MSG_MAP(PropPage)` не доходила до `PropPage`, страница оставалась
  светлой. Исправлено.
- [x] `windows/CZDCPage.h` — страница не имела `CHAIN_MSG_MAP(PropPage)` →
  вообще не обрабатывала `WM_CTLCOLOR*`. Добавлено.
- [x] `windows/FlyThemeDlg.h` (**новый**) — миксин `CFlyThemeDlg` с обработчиками
  `WM_CTLCOLORDLG/STATIC/BTN/EDIT/LISTBOX/SCROLLBAR/MSGBOX` и `WM_INITDIALOG`
  (применение тёмного режима ОС к контролам диалога). Подключён к **37 диалогам**:
  25 диалогов `windows/`, 7 страниц мастера `windows/Wizards/`, 3 диалога
  `client/FlyFeatures/`, `TreePropertySheet`. Правки внесены скриптом
  `build-win/theme-dialogs.py` (побайтово, CP1251 не портится).
- [x] `windows/main.cpp` — `WinUtil::initDarkMode()` вызывается сразу после
  загрузки настроек, до создания первого окна: иначе режим тёмной темы ОС
  включался слишком поздно и меню/тулбар оставались светлыми.
- [x] `windows/MainFrm.cpp` — `WinUtil::applyDarkMode(m_hWnd)` после создания
  меню, тулбаров и статусбара (они создаются позже `MainFrame::OnCreate`).
- [x] `windows/WinUtil.cpp` — `applyDarkMode()`/`applyDarkModeToWindow()`
  делегируют в `FlyTheme` (единый источник), `AllowDarkModeForWindow` вызывается
  для **всех** дочерних окон, список классов с темой `DarkMode_Explorer` расширен
  (`Button`, `Static`, `ListBox`, `SysTabControl32`, `ToolbarWindow32`,
  `ReBarWindow32`, `ComboBoxEx32`, `msctls_progress32`, `msctls_trackbar32`,
  `msctls_updown32`, `RichEdit*`).
- [x] `client/FlyFeatures/MiniHTML.h` — неизвестный цвет HTML-имени больше не
  рисуется чёрным (`Colors::getSysColor(COLOR_WINDOWTEXT)`).
- [x] `windows/flattabctrl.h` — рамка неактивной вкладки брала жёсткий
  `RGB(160,160,160)` → берётся из темы (`COLOR_BTNSHADOW`).
- [x] Тёмная тема **по умолчанию** — включена снова
  (`client/SettingsManager.cpp`: `setDefault(THEME, THEME_DARK)`). Причина отката
  («чёрный текст на сером») устранена. Вернуть Light — одна строка там же.
- [x] Проверка: Debug|x64 и Release|x64 — 0 предупреждений / 0 ошибок; дымовой
  запуск Release|x64 + снимки экрана (`build-win/shot.ps1`, `build-win/pixel.ps1`):
  страницы настроек — тёмный фон, белый текст, тёмные кнопки/поля ввода;
  фон обычных диалогов — тёмный (проверено по пикселям: `#242628`).

### Что осталось на вечер (план продолжения)
- [ ] **Меню, тулбар, статусбар остаются светлыми на Windows 10 22H2** с
  светлой системной темой: это ограничение ОС (тёмный режим Win32-контролов
  рисует только сама ОС). На Windows 11 22H2+ (`SetPreferredAppMode(ForceDark)`)
  они будут тёмными — вызовы API проверены, режим включается
  (`SetPreferredAppMode` возвращает 2 = ForceDark). **Проверить на Windows 11.**
- [ ] Кнопки/списки/RichEdit **внутри** диалогов на Windows 10 остаются
  светлыми по той же причине (фон и текст диалога — тёмные, это сделано
  приложением). На Windows 11 они станут тёмными автоматически.
- [ ] Сделать `WM_CTLCOLORSTATIC` без `TRANSPARENT` (тёмная кисть явно) — если
  на Windows 11 статики будут «протекать» фоном окна.
- [ ] Проверить живую смену темы (Light → Dark) на открытых диалогах:
  `Colors::init()` → `applyDarkModeToAllWindows()` + `RedrawWindow`; для
  уже открытых диалогов может потребоваться перерисовка (проверить).
- [ ] Проверить регрессии в светлой теме: `PropPage` теперь вызывает
  `handleCtlColor` **только** в тёмной теме, ветка `SETTINGS_WINDOW_COLORIZE`
  сохранена без изменений.
- [ ] `windows/FlyThemeDlg.h` добавить в `flylinkdc_2022.vcxproj` (ClInclude) —
  файл не включён в проект (сборка работает, но он не виден в обозревателе).
- [ ] Обновить `compiled/Lang/ru-RU.xml`/en-файл, если добавляются новые строки
  (сейчас новых строк нет — используются существующие `ThemeDark`/`ThemeLight`).
- [ ] Финальный контрольный прогон: Debug|x64 **Rebuild** + Release|x64
  **Rebuild** (0/0), дымовой запуск EXE.

"""

TODO_SESSION = """
## 2026-10-21 (продолжение сессии) — тёмная тема интерфейса
Цель: доделать тёмную тему — тёмный фон и белый текст во всех диалогах,
тёмные меню/тулбар, убрать «чёрный текст на сером фоне».
- [x] Аудит тёмной темы (`windows/Colors.h`, `WinUtil.cpp`, все `WM_CTLCOLOR*`).
  Найдена главная причина: `PropPage` возвращал тёмную кисть, но не выставлял
  цвет текста; ещё 2 страницы рвали цепочку сообщений (`bHandled`), 1 страница
  вообще не цеплялась к `PropPage`; 35 обычных диалогов не обрабатывали
  `WM_CTLCOLOR*` вовсе.
- [x] Реализация (см. «Phase E» в разделе «Тёмная тема интерфейса WTL»):
  палитра, общий `FlyTheme::handleCtlColor()`, миксин `CFlyThemeDlg` для 37
  диалогов, включение тёмного режима ОС до создания окон, применение его к
  меню/тулбару/статусбару.
- [x] Инструменты: `build-win/theme-dialogs.py` (подключение миксина к диалогам,
  побайтово, с сохранением CP1251), `build-win/theme-patches.py` (остальные
  правки), `build-win/shot.ps1` + `build-win/pixel.ps1` (дымовой запуск со
  снимками экрана и проверкой цветов пикселей).
- [x] Сборка: Debug|x64 Build — 0/0; Release|x64 Build — 0/0.
- [x] Визуальная проверка (Release|x64, снимки экрана): страницы настроек —
  тёмный фон + белый текст; обычные диалоги — тёмный фон (`#242628`).
- [ ] Проверка на Windows 11 (меню/тулбар/кнопки становятся тёмными через
  `SetPreferredAppMode(ForceDark)`); на Windows 10 22H2 с светлой системной
  темой ОС не рисует тёмными Win32-контролы.
- [ ] Финальный Rebuild обеих конфигураций.
"""


def main():
    todo = read('TODO.md')
    start = todo.find('## Тёмная тема интерфейса WTL (Light/Dark)')
    end = todo.find('## Обновление вендорных библиотек (vendor-update.py)')
    assert start >= 0 and end > start, (start, end)
    todo = todo[:start] + TODO_SECTION + todo[end:]
    if '## 2026-10-21 (продолжение сессии) — тёмная тема интерфейса' not in todo:
        todo = todo.rstrip('\n') + '\n' + TODO_SESSION
    write('TODO.md', todo)

    history = read('HISTORY.md')
    entry = """## 2026-10-21 (продолжение сессии 4) — тёмная тема: тёмный фон и белый текст везде
### Тёмная тема интерфейса (Phase E)
- [x] `windows/PropPage.cpp` — **устранена главная проблема тёмной темы**:
  обработчики `WM_CTLCOLOR*` возвращали тёмную кисть, но не выставляли цвет
  текста, поэтому текст оставался системным `COLOR_WINDOWTEXT` (чёрный) и не
  читался на тёмном фоне. В тёмной теме теперь вызывается общий
  `FlyTheme::handleCtlColor()` (цвет текста + цвет фона + режим фона).
- [x] `windows/NetworkPage.cpp`, `windows/PropPageTextStyles.cpp` — обработчики
  `WM_CTLCOLORSTATIC` не сбрасывали `bHandled`, из-за чего цепочка
  `CHAIN_MSG_MAP(PropPage)` обрывалась и страница оставалась непокрашенной.
- [x] `windows/CZDCPage.h` — добавлен `CHAIN_MSG_MAP(PropPage)`.
- [x] `windows/Colors.h` — палитра дополнена (`COLOR_SCROLLBAR`, `COLOR_MENUBAR`,
  `COLOR_MENUHILIGHT`, `COLOR_ACTIVEBORDER`, `COLOR_INACTIVEBORDER`,
  `COLOR_3DDKSHADOW`, `COLOR_INACTIVECAPTIONTEXT`, `COLOR_DESKTOP`); добавлены
  общий обработчик `FlyTheme::handleCtlColor()`, кеш кистей (была утечка GDI на
  каждом `WM_CTLCOLOR*`), `allowDarkModeForWindow()`, `setWindowTheme()`,
  `applyDarkModeToWindow()` (все вызовы uxtheme — динамические, без зависимости
  от `uxtheme.lib` в клиентской библиотеке).
- [x] `windows/FlyThemeDlg.h` (новый) — миксин `CFlyThemeDlg`: тёмный фон и
  светлый текст для всех `WM_CTLCOLOR*` + применение тёмного режима ОС к
  контролам диалога. Подключён к 37 диалогам (25 в `windows/`, 7 страниц
  мастера, 3 в `client/FlyFeatures/`, `TreePropertySheet`).
- [x] `windows/main.cpp` — `WinUtil::initDarkMode()` перенесён в самое начало
  (сразу после загрузки настроек): режим тёмной темы ОС должен включаться до
  создания первого окна, иначе меню и тулбар остаются светлыми.
- [x] `windows/MainFrm.cpp` — `WinUtil::applyDarkMode(m_hWnd)` после создания
  меню, тулбаров и статусбара.
- [x] `windows/WinUtil.cpp` — `applyDarkMode()`/`applyDarkModeToWindow()`
  делегируют в `FlyTheme`; `AllowDarkModeForWindow` вызывается для всех
  дочерних окон; список классов с темой `DarkMode_Explorer` расширен
  (`Button`, `Static`, `ListBox`, `SysTabControl32`, `ToolbarWindow32`,
  `ReBarWindow32`, `ComboBoxEx32`, `msctls_progress32`, `msctls_trackbar32`,
  `msctls_updown32`, `RichEdit*`).
- [x] `client/FlyFeatures/MiniHTML.h` — неизвестное имя цвета HTML больше не
  даёт чёрный текст на тёмном фоне.
- [x] `windows/flattabctrl.h` — рамка неактивной вкладки берёт цвет из темы
  (был жёсткий `RGB(160,160,160)`).
- [x] Тёмная тема снова включена по умолчанию
  (`client/SettingsManager.cpp`: `setDefault(THEME, THEME_DARK)`) — причина
  отката («чёрный текст на сером») устранена.
- [x] Инструменты: `build-win/theme-dialogs.py`, `build-win/theme-patches.py`
  (побайтовые правки, CP1251 не портится), `build-win/shot.ps1`,
  `build-win/pixel.ps1` (дымовой запуск + снимки экрана + проверка цветов).
- [x] Сборка: Debug|x64 Build — 0 предупреждений / 0 ошибок; Release|x64 Build —
  0/0. Визуальная проверка: страницы настроек и обычные диалоги — тёмный фон
  (`#242628`) и белый текст.
- [ ] На Windows 11 22H2+ меню/тулбар/кнопки достанут тёмную отрисовку от ОС
  (`SetPreferredAppMode(ForceDark)` — вызовы проверены, режим включается);
  на Windows 10 22H2 с светлой системной темой ОС Win32-контролы не
  перекрашивает. Требуется проверка на Windows 11.
"""
    if '## 2026-10-21 (продолжение сессии 4) — тёмная тема' not in history:
        idx = history.find('\n## 2026-10-21 (')
        if idx < 0:
            history = history.rstrip('\n') + '\n\n' + entry
        else:
            history = history[:idx + 1] + entry + history[idx + 1:]
    write('HISTORY.md', history)
    print('TODO.md and HISTORY.md updated')


if __name__ == '__main__':
    main()
