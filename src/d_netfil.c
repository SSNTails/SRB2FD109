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
/// \brief Transfer a file using HSendPacket.

#include <stdio.h>
#ifdef SDLIO
#include <SDL/SDL_rwops.h>
#else
#ifndef _WIN32_WCE
#include <fcntl.h>
#endif
#endif
#ifndef _WIN32_WCE
#ifdef __OS2__
#include <sys/types.h>
#endif // __OS2__
#include <sys/stat.h>
#endif

#if !defined(UNDER_CE)
#include <time.h>
#endif

#if (defined(_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64) || defined(__DJGPP__)
#include <io.h>
#include <direct.h>
#elif !defined (_WIN32_WCE)
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#elif (defined (_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)
#include <sys/utime.h>
#endif
#ifdef __DJGPP__
#include <dir.h>
#include <utime.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "i_net.h"
#include "i_system.h"
#include "m_argv.h"
#include "d_net.h"
#include "w_wad.h"
#include "d_netfil.h"
#include "z_zone.h"
#include "byteptr.h"
#include "p_setup.h"
#include "m_misc.h"
#include "m_menu.h"
#ifndef NOMD5
#include "md5.h"
#endif
#include "filesrch.h"

// sender structure
typedef struct filetx_s
{
	int ram;
	char* filename; // name of the file or ptr of the data in ram
	ULONG size;
	char fileid;
	int node; // destination
	struct filetx_s* next; // a queue
} filetx_t;

// current transfers (one for each node)
typedef struct filetran_s
{
	filetx_t* txlist;
	ULONG position;
	FILE* currentfile;
} filetran_t;
static filetran_t transfer[MAXNETNODES];

// read time of file: stat _stmtime
// write time of file: utime

// receiver structure
int fileneedednum;
fileneeded_t fileneeded[MAX_WADFILES];
char downloaddir[256] = "DOWNLOAD";

/** Fills a serverinfo packet with information about wad files loaded.
  *
  * \todo Give this function a better name since it is in global scope.
  */
char* PutFileNeeded(void)
{
	int i, count = 0;
	char* p;
	char wadfilename[MAX_WADPATH];

	p = (char *)&netbuffer->u.serverinfo.fileneeded;
	for(i = 0; i < numwadfiles; i++)
	{
		// if it has only music/sound lumps, mark it as unimportant
		if(W_VerifyNMUSlumps(wadfiles[i]->filename))
			WRITEBYTE(p, 0);
		else
			WRITEBYTE(p, 1); // important

		if(cv_nodownloading.value)
			WRITEBYTE(p, 2); // won't send
		else if((wadfiles[i]->filesize > (unsigned)cv_maxsend.value * 1024))
			WRITEBYTE(p, 0); // won't send
		else
			WRITEBYTE(p, 1); // will send if requested

		count++;
		WRITEULONG(p, wadfiles[i]->filesize);
		strcpy(wadfilename, wadfiles[i]->filename);
		nameonly(wadfilename);
		WRITESTRING(p, wadfilename);
		WRITEMEM(p, wadfiles[i]->md5sum, 16);
	}
	netbuffer->u.serverinfo.fileneedednum = (byte)count;
	return p;
}

// parse the serverinfo packet and fill fileneeded table on client
void D_ParseFileneeded(int fileneedednum_parm, char* fileneededstr)
{
	int i;
	byte* p;

	fileneedednum = fileneedednum_parm;
	p = (byte *)fileneededstr;
	for(i = 0; i < fileneedednum; i++)
	{
		fileneeded[i].status = FS_NOTFOUND;
		fileneeded[i].important = READBYTE(p);
		fileneeded[i].willsend = READBYTE(p);
		fileneeded[i].totalsize = READULONG(p);
		fileneeded[i].phandle = NULL;
		READSTRING(p, fileneeded[i].filename);
		READMEM(p, fileneeded[i].md5sum, 16);
	}
}

void CL_PrepareDownloadSaveGame(const char* tmpsave)
{
	fileneedednum = 1;
	fileneeded[0].status = FS_REQUESTED;
	fileneeded[0].totalsize = (ULONG)-1;
	fileneeded[0].phandle = NULL;
	memset(fileneeded[0].md5sum, 0, 16);
	strcpy(fileneeded[0].filename, tmpsave);
}

/** Send requests for files in the ::fileneeded table with a status of
  * ::FS_NOTFOUND.
  *
  * \todo Global function, needs a different name.
  * \todo Cleanup, too long.
  */
boolean SendRequestFile(void)
{
	boolean candownloadfiles = true;
	char* p;
	int i;
	ULONG totalfreespaceneeded = 0;
	INT64 availablefreespace;

	if(M_CheckParm("-nodownload"))
		candownloadfiles = false;
	else
		for(i = 0; i < fileneedednum; i++)
			if(fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN
				&& fileneeded[i].important && (fileneeded[i].willsend == 0 || fileneeded[i].willsend == 2))
			{
				candownloadfiles = false;
			}

	if(!candownloadfiles)
	{
		boolean coulddownload = true;
		char s[(MAX_WADPATH+100)*MAX_WADFILES] = ""; // more space than needed but safe

		for(i = 0; i < fileneedednum; i++)
			if(fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN
				&& fileneeded[i].important)
			{
				size_t strl;

				strl = strlen(s);
				sprintf(&s[strl], "  \"%s\" (%luKB)", fileneeded[i].filename,
					fileneeded[i].totalsize / 1024);

				if(fileneeded[i].status == FS_NOTFOUND)
					strcat(s, " not found");
				else if(fileneeded[i].status == FS_MD5SUMBAD)
				{
					int j;

					strcat(s, " has wrong md5sum, needs: ");
					strl = strlen(s);

					for(j = 0; j < 16; j++)
						sprintf(&s[strl+2*j], "%02x", fileneeded[i].md5sum[j]);
					s[strl+32]='\0';
				}
				if(fileneeded[i].willsend != 1)
				{
					coulddownload = false;

					if(fileneeded[i].willsend == 2)
						strcat(s, " (server has downloading disabled)");
					else
						strcat(s, " (too big to download from server)");
				}
				strcat(s, "\n");
			}

		if(coulddownload)
			I_Error("To play on this server you should have these files:\n%s\n"
				"Remove -nodownload if you want to download the files!\n", s);

		I_Error("To play on this server you need these files:\n%s\n"
			"Make sure you get them somewhere, or you won't be able to join!\n", s);
	}

	netbuffer->packettype = PT_REQUESTFILE;
	p = (char *)netbuffer->u.textcmd;
	for(i = 0; i < fileneedednum; i++)
		if((fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD)
			&& fileneeded[i].important)
		{
			totalfreespaceneeded += fileneeded[i].totalsize;
			nameonly(fileneeded[i].filename);
			WRITECHAR(p, i); // fileid
			WRITESTRING(p, fileneeded[i].filename);
			// put it in download dir
			strcatbf(fileneeded[i].filename, downloaddir, "/");
			fileneeded[i].status = FS_REQUESTED;
		}
	WRITECHAR(p, -1);
	I_GetDiskFreeSpace(&availablefreespace);
	if(totalfreespaceneeded > availablefreespace)
		I_Error("To play on this server you should download %dKb\n"
			"but you have only %dKb freespace on this drive\n",
			totalfreespaceneeded, availablefreespace);

	// prepare to download
	I_mkdir(downloaddir, 0755);
	return HSendPacket(servernode, true, 0, p - (char*)netbuffer->u.textcmd);
}

// get request filepak and put it on the send queue
void Got_RequestFilePak(int node)
{
	char* p;

	p = (char *)netbuffer->u.textcmd;
	while(*p != (char)-1)
	{
		SendFile(node, p + 1, *p);
		p++; // skip fileid
		SKIPSTRING(p);
	}
}

// client check if the fileneeded aren't already loaded or on the disk
int CL_CheckFiles(void)
{
	int i, j;
	char wadfilename[MAX_WADPATH];
	int ret = 1;

	if(M_CheckParm("-nofiles"))
		return 1;

	// the first is the iwad (the main wad file)
	// do not check file date, also don't download it (copyright problem)
	strcpy(wadfilename, wadfiles[0]->filename);
	nameonly(wadfilename);
	if(stricmp(wadfilename, fileneeded[0].filename))
	{
		M_StartMessage(va("You cannot connect to this server\nsince it uses %s\n"
			"You are using %s\n", fileneeded[0].filename, wadfilename), NULL, MM_NOTHING);
		return 2;
	}
	fileneeded[0].status = FS_OPEN;

	for(i = 1; i < fileneedednum; i++)
	{
		if(devparm)
			CONS_Printf("searching for '%s' ", fileneeded[i].filename);

		// check in allready loaded files
		for(j = 1; wadfiles[j]; j++)
		{
			strcpy(wadfilename, wadfiles[j]->filename);
			nameonly(wadfilename);
			if(!stricmp(wadfilename, fileneeded[i].filename) &&
				!memcmp(wadfiles[j]->md5sum, fileneeded[i].md5sum, 16))
			{
				if(devparm)
					CONS_Printf("already loaded\n");
				fileneeded[i].status = FS_OPEN;
				break;
			}
		}
		if(fileneeded[i].status != FS_NOTFOUND || !fileneeded[i].important)
			continue;

		fileneeded[i].status = findfile(fileneeded[i].filename, fileneeded[i].md5sum, true);
		if(devparm)
			CONS_Printf("found %d\n", fileneeded[i].status);
		if(fileneeded[i].status != FS_FOUND)
			ret = 0;
	}
	return ret;
}

// load it now
void CL_LoadServerFiles(void)
{
	int i;

	for(i = 1; i < fileneedednum; i++)
	{
		if(fileneeded[i].status == FS_OPEN)
			continue; // already loaded
		else if(fileneeded[i].status == FS_FOUND)
		{
			P_AddWadFile(fileneeded[i].filename, NULL);
			fileneeded[i].status = FS_OPEN;
		}
		else if(fileneeded[i].status == FS_MD5SUMBAD)
		{
			// If the file is marked important, don't even bother proceeding.
			if(fileneeded[i].important)
				I_Error("Wrong version of important file %s", fileneeded[i].filename);

			// If it isn't, no need to worry the user with a console message,
			// although it can't hurt to put something in the debug file.

			// ...but wait a second. What if the local version is "important"?
			if(!W_VerifyNMUSlumps(fileneeded[i].filename))
				I_Error("File %s should only contain music and sound effects!",
					fileneeded[i].filename);

			// Okay, NOW we know it's safe. Whew.
			P_AddWadFile(fileneeded[i].filename, NULL);
			fileneeded[i].status = FS_OPEN;
			DEBFILE(va("File %s found but with different md5sum\n", fileneeded[i].filename));
		}
		else if(fileneeded[i].important)
			I_Error("Try to load file %s with status of %d\n", fileneeded[i].filename,
				fileneeded[i].status);
	}
}

// little optimization to test if there is a file in the queue
static int filetosend = 0;

void SendFile(int node, char* filename, char fileid)
{
	filetx_t** q;
	filetx_t* p;
	int i;
	char wadfilename[MAX_WADPATH];

	q = &transfer[node].txlist;
	while(*q)
		q = &((*q)->next);
	*q=(filetx_t*)malloc(sizeof(filetx_t));
	if(!*q)
		I_Error("SendFile: No more ram\n");
	p = *q;
	p->filename = (char*)malloc(MAX_WADPATH);
	if(!p->filename)
		I_Error("SendFile: No more ram\n");
	strcpy(p->filename, filename);

	// a minimum of security, can get only file in srb2 direcory
	nameonly(p->filename);

	// check first in wads loaded the majority of case
	for(i = 0; wadfiles[i]; i++)
	{
		strcpy(wadfilename, wadfiles[i]->filename);
		nameonly(wadfilename);
		if(!stricmp(wadfilename, p->filename))
		{
			// copy filename with full path
			strcpy(p->filename, wadfiles[i]->filename);
			break;
		}
	}

	if(!wadfiles[i])
	{
		DEBFILE(va("%s not found in wadfiles\n", filename));
		// this formerly checked if(!findfile(p->filename, NULL, true))

		// not found
		// don't inform client (probably hacker)
		DEBFILE(va("Client %d request %s: not found\n", node, filename));
		free(p->filename);
		free(p);
		*q = NULL;
		return;
	}

	if(wadfiles[i]->filesize > (unsigned)cv_maxsend.value * 1024)
	{
		// too big
		// don't inform client (client sucks, man)
		DEBFILE(va("Client %d request %s: file too big, not sending\n", node, filename));
		free(p->filename);
		free(p);
		*q = NULL;
		return;
	}

	DEBFILE(va("Sending file %s (id=%d) to %d\n", filename, fileid, node));
	p->ram = SF_FILE;
	p->fileid = fileid;
	p->next = NULL; // end of list
	filetosend++;
}

void SendRam(int node, byte* data, size_t size, freemethod_t freemethod, char fileid)
{
	filetx_t** q;
	filetx_t* p;

	q = &transfer[node].txlist;
	while(*q)
		q = &((*q)->next);
	*q = (filetx_t*)malloc(sizeof(filetx_t));
	if(!*q)
		I_Error("SendRam : No more ram\n");
	p = *q;
	p->ram = freemethod;
	p->filename = (char *)data;
	p->size = (ULONG)size;
	p->fileid = fileid;
	p->next = NULL; // end of list

	DEBFILE(va("Sending ram %x( size:%d) to %d (id=%d)\n",p->filename,size,node,fileid));

	filetosend++;
}

static void EndSend(int node)
{
	filetx_t* p = transfer[node].txlist;
	switch(p->ram)
	{
		case SF_FILE:
			if(transfer[node].currentfile)
				fclose(transfer[node].currentfile);
			free(p->filename);
			break;
		case SF_Z_RAM:
			Z_Free(p->filename);
			break;
		case SF_RAM:
			free(p->filename);
		case SF_NOFREERAM:
			break;
	}
	transfer[node].txlist = p->next;
	transfer[node].currentfile = NULL;
	free(p);
	filetosend--;
}

#define PACKETPERTIC net_bandwidth/(TICRATE*software_MAXPACKETLENGTH)

void FiletxTicker(void)
{
	static int currentnode = 0;
	filetx_pak* p;
	size_t size;
	filetx_t* f;
	int packetsent = PACKETPERTIC, ram, i;

	if(!filetosend)
		return;
	if(!packetsent)
		packetsent++;
	// (((sendbytes-nowsentbyte)*TICRATE)/(I_GetTime()-starttime)<(ULONG)net_bandwidth)
	while( packetsent-- && filetosend!=0)
	{
		for(i=currentnode,ram=0;ram<MAXNETNODES;i=(i+1)%MAXNETNODES,ram++)
			if(transfer[i].txlist)
				goto found;
		// no transfer to do
		I_Error("filetosend=%d but no filetosend found\n",filetosend);
	found:
		currentnode=(i+1)%MAXNETNODES;
		f=transfer[i].txlist;
		ram=f->ram;

		if(!transfer[i].currentfile) // file not allready open
		{
			if(!ram)
			{
				long filesize;

				transfer[i].currentfile = fopen(f->filename, "rb");

				if(!transfer[i].currentfile)
					I_Error("File %s not exist", f->filename);

				fseek(transfer[i].currentfile, 0, SEEK_END);
				filesize = ftell(transfer[i].currentfile);

				// nobody wants to transfer a file bigger than 4GB!
				// and computers will never need more than 640kb of RAM ;-)
				if(-1 == filesize)
				{
#ifndef SDLIO
					perror("Error");
#endif
					I_Error("Error getting filesize of %s\n", f->filename);
				}

				f->size = filesize;
				fseek(transfer[i].currentfile, 0, SEEK_SET);
			}
			else
				transfer[i].currentfile = (FILE*)1;
			transfer[i].position=0;
		}

		p = &netbuffer->u.filetxpak;
		size = software_MAXPACKETLENGTH - (FILETXHEADER + BASEPACKETSIZE);
		if(f->size-transfer[i].position < size)
			size = f->size-transfer[i].position;
		if(ram)
			memcpy(p->data, &f->filename[transfer[i].position], size);
		else if(fread(p->data, size, 1, transfer[i].currentfile) != 1)
			I_Error("FiletxTicker : can't get %d byte on %s at %d",size,f->filename,transfer[i].position);
		p->position = transfer[i].position;
		// put flag so receiver know the totalsize
		if(transfer[i].position + size == f->size)
			p->position |= 0x80000000;
		p->fileid = f->fileid;
		p->size = (USHORT)size;
		netbuffer->packettype = PT_FILEFRAGMENT;
		if(!HSendPacket(i, true, 0, FILETXHEADER + size)) // reliable SEND
		{ // not sent for some odd reason, retry at next call
			if(!ram)
				fseek(transfer[i].currentfile,transfer[i].position,SEEK_SET);
			// exit the while (can't send this one so why should i send the next?)
			break;
		}
		else // success
		{
			transfer[i].position = (ULONG)(size+transfer[i].position);
			if(transfer[i].position==f->size) //  finish ?
				EndSend(i);
		}
	}
}

void Got_Filetxpak(void)
{
	int filenum = netbuffer->u.filetxpak.fileid;
	static int time = 0;

	if(filenum >= fileneedednum)
	{
		DEBFILE(va("fileframent not needed %d>%d\n",filenum, fileneedednum));
		return;
	}

	if( fileneeded[filenum].status==FS_REQUESTED )
	{
		if(fileneeded[filenum].phandle) I_Error("Got_Filetxpak : allready open file\n");
			fileneeded[filenum].phandle=fopen(fileneeded[filenum].filename, "wb");
		if(!fileneeded[filenum].phandle) I_Error("Can't create file %s : disk full ?",fileneeded[filenum].filename);
			CONS_Printf("\r%s...",fileneeded[filenum].filename);
		fileneeded[filenum].currentsize = 0;
		fileneeded[filenum].status=FS_DOWNLOADING;
	}

	if( fileneeded[filenum].status==FS_DOWNLOADING )
	{
		// use a special tric to know when file is finished (not allways used)
		// WARNING: filepak can arrive out of order so don't stop now !
		if( netbuffer->u.filetxpak.position & 0x80000000 )
		{
			netbuffer->u.filetxpak.position &= ~0x80000000;
			fileneeded[filenum].totalsize = netbuffer->u.filetxpak.position + netbuffer->u.filetxpak.size;
		}
		// we can receive packet in the wrong order, anyway all os support gaped file
		fseek(fileneeded[filenum].phandle,netbuffer->u.filetxpak.position,SEEK_SET);
		if( fwrite(netbuffer->u.filetxpak.data,netbuffer->u.filetxpak.size,1,fileneeded[filenum].phandle)!=1 )
			I_Error("Can't write %s : disk full ?\n",fileneeded[filenum].filename);
		fileneeded[filenum].currentsize+=netbuffer->u.filetxpak.size;
		if(time==0)
		{
			Net_GetNetStat();
			CONS_Printf("\r%s %dK/%dK %.1fK/s",fileneeded[filenum].filename,
			                                   fileneeded[filenum].currentsize>>10,
			                                   fileneeded[filenum].totalsize>>10,
			                                   ((float)getbps)/1024);
		}

		// finish ?
		if(fileneeded[filenum].currentsize==fileneeded[filenum].totalsize)
		{
			fclose(fileneeded[filenum].phandle);
			fileneeded[filenum].phandle=NULL;
			fileneeded[filenum].status=FS_FOUND;
			CONS_Printf("\rDownloading %s...(done)\n",fileneeded[filenum].filename);
		}
	}
	else
		I_Error("Received a file not requested\n");
	// send ack back quickly

	if(++time==4)
	{
		Net_SendAcks(servernode);
		time=0;
	}

}

void AbortSendFiles(int node)
{
	while(transfer[node].txlist)
		EndSend(node);
}

void CloseNetFile(void)
{
	int i;
	// is sending?
	for(i = 0; i < MAXNETNODES; i++)
		AbortSendFiles(i);

	// receiving a file?
	for(i = 0; i < MAX_WADFILES; i++)
		if(fileneeded[i].status == FS_DOWNLOADING && fileneeded[i].phandle)
		{
			fclose(fileneeded[i].phandle);
			// file is not complete delete it
			remove(fileneeded[i].filename);
		}

	// remove FILEFRAGMENT from acknledge list
	Net_AbortPacketType(PT_FILEFRAGMENT);
}

// functions cut and pasted from doomatic :)

void nameonly(char* s)
{
	size_t j;

	for(j=strlen(s);j != (size_t)-1;j--)
		if( (s[j]=='\\') || (s[j]==':') || (s[j]=='/') )
		{
			memcpy(s, &(s[j+1]), strlen(&(s[j+1]))+1 );
			return;
		}
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

// UNUSED for now
boolean fileexist(char* filename, time_t time)
{
#ifdef SDLIO
	SDL_RWops* handle;
	time = 0; //?????
	handle = SDL_RWFromFile(filename, "rb");
	if(handle)
	{
		SDL_RWclose(handle);
#else
	int handel;
	handel = open(filename, O_RDONLY|O_BINARY);
	if(handel != -1)
	{
		close(handel);
		if(time)
		{
			struct stat bufstat;
			stat(filename,&bufstat);
			if(time != bufstat.st_mtime)
				return false;
		}
#endif
		return true;
	}
	else
		return false;
}

filestatus_t checkfilemd5(char* filename, const unsigned char* wantedmd5sum)
{
#ifndef NOMD5
	FILE* fhandle;
	unsigned char md5sum[16];

	if(!wantedmd5sum)
		return FS_FOUND;

	fhandle = fopen(filename, "rb");
	if(fhandle)
	{
		md5_stream(fhandle,md5sum);
		fclose(fhandle);
		if(!memcmp(wantedmd5sum, md5sum, 16))
			return FS_FOUND;
		return FS_MD5SUMBAD;
	}

	I_Error("Couldn't open %s for md5 check");
#endif
	return FS_FOUND; // will never happen, but makes the compiler shut up
}

filestatus_t findfile(char* filename, const unsigned char* wantedmd5sum, boolean completepath)
{
	filestatus_t homecheck = filesearch(filename, srb2home, wantedmd5sum, false, 10);
	//FIXME: implement wadpath-search
	//just for the start... recursive 10 levels from current dir should bring back old behaviour

	if(homecheck == FS_FOUND)
		return filesearch(filename, srb2home, wantedmd5sum, completepath, 10);
	return filesearch(filename, ".", wantedmd5sum, completepath, 10);
}
