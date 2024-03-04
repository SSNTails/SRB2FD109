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
/// \brief Heads up display

#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"

#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"

#include "dstrings.h"
#include "st_stuff.h" // ST_HEIGHT
#include "r_local.h"

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#include "p_tick.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// coords are scaled
#define HU_INPUTX 0
#define HU_INPUTY 0

//-------------------------------------------
//              heads up font
//-------------------------------------------
patch_t* hu_font[HU_FONTSIZE];

// Level title and credits fonts
patch_t* lt_font[LT_FONTSIZE];
patch_t* cred_font[CRED_FONTSIZE];

static player_t* plr;
boolean chat_on; // entering a chat message?
static char w_chat[HU_MAXMSGLEN];
static boolean headsupactive = false;
static boolean hu_showscores; // draw rankings
static char hu_tick;

//-------------------------------------------
//              coop hud
//-------------------------------------------

patch_t* emerald1;
patch_t* emerald2;
patch_t* emerald3;
patch_t* emerald4;
patch_t* emerald5;
patch_t* emerald6;
patch_t* emerald7;
patch_t* emerald8;
static patch_t* emblemicon;

//-------------------------------------------
//              misc vars
//-------------------------------------------

static consvar_t* chat_macros[10];

// crosshair 0 = off, 1 = cross, 2 = angle, 3 = point, see m_menu.c
static patch_t* crosshair[3]; // 3 precached crosshair graphics

// -------
// protos.
// -------
static void HU_DrawRankings(void);
static void HU_DrawCoopOverlay(void);
static inline void HU_DrawCrosshair(void);
static inline void HU_DrawCrosshair2(void);

//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

char* shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'[', // shift-[
	'!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	']', // shift-]
	'"', '_',
	'~', // shift-` for some stupid reason this was a '
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

char cechotext[1024];
tic_t cechotimer = 0;
tic_t cechoduration = 5*TICRATE;
int cechoflags = 0;

//======================================================================
//                          HEADS UP INIT
//======================================================================

// just after
static void Command_Say_f(void);
static void Command_Sayto_f(void);
static void Command_Sayteam_f(void);
static void Got_Saycmd(char** p,int playernum);

// Initialise Heads up
// once at game startup.
//
void HU_Init(void)
{
	int i, j;
	char buffer[9];

	COM_AddCommand("say", Command_Say_f);
	COM_AddCommand("sayto", Command_Sayto_f);
	COM_AddCommand("sayteam", Command_Sayteam_f);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);

	if(dedicated)
		return;

	// set shift translation table
	shiftxform = english_shiftxform;

	// cache the heads-up font for entire game execution
	j = HU_FONTSTART;
	for(i = 0; i < HU_FONTSIZE; i++)
	{
		sprintf(buffer, "STCFN%.3d", j);
		j++;
		if(i >= HU_REALFONTSIZE && i != '~' - HU_FONTSTART && i != '`' - HU_FONTSTART) /// \note font end hack
			continue;
		hu_font[i] = (patch_t*)W_CachePatchName(buffer, PU_STATIC);
	}

	// cache the level title font for entire game execution
	lt_font[0] = (patch_t*)W_CachePatchName("LTFNT039", PU_STATIC); /// \note fake start hack
	j = LT_REALFONTSTART;
	for(i = LT_REALFONTSTART - LT_FONTSTART; i < LT_FONTSIZE; i++)
	{
		sprintf(buffer, "LTFNT%.3d", j);
		j++;
		lt_font[i] = (patch_t*)W_CachePatchName(buffer, PU_STATIC);
	}

	// cache the credits font for entire game execution (why not?)
	j = CRED_FONTSTART;
	for(i = 0; i < CRED_FONTSIZE; i++)
	{
		sprintf(buffer, "CRFNT%.3d", j);
		j++;
		cred_font[i] = (patch_t*)W_CachePatchName(buffer, PU_STATIC);
	}

	// cache the crosshairs, don't bother to know which one is being used,
	// just cache all 3, they're so small anyway.
	for(i = 0; i < HU_CROSSHAIRS; i++)
	{
		sprintf(buffer, "CROSHAI%c", '1'+i);
		crosshair[i] = (patch_t*)W_CachePatchName(buffer, PU_STATIC);
	}

	emblemicon = W_CachePatchName("EMBLICON", PU_STATIC);
	emerald1 = W_CachePatchName("CHAOS1", PU_STATIC);
	emerald2 = W_CachePatchName("CHAOS2", PU_STATIC);
	emerald3 = W_CachePatchName("CHAOS3", PU_STATIC);
	emerald4 = W_CachePatchName("CHAOS4", PU_STATIC);
	emerald5 = W_CachePatchName("CHAOS5", PU_STATIC);
	emerald6 = W_CachePatchName("CHAOS6", PU_STATIC);
	emerald7 = W_CachePatchName("CHAOS7", PU_STATIC);
	emerald8 = W_CachePatchName("CHAOS8", PU_STATIC);
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}

//
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
	if(headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];
	HU_clearChatChars();

	headsupactive = true;
}

//======================================================================
//                            EXECUTION
//======================================================================

void TeamPlay_OnChange(void)
{
	int i;
	char s[50];

	// Change the name of the teams

	if(cv_teamplay.value == 1)
	{
		// color
		for(i = 0; i < MAXSKINCOLORS; i++)
		{
			sprintf(s, "%s team", Color_Names[i]);
			strcpy(team_names[i],s);
		}
	}
	else if(cv_teamplay.value == 2)
	{
		// skins

		for(i = 0; i < numskins; i++)
		{
			sprintf(s, "%s team", skins[i].name);
			strcpy(team_names[i], s);
		}
	}
}

static void Command_Say_f(void)
{
	char buf[255];
	size_t i, j;

	if((j = COM_Argc()) < 2)
	{
		CONS_Printf("say <message> : send a message\n");
		return;
	}

	if(cv_mute.value && !(server || admin))
	{
		CONS_Printf("The chat is muted. You can't say anything at the moment.\n");
		return;
	}

	buf[0] = 0;
	strcpy(&buf[1], COM_Argv(1));
	for(i = 2; i < j; i++)
	{
		strcat(&buf[1], " ");
		strcat(&buf[1], COM_Argv(i));
	}
	SendNetXCmd(XD_SAY, buf, strlen(buf + 1) + 2); // +2 because 1 for buf[0] and the other for null terminated string
}

static void Command_Sayto_f(void)
{
	char buf[255];
	size_t i, j;

	if((j = COM_Argc()) < 3)
	{
		CONS_Printf("sayto <playername|playernum> <message> : send a message to a player\n");
		return;
	}

	if(cv_mute.value && !(server || admin))
	{
		CONS_Printf("The chat is muted. You can't say anything at the moment.\n");
		return;
	}

	buf[0] = nametonum(COM_Argv(1));
	if(buf[0] == (char)-1)
		return;
	strcpy(&buf[1], COM_Argv(2));
	for(i = 3; i < j; i++)
	{
		strcat(&buf[1], " ");
		strcat(&buf[1], COM_Argv(i));
	}
	SendNetXCmd(XD_SAY, buf, strlen(buf + 1) + 2);
}

static void Command_Sayteam_f(void)
{
	char buf[255];
	size_t i, j;

	if((j = COM_Argc()) < 2)
	{
		CONS_Printf("sayteam <message> : send a message to your team\n");
		return;
	}

	if(cv_mute.value && !(server || admin))
	{
		CONS_Printf("The chat is muted. You can't say anything at the moment.\n");
		return;
	}

	buf[0] = (char)(-consoleplayer);

	strcpy(&buf[1], "*team* ");

	strcat(&buf[1], COM_Argv(1));
	for(i = 2; i < j; i++)
	{
		strcat(&buf[1], " ");
		strcat(&buf[1], COM_Argv(i));
	}
	SendNetXCmd(XD_SAY, buf, strlen(buf + 1) + 2); // +2 because 1 for buf[0] and the other for null terminated string
}

// netsyntax : to : byte 1->32  player 1 to 32
//                       0      all
//                      -1->-32 say team -numplayer of the sender

static void Got_Saycmd(char** p, int playernum)
{
	int to;

	if(cv_mute.value && playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal say command received from %s while muted\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	to = *(*p)++;
	if(to == 0 || to == consoleplayer || consoleplayer == playernum
		|| (to < 0 && ST_SameTeam(&players[consoleplayer], &players[-to])))
	{
		if(strlen(*p) > 2)
		{
			// ME support! Tails
			if((strncmp(*p, "/ME", 3) == 0) || (strncmp(*p, "/me", 3) == 0))
			{
				char message[255];
				char* funky;

				funky = *p + 3; // Graue 06-30-2004: not + 4
				strcat(message, funky);
				CONS_Printf("* %s%s\n", player_names[playernum], funky); // no space between %s's
				*p += strlen(*p) + 1;
				return;
			}
		}

		// Graue 06-19-2004: smarter display
		if(to == 0)
		{
			if(gametype == GT_CTF && players[playernum].ctfteam)
			{
				if(players[playernum].ctfteam == 1) // red
					CONS_Printf("\3<\x85%s\x80> %s\n", player_names[playernum], *p);
				else if(players[playernum].ctfteam == 2) // blue
					CONS_Printf("\3<\x84%s\x80> %s\n", player_names[playernum], *p);
			}
			else
				CONS_Printf("\3<%s> %s\n", player_names[playernum], *p);
		}
		else if(to == consoleplayer) // Graue 07-06-2004
		{
			if(gametype == GT_CTF && players[playernum].ctfteam)
			{
				if(players[playernum].ctfteam == 1) // red
					CONS_Printf("\3*\x85%s\x80* %s\n", player_names[playernum], *p);
				else if(players[playernum].ctfteam == 2) // blue
					CONS_Printf("\3*\x84%s\x80* %s\n", player_names[playernum], *p);
			}
			else
				CONS_Printf("\3[%s] %s\n", player_names[playernum], *p);
		}
		else // sayteam Graue 07-06-2004
			CONS_Printf("\3>>%s<< %s\n", player_names[playernum], *p);
	}

	*p += strlen(*p) + 1;
}

// Handles key input and string input
//
static inline boolean HU_keyInChatString(char* s, char ch)
{
	size_t l;

	if(ch >= ' ' && (ch <= '_' || ch == '~' || ch == '`')) /// \note font end hack
	{
		l = strlen(s);
		if(l < HU_MAXMSGLEN - 1)
		{
			s[l++] = ch;
			s[l]=0;
			return true;
		}
		return false;
	}
	else if(ch == KEY_BACKSPACE)
	{
		l = strlen(s);
		if(l)
			s[--l] = 0;
		else
			return false;
	}
	else if(ch != KEY_ENTER)
		return false; // did not eat key

	return true; // ate the key
}

//
//
void HU_Ticker(void)
{
	player_t* pl;

#if (!defined(_WINDOWS) && !defined(SDLMAIN))
	if(dedicated)
		return;
#endif

	hu_tick++;
	hu_tick &= 7; // currently only to blink chat input cursor

	// display message if necessary
	// (display the viewplayer's messages)
	pl = &players[displayplayer];

	if(cv_showmessages.value && pl->message)
	{
		CONS_Printf("%s\n", pl->message);
		pl->message = 0;
	}

	// In splitscreen, display second player's messages
	if(cv_splitscreen.value)
	{
		pl = &players[secondarydisplayplayer];
		if(cv_showmessages.value && pl->message)
		{
			CONS_Printf("\2%s\n",pl->message);
			pl->message = 0;
		}
	}

	if((gamekeydown[gamecontrol[gc_scores][0]] || gamekeydown[gamecontrol[gc_scores][1]]))
		hu_showscores = !chat_on;
	else
	{
		if(gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF
			|| gametype == GT_CHAOS)
		{
			hu_showscores = playerdeadview;
		}
		else
			hu_showscores = false;
	}
}

#define QUEUESIZE 128

static char chatchars[QUEUESIZE];
static int head = 0, tail = 0;

//
// HU_dequeueChatChar
//
char HU_dequeueChatChar(void)
{
	char c;

	if(head != tail)
	{
		c = chatchars[tail];
		tail = (tail + 1) & (QUEUESIZE-1);
	}
	else
		c = 0;

	return c;
}

//
//
static void HU_queueChatChar(char c)
{
	if(((head + 1) & (QUEUESIZE-1)) == tail)
		plr->message = HUSTR_MSGU; // message not sent
	else
	{
		if(c == KEY_BACKSPACE)
		{
			if(tail != head)
				head = (head - 1) & (QUEUESIZE-1);
		}
		else
		{
			chatchars[head] = c;
			head = (head + 1) & (QUEUESIZE-1);
		}
	}

	// send automaticly the message (no more chat char)
	if(c == KEY_ENTER)
	{
		char buf[255], c;
		int i = 0;

		do {
			c = HU_dequeueChatChar();
			if(c != 13) // Graue 07-04-2004: don't know why this has to be done
				buf[i++]=c;
		} while(c);
		// Graue 09-04-2004: 1 not 2, hell if I know why
		if(i > 1) // Graue 07-02-2004: 2 not 3, with HU_BROADCAST disposed of
			COM_BufInsertText(va("say \"%s\"", buf)); // Graue 07-04-2004: quote it!
	}
}

void HU_clearChatChars(void)
{
	while(tail != head)
		HU_queueChatChar(KEY_BACKSPACE);
	chat_on = false;
}

//
// Returns true if key eaten
//
boolean HU_Responder(event_t* ev)
{
	static boolean shiftdown = false, altdown = false;
	boolean eatkey = false;
	const char* macromessage;
	unsigned char c;

	if(ev->data1 == KEY_SHIFT)
	{
		shiftdown = (ev->type == ev_keydown);
		return false;
	}
	else if(ev->data1 == KEY_ALT)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}

	if(ev->type != ev_keydown)
		return false;

	// only KeyDown events now...

	if(!chat_on)
	{
		// enter chat mode
		if((ev->data1 == gamecontrol[gc_talkkey][0] || ev->data1 == gamecontrol[gc_talkkey][1])
			&& netgame && (!cv_mute.value || server || admin))
		{
			eatkey = chat_on = true;
			w_chat[0] = 0;
		}
	}
	else
	{
		c = (unsigned char)ev->data1;

		// use console translations
		if(shiftdown)
			c = shiftxform[c];

		// send a macro
		if(altdown)
		{
			c = (unsigned char)(c - '0');
			if(c > 9)
				return false;

			macromessage = chat_macros[c]->string;

			// kill last message with a '\n'
			HU_queueChatChar(KEY_ENTER); // DEBUG!!!

			// send the macro message
			while(*macromessage)
				HU_queueChatChar(*macromessage++);
			HU_queueChatChar(KEY_ENTER);

			// leave chat mode and notify that it was sent
			chat_on = false;
			eatkey = true;
		}
		else
		{
			// Graue 07-04-2004: on following line, added a ! before shiftdown
			//                   and changed || to &&
			// if shiftdown, then the shiftxform has ALREADY been done
			// (this is what was causing ^ to show up as ", shiftxform being done twice)
			if(!shiftdown && (c >= 'a' && c <= 'z'))
				c = shiftxform[c];

			if(c == '"') // Graue 07-04-2004: quote marks mess it up
				c = '\'';

			eatkey = HU_keyInChatString(w_chat,c);
			if(eatkey)
				HU_queueChatChar(c);
			if(c == KEY_ENTER)
				chat_on = false;
			else if(c == KEY_ESCAPE)
				chat_on = false;
		}
	}

	return eatkey;
}

//======================================================================
//                         HEADS UP DRAWING
//======================================================================

//
// HU_DrawChat
//
// Draw chat input
//
static void HU_DrawChat(void)
{
	int i, c, y;

	c = 0;
	i = 0;
	y = HU_INPUTY;
	while(w_chat[i])
	{
		//Hurdler: isn't it better like that?
		V_DrawCharacter(HU_INPUTX + (c<<3), y, w_chat[i++] | V_NOSCALEPATCH|V_NOSCALESTART);

		c++;
		if(c >= (vid.width>>3))
		{
			c = 0;
			y += 8;
		}
	}

	if(hu_tick < 4)
		V_DrawCharacter(HU_INPUTX + (c<<3), y, '_' | V_NOSCALEPATCH|V_NOSCALESTART);
}

// Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
	// draw chat string plus cursor
	if(chat_on)
		HU_DrawChat();

	if(gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE)
		return;

	// draw multiplayer rankings
	if(hu_showscores)
	{
		if(gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF || gametype == GT_CHAOS)
			HU_DrawRankings();
		else if(gametype == GT_COOP)
		{
			if(multiplayer || netgame)
				HU_DrawRankings();

			HU_DrawCoopOverlay();
		}
	}

	// draw the crosshair, not when viewing demos nor with chasecam
	if(!automapactive && cv_crosshair.value && !demoplayback && !cv_chasecam.value)
		HU_DrawCrosshair();

	if(!automapactive && cv_crosshair2.value && !demoplayback && !cv_chasecam2.value)
		HU_DrawCrosshair2();

	if(cechotimer)
	{
		int i = 0;
		int y = (BASEVIDHEIGHT/2)-4;
		int numlines = 0;
		char* line;
		char* echoptr;
		char temp[1024];

		cechotimer--;

		while(cechotext[i] != '\0')
		{
			if(cechotext[i] == '\\')
				numlines++;

			i++;
		}

		y -= (numlines-1)*6;
		
		strcpy(temp, cechotext);

		echoptr = &temp[0];

		while(*echoptr != '\0')
		{
			line = strchr(echoptr, '\\');

			if(line == NULL)
				return;

			*line = '\0';

			V_DrawCenteredString(BASEVIDWIDTH/2, y, cechoflags, echoptr);
			y += 12;

			echoptr = line;
			echoptr++;
		}
	}
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

// Clear old messages from the borders around the view window
// (only for reduced view, refresh the borders when needed)
//
// startline: y coord to start clear,
// clearlines: how many lines to clear.
//
static int oldclearlines;

void HU_Erase(void)
{
	int topline, bottomline;
	int y, yoffset;

	// clear hud msgs on double buffer (Glide mode)
	boolean secondframe;
	static int secondframelines;

	if(con_clearlines == oldclearlines && !con_hudupdate && !chat_on)
		return;

	// clear the other frame in double-buffer modes
	secondframe = (con_clearlines != oldclearlines);
	if(secondframe)
		secondframelines = oldclearlines;

	// clear the message lines that go away, so use _oldclearlines_
	bottomline = oldclearlines;
	oldclearlines = con_clearlines;
	if(chat_on)
		if(bottomline < 8)
			bottomline = 8;

	if(automapactive || viewwindowx == 0) // hud msgs don't need to be cleared
		return;

	// software mode copies view border pattern & beveled edges from the backbuffer
	if(rendermode == render_soft)
	{
		topline = 0;
		for(y = topline, yoffset = y*vid.width; y < bottomline; y++, yoffset += vid.width)
		{
			if(y < viewwindowy || y >= viewwindowy + viewheight)
				R_VideoErase(yoffset, vid.width); // erase entire line
			else
			{
				R_VideoErase(yoffset, viewwindowx); // erase left border
				// erase right border
				R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
			}
		}
		con_hudupdate = false; // if it was set..
	}
#ifdef HWRENDER
	else if(rendermode != render_none)
	{
		// refresh just what is needed from the view borders
		HWR_DrawViewBorder(secondframelines);
		con_hudupdate = secondframe;
	}
#endif
}

//======================================================================
//                   IN-LEVEL MULTIPLAYER RANKINGS
//======================================================================

// count frags for each team
int HU_CreateTeamScoresTbl(playersort_t* tab, int dmtotals[])
{
	int i, j, scorelines, team;

	scorelines = 0;
	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(playeringame[i])
		{
			if(cv_teamplay.value == 1)
				team = players[i].skincolor;
			else
				team = players[i].skin;

			for(j = 0; j < scorelines; j++)
				if(tab[j].num == team)
				{
					// found team
					tab[j].count += players[i].score;
					if(dmtotals)
						dmtotals[team] = tab[j].count;
					break;
				}
			if(j == scorelines)
			{
				// team not found, so add it
				tab[scorelines].count = players[i].score;
				tab[scorelines].num = team;
				tab[scorelines].color = players[i].skincolor;
				tab[scorelines].name = team_names[team];

				if(dmtotals)
					dmtotals[team] = tab[scorelines].count;

				scorelines++;
			}
		}
	}
	return scorelines;
}

//
// HU_DrawTabRankings
//
void HU_DrawTabRankings(int x, int y, playersort_t* tab, int scorelines, int whiteplayer)
{
	int i;
	byte* colormap;

	for(i=0; i<scorelines; i++)
	{
		V_DrawString(cv_teamplay.value ? x : x+24, y, (tab[i].num == whiteplayer && !cv_teamplay.value) ? V_WHITEMAP : 0, tab[i].name);

		if(!cv_teamplay.value)
		{
			if(tab[i].color==0)
				colormap = colormaps;
			else
				colormap = (byte *) translationtables[players[tab[i].num].skin] - 256 + (tab[i].color<<8);

			V_DrawSmallScaledPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap); // Tails 03-11-2000
		}

		if(gametype == GT_CTF)
		{
			patch_t* p;

			if(players[tab[i].num].gotflag & MF_REDFLAG) // Red
			{
				p = W_CachePatchName("RFLAGICO", PU_CACHE);
				V_DrawSmallScaledPatch(x-32, y-4, 0, p, colormaps);
			}
			else if(players[tab[i].num].gotflag & MF_BLUEFLAG) // Blue
			{
				p = W_CachePatchName("BFLAGICO", PU_CACHE);
				V_DrawSmallScaledPatch(x-32, y-4, 0, p, colormaps);
			}
		}

		V_DrawRightAlignedString(x+240, y, 0, va("%i", tab[i].count));

		y += 16;
	}
}


//
// HU_DrawRankings
//
static void HU_DrawRankings(void)
{
	patch_t* p;
	playersort_t tab[MAXPLAYERS];
	int i, scorelines, whiteplayer;

	// draw the ranking title panel
/*	if(gametype != GT_CTF)
	{
		p = W_CachePatchName("RESULT", PU_CACHE);
		V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, 5, 0, p);
	}*/

	if(gametype == GT_CTF)
	{
		p = W_CachePatchName("BFLAGICO", PU_CACHE);
		V_DrawSmallScaledPatch(128 - p->width/4, 4, 0, p, colormaps);

		V_DrawCenteredString(128, 16, 0, va("%i", bluescore));

		p = W_CachePatchName("RFLAGICO", PU_CACHE);
		V_DrawSmallScaledPatch(192 - p->width/4, 4, 0, p, colormaps);

		V_DrawCenteredString(192, 16, 0, va("%i", redscore));
	}

	if(cv_timelimit.value && timelimitintics > 0)
	{
		V_DrawCenteredString(64, 8, 0, "TIME LEFT");
		V_DrawCenteredString(64, 16, 0, va("%i", (timelimitintics+1-leveltime)/TICRATE));
	}

	if(cv_pointlimit.value > 0)
	{
		V_DrawCenteredString(256, 8, 0, "POINT LIMIT");
		V_DrawCenteredString(256, 16, 0, va("%i", cv_pointlimit.value));
	}

	// When you play, you quickly see your score because your name is displayed in white.
	// When playing back a demo, you quickly see who's the view.
	whiteplayer = demoplayback ? displayplayer : consoleplayer;

	if(!cv_teamplay.value)
	{
		int j;
		boolean completed[MAXPLAYERS];

		scorelines = 0;
		memset(completed, 0, sizeof(completed));
		memset(tab, 0, sizeof(playersort_t)*MAXPLAYERS);

		for(i=0; i<MAXPLAYERS; i++)
		{
			tab[i].num = -1;
			tab[i].name = 0;
			tab[i].count = -1;
		}

		for(j=0; j<MAXPLAYERS; j++)
		{
			if(!playeringame[j])
				continue;

			for(i = 0; i < MAXPLAYERS; i++)
			{
				if(playeringame[i])
				{
					if(gametype == GT_TAG)
					{
						if(players[i].tagcount >= tab[scorelines].count && completed[i] == false)
						{
							tab[scorelines].count = players[i].tagcount;
							tab[scorelines].num = i;
							tab[scorelines].color = players[i].skincolor;
							tab[scorelines].name = player_names[i];
						}
					}
					else
					{
						if(players[i].score >= tab[scorelines].count && completed[i] == false)
						{
							tab[scorelines].count = players[i].score;
							tab[scorelines].num = i;
							tab[scorelines].color = players[i].skincolor;
							tab[scorelines].name = player_names[i];
						}
					}
				}
			}
			completed[tab[scorelines].num] = true;
			scorelines++;
		}

		if(scorelines > 10)
			scorelines = 10; //dont draw past bottom of screen, show the best only

		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
	}
	else
	{
		memset(tab, 0, sizeof(tab));

		scorelines = HU_CreateTeamScoresTbl(tab, NULL);

		if(scorelines > 10)
			scorelines = 10; // only show 10 best teams

		// and the team frag to the left
		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
		//	cv_teamplay.value == 1 ? players[whiteplayer].skincolor : players[whiteplayer].skin);
		// first param was "Teams"
		// Graue 04-08-2004: only display team counts not individual counts
		// there can be up to 16 teams by color (teamplay 1) or 32 by skin (teamplay 2)
	}
}

static void HU_DrawCoopOverlay(void)
{
	if(!netgame && (!modifiedgame || savemoddata))
	{
		char emblemsfound[20];
		int found = 0;
		int i;

		for(i=0; i<MAXEMBLEMS; i++)
		{
			if(emblemlocations[i].collected)
				found++;
		}

		sprintf(emblemsfound, "- %d/%d", found, numemblems);
		V_DrawString(160, 96, 0, emblemsfound);
		V_DrawScaledPatch(128, 96 - emblemicon->height/4, 0, emblemicon);
	}

	if(emeralds & EMERALD1)
		V_DrawScaledPatch(64, 128, V_TRANSLUCENT, emerald1);
	if(emeralds & EMERALD2)
		V_DrawScaledPatch(96, 128, V_TRANSLUCENT, emerald2);
	if(emeralds & EMERALD3)
		V_DrawScaledPatch(128, 128, V_TRANSLUCENT, emerald3);
	if(emeralds & EMERALD4)
		V_DrawScaledPatch(160, 128, V_TRANSLUCENT, emerald4);
	if(emeralds & EMERALD5)
		V_DrawScaledPatch(192, 128, V_TRANSLUCENT, emerald5);
	if(emeralds & EMERALD6)
		V_DrawScaledPatch(224, 128, V_TRANSLUCENT, emerald6);
	if(emeralds & EMERALD7)
		V_DrawScaledPatch(256, 128, V_TRANSLUCENT, emerald7);
	if(emeralds & EMERALD8)
		V_DrawScaledPatch(160, 144, V_TRANSLUCENT, emerald8);
}

// draw the Crosshair, at the exact center of the view.
//
// Crosshairs are pre-cached at HU_Init

static inline void HU_DrawCrosshair(void)
{
	int i, y;

	i = cv_crosshair.value & 3;
	if(!i)
		return;

	if(gametype == GT_CTF && !players[consoleplayer].ctfteam)
		return;

#ifdef HWRENDER
	if(rendermode != render_soft)
		y = (int)gr_basewindowcentery;
	else
#endif
		y = viewwindowy + (viewheight>>1);

	V_DrawTranslucentPatch(vid.width>>1, y, V_NOSCALESTART, crosshair[i - 1]);
}

static inline void HU_DrawCrosshair2(void)
{
	int i, y;

	i = cv_crosshair2.value & 3;
	if(!i)
		return;

	if(gametype == GT_CTF && !players[secondarydisplayplayer].ctfteam)
		return;

#ifdef HWRENDER
	if(rendermode != render_soft)
		y = (int)gr_basewindowcentery;
	else
#endif
		y = viewwindowy + (viewheight>>1);

	if(cv_splitscreen.value)
	{
#ifdef HWRENDER
		if(rendermode != render_soft)
			y += (int)gr_viewheight;
		else
#endif
			y += viewheight;

		V_DrawTranslucentPatch(vid.width>>1, y, V_NOSCALESTART, crosshair[i - 1]);
	}
}

//======================================================================
//                    CHAT MACROS COMMAND & VARS
//======================================================================

// make sure to call HU_HackChatmacros(), because the strings are NULL
static consvar_t cv_chatmacro1 = {"_chatmacro1", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro2 = {"_chatmacro2", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro3 = {"_chatmacro3", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro4 = {"_chatmacro4", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro5 = {"_chatmacro5", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro6 = {"_chatmacro6", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro7 = {"_chatmacro7", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro8 = {"_chatmacro8", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro9 = {"_chatmacro9", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_chatmacro0 = {"_chatmacro0", NULL, CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

//
// HU_HackChatmacros
//
// Set the original chatmacro text before a dehacked patch is loaded, which would override it.
//
void HU_HackChatmacros(void)
{
	int i;

	// this is either the original text, or dehacked ones
	cv_chatmacro0.defaultvalue = HUSTR_CHATMACRO0;
	cv_chatmacro1.defaultvalue = HUSTR_CHATMACRO1;
	cv_chatmacro2.defaultvalue = HUSTR_CHATMACRO2;
	cv_chatmacro3.defaultvalue = HUSTR_CHATMACRO3;
	cv_chatmacro4.defaultvalue = HUSTR_CHATMACRO4;
	cv_chatmacro5.defaultvalue = HUSTR_CHATMACRO5;
	cv_chatmacro6.defaultvalue = HUSTR_CHATMACRO6;
	cv_chatmacro7.defaultvalue = HUSTR_CHATMACRO7;
	cv_chatmacro8.defaultvalue = HUSTR_CHATMACRO8;
	cv_chatmacro9.defaultvalue = HUSTR_CHATMACRO9;

	// link chatmacros to cvars
	chat_macros[0] = &cv_chatmacro0;
	chat_macros[1] = &cv_chatmacro1;
	chat_macros[2] = &cv_chatmacro2;
	chat_macros[3] = &cv_chatmacro3;
	chat_macros[4] = &cv_chatmacro4;
	chat_macros[5] = &cv_chatmacro5;
	chat_macros[6] = &cv_chatmacro6;
	chat_macros[7] = &cv_chatmacro7;
	chat_macros[8] = &cv_chatmacro8;
	chat_macros[9] = &cv_chatmacro9;

#if (!defined(_WINDOWS) && !defined(SDLMAIN))
	if(dedicated)
		return;
#endif

	// register chatmacro vars ready for config.cfg
	for(i = 0; i < 10; i++)
		CV_RegisterVar(chat_macros[i]);
}

// chatmacro <0-9> "chat message"
//
void Command_Chatmacro_f(void)
{
	int i;

	if(COM_Argc() < 2)
	{
		CONS_Printf("chatmacro <0-9> : view chatmacro\n"
			"chatmacro <0-9> \"chat message\" : change chatmacro\n");
		return;
	}

	i = atoi(COM_Argv(1)) % 10;

	if(COM_Argc() == 2)
	{
		CONS_Printf("chatmacro %d is \"%s\"\n", i, chat_macros[i]->string);
		return;
	}

	// change a chatmacro
	CV_Set(chat_macros[i], COM_Argv(2));
}
