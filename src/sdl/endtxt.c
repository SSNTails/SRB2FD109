/*
 * Function to write the SRB2 end message text
 *
 * Copyright (C) 1998 by Udo Munk <udo@umserver.umnet.de>
 *
 * This code is provided AS IS and there are no guarantees, none.
 * Feel free to share and modify.
 */
//-----------------------------------------------------------------------------
/// \file
/// \brief Support to show ENDOOM text
///
/// Loads the lump ENDOOM, set up the console to print
/// out the colors and text

#include <stdio.h>
#include <stdlib.h>

// need this 19990118 by Kin
#include "../doomdef.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "endtxt.h"
/**	\brief	The ShowEndTxt function


  Prints out the ENDOOM the way DOOM.EXE/DOOM2.EXE did for Win32 or Linux/GNU

	\return	void

	
*/

void ShowEndTxt()
{
#ifndef _WIN32_WCE
	int i;
	unsigned short j, att = 0;
	int nlflag = 1;
#if defined(_WIN32) || defined(_WIN64)
	HANDLE contextout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO backupcon;
	COORD resizewin = {80,-1};
#endif
	unsigned short *text;
	void *data;
	int endoomnum = W_GetNumForName("ENDOOM");
//	char *col;

	/* if the xterm has more then 80 columns we need to add nl's */
	/* doesn't work, COLUMNS is not in the environment at this time ???
	col = getenv("COLUMNS");
	if (col) {
		if (atoi(col) > 80)
			nlflag++;
	}
	*/

	/* get the lump with the text */
	data = text = W_CacheLumpNum(endoomnum, PU_CACHE);

#if defined(_WIN32) || defined(_WIN64)
	if(contextout == (HANDLE)(-1)) // test if it a good handle
	{
		Z_Free(data);
		return;
	}
	else
	{
		backupcon.wAttributes = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE; // Just in case
		GetConsoleScreenBufferInfo(contextout, &backupcon); //Store old state
		resizewin.Y = backupcon.dwSize.Y;
		if(backupcon.dwSize.X < resizewin.X)
			SetConsoleScreenBufferSize(contextout, resizewin);
	}
	for (i=1; i<=80*25; i++) // print 80x25 text and deal with the attributes too
	{
		j = (unsigned short)(*text >> 8); // attribute first
		if (j != att) // attribute changed?
		{
			att = j; // save current attribute
			SetConsoleTextAttribute(contextout, j); //set fg and bg color for buffer
		}

		putchar(*text++ & 0xff); // now the text

		if (nlflag && !(i % 80) && backupcon.dwSize.X > resizewin.X) // do we need a nl?
		{
			att = backupcon.wAttributes;
			SetConsoleTextAttribute(contextout, att); // all attributes off
			printf("\n");
		}
	}
	SetConsoleTextAttribute(contextout, backupcon.wAttributes); // all attributes off
#else
	/* print 80x25 text and deal with the attributes too */
	for (i=1; i<=80*25; i++) {
		/* attribute first */
		/* attribute changed? */
		if ((j = *text >> 8) != att) {
			/* save current attribute */
			att = j;
			/* set new attribute, forground color first */
			printf("\033[");
			switch (j & 0x0f) {
			case 0:		/* black */
				printf("30");
				break;
			case 1:		/* blue */
				printf("34");
				break;
			case 2:		/* green */
				printf("32");
				break;
			case 3:		/* cyan */
				printf("36");
				break;
			case 4:		/* red */
				printf("31");
				break;
			case 5:		/* magenta */
				printf("35");
				break;
			case 6:		/* brown */
				printf("33");
				break;
			case 7:		/* bright grey */
				printf("37");
				break;
			case 8:		/* dark grey */
				printf("1;30");
				break;
			case 9:		/* bright blue */
				printf("1;34");
				break;
			case 10:	/* bright green */
				printf("1;32");
				break;
			case 11:	/* bright cyan */
				printf("1;36");
				break;
			case 12:	/* bright red */
				printf("1;31");
				break;
			case 13:	/* bright magenta */
				printf("1;35");
				break;
			case 14:	/* yellow */
				printf("1;33");
				break;
			case 15:	/* white */
				printf("1;37");
				break;
			}
			printf("m");
			/* now background color */
			printf("\033[");
			switch((j >> 4) & 0x0f) {
			case 0:		/* black */
				printf("40");
				break;
			case 1:		/* blue */
				printf("44");
				break;
			case 2:		/* green */
				printf("42");
				break;
			case 3:		/* cyan */
				printf("46");
				break;
			case 4:		/* red */
				printf("41");
				break;
			case 5:		/* magenta */
				printf("45");
				break;
			case 6:		/* brown */
				printf("43");
				break;
			case 7:		/* bright grey */
				printf("47");
				break;
			case 8:		/* dark grey */
				printf("1;40");
				break;
			case 9:		/* bright blue */
				printf("1;44");
				break;
			case 10:	/* bright green */
				printf("1;42");
				break;
			case 11:	/* bright cyan */
				printf("1;46");
				break;
			case 12:	/* bright red */
				printf("1;41");
				break;
			case 13:	/* bright magenta */
				printf("1;45");
				break;
			case 14:	/* yellow */
				printf("1;43");
				break;
			case 15:	/* white */
				printf("1;47");
				break;
			}
			printf("m");
		}

		/* now the text */
		putchar(*text++ & 0xff);

		/* do we need a nl? */
		if (nlflag)
		{
			if (!(i % 80))
			{
				printf("\033[0m");
				att = 0;
				printf("\n");
			}
		}
	}
	/* all attributes off */
	printf("\033[0m");
#endif
	if (nlflag)
		printf("\n");

	Z_Free(data);
#endif
}
