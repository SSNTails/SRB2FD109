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
/// \brief Commonly used routines
/// 
///	Default Config File.
///	PCX Screenshots.
///	File i/o

#ifdef __GNUC__
#include <unistd.h>
#endif
#ifdef SDLIO
#include <SDL/SDL_rwops.h>
#else
#ifndef _WIN32_WCE
#include <fcntl.h>
#endif
#endif

#include "doomdef.h"
#include "g_game.h"
#include "m_misc.h"
#include "hu_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_video.h"
#include "d_main.h"
#include "m_argv.h"

#ifdef _WIN32_WCE
#include "sdl/SRB2CE/cehelp.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// Extended map support.
#include <ctype.h>

/** Returns the map number for a map identified by the last two characters in
  * its name.
  *
  * \param first  The first character after MAP.
  * \param second The second character after MAP.
  * \return The map number, or 0 if no map corresponds to these characters.
  * \sa G_BuildMapName
  */
int M_MapNumber(char first, char second)
{
	if(isdigit(first))
	{
		if(isdigit(second))
			return ((int)first - '0') * 10 + ((int)second - '0');
		return 0;
	}

	if(!isalpha(first))
		return 0;
	if(!isalnum(second))
		return 0;

	return 100 + ((int)tolower(first) - 'a') * 36 + (isdigit(second) ? ((int)second - '0') :
		((int)tolower(second) - 'a') + 10);
}

// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================

//
// FIL_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

/** Writes out a file.
  *
  * \param name   Name of the file to write.
  * \param source Memory location to write from.
  * \param length How many bytes to write.
  * \return True on success, false on failure.
  */
boolean FIL_WriteFile(char const* name, void* source, size_t length)
{
#ifdef SDLIO
	SDL_RWops* handle;
#else
	int handle;
#endif
	size_t count;

#ifdef SDLIO
	handle = SDL_RWFromFile(name, "w+b");
#else
	handle = open(name, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);
#endif

#ifdef SDLIO
	if(!handle)
#else
	if(handle == -1)
#endif
		return false;

#ifdef SDLIO
	count = SDL_RWwrite(handle, source, 1, length);
	SDL_RWclose(handle);
#else
	count = write(handle, source, (unsigned int)length);
	close(handle);
#endif

	if(count < length)
		return false;

	return true;
}

/** Reads in a file, appending a zero byte at the end.
  *
  * \param name   Filename to read.
  * \param buffer Pointer to a pointer, which will be set to the location of a
  *               newly allocated buffer holding the file's contents.
  * \return Number of bytes read, not counting the zero byte added to the end,
  *         or 0 on error.
  */
int FIL_ReadFile(char const* name, byte** buffer)
{
#ifdef SDLIO
	SDL_RWops* handle;
#else
	int handle;
#endif
	int count, length;
#ifndef SDLIO
	struct stat fileinfo;
#endif
	byte* buf;

#ifdef SDLIO
	handle = SDL_RWFromFile(name, "rb");
	if(!handle)
#else
	handle = open(name, O_RDONLY | O_BINARY, 0666);
	if(handle == -1)
#endif
		return 0;

#ifndef SDLIO
	if(fstat(handle, &fileinfo) == -1)
		return 0;
#endif

#ifdef SDLIO
	{
		int currpos = SDL_RWtell(handle);
		SDL_RWseek(handle,0,SEEK_END);
		length = SDL_RWtell(handle);
		SDL_RWseek(handle,currpos,SEEK_SET);
	}
#else
	length = fileinfo.st_size;
#endif
	buf = Z_Malloc(length + 1, PU_STATIC, 0);
#ifdef SDLIO
	count = SDL_RWread(handle, buf, 1, length);
	SDL_RWclose(handle);
#else
	count = read(handle, buf, length);
	close(handle);
#endif

	if(count < length)
		return 0;

	// append 0 byte for script text files
	buf[length] = 0;

	*buffer = buf;
	return length;
}

/** Checks if a pathname has a file extension and adds the extension provided
  * if not.
  *
  * \param path      Pathname to check.
  * \param extension Extension to add if no extension is there.
  */
void FIL_DefaultExtension(char* path, const char* extension)
{
	char* src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while(*src != '/' && src != path)
	{
		if(*src == '.')
			return; // it has an extension
		src--;
	}

	strcat(path, extension);
}

/** Checks if a filename extension is found.
  * Lump names do not contain dots.
  *
  * \param in String to check.
  * \return True if an extension is found, otherwise false.
  */
boolean FIL_CheckExtension(char* in)
{
	while(*in++)
		if(*in == '.')
			return true;

	return false;
}

// ==========================================================================
//                        CONFIGURATION FILE
// ==========================================================================

//
// DEFAULTS
//

char configfile[MAX_WADPATH];

// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
static boolean gameconfig_loaded = false; // true once config.cfg loaded AND executed

/** Saves a player's config, possibly to a particular file.
  *
  * \sa Command_LoadConfig_f
  */
void Command_SaveConfig_f(void)
{
	char tmpstr[MAX_WADPATH];

	if(COM_Argc() != 2)
	{
		CONS_Printf("saveconfig <filename[.cfg]> : save config to a file\n");
		return;
	}
	strcpy(tmpstr, COM_Argv(1));
	FIL_DefaultExtension(tmpstr, ".cfg");

	M_SaveConfig(tmpstr);
	CONS_Printf("config saved as %s\n", configfile);
}

/** Loads a game config, possibly from a particular file.
  *
  * \sa Command_SaveConfig_f, Command_ChangeConfig_f
  */
void Command_LoadConfig_f(void)
{
	if(COM_Argc() != 2)
	{
		CONS_Printf("loadconfig <filename[.cfg]> : load config from a file\n");
		return;
	}

	strcpy(configfile, COM_Argv(1));
	FIL_DefaultExtension(configfile, ".cfg");
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
}

/** Saves the current configuration and loads another.
  *
  * \sa Command_LoadConfig_f, Command_SaveConfig_f
  */
void Command_ChangeConfig_f(void)
{
	if(COM_Argc() != 2)
	{
		CONS_Printf("changeconfig <filename[.cfg]> : save current config and load another\n");
		return;
	}

	COM_BufAddText(va("saveconfig \"%s\"\n", configfile));
	COM_BufAddText(va("loadconfig \"%s\"\n", COM_Argv(1)));
}

/** Loads the default config file.
  *
  * \sa Command_LoadConfig_f
  */
void M_FirstLoadConfig(void)
{
	// configfile is initialised by d_main when searching for the wad?

	// check for a custom config file
	if(M_CheckParm("-config") && M_IsNextParm())
	{
		strcpy(configfile, M_GetNextParm());
		CONS_Printf("config file: %s\n",configfile);
	}

	// load default control
	G_Controldefault();

	// load config, make sure those commands doesnt require the screen..
	CONS_Printf("\n");
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
	// no COM_BufExecute() needed; that does it right away

	// make sure I_Quit() will write back the correct config
	// (do not write back the config if it crash before)
	gameconfig_loaded = true;
}

/** Saves the game configuration.
  *
  * \sa Command_SaveConfig_f
  */
void M_SaveConfig(char* filename)
{
	FILE* f;

	// make sure not to write back the config until it's been correctly loaded
	if(!gameconfig_loaded)
		return;

	// can change the file name
	if(filename)
	{
		f = fopen(filename, "w");
		// change it only if valid
		if(f)
			strcpy(configfile, filename);
		else
		{
			CONS_Printf("Couldn't save game config file %s\n", filename);
			return;
		}
	}
	else
	{
		f = fopen(configfile, "w");
		if(!f)
		{
			CONS_Printf("Couldn't save game config file %s\n", configfile);
			return;
		}
	}

	// header message
	fprintf(f, "// SRB2 configuration file.\n");

	// FIXME: save key aliases if ever implemented..

	CV_SaveVariables(f);
	if(!dedicated) G_SaveKeySetting(f);

	fclose(f);
}

// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================

/** PCX file structure.
  */
typedef struct
{
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;

	unsigned short xmin, ymin;
	unsigned short xmax, ymax;
	unsigned short hres, vres;
	unsigned char palette[48];

	char reserved;
	char color_planes;
	unsigned short bytes_per_line;
	unsigned short palette_type;

	char filler[58];
	unsigned char data; ///< Unbounded; used for all picture data.
} pcx_t;

/** Writes a PCX file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data
  */
static boolean WritePCXfile(char* filename, byte* data, int width, int height, byte* palette)
{
	int i;
	size_t length;
	pcx_t* pcx;
	byte* pack;

	pcx = Z_Malloc(width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a; // PCX id
	pcx->version = 5; // 256 color
	pcx->encoding = 1; // uncompressed
	pcx->bits_per_pixel = 8; // 256 color
	pcx->xmin = pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset(pcx->palette, 0, sizeof(pcx->palette));
	pcx->color_planes = 1; // chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(1); // not a grey scale
	memset(pcx->filler, 0, sizeof(pcx->filler));

	// pack the image
	pack = &pcx->data;

	for(i = 0; i < width*height; i++)
	{
		if((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

	// write the palette
	*pack++ = 0x0c; // palette ID byte
	for(i = 0; i < 768; i++)
		*pack++ = *palette++;

	// write output file
	length = pack - (byte*)pcx;
	i = FIL_WriteFile(filename, pcx, length);

	Z_Free(pcx);
	return i;
}

/** Takes a screenshot.
  * The screenshot is saved as "srb2xxxx.pcx" (or "srb2xxxx.tga" in hardware
  * rendermode) where xxxx is the lowest four-digit number for which a file
  * does not already exist.
  *
  * \sa HWR_ScreenShot
  */
void M_ScreenShot(void)
{
	char freename[13];
	boolean ret = false;

	{
		int i = 5000; // start in the middle: num screenshots divided by 2
		int add; // how much to add or subtract if wrong; gets divided by 2 each time
		int result; // -1 = guess too high, 0 = correct, 1 = guess too low
		byte* linear = NULL; // just so the compiler shuts up

		// find a file name to save it to
		strcpy(freename, "You are dumb"); // slots are 0 to 9999
		freename[9] = 'p'; freename[0] = 's'; freename[10] = 'c';
		freename[1] = 'r'; freename[3] = '2'; freename[11] = 'x';
		freename[2] = 'b'; freename[8] = '.';
		if(rendermode != render_soft)
		{
			freename[11] = 'a'; freename[9] = 't'; freename[10] = 'g';
		}
		else if(rendermode != render_none)
		{
			// munge planar buffer to linear
			linear = screens[2];
			I_ReadScreen(linear);
		}
		else
			I_Error("Can't take a screenshot without a render system");

		add = i;

		for(;;)
		{
			freename[4] = (char)('0' + (char)(i/1000));
			freename[5] = (char)('0' + (char)((i/100)%10));
			freename[6] = (char)('0' + (char)((i/10)%10));
			freename[7] = (char)('0' + (char)(i%10));

			if(access(freename, W_OK) != -1) // access succeeds
				result = 1; // too low
			else // access fails: equal or too high
			{
				if(!i)
					break; // not too high, so it must be equal! YAY!

				freename[4] = (char)('0' + (char)((i-1)/1000));
				freename[5] = (char)('0' + (char)(((i-1)/100)%10));
				freename[6] = (char)('0' + (char)(((i-1)/10)%10));
				freename[7] = (char)('0' + (char)((i-1)%10));
				if(access(freename, W_OK) == -1) // access fails
					result = -1; // too high
				else
					break; // not too high, so equal, YAY!
			}

			add /= 2;

			if(!add) // don't get stuck at 5 due to truncation!
				add = 1;

			i += add * result;

			if(add < 0 || add > 9999)
				goto failure;
		}

		freename[4] = (char)('0' + (char)(i/1000));
		freename[5] = (char)('0' + (char)((i/100)%10));
		freename[6] = (char)('0' + (char)((i/10)%10));
		freename[7] = (char)('0' + (char)(i%10));

		// save the pcx file
#ifdef HWRENDER
		if(rendermode != render_soft)
			ret = HWR_Screenshot(freename);
		else
#endif
		if(rendermode != render_none)
			ret = WritePCXfile(freename, linear, vid.width, vid.height,
				W_CacheLumpName("PLAYPAL", PU_CACHE));
	}

failure:
	if(ret)
		CONS_Printf("screen shot %s saved\n", freename);
	else
	{
		if(freename)
			CONS_Printf("Couldn't create screen shot %s\n", freename);
		else
			CONS_Printf("Couldn't create screen shot (all 10000 slots used!)\n");
	}
}

// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================

/** Returns a temporary string made out of varargs.
  * For use with CONS_Printf().
  *
  * \param format Format string.
  * \return Pointer to a static buffer of 1024 characters, containing the
  *         resulting string.
  */
char* va(const char* format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

// creates a copy of a string, null-terminated
// returns ptr to the new duplicate string
//
char* Z_StrDup(const char* in)
{
	char* out;

	out = ZZ_Alloc(strlen(in) + 1);
	strcpy(out, in);
	return out;
}

/** Creates a string in the first argument that is the second argument followed
  * by the third argument followed by the first argument.
  * Useful for making filenames with full path. s1=s2+s3+s1
  *
  * \param s1 First string, suffix, and destination.
  * \param s2 Second string. Ends up first in the result.
  * \param s3 Third string. Ends up second in the result.
  */
void strcatbf(char* s1, const char* s2, const char* s3)
{
	char tmp[1024];

	strcpy(tmp, s1);
	strcpy(s1, s2);
	strcat(s1, s3);
	strcat(s1, tmp);
}
