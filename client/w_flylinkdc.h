#pragma once

#ifndef DCPLUSPLUS_DCPP_W_FLYLINKDC_H
#define DCPLUSPLUS_DCPP_W_FLYLINKDC_H

#include "version.h"

// https://msdn.microsoft.com/ru-ru/library/windows/desktop/aa383745%28v=vs.85%29.aspx
#if defined(_M_ARM) || defined (_M_ARM64)
# define _WIN32_WINNT _WIN32_WINNT_WINBLUE
#endif

#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0600 // Vista+  https://docs.microsoft.com/ru-ru/cpp/porting/modifying-winver-and-win32-winnt?view=vs-2017
#endif


#endif // DCPLUSPLUS_DCPP_COMPILER_FLYLINKDC_H
