/*!****************************************************************************
 @File          SPECAN_logging.c

 @Title

 @Date          11 January 2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "SPECAN_logging.h"

/* Log module context.  Make this global so it is easier for the download script to locate it.  Here we assume
we are the only thing in the system.  In a multi-context system the logging should really be dealt with centrally,
preferably as part of the UCC framework. */
LOG_CONTEXT_T GLB_eventLogCtx;

#define TIMER_FREQUENCY  (1000000) /* Timer is set for 1MHz frequency  */
#define MAX_TIME_STAMP   (4000000)

/* Size of log space to be allocated, in ints */
#define SIZE_OF_LOG_INTS      				(6000)
/* Part of the log space is used for storage of strings to be associated with log entries */
#define LOG_STRING_PARTITION_SIZE_BYTES		(3000)


/* Logging space, located in the separate application logBuffer so it can be relocated */
unsigned int GLB_logRecordArray[SIZE_OF_LOG_INTS];


void SPECAN_initLogging(SPECAN_LOG_CTX *logCtx)
{
	/* Initialise the logging module */
    LOG_init(&GLB_eventLogCtx,
    		GLB_logRecordArray,
    		SIZE_OF_LOG_INTS * sizeof(unsigned int),
    		LOG_STRING_PARTITION_SIZE_BYTES,
            TIMER_FREQUENCY,
            MAX_TIME_STAMP);
    /* Switch logging module on */
    LOG_on();
    /* Enable wrapping */
    //LOG_wrapOn(&GLB_eventLogCtx);

    /* Initialise each category.  Edit to turn different categories on and off */
    LOG_categoryInit(&logCtx->logCat[SPECAN_LOG_CATEGORY_CTRL_STATES], "Control states", LOG_SWITCH_ON);
//    LOG_categoryInit(&logCtx->logCat[SPECAN_LOG_CATEGORY_API_STATES], "API states", LOG_SWITCH_OFF);
    LOG_categoryInit(&logCtx->logCat[SPECAN_LOG_CATEGORY_PERFORMANCE], "Performance", LOG_SWITCH_OFF);
    LOG_categoryInit(&logCtx->logCat[SPECAN_LOG_CATEGORY_AGC], "AGC", LOG_SWITCH_ON);
//    LOG_categoryInit(&logCtx->logCat[SPECAN_LOG_CATEGORY_MISC], "Miscellaneous logged items", LOG_SWITCH_OFF);

    /* Initialise each event.  Edit to turn different events on and off */
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_CTRLSTATE_IDLE], "IDLE", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_CTRLSTATE_START], "START", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_CTRLSTATE_TUNE], "TUNE", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_CTRLSTATE_AGC_UPDATE], "AGC_UPDATE", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_CTRLSTATE_PROC_FRAGMENT], "PROC_FRAGMENT", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_CTRLSTATE_PROC_SPECTRUM], "PROC_COMPLETE", LOG_SWITCH_ON);

    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_FSM_MSG_RUN_SCAN], "FSM received RUN_SCAN", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_FSM_MSG_STOP_SCAN], "FSM received STOP_SCAN", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_FSM_MSG_CHANGE_STATE], "FSM received CHANGE_STATE", LOG_SWITCH_ON);
    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_FSM_MSG_TUNE_EVENT], "FSM received TUNE_EVENT", LOG_SWITCH_ON);

//    LOG_eventInit(&logCtx->logObj[SPECAN_LOGEVENT_FRONT_END_AGC], "Front-end AGC task", LOG_SWITCH_ON);

    LOG_eventInit(&logCtx->logObj[SPECAN_LOGVAL_FRONT_END_GAIN], "Front end gain Q15.16", LOG_SWITCH_ON);
}
