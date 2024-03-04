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
//      stub and replacement "ANSI" C functions for use under Windows CE
// 
//-----------------------------------------------------------------------------

#include "../../doomdef.h"
#include "cehelp.h"

#define _SEC_IN_MINUTE 60
#define _SEC_IN_HOUR 3600
#define _SEC_IN_DAY 86400

static const int DAYS_IN_MONTH[12] =
{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define _DAYS_IN_MONTH(x) ((x == 1) ? days_in_feb : DAYS_IN_MONTH[x])

static const int _DAYS_BEFORE_MONTH[12] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

char* strerror(int ecode)
{
	static char buff[1024 + 1];
	DWORD dwMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		ecode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &buff[0], 1024, NULL);
	return buff;
}

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

int unlink( const char *filename )
{
	return remove(filename);
}

int remove( const char *path )
{
	return DeleteFileA(path)-1;
}

static inline void STToTM(const SYSTEMTIME *st, struct tm *tm)
{
	if(!st || !tm) return;
	tm->tm_sec   = st->wSecond;
	tm->tm_min   = st->wMinute;
	tm->tm_hour  = st->wHour;
	tm->tm_mday  = st->wDay;
	tm->tm_mon   = st->wMonth - 1;
	tm->tm_year  = st->wYear - 1900;
	tm->tm_wday  = st->wDayOfWeek;
	tm->tm_yday  = 0;
	tm->tm_isdst = 0;
}

time_t time(time_t *T)
{
	time_t returntime;
	SYSTEMTIME st;
	struct tm stft;
	GetSystemTime(&st);
	STToTM(&st,&stft);
	returntime = mktime(&stft);
	if(T) *T = returntime;
	return returntime;
}

static inline UINT64 TTtoFT(const time_t wt, FILETIME *ft)
{
	UINT64 temptime = wt; // FILETIME: 1/(10^7) secs since January 1, 1601
	temptime *= 10000000; // time_t  : 1 secs since January 1, 1970
	// 369 years * 365 days * 24 hours * 60 mins * 60 secs * 10
	//       123 leaps days * 24 hours * 60 mins * 60 secs * 10
	  temptime += 116444736000000000;
	CopyMemory(ft,&temptime,sizeof(ULARGE_INTEGER));
	return temptime;
}

static struct tm cehelptm;

struct tm * localtime(const time_t *CLOCK)
{
	SYSTEMTIME st;
	FILETIME stft;
	UINT64 ftli = 0;
	if(CLOCK) ftli = TTtoFT(*CLOCK, &stft);
	if(ftli)
		FileTimeToSystemTime(&stft,&st);
	else
		GetSystemTime(&st);
	STToTM(&st,&cehelptm);
	if(st.wYear >= 1970)
		return &cehelptm;
	else
		return NULL;
}

static void validate_structure (struct tm *tim_p) // from newlib
{
  div_t res;
  int days_in_feb = 28;

  /* calculate time & date to account for out of range values */
  if (tim_p->tm_sec < 0 || tim_p->tm_sec > 59)
    {
      res = div (tim_p->tm_sec, 60);
      tim_p->tm_min += res.quot;
      if ((tim_p->tm_sec = res.rem) < 0)
	{
	  tim_p->tm_sec += 60;
	  --tim_p->tm_min;
	}
    }

  if (tim_p->tm_min < 0 || tim_p->tm_min > 59)
    {
      res = div (tim_p->tm_min, 60);
      tim_p->tm_hour += res.quot;
      if ((tim_p->tm_min = res.rem) < 0)
	{
	  tim_p->tm_min += 60;
	  --tim_p->tm_hour;
        }
    }

  if (tim_p->tm_hour < 0 || tim_p->tm_hour > 23)
    {
      res = div (tim_p->tm_hour, 24);
      tim_p->tm_mday += res.quot;
      if ((tim_p->tm_hour = res.rem) < 0)
	{
	  tim_p->tm_hour += 24;
	  --tim_p->tm_mday;
        }
    }

  if (tim_p->tm_mon > 11)
    {
      res = div (tim_p->tm_mon, 12);
      tim_p->tm_year += res.quot;
      if ((tim_p->tm_mon = res.rem) < 0)
        {
	  tim_p->tm_mon += 12;
	  --tim_p->tm_year;
        }
    }

  if (_DAYS_IN_YEAR (tim_p->tm_year) == 366)
    days_in_feb = 29;

  if (tim_p->tm_mday <= 0)
    {
      while (tim_p->tm_mday <= 0)
	{
	  if (--tim_p->tm_mon == -1)
	    {
	      tim_p->tm_year--;
	      tim_p->tm_mon = 11;
	      days_in_feb =
		((_DAYS_IN_YEAR (tim_p->tm_year) == 366) ?
		 29 : 28);
	    }
	  tim_p->tm_mday += _DAYS_IN_MONTH (tim_p->tm_mon);
	}
    }
  else
    {
      while (tim_p->tm_mday > _DAYS_IN_MONTH (tim_p->tm_mon))
	{
	  tim_p->tm_mday -= _DAYS_IN_MONTH (tim_p->tm_mon);
	  if (++tim_p->tm_mon == 12)
	    {
	      tim_p->tm_year++;
	      tim_p->tm_mon = 0;
	      days_in_feb =
		((_DAYS_IN_YEAR (tim_p->tm_year) == 366) ?
		 29 : 28);
	    }
	}
    }
}

time_t mktime (struct tm *tim_p) // from newlib
{
  time_t tim = 0;
  long days = 0;
  int year;

  /* validate structure */
  validate_structure (tim_p);

  /* compute hours, minutes, seconds */
  tim += tim_p->tm_sec + (tim_p->tm_min * _SEC_IN_MINUTE) +
    (tim_p->tm_hour * _SEC_IN_HOUR);

  /* compute days in year */
  days += tim_p->tm_mday - 1;
  days += _DAYS_BEFORE_MONTH[tim_p->tm_mon];
  if (tim_p->tm_mon > 1 && _DAYS_IN_YEAR (tim_p->tm_year) == 366)
    days++;

  /* compute day of the year */
  tim_p->tm_yday = days;

  if (tim_p->tm_year > 10000
      || tim_p->tm_year < -10000)
    {
      return (time_t) -1;
    }

  /* compute days in other years */
  if (tim_p->tm_year > 70)
    {
      for (year = 70; year < tim_p->tm_year; year++)
	days += _DAYS_IN_YEAR (year);
    }
  else if (tim_p->tm_year < 70)
    {
      for (year = 69; year > tim_p->tm_year; year--)
	days -= _DAYS_IN_YEAR (year);
      days -= _DAYS_IN_YEAR (year);
    }

  /* compute day of the week */
  if ((tim_p->tm_wday = (days + 4) % 7) < 0)
    tim_p->tm_wday += 7;

  /* compute total seconds */
  tim += (days * _SEC_IN_DAY);

  return tim;
}

static inline int STRtoWSTR(
	LPCSTR pMultiByteStr,
	LPWSTR lpWideCharStr,
	int cchWideChar
	)
{
	return MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pMultiByteStr,-1,lpWideCharStr,cchWideChar);
}

#ifdef ARMV4
WINBASEAPI DWORD WINAPI FormatMessageA(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments)
{
	int sLen = STRtoWSTR(lpBuffer, NULL, 0);
	LPWSTR lpBufferW = alloca(sizeof(wchar_t)*(sLen+1));
	if(lpBufferW)
		STRtoWSTR(lpBuffer, lpBufferW, sLen);
	else
		lpBufferW = (LPWSTR)lpBuffer; //heh, backup
	return FormatMessageW(dwFlags,lpSource,dwMessageId,dwLanguageId,lpBufferW,nSize,Arguments);
}

WINBASEAPI BOOL WINAPI DeleteFileA(
    LPCSTR lpFileName)
{
	int sLen = STRtoWSTR(lpFileName, NULL, 0);
	LPWSTR lpFileNameW = alloca(sizeof(wchar_t)*(sLen+1));
	if(lpFileNameW)
		STRtoWSTR(lpFileName, lpFileNameW, sLen);
	else
		lpFileNameW = (LPWSTR)lpFileName; //heh, backup
	return DeleteFileW(lpFileNameW);
}

WINBASEAPI HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
	int sLen = STRtoWSTR(lpFileName, NULL, 0);
	LPWSTR lpFileNameW = alloca(sizeof(wchar_t)*(sLen+1));
	if(lpFileNameW)
		STRtoWSTR(lpFileName, lpFileNameW, sLen);
	else
		lpFileNameW = (LPWSTR)lpFileName; //heh, backup
	return CreateFileW(lpFileNameW,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);
}

WINBASEAPI BOOL WINAPI CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	int sLen = STRtoWSTR(lpPathName, NULL, 0);
	LPWSTR lpPathNameW = alloca(sizeof(wchar_t)*(sLen+1));
	if(lpPathNameW)
		STRtoWSTR(lpPathName, lpPathNameW, sLen);
	else
		lpPathNameW = (LPWSTR)lpPathName; //heh, backup
	return CreateDirectoryW(lpPathNameW,lpSecurityAttributes);
}
#endif

int WINAPI MessageBoxA(
    HWND hWnd ,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType)
{
	int sLen1 = STRtoWSTR(lpText, NULL, 0);
	LPWSTR lpTextW = alloca(sizeof(wchar_t)*(sLen1+1));
	int sLen2 = STRtoWSTR(lpCaption, NULL, 0);
	LPWSTR lpCaptionW = alloca(sizeof(wchar_t)*(sLen2+1));
	if(lpTextW)
		STRtoWSTR(lpText, lpTextW, sLen1);
	else
		lpTextW = (LPWSTR)lpText; //heh, backup
	if(lpCaptionW)
		STRtoWSTR(lpCaption, lpCaptionW, sLen2);
	else
		lpCaptionW = (LPWSTR)lpCaption; //heh, backup
	return MessageBoxW(hWnd,lpTextW,lpCaptionW,uType);
}
