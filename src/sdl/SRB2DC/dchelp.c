// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2004 by Sonic Team Jr.
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
// DESCRIPTION:
//      stub and replacement "ANSI" C functions for use on Dreamcast/KOS
// 
//-----------------------------------------------------------------------------
#include "../../doomdef.h"

int access(const char* path, int amode)
{
	int accesshandle = 1;
	FILE *handle = NULL;
	if(amode == 6)
		handle = fopen(path, "r+");
	else if(amode == 4)
		handle = fopen(path, "r");
	else if(amode == 2)
		handle = fopen(path, "a+");
	else if(amode == 0)
		handle = fopen(path, "rb");
	if(handle)
	{
		accesshandle = 0;
		fclose(handle);
	}
	return accesshandle;
}

int mkdir(const char *_path, mode_t __mode )
{
	_path = NULL;
	__mode = 0;
	return -1;
}

static char cdromdir[8] = "/cdrom/";

char *getcwd(char *__buf, size_t __size )
{
	__buf = NULL;
	__size = 0;
	return cdromdir;
}

int chdir(const char *folder)
{
	folder = NULL;
	return -1;
}
