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
/// \brief Load dehacked file and change tables and text

#include "doomdef.h"
#include "g_game.h"
#include "sounds.h"
#include "info.h"
#include "d_think.h"
#include "dstrings.h"
#include "m_argv.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_menu.h"
#include "m_misc.h"
#include "f_finale.h"
#include "dehacked.h"
#include "st_stuff.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif

#if !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(__CYGWIN__) && !defined(_MSC_VER)
/** Converts a string to uppercase in-place.
  * Replaces strupr() from the Microsoft C runtime.
  *
  * \param n String to uppercase.
  * \return Pointer to the same string passed in, now in all caps.
  * \author Alam Arias
  */
static char* strup1(char* n)
{
	int i;
	char* upr = n;
	for(i = 0; n[i]; i++)
		upr[i] = toupper(n[i]);
	return upr;
}
#undef strupr
#define strupr(n) strup1(n)
#endif

boolean deh_loaded = false;
boolean modcredits = false; // Whether a mod creator's name will show in the credits.

#define MAXLINELEN 1024

// the code was first write for a file
// converted to use memory with this functions
typedef struct
{
	char* data;
	char* curpos;
	int size;
} MYFILE;

#define myfeof(a) (a->data + a->size <= a->curpos)

static char* myfgets(char* buf, int bufsize, MYFILE* f)
{
	int i = 0;
	if(myfeof(f))
		return NULL;
	// we need one byte for a null terminated string
	bufsize--;
	while(i < bufsize && !myfeof(f))
	{
		char c = *f->curpos++;
		if(c != '\r')
			buf[i++] = c;
		if(c == '\n')
			break;
	}
	buf[i] = '\0';

	return buf;
}

static char* myhashfgets(char* buf, size_t bufsize, MYFILE* f)
{
	size_t i = 0;
	if(myfeof(f))
		return NULL;
	// we need one byte for a null terminated string
	bufsize--;
	while(i < bufsize && !myfeof(f))
	{
		char c = *f->curpos++;
		if(c != '\r')
			buf[i++] = c;
		if(c == '#')
			break;
	}
	i++;
	buf[i] = '\0';

	return buf;
}

static int deh_num_error = 0;

FUNCPRINTF static void deh_error(const char* first, ...)
{
	va_list argptr;

	if(devparm || cv_debug)
	{
		char buf[1000];

		va_start(argptr, first);
		vsprintf(buf, first, argptr);
		va_end(argptr);

		CONS_Printf("%s\n", buf);
	}

	deh_num_error++;
}

/* ======================================================================== */
// Load a dehacked file format
/* ======================================================================== */
/* a sample to see
                   Thing 1 (Player)       {           // MT_PLAYER
int doomednum;     ID # = 3232              -1,             // doomednum
int spawnstate;    Initial frame = 32       S_PLAY,         // spawnstate
int spawnhealth;   Hit points = 3232        100,            // spawnhealth
int seestate;      First moving frame = 32  S_PLAY_RUN1,    // seestate
int seesound;      Alert sound = 32         sfx_None,       // seesound
int reactiontime;  Reaction time = 3232     0,              // reactiontime
int attacksound;   Attack sound = 32        sfx_None,       // attacksound
int painstate;     Injury frame = 32        S_PLAY_PAIN,    // painstate
int painchance;    Pain chance = 3232       255,            // painchance
int painsound;     Pain sound = 32          sfx_plpain,     // painsound
int meleestate;    Close attack frame = 32  S_NULL,         // meleestate
int missilestate;  Far attack frame = 32    S_PLAY_ATK1,    // missilestate
int deathstate;    Death frame = 32         S_PLAY_DIE1,    // deathstate
int xdeathstate;   Exploding frame = 32     S_PLAY_XDIE1,   // xdeathstate
int deathsound;    Death sound = 32         sfx_pldeth,     // deathsound
int speed;         Speed = 3232             0,              // speed
int radius;        Width = 211812352        16*FRACUNIT,    // radius
int height;        Height = 211812352       56*FRACUNIT,    // height
int mass;          Mass = 3232              100,            // mass
int damage;        Missile damage = 3232    0,              // damage
int activesound;   Action sound = 32        sfx_None,       // activesound
int flags;         Bits = 3232              MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
int raisestate;    Respawn frame = 32       S_NULL          // raisestate
                                         }, */

static int searchvalue(const char* s)
{
	while(s[0] != '=' && s[0])
		s++;
	if(s[0] == '=')
		return atoi(&s[1]);
	else
	{
		deh_error("No value found\n");
		return 0;
	}
}

#ifdef HWRENDER
static float searchfvalue(const char* s)
{
	while(s[0] != '=' && s[0])
		s++;
	if(s[0] == '=')
		return (float)atof(&s[1]);
	else
	{
		deh_error("No value found\n");
		return 0;
	}
}
#endif


/*
// Edits an animated texture slot on the array
// Tails 12-27-2003
static void readAnimTex(MYFILE* f,int num)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	int i;

	do {
		if(myfgets(s,sizeof(s),f)!=NULL)
		{
			if(s[0]=='\n') break;
			// set the value in apropriet field
			word=strupr(strtok(s, " "));

			word2=strupr(strtok(NULL, " = "));

			word2[strlen(word2)-1]='\0';

			i = atoi(word2);

			if(!strcmp(word, "START"))
			{
				strncpy(harddefs[num].startname, word2, 8);
			}
			if(!strcmp(word, "END"))
			{
				strncpy(harddefs[num].endname, word2, 8);
			}
			else if(!strcmp(word, "SPEED"))	  harddefs[num].speed = i;
			else if(!strcmp(word, "ISTEXTURE")) harddefs[num].istexture = i;

			else deh_error("readAnimTex %d: unknown word '%s'\n",num,word);
		}
	} while(s[0]!='\n' && !myfeof(f)); //finish when the line is empty
}
*/

// Reads a player.
// For modifying the character select screen
static void readPlayer(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	int i;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			word = strupr(strtok(s, " "));

			if(!strcmp(word, "PLAYERTEXT"))
			{
				char* playertext = NULL;
				for(i = 0; i < MAXLINELEN; i++)
					if(s[i] == '=')
					{
						playertext = &s[i+2];
						break;
					}
				for(i = 0; i < MAXLINELEN; i++)
				{
					if(s[i] == '\0')
					{
						s[i] = '\n';
						s[i+1] = '\0';
						break;
					}
				}

				strcpy(description[num].info, playertext);
				strcat(description[num].info,
					myhashfgets(playertext, sizeof(description[num].info), f));
				continue;
			}

			word2 = strupr(strtok(NULL, " = "));
			word2[strlen(word2)-1] = '\0';
			i = atoi(word2);

			if(!strcmp(word, "PLAYERNAME"))
			{
				strncpy(&description[num].text[0], word2, 64);
				PlayerMenu[num].text = &description[num].text[0];
			}
			else if(!strcmp(word, "MENUPOSITION"))
				PlayerMenu[num].alphaKey = (byte)i;
			else if(!strcmp(word, "PICNAME"))
				strncpy(&description[num].picname[0], word2, 9);
			else if(!strcmp(word, "STATUS"))
				PlayerMenu[num].status = (short)i;
			else if(!strcmp(word, "SKINNAME"))
				strcpy(&description[num].skinname[0], word2);

			else
				deh_error("readPlayer %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

static void readthing(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	int value;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;
			value = searchvalue(s);

			word = strupr(strtok(s, " "));
			if(!strcmp(word, "MAPTHINGNUM"))       mobjinfo[num].doomednum = value;
			else if(!strcmp(word, "SPAWNSTATE"))   mobjinfo[num].spawnstate = value;
			else if(!strcmp(word, "SPAWNHEALTH"))  mobjinfo[num].spawnhealth = value;
			else if(!strcmp(word, "SEESTATE"))     mobjinfo[num].seestate = value;
			else if(!strcmp(word, "SEESOUND"))     mobjinfo[num].seesound = value;
			else if(!strcmp(word, "REACTIONTIME")) mobjinfo[num].reactiontime = value;
			else if(!strcmp(word, "ATTACKSOUND"))  mobjinfo[num].attacksound = value;
			else if(!strcmp(word, "PAINSTATE"))    mobjinfo[num].painstate = value;
			else if(!strcmp(word, "PAINCHANCE"))   mobjinfo[num].painchance = value;
			else if(!strcmp(word, "PAINSOUND"))    mobjinfo[num].painsound = value;
			else if(!strcmp(word, "MELEESTATE"))   mobjinfo[num].meleestate = value;
			else if(!strcmp(word, "MISSILESTATE")) mobjinfo[num].missilestate = value;
			else if(!strcmp(word, "DEATHSTATE"))   mobjinfo[num].deathstate = value;
			else if(!strcmp(word, "DEATHSOUND"))   mobjinfo[num].deathsound = value;
			else if(!strcmp(word, "XDEATHSTATE"))  mobjinfo[num].xdeathstate = value;
			else if(!strcmp(word, "SPEED"))        mobjinfo[num].speed = value;
			else if(!strcmp(word, "RADIUS"))       mobjinfo[num].radius = value;
			else if(!strcmp(word, "HEIGHT"))       mobjinfo[num].height = value;
			else if(!strcmp(word, "MASS"))         mobjinfo[num].mass = value;
			else if(!strcmp(word, "DAMAGE"))       mobjinfo[num].damage = value;
			else if(!strcmp(word, "ACTIVESOUND"))  mobjinfo[num].activesound = value;
			else if(!strcmp(word, "FLAGS"))        mobjinfo[num].flags = value;
			else if(!strcmp(word, "RAISESTATE"))   mobjinfo[num].raisestate = value;
			else
				deh_error("Thing %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

#ifdef HWRENDER
static void readlight(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	int value;
	float fvalue;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;
			fvalue = searchfvalue(s);
			value = searchvalue(s);

			word = strupr(strtok(s, " "));
			if(!strcmp(word, "TYPE"))               lspr[num].type = (USHORT)value;
			else if(!strcmp(word, "OFFSETX"))       lspr[num].light_xoffset = fvalue;
			else if(!strcmp(word, "OFFSETY"))       lspr[num].light_yoffset = fvalue;
			else if(!strcmp(word, "CORONACOLOR"))   lspr[num].corona_color = value;
			else if(!strcmp(word, "CORONARADIUS"))  lspr[num].corona_radius = fvalue;
			else if(!strcmp(word, "DYNAMICCOLOR"))  lspr[num].dynamic_color = value;
			else if(!strcmp(word, "DYNAMICRADIUS"))
			{
				lspr[num].dynamic_radius = fvalue;

				/// \note Update the sqrradius! unnecessary?
				lspr[num].dynamic_sqrradius = fvalue * fvalue;
			}
			else
				deh_error("Light %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

static void readspritelight(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	int value;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;
			value = searchvalue(s);

			word = strupr(strtok(s, " "));
			if(!strcmp(word, "LIGHTTYPE")) t_lspr[num] = &lspr[value];
			else
				deh_error("Sprite %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}
#endif // HWRENDER

static void readlevelheader(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	char* tmp;
	int i;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if(tmp)
				*tmp = '\0';

			// Get the part before the " = "
			tmp = strchr(s, '=');
			*(tmp-1) = '\0';
			word = strupr(s);

			// Now get the part after
			tmp += 2;
			word2 = strupr(tmp);

			i = atoi(word2); // used for numerical settings

			if(!strcmp(word, "LEVELNAME"))        strncpy(mapheaderinfo[num-1].lvlttl, word2, 33);
			else if(!strcmp(word, "INTERSCREEN")) strncpy(mapheaderinfo[num-1].interscreen, word2, 8);
			else if(!strcmp(word, "ACT"))
			{
				if(i >= 0 && i < 20) // 0 for no act number, TTL1 through TTL19
			                                              mapheaderinfo[num-1].actnum = (byte)i;
				else
					deh_error("Level header %d: invalid act number %d\n", num, i);
			}
			else if(!strcmp(word, "NOZONE"))              mapheaderinfo[num-1].nozone = i;
			else if(!strcmp(word, "TYPEOFLEVEL"))         mapheaderinfo[num-1].typeoflevel = (short)i;
			else if(!strcmp(word, "NEXTLEVEL"))           mapheaderinfo[num-1].nextlevel = (short)i;
			else if(!strcmp(word, "MUSICSLOT"))           mapheaderinfo[num-1].musicslot = (short)i;
			else if(!strcmp(word, "FORCECHARACTER"))      mapheaderinfo[num-1].forcecharacter = (byte)i;
			else if(!strcmp(word, "WEATHER"))             mapheaderinfo[num-1].weather = (byte)i;
			else if(!strcmp(word, "SKYNUM"))              mapheaderinfo[num-1].skynum = (short)i;
			else if(!strcmp(word, "SCRIPTNAME"))  strncpy(mapheaderinfo[num-1].scriptname, word2, 192);
			else if(!strcmp(word, "SCRIPTISLUMP"))        mapheaderinfo[num-1].scriptislump = i;
			else if(!strcmp(word, "PRECUTSCENENUM"))      mapheaderinfo[num-1].precutscenenum = (byte)i;
			else if(!strcmp(word, "CUTSCENENUM"))         mapheaderinfo[num-1].cutscenenum = (byte)i;
			else if(!strcmp(word, "COUNTDOWN"))           mapheaderinfo[num-1].countdown = (short)i;
			else if(!strcmp(word, "HIDDEN"))              mapheaderinfo[num-1].hideinmenu = i;
			else if(!strcmp(word, "NOSSMUSIC"))           mapheaderinfo[num-1].nossmusic = i;
			else
				deh_error("Level header %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

static void readcutscenescene(MYFILE* f, int num, int scenenum)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	int i;
	unsigned short usi;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			word = strupr(strtok(s, " "));

			if(!strcmp(word, "SCENETEXT"))
			{
				char* scenetext = NULL;
				char buffer[1024];
				for(i = 0; i < MAXLINELEN; i++)
				{
					if(s[i] == '=')
					{
						scenetext = &s[i+2];
						break;
					}
				}
				for(i = 0; i < MAXLINELEN; i++)
				{
					if(s[i] == '\0')
					{
						s[i] = '\n';
						s[i+1] = '\0';
						break;
					}
				}

				strcpy(buffer, scenetext);

				strcat(buffer,
					myhashfgets(scenetext, sizeof(buffer)
					- strlen(buffer) - 1, f));

				// A cutscene overwriting another one...
				if(cutscenes[num].scene[scenenum].text != NULL)
					Z_Free(cutscenes[num].scene[scenenum].text);

				cutscenes[num].scene[scenenum].text = Z_Malloc(strlen(buffer), PU_STATIC, NULL);

				strcpy(cutscenes[num].scene[scenenum].text, buffer);

				continue;
			}

			word2 = strupr(strtok(NULL, " = "));
			word2[strlen(word2)-1] = '\0';
			i = atoi(word2);
			usi = (unsigned short)i;

			if(!strcmp(word, "NUMBEROFPICS"))          cutscenes[num].scene[scenenum].numpics = (byte)i;
			else if(!strcmp(word, "PIC1NAME")) strncpy(cutscenes[num].scene[scenenum].picname[0], word2, 8);
			else if(!strcmp(word, "PIC2NAME")) strncpy(cutscenes[num].scene[scenenum].picname[1], word2, 8);
			else if(!strcmp(word, "PIC3NAME")) strncpy(cutscenes[num].scene[scenenum].picname[2], word2, 8);
			else if(!strcmp(word, "PIC4NAME")) strncpy(cutscenes[num].scene[scenenum].picname[3], word2, 8);
			else if(!strcmp(word, "PIC5NAME")) strncpy(cutscenes[num].scene[scenenum].picname[4], word2, 8);
			else if(!strcmp(word, "PIC6NAME")) strncpy(cutscenes[num].scene[scenenum].picname[5], word2, 8);
			else if(!strcmp(word, "PIC7NAME")) strncpy(cutscenes[num].scene[scenenum].picname[6], word2, 8);
			else if(!strcmp(word, "PIC8NAME")) strncpy(cutscenes[num].scene[scenenum].picname[7], word2, 8);
			else if(!strcmp(word, "PIC1HIRES"))        cutscenes[num].scene[scenenum].pichires[0] = i;
			else if(!strcmp(word, "PIC2HIRES"))        cutscenes[num].scene[scenenum].pichires[1] = i;
			else if(!strcmp(word, "PIC3HIRES"))        cutscenes[num].scene[scenenum].pichires[2] = i;
			else if(!strcmp(word, "PIC4HIRES"))        cutscenes[num].scene[scenenum].pichires[3] = i;
			else if(!strcmp(word, "PIC5HIRES"))        cutscenes[num].scene[scenenum].pichires[4] = i;
			else if(!strcmp(word, "PIC6HIRES"))        cutscenes[num].scene[scenenum].pichires[5] = i;
			else if(!strcmp(word, "PIC7HIRES"))        cutscenes[num].scene[scenenum].pichires[6] = i;
			else if(!strcmp(word, "PIC8HIRES"))        cutscenes[num].scene[scenenum].pichires[7] = i;
			else if(!strcmp(word, "PIC1DURATION"))     cutscenes[num].scene[scenenum].picduration[0] = usi;
			else if(!strcmp(word, "PIC2DURATION"))     cutscenes[num].scene[scenenum].picduration[1] = usi;
			else if(!strcmp(word, "PIC3DURATION"))     cutscenes[num].scene[scenenum].picduration[2] = usi;
			else if(!strcmp(word, "PIC4DURATION"))     cutscenes[num].scene[scenenum].picduration[3] = usi;
			else if(!strcmp(word, "PIC5DURATION"))     cutscenes[num].scene[scenenum].picduration[4] = usi;
			else if(!strcmp(word, "PIC6DURATION"))     cutscenes[num].scene[scenenum].picduration[5] = usi;
			else if(!strcmp(word, "PIC7DURATION"))     cutscenes[num].scene[scenenum].picduration[6] = usi;
			else if(!strcmp(word, "PIC8DURATION"))     cutscenes[num].scene[scenenum].picduration[7] = usi;
			else if(!strcmp(word, "PIC1XCOORD"))       cutscenes[num].scene[scenenum].xcoord[0] = usi;
			else if(!strcmp(word, "PIC2XCOORD"))       cutscenes[num].scene[scenenum].xcoord[1] = usi;
			else if(!strcmp(word, "PIC3XCOORD"))       cutscenes[num].scene[scenenum].xcoord[2] = usi;
			else if(!strcmp(word, "PIC4XCOORD"))       cutscenes[num].scene[scenenum].xcoord[3] = usi;
			else if(!strcmp(word, "PIC5XCOORD"))       cutscenes[num].scene[scenenum].xcoord[4] = usi;
			else if(!strcmp(word, "PIC6XCOORD"))       cutscenes[num].scene[scenenum].xcoord[5] = usi;
			else if(!strcmp(word, "PIC7XCOORD"))       cutscenes[num].scene[scenenum].xcoord[6] = usi;
			else if(!strcmp(word, "PIC8XCOORD"))       cutscenes[num].scene[scenenum].xcoord[7] = usi;
			else if(!strcmp(word, "PIC1YCOORD"))       cutscenes[num].scene[scenenum].ycoord[0] = usi;
			else if(!strcmp(word, "PIC2YCOORD"))       cutscenes[num].scene[scenenum].ycoord[1] = usi;
			else if(!strcmp(word, "PIC3YCOORD"))       cutscenes[num].scene[scenenum].ycoord[2] = usi;
			else if(!strcmp(word, "PIC4YCOORD"))       cutscenes[num].scene[scenenum].ycoord[3] = usi;
			else if(!strcmp(word, "PIC5YCOORD"))       cutscenes[num].scene[scenenum].ycoord[4] = usi;
			else if(!strcmp(word, "PIC6YCOORD"))       cutscenes[num].scene[scenenum].ycoord[5] = usi;
			else if(!strcmp(word, "PIC7YCOORD"))       cutscenes[num].scene[scenenum].ycoord[6] = usi;
			else if(!strcmp(word, "PIC8YCOORD"))       cutscenes[num].scene[scenenum].ycoord[7] = usi;
			else if(!strcmp(word, "MUSICSLOT"))        cutscenes[num].scene[scenenum].musicslot = (short)i;
			else if(!strcmp(word, "TEXTXPOS"))         cutscenes[num].scene[scenenum].textxpos = usi;
			else if(!strcmp(word, "TEXTYPOS"))         cutscenes[num].scene[scenenum].textypos = usi;
			else
				deh_error("CutSceneScene %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

static void readcutscene(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	int value;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			word = strupr(strtok(s, " "));
			word2 = strtok(NULL, " ");
			if(word2)
				value = atoi(word2);
			else
			{
				deh_error("No value for token %s", word);
				continue;
			}

			if(!strcmp(word, "NUMSCENES")) cutscenes[num].numscenes = value;
			else if(!strcmp(word, "SCENE"))
			{
				if(value >= 1 && value <= 128)
					readcutscenescene(f, num, value - 1);
				else
					deh_error("Scene number %d out of range\n", value);
			}
			else
				deh_error("Cutscene %d: unknown word '%s', Scene <num> expected.\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

static void readhuditem(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	char* tmp;
	int i;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if(tmp)
				*tmp = '\0';

			// Get the part before the " = "
			tmp = strchr(s, '=');
			*(tmp-1) = '\0';
			word = strupr(s);

			// Now get the part after
			tmp += 2;
			word2 = strupr(tmp);

			i = atoi(word2); // used for numerical settings

			if(!strcmp(word, "X"))              hudinfo[num].x = i;
			else if(!strcmp(word, "Y"))         hudinfo[num].y = i;
			else
				deh_error("Level header %d: unknown word '%s'\n", num, word);
		}
	} while(!myfeof(f)); // finish when the line is empty
}

/*
Sprite number = 10
Sprite subnumber = 32968
Duration = 200
Next frame = 200
*/

/** Action pointer for reading actions from Dehacked lumps.
  */
typedef struct
{
	actionf_t action; ///< Function pointer corresponding to the actual action.
	const char* name; ///< Name of the action in ALL CAPS.
} actionpointer_t;

/** Array mapping action names to action functions.
  * Names must be in ALL CAPS for case insensitive comparisons.
  */
static actionpointer_t actionpointers[] =
{
	{{A_Explode},              "A_EXPLODE"},
	{{A_Pain},                 "A_PAIN"},
	{{A_Fall},                 "A_FALL"},
	{{A_MonitorPop},           "A_MONITORPOP"},
	{{A_Look},                 "A_LOOK"},
	{{A_Chase},                "A_CHASE"},
	{{A_FaceTarget},           "A_FACETARGET"},
	{{A_Scream},               "A_SCREAM"},
	{{A_BossDeath},            "A_BOSSDEATH"},
	{{A_CustomPower},          "A_CUSTOMPOWER"},
	{{A_RingShield},           "A_RINGSHIELD"},
	{{A_RingBox},              "A_RINGBOX"},
	{{A_Invincibility},        "A_INVINCIBILITY"},
	{{A_SuperSneakers},        "A_SUPERSNEAKERS"},
	{{A_BunnyHop},             "A_BUNNYHOP"},
	{{A_BubbleSpawn},          "A_BUBBLESPAWN"},
	{{A_BubbleRise},           "A_BUBBLERISE"},
	{{A_BubbleCheck},          "A_BUBBLECHECK"},
	{{A_ExtraLife},            "A_EXTRALIFE"},
	{{A_BombShield},           "A_BOMBSHIELD"},
	{{A_JumpShield},           "A_JUMPSHIELD"},
	{{A_WaterShield},          "A_WATERSHIELD"},
	{{A_FireShield},           "A_FIRESHIELD"},
	{{A_ScoreRise},            "A_SCORERISE"},
	{{A_AttractChase},         "A_ATTRACTCHASE"},
	{{A_DropMine},             "A_DROPMINE"},
	{{A_FishJump},             "A_FISHJUMP"},
	{{A_SignPlayer},           "A_SIGNPLAYER"},
	{{A_ThrownRing},           "A_THROWNRING"},
	{{A_SetSolidSteam},        "A_SETSOLIDSTEAM"},
	{{A_UnsetSolidSteam},      "A_UNSETSOLIDSTEAM"},
	{{A_JetChase},             "A_JETCHASE"},
	{{A_JetbThink},            "A_JETBTHINK"},
	{{A_JetgThink},            "A_JETGTHINK"},
	{{A_JetgShoot},            "A_JETGSHOOT"},
	{{A_ShootBullet},          "A_SHOOTBULLET"},
	{{A_MouseThink},           "A_MOUSETHINK"},
	{{A_DetonChase},           "A_DETONCHASE"},
	{{A_CapeChase},            "A_CAPECHASE"},
	{{A_RotateSpikeBall},      "A_ROTATESPIKEBALL"},
	{{A_SnowBall},             "A_SNOWBALL"},
	{{A_CrawlaCommanderThink}, "A_CRAWLACOMMANDERTHINK"},
	{{A_SmokeTrailer},         "A_SMOKETRAILER"},
	{{A_RingExplode},          "A_RINGEXPLODE"},
	{{A_MixUp},                "A_MIXUP"},
	{{A_BossScream},           "A_BOSSSCREAM"},
	{{A_Invinciblerize},       "A_INVINCIBLERIZE"},
	{{A_DeInvinciblerize},     "A_DEINVINCIBLERIZE"},
	{{A_Boss2PogoSFX},         "A_BOSS2POGOSFX"},
	{{A_EggmanBox},            "A_EGGMANBOX"},
	{{A_TurretFire},           "A_TURRETFIRE"},
	{{A_SuperTurretFire},      "A_SUPERTURRETFIRE"},
	{{A_TurretStop},           "A_TURRETSTOP"},
	{{A_SkimChase},            "A_SKIMCHASE"},
	{{A_PumaJump},             "A_PUMAJUMP"},
	{{A_1upThinker},           "A_1UPTHINKER"},
	{{A_SkullAttack},          "A_SKULLATTACK"},
	{{A_CyberAttack},          "A_CYBERATTACK"},
	{{A_SparkFollow},          "A_SPARKFOLLOW"},
	{{A_BuzzFly},              "A_BUZZFLY"},
	{{A_SetReactionTime},      "A_SETREACTIONTIME"},
	{{A_LinedefExecute},       "A_LINEDEFEXECUTE"},
	{{A_PlaySeeSound},         "A_PLAYSEESOUND"},
	{{A_PlayAttackSound},      "A_PLAYATTACKSOUND"},
	{{A_PlayActiveSound},      "A_PLAYACTIVESOUND"},
	{{NULL},                   "NONE"},

	// This NULL entry must be the last in the list
	{{NULL},                   NULL},
};

static void readframe(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word1;
	char* word2;
	int i, j;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			for(j = 0; s[j] != '\n'; j++)
			{
				if(s[j] == '=')
				{
					j += 2;
					j = atoi(&s[j]);
					break;
				}
			}
			word1 = strupr(strtok(s, " "));
			word2 = strupr(strtok(NULL, " = "));
			word2[strlen(word2)-1] = '\0';
			i = atoi(word2);

			if(!strcmp(word1, "SPRITENUMBER"))         states[num].sprite = i;
			else if(!strcmp(word1, "SPRITESUBNUMBER")) states[num].frame = i;
			else if(!strcmp(word1, "DURATION"))        states[num].tics = i;
			else if(!strcmp(word1, "NEXT"))            states[num].nextstate = i+1;
			else if(!strcmp(word1, "ACTION"))
			{
				unsigned int z;
				boolean found = false;
				char actiontocompare[32];

				strncpy(actiontocompare, word2, 31);

				for(z = 0; z < 32; z++)
				{
					if(actiontocompare[z] == '\n' || actiontocompare[z] == '\r')
					{
						actiontocompare[z] = '\0';
						break;
					}
				}

				z = 0;
				while(actionpointers[z].name)
				{
					if(!strcmp(strupr(actiontocompare), actionpointers[z].name))
					{
						states[num].action = actionpointers[z].action;
						states[num].action.acv = actionpointers[z].action.acv; // assign
						states[num].action.acp1 = actionpointers[z].action.acp1;
						found = true;
						break;
					}
					z++;
				}

				if(!found)
					deh_error("Unknown action %s\n", actiontocompare);
			}
			else
				deh_error("Frame %d: unknown word '%s'\n", num, word1);
		}
	} while(!myfeof(f));
}

static void readsound(MYFILE* f, int num, const char* savesfxnames[])
{
	char s[MAXLINELEN];
	char* word;
	int value;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;
			value = searchvalue(s);
			word = strupr(strtok(s, " "));
/*			if(!strcmp(word, "OFFSET"))
			{
				value -= 150360;
				if(value <= 64)
					value /= 8;
				else if(value <= 260)
					value = (value+4)/8;
				else
					value = (value+8)/8;
				if(value >= -1 && value < sfx_freeslot0 - 1)
					S_sfx[num].name = savesfxnames[value+1];
				else
					deh_error("Sound %d: offset out of bounds\n", num);
			}
			else */if(!strcmp(word, "SINGULAR")) S_sfx[num].singularity = value;
			else if(!strcmp(word, "PRIORITY"))    S_sfx[num].priority = value;
			else if(!strcmp(word, "FLAGS"))    S_sfx[num].pitch = value;
			else
				deh_error("Sound %d : unknown word '%s'\n",num,word);
		}
	} while(!myfeof(f));
	savesfxnames = NULL;
}

static void readmaincfg(MYFILE* f)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	char* tmp;
	int value;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if(tmp)
				*tmp = '\0';

			// Get the part before the " = "
			tmp = strchr(s, '=');
			*(tmp-1) = '\0';
			word = strupr(s);

			// Now get the part after
			tmp += 2;
			word2 = strupr(tmp);

			value = atoi(word2); // used for numerical settings

			if(!strcmp(word, "SSTAGE_START"))
			{
				sstage_start = value;
				sstage_end = sstage_start+6;
			}
			else if(!strcmp(word, "EXECCFG"))
				COM_BufAddText(va("exec %s\n", word2));
			else if(!strcmp(word, "SPSTAGE_START"))   spstage_start = value;
			else if(!strcmp(word, "RACESTAGE_START")) racestage_start = value;
			else if(!strcmp(word, "INVULNTICS"))      invulntics = value;
			else if(!strcmp(word, "SNEAKERTICS"))     sneakertics = value;
			else if(!strcmp(word, "FLASHINGTICS"))    flashingtics = value;
			else if(!strcmp(word, "TAILSFLYTICS"))    tailsflytics = value;
			else if(!strcmp(word, "UNDERWATERTICS"))  underwatertics = value;
			else if(!strcmp(word, "SPACETIMETICS"))   spacetimetics = value;
			else if(!strcmp(word, "EXTRALIFETICS"))   extralifetics = value;
			else if(!strcmp(word, "PARALOOPTICS"))    paralooptics = value;
			else if(!strcmp(word, "HELPERTICS"))      helpertics = value;
			else if(!strcmp(word, "GAMEOVERTICS"))    gameovertics = value;
			else if(!strcmp(word, "INTROTOPLAY"))     introtoplay = (byte)value;
			else if(!strcmp(word, "GAMEDATA"))
			{
				G_SaveGameData();
				strncpy(gamedatafilename, word2, 64);
				strlwr(gamedatafilename);
				savemoddata = true;
				G_LoadGameData();
			}
			else if(!strcmp(word, "NUMEMBLEMS"))
			{
				numemblems = value+2;
				if(numemblems > MAXEMBLEMS-2)
					I_Error("Sorry, a maximum of %i emblems is allowed.\n", MAXEMBLEMS-2);
			}
			else
				deh_error("Maincfg: unknown word '%s'\n", word);
		}
	} while(!myfeof(f));
}

static void reademblemdata(MYFILE* f, int num)
{
	char s[MAXLINELEN];
	char* word;
	char* word2;
	int value;

	do
	{
		if(myfgets(s, sizeof(s), f))
		{
			if(s[0] == '\n')
				break;
			value = searchvalue(s);
			word = strupr(strtok(s, " "));
			word2 = strupr(strtok(NULL, " "));

			if(!strcmp(word, "X"))				emblemlocations[num-1].x = (short)value;
			else if(!strcmp(word, "Y"))			emblemlocations[num-1].y = (short)value;
			else if(!strcmp(word, "Z"))			emblemlocations[num-1].z = (short)value;
			else if(!strcmp(word, "PLAYERNUM"))	emblemlocations[num-1].player = (byte)value;
			else if(!strcmp(word, "MAPNUM"))    emblemlocations[num-1].level = (short)value;
			else
				deh_error("Emblem: unknown word '%s'\n", word);
		}
	} while(!myfeof(f));
}

static void DEH_LoadDehackedFile(MYFILE* f)
{
	char s[1000];
	char* word;
	char* word2;
	int i;
	// do a copy of this for cross references probleme
	actionf_t saveactions[NUMSTATES];
	const char* savesprnames[NUMSPRITES];
	const char* savesfxnames[NUMSFX];

	deh_num_error = 0;
	// save values for cross reference
	for(i = 0; i < NUMSTATES; i++)
		saveactions[i] = states[i].action;
	for(i = 0; i < NUMSPRITES; i++)
		savesprnames[i] = sprnames[i];
	for(i = 0; i < NUMSFX; i++)
		savesfxnames[i] = S_sfx[i].name;

	// it doesn't test the version of SRB2 and version of dehacked file
	while(!myfeof(f))
	{
		char origpos[32];
		int size = 0;
		char* traverse;

		myfgets(s, sizeof(s), f);
		if(s[0] == '\n' || s[0] == '#')
			continue;

		traverse = s;

		while(traverse[0] != '\n')
		{
			traverse++;
			size++;
		}

		strncpy(origpos, s, size);
		origpos[size] = '\0';

		word = strupr(strtok(s, " "));
		if(word)
		{
			char* p = strtok(NULL, " ");
			if(p) word2 = strupr(p);
			else word2 = NULL;
			if(word2)
			{
				i = atoi(word2);

				if(!strcmp(word, "THING"))
				{
					if(i < NUMMOBJTYPES && i >= 0)
						readthing(f, i);
					else
						deh_error("Thing %d doesn't exist\n", i);
				}
				else if(!strcmp(word, "MODBY"))
				{
					strcpy(credits[18].fakenames[0], origpos+3);
					strcpy(credits[18].realnames[0], origpos+3);
					credits[18].numnames = 1;
					strcpy(&credits[18].header[0], "Modification By\n");
					modcredits = true;
				}
/*				else if(!strcmp(word, "ANIMTEX"))
				{
					readAnimTex(f, i);
				}*/
				else if(!strcmp(word, "CHARACTER"))
				{
					if(i < 8)
						readPlayer(f, i);
					else
						deh_error("Character %d out of range\n", i);
				}
				else if(!strcmp(word, "LIGHT"))
				{
#ifdef HWRENDER
					if(i > 0 && i < NUMLIGHTS)
						readlight(f, i);
					else
						deh_error("Light number %d out of range\n", i);
#endif
				}
				else if(!strcmp(word, "SPRITE"))
				{
#ifdef HWRENDER
					if(i < NUMSPRITES && i >= 0)
						readspritelight(f, i);
					else
						deh_error("Sprite number %d out of range\n", i);
#endif
				}
				else if(!strcmp(word, "LEVEL"))
				{
					if(i > 0 && i <= NUMMAPS)
						readlevelheader(f, i);
					else
						deh_error("Level number %d out of range\n", i);
				}
				else if(!strcmp(word, "CUTSCENE"))
				{
					if(i > 0 && i < 129)
						readcutscene(f, i - 1);
					else
						deh_error("Cutscene number %d out of range\n", i);
				}
				else if(!strcmp(word, "FRAME"))
				{
					if(i < NUMSTATES && i >= 0)
						readframe(f, i);
					else
						deh_error("Frame %d doesn't exist\n", i);
				}
/*				else if(!strcmp(word, "POINTER"))
				{
					word = strtok(NULL, " "); // get frame
					word = strtok(NULL, ")");
					if(word)
					{
						i = atoi(word);
						if(i < NUMSTATES && i >= 0)
						{
							if(myfgets(s, sizeof(s), f))
								states[i].action = saveactions[searchvalue(s)];
						}
						else
							deh_error("Pointer: Frame %d doesn't exist\n", i);
					}
					else
						deh_error("pointer (Frame %d) : missing ')'\n", i);
				}*/
				else if(!strcmp(word, "SOUND"))
				{
					if(i < NUMSFX && i >= 0)
						readsound(f, i, savesfxnames);
					else
						deh_error("Sound %d doesn't exist\n", i);
				}
/*				else if(!strcmp(word, "SPRITE"))
				{
					if(i < NUMSPRITES && i >= 0)
					{
						if(myfgets(s, sizeof(s), f))
						{
							int k;
							k = (searchvalue(s) - 151328)/8;
							if(k >= 0 && k < NUMSPRITES)
								sprnames[i] = savesprnames[k];
							else
								deh_error("Sprite %i: offset out of bound\n", i);
						}
					}
					else
						deh_error("Sprite %d doesn't exist\n",i);
				}*/
				else if(!strcmp(word, "HUDITEM"))
				{
					if(i >= 0 && i < NUMHUDITEMS)
						readhuditem(f, i);
					else
						deh_error("HUD item number %d out of range\n", i);
				}
				else if(!strcmp(word, "EMBLEM"))
				{
					if(i > 0 && i <= MAXEMBLEMS)
						reademblemdata(f, i);
					else
						deh_error("HUD item number %d out of range\n", i);
				}
				else if(!strcmp(word, "MAINCFG"))
					readmaincfg(f);
				else if(!strcmp(word, "SRB2"))
				{
					int ver = searchvalue(strtok(NULL, "\n"));
					if(ver != 11)
						deh_error("Warning: patch from a different SRB2 version (%d), "
							"only version 1.1 is supported\n", ver);
				}
				else
					deh_error("Unknown word: %s\n", word);
			}
			else
				deh_error("missing argument for '%s'\n", word);
		}
		else
			deh_error("No word in this line:\n%s\n", s);
	} // end while
	if(deh_num_error)
	{
		CONS_Printf("%d warning%s in the dehacked file\n", deh_num_error,
			deh_num_error == 1 ? "" : "s");
		if(devparm)
			I_GetKey(); //getchar(); Alam: no getchar!
	}

	deh_loaded = true;
}

// read dehacked lump in a wad (there is special trick for for deh
// file that are converted to wad in w_wad.c)
void DEH_LoadDehackedLump(int lump)
{
	MYFILE f;

	f.size = W_LumpLength(lump);
	f.data = Z_Malloc(f.size + 1, PU_STATIC, 0);
	W_ReadLump(lump, f.data);
	f.curpos = f.data;
	f.data[f.size] = 0;

	DEH_LoadDehackedFile(&f);
	Z_Free(f.data);
}
