/*
 *  free-snd.c -- WAV reader/player
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

#include "kbsys.h"
#include "kbfile.h"
#include "kbres.h"

#include "kbsound.h"

#include "free-snd.h"

/* 前向宣告:wavFile_load_FILE 在它定義 (本檔下方) 之前就呼叫它;
 * clang/mingw 對 implicit declaration 報錯 (gcc 僅警告)。 */
int wavFile_read_FILE(struct wavFile *wav, KB_File *f, SDL_AudioSpec *wav_obtained);

int wavFile_reset(struct wavFile *tun, Uint16 format) {

	tun->playhead = 0;

	return 0;
}

struct wavFile* wavFile_load_FILE(KB_File *f, SDL_AudioSpec *spec) {
	int i;

	struct wavFile *wav;

	wav = malloc(sizeof(struct wavFile));

	if (wav == NULL) return NULL;

	i = wavFile_read_FILE(wav, f, spec);

	if (i) {
		free(wav);
		return NULL;
	}

	return wav;
}

int wavFile_read_FILE(struct wavFile *wav, KB_File *f, SDL_AudioSpec *wav_obtained) {

	SDL_AudioSpec wav_spec;
	SDL_AudioCVT wav_cvt;

	SDL_RWops* rw = KBRW_open(f);


	/* Load the WAV */
	if (SDL_LoadWAV_RW(rw, 1, &wav_spec, &wav->bytes, &wav->num_bytes) == NULL)
	{
//		KB_debuglog(0, "Could not open %s: %s", buf, SDL_GetError());
//		return 1;
	}

	/* Build the audio converter */
	if (SDL_BuildAudioCVT(&wav_cvt, wav_spec.format, wav_spec.channels, wav_spec.freq,
		wav_obtained->format, wav_obtained->channels, wav_obtained->freq) < 0)
	{
		KB_debuglog(0, "Could not build audio converter: %s", SDL_GetError());
		return 1;
	}

	/* Allocate a buffer for the audio converter */
	wav_cvt.buf = malloc(wav->num_bytes * wav_cvt.len_mult);
	wav_cvt.len = wav->num_bytes;
	memcpy(wav_cvt.buf, wav->bytes, wav->num_bytes);

	/* Convert audio data to correct format */
	if (SDL_ConvertAudio(&wav_cvt) != 0)
	{
		KB_debuglog(0, "Could not convert audio file: %s", SDL_GetError());
		return 1;
	}

	/* Free the WAV */
	SDL_FreeWAV(wav->bytes);

	/* Allocate a buffer for the audio data */
	wav->bytes = malloc(wav_cvt.len_cvt);
	memcpy(wav->bytes, wav_cvt.buf, wav_cvt.len_cvt);
	free(wav_cvt.buf);
	wav->num_bytes = wav_cvt.len_cvt;

	return 0;
}


/*
 */
int wavFile_play(struct wavFile *wav, Uint8 *stream, int len, int freq) {
	/* Paranoia */
	Uint32 tocopy = ((wav->num_bytes - wav->playhead > len) ? len: wav->num_bytes - wav->playhead);

	/* Copy data to audio buffer */
	memcpy(stream, wav->bytes + wav->playhead, tocopy);

	/* Advance */
	wav->playhead += tocopy;

	return tocopy;
};

