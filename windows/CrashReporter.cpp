/*
 * FlylinkDC++ crash reporting based on sentry-native (inproc backend).
 *
 * inproc-бэкенд sentry-native самостоятельно ставит SetUnhandledExceptionFilter
 * и при SEH-исключении снимает стек через dbghelp и кладёт событие в очередь
 * отправки (winhttp). Крэшпад/брейкпад не используются.
 */

#include "stdafx.h"

#include <signal.h>

// sentry-native собрана как статическая библиотека - см. sentry-native_2022.vcxproj.
#define SENTRY_BUILD_STATIC
#include <sentry.h>

#include "../client/Text.h"
#include "../client/Util.h"
#include "../client/version.h"

namespace CrashReporter
{

// DSN проекта FlylinkDC++ в Sentry.
// Пустая строка означает "отправка выключена": sentry_init вызывается, но без DSN
// события никуда не уходят.
// TODO(ppa): подставить реальный DSN.
static const char* const g_sentry_dsn_default = "";

static bool g_initialized = false;

static void captureFatal(const char* p_type, const char* p_message)
{
	if (!g_initialized)
	{
		return;
	}
	sentry_value_t l_event = sentry_value_new_event();
	sentry_value_set_by_key(l_event, "level", sentry_value_new_string("fatal"));
	sentry_value_t l_exception = sentry_value_new_exception(p_type, p_message);
	void* l_frames[128];
	size_t l_frame_count = sentry_unwind_stack(NULL, l_frames, 128);
	sentry_value_t l_stacktrace = sentry_value_new_stacktrace(l_frames, l_frame_count);
	sentry_value_set_by_key(l_exception, "stacktrace", l_stacktrace);
	sentry_event_add_exception(l_event, l_exception);
	sentry_capture_event(l_event);
	sentry_flush(2000);
}

static void onTerminate()
{
	captureFatal("abort", "C++ std::terminate() called (unhandled exception)");
	abort();
}

static void onPureCall()
{
	captureFatal("abort", "Pure virtual function call");
	abort();
}

static void onInvalidParameter(const wchar_t* /*p_expression*/, const wchar_t* /*p_function*/, const wchar_t* /*p_file*/, unsigned int /*p_line*/, uintptr_t /*p_reserved*/)
{
	captureFatal("abort", "Invalid parameter to CRT function");
	abort();
}

void init()
{
	if (g_initialized)
	{
		return;
	}

	// Значение копируем в std::string, чтобы компилятор не вырезал код Sentry
	// как недостижимый при пустом DSN (константная свёртка).
	std::string l_dsn = g_sentry_dsn_default;

	sentry_options_t* l_options = sentry_options_new();
	sentry_options_set_dsn(l_options, l_dsn.c_str());
	sentry_options_set_release(l_options, A_VERSIONSTRING);
#ifdef _DEBUG
	sentry_options_set_environment(l_options, "debug");
#else
	sentry_options_set_environment(l_options, "production");
#endif
	sentry_options_set_debug(l_options, 0);
	sentry_options_set_symbolize_stacktraces(l_options, 1);

	const tstring l_db_path = Text::toT(Util::getConfigPath()) + _T("\\sentry-native");
	sentry_options_set_database_pathw(l_options, l_db_path.c_str());

	sentry_init(l_options);
	sentry_set_tag("dcpp.version", DCVERSIONSTRING);

	std::set_terminate(onTerminate);
	_set_purecall_handler(onPureCall);
	_set_invalid_parameter_handler(onInvalidParameter);

	g_initialized = true;
}

void shutdown()
{
	if (!g_initialized)
	{
		return;
	}
	g_initialized = false;
	sentry_close();
}

void addBreadcrumb(const char* p_category, const char* p_message)
{
	if (!g_initialized)
	{
		return;
	}
	sentry_value_t l_breadcrumb = sentry_value_new_breadcrumb(NULL, p_message);
	if (p_category != NULL)
	{
		sentry_value_set_by_key(l_breadcrumb, "category", sentry_value_new_string(p_category));
	}
	sentry_add_breadcrumb(l_breadcrumb);
}

} // namespace CrashReporter
