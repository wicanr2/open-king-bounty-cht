/*
 *  kbauto.h -- Global "automatic" modules for your convinience.
 *  Copyright (C) 2011 Vitaly Driedfruit
 *
 *  This file is part of openkb.
 *
 *  openkb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openkb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openkb.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _OPENKB_LIBKB_AUTO
#define _OPENKB_LIBKB_AUTO

#include "kbconf.h"
#include "kbfile.h"
#include "kbdir.h"

extern void discover_modules(const char *path, KBconfig *conf);

extern void init_module(KBmodule *mod);
extern void init_modules(KBconfig *conf);
extern void stop_module(KBmodule *mod);
extern void stop_modules(KBconfig *conf);

extern void wipe_module(KBmodule *mod);

/* 程式化加入一個模組 (game.c 的 DOS/Genesis/Amiga 自動偵測、android bootstrap 用)。
 * 需顯式宣告:clang / mingw 把 implicit declaration 當錯誤 (gcc 只當警告)。 */
extern void add_module_aux(KBconfig *conf, const char *name, int family, int bpp,
                           const char *path, const char *slotA, const char *slotB, const char *slotC);

extern void register_module(KBconfig *conf, KBmodule *mod);
extern void register_modules(KBconfig *conf);

extern KB_DIR  * KB_opendir_with(const char *filename, KBmodule *mod);
extern KB_File * KB_fopen_with(const char *filename, char *mode, KBmodule *mod);
extern KB_File * KB_fcaseopen_with(const char *filename, char *mode, KBmodule *mod);

extern int match_file(const char *path, const char *filename, char *match);

#endif /* _OPENKB_LIBKB_AUTO */
