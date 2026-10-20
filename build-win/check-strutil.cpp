// check-strutil.cpp - a standalone regression test of the saturation logic of
// Util::stringToInt() (client/StrUtil.h).
//
// The code under test is copied verbatim from the header (the header itself
// cannot be included stand-alone: it depends on the whole client library).
//
// Build and run:
//   cl /nologo /EHsc /std:c++17 /Fe:check-strutil.exe check-strutil.cpp
//   check-strutil.exe
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <type_traits>
#include <limits>

static bool isWhiteSpace(int c)
{
	return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

template<typename int_type, typename char_type>
inline int_type stringToInt(const char_type* s)
{
	while (isWhiteSpace(*s)) s++;
	bool negative = false;
	if (std::is_signed<int_type>::value && *s == '-')
	{
		negative = true;
		s++;
	}
	else if (*s == '+')
		s++;
	// the value is accumulated in the unsigned domain: an overflow of a signed
	// type would be undefined behaviour, the result is saturated instead
	using utype = typename std::make_unsigned<int_type>::type;
	// the sign bit of the type: 0x80000000 for a 32 bit type
	const utype l_sign_bit = static_cast<utype>(~(utype)0) << (sizeof(utype) * 8 - 1);
	const utype l_max = std::is_signed<int_type>::value
	                    ? (negative ? l_sign_bit : (l_sign_bit - 1))
	                    : static_cast<utype>(~(utype)0);
	utype result = 0;
	while (*s >= '0' && *s <= '9')
	{
		const utype l_digit = static_cast<utype>(*s - '0');
		if (result > (l_max - l_digit) / 10)
		{
			result = l_max;
			s++;
			break;
		}
		result = result * 10 + l_digit;
		s++;
	}
	if (!std::is_signed<int_type>::value)
	{
		return static_cast<int_type>(result);
	}
	if (negative)
	{
		if (result == 0)
			return static_cast<int_type>(0);
		// the magnitude of the most negative value is one bigger than the
		// magnitude of the most positive one
		return static_cast<int_type>(-static_cast<int_type>(result - 1) - 1);
	}
	return static_cast<int_type>(result);
}

static int g_failed = 0;

static void check_int(const char* p_text, int p_expected)
{
	const int l_value = stringToInt<int, char>(p_text);
	if (l_value != p_expected)
	{
		printf("FAIL toInt(\"%s\") = %d, expected %d\n", p_text, l_value, p_expected);
		++g_failed;
	}
}

static void check_int64(const char* p_text, long long p_expected)
{
	const long long l_value = stringToInt<long long, char>(p_text);
	if (l_value != p_expected)
	{
		printf("FAIL toInt64(\"%s\") = %lld, expected %lld\n", p_text, l_value, p_expected);
		++g_failed;
	}
}

static void check_uint32(const char* p_text, unsigned long p_expected)
{
	const unsigned long l_value = stringToInt<unsigned long, char>(p_text);
	if (l_value != p_expected)
	{
		printf("FAIL toUInt32(\"%s\") = %lu, expected %lu\n", p_text, l_value, p_expected);
		++g_failed;
	}
}

int main()
{
	// the normal values must not change
	check_int("0", 0);
	check_int("1", 1);
	check_int("42", 42);
	check_int("-42", -42);
	check_int("+42", 42);
	check_int("  7", 7);
	check_int("", 0);
	check_int("-", 0);
	check_int("12abc", 12);
	check_int("2147483647", 2147483647);
	check_int("-2147483648", (int)(-2147483647 - 1));
	check_int64("9223372036854775807", 9223372036854775807LL);
	check_int64("-9223372036854775808", -9223372036854775807LL - 1);
	check_uint32("4294967295", 4294967295UL);

	// the values out of the range are saturated instead of overflowing
	check_int("2147483648", 2147483647);
	check_int("-2147483649", (int)(-2147483647 - 1));
	check_int("99999999999999999999", 2147483647);
	check_int64("99999999999999999999", 9223372036854775807LL);
	check_int64("-99999999999999999999", -9223372036854775807LL - 1);
	check_uint32("99999999999999999999", 4294967295UL);

	if (g_failed == 0)
	{
		printf("check-strutil: all checks passed\n");
		return 0;
	}
	printf("check-strutil: %d check(s) failed\n", g_failed);
	return 1;
}
