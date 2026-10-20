// check-tformat.cpp - a standalone check of the variadic Text::tformat()
// (client/Text.h) - the code under test is copied verbatim from the header.
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <wchar.h>

template<typename... T>
inline std::wstring tformat(const std::wstring& src, T... t)
{
	std::wstring ret(src.size() + 128, 0);
	int n = _snwprintf(&ret[0], ret.size(), src.c_str(), t...);
	if (n != -1 && n < static_cast<int>(ret.size()))
	{
		ret.resize(static_cast<std::wstring::size_type>(n));
	}
	return ret;
}

static int g_failed = 0;

static void check(const char* p_name, const std::wstring& p_result, const std::wstring& p_expected)
{
	if (p_result != p_expected)
	{
		printf("FAIL %s: got \"%s\", expected \"%s\"\n", p_name, p_result.c_str(), p_expected.c_str());
		++g_failed;
	}
}

int main()
{
	check("1 arg", tformat(L"%d items", 42), L"42 items");
	check("2 args", tformat(L"%d/%d", 3, 7), L"3/7");
	check("3 args", tformat(L"%s (%.01f%%) %s", L"12.5 MB", 3.5, L"done"), L"12.5 MB (3.5%) done");
	check("4 args", tformat(L"%u (%u/%u/%u)", 10u, 1u, 2u, 3u), L"10 (1/2/3)");
	// a truncated result must stay a valid (terminated) string
	{
		const std::wstring l_result = tformat(L"%s", std::wstring(500, L'x').c_str());
		if (l_result.size() != 130)
		{
			printf("FAIL truncated: size %d\n", (int)l_result.size());
			++g_failed;
		}
	}
	if (g_failed == 0)
	{
		printf("check-tformat: all checks passed\n");
		return 0;
	}
	printf("check-tformat: %d check(s) failed\n", g_failed);
	return 1;
}





