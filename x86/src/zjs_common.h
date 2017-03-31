// Copyright (c) 2016, Intel Corporation.

#ifndef __zjs_common_h__
#define __zjs_common_h__

// This file includes code common to both X86 and ARC

#include <stdio.h>

#define ZJS_PRINT printf

/**
 * Return a pointer to the filename portion of a string plus one parent dir
 *
 * @param filepath  A valid null-terminated string
 * @returns A pointer to a substring of filepath, namely the character right
 *   after the second to last slash, or filepath itself if not found.
 */
char *zjs_shorten_filepath(char *filepath);

#ifdef DEBUG_BUILD

int zjs_get_sec(void);
int zjs_get_ms(void);

#define DBG_PRINT \
    ZJS_PRINT("\n%u.%3.3u %s:%d %s():\n[INFO] ", zjs_get_sec(), zjs_get_ms(), zjs_shorten_filepath(__FILE__), __LINE__, __func__); \
    ZJS_PRINT

#define ERR_PRINT \
    ZJS_PRINT("\n%u.%3.3u %s:%d %s():\n[ERROR] ", zjs_get_sec(), zjs_get_ms(), zjs_shorten_filepath(__FILE__), __LINE__, __func__); \
    ZJS_PRINT

#else
#define DBG_PRINT(fmat ...) do {} while(0);
#define ERR_PRINT \
    ZJS_PRINT("\n%s:%d %s():\n[ERROR] ", zjs_shorten_filepath(__FILE__), __LINE__, __func__); \
    ZJS_PRINT
#endif

// TODO: We should instead have a macro that changes in debug vs. release build,
// to save string space and instead print error codes or something for release.

// this is arbitrary but okay for now; added to avoid plain strlen below
#define MAX_SCRIPT_SIZE 8192

#if defined(CONFIG_BOARD_ARDUINO_101) || defined(CONFIG_BOARD_ARDUINO_101_SSS)
#define ARC_AIO_MIN 9
#define ARC_AIO_MAX 14
// ARC_AIO_LEN = ARC_AIO_MAX - ARC_AIO_MIN + 1
#define ARC_AIO_LEN 6
#endif

#endif  // __zjs_common_h__
