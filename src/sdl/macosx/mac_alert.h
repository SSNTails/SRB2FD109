// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: mac_alert.h,v 1.1 2004/03/23 17:38:03 araftis Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
//
// $Log: mac_alert.h,v $
// Revision 1.1  2004/03/23 17:38:03  araftis
//
// Added support for Mac OS X (version 10.3+). Also fixed a number of endian
// bugs that were introduced by the over zealous use of the SHORT macro. This
// version also removes support for Mac OS 9 and earlier.
//-----------------------------------------------------------------------------
/// \file
/// \brief Graphical Alerts for MacOSX
///
///	Shows alerts, since we can't just print these to the screen when
///	launched graphically on a mac.

#ifdef __APPLE_CC__

extern int MacShowAlert(const char *title, const char *message, const char *button1, const char *button2, const char *button3);

#endif
