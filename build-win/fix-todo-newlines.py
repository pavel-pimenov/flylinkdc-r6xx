#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fix-todo-newlines.py - restore the line breaks in TODO.md.

The file was written without any line separator, which makes it unreadable.
Run once from the repository root:
    python build-win/fix-todo-newlines.py
"""
import io
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATH = os.path.join(REPO, 'TODO.md')

HEADINGS = [
    '## Интеграция Sentry (crash reporting)',
    '## AddressSanitizer (нагрузочное тестирование)',
    '## Тёмная тема интерфейса WTL (Light/Dark)',
    '## Обновление вендорных библиотек (vendor-update.py)',
    '## Сборка (правила — только Docker, см. AGENTS.md)',
    '## Аудит апгрейдов вендоров (2026-10-13)',
    '## Рефакторинг client (2026-10-15)',
    '## История / статус прошлых сессий',
    '## 2026-10-16 (продолжение сессии)',
]


def main() -> int:
    with io.open(PATH, 'r', encoding='utf-8', newline='') as f:
        text = f.read()
    if '\n' in text:
        print('TODO.md already contains line breaks - nothing to do')
        return 0

    text = text.replace('## ', '\r\n## ')
    text = text.replace('# TODO', '\r\n# TODO')
    text = text.replace('- [', '\r\n- [')

    for heading in HEADINGS:
        text = text.replace(heading, heading + '\r\n')

    text = text.replace('# TODO\r\nВсе задачи', '# TODO\r\n\r\nВсе задачи')

    text = re.sub(r'(\r\n){3,}', '\r\n\r\n', text)
    text = re.sub(r'[ \t]+\r\n', '\r\n', text)
    text = text.lstrip('\r\n') + '\r\n'

    with io.open(PATH, 'w', encoding='utf-8', newline='') as f:
        f.write(text)
    print('TODO.md: %d lines, %d bytes' % (text.count('\n'), len(text.encode('utf-8'))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
