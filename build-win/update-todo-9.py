#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
update-todo-9.py - the saturation of Util::toInt()/toInt64() and the regression tests.
"""
import io
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATH = os.path.join(REPO, 'TODO.md')

TEXT = u"""
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
"""


def main() -> int:
    with io.open(PATH, 'r', encoding='utf-8', newline='') as f:
        text = f.read().replace('\r\n', '\n')
    if u'### Исправление насыщения при разборе чисел' in text:
        print('already present')
        return 0
    text = text.rstrip('\n') + '\n' + TEXT
    with io.open(PATH, 'w', encoding='utf-8', newline='') as f:
        f.write(text.replace('\n', '\r\n'))
    print('TODO.md updated')
    return 0


if __name__ == '__main__':
    sys.exit(main())
