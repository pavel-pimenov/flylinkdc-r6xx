#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fix-util-toint.py - Util::toInt()/toInt64() used atoi()/_atoi64(): the value of a
string that is out of the range of the type was undefined behaviour (the overflow
of a signed type). The value is now saturated at the limits of the type.
"""
import io
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATH = os.path.join(REPO, 'client', 'Util.h')

OLD_INT64 = u"""		static int64_t toInt64(const char* aString)
		{
#ifdef _WIN32
			return _atoi64(aString);
#else
			return strtoll(aString, (char **)NULL, 10);
#endif
		}"""
NEW_INT64 = u"""		static int64_t toInt64(const char* aString)
		{
			return stringToInt<int64_t, char>(aString);
		}"""

OLD_INT64_W = u"""		static int64_t toInt64(const wchar_t* aString)
		{
#ifdef _WIN32
			return _wtoi64(aString);
#else
			// TODO return strtoll(aString, (char **)NULL, 10);
#endif
		}"""
NEW_INT64_W = u"""		static int64_t toInt64(const wchar_t* aString)
		{
			return stringToInt<int64_t, wchar_t>(aString);
		}"""

OLD_INT = u"""		static int toInt(const char* aString)
		{
			return atoi(aString);
		}"""
NEW_INT = u"""		static int toInt(const char* aString)
		{
			return stringToInt<int, char>(aString);
		}"""

OLD_INT_W = u"""		static int toInt(const wchar_t* aString)
		{
			return _wtoi(aString);
		}"""
NEW_INT_W = u"""		static int toInt(const wchar_t* aString)
		{
			return stringToInt<int, wchar_t>(aString);
		}"""

HELPER = u"""		// Parses a decimal number. The value is saturated at the limits of the type:
		// the overflow of a signed type would be undefined behaviour (atoi()).
		template<typename int_type, typename char_type>
		static int_type stringToInt(const char_type* p_text)
		{
			if (p_text == nullptr)
				return 0;
			while (isWhiteSpace(*p_text))
				p_text++;
			bool l_negative = false;
			if (std::is_signed<int_type>::value && *p_text == '-')
			{
				l_negative = true;
				p_text++;
			}
			else if (*p_text == '+')
				p_text++;
			using utype = typename std::make_unsigned<int_type>::type;
			// the sign bit of the type: 0x80000000 for a 32 bit type
			const utype l_sign_bit = static_cast<utype>(~(utype)0) << (sizeof(utype) * 8 - 1);
			const utype l_max = std::is_signed<int_type>::value
			                    ? (l_negative ? l_sign_bit : (l_sign_bit - 1))
			                    : static_cast<utype>(~(utype)0);
			utype l_result = 0;
			while (*p_text >= '0' && *p_text <= '9')
			{
				const utype l_digit = static_cast<utype>(*p_text - '0');
				if (l_result > (l_max - l_digit) / 10)
				{
					l_result = l_max;
					p_text++;
					break;
				}
				l_result = l_result * 10 + l_digit;
				p_text++;
			}
			if (!std::is_signed<int_type>::value)
				return static_cast<int_type>(l_result);
			if (l_negative)
			{
				if (l_result == 0)
					return static_cast<int_type>(0);
				// the magnitude of the most negative value is one bigger than the
				// magnitude of the most positive one
				return static_cast<int_type>(-static_cast<int_type>(l_result - 1) - 1);
			}
			return static_cast<int_type>(l_result);
		}
		
		static bool isWhiteSpace(int c)
		{
			return c == ' ' || c == '\\f' || c == '\\n' || c == '\\r' || c == '\\t' || c == '\\v';
		}
		
		static int64_t toInt64(const char* aString)"""


def main() -> int:
    with open(PATH, 'rb') as f:
        data = f.read()
    crlf = b'\r\n' in data
    text = data.replace(b'\r\n', b'\n').decode('utf-8', errors='surrogateescape')

    for old, new in ((OLD_INT64, NEW_INT64), (OLD_INT64_W, NEW_INT64_W),
                     (OLD_INT, NEW_INT), (OLD_INT_W, NEW_INT_W)):
        if old not in text:
            print('pattern not found:\n' + old)
            return 1
        text = text.replace(old, new, 1)

    # the helper is inserted before the first toInt64()
    anchor = u'\t\tstatic int64_t toInt64(const char* aString)'
    idx = text.find(anchor)
    if idx < 0:
        print('the anchor not found', file=sys.stderr)
        return 1
    text = text[:idx] + HELPER + text[idx + len(anchor):]

    if '#include <type_traits>' not in text:
        text = text.replace('#include <algorithm>', '#include <algorithm>\n#include <type_traits>', 1)

    out = text.encode('utf-8', errors='surrogateescape')
    if crlf:
        out = out.replace(b'\n', b'\r\n')
    with open(PATH, 'wb') as f:
        f.write(out)
    print('client/Util.h: toInt()/toInt64() saturate the value now')
    return 0


if __name__ == '__main__':
    sys.exit(main())
