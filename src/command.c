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
/// \brief Parse and execute commands from console input/scripts and remote server
/// 
///	handles console variables, which is a simplified version
///	of commands, each consvar can have a function called when
///	it is modified.. thus it acts nearly as commands.
///
///	code shamelessly inspired by the QuakeC sources, thanks Id :)

#include "doomdef.h"
#include "doomstat.h"
#include "command.h"
#include "console.h"
#include "z_zone.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_fixed.h"
#include "m_argv.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "g_game.h" // for player_names
#include "d_netcmd.h"
#include "hu_stuff.h"

//========
// protos.
//========
static boolean COM_Exists(const char* com_name);
static void COM_ExecuteString(char* text);

static void COM_Alias_f(void);
static void COM_Echo_f(void);
static void COM_CEcho_f(void);
static void COM_CEchoFlags_f(void);
static void COM_CEchoDuration_f(void);
static void COM_Exec_f(void);
static void COM_Wait_f(void);
static void COM_Help_f(void);
static void COM_Toggle_f(void);

static boolean CV_Command(void);
static consvar_t* CV_FindVar(const char* name);
static const char* CV_StringValue(const char* var_name);
static consvar_t* consvar_vars; // list of registered console variables

static char com_token[1024];
static char* COM_Parse(char* data);

CV_PossibleValue_t CV_OnOff[] = {{0, "Off"}, {1, "On"}, {0, NULL}};
CV_PossibleValue_t CV_YesNo[] = {{0, "No"}, {1, "Yes"}, {0, NULL}};
CV_PossibleValue_t CV_Unsigned[] = {{0, "MIN"}, {999999999, "MAX"}, {0, NULL}};

#define COM_BUF_SIZE 8192 // command buffer size

static int com_wait; // one command per frame (for cmd sequences)

// command aliases
//
typedef struct cmdalias_s
{
	struct cmdalias_s* next;
	char* name;
	char* value; // the command string to replace the alias
} cmdalias_t;

static cmdalias_t* com_alias; // aliases list

// =========================================================================
//                            COMMAND BUFFER
// =========================================================================

static vsbuf_t com_text; // variable sized buffer

/** Adds text into the command buffer for later execution.
  *
  * \param text The text to add.
  * \sa COM_BufInsertText
  */
void COM_BufAddText(const char* text)
{
	size_t l;

	l = strlen(text);

	if(com_text.cursize + l >= com_text.maxsize)
	{
		CONS_Printf("Command buffer full!\n");
		return;
	}
	VS_Write(&com_text, text, l);
}

/** Adds command text and executes it immediately.
  *
  * \param text The text to execute. A newline is automatically added.
  * \sa COM_BufAddText
  */
void COM_BufInsertText(const char* text)
{
	char* temp;
	size_t templen;

	// copy off any commands still remaining in the exec buffer
	templen = com_text.cursize;
	if(templen)
	{
		temp = ZZ_Alloc(templen);
		memcpy(temp, com_text.data, templen);
		VS_Clear(&com_text);
	}
	else
		temp = NULL; // shut up compiler

	// add the entire text of the file (or alias)
	COM_BufAddText(text);
	COM_BufExecute(); // do it right away

	// add the copied off data
	if(templen)
	{
		VS_Write(&com_text, temp, templen);
		Z_Free(temp);
	}
}

/** Flushes (executes) console commands in the buffer.
  */
void COM_BufExecute(void)
{
	size_t i;
	char* text;
	char line[1024];
	int quotes;

	if(com_wait)
	{
		com_wait--;
		return;
	}

	while(com_text.cursize)
	{
		// find a '\n' or ; line break
		text = (char*)com_text.data;

		quotes = 0;
		for(i = 0; i < com_text.cursize; i++)
		{
			if(text[i] == '"')
				quotes++;
			if(!(quotes & 1) && text[i] == ';')
				break; // don't break if inside a quoted string
			if(text[i] == '\n' || text[i] == '\r')
				break;
		}

		memcpy(line, text, i);
		line[i] = 0;

		// flush the command text from the command buffer, _BEFORE_
		// executing, to avoid that 'recursive' aliases overflow the
		// command text buffer, in that case, new commands are inserted
		// at the beginning, in place of the actual, so it doesn't
		// overflow
		if(i == com_text.cursize)
			// the last command was just flushed
			com_text.cursize = 0;
		else
		{
			i++;
			com_text.cursize -= i;
			memcpy(text, text+i, com_text.cursize);
		}

		// execute the command line
		COM_ExecuteString(line);

		// delay following commands if a wait was encountered
		if(com_wait)
		{
			com_wait--;
			break;
		}
	}
}

// =========================================================================
//                            COMMAND EXECUTION
// =========================================================================

typedef struct xcommand_s
{
	const char* name;
	struct xcommand_s* next;
	com_func_t function;
} xcommand_t;

static xcommand_t* com_commands = NULL; // current commands

#define MAX_ARGS 80
static size_t com_argc;
static char* com_argv[MAX_ARGS];
static const char* com_null_string = "";
static char* com_args = NULL; // current command args or NULL

static void Got_NetVar(char** p, int playernum);

/** Initializes command buffer and adds basic commands.
  */
void COM_Init(void)
{
	// allocate command buffer
	VS_Alloc(&com_text, COM_BUF_SIZE);

	// add standard commands
	COM_AddCommand("alias", COM_Alias_f);
	COM_AddCommand("echo", COM_Echo_f);
	COM_AddCommand("cecho", COM_CEcho_f);
	COM_AddCommand("cechoflags", COM_CEchoFlags_f);
	COM_AddCommand("cechoduration", COM_CEchoDuration_f);
	COM_AddCommand("exec", COM_Exec_f);
	COM_AddCommand("wait", COM_Wait_f);
	COM_AddCommand("help", COM_Help_f);
	COM_AddCommand("toggle", COM_Toggle_f);
	RegisterNetXCmd(XD_NETVAR, Got_NetVar);
}

/** Gets a console command argument count.
  *
  * \return Number of arguments for the last command.
  * \sa COM_Argv
  */
size_t COM_Argc(void)
{
	return com_argc;
}

/** Gets a console command argument.
  *
  * \param arg Index of the argument (0 to COM_Argc() - 1).
  * \return String pointer to the indicated argument.
  * \sa COM_Argc, COM_Args
  */
const char* COM_Argv(size_t arg)
{
	if(arg >= com_argc || (signed)arg < 0)
		return com_null_string;
	return com_argv[arg];
}

/** Gets all console command arguments.
  *
  * \return String pointer to all arguments for the last command.
  * \sa COM_Argv
  */
char* COM_Args(void)
{
	return com_args;
}

/** Checks if a parameter was passed to a console command.
  *
  * \param check The parameter to look for, e.g. "-noerror".
  * \return The index of the argument containing the parameter,
  *         or 0 if the parameter was not found.
  */
size_t COM_CheckParm(const char* check)
{
	size_t i;

	for(i = 1; i < com_argc; i++)
		if(!strcasecmp(check, com_argv[i]))
			return i;
	return 0;
}

/** Parses a string into command-line tokens.
  *
  * \param text A null-terminated string. Does not need to be
  *             newline-terminated.
  */
static void COM_TokenizeString(char* text)
{
	size_t i;

	// clear the args from the last string
	for(i = 0; i < com_argc; i++)
		Z_Free(com_argv[i]);

	com_argc = 0;
	com_args = NULL;

	for(;;)
	{
		// skip whitespace up to a newline
		while(*text && *text <= ' ' && *text != '\n')
			text++;

		if(*text == '\n')
		{ // a newline means end of command in buffer,
			// thus end of this command's args too
			text++;
			break;
		}

		if(!*text)
			return;

		if(com_argc == 1)
			com_args = text;

		text = COM_Parse(text);
		if(!text)
			return;

		if(com_argc < MAX_ARGS)
		{
			com_argv[com_argc] = ZZ_Alloc(strlen(com_token) + 1);
			strcpy(com_argv[com_argc], com_token);
			com_argc++;
		}
	}
}

/** Adds a console command.
  *
  * \param name Name of the command.
  * \param func Function called when the command is run.
  */
void COM_AddCommand(const char* name, com_func_t func)
{
	xcommand_t* cmd;

	// fail if the command is a variable name
	if(CV_StringValue(name)[0])
	{
		I_Error("%s is a variable name\n", name);
		return;
	}

	// fail if the command already exists
	for(cmd = com_commands; cmd; cmd = cmd->next)
	{
		if(!strcmp(name, cmd->name))
		{
			I_Error("Command %s already exists\n", name);
			return;
		}
	}

	cmd = ZZ_Alloc(sizeof(xcommand_t));
	cmd->name = name;
	cmd->function = func;
	cmd->next = com_commands;
	com_commands = cmd;
}

/** Tests if a command exists.
  *
  * \param com_name Name to test for.
  * \return True if a command by the given name exists.
  */
static boolean COM_Exists(const char* com_name)
{
	xcommand_t* cmd;

	for(cmd = com_commands; cmd; cmd = cmd->next)
		if(!strcmp(com_name, cmd->name))
			return true;

	return false;
}

/** Does command completion for the console.
  *
  * \param partial The partial name of the command (potentially).
  * \param skips   Number of commands to skip.
  * \return The complete command name, or NULL.
  * \sa CV_CompleteVar
  */
const char* COM_CompleteCommand(const char* partial, int skips)
{
	xcommand_t* cmd;
	size_t len;

	len = strlen(partial);

	if(!len)
		return NULL;

	// check functions
	for(cmd = com_commands; cmd; cmd = cmd->next)
		if(!strncmp(partial, cmd->name, len))
			if(!skips--)
				return cmd->name;

	return NULL;
}

/** Parses a single line of text into arguments and tries to execute it.
  * The text can come from the command buffer, a remote client, or stdin.
  *
  * \param text A single line of text.
  */
static void COM_ExecuteString(char* text)
{
	xcommand_t* cmd;
	cmdalias_t* a;

	COM_TokenizeString(text);

	// execute the command line
	if(!COM_Argc())
		return; // no tokens

	// check functions
	for(cmd = com_commands; cmd; cmd = cmd->next)
	{
		if(!strcmp(com_argv[0], cmd->name))
		{
			cmd->function();
			return;
		}
	}

	// check aliases
	for(a = com_alias; a; a = a->next)
	{
		if(!strcmp(com_argv[0], a->name))
		{
			COM_BufInsertText(a->value);
			return;
		}
	}

	// check cvars
	// Hurdler: added at Ebola's request ;)
	// (don't flood the console in software mode with bad gr_xxx command)
	if(!CV_Command() && con_destlines)
		CONS_Printf("Unknown command '%s'\n", COM_Argv(0));
}

// =========================================================================
//                            SCRIPT COMMANDS
// =========================================================================

/** Creates a command name that replaces another command.
  */
static void COM_Alias_f(void)
{
	cmdalias_t* a;
	char cmd[1024];
	size_t i, c;

	if(COM_Argc() < 3)
	{
		CONS_Printf("alias <name> <command>\n");
		return;
	}

	a = ZZ_Alloc(sizeof(cmdalias_t));
	a->next = com_alias;
	com_alias = a;

	a->name = Z_StrDup(COM_Argv(1));

	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string
	c = COM_Argc();
	for(i = 2; i < c; i++)
	{
		strcat(cmd, COM_Argv(i));
		if(i != c)
			strcat(cmd, " ");
	}
	strcat(cmd, "\n");

	a->value = Z_StrDup(cmd);
}

/** Prints a line of text to the console.
  */
static void COM_Echo_f(void)
{
	size_t i;

	for(i = 1; i < COM_Argc(); i++)
		CONS_Printf("%s ", COM_Argv(i));
	CONS_Printf("\n");
}

/** Displays text on the center of the screen for a short time.
  */
static void COM_CEcho_f(void)
{
	size_t i;

	cechotext[0] = 0;

	for(i = 1; i < COM_Argc(); i++)
	{
		strcat(cechotext, COM_Argv(i));
		strcat(cechotext, " ");
	}
	strcat(cechotext, "\\");

	cechotimer = cechoduration;
}

/** Sets drawing flags for the CECHO command.
  */
static void COM_CEchoFlags_f(void)
{
	if(COM_Argc() > 1)
		cechoflags = atoi(COM_Argv(1));
}

/** Sets the duration for CECHO commands to stay on the screen
  */
static void COM_CEchoDuration_f(void)
{
	if(COM_Argc() > 1)
		cechoduration = atoi(COM_Argv(1))*TICRATE;
}

/** Executes a script file.
  */
static void COM_Exec_f(void)
{
	int length;
	byte* buf = NULL;

	if(COM_Argc() < 2 || COM_Argc() > 3)
	{
		CONS_Printf("exec <filename> : run a script file\n");
		return;
	}

	// load file
	length = FIL_ReadFile(COM_Argv(1), &buf);

	if(!buf)
	{
		if(!COM_CheckParm("-noerror"))
			CONS_Printf("couldn't execute file %s\n", COM_Argv(1));
		return;
	}

	CONS_Printf("executing %s\n", COM_Argv(1));

	// insert text file into the command buffer
	COM_BufInsertText((char *)buf);

	// free buffer
	Z_Free(buf);
}

/** Delays execution of the rest of the commands until the next frame.
  * Allows sequences of commands like "jump; fire; backward".
  */
static void COM_Wait_f(void)
{
	if(COM_Argc() > 1)
		com_wait = atoi(COM_Argv(1));
	else
		com_wait = 1; // 1 frame
}

/** Prints help on variables and commands.
  */
static void COM_Help_f(void)
{
	xcommand_t* cmd;
	consvar_t* cvar;
	int i = 0;

	if(COM_Argc() > 1)
	{
		cvar = CV_FindVar(COM_Argv(1));
		if(cvar)
		{
			CONS_Printf("Variable %s:\n",cvar->name);
			CONS_Printf("  flags :");
			if(cvar->flags & CV_SAVE)
				CONS_Printf("AUTOSAVE ");
			if(cvar->flags & CV_FLOAT)
				CONS_Printf("FLOAT ");
			if(cvar->flags & CV_NETVAR)
				CONS_Printf("NETVAR ");
			if(cvar->flags & CV_CALL)
				CONS_Printf("ACTION ");
			CONS_Printf("\n");
			if(cvar->PossibleValue)
			{
				if(stricmp(cvar->PossibleValue[0].strvalue, "MIN") == 0)
				{
					for(i = 1; cvar->PossibleValue[i].strvalue != NULL; i++)
						if(!stricmp(cvar->PossibleValue[i].strvalue, "MAX"))
							break;
					CONS_Printf("  range from %d to %d\n", cvar->PossibleValue[0].value,
						cvar->PossibleValue[i].value);
				}
				else
				{
					CONS_Printf("  possible value :\n", cvar->name);
					while(cvar->PossibleValue[i].strvalue)
					{
						CONS_Printf("    %-2d : %s\n", cvar->PossibleValue[i].value,
							cvar->PossibleValue[i].strvalue);
						i++;
					}
				}
			}
		}
		else
			CONS_Printf("No Help for this command/variable\n");
	}
	else
	{
		// commands
		CONS_Printf("\2Commands\n");
		for(cmd = com_commands; cmd; cmd = cmd->next)
		{
			if(!(strcmp(cmd->name, "emilydampstone")))
				continue;

			if(!(strcmp(cmd->name, "iamaghost")))
				continue;

			CONS_Printf("%s ",cmd->name);
			i++;
		}

		// variables
		CONS_Printf("\2\nVariables\n");
		for(cvar = consvar_vars; cvar; cvar = cvar->next)
		{
			if(!(cvar->flags & CV_NOSHOWHELP))
				CONS_Printf("%s ", cvar->name);
			i++;
		}

		CONS_Printf("\2\nread help file for more or type help <command or variable>\n");

		if(devparm)
			CONS_Printf("\2Total : %d\n", i);
	}
}

/** Toggles a console variable. Useful for on/off values.
  *
  * \todo Make this work on on/off, yes/no values only?
  */
static void COM_Toggle_f(void)
{
	consvar_t* cvar;

	if(COM_Argc() != 2 && COM_Argc() != 3)
	{
		CONS_Printf("Toggle <cvar_name> [-1]\n"
			"Toggle the value of a cvar\n");
		return;
	}
	cvar = CV_FindVar(COM_Argv(1));
	if(!cvar)
	{
		CONS_Printf("%s is not a cvar\n",COM_Argv(1));
		return;
	}

	// netcvar don't change imediately
	cvar->flags |= CV_SHOWMODIFONETIME;
	if(COM_Argc() == 3)
		CV_AddValue(cvar, atol(COM_Argv(2)));
	else
		CV_AddValue(cvar, +1);
}

// =========================================================================
//                      VARIABLE SIZE BUFFERS
// =========================================================================

/** Initializes a variable size buffer.
  *
  * \param buf      Buffer to initialize.
  * \param initsize Initial size for the buffer.
  */
void VS_Alloc(vsbuf_t* buf, size_t initsize)
{
#define VSBUFMINSIZE 256
	if(initsize < VSBUFMINSIZE)
		initsize = VSBUFMINSIZE;
	buf->data = Z_Malloc(initsize, PU_STATIC, NULL);
	buf->maxsize = initsize;
	buf->cursize = 0;
#undef VSBUFMINSIZE
}

/** Frees a variable size buffer.
  *
  * \param buf Buffer to free.
  */
void VS_Free(vsbuf_t* buf)
{
	buf->cursize = 0;
}

/** Clears a variable size buffer.
  *
  * \param buf Buffer to clear.
  */
void VS_Clear(vsbuf_t* buf)
{
	buf->cursize = 0;
}

/** Makes sure a variable size buffer has enough space for data of a
  * certain length.
  *
  * \param buf    The buffer. It is enlarged if necessary.
  * \param length The length of data we need to add.
  * \return Pointer to where the new data can go.
  */
void* VS_GetSpace(vsbuf_t* buf, size_t length)
{
	void* data;

	if(buf->cursize + length > buf->maxsize)
	{
		if(!buf->allowoverflow)
			I_Error("overflow 111");

		if(length > buf->maxsize)
			I_Error("overflow l%d 112", length);

		buf->overflowed = true;
		CONS_Printf("VS buffer overflow");
		VS_Clear(buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

/** Copies data to the end of a variable size buffer.
  *
  * \param buf    The buffer.
  * \param data   The data to copy.
  * \param length The length of the data.
  * \sa VS_Print
  */
void VS_Write(vsbuf_t* buf, const void* data, size_t length)
{
	memcpy(VS_GetSpace(buf, length), data, length);
}

/** Prints text in a variable buffer. Like VS_Write() plus a
  * trailing NUL.
  *
  * \param buf  The buffer.
  * \param data The NUL-terminated string.
  * \sa VS_Write
  */
void VS_Print(vsbuf_t* buf, const char* data)
{
	size_t len;

	len = strlen(data) + 1;

	if(buf->data[buf->cursize-1])
		memcpy((byte*)VS_GetSpace(buf, len), data, len); // no trailing 0
	else
		memcpy((byte*)VS_GetSpace(buf, len-1) - 1, data, len); // write over trailing 0
}

// =========================================================================
//
//                           CONSOLE VARIABLES
//
//   console variables are a simple way of changing variables of the game
//   through the console or code, at run time.
//
//   console vars acts like simplified commands, because a function can be
//   attached to them, and called whenever a console var is modified
//
// =========================================================================

static const char* cv_null_string = "";

/** Searches if a variable has been registered.
  *
  * \param name Variable to search for.
  * \return Pointer to the variable if found, or NULL.
  * \sa CV_FindNetVar
  */
static consvar_t* CV_FindVar(const char* name)
{
	consvar_t* cvar;

	for(cvar = consvar_vars; cvar; cvar = cvar->next)
		if(!strcmp(name,cvar->name))
			return cvar;

	return NULL;
}

/** Builds a unique Net Variable identifier number, which is used
  * in network packets instead of the full name.
  *
  * \param s Name of the variable.
  * \return A new unique identifier.
  * \sa CV_FindNetVar
  */
static inline unsigned short CV_ComputeNetid(const char* s)
{
	unsigned short ret = 0, i = 0;
	static unsigned short premiers[16] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53};

	while(*s)
	{
		ret = (unsigned short)(ret + (*s)*premiers[i]);
		s++;
		i = (unsigned short)((i+1) % 16);
	}
	return ret;
}

/** Finds a net variable based on its identifier number.
  *
  * \param netid The variable's identifier number.
  * \return A pointer to the variable itself if found, or NULL.
  * \sa CV_ComputeNetid
  */
static consvar_t* CV_FindNetVar(unsigned short netid)
{
	consvar_t* cvar;

	for(cvar = consvar_vars; cvar; cvar = cvar->next)
		if(cvar->netid == netid)
			return cvar;

	return NULL;
}

static void Setvalue(consvar_t* var, const char* valstr);

/** Registers a variable for later use from the console.
  *
  * \param variable The variable to register.
  */
void CV_RegisterVar(consvar_t* variable)
{
	// first check to see if it has already been defined
	if(CV_FindVar(variable->name))
	{
		CONS_Printf("Variable %s is already defined\n", variable->name);
		return;
	}

	// check for overlap with a command
	if(COM_Exists(variable->name))
	{
		CONS_Printf("%s is a command name\n", variable->name);
		return;
	}

	// check net variables
	if(variable->flags & CV_NETVAR)
	{
		variable->netid = CV_ComputeNetid(variable->name);
		if(CV_FindNetVar(variable->netid))
			I_Error("Variable %s have same netid\n", variable->name);
	}

	// link the variable in
	if(!(variable->flags & CV_HIDEN))
	{
		variable->next = consvar_vars;
		consvar_vars = variable;
	}
	variable->string = variable->zstring = NULL;
	variable->changed = 0; // new variable has not been modified by the user

#ifdef PARANOIA
	if((variable->flags & CV_NOINIT) && !(variable->flags & CV_CALL))
		I_Error("variable %s has CV_NOINIT without CV_CALL\n");
	if((variable->flags & CV_CALL) && !variable->func)
		I_Error("variable %s has CV_CALL without a function");
#endif
	if(variable->flags & CV_NOINIT)
		variable->flags &= ~CV_CALL;

	Setvalue(variable, variable->defaultvalue);

	if(variable->flags & CV_NOINIT)
		variable->flags |= CV_CALL;

	// the SetValue will set this bit
	variable->flags &= ~CV_MODIFIED;
}

/** Finds the string value of a console variable.
  *
  * \param var_name The variable's name.
  * \return The string value or "" if the variable is not found.
  */
static const char* CV_StringValue(const char* var_name)
{
	consvar_t* var;

	var = CV_FindVar(var_name);
	if(!var)
		return cv_null_string;
	return var->string;
}

/** Completes the name of a console variable.
  *
  * \param partial The partial name of the variable (potentially).
  * \param skips   Number of variables to skip.
  * \return The complete variable name, or NULL.
  * \sa COM_CompleteCommand
  */
const char* CV_CompleteVar(char* partial, int skips)
{
	consvar_t* cvar;
	size_t len;

	len = strlen(partial);

	if(!len)
		return NULL;

	// check variables
	for(cvar = consvar_vars; cvar; cvar = cvar->next)
		if(!strncmp(partial, cvar->name, len))
			if(!skips--)
				return cvar->name;

	return NULL;
}

/** Sets a value to a variable with less checking. Only for internal use.
  *
  * \param var    Variable to set.
  * \param valstr String value for the variable.
  */
static void Setvalue(consvar_t* var, const char* valstr)
{
	if(var->PossibleValue)
	{
		int v = atoi(valstr);

		if(!stricmp(var->PossibleValue[0].strvalue, "MIN")) // bounded cvar
		{
			int i;
			// search for maximum
			for(i = 1; var->PossibleValue[i].strvalue; i++)
				if(!stricmp(var->PossibleValue[i].strvalue, "MAX"))
					break;
#ifdef PARANOIA
			if(!var->PossibleValue[i].strvalue)
				I_Error("Bounded cvar \"%s\" without maximum!", var->name);
#endif
			if(v < var->PossibleValue[0].value)
			{
				v = var->PossibleValue[0].value;
				valstr = var->PossibleValue[0].strvalue;
				//sprintf(valstr, "%d", v);
			}
			if(v > var->PossibleValue[i].value)
			{
				v = var->PossibleValue[i].value;
				valstr = var->PossibleValue[i].strvalue;
				//sprintf(valstr, "%d", v);
			}
		}
		else
		{
			int i;

			// check first strings
			for(i = 0; var->PossibleValue[i].strvalue; i++)
				if(!stricmp(var->PossibleValue[i].strvalue, valstr))
					goto found;
			if(!v)
				if(strcmp(valstr, "0"))
					goto error;
			// check int now
			for(i = 0; var->PossibleValue[i].strvalue; i++)
				if(v == var->PossibleValue[i].value)
					goto found;

error:
			// not found
			CONS_Printf("\"%s\" is not a possible value for \"%s\"\n", valstr, var->name);
			if(var->defaultvalue == valstr)
				I_Error("Variable %s default value \"%s\" is not a possible value\n",
					var->name,var->defaultvalue);
			return;
found:
			var->value = var->PossibleValue[i].value;
			var->string = var->PossibleValue[i].strvalue;
			goto finish;
		}
	}

	// free the old value string
	if(var->zstring)
		Z_Free(var->zstring);

	var->string = var->zstring = Z_StrDup(valstr);

	if(var->flags & CV_FLOAT)
	{
		double d;
		d = atof(var->string);
		var->value = (int)(d * FRACUNIT);
	}
	else
		var->value = atoi(var->string);

finish:
	if(var->flags & CV_SHOWMODIFONETIME || var->flags & CV_SHOWMODIF)
	{
		CONS_Printf("%s set to %s\n",var->name, var->string);
		var->flags &= ~CV_SHOWMODIFONETIME;
	}
	DEBFILE(va("%s set to %s\n", var->name, var->string));
	var->flags |= CV_MODIFIED;
	// raise 'on change' code
	if(var->flags & CV_CALL)
		var->func();
}

//
// Use XD_NETVAR argument:
//      2 byte for variable identification
//      then the value of the variable followed with a 0 byte (like str)
//
static void Got_NetVar(char** p, int playernum)
{
	consvar_t* cvar;
	unsigned short netid;
	char* svalue;

	if(playernum != serverplayer && playernum != adminplayer)
	{
		// not from server or remote admin, must be hacked/buggy client
		CONS_Printf("Illegal netvar command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	netid = READUSHORT(*p);
	cvar = CV_FindNetVar(netid);
	svalue = *p;
	SKIPSTRING(*p);
	DEBFILE(va("Netvar received: %s [netid=%d] value %s\n", cvar->name, netid, svalue));
	if(!cvar)
	{
		CONS_Printf("\2Netvar not found with netid %hu\n", netid);
		return;
	}
	Setvalue(cvar, svalue);
}

// get implicit parameter save_p
void CV_SaveNetVars(char** p)
{
	consvar_t* cvar;

	// we must send all cvars because on the other side maybe
	// it has a cvar modified and here not (same for true savegame)
	for(cvar = consvar_vars; cvar; cvar = cvar->next)
		if(cvar->flags & CV_NETVAR)
		{
			WRITESHORT(*p, cvar->netid);
			WRITESTRING(*p, cvar->string);
		}
}

// get implicit parameter save_p
void CV_LoadNetVars(char** p)
{
	consvar_t* cvar;

	for(cvar = consvar_vars; cvar; cvar = cvar->next)
		if(cvar->flags & CV_NETVAR)
			Got_NetVar(p, 0);
}

/** Sets a value to a variable without calling its callback function.
  *
  * \param var   The variable.
  * \param value The string value.
  * \sa CV_Set, CV_StealthSetValue
  */
void CV_StealthSet(consvar_t* var, const char* value)
{
	int oldflags;

	oldflags = var->flags;
	var->flags &= ~CV_CALL;
	CV_Set(var, value);
	var->flags = oldflags;
}

/** Sets a value to a variable, performing some checks and calling the
  * callback function if there is one.
  * Does as if "<varname> <value>" is entered at the console.
  *
  * \param var   The variable.
  * \param value The string value.
  * \sa CV_StealthSet, CV_SetValue
  */
void CV_Set(consvar_t* var, const char* value)
{
#ifdef PARANOIA
	if(!var)
		I_Error("CV_Set: no variable\n");
	if(!var->string)
		I_Error("CV_Set: %s no string set!\n", var->name);
#endif
	if(!stricmp(var->string, value))
		return; // no changes

	// Don't allow skin/color changes in single player
	if((var == &cv_skin || var == &cv_playercolor) &&
		!(cv_debug || devparm) && !(multiplayer || netgame)
		&& (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION
		|| gamestate == GS_FINALE))
	{
		return;
	}

	if(var->flags & CV_NETVAR)
	{
		// send the value of the variable
		char buf[128];
		char* p;
		if(!(server || admin))
		{
			CONS_Printf("Only the server can change this variable: %s %s\n",
				var->name, var->string);
			return;
		}
		p = buf;
		WRITEUSHORT(p, var->netid);
		WRITESTRING(p, value);
		SendNetXCmd(XD_NETVAR, buf, p-buf);
	}
	else
		if((var->flags & CV_NOTINNET) && netgame)
		{
			CONS_Printf("This variable can't be changed while in netgame: %s %s\n",
				var->name, var->string);
			return;
		}
		else
			Setvalue(var, value);
}

/** Sets a numeric value to a variable without calling its callback 
  * function.
  *
  * \param var   The variable.
  * \param value The numeric value, converted to a string before setting.
  * \sa CV_SetValue, CV_StealthSet
  */
void CV_StealthSetValue(consvar_t* var, int value)
{
	int oldflags;

	oldflags = var->flags;
	var->flags &= ~CV_CALL;
	CV_SetValue(var, value);
	var->flags = oldflags;
}

/** Sets a numeric value to a variable, performing some checks and 
  * calling the callback function if there is one.
  *
  * \param var   The variable.
  * \param value The numeric value, converted to a string before setting.
  * \sa CV_Set, CV_StealthSetValue
  */
void CV_SetValue(consvar_t* var, int value)
{
	char val[32];

	sprintf(val, "%d", value);
	CV_Set(var, val);
}

/** Adds a value to a console variable.
  * Used to increment and decrement variables from the menu.
  * Contains special cases to handle pointlimit in some multiplayer modes,
  * map number for game hosting, etc.
  *
  * \param var       The variable to add to.
  * \param increment The change in the variable; can be negative for a
  *                  decrement.
  * \sa CV_SetValue
  */
void CV_AddValue(consvar_t* var, int increment)
{
	int newvalue, gt;

	// count pointlimit better
	if(var == &cv_pointlimit && (gametype == GT_MATCH || gametype == GT_CHAOS))
		increment *= 50;
	newvalue = var->value + increment;

	if(var->PossibleValue)
	{
#define MINVAL 0
		if(!strcmp(var->PossibleValue[MINVAL].strvalue, "MIN"))
		{
			int max;
			// search the next to last
			for(max = 0; var->PossibleValue[max+1].strvalue; max++)
				;

			if(newvalue < var->PossibleValue[MINVAL].value) // add the max+1
				newvalue += var->PossibleValue[max].value - var->PossibleValue[MINVAL].value + 1;

			newvalue = var->PossibleValue[MINVAL].value + (newvalue - var->PossibleValue[MINVAL].value)
				% (var->PossibleValue[max].value - var->PossibleValue[MINVAL].value + 1);

			CV_SetValue(var, newvalue);
#undef MINVAL
		}
		else
		{
			int max, currentindice = -1, newindice;

			// this code do not support more than same value for differant PossibleValue
			for(max = 0; var->PossibleValue[max].strvalue; max++)
				if(var->PossibleValue[max].value == var->value)
					currentindice = max;

			max--;

			if(var == &cv_nextmap)
			{
				// Special case for the nextmap variable, used only directly from the menu
				int oldvalue = var->value - 1;
				gt = cv_newgametype.value;
				if(increment > 0) // Going up!
				{
					newvalue = var->value - 1;
					do
					{
						newvalue++;
						if(newvalue == 1035)
							newvalue = 0;
						if(newvalue == oldvalue)
							gt = -1; // don't loop forever if there's none of a certain gametype
					} while(var->PossibleValue[newvalue].strvalue == NULL
						|| (gt == GT_COOP && !(mapheaderinfo[newvalue].typeoflevel & TOL_COOP))
						|| ((gt == GT_MATCH||gt==42) && !(mapheaderinfo[newvalue].typeoflevel & TOL_MATCH))
						|| ((gt == GT_RACE||gt==43) && !(mapheaderinfo[newvalue].typeoflevel & TOL_RACE))
						|| (gt == GT_TAG && !(mapheaderinfo[newvalue].typeoflevel & TOL_TAG))
						|| (gt == GT_CTF && !(mapheaderinfo[newvalue].typeoflevel & TOL_CTF))
						|| (gt == GT_CHAOS && !(mapheaderinfo[newvalue].typeoflevel & TOL_CHAOS)));
					var->value = newvalue + 1;
					var->string = var->PossibleValue[newvalue].strvalue;
					return;
				}
				else if(increment < 0) // Going down!
				{
					newvalue = var->value - 1;
					do
					{
						newvalue--;
						if(newvalue == -1)
							newvalue = 1034;
						if(newvalue == oldvalue)
							gt = -1; // don't loop forever if there's none of a certain gametype
					} while(var->PossibleValue[newvalue].strvalue == NULL
						|| (gt == GT_COOP && !(mapheaderinfo[newvalue].typeoflevel & TOL_COOP))
						|| (gt == GT_MATCH && !(mapheaderinfo[newvalue].typeoflevel & TOL_MATCH))
						|| (gt == GT_RACE && !(mapheaderinfo[newvalue].typeoflevel & TOL_RACE))
						|| (gt == GT_TAG && !(mapheaderinfo[newvalue].typeoflevel & TOL_TAG))
						|| (gt == GT_CTF && !(mapheaderinfo[newvalue].typeoflevel & TOL_CTF))
						|| (gt == GT_CHAOS && !(mapheaderinfo[newvalue].typeoflevel & TOL_CHAOS)));
					var->value = newvalue + 1;
					var->string = var->PossibleValue[newvalue].strvalue;
					return;
				}
			}

#ifdef PARANOIA
			if(currentindice == -1)
				I_Error("CV_AddValue: current value %d not found in possible value\n",
					var->value);
#endif

			newindice = (currentindice + increment + max + 1) % (max+1);
			CV_Set(var, var->PossibleValue[newindice].strvalue);
		}
	}
	else
		CV_SetValue(var, newvalue);

	var->changed = 1; // user has changed it now
}

/** Displays or changes a variable from the console.
  * Since the user is presumed to have been directly responsible
  * for this change, the variable is marked as changed this game.
  *
  * \return False if passed command was not recognized as a console
  *         variable, otherwise true.
  * \sa CV_ClearChangedFlags
  */
static boolean CV_Command(void)
{
	consvar_t* v;

	// check variables
	v = CV_FindVar(COM_Argv(0));
	if(!v)
		return false;

	// perform a variable print or set
	if(COM_Argc() == 1)
	{
		CONS_Printf("\"%s\" is \"%s\" default is \"%s\"\n", v->name, v->string, v->defaultvalue);
		return true;
	}

	CV_Set(v, COM_Argv(1));
	v->changed = 1; // now it's been changed by (presumably) the user
	return true;
}

/** Marks all variables as unchanged, indicating they've not been changed
  * by the user this game.
  *
  * \sa CV_Command
  * \author Graue <graue@oceanbase.org>
  */
void CV_ClearChangedFlags(void)
{
	consvar_t* cvar;

	for(cvar=consvar_vars; cvar; cvar = cvar->next)
		cvar->changed = 0;
}

/** Saves console variables to a file if they have the ::CV_SAVE
  * flag set.
  *
  * \param f File to save to.
  */
void CV_SaveVariables(FILE* f)
{
	consvar_t* cvar;

	for(cvar = consvar_vars; cvar; cvar = cvar->next)
		if(cvar->flags & CV_SAVE)
			fprintf(f, "%s \"%s\"\n", cvar->name, cvar->string);
}

//============================================================================
//                            SCRIPT PARSE
//============================================================================

/** Parses a token out of a string. Handles script files too.
  *
  * \param data String to parse.
  * \return The data pointer after the token.
  */
static char* COM_Parse(char* data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if(!data)
		return NULL;

	// skip whitespace
skipwhite:
	while((c = *data) <= ' ')
	{
		if(!c)
			return NULL; // end of file;
		data++;
	}

	// skip // comments
	if(c == '/' && data[1] == '/')
	{
		while(*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if(c == '\"')
	{
		data++;
		for(;;)
		{
			c = *data++;
			if(c == '\"' || c == '\0')
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = (char)c;
			len++;
		}
	}

	// parse single characters
	if(c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':')
	{
		com_token[len] = (char)c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = (char)c;
		data++;
		len++;
		c = *data;
		if(c == '{' || c == '}' || c == ')'|| c == '(' || c == '\'' || c == ':')
			break;
	} while(c > 32);

	com_token[len] = 0;
	return data;
}
