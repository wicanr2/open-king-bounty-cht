/*
 *  free-snd.h -- WAV files reader/player interface
 *  Copyright (C) 2020 Vitaly Driedfruit
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
#ifndef _OPENKB_LIBKB_FREESOUND
#define _OPENKB_LIBKB_FREESOUND

#include "kbfile.h"

struct wavFile {

	struct wavHeader {

	} header;

	byte *bytes;
	unsigned int num_bytes;

	/* ----------------------------------------------------------- */
	int playhead;
};

extern struct wavFile* wavFile_load_FILE(KB_File *f, SDL_AudioSpec *spec);
extern int wavFile_play(struct wavFile *wav, Uint8 *stream, int len, int freq);
extern int wavFile_reset(struct wavFile *wav, Uint16 format);


#endif
