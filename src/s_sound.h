// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief The not so system specific sound interface

#ifndef __S_SOUND__
#define __S_SOUND__

#include "sounds.h"

// mask used to indicate sound origin is player item pickup
#define PICKUP_SOUND 0x8000

extern consvar_t stereoreverse;
extern consvar_t cv_soundvolume, cv_digmusicvolume, cv_midimusicvolume;
extern consvar_t cv_numChannels;

#ifdef SNDSERV
extern consvar_t sndserver_cmd, sndserver_arg;
#endif
#ifdef MUSSERV
extern consvar_t musserver_cmd, musserver_arg;
#endif

extern CV_PossibleValue_t soundvolume_cons_t[];
//part of i_cdmus.c
extern consvar_t cd_volume, cdUpdate;
#ifdef LINUX
extern consvar_t cv_jigglecdvol;
#endif

#if defined (__MACOS__) && !defined(SDL)
typedef enum
{
	music_normal,
	playlist_random,
	playlist_normal
} playmode_t;

extern consvar_t play_mode;
#endif

typedef enum
{
	SF_TOTALLYSINGLE =  1,
	SF_MULTIPLESOUND =  2,
	SF_OUTSIDESOUND  =  4,
	SF_X4AWAYSOUND   =  8,
	SF_X8AWAYSOUND   = 16,
} soundflags_t;

// register sound vars and commands at game startup
void S_RegisterSoundStuff(void);

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume, allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int digMusicVolume, int midiMusicVolume);

//
// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
//
void S_StopSounds(void);
void S_Start(void);

//
// Basically a W_GetNumForName that adds "ds" at the beginning of the string. Returns a lumpnum.
//
int S_GetSfxLumpNum(sfxinfo_t* sfx);

//
// Start sound for thing at <origin> using <sound_id> from sounds.h
//
void S_StartSound(const void* origin, int sound_id);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(const void* origin, int sound_id, int volume);

// Stop sound for thing at <origin>
void S_StopSound(void* origin);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);

// Start music using <music_id> from sounds.h, and set whether looping
void S_ChangeMusic(int music_num, int looping);

// Stops the music.
void S_StopMusic(void);

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

//
// Updates music & sounds
//
void S_UpdateSounds(void);

void S_SetDigMusicVolume(int volume);
void S_SetMIDIMusicVolume(int volume);
void S_SetSfxVolume(int volume);

int S_SoundPlaying(void* origin, int id);
void S_StartSoundName(void* mo, char* soundname);

#endif
