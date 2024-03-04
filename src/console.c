// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
/// \file
/// \brief console drawing, input

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "g_input.h"
#include "hu_stuff.h"
#include "keys.h"
#include "r_defs.h"
#include "sounds.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "i_system.h"
#include "d_main.h"

#ifdef _WINDOWS
#include "win32/win_main.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

static boolean con_started = false; // console has been initialised
       boolean con_startup = false; // true at game startup, screen need refreshing
static boolean con_forcepic = true; // at startup toggle console translucency when first off
       boolean con_recalc;          // set true when screen size has changed

static tic_t con_tick; // console ticker for anim or blinking prompt cursor
                        // con_scrollup should use time (currenttime - lasttime)..

static boolean consoletoggle; // true when console key pushed, ticker will handle
static boolean consoleready;  // console prompt is ready

       int con_destlines; // vid lines used by console at final position
static int con_curlines;  // vid lines currently used by console

       int con_clipviewtop; // clip value for planes & sprites, so that the
                            // part of the view covered by the console is not
                            // drawn when not needed, this must be -1 when
                            // console is off

/// \todo choose max hud msg lines
#define CON_MAXHUDLINES 5

static int con_hudlines;        // number of console heads up message lines
static int con_hudtime[5];      // remaining time of display for hud msg lines

       int con_clearlines;      // top screen lines to refresh when view reduced
       boolean con_hudupdate;   // when messages scroll, we need a backgrnd refresh

// console text output
static char* con_line;          // console text output current line
static size_t con_cx;           // cursor position in current line
static size_t con_cy;           // cursor line number in con_buffer, is always
                                // increasing, and wrapped around in the text
                                // buffer using modulo.

static size_t con_totallines;      // lines of console text into the console buffer
static size_t con_width;           // columns of chars, depend on vid mode width

static size_t con_scrollup;        // how many rows of text to scroll up (pgup/pgdn)

// hold 32 last lines of input for history
#define CON_MAXPROMPTCHARS 256
#define CON_PROMPTCHAR '>'

static char inputlines[32][CON_MAXPROMPTCHARS]; // hold last 32 prompt lines

static int inputline;    // current input line number
static int inputhist;    // line number of history input line to restore
static size_t input_cx;  // position in current input line

static patch_t* con_backpic; // console background picture, loaded static
static pic_t* con_bordleft;
static pic_t* con_bordright; // console borders in translucent mode

// protos.
static void CON_InputInit(void);
static void CON_RecalcSize(void);

static void CONS_speed_Change(void);
static void CON_DrawBackpic(patch_t* pic, int startx, int destwidth);
static void CON_DrawBackpic2(pic_t* pic, int startx, int destwidth);

//======================================================================
//                   CONSOLE VARS AND COMMANDS
//======================================================================
#if defined(__MACOS__) &&  !defined(__MACH__)
#define CON_BUFFERSIZE 4096 // my compiler can't handle local vars >32k
#else
#define CON_BUFFERSIZE 16384
#endif

static char con_buffer[CON_BUFFERSIZE];

// how many seconds the hud messages lasts on the screen
static consvar_t cons_msgtimeout = {"con_hudtime", "5", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

// number of lines console move per frame
static consvar_t cons_speed = {"con_speed", "8", CV_CALL|CV_SAVE, CV_Unsigned, CONS_speed_Change, 0, NULL, NULL, 0, 0, NULL};

// percentage of screen height to use for console
static consvar_t cons_height = {"con_height", "50", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t backpic_cons_t[] = {{0, "translucent"}, {1, "picture"}, {0, NULL}};
// whether to use console background picture, or translucent mode
static consvar_t cons_backpic = {"con_backpic", "translucent", CV_SAVE, backpic_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static void CON_Print(char* msg);

// Check CONS_speed value (must be positive)
//
static void CONS_speed_Change(void)
{
	if(cons_speed.value < 1)
		CV_SetValue(&cons_speed, 1);
}

// Clear console text buffer
//
static void CONS_Clear_f(void)
{
	if(con_buffer)
		memset(con_buffer, 0, CON_BUFFERSIZE);

	con_cx = 0;
	con_cy = con_totallines-1;
	con_line = &con_buffer[con_cy*con_width];
	con_scrollup = 0;
}

// Choose english keymap
//
static void CONS_English_f(void)
{
	shiftxform = english_shiftxform;
	CONS_Printf("English keymap.\n");
}

static char* bindtable[NUMINPUTS];

static void CONS_Bind_f(void)
{
	size_t na;
	int key;

	na = COM_Argc();

	if(na != 2 && na != 3)
	{
		CONS_Printf("bind <keyname> [<command>]\n");
		CONS_Printf("\2bind table :\n");
		na = 0;
		for(key = 0; key < NUMINPUTS; key++)
			if(bindtable[key])
			{
				CONS_Printf("%s : \"%s\"\n", G_KeynumToString(key), bindtable[key]);
				na = 1;
			}
		if(!na)
			CONS_Printf("Empty\n");
		return;
	}

	key = G_KeyStringtoNum(COM_Argv(1));
	if(!key)
	{
		CONS_Printf("Invalid key name\n");
		return;
	}

	if(bindtable[key] != NULL)
	{
		Z_Free(bindtable[key]);
		bindtable[key] = NULL;
	}

	if(na == 3)
		bindtable[key] = Z_StrDup(COM_Argv(2));
}

//======================================================================
//                          CONSOLE SETUP
//======================================================================

// Prepare a colormap for GREEN ONLY translucency over background
//
byte* whitemap;
byte* purplemap;
byte* lgreenmap;
byte* bluemap;
byte* greenmap;
byte* graymap;
byte* redmap;
static void CON_SetupBackColormap(void)
{
	int i, j, k;
	byte* pal;

	greenmap  = (byte*)Z_Malloc(256, PU_STATIC, NULL);
	whitemap  = (byte*)Z_Malloc(256, PU_STATIC, NULL);
	graymap   = (byte*)Z_Malloc(256, PU_STATIC, NULL);
	purplemap = (byte*)Z_Malloc(256, PU_STATIC, NULL);
	lgreenmap = (byte*)Z_Malloc(256, PU_STATIC, NULL);
	bluemap   = (byte*)Z_Malloc(256, PU_STATIC, NULL);
	redmap    = (byte*)Z_Malloc(256, PU_STATIC, NULL);

	pal = W_CacheLumpName("PLAYPAL", PU_CACHE);

	// setup the green translucent background colormap
	for(i = 0, k = 0; i < 768; i += 3, k++)
	{
		j = pal[i] + pal[i+1] + pal[i+2];
		greenmap[k] = (byte)(127 - (j>>6));
	}

	// setup the other colormaps, for console text

	// these don't need to be aligned, unless you convert the
	// V_DrawMappedPatch() into optimised asm.

	for(i = 0; i < 256; i++)
	{
		whitemap[i] = (byte)i; // remap each color to itself...
		graymap[i] = (byte)i;
		purplemap[i] = (byte)i;
		lgreenmap[i] = (byte)i;
		bluemap[i] = (byte)i;
		redmap[i] = (byte)i;
	}

	for(i = 82; i < 83; i++)
	{
		whitemap[i]  = (byte)(i + 149);
		purplemap[i] = (byte)(i + 170);
		lgreenmap[i] = (byte)(i +  32);
		bluemap[i]   = (byte)(i + 114);
		graymap[i]   = (byte)(i -  80);
		redmap[i]    = (byte)(i +  90);
	}
	whitemap[45] = 190 - 88; // colors 45 and 190 are the same
	graymap[45]  = 190 - 80;
	whitemap[47] = 191 - 88; // as are 47 and 191
	graymap[47]  = 191 - 80;
}

// Setup the console text buffer
//
void CON_Init(void)
{
	int i;

	for(i = 0; i < NUMINPUTS; i++)
		bindtable[i] = NULL;

	// clear all lines
	memset(con_buffer, 0, CON_BUFFERSIZE);

	// make sure it is ready for the loading screen
	con_width = 0;
	CON_RecalcSize();

	CON_SetupBackColormap();

	//note: CON_Ticker should always execute at least once before D_Display()
	con_clipviewtop = -1; // -1 does not clip

	con_hudlines = CON_MAXHUDLINES;

	// setup console input filtering
	CON_InputInit();

	// load console background pic
	con_backpic = (patch_t*)W_CacheLumpName("CONSBACK",PU_STATIC);

	// borders MUST be there
	con_bordleft = (pic_t*)W_CacheLumpName("CBLEFT",PU_STATIC);
	con_bordright = (pic_t*)W_CacheLumpName("CBRIGHT",PU_STATIC);

	// register our commands
	//
	COM_AddCommand("cls", CONS_Clear_f);
	COM_AddCommand("english", CONS_English_f);
	// set console full screen for game startup MAKE SURE VID_Init() done !!!
	con_destlines = vid.height;
	con_curlines = vid.height;


	if(!dedicated)
	{
		con_started = true;
		con_startup = true; // need explicit screen refresh until we are in Doom loop
		consoletoggle = false;
		CV_RegisterVar(&cons_msgtimeout);
		CV_RegisterVar(&cons_speed);
		CV_RegisterVar(&cons_height);
		CV_RegisterVar(&cons_backpic);
		COM_AddCommand("bind", CONS_Bind_f);
	}
	else
	{
		con_started = true;
		con_startup = false; // need explicit screen refresh until we are in Doom loop
		consoletoggle = true;
	}
}
// Console input initialization
//
static void CON_InputInit(void)
{
	int i;

	// prepare the first prompt line
	memset(inputlines, 0, sizeof(inputlines));
	for(i = 0; i < 32; i++)
		inputlines[i][0] = CON_PROMPTCHAR;
	inputline = 0;
	input_cx = 1;
}

//======================================================================
//                        CONSOLE EXECUTION
//======================================================================

// Called at screen size change to set the rows and line size of the
// console text buffer.
//
static void CON_RecalcSize(void)
{
	size_t conw, oldcon_width, oldnumlines, i, oldcon_cy;
	char tmp_buffer[CON_BUFFERSIZE];
	char string[CON_BUFFERSIZE]; // BP: it is a line but who know

	con_recalc = false;

	conw = (vid.width>>3) - 2;

	if(con_curlines == 200) // first init
	{
		con_curlines = vid.height;
		con_destlines = vid.height;
	}

	// check for change of video width
	if(conw == con_width)
		return; // didn't change

	oldcon_width = con_width;
	oldnumlines = con_totallines;
	oldcon_cy = con_cy;
	memcpy(tmp_buffer, con_buffer, CON_BUFFERSIZE);

	if(conw < 1)
		con_width = (BASEVIDWIDTH>>3) - 2;
	else
		con_width = conw;

	con_width += 11; // Graue 06-19-2004 up to 11 control chars per line

	con_totallines = CON_BUFFERSIZE / con_width;
	memset(con_buffer, ' ', CON_BUFFERSIZE);

	con_cx = 0;
	con_cy = con_totallines-1;
	con_line = &con_buffer[con_cy*con_width];
	con_scrollup = 0;

	// re-arrange console text buffer to keep text
	if(oldcon_width) // not the first time
	{
		for(i = oldcon_cy + 1; i < oldcon_cy + oldnumlines; i++)
		{
			if(tmp_buffer[(i%oldnumlines)*oldcon_width])
			{
				memcpy(string, &tmp_buffer[(i%oldnumlines)*oldcon_width], oldcon_width);
				conw = oldcon_width - 1;
				while(string[conw] == ' ' && conw)
					conw--;
				string[conw+1] = '\n';
				string[conw+2] = '\0';
				CON_Print(string);
			}
		}
	}
}

// Handles Console moves in/out of screen (per frame)
//
static void CON_MoveConsole(void)
{
	// up/down move to dest
	if(con_curlines < con_destlines)
	{
		con_curlines += cons_speed.value/NEWTICRATERATIO;
		if(con_curlines > con_destlines)
			con_curlines = con_destlines;
	}
	else if(con_curlines > con_destlines)
	{
		con_curlines -= cons_speed.value/NEWTICRATERATIO;
		if(con_curlines < con_destlines)
			con_curlines = con_destlines;
	}
}

// Clear time of console heads up messages
//
void CON_ClearHUD(void)
{
	int i;

	for(i = 0; i < con_hudlines; i++)
		con_hudtime[i] = 0;
}

// Force console to move out immediately
// note: con_ticker will set consoleready false
void CON_ToggleOff(void)
{
	if(!con_destlines)
		return;

	con_destlines = 0;
	con_curlines = 0;
	CON_ClearHUD();
	con_forcepic = 0;
	con_clipviewtop = -1; // remove console clipping of view
}

// Console ticker: handles console move in/out, cursor blinking
//
void CON_Ticker(void)
{
	int i;

	// cursor blinking
	con_tick++;
	con_tick &= 7;

	// console key was pushed
	if(consoletoggle)
	{
		consoletoggle = false;

		// toggle off console
		if(con_destlines > 0)
		{
			con_destlines = 0;
			CON_ClearHUD();
		}
		else
		{
			// toggle console in
			con_destlines = (cons_height.value*vid.height)/100;
			if(con_destlines < 20)
				con_destlines = 20;
			else if(con_destlines > vid.height)
				con_destlines = vid.height;

			con_destlines &= ~0x3; // multiple of text row height
		}
	}

	// console movement
	if(con_destlines != con_curlines)
		CON_MoveConsole();

	// clip the view, so that the part under the console is not drawn
	con_clipviewtop = -1;
	if(cons_backpic.value) // clip only when using an opaque background
	{
		if(con_curlines > 0)
			con_clipviewtop = con_curlines - viewwindowy - 1 - 10;
		// NOTE: BIG HACK::SUBTRACT 10, SO THAT WATER DON'T COPY LINES OF THE CONSOLE
		//       WINDOW!!! (draw some more lines behind the bottom of the console)
		if(con_clipviewtop < 0)
			con_clipviewtop = -1; // maybe not necessary, provided it's < 0
	}

	// check if console ready for prompt
	if(con_destlines >= 20)
		consoleready = true;
	else
		consoleready = false;

	// make overlay messages disappear after a while
	for(i = 0; i < con_hudlines; i++)
	{
		con_hudtime[i]--;
		if(con_hudtime[i] < 0)
			con_hudtime[i] = 0;
	}
}

// Handles console key input
//
boolean CON_Responder(event_t* ev)
{
	static boolean shiftdown;

	// sequential completions a la 4dos
	static char completion[80];
	static int comskips, varskips;

	const char* cmd = "";
	int key;

	if(chat_on)
		return false;

	// special keys state
	if(ev->data1 == KEY_SHIFT && ev->type == ev_keyup)
	{
		shiftdown = false;
		return false;
	}

	// let go keyup events, don't eat them
	if(ev->type != ev_keydown)
		return false;

	key = ev->data1;

	// check for console toggle key
	if(key == gamecontrol[gc_console][0] || key == gamecontrol[gc_console][1])
	{
		consoletoggle = true;
		return true;
	}

	// check other keys only if console prompt is active
	if(!consoleready && key < NUMINPUTS) // metzgermeister: boundary check!!
	{
		if(bindtable[key])
		{
			COM_BufAddText(bindtable[key]);
			COM_BufAddText("\n");
			return true;
		}
		return false;
	}

	// eat shift only if console active
	if(key == KEY_SHIFT)
	{
		shiftdown = true;
		return true;
	}

	// escape key toggle off console
	if(key == KEY_ESCAPE)
	{
		consoletoggle = true;
		return true;
	}

	// command completion forward (tab) and backward (shift-tab)
	if(key == KEY_TAB)
	{
		// sequential command completion forward and backward

		// remember typing for several completions (a-la-4dos)
		if(inputlines[inputline][input_cx-1] != ' ')
		{
			if(strlen(inputlines[inputline]+1) < 80)
				strcpy(completion, inputlines[inputline]+1);
			else
				completion[0] = 0;

			comskips = varskips = 0;
		}
		else
		{
			if(shiftdown)
			{
				if(comskips < 0)
				{
					if(--varskips < 0)
						comskips = -comskips - 2;
				}
				else if(comskips > 0)
					comskips--;
			}
			else
			{
				if(comskips < 0)
					varskips++;
				else
					comskips++;
			}
		}

		if(comskips >= 0)
		{
			cmd = COM_CompleteCommand(completion, comskips);
			if(!cmd)
				// dirty:make sure if comskips is zero, to have a neg value
				comskips = -comskips - 1;
		}
		if(comskips < 0)
			cmd = CV_CompleteVar(completion, varskips);

		if(cmd)
		{
			memset(inputlines[inputline]+1, 0, CON_MAXPROMPTCHARS-1);
			strcpy(inputlines[inputline]+1, cmd);
			input_cx = strlen(cmd) + 1;
			inputlines[inputline][input_cx] = ' ';
			input_cx++;
			inputlines[inputline][input_cx] = 0;
		}
		else
		{
			if(comskips > 0)
				comskips--;
			else if(varskips > 0)
				varskips--;
		}

		return true;
	}

	// move up (backward) in console textbuffer
	if(key == KEY_PGUP)
	{
		if(con_scrollup < (con_totallines-((con_curlines-16)>>3)))
			con_scrollup++;
		return true;
	}
	else if(key == KEY_PGDN)
	{
		if(con_scrollup > 0)
			con_scrollup--;
		return true;
	}

	if(key == KEY_HOME) // oldest text in buffer
	{
		con_scrollup = (con_totallines-((con_curlines-16)>>3));
		return true;
	}
	else if(key == KEY_END) // most recent text in buffer
	{
		con_scrollup = 0;
		return true;
	}

	// command enter
	if(key == KEY_ENTER)
	{
		if(input_cx < 2)
			return true;

		// push the command
		COM_BufAddText(inputlines[inputline]+1);
		COM_BufAddText("\n");

		CONS_Printf("%s\n", inputlines[inputline]);

		inputline = (inputline+1) & 31;
		inputhist = inputline;

		memset(inputlines[inputline], 0, CON_MAXPROMPTCHARS);
		inputlines[inputline][0] = CON_PROMPTCHAR;
		input_cx = 1;

		return true;
	}

	// backspace command prompt
	if(key == KEY_BACKSPACE)
	{
		if(input_cx > 1)
		{
			input_cx--;
			inputlines[inputline][input_cx] = 0;
		}
		return true;
	}

	// move back in input history
	if(key == KEY_UPARROW)
	{
		// copy one of the previous inputlines to the current
		do
		{
			inputhist = (inputhist - 1) & 31; // cycle back
		} while(inputhist != inputline && !inputlines[inputhist][1]);

		// stop at the last history input line, which is the
		// current line + 1 because we cycle through the 32 input lines
		if(inputhist == inputline)
			inputhist = (inputline + 1) & 31;

		memcpy(inputlines[inputline], inputlines[inputhist], CON_MAXPROMPTCHARS);
		input_cx = strlen(inputlines[inputline]);

		return true;
	}

	// move forward in input history
	if(key == KEY_DOWNARROW)
	{
		if(inputhist == inputline)
			return true;
		do
		{
			inputhist = (inputhist + 1) & 31;
		} while(inputhist != inputline && !inputlines[inputhist][1]);

		memset(inputlines[inputline], 0, CON_MAXPROMPTCHARS);

		// back to currentline
		if(inputhist == inputline)
		{
			inputlines[inputline][0] = CON_PROMPTCHAR;
			input_cx = 1;
		}
		else
		{
			strcpy(inputlines[inputline], inputlines[inputhist]);
			input_cx = strlen(inputlines[inputline]);
		}
		return true;
	}

	// allow people to use keypad in console (good for typing IP addresses) - Calum
	if(key >= KEY_KEYPAD7 && key <= KEY_KPADDEL)
	{
		char keypad_translation[] = {'7','8','9','-',
		                             '4','5','6','+',
		                             '1','2','3',
		                             '0','.'};

		key = keypad_translation[key - KEY_KEYPAD7];
	}
	else if(key == KEY_KPADSLASH)
		key = '/';

	if(shiftdown)
		key = shiftxform[key];

	// enter a char into the command prompt
	if(key < 32 || key > 127)
		return false;

	// add key to cmd line here
	if(input_cx < CON_MAXPROMPTCHARS)
	{
		// make sure letters are lowercase for commands & cvars
		if(key >= 'A' && key <= 'Z')
			key = key + 'a' - 'A';

		inputlines[inputline][input_cx] = (char)key;
		inputlines[inputline][input_cx + 1] = 0;
		input_cx++;
	}

	return true;
}

// Insert a new line in the console text buffer
//
static void CON_Linefeed(void)
{
	// set time for heads up messages
	con_hudtime[con_cy%con_hudlines] = cons_msgtimeout.value*TICRATE;

	con_cy++;
	con_cx = 0;

	con_line = &con_buffer[(con_cy%con_totallines)*con_width];
	memset(con_line, ' ', con_width);

	// make sure the view borders are refreshed if hud messages scroll
	con_hudupdate = true; // see HU_Erase()
}

// Outputs text into the console text buffer
static void CON_Print(char* msg)
{
	size_t l;
	int controlchars = 0; // for color changing

	if(*msg == '\3') // chat text, makes ding sound
		S_StartSound(0, sfx_radio);
	else if(*msg == '\4') // chat action, dings and is in purple
	{
		*msg = '\x81'; // purple
		S_StartSound(0, sfx_radio);
	}

	if(!(*msg & 0x80))
	{
		con_line[con_cx++] = '\x80';
		controlchars = 1;
	}

	while(*msg)
	{
		// skip non-printable characters and white spaces
		while(*msg && *msg <= ' ')
		{
			if(*msg & 0x80)
			{
				con_line[con_cx++] = *(msg++);
				controlchars++;
				continue;
			}
			else if(*msg == '\r') // carriage return
			{
				con_cy--;
				CON_Linefeed();
				controlchars = 0;
			}
			else if(*msg == '\n') // linefeed
			{
				CON_Linefeed();
				controlchars = 0;
			}
			else if(*msg == ' ') // space
			{
				con_line[con_cx++] = ' ';
				if(con_cx - controlchars >= con_width-11)
				{
					CON_Linefeed();
					controlchars = 0;
				}
			}
			else if(*msg == '\t')
			{
				// adds tab spaces for nice layout in console

				do
				{
					con_line[con_cx++] = ' ';
				} while((con_cx - controlchars) % 4 != 0);

				if(con_cx - controlchars >= con_width-11)
				{
					CON_Linefeed();
					controlchars = 0;
				}
			}
			msg++;
		}

		if(*msg == '\0')
			return;

		// printable character
		for(l = 0; l < (con_width-11) && msg[l] > ' '; l++)
			;

		// word wrap
		if((con_cx - controlchars) + l > con_width-11)
		{
			CON_Linefeed();
			controlchars = 0;
		}

		// a word at a time
		for(; l > 0; l--)
			con_line[con_cx++] = *(msg++);
	}
}

#ifdef LOGMESSAGES
void CON_LogMessage(const char* msg)
{
	const char* p;

	for(p = msg; *p != '\0'; p++)
		if(*p == '\n' || *p >= ' ') // don't log CON_Print's control characters
		{
#ifdef _WINDOWS
			if(*p == '\n' && ( p == msg || *(p-1) != '\r')) FPutChar(logstream, "\r");
			FPutChar(logstream, p);
#elif defined(SDLIO)
			SDL_RWwrite(logstream, p, 1, 1);
#else
			write(logstream, p, 1);
#endif
		}
}
#endif

// Console print! Wahooo! Lots o fun!
//
void CONS_Printf(const char* fmt, ...)
{
	va_list argptr;
	char txt[8192];

	va_start(argptr, fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

	// echo console prints to log file
#ifdef LOGMESSAGES
	if(logstream != INVALID_HANDLE_VALUE)
		CON_LogMessage(txt);
#endif
	DEBFILE(txt);

	if(!con_started)
	{
#if !defined(_WINDOWS) && !(defined(__OS2__) && !defined(SDL))// && !defined(WATTCP)
		I_OutputMsg("%s",txt);
#endif
		return;
	}
	else
		// write message in con text buffer
		CON_Print(txt);
#if (defined(SDL) || defined(LINUX)) && !defined(_arch_dreamcast)
	I_OutputMsg("%s",txt);
#endif

	// make sure new text is visible
	con_scrollup = 0;

	// if not in display loop, force screen update
	if(con_startup)
	{
#if(defined(_WINDOWS) ) || (defined(__OS2__) && !defined(SDL))
		// show startup screen and message using only 'software' graphics
		// (rendermode may be hardware accelerated, but the video mode is not set yet)
		CON_DrawBackpic(con_backpic, 0, vid.width); // put console background
		I_LoadingScreen(txt);
#else
		// here we display the console background and console text
		// (no hardware accelerated support for these versions)
		CON_Drawer();
		I_FinishUpdate(); // page flip or blit buffer
#endif
	}
}

// Print an error message, and wait for ENTER key to continue.
// To make sure the user has seen the message
//
void CONS_Error(const char* msg)
{
#if (defined (_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)
#ifdef _WINDOWS
	if(!graphics_started)
	{
		MessageBox(hWndMain, msg, "SRB2 Warning", MB_OK);
		return;
	}
#endif
#ifdef SDLMAIN
	if(!graphics_started)
	{
		MessageBox(NULL, msg, "SRB2 Warning", MB_OK);
		return;
	}
#endif
#endif
	CONS_Printf("\2%s", msg); // write error msg in different colour
	CONS_Printf("Press ENTER to continue\n");

	// dirty quick hack, but for the good cause
	while(I_GetKey() != KEY_ENTER)
		;
}

//======================================================================
//                          CONSOLE DRAW
//======================================================================

// draw console prompt line
//
static void CON_DrawInput(void)
{
	char* p;
	size_t x;
	int y;

	// input line scrolls left if it gets too long
	p = inputlines[inputline];
	if(input_cx >= con_width-11)
		p += input_cx - (con_width-11) + 1;

	y = con_curlines - 12;

	for(x = 0; x < con_width-11; x++)
		V_DrawCharacter((int)(x+1)<<3, y, p[x]|V_NOSCALEPATCH|V_NOSCALESTART);

	// draw the blinking cursor
	//
	x = (input_cx >= con_width-11) ? (con_width-11) - 1 : input_cx;
	if(con_tick < 4)
		V_DrawCharacter((int)(x+1)<<3, y, '_'|V_NOSCALEPATCH|V_NOSCALESTART);
}

// draw the last lines of console text to the top of the screen
static void CON_DrawHudlines(void)
{
	unsigned char* p;
	size_t i, x;
	int y;
	int charflags = 0;

	if(con_hudlines <= 0)
		return;

	if(chat_on)
		y = 8; // leave place for chat input in the first row of text
	else
		y = 0;

	for(i = con_cy - con_hudlines+1; i <= con_cy; i++)
	{
		if((signed)i < 0)
			continue;
		if(con_hudtime[i%con_hudlines] == 0)
			continue;

		p = (unsigned char *)&con_buffer[(i%con_totallines)*con_width];

		for(x = 0; x < con_width; x++, p++)
		{
			while(*p & 0x80) // Graue 06-19-2004
			{
				charflags = (*p & 0x7f) << 8;
				p++;
			}
			V_DrawCharacter((int)(x)<<3, y, (int)(*p) | charflags | V_NOSCALEPATCH|V_NOSCALESTART);
		}

		V_DrawCharacter((int)(x)<<3, y, (p[x]&0xff)|V_NOSCALEPATCH|V_NOSCALESTART);
		y += 8;
	}

	// top screen lines that might need clearing when view is reduced
	con_clearlines = y; // this is handled by HU_Erase();
}

// Scale a pic_t at 'startx' pos, to 'destwidth' columns.
//   startx, destwidth is resolution dependent
// Used to draw console borders, console background.
// The pic must be sized BASEVIDHEIGHT height.
static void CON_DrawBackpic(patch_t* pic, int startx, int destwidth)
{
	startx = destwidth = 0;
	V_DrawScaledPatch(0, 0, 0, pic);
}

static void CON_DrawBackpic2(pic_t* pic, int startx, int destwidth)
{
	int x, y;
	int v;
	byte* src;
	byte* dest;
	int frac, fracstep;

	dest = screens[0]+startx;

	for(y = 0; y < con_curlines; y++, dest += vid.width)
	{
		// scale the picture to the resolution
		v = SHORT(pic->height) - ((con_curlines - y) * (BASEVIDHEIGHT-1) / vid.height) - 1;

		src = pic->data + v*SHORT(pic->width);

		// in case of the console backpic, simplify
		if(pic->width == destwidth)
			memcpy(dest, src, destwidth);
		else
		{
			// scale pic to screen width
			frac = 0;
			fracstep = (SHORT(pic->width)<<16)/destwidth;
			for(x = 0; x < destwidth; x += 4)
			{
				dest[x] = src[frac>>16];
				frac += fracstep;
				dest[x+1] = src[frac>>16];
				frac += fracstep;
				dest[x+2] = src[frac>>16];
				frac += fracstep;
				dest[x+3] = src[frac>>16];
				frac += fracstep;
			}
		}
	}
}

// draw the console background, text, and prompt if enough place
//
static void CON_DrawConsole(void)
{
	unsigned char* p;
	size_t i, x;
	int y;
	int w = 0, x2 = 0;
	int charflags = 0;

	if(con_curlines <= 0)
		return;

	//FIXME: refresh borders only when console bg is translucent
	con_clearlines = con_curlines; // clear console draw from view borders
	con_hudupdate = true; // always refresh while console is on

	// draw console background
	if(cons_backpic.value || con_forcepic)
	{
		if(rendermode != render_soft)
			V_DrawScaledPatch(0, 0, 0, con_backpic);
		else if(rendermode != render_none)
			CON_DrawBackpic(con_backpic, 0, vid.width); // picture as background
	}
	else
	{
		if(rendermode == render_soft)
		{
			w = vid.dupx * 8;
			x2 = vid.width - w;
			CON_DrawBackpic2(con_bordleft, 0, w);
			CON_DrawBackpic2(con_bordright, x2, w);
		}
		// Hurdler: what's the correct value of w and x2 in hardware mode???
		if(rendermode != render_none) V_DrawFadeConsBack(w, 0, x2, con_curlines); // translucent background
	}

	// draw console text lines from top to bottom
	if(con_curlines < 20) // 8+8+4
		return;

	i = con_cy - con_scrollup;

	// skip the last empty line due to the cursor being at the start of a new line
	if(!con_scrollup && !con_cx)
		i--;

	i -= (con_curlines - 20) / 8;

	if(rendermode == render_none) return;

	for(y = (con_curlines-20) % 8; y <= con_curlines-20; y += 8, i++)
	{
		p = (unsigned char*)&con_buffer[((i>0?i:0)%con_totallines)*con_width];

		for(x = 0; x < con_width; x++, p++)
		{
			while(*p & 0x80)
			{
				charflags = (*p & 0x7f) << 8;
				p++;
			}
			V_DrawCharacter((int)(x+1)<<3, y, (int)(*p)|charflags|V_NOSCALEPATCH|V_NOSCALESTART);
		}
	}

	// draw prompt if enough place (not while game startup)
	if((con_curlines == con_destlines) && (con_curlines >= 20) && !con_startup)
		CON_DrawInput();
}

// Console refresh drawer, call each frame
//
void CON_Drawer(void)
{
	if(!con_started || !graphics_started)
		return;

	if(con_recalc)
		CON_RecalcSize();

	// patch 'I' letter leftoffset so it centers
	hu_font['I'-HU_FONTSTART]->leftoffset = -2;

	if(con_curlines > 0)
		CON_DrawConsole();
	else if(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE)
		CON_DrawHudlines();

	hu_font['I'-HU_FONTSTART]->leftoffset = 0;
}
