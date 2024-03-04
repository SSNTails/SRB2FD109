// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sound.c,v 1.12 2004/04/18 12:53:42 hurdler Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log: i_sound.c,v $
// Revision 1.12  2004/04/18 12:53:42  hurdler
// fix Heretic issue with SDL and OS/2
//
// Revision 1.11  2003/07/13 13:16:15  hurdler
// go RC1
//
// Revision 1.10  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.9  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.8  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.7  2001/04/14 14:15:14  metzgermeister
// fixed bug no sound device
//
// Revision 1.6  2001/04/09 20:21:56  metzgermeister
// dummy for I_FreeSfx
//
// Revision 1.5  2001/03/25 18:11:24  metzgermeister
//   * SDL sound bug with swapped stereo channels fixed
//   * separate hw_trick.c now for HW_correctSWTrick(.)
//
// Revision 1.4  2001/03/09 21:53:56  metzgermeister
// *** empty log message ***
//
// Revision 1.3  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//-----------------------------------------------------------------------------
/// \file
/// \brief SDL interface for sound

#include <math.h>
#include "../doomtype.h"

#include <SDL/SDL.h>
#ifdef HAVE_MIXER
#ifdef __APPLE_CC__
#include <SDL_mixer.h> // MacOSX's SDL_mixer
#else
#include <SDL/SDL_mixer.h>
#endif
#endif

#if (defined (_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)
#include <direct.h>
#elif defined (__GNUC__)
#include <unistd.h>
#endif
#include "../z_zone.h"

#include "../m_swap.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../screen.h" //vid.WndParent
#include "../doomdef.h"
#include "../doomstat.h"
#include "../s_sound.h"

#include "../d_main.h"

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"
#include "../hardware/hw3sound.h"
#include "hwsym_sdl.h"
#endif

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Needed for calling the actual sound output.
#ifdef _WIN32_WCE
#define NUM_CHANNELS            8
#else
#define NUM_CHANNELS            32
#endif

#define INDEXOFSFX(x) ((char *)x - (char *)S_sfx) / sizeof(sfxinfo_t)

#ifdef _WIN32_WCE
static Uint16 samplecount = 512;
#else
static Uint16 samplecount = 1024; //Alam: 1KB samplecount at 22050hz is 46.439909297052154195011337868481ms of buffer
#endif

static Uint32 lengths[NUMSFX];     // The actual lengths of all sound effects.

typedef struct chan_struct
{
	// The channel data pointers, start and end.
	Uint8 *data; //static unsigned char *channels[NUM_CHANNELS];
	Uint8 *end; //static unsigned char *channelsend[NUM_CHANNELS];

	// pitch
	Uint32 realstep; // The channel step amount...
	Uint32 step;          //static unsigned int channelstep[NUM_CHANNELS];
	Uint32 stepremainder; //static unsigned int channelstepremainder[NUM_CHANNELS];
	Uint32 samplerate; // ... and a 0.16 bit remainder of last step.

	// Time/gametic that the channel started playing,
	//  used to determine oldest, which automatically
	//  has lowest priority.
	tic_t starttic; //static int channelstart[NUM_CHANNELS];

	// The sound handle, determined on registration,
	//  used to unregister/stop/modify,
	int handle; //static int channelhandles[NUM_CHANNELS];

	// SFX id of the playing sound effect.
	void *id; // Used to catch duplicates (like chainsaw).
	int sfxid; //static int channelids[NUM_CHANNELS];
	int vol; //the channel volume
	int sep; //the channel pan

	// Hardware left and right channel volume lookup.
	Sint16* leftvol_lookup; //static int *channelleftvol_lookup[NUM_CHANNELS];
	Sint16* rightvol_lookup; //static int *channelrightvol_lookup[NUM_CHANNELS];
} chan_t;

static chan_t channels[NUM_CHANNELS];

// Pitch to stepping lookup
static int steptable[256];

// Volume lookups.
static Sint16 vol_lookup[128 * 256];

byte sound_started = false;
static SDL_mutex *Snd_Mutex = NULL;

//SDL's Audio
static SDL_AudioSpec audio;

static SDL_bool musicStarted = SDL_FALSE;
#ifdef HAVE_MIXER
static SDL_mutex *Msc_Mutex = NULL;
/* FIXME: Make this file instance-specific */
#define MIDI_TMPFILE    "srb2music"
#define MIDI_TMPFILE2    "srb2wav"
static int musicvol = 62;

static long double loopstartDig = 0.0L; // warning: use of `long double' type; its size may change in a future release
static boolean loopingDig = false;
static void I_FinishMusic(void);

#if (MIX_MAJOR_VERSION > 1) || (MIX_MINOR_VERSION >= 3) || (MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL >= 4)
#define MIXER_POS //Mix_GetMusicType and Mix_FadeInMusicPos in 1.2.4+
#else
#define Mix_FadeInMusicPos(music,loops,ms,position) Mix_FadeInMusic(music,loops,ms)
#endif

static const int MIDIfade = 500;
static const int Digfade = 0;

static Mix_Music *music[2] = { NULL, NULL };
#endif

static inline void Snd_LockAudio(void) //Alam: Lock audio data and uninstall audio callback
{
	if(Snd_Mutex) SDL_LockMutex(Snd_Mutex);
	else if(nosound) return;
	else if(nomusic && nofmod
#ifdef HW3SOUND
			&& hws_mode == HWS_DEFAULT_MODE
#endif
			) SDL_LockAudio();
#ifdef HAVE_MIXER
	else if(musicStarted) Mix_SetPostMix(NULL, NULL);
#endif
}

static inline void Snd_UnlockAudio(void) //Alam: Unlock audio data and reinstall audio callback
{
	if(Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
	else if(nosound) return;
	else if(nomusic && nofmod
#ifdef HW3SOUND
			&& hws_mode == HWS_DEFAULT_MODE
#endif
			) SDL_UnlockAudio();
#ifdef HAVE_MIXER
	else if(musicStarted) Mix_SetPostMix(audio.callback, audio.userdata);
#endif
}

static inline SDL_bool Snd_Convert(Uint16 sr)
{
	return (sr > audio.freq) || (sr % 11025); // more samples then needed or odd samplerate
}

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
static void *getsfx(int sfxlump, Uint32 *len)
{
	Uint8 *sfx, *paddedsfx = NULL;
	Uint16 sr;
	Uint32 i, size = *len, paddedsize = 0;
	SDL_AudioCVT sfxcvt;

	sfx = (Uint8 *) W_CacheLumpNum(sfxlump, PU_STATIC);
	sr = (Uint16)((sfx[3]<<8)+sfx[2]);

	if(Snd_Convert(sr) && SDL_BuildAudioCVT(&sfxcvt, AUDIO_U8, 1, sr, AUDIO_U8, 1, audio.freq))
	{//Alam: Setup the AudioCVT for the SFX

		sfxcvt.len = size-8; //Alam: Chop off the header
		sfxcvt.buf = malloc(sfxcvt.len * sfxcvt.len_mult); //Alam: make room
		if(sfxcvt.buf) memcpy(sfxcvt.buf, sfx+8, sfxcvt.len); //Alam: copy the sfx sample

		if(sfxcvt.buf && SDL_ConvertAudio(&sfxcvt) == 0) //Alam: let convert it!
		{
				Uint16 *sfxfq = NULL;
				size = sfxcvt.len_cvt + 8;

				// Pads the sound effect out to the mixing buffer size.
				// The original realloc would interfere with zone memory.
				paddedsize = ((sfxcvt.len_cvt + (samplecount - 1)) / samplecount) * samplecount;

				// Allocate from zone memory.
				paddedsfx = (Uint8 *) Z_Malloc(paddedsize + 8, PU_STATIC, 0);
				sfxfq = (Uint16 *)paddedsfx+1;
				// This should interfere with zone memory handling,
				//  which does not kick in in the soundserver.

				// Now copy and pad.
				memcpy(paddedsfx+8, sfxcvt.buf, sfxcvt.len_cvt);
				free(sfxcvt.buf);
				memcpy(paddedsfx,sfx,8);
				*sfxfq = SHORT((Uint16)audio.freq); // new freq
		}
		else //Alam: the convert failed, not needed or i couldn't malloc the buf
		{
			if(sfxcvt.buf) free(sfxcvt.buf);
			// Pads the sound effect out to the mixing buffer size.
			// The original realloc would interfere with zone memory.
			paddedsize = ((size - 8 + (samplecount - 1)) / samplecount) * samplecount;

			// Allocate from zone memory.
			paddedsfx = (Uint8 *) Z_Malloc(paddedsize + 8, PU_STATIC, 0);
			// This should interfere with zone memory handling,
			//  which does not kick in in the soundserver.

			// Now copy and pad.
			memcpy(paddedsfx, sfx, size);
		}
	}
	else
	{
		// Pads the sound effect out to the mixing buffer size.
		// The original realloc would interfere with zone memory.
		paddedsize = ((size - 8 + (samplecount - 1)) / samplecount) * samplecount;

		// Allocate from zone memory.
		paddedsfx = (Uint8 *) Z_Malloc(paddedsize + 8, PU_STATIC, 0);
		// This should interfere with zone memory handling,
		//  which does not kick in in the soundserver.

		// Now copy and pad.
		memcpy(paddedsfx, sfx, size);
	}

	for(i = size; i < paddedsize + 8; i++)
		paddedsfx[i] = 128;

	// Remove the cached lump.
	Z_Free(sfx);

	// Preserve padded length.
	*len = paddedsize;

	// Return allocated padded data.
	return paddedsfx;
}

// used to (re)calculate channel params based on vol, sep, pitch
static void I_SetChannelParams(chan_t *c, int vol, int sep, int step)
{
	int leftvol;
	int rightvol;
	c->vol = vol;
	c->sep = sep;
	c->step = c->realstep = step;

	if(step != steptable[128])
		c->step += (((c->samplerate<<16)/audio.freq)-65536);
	else if(c->samplerate != (unsigned)audio.freq)
		c->step = ((c->samplerate<<16)/audio.freq);
	// x^2 separation, that is, orientation/stereo.
	//  range is: 0 (left) - 255 (right)

	// Volume arrives in range 0..255 and it must be in 0..cv_soundvolume...
	vol = (vol * cv_soundvolume.value) >> 7;
	// note: >> 6 would use almost the entire dynamical range, but
	// then there would be no "dynamical room" for other sounds :-/

	leftvol  = vol - ((vol*sep*sep) >> 16); ///(256*256);
	sep = 255 - sep;
	rightvol = vol - ((vol*sep*sep) >> 16);

	// Sanity check, clamp volume.
	if(rightvol < 0 || rightvol > 127)
	{
		rightvol = 63;
		//I_Error("rightvol out of bounds");
	}

	if(leftvol < 0 || leftvol > 127)
	{
		leftvol = 63;
		//I_Error("leftvol out of bounds");
	}

	// Get the proper lookup table piece
	//  for this volume level
	c->leftvol_lookup = &vol_lookup[leftvol*256];
	c->rightvol_lookup = &vol_lookup[rightvol*256];
}

static int FindChannel(int handle)
{
	int i;

	for (i = 0; i < NUM_CHANNELS; i++)
		if(channels[i].handle == handle)
			return i;

	// not found
	return -1;
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
static int addsfx(int sfxid, int volume, int step, int seperation)
{
	static unsigned short handlenums = 0;
	int i, slot, oldestnum = 0;
	tic_t oldest = gametic;

	// Play these sound effects only one at a time.
#if 1
	if(sfxid == sfx_stnmov
#if 0
	|| sfxid == sfx_sawup || sfxid == sfx_sawidl || sfxid == sfx_sawful || sfxid == sfx_sawhit || sfxid == sfx_pistol
#else
	|| ( sfx_litng1 <= sfxid && sfxid <= sfx_rainin) || sfxid == sfx_trfire || sfxid == sfx_alarm || sfxid == sfx_spin
#endif
	 )
	{
		// Loop all channels, check.
		for (i = 0; i < NUM_CHANNELS; i++)
		{
			// Active, and using the same SFX?
			if((channels[i].end) && (channels[i].sfxid == sfxid))
			{
				// Reset.
				channels[i].end = NULL;
				// We are sure that iff,
				//  there will only be one.
				break;
			}
		}
	}
#endif

	// Loop all channels to find oldest SFX.
	for (i = 0; (i < NUM_CHANNELS) && (channels[i].end); i++)
	{
		if(channels[i].starttic < oldest)
		{
			oldestnum = i;
			oldest = channels[i].starttic;
		}
	}

	// Tales from the cryptic.
	// If we found a channel, fine.
	// If not, we simply overwrite the first one, 0.
	// Probably only happens at startup.
	if(i == NUM_CHANNELS)
		slot = oldestnum;
	else
		slot = i;

	channels[slot].end = NULL;
	// Okay, in the less recent channel,
	//  we will handle the new SFX.
	// Set pointer to raw data.
	channels[slot].data = (Uint8 *)S_sfx[sfxid].data;
	channels[slot].samplerate = (channels[slot].data[3]<<8)+channels[slot].data[2];
	channels[slot].data += 8; //Alam: offset of the sound header

	for(;FindChannel(handlenums)!=-1;)
	{
		handlenums++;
		// Reset current handle number, limited to 0..65535.
		if(handlenums == (unsigned short)-1)
			handlenums = 0;
	}

	// Assign current handle number.
	// Preserved so sounds could be stopped.
	channels[slot].handle = handlenums;

	// Restart steper
	channels[slot].stepremainder = 0;
	// Should be gametic, I presume.
	channels[slot].starttic = gametic;

	I_SetChannelParams(&channels[slot], volume, seperation, step);

	// Preserve sound SFX id,
	//  e.g. for avoiding duplicates of chainsaw.
	channels[slot].id = S_sfx[sfxid].data;

	channels[slot].sfxid = sfxid;

	// Set pointer to end of raw data.
	channels[slot].end = channels[slot].data + lengths[sfxid];


	// You tell me.
	return handlenums;
}

//
// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//
// Well... To keep compatibility with legacy doom, I have to call this in
// I_InitSound since it is not called in S_Init... (emanne@absysteme.fr)

static inline void I_SetChannels(void)
{
	// Init internal lookups (raw data, mixing buffer, channels).
	// This function sets up internal lookups used during
	//  the mixing process.
	int i;
	int j;

	int *steptablemid = steptable + 128;

	if(nosound)
		return;

	// This table provides step widths for pitch parameters.
	for (i = -128; i < 128; i++)
		steptablemid[i] = (int)(pow(2.0, (i / 64.0)) * 65536.0);

	// Generates volume lookup tables
	//  which also turn the unsigned samples
	//  into signed samples.
	for (i = 0; i < 128; i++)
		for (j = 0; j < 256; j++)
		{
			//From PrDoom
			// proff - made this a little bit softer, because with
			// full volume the sound clipped badly
			vol_lookup[i * 256 + j] = (Sint16)((i * (j - 128) * 256) / 191);
			//Alam: hmm, lighting = !@#?
			//vol_lookup[i * 256 + j] = (Sint16)((i * (j - 128) * 256) / 127);
		}
}

void I_SetSfxVolume(int volume)
{
	int i = volume;

	//Snd_LockAudio();

	for (i = 0; i < NUM_CHANNELS; i++)
		if(channels[i].end) I_SetChannelParams(&channels[i], channels[i].vol, channels[i].sep, channels[i].realstep);

	//Snd_UnlockAudio();
}

void *I_GetSfx(sfxinfo_t* sfx)
{
	if(sfx->lumpnum < 0)
		sfx->lumpnum = S_GetSfxLumpNum(sfx);
//	else if(sfx->lumpnum != S_GetSfxLumpNum(sfx))
//		I_FreeSfx(sfx);

#ifdef HW3SOUND
	if(hws_mode != HWS_DEFAULT_MODE)
		return HW3S_GetSfx(sfx);
#endif

#if 0
	if(sfx->link)
	{
		sfx->link->data = I_GetSfx(sfx->link); //Alam: Look for real data
		lengths[INDEXOFSFX(sfx)] = lengths[INDEXOFSFX(sfx->link)]; //Alam: length from link
		sfx->lumpnum = sfx->link->lumpnum;
		return sfx->link->data;
	}
#endif

	if(sfx->data)
		return sfx->data; //Alam: I have it done!

	lengths[INDEXOFSFX(sfx)] = W_LumpLength (sfx->lumpnum);

	return getsfx(sfx->lumpnum, &lengths[INDEXOFSFX(sfx)]);

}

void I_FreeSfx(sfxinfo_t * sfx)
{
//	if(sfx->lumpnum<0)
//		return;

#ifdef HW3SOUND
	if(hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_FreeSfx(sfx);
	}
	else
#endif
	{
		int i;

		for (i = 1; i < NUMSFX; i++)
		{
			// Alias? Example is the chaingun sound linked to pistol.
			if(S_sfx[i].data == sfx->data)
			{
				if(&S_sfx[i] != sfx) S_sfx[i].data = NULL;
				S_sfx[i].lumpnum = -1;
				lengths[i] = 0;
			}
		}
		//Snd_LockAudio(); //Alam: too much?
		// Loop all channels, check.
		for (i = 0; i < NUM_CHANNELS; i++)
		{
			// Active, and using the same SFX?
			if(channels[i].end && channels[i].id == sfx->data)
			{
				channels[i].end = NULL; // Reset.
			}
		}
		//Snd_UnlockAudio(); //Alam: too much?
		if(sfx->data) Z_Free(sfx->data);
	}
	sfx->data = NULL;
	sfx->lumpnum = -1;
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int I_StartSound(int id, int vol, int sep, int pitch, int priority)
{
	if(nosound)
		return false;

	if(S_sfx[id].data == NULL) return -1;
	priority = 128;
	pitch = priority;

	Snd_LockAudio();
	id = addsfx(id, vol, steptable[pitch], sep);
	Snd_UnlockAudio();

	return id; // Returns a handle (not used).
}

void I_StopSound(int handle)
{
	// You need the handle returned by StartSound.
	// Would be looping all channels,
	//  tracking down the handle,
	//  an setting the channel to zero.
	int i;

	i = FindChannel(handle);

	if(i != -1)
	{
		//Snd_LockAudio(); //Alam: too much?
		channels[i].end = NULL;
		//Snd_UnlockAudio(); //Alam: too much?
		channels[i].handle = -1;
		channels[i].starttic = 0;
	}

}

int I_SoundIsPlaying(int handle)
{
	int isplaying = false, chan;
	chan = FindChannel(handle);
	if(chan != -1)
		isplaying = (channels[chan].end != NULL);
	return isplaying;
}

static void I_UpdateStream(void *userdata, Uint8 *stream, int len)
{
	// Mix current sound data.
	// Data, from raw sound
	register Sint32 dr; // Right 16bit stream
	register Uint8 sample; // Center 8bit sfx
	register Sint32 dl; // Left 16bit stream

	// Pointers in audio stream
	Sint16 *rightout = (Sint16 *) stream; // currect right
	Sint16 *leftout = rightout + 1;// currect left
	const Uint8 step = 2; // Step in stream, left and right, thus two.

	int chan; // Mixing channel index.

	if(!sound_started || !userdata)
		return;

	// Determine end of the stream
	len /= 4; // not 8bit mono samples, 16bit stereo ones

	if(Snd_Mutex) SDL_LockMutex(Snd_Mutex);

	// Mix sounds into the mixing buffer.
	// Loop over len
	while (len--)
	{
		// Reset left/right value.
		dl = *leftout;
		dr = *rightout;

		// Love thy L2 chache - made this a loop.
		// Now more channels could be set at compile time
		//  as well. Thus loop those channels.
		for (chan = 0; chan < NUM_CHANNELS; chan++)
		{
			// Check channel, if active.
			if(channels[chan].end)
			{
#if 1
				// Get the raw data from the channel.
				sample = channels[chan].data[0];
#else
				// linear filtering from PrDoom
				sample = (((Uint32)channels[chan].data[0] *(0x10000 - channels[chan].stepremainder))
					+ ((Uint32)channels[chan].data[1]) * (channels[chan].stepremainder))) >> 16;
#endif
				// Add left and right part
				//  for this channel (sound)
				//  to the current data.
				// Adjust volume accordingly.
				dl += channels[chan].leftvol_lookup[sample];
				dr += channels[chan].rightvol_lookup[sample];
				// Increment stepage
				channels[chan].stepremainder += channels[chan].step;
				// Check whether we are done.
				if(channels[chan].data + (channels[chan].stepremainder >> 16) >= channels[chan].end)
					channels[chan].end = NULL;
				else
				{
					// step to next sample
					channels[chan].data += (channels[chan].stepremainder >> 16);
					// Limit to LSB???
					channels[chan].stepremainder &= 0xffff;
				}
			}
		}

		// Clamp to range. Left hardware channel.
		// Has been char instead of short.

		if(dl > 0x7fff)
			*leftout = 0x7fff;
		else if(dl < -0x8000)
			*leftout = -0x8000;
		else
			*leftout = (Sint16)dl;

		// Same for right hardware channel.
		if(dr > 0x7fff)
			*rightout = 0x7fff;
		else if(dr < -0x8000)
			*rightout = -0x8000;
		else
			*rightout = (Sint16)dr;

		// Increment current pointers in stream
		leftout += step;
		rightout += step;

	}
	if(Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
	// Would be using the handle to identify
	//  on which channel the sound might be active,
	//  and resetting the channel parameters.

	int i = FindChannel(handle);
	pitch = 128;

	if(i != -1 && channels[i].end)
	{
		//Snd_LockAudio(); //Alam: too much?
		I_SetChannelParams(&channels[i], vol, sep, steptable[pitch]);
		//Snd_UnlockAudio(); //Alam: too much?
	}

}


#ifdef HW3SOUND

static void *soundso = NULL;

static int Init3DSDriver(const char *soName)
{
	if(soName) soundso = hwOpen(soName);
#if (defined (_WIN32) || defined(_WIN64)) && !defined(STATIC3DS)
	HW3DS.pfnStartup            = hwSym("_Startup@8",soundso);
	HW3DS.pfnShutdown           = hwSym("_Shutdown@0",soundso);
	HW3DS.pfnAdd3DSource        = hwSym("_Add3DSource@8",soundso);
	HW3DS.pfnAdd2DSource        = hwSym("_Add2DSource@4",soundso);
	HW3DS.pfnStopSource         = hwSym("_StopSource@4",soundso);
	HW3DS.pfnStartSource        = hwSym("_StartSource@4",soundso);
	HW3DS.pfnGetHW3DSVersion    = hwSym("_GetHW3DSVersion@0",soundso);
	HW3DS.pfnBeginFrameUpdate   = hwSym("_BeginFrameUpdate@0",soundso);
	HW3DS.pfnEndFrameUpdate     = hwSym("_EndFrameUpdate@0",soundso);
	HW3DS.pfnIsPlaying          = hwSym("_IsPlaying@4",soundso);
	HW3DS.pfnUpdateListener     = hwSym("_UpdateListener@4",soundso);
	HW3DS.pfnUpdateListener2    = hwSym("_UpdateListener2@4",soundso);
	HW3DS.pfnSetGlobalSfxVolume = hwSym("_SetGlobalSfxVolume@4",soundso);
	HW3DS.pfnSetCone            = hwSym("_SetCone@8",soundso);
	HW3DS.pfnUpdate2DSoundParms = hwSym("_Update2DSoundParms@12",soundso);
	HW3DS.pfnUpdate3DSource     = hwSym("_Update3DSource@8",soundso);
	HW3DS.pfnUpdateSourceVolume = hwSym("_UpdateSourceVolume@8",soundso);
	HW3DS.pfnReload3DSource     = hwSym("_Reload3DSource@8",soundso);
	HW3DS.pfnKillSource         = hwSym("_KillSource@4",soundso);
#else
	HW3DS.pfnStartup            = hwSym("Startup",soundso);
	HW3DS.pfnShutdown           = hwSym("Shutdown",soundso);
	HW3DS.pfnAdd3DSource        = hwSym("Add3DSource",soundso);
	HW3DS.pfnAdd2DSource        = hwSym("Add2DSource",soundso);
	HW3DS.pfnStopSource         = hwSym("StopSource",soundso);
	HW3DS.pfnStartSource        = hwSym("StartSource",soundso);
	HW3DS.pfnGetHW3DSVersion    = hwSym("GetHW3DSVersion",soundso);
	HW3DS.pfnBeginFrameUpdate   = hwSym("BeginFrameUpdate",soundso);
	HW3DS.pfnEndFrameUpdate     = hwSym("EndFrameUpdate",soundso);
	HW3DS.pfnIsPlaying          = hwSym("IsPlaying",soundso);
	HW3DS.pfnUpdateListener     = hwSym("UpdateListener",soundso);
	HW3DS.pfnUpdateListener2    = hwSym("UpdateListener2",soundso);
	HW3DS.pfnSetGlobalSfxVolume = hwSym("SetGlobalSfxVolume",soundso);
	HW3DS.pfnSetCone            = hwSym("SetCone",soundso);
	HW3DS.pfnUpdate2DSoundParms = hwSym("Update2DSourceParms",soundso);
	HW3DS.pfnUpdate3DSource     = hwSym("Update3DSource",soundso);
	HW3DS.pfnUpdateSourceVolume = hwSym("UpdateSourceVolume",soundso);
	HW3DS.pfnReload3DSource     = hwSym("Reload3DSource",soundso);
	HW3DS.pfnKillSource         = hwSym("KillSource",soundso);
#endif

	if(HW3DS.pfnUpdateListener2 && HW3DS.pfnUpdateListener2 != soundso)
		return true;
	else
		return false;
}
#endif

void I_ShutdownSound(void)
{
	if(nosound || !sound_started)
		return;

	CONS_Printf("I_ShutdownSound: ");

#ifdef HW3SOUND
	if(hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_Shutdown();
		hwClose(soundso);
		return;
	}
#endif

	if(!sound_started)
		return;

	if(nomusic && nofmod)
		SDL_CloseAudio();
	CONS_Printf("shut down\n");
	sound_started = false;
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	if(Snd_Mutex)
		SDL_DestroyMutex(Snd_Mutex);
	Snd_Mutex = NULL;
}

void I_StartupSound(void)
{
#ifdef HW3SOUND
	const char *sdrv_name = NULL;
#endif
#ifndef HAVE_MIXER
	nomusic = nofmod = true;
#endif

	memset(channels, 0, sizeof(channels)); //Alam: Clean it

	audio.format = AUDIO_S16SYS;
	audio.channels = 2;
	audio.callback = I_UpdateStream;
	audio.userdata = channels;

	if(dedicated)
	{
		nosound = nomusic = nofmod = true;
		return;
	}

	// Configure sound device
	CONS_Printf("I_StartupSound:\n");

	// Open the audio device
	if(M_CheckParm ("-freq") && M_IsNextParm())
	{
		audio.freq = atoi(M_GetNextParm());
		if(!audio.freq) audio.freq = cv_samplerate.value;
		audio.samples = (Uint16)(samplecount*(int)(audio.freq/22050)); //Alam: to keep it around the same XX ms
		CONS_Printf (" requested frequency of %d hz\n", audio.freq);
	}
	else
	{
		audio.samples = samplecount;
#ifdef _WIN32_WCE
		audio.freq = 11025;
#else
		audio.freq = cv_samplerate.value;
#endif
	}

	if(nosound)
		return;

#ifdef HW3SOUND
#ifdef STATIC3DS
	if(M_CheckParm("-3dsound") || M_CheckParm("-ds3d"))
	{
		hws_mode = HWS_OPENAL;
	}
#elif defined(_WIN32) || defined(_WIN64)
	if(M_CheckParm("-ds3d"))
	{
		hws_mode = HWS_DS3D;
		sdrv_name = "s_ds3d.dll";
	}
	else if(M_CheckParm("-fmod3d"))
	{
		hws_mode = HWS_FMOD3D;
		sdrv_name = "s_fmod.dll";
	}
	else if(M_CheckParm("-openal"))
	{
		hws_mode = HWS_OPENAL;
		sdrv_name = "s_openal.dll";
	}
#else
	if(M_CheckParm("-fmod3d"))
	{
		hws_mode = HWS_FMOD3D;
		sdrv_name = "./s_fmod.so";
	}
	else if(M_CheckParm("-openal"))
	{
		hws_mode = HWS_OPENAL;
		sdrv_name = "./s_openal.so";
	}
#endif
	else if(M_CheckParm("-sounddriver") &&  M_IsNextParm())
	{
		hws_mode = HWS_OTHER;
		sdrv_name = M_GetNextParm();
	}
	if(hws_mode != HWS_DEFAULT_MODE)
	{
		if(Init3DSDriver(sdrv_name))
		{
			snddev_t            snddev;

			//nosound = true;
			//I_AddExitFunc(I_ShutdownSound);
			snddev.bps = 16;
			snddev.sample_rate = audio.freq;
#if defined(_WIN32) || defined(_WIN64)
			snddev.cooplevel = 0x00000002;
			snddev.hWnd = vid.WndParent;
#endif
			if(HW3S_Init(I_Error, &snddev))
			{
				CONS_Printf(" Using 3D sound driver\n");
				return;
			}
			audio.userdata = NULL;
			CONS_Printf(" Failed 3D sound Init\n");
			// Falls back to default sound system
			HW3S_Shutdown();
			hwClose(soundso);
		}
		CONS_Printf(" Failed loading 3D sound driver\n");
		hws_mode = HWS_DEFAULT_MODE;
	}
#endif
	if(!musicStarted && SDL_OpenAudio(&audio, NULL) < 0)
	{
		CONS_Printf(" couldn't open audio with desired format\n");
		SDL_CloseAudio();
		nosound = true;
		return;
	}
	samplecount = audio.samples;
	CV_SetValue(&cv_samplerate,audio.freq);
	CONS_Printf(" configured audio device with %d samples/slice at %ikhz(%dms buffer)\n", samplecount, audio.freq/1000, (int) (((float)audio.samples * 1000.0) / audio.freq));
	// Finished initialization.
	CONS_Printf("I_InitSound: sound module ready\n");
	//[segabor]
	if(!musicStarted) SDL_PauseAudio(0);
	//Mix_Pause(0);
	I_SetChannels();
	sound_started = true;
	Snd_Mutex = SDL_CreateMutex();
}

//
// MUSIC API.
//


void I_ShutdownMIDIMusic(void)
{
	nomusic = false;
	if(nofmod) I_ShutdownMusic();
}
void I_ShutdownDigMusic(void)
{
	nofmod = false;
	if(nomusic) I_ShutdownMusic();
}


void I_ShutdownMusic(void)
{
#ifdef HAVE_MIXER
	if(nomusic && nofmod)
		return;

	if(!musicStarted)
		return;

	CONS_Printf("I_ShutdownMusic: ");

	I_StopDigSong();
	I_UnRegisterSong(0);
	Mix_CloseAudio();

	CONS_Printf("shut down\n");
	musicStarted = SDL_FALSE;
	if(Msc_Mutex)
		SDL_DestroyMutex(Msc_Mutex);
	Msc_Mutex = NULL;
#endif
}

void I_InitMIDIMusic(void)
{
	if(nofmod) I_InitMusic();
}
void I_InitDigMusic(void)
{
	if(nomusic) I_InitMusic();
}

void I_InitMusic(void)
{
	if(nomusic && nofmod)
		return;

	if(dedicated)
		return;

#ifdef HAVE_MIXER
	if(audio.freq < 44100 && !M_CheckParm ("-freq")) //I want atleast 44Khz
	{
		audio.samples = (Uint16)(audio.samples*(int)(44100/audio.freq));
		audio.freq = 44100; //Alam: to keep it around the same XX ms
	}

	if(sound_started
#ifdef HW3SOUND
		&& hws_mode == HWS_DEFAULT_MODE
#endif
		)
	{
		CONS_Printf("Temp Shutdown of SDL Audio System");
		SDL_CloseAudio();
		CONS_Printf(" Done\n");
	}

	CONS_Printf("I_InitMusic:");

	if(Mix_OpenAudio(audio.freq, audio.format, audio.channels, audio.samples) < 0) //open_music(&audio)
	{
		CONS_Printf(" Unable to open music: %s\n", Mix_GetError());
		nomusic = nofmod = true;
		if(sound_started
#ifdef HW3SOUND
			&& hws_mode == HWS_DEFAULT_MODE
#endif
			)
		{
			if(SDL_OpenAudio(&audio, NULL) < 0) //retry
			{
				CONS_Printf(" couldn't reopen audio with desired format\n");
				SDL_CloseAudio();
				nosound = true;
				sound_started = false;
			}
		}
		return;
	}


#if 0 /// \todo Alam: Test!@#
	if (!Mix_QuerySpec(&audio.freq, &audio.format, &audio.channels)
	{
		CONS_Printf(" Unable to open music:  %s\n", Mix_GetError());
		nosound = nomusic = true;
		return;
	}
#endif

	samplecount = audio.samples;
	CV_SetValue(&cv_samplerate,audio.freq);
	if(sound_started
#ifdef HW3SOUND
		&& hws_mode == HWS_DEFAULT_MODE
#endif
		)
		CONS_Printf(" Reconfigured SDL Audio System");
	else CONS_Printf(" Configured SDL_Mixer System");
	CONS_Printf(" with %d samples/slice at %ikhz(%dms buffer)\n", samplecount, audio.freq/1000, (int) ((audio.samples * 1000.0f) / audio.freq));
	Mix_SetPostMix(audio.callback, audio.userdata);  // after mixing music, add sound effects
	Mix_Resume(-1);
	CONS_Printf("I_InitMusic: music initialized\n");
	musicStarted = SDL_TRUE;
	Msc_Mutex = SDL_CreateMutex();
#endif
}

boolean I_PlaySong(int handle, int looping)
{
	handle = 0;
#ifdef HAVE_MIXER
	if(nomusic || !musicStarted)
		return false;

	if(music[handle])
	{
		int musicst;
		Mix_HookMusicFinished(NULL);
		musicst = Mix_FadeInMusic(music[handle], looping ? -1 : 0, MIDIfade);
		if(musicst == -1)
			CONS_Printf("I_PlaySong: Couldn't play song because %s\n",Mix_GetError());
		else
		{
			Mix_VolumeMusic(musicvol);
			return true;
		}
	}
#else
	looping = 0;
#endif
	return false;
}

void I_PauseSong(int handle)
{
	handle = 0;
#ifdef HAVE_MIXER
	if((nomusic && nofmod) || !musicStarted)
		return;

	Mix_PauseMusic();
	//I_StopSong(handle);
#endif
}

void I_ResumeSong(int handle)
{
	handle = 0;
#ifdef HAVE_MIXER
	if((nomusic && nofmod) || !musicStarted)
		return;

	Mix_VolumeMusic(musicvol);
	Mix_ResumeMusic();
	//I_PlaySong(handle, true);
#endif
}

void I_StopSong(int handle)
{
	handle = 0;
#ifdef HAVE_MIXER
	if(nomusic || !musicStarted)
		return;
	Mix_FadeOutMusic(MIDIfade);
#endif
}

void I_UnRegisterSong(int handle)
{
	handle = 0;
#ifdef HAVE_MIXER
	if(nomusic || !musicStarted)
		return;

	Mix_HaltMusic();

	if(music[handle])
	{
		Mix_FreeMusic(music[handle]);
		music[handle] = NULL;
	}
	unlink(va("%s"MIDI_TMPFILE,srb2home));
#endif
}

int I_RegisterSong(void *data, int len)
{
#ifdef HAVE_MIXER
	FILE *midfile;
	const char *tempname = va("%s"MIDI_TMPFILE,srb2home);

	if(nomusic || !musicStarted)
		return false;

	midfile = fopen(tempname, "wb");
	if(!midfile)
	{
		CONS_Printf("Couldn't write MIDI to %s\n", tempname);
		return false;
	}

	if(memcmp(data, "MThd", 4) == 0) // support mid file in WAD !!!
	{
		fwrite(data, 1, len, midfile);
	}
	else
	{

		CONS_Printf("Music Lump is not a MIDI lump\n");
		return false;
	}

	fclose(midfile);

	music[0] = Mix_LoadMUS(tempname);

	if(music[0] == NULL)
	{
		CONS_Printf("Couldn't load MIDI from %s: %s\n", tempname, Mix_GetError());
	}
#else
	len = 0;
	data = NULL;
#endif
	return (0);
}

void I_SetMIDIMusicVolume(int volume)
{
#ifdef HAVE_MIXER
	if((nomusic && nofmod) || !musicStarted)
		return;

	if(Msc_Mutex) SDL_LockMutex(Msc_Mutex);
	musicvol = volume * 2;
	if(Msc_Mutex) SDL_UnlockMutex(Msc_Mutex);
	Mix_VolumeMusic(musicvol);
#else
	volume = 0;
#endif
}

boolean I_StartDigSong (const char* musicname, int looping)
{
#ifdef HAVE_MIXER
	char filename[9];
	const char *tempname;
	void *data;
	int lumpnum,lumplength;
	FILE *midfile;

	if(nofmod)
		return false;

	sprintf(filename, "o_%s", musicname);

	lumpnum = W_CheckNumForName(filename);

	I_StopDigSong();

	//unlink(MIDI_TMPFILE2);

	if(lumpnum == -1)
	{
	// Alam_GBC: like in win32/win_snd.c: Graue 02-29-2004: don't worry about missing music, there might still be a MIDI
		//CONS_Printf("Music lump %s not found!\n", filename);
		return false; // No music found. Oh well!
	}
	else
		lumplength = W_LumpLength(lumpnum);

	tempname = va("%s"MIDI_TMPFILE,srb2home);
	midfile = fopen(tempname, "wb");
	if(!midfile)
	{
		CONS_Printf("Couldn't write WAV to %s\n", tempname);
		return false;
	}

	data = W_CacheLumpName ( filename, PU_CACHE );

	fwrite(data, 1, lumplength, midfile);
	fclose(midfile);

	Z_Free(data);

	music[1] = Mix_LoadMUS(tempname);

	if(!music[1])
	{
		//CONS_Printf("Couldn't load music lump %s from the file %s\n",filename,tempname);
	}
	else
	{
		if(Msc_Mutex) SDL_LockMutex(Msc_Mutex);
		loopingDig = looping;
#ifdef MIXER_POS
		if(Mix_GetMusicType(music[1]) == MUS_OGG) //Only Ogg files
			if(loopingDig) looping = false; //Alam: SDL_mixer will play while i look for the loop point
#endif
		loopstartDig = 0.1L;
		if(Mix_FadeInMusic(music[1], looping ? -1 : 0,Digfade) == -1)
		{
			if(Msc_Mutex) SDL_UnlockMutex(Msc_Mutex);
			if(devparm)
				CONS_Printf("I_StartDigSong: Couldn't play song %s because %s\n",musicname, Mix_GetError());
			return false;
		}
		Mix_VolumeMusic(musicvol);
		// Scan the Ogg Vorbis file for the COMMENT= field for a custom loop point
		if(!looping && loopingDig)
		{
			int scan;
			byte* dataum;
			char looplength[64];
			unsigned int loopstart = 0;
			int newcount = 0;

			Mix_HookMusicFinished(I_FinishMusic);

			data = W_CacheLumpName ( filename, PU_CACHE );

			dataum = (byte *)data;

			for(scan = 0;scan < lumplength; scan++)
			{
				if(*dataum++ == 'C'){
				if(*dataum++ == 'O'){
				if(*dataum++ == 'M'){
				if(*dataum++ == 'M'){
				if(*dataum++ == 'E'){
				if(*dataum++ == 'N'){
				if(*dataum++ == 'T'){
				if(*dataum++ == '='){
				if(*dataum++ == 'L'){
				if(*dataum++ == 'O'){
				if(*dataum++ == 'O'){
				if(*dataum++ == 'P'){
				if(*dataum++ == 'P'){
				if(*dataum++ == 'O'){
				if(*dataum++ == 'I'){
				if(*dataum++ == 'N'){
				if(*dataum++ == 'T'){
				if(*dataum++ == '=')
				{

					while(*dataum != 1 && newcount != 63)
					{
						looplength[newcount++] = *dataum++;
					}

					looplength[newcount] = '\n';

					loopstart = atoi(looplength);

				}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
				else
					dataum--;}
			}

			Z_Free(data);

			if(loopstart > 0)
			{
				loopstartDig = (8.0L+loopstart) / 44100.0L; //8 PCM chucks off and PCM to secs
#ifdef PARANOIA
				//CONS_Printf("looping start at %i\n",(int)loopstartDig);
#endif
			}
			else
			{
				loopstartDig = 0.0L; // loopingDig true, but couldn't find start loop point
			}
		}
		else
			loopstartDig = 0.0L;
		if(Msc_Mutex) SDL_UnlockMutex(Msc_Mutex);
		return true;
	}
#else
	looping = 0;
	musicname = NULL;
#endif
	return false;
}

void I_StopDigSong(void)
{
#ifdef HAVE_MIXER
	if(nofmod)
		return;

	Mix_HookMusicFinished(NULL);
	Mix_HaltMusic();

	if(music[1])
	{
		Mix_FreeMusic(music[1]);
		music[1] = NULL;
	}
#endif
}

void I_SetDigMusicVolume(int volume)
{
#ifdef HAVE_MIXER
	//if(music[1] == NULL) return;

	I_SetMIDIMusicVolume(volume);
#else
	volume = 0;
#endif
}


#ifdef HAVE_MIXER
static void I_FinishMusic(void)
{
	int musicst;
	if(!music[1])
		return;
	if(Msc_Mutex) SDL_LockMutex(Msc_Mutex);
	musicst = Mix_FadeInMusicPos(music[1],loopstartDig?0:-1,Digfade,(double)loopstartDig);
	if(musicst == 0)
		Mix_VolumeMusic(musicvol);
	if(Msc_Mutex) SDL_UnlockMutex(Msc_Mutex);
	else if(devparm)
		CONS_Printf("I_FinishMusic: Couldn't loop song because %s\n",Mix_GetError());
}
#endif
