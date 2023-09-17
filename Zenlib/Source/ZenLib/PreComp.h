/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Helpers for compilers (precompilation)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenLib_PreCompH
#define ZenLib_PreCompH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(_MSC_VER) || defined(__BORLANDC__)
//[-] FlylinkDC++ Team 
/*
    #include <cstring>
    #include <cstdio>
    #include <cstdlib>
    #include <ctime>
    #include <algorithm>
    #include <map>
    #include <sstream>
    #include <iomanip>
    #include <cmath>
*/
    #undef __TEXT
    #include <windows.h>
    #include "ZenLib/Conf.h"
    #include "ZenLib/Conf_Internal.h"
    #include "ZenLib/Ztring.h"
    #include "ZenLib/ZtringList.h"
    #include "ZenLib/File.h"
    #include "ZenLib/OS_Utils.h"

#endif //defined(_MSC_VER) || defined(__BORLANDC__)
//---------------------------------------------------------------------------

#endif
