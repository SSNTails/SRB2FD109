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
/// \brief Menu widget stuff, skill selection and such

#ifndef __M_MENU__
#define __M_MENU__

#include "d_event.h"
#include "command.h"

//
// MENUS
//
// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
boolean M_Responder(event_t *ev);

// Called by main loop, only used for menu (skull cursor) animation.
void M_Ticker(void);

// Called by main loop, draws the menus directly into the screen buffer.
void M_Drawer(void);

// Called by D_SRB2Main, loads the config file.
void M_Init(void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.
void M_StartControlPanel(void);

// Draws a box with a texture inside as background for messages
void M_DrawTextBox(int x, int y, int width, int lines);
// show or hide the setup for player 2 (called at splitscreen change)
void M_SwitchSplitscreen(void);

// the function to show a message box typing with the string inside
// string must be static (not in the stack)
// routine is a function taking a int in parameter
typedef enum
{
	MM_NOTHING = 0, // is just displayed until the user do someting
	MM_YESNO,       // routine is called with only 'y' or 'n' in param
	MM_EVENTHANDLER // the same of above but without 'y' or 'n' restriction
	                // and routine is void routine(event_t *) (ex: set control)
} menumessagetype_t;
void M_StartMessage(const char* string, void* routine, menumessagetype_t itemtype);

// Called by linux_x/i_video_xshm.c
void M_QuitResponse(int ch);

typedef struct menuitem_s
{
	// show IT_xxx
	short status;

	const char* patch;
	const char* text; // used when FONTBxx lump is found

	// FIXME: should be itemaction_t
	void* itemaction;

	// hotkey in menu or y of the item
	byte alphaKey;
} menuitem_t;

extern menuitem_t PlayerMenu[];
// Stuff for customizing the player select screen
typedef struct
{
	char info[255];
	char picname[9];
	char text[64];
	char skinname[16];
} description_t;

extern description_t description[8];

extern consvar_t cv_newgametype, cv_nextmap;
extern CV_PossibleValue_t gametype_cons_t[], skill_cons_t[];

#endif
