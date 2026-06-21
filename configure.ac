#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.56])
AC_INIT(openkb, 0.0.3, https://sourceforge.net/p/openkb/tickets/)

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

: ${CFLAGS="-g"}

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Searches for *-config (SDL 2)
AC_PATH_PROG(SDL_CONFIG, sdl2-config)
#AC_PATH_PROG(PKG_CONFIG, pkg-config)

# Pull SDL2 cflags/libs up-front so subsequent link tests can see SDL2.
AS_IF([test -n "$SDL_CONFIG"], [
	LIBS="$LIBS `$SDL_CONFIG --libs`"
	CFLAGS="$CFLAGS `$SDL_CONFIG --cflags`"
	AC_DEFINE([HAVE_LIBSDL], [1], [Define to 1 if you have the SDL2 library])
], [AC_MSG_ERROR([cannot find `sdl2-config' tool, SDL2 is a mandatory dependancy])])

# Checks for libraries.
AC_CHECK_LIB([SDL2], [SDL_Init], [ ], [AC_MSG_ERROR([cannot find `SDL2' library, a mandatory dependancy])])
AC_CHECK_LIB([SDL2_image], [IMG_Load], [ ])
AC_CHECK_LIB([SDL2_net], [SDLNet_Init])
AC_CHECK_LIB([SDL2_mixer], [Mix_OpenAudio], [LIBS="$LIBS -lSDL2_mixer"])
AC_CHECK_LIB([png], [png_create_write_struct], [ ])

# Optional libraries.
AC_ARG_WITH([png], AS_HELP_STRING([--without-png], [Build without png library (default: test)]))
AC_ARG_WITH([sdl_image], AS_HELP_STRING([--without-sdl_image], [Build without SDL_image library (default: test)]))

# Enables/Disables features based on CHECK_LIB tests.
AS_IF([test "$ac_cv_lib_png_png_create_write_struct" = yes], [HAVE_LIBPNG=1], [HAVE_LIBPNG=0])
AS_IF([test "$ac_cv_lib_SDL2_image_IMG_Load" = yes], [HAVE_LIBSDL_IMAGE=1], [HAVE_LIBSDL_IMAGE=0])

# Enables/Disables features based on ARG_WITH variables.
AS_IF([test "$with_png" != no], [], [HAVE_LIBPNG=0])
AS_IF([test "$with_sdl_image" != no], [], [HAVE_LIBSDL_IMAGE=0])

# Exports defines.
#AC_DEFINE_UNQUOTED(HAVE_LIBPNG, [$HAVE_LIBPNG])
#AC_DEFINE_UNQUOTED(HAVE_LIBSDL_IMAGE, [$HAVE_LIBSDL_IMAGE])

# Add compiler flags
AS_IF([test "$HAVE_LIBPNG" = 1], [
	AC_DEFINE(HAVE_LIBPNG, [], "libpng is present")
	LIBS="$LIBS -lpng"
])
AS_IF([test "$HAVE_LIBSDL_IMAGE" = 1], [
	AC_DEFINE(HAVE_LIBSDL_IMAGE, [], "SDL_image is present")
	LIBS="$LIBS -lSDL2_image"
])

# Checks for header files.
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([endian.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([isascii mkdir strcasecmp strlcpy strlcat strdup getcwd])

AC_CONFIG_FILES([Makefile                 ])
#AC_CONFIG_SUBDIRS([vendor/scale2x-2.4])
AC_OUTPUT
