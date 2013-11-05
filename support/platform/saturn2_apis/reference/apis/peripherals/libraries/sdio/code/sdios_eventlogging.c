/*
    SDIOS implementation
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <string.h>
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>

#define SDIOS_TIMER_0_ADDR     ((volatile unsigned int *) 0x04800020)
volatile unsigned int *       pTimer0 = SDIOS_TIMER_0_ADDR;

#define SDIOS_EVENTSIZE             4                                       /* entries per event */
#define SDIOS_EVENTS                4000                                   /* events per buffer */
#define SDIOS_EVENTLOGSIZE          (SDIOS_EVENTS * SDIOS_EVENTSIZE)        /* buffer size */
#define SDIOS_EVENTLOGCIRCULAR      1                                       /* 0 = not circular, 1 = circular buffer */
#define SDIOS_EVENTLOGSAFE          1                                       /* 0 = ints untouched, 1 = ints disabled */

unsigned int    SDIOS_eventlogindex = 0;
unsigned int    SDIOS_eventlogindex_loop = 1;
unsigned int    SDIOS_event_log[SDIOS_EVENTLOGSIZE];
unsigned int    SDIOS_eventlogstart = 1;
unsigned int    SDIOS_eventloglastlogtime = 0;

#if SDIOS_EVENTLOGCIRCULAR
volatile unsigned int       SDIOS_eventlogcircular = 1;
#else
volatile unsigned int       SDIOS_eventlogcircular = 0;
#endif

#if SDIOS_EVENTLOGSAFE
volatile unsigned int       SDIOS_eventlogsafe = 1;
#else
volatile unsigned int       SDIOS_eventlogsafe = 0;
#endif

KRN_IPL_T           oldipl;

void Add_SDIOS_EventLog( unsigned int e, unsigned int d0, unsigned int d1 )
{
    if ( SDIOS_eventlogstart )
    {
        if ( SDIOS_eventlogsafe )
        {
            oldipl = KRN_raiseIPL();
        }

        if ( SDIOS_eventlogindex == 0 )
        {
            /*
                First time in - clear event log buffer to 0xFFFFFFFF
            */
            memset( SDIOS_event_log, 0xFF, (SDIOS_EVENTLOGSIZE * sizeof(unsigned int)) );
        }

        SDIOS_eventloglastlogtime              = *pTimer0;

        if ( SDIOS_eventlogindex < (SDIOS_EVENTLOGSIZE - (SDIOS_EVENTSIZE - 1)) )
        {
            SDIOS_event_log[SDIOS_eventlogindex++] = *pTimer0;
            SDIOS_event_log[SDIOS_eventlogindex++] = e;
            SDIOS_event_log[SDIOS_eventlogindex++] = d0;
            SDIOS_event_log[SDIOS_eventlogindex++] = d1;
        }
        else
        {
            if ( SDIOS_eventlogcircular )
            {
                SDIOS_eventlogindex = 0;
                SDIOS_eventlogindex_loop++;
                SDIOS_event_log[SDIOS_eventlogindex++] = *pTimer0;
                SDIOS_event_log[SDIOS_eventlogindex++] = e;
                SDIOS_event_log[SDIOS_eventlogindex++] = d0;
                SDIOS_event_log[SDIOS_eventlogindex++] = d1;
            }
        }

        if ( SDIOS_eventlogsafe )
        {
            KRN_restoreIPL(oldipl);
        }
    }
}
