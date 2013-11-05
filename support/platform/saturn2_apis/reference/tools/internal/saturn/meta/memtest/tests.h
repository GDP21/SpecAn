/*
 * Very simple (yet, for some reason, very effective) memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <memtest@discworld.dyndns.org>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004 Charles Cazabon <memtest@discworld.dyndns.org>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the declarations for the functions for the actual tests,
 * called from the main routine in memtester.c.  See other comments in that
 * file.
 *
 */
#ifndef _TESTS_H_
#define _TESTS_H_

#include "types.h"

/* Function declaration. */

int test_stuck_address(ulv *bufa, size_t count);
int test_random_value(ulv *bufa, ulv *bufb, size_t count);
int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_div_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_or_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_and_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count);
int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count);

#endif
