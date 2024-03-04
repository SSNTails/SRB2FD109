// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_cdmus.c,v 1.3 2001/05/16 22:33:35 bock Exp $
//
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
//
//
// $Log: i_cdmus.c,v $
// Revision 1.3  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//-----------------------------------------------------------------------------
/// \file
/// \brief cd music interface
///


#include <stdlib.h>
#include <SDL/SDL.h>
#include "../doomtype.h"
#include "../i_sound.h"
#include "../command.h"
#include "../m_argv.h"
#include "../s_sound.h"

#define MAX_CD_TRACKS 256

byte   cdaudio_started      = 0;   // for system startup/shutdown

static SDL_bool cdValid     = SDL_FALSE;
static SDL_bool cdPlaying   = SDL_FALSE;
static SDL_bool wasPlaying  = SDL_FALSE;
static SDL_bool cdEnabled   = SDL_FALSE;
static SDL_bool playLooping = SDL_FALSE;
static Uint8    playTrack   = 0;
static Uint8    maxTrack    = MAX_CD_TRACKS-1;
static Uint8    cdRemap[MAX_CD_TRACKS];
static int      cdvolume    = -1;
static SDL_CD  *cdrom       = NULL;
static CDstatus cdStatus    = CD_ERROR;

consvar_t cd_volume = {"cd_volume","31",CV_SAVE,soundvolume_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cdUpdate  = {"cd_update","1",CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

/**************************************************************************
 *
 * function: CDAudio_GetAudioDiskInfo
 *
 * description:
 * set number of tracks if CD is available
 *
 **************************************************************************/
static int CDAudio_GetAudioDiskInfo(void)
{
	cdValid = SDL_FALSE;
	maxTrack = 0;

	if(!cdrom)
		return 0;//Alam: Lies!

	cdStatus = SDL_CDStatus(cdrom);

	if(!CD_INDRIVE(cdStatus))
	{
		CONS_Printf("No CD in drive\n");
		return -1;
	}

	if(cdStatus == CD_ERROR)
	{
		CONS_Printf("CD Error: %s\n", SDL_GetError());
		return -1;
	}

	cdValid = true;
	maxTrack = (Uint8)cdrom->numtracks;

	return 0;
}


/**************************************************************************
 *
 * function: I_EjectCD
 *
 * description:
 *
 *
 **************************************************************************/
static void I_EjectCD(void)
{
	if (!cdrom || !cdEnabled)
		return; // no cd init'd

	I_StopCD();

	if(SDL_CDEject(cdrom))
		CONS_Printf("CD eject failed\n");
}

/**************************************************************************
 *
 * function: Command_Cd_f
 *
 * description:
 * handles all CD commands from the console
 *
 **************************************************************************/
static void Command_Cd_f (void)
{
	const char	*command;
	size_t ret, n;

	if(!cdaudio_started)
		return;

	if(COM_Argc() < 2)
	{
		CONS_Printf ("cd [on] [off] [remap] [reset] [select]\n"
				 "   [open] [info] [play <track>] [resume]\n"
				 "   [stop] [pause] [loop <track>]\n");
		return;
	}

	command = COM_Argv (1);

	if(!strncmp(command, "on", 2))
	{
		cdEnabled = true;
		return;
	}

	if(!strncmp(command, "off", 3))
	{
		I_StopCD();
		cdEnabled = SDL_FALSE;
		return;
	}

	if(!strncmp(command, "select", 6))
	{
		int newcddrive = atoi(COM_Argv(2));
		I_StopCD();
		cdEnabled = SDL_FALSE;
		SDL_CDClose(cdrom);
		cdrom = SDL_CDOpen(newcddrive);
		if(cdrom) cdEnabled = true;
		else CONS_Printf("Couldn't open CD-ROM drive %s: %s\n",
		 SDL_CDName(newcddrive)?SDL_CDName(newcddrive):COM_Argv(2), SDL_GetError());
		return;
	}

	if(!strncmp(command, "remap", 5))
	{
		ret = COM_Argc() - 2;
		if(ret <= 0)
		{
			for (n = 1; n < MAX_CD_TRACKS; n++)
			if(cdRemap[n] != n)
				CONS_Printf("  %u -> %u\n", n, cdRemap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			cdRemap[n] = (Uint8)atoi(COM_Argv (n+1));
		return;
	}

	if(!strncmp(command, "reset", 5))
	{
		if(!cdrom) return;
		cdEnabled = true;
		I_StopCD();
		for (n = 0; n < MAX_CD_TRACKS; n++)
			cdRemap[n] = (Uint8)n;
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if(!cdValid)
	{
		if(CDAudio_GetAudioDiskInfo()==-1 && !cdValid)
		{
			CONS_Printf("No CD in player.\n");
			return;
		}
	}

	if(!strncmp(command, "open", 4))
	{
		I_EjectCD();
		cdValid = SDL_FALSE;
		return;
	}

	if(!strncmp(command, "info", 4))
	{
		CONS_Printf("%u tracks\n", maxTrack);
		if(cdPlaying)
			CONS_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if(wasPlaying)
			CONS_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		CONS_Printf("Volume is %d\n", cdvolume);
		return;
	}

	if(!strncmp(command, "play", 4))
	{
		I_PlayCD((byte)atoi(COM_Argv (2)), SDL_FALSE);
		return;
	}

	if(!strncmp(command, "loop", 4)) 
	{
		I_PlayCD((byte)atoi(COM_Argv (2)), true);
		return;
	}

	if(!strncmp(command, "stop", 4))
	{
		I_StopCD();
		return;
	}
	if(!strncmp(command, "pause", 5))
	{
		I_PauseCD();
		return;
	}

	if(!strncmp(command, "resume", 6))
	{
		I_ResumeCD();
		return;
	}

	CONS_Printf("Invalid command \"cd %s\"\n", COM_Argv (1));
}

/**************************************************************************
 *
 * function: StopCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_StopCD(void)
{
	if(!cdrom || !cdEnabled)
		return;

	if(!(cdPlaying || wasPlaying))
		return;

	if(SDL_CDStop(cdrom))
		CONS_Printf("cdromstop failed\n");

	wasPlaying = SDL_FALSE;
	cdPlaying = SDL_FALSE;
}

/**************************************************************************
 *
 * function: PauseCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_PauseCD (void)
{
	if(!cdrom || !cdEnabled)
		return;

	if(!cdPlaying)
		return;

	if(SDL_CDPause(cdrom))
		CONS_Printf("cdrompause failed\n");

	wasPlaying = cdPlaying;
	cdPlaying = SDL_FALSE;
}

/**************************************************************************
 *
 * function: ResumeCD
 *
 * description:
 *
 *
 **************************************************************************/
// continue after a pause
void I_ResumeCD (void)
{
	if(!cdrom || !cdEnabled)
		return;

	if(!cdValid)
		return;

	if(!wasPlaying)
		return;
	
	if(cd_volume.value == 0)
		return;

	if(SDL_CDResume(cdrom))
		CONS_Printf("cdromresume failed\n");

	cdPlaying = true;
	wasPlaying = SDL_FALSE;
}


/**************************************************************************
 *
 * function: ShutdownCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_ShutdownCD (void)
{
	if(!cdaudio_started)
		return;

	I_StopCD();

	CONS_Printf("I_ShutdownCD: ");
	SDL_CDClose(cdrom);
	cdrom = NULL;
	cdaudio_started = SDL_FALSE;
	CONS_Printf("shut down\n");
	SDL_QuitSubSystem(SDL_INIT_CDROM);
	cdEnabled = SDL_FALSE;
}

/**************************************************************************
 *
 * function: InitCD
 *
 * description:
 * Initialize the first CD drive SDL detects and add console command 'cd'
 *
 **************************************************************************/
void I_InitCD (void)
{
	int i;
	//char *cdName;

	// Has been checked in d_main.c, but doesn't hurt here
	if(M_CheckParm ("-nocd"))
		return;

	CONS_Printf("I_InitCD: Init CD audio\n");

	// Initialize SDL first
	if(SDL_InitSubSystem(SDL_INIT_CDROM) < 0)
	{
		CONS_Printf("Couldn't initialize SDL CDROM: %s\n",SDL_GetError());
		return;
	}

	// Open drive
	cdrom = SDL_CDOpen(0);
	//cdName = SDL_CDName(0);

	if(!cdrom)
	{
		if(!SDL_CDName(0))
		{
			
			CONS_Printf("Couldn't open default CD-ROM drive: %s\n",
				SDL_GetError());
		}
		else
		{
			CONS_Printf("Couldn't open default CD-ROM drive %s: %s\n",
				SDL_CDName(0), SDL_GetError());
		}
		//return;
	}

	for (i = 0; i < MAX_CD_TRACKS; i++)
		cdRemap[i] = (Uint8)i;

	cdaudio_started = true;
	if(cdrom) cdEnabled = true;

	if(CDAudio_GetAudioDiskInfo()==-1)
	{
		CONS_Printf("I_InitCD: No CD in player.\n");
		cdValid = SDL_FALSE;
	}

	COM_AddCommand ("cd", Command_Cd_f);

	CONS_Printf("CD audio Initialized\n");
}



//
/**************************************************************************
 *
 * function: UpdateCD
 *
 * description:
 * sets CD volume (may have changed) and initiates play evey 2 seconds
 * in case the song has elapsed
 *
 **************************************************************************/
void I_UpdateCD (void)
{
	static Uint32 lastchk = 0;

	if(!cdEnabled || !cdrom)
		return;

	I_SetVolumeCD(cd_volume.value);

	if(cdPlaying && lastchk < SDL_GetTicks()) 
	{
		lastchk = SDL_GetTicks() + 2000; //two seconds between chks

		if(CDAudio_GetAudioDiskInfo()==-1)
		{
			cdPlaying = SDL_FALSE;
			return;
		}

		if(cdStatus != CD_PLAYING && cdStatus != CD_PAUSED)
		{
			cdPlaying = SDL_FALSE;
			if(playLooping)
				I_PlayCD(playTrack, true);
		}
	}
}



/**************************************************************************
 *
 * function: PlayCD
 *
 * description:
 * play the requested track and set the looping flag
 * pauses the CD if volume is 0
 * 
 **************************************************************************/

void I_PlayCD (int track, boolean looping)
{
	if(!cdrom || !cdEnabled)
		return;

	if(!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if(!cdValid)
			return;
	}

	track = cdRemap[track];

	if(track < 1 || track > maxTrack)
	{
		CONS_Printf("I_PlayCD: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	if(cdrom->track[track].type == SDL_DATA_TRACK)
	{
		CONS_Printf("I_PlayCD: track %i is not audio\n", track);
		return;
	}

	if(cdPlaying)
	{
		if(playTrack == track)
			return;
		I_StopCD();
	}

	if(SDL_CDPlayTracks(cdrom, track, 0, 1, 0))
	{
		CONS_Printf("Error playing track %d: %s\n",
				track, SDL_GetError());
		return;
	}

	playLooping = looping;
	playTrack = (Uint8)track;
	cdPlaying = true;

	if(cd_volume.value == 0)
		I_PauseCD();
}


/**************************************************************************
 *
 * function: SetVolumeCD
 *
 * description:
 * SDL does not support setting the CD volume
 * use pause instead and toggle between full and no music
 * 
 **************************************************************************/

int I_SetVolumeCD (int volume)
{
	if(volume != cdvolume)
	{
		if(volume > 0 && volume < 16)
		{
			CV_SetValue(&cd_volume, 31);
			cdvolume = 31;
			I_ResumeCD();
		}
		else if(volume > 15 && volume < 31)
		{
			CV_SetValue(&cd_volume, 0);
			cdvolume = 0;
			I_PauseCD();
		}
	}

	return 0;
}
