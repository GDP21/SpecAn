/*
 *
 *	This version of the memory test is intended to be run on either of teh two MTXs in Comet.
 *	The total size is (or was!) less than 16kbytes so it is suggested that two variants are produced
 *	linked at the bottom and top of MTX core memory (Addresses 0x80900000/0x82880000 and 0x80998000/0x828CC00
 *	respectively
 */

#define __version__ "4.0.4"

/******************************** Includes ***********************************/

#include <machine.inc>
#include <metagtbi.h>

#include <stdlib.h>
#include "types.h"
#include "sizes.h"
#include "tests.h"

/******************************** Defines ************************************/

/*
 * Added default values for MEMTEST_xxx parameters.
 *
 * The following values test completely a 976K block of GRAMmemory based
 * at 0xB7000000
 *
 * To test an entire area, simply choose values such that
 * CHUNKS * BYTES == RANGE
 */
#ifndef MEMTEST_START
#define MEMTEST_START	0xB7000000
#endif

#ifndef MEMTEST_RANGE
#define MEMTEST_RANGE	0x000F4000
#endif

#ifndef MEMTEST_CHUNKS
#define MEMTEST_CHUNKS	1
#endif

#ifndef MEMTEST_BYTES
#define MEMTEST_BYTES	0x000F4000
#endif

/* Time in seconds to wait in memory retention test */
#ifndef MEMTEST_WAITTIME
#define MEMTEST_WAITTIME	0
#endif

/* 0 = run forever, otherwise loop count */
#ifndef MEMTEST_REPEAT
#define MEMTEST_REPEAT		1
#endif

#define ErrorMsg(...)       __TBILogF(__VA_ARGS__)
#define ProgressMsg(...)    __TBILogF(__VA_ARGS__)

//#define NOISY
#ifdef NOISY
  #define InfoMsg(...)      __TBILogF(__VA_ARGS__)
#else
  #define InfoMsg(...)
#endif

/* Return codes */
#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04
#define EXIT_FAIL_RETENTIONTEST 0x08

/******************************** Global Variables ***************************/

struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
    { NULL, NULL }
};

/* Globals set up by init, and used to drive the test. Made global so that they
   can be patched in out.txt. */
void         *TPMemStart;
unsigned long TPMemRange;
unsigned long TPNumChunks;
unsigned long TPChunkBytes;
unsigned long TPWaitTime;
unsigned long TPRepeat;

/******************************** Local Functions ****************************/

static void fill_random ( void *mem, unsigned long bytes, int *pSeed );
static int  check_random( void *mem, unsigned long bytes, int *pSeed );

/******************************** Code ***************************************/

int memtest(void)
{
	unsigned long  loop, i, j, k, chunkwords, chunkoffs;
    TWv            *chunks[64];
    int            seed, r, exit_code = 0;
#ifndef SIM_FACTOR
	unsigned       divider;
	unsigned long long waitticks, endtime;
#endif

//    InfoMsg("memtester version " __version__ " (%d-bit)\n", UL_LEN);
//    InfoMsg("Copyright (C) 2004 Charles Cazabon.\n");
//    InfoMsg("Licensed under the GNU General Public License version 2 (only).\n");
//    InfoMsg("\n");


    if ( TPNumChunks > (sizeof(chunks)/sizeof(chunks[0])) )
    {
        ErrorMsg("ERROR: too many chunks\n");
        abort();
    }
	if ( TPNumChunks < 2 )
	{
        ErrorMsg("ERROR: Need at least 2 MEMTEST_CHUNKS\n");
        abort();
	}
    if ( TPNumChunks * TPChunkBytes > TPMemRange
       || ((TPMemRange / TPNumChunks) * (TPNumChunks-1)) + TPChunkBytes > TPMemRange )
    {
        ErrorMsg("ERROR: Cannot fit %u (MEMTEST_CHUNKS) disjoint chunks of 0x%08x bytes "
                 "(MEMTEST_BYTES) in 0x%08x (MEMTEST_RANGE)\n", TPNumChunks, TPChunkBytes, TPMemRange);
        abort();
    }

    ProgressMsg( "TESTING %u CHUNKS OF 0x%08x BYTES WITHIN 0x%08x-0x%08x"
                 " (%u-bit accesses)\n",
                 TPNumChunks, TPChunkBytes, TPMemStart, (unsigned long) TPMemStart + TPMemRange, sizeof(TW)*8 );
    if ( TPRepeat == 0 )
        ProgressMsg( "REPEATING FOREVER\n" );
    else
        ProgressMsg( "REPEATING %u TIME(S)\n", TPRepeat );
    ProgressMsg( "\n" );

    chunkoffs   = TPMemRange / TPNumChunks;
    chunkwords  = TPChunkBytes / sizeof(TW);

    for ( i = 0 ; i < TPNumChunks ; i++ )
        chunks[i] = (TWv*) ((unsigned long) TPMemStart + i*chunkoffs);

    for ( loop = 1 ; (!TPRepeat) || loop <= TPRepeat ; loop++ )
    {
        if ( TPRepeat == 0 )
            ProgressMsg( "Loop %u\n", loop );
        else
            ProgressMsg( "Loop %u/%u\n", loop, TPRepeat );

        for ( j = 0 ; j < TPNumChunks ; j++)
        {
			ProgressMsg( "  Testing 0x%08x\n", chunks[j]);
            r = test_stuck_address(chunks[j], chunkwords);
            InfoMsg("  %-26s:  %s\n", "Stuck Address", r ? "FAILED" : "PASSED" );
            if ( r )
            {
                ErrorMsg("*** FAILED TEST %s\n", "Stuck Address" );
                exit_code |= EXIT_FAIL_ADDRESSLINES;
				abort();
            }

            for ( k = j+1 ; k < TPNumChunks ; k++ )
            {
                if ( j == k )
                    continue;
                ProgressMsg( "  Testing 0x%08x with 0x%08x\n", chunks[j], chunks[k] );

                for ( i = 0 ; ; i++ )
                {
					InfoMsg( "i=%d\n", i);
                    if ( !tests[i].name )
                        break;
                    r = tests[i].fp(chunks[j], chunks[k], chunkwords);
                    InfoMsg( "    %-24s:  %s\n", tests[i].name, r ? "FAILED" : "PASSED" );
                    if ( r )
                    {
                        ErrorMsg( "*** FAILED TEST %s\n", tests[i].name );
                        exit_code |= EXIT_FAIL_OTHERTEST;
                    }
                }
            }
       }

        /* Fill all the chunks */
        ProgressMsg("FILLING MEMORY FOR RETENTION TEST ...\n");
        seed = 0xbaadf00d;
        for ( j = 0 ; j < TPNumChunks ; j++ )
            fill_random( (void*) chunks[j], TPChunkBytes, &seed );

#ifndef SIM_FACTOR
        /* Wait some time */
        ProgressMsg("WAITING %u s ...\n", TPWaitTime);

#ifdef MTXC_ANY
		divider = 1;
#else /* !MTXC_ANY */
        divider = (TBI_GETREG(TXDIVTIME) & TXDIVTIME_DIV_BITS) >> TXDIVTIME_DIV_S;
        if (divider == 0)
	        divider = 256;
#endif /* MTXC_ANY */

        waitticks = (unsigned long long) TPWaitTime * (TXDIVTIME_BASE_HZ / divider);

		/* Use a wasteful polling loop rather than more conventional blocking
		   on a timer event. For this reason, don't set the timer negative,
		   which would generate an event */

		/* Ensure the META hardware timer is running. Under the simulator
		   it only runs if explicitly started */
		if (TBI_GETREG(TXTIMER) == 0)
			TBI_SETREG(TXTIMER, 1);

		endtime = waitticks + __TBITimeStamp( 0 );
		for (;;)
		{
			/* Avoid trigger event by winding deadline backwards
			   (but still positive) */
			if (TBI_GETREG(TXTIMER) > 0x40000000)
				__TBITimerAdd(0, -0x20000000);
			if ( (unsigned long long)__TBITimeStamp(0) >= endtime )
				break;
		}
		/* Disable background timer */
		__TBITimerCtrl( 0, 0 );
#endif

        /* Check all the chunks */
        ProgressMsg("CHECKING MEMORY ...\n");
        seed = 0xbaadf00d;
        for ( j = 0 ; j < TPNumChunks ; j++ )
        {
            if ( check_random( (void*) chunks[j], TPChunkBytes, &seed ) != 0 )
            {
                ErrorMsg( "*** FAILED TEST %s\n", "Retention" );
                exit_code |= EXIT_FAIL_RETENTIONTEST;
				abort();
            }
        }
    }
    return exit_code;
}

static void fill_random( void *mem, unsigned long bytes, int *pSeed )
{
    U32 *p = (U32*) mem;
    unsigned n  = bytes / sizeof(U32);
    int seed = *pSeed;

    while ( n-- )
    {
        *p++ = seed;
        seed = (seed * 69069) + 3;
    }
    *pSeed = seed;
}

static int check_random( void *mem, unsigned long bytes, int *pSeed )
{
    U32 *p = (U32*) mem;
    unsigned n  = bytes / sizeof(U32);
    int seed = *pSeed;
    int r = 0;

    while ( n-- )
    {
        if ( *p != (U32)seed )
        {
            ErrorMsg( "FAILURE: 0x%08x (@0x%08x) != 0x%08x\n", *p, p, seed );
            r = -1;
			return -1;
        }
        p++;
        seed = (seed * 69069) + 3;
    }
    *pSeed = seed;
    return r;
}



#define	MEMTEST_CODEGAP	(80*1024)

int memtest_harness(void)
{
	unsigned long				OtherMTXBase = 0;
	//unsigned long				MyCoreBaseInternal = 0;
	unsigned long				MyCodeBaseExternal = 0;
	unsigned long				MyCodeAddr = (unsigned long)&memtest_harness;
	int							i32Status = 0;
	int							Low = 0;

	/*
		Use memtest() to check different areas of Saturn RAM.
	*/

	// Identify if we are built as ADDR_LOW or ADDR_HIGH - UNTESTED
	if(MyCodeAddr < 0xB7014000 )
	{
		/* We are the low-memory loaded version of this memory test, so set our gram memory base to avoid the lower 80kb. */
		MyCodeBaseExternal = 0xB7014000;
		Low=1;
	}
	else
	{
		/* We are the high-memory loaded version of this memory test, so set our gram memory base to the bottom. */
		MyCodeBaseExternal = 0xB7000000;
		Low=0;
	}

	if(0)
	{
		if (!i32Status)
		{
			/* ----- Test Core Memory  ----- */
			ProgressMsg("Testing Core Memory\n");
			TPMemStart = (void *)0x80000000;
			TPMemRange = (64) * 1024;
			TPChunkBytes = (4 * 1024);
			TPNumChunks = TPMemRange / TPChunkBytes;
			TPWaitTime = MEMTEST_WAITTIME;
			TPRepeat = MEMTEST_REPEAT;

			i32Status = memtest();
		}
	}


    if (!i32Status)
    {
        /* ----- Test GRAM  ----- */
        ProgressMsg("Testing GRAM\n");
        TPMemStart = (void *)MyCodeBaseExternal;
		if(Low){TPMemRange = (976*1024) - MEMTEST_CODEGAP;}
		else{TPMemRange = MEMTEST_CODEGAP;}
        TPChunkBytes = (16 * 1024);
        TPNumChunks = TPMemRange / TPChunkBytes;
        TPWaitTime = MEMTEST_WAITTIME;
        TPRepeat = MEMTEST_REPEAT;

        i32Status = memtest();
    }

	if(Low)
	{
		if (!i32Status)
		{
			/* ----- Test DDR2  ----- */
			ProgressMsg("Testing External Memory - DDR2\n");

			TPMemStart = (void *)0xB0000000;
			TPMemRange = (64 * 1024) * 1024; // assuming 64MB PART - this may take some time!
			TPChunkBytes = (1024 * 1024);

			TPNumChunks = TPMemRange / TPChunkBytes;
			TPWaitTime = MEMTEST_WAITTIME;
			TPRepeat = MEMTEST_REPEAT;

			i32Status = memtest();
		}
	}

	return i32Status;
}

/* End of memtest.c */

// Configuration commands
