/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <memtest@discworld.dyndns.org>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004 Charles Cazabon <memtest@discworld.dyndns.org>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains typedefs and structure definitions.
 *
 ******************************************************************************
 *
 * Modified for MTX testing 
 * Tests can be built to allow testing of 32-bit or 64-bit accesses
 * (define MEMTEST_64BIT when compiling). Confusingly in the 64-bit case,
 * "ul" is "unsigned long long"
 *
 */
#ifndef _TYPES_H_
#define _TYPES_H_

typedef unsigned long U32;
typedef unsigned long volatile VU32;

/* Test word type */
#ifdef MEMTEST_64BIT
typedef unsigned long long TW;
typedef unsigned long long volatile TWv;
#else
typedef unsigned long TW;
typedef unsigned long volatile TWv;
#endif

/* Nasty. Used as 'ul' throughout tests.c */
typedef TW  ul;
typedef TWv ulv;


struct test
{
	char *name;
	int (*fp)();
};

#endif

/* End of types.h */
