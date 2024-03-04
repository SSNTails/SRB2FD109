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
/// \brief Fixed point arithmetics implementation
/// 
///	Fixed point, 32bit as 16.16.

#ifndef __M_FIXED__
#define __M_FIXED__

#include "doomtype.h"
#include <math.h>
#ifdef __GNUC__
#include <stdlib.h>
#endif

#ifdef _WIN32_WCE
#include "sdl/SRB2CE/cehelp.h"
#endif

/*!
  \brief bits of the fraction
*/
#define FRACBITS 16
/*!
  \brief units of the fraction
*/
#define FRACUNIT (1<<FRACBITS)
/**	\brief	Redefinition of int as fixed_t
	unit used as fixed_t
*/

#if defined(_MSC_VER)
typedef __int32 fixed_t;
#else
typedef int fixed_t;
#endif

/*!
  \brief convert fixed_t into floating number
*/
#define FIXED_TO_FLOAT(x) (((float)(x)) / 65536.0f)


/**	\brief	The TMulScale16 function

	\param	a	a parameter of type fixed_t
	\param	b	a parameter of type fixed_t
	\param	c	a parameter of type fixed_t
	\param	d	a parameter of type fixed_t
	\param	e	a parameter of type fixed_t
	\param	f	a parameter of type fixed_t

	\return	fixed_t

	
*/
FUNCINLINE static ATTRINLINE fixed_t TMulScale16(fixed_t a, fixed_t b, fixed_t c, fixed_t d, fixed_t e, fixed_t f) \
{ \
	return (fixed_t)((((INT64)a * (INT64)b) + ((INT64)c * (INT64)d) \
		+ ((INT64)e * (INT64)f)) >> 16); \
}

/**	\brief	The DMulScale16 function

	\param	a	a parameter of type fixed_t
	\param	b	a parameter of type fixed_t
	\param	c	a parameter of type fixed_t
	\param	d	a parameter of type fixed_t

	\return	fixed_t

	
*/
FUNCINLINE static ATTRINLINE fixed_t DMulScale16(fixed_t a, fixed_t b, fixed_t c, fixed_t d) \
{ \
	return (fixed_t)((((INT64)a * (INT64)b) + ((INT64)c * (INT64)d)) >> 16); \
}

#ifndef USEASM
	fixed_t FixedMul(fixed_t a, fixed_t b);
	fixed_t FixedDiv2(fixed_t a, fixed_t b);
#elif defined(__WATCOMC__)
	#pragma aux FixedMul =  \
		"imul ebx",         \
		"shrd eax,edx,16"   \
		parm    [eax] [ebx] \
		value   [eax]       \
		modify exact [eax edx]

	#pragma aux FixedDiv2 = \
		"cdq",              \
		"shld edx,eax,16",  \
		"sal eax,16",       \
		"idiv ebx"          \
		parm    [eax] [ebx] \
		value   [eax]       \
		modify exact [eax edx]
#elif defined(__GNUC__) && defined(__i386__)
	// DJGPP, i386 linux, cygwin or mingw
	static inline fixed_t FixedMul (fixed_t a, fixed_t b) // asm
	{
		fixed_t ret;
		int dummy;
		asm
		(
			  "  imull %3 ;"
			  "  shrdl $16,%1,%0 ;"
			: "=a" (ret),          /* eax is always the result */
			  "=d" (dummy)         /* fix compile problem with gcc-2.95.1
			                          edx is clobbered, but it might be an input */
			: "0" (a),             /* eax is also first operand */
			  "r" (b)              /* second operand could be mem or reg before,
			                          but gcc compile problems mean i can only use reg */
			: "%cc"                /* edx and condition codes clobbered */
		);
		return ret;
	}

	static inline fixed_t FixedDiv2(fixed_t a, fixed_t b)
	{
		fixed_t ret;
		asm
		(
			  "movl  %%eax,%%edx      \n" // these two instructions allow the next
			  "sarl  $31,%%edx        \n" // two to pair, on the Pentium processor.
			  "shldl $16,%%eax,%%edx  \n"
			  "sall  $16,%%eax        \n"
			  "idivl %%ecx            \n"

			: "=a" (ret)
			: "a" (a), "c" (b)
			: "dx"
		);
		return ret;
	}
#elif defined(__GNUC__)
#error "turn off USEASM, it's only for i386, use NOASM=1 with the makefile"
#elif defined(_MSC_VER)
	// Microsoft Visual C++ (no asm inline)
	fixed_t __cdecl FixedMul(fixed_t a, fixed_t b);
	fixed_t __cdecl FixedDiv2(fixed_t a, fixed_t b);
#else
"No USEASM for FixedMul and FixedDiv2?"
#endif // useasm

/**	\brief	The FixedDiv function

	\param	a	fixed_t number
	\param	b	fixed_t number

	\return	a/b

	
*/
FUNCINLINE static ATTRINLINE fixed_t FixedDiv(fixed_t a, fixed_t b)
{
	if((abs(a) >> 14) >= abs(b))
		return (a^b) < 0 ? MININT : MAXINT;

	return FixedDiv2(a, b);
}

#endif //m_fixed.h
