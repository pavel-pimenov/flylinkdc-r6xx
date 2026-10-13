#ifndef MAXMINDDB_CONFIG_H
#define MAXMINDDB_CONFIG_H

#ifndef MMDB_UINT128_USING_MODE
/* Define as 1 if we use unsigned int __attribute__ ((__mode__(TI))) for uint128 values */
#define MMDB_UINT128_USING_MODE 0
#endif

#ifndef MMDB_UINT128_IS_BYTE_ARRAY
/* Define as 1 if we don't have an unsigned __int128 type */
#define MMDB_UINT128_IS_BYTE_ARRAY 1
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.14.1"
#endif

#endif                          /* MAXMINDDB_CONFIG_H */
