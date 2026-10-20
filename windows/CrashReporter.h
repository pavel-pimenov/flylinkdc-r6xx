/*
 * FlylinkDC++ crash reporting based on sentry-native (inproc backend).
 */

#pragma once

namespace CrashReporter
{
// Инициализирует Sentry и подписывается на фатальные ситуации (SEH через
// inproc-бэкенд sentry-native, а также terminate/purecall/invalid-parameter).
// Безопасно вызывать повторно, лишние вызовы игнорируются.
void init();

// Сбрасывает очередь отправки и закрывает Sentry. Вызывать при выходе.
void shutdown();

// Отправляет breadcrumb (используется для контекста перед падением).
void addBreadcrumb(const char* p_category, const char* p_message);
} // namespace CrashReporter
