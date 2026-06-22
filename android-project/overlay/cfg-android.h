/* Android 手寫 config.h (取代 autotools configure 產物) */
#ifndef OPENKB_CONFIG_H_ANDROID
#define OPENKB_CONFIG_H_ANDROID
#define PACKAGE "openkb"
#define PACKAGE_NAME "openkb"
#define PACKAGE_VERSION "0.0.3"
#define VERSION "0.0.3"
#define STDC_HEADERS 1
#define HAVE_STDLIB_H 1
#define HAVE_MALLOC 1
/* 不定義 HAVE_MALLOC_H → kbstd.h 走 <stdlib.h> (bionic 兩者皆有,stdlib 較可攜) */
#define HAVE_LIBSDL 1
#define HAVE_LIBSDL_IMAGE 1
#define HAVE_ISASCII 1
#define HAVE_MKDIR 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_GETCWD 1
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1
#endif
