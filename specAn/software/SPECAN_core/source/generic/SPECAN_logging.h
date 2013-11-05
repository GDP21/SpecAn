/*!****************************************************************************
 @File          SPECAN_logging.h

 @Title         Logging for the spectrum analyser

 @Date          11 January 2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 ******************************************************************************/

#ifndef SPECAN_LOGGING_H_
#define SPECAN_LOGGING_H_

#include "log.h"


/* Log module context.  Make this global so it is easier for the download script to locate it.  Here we assume
we are the only thing in the system.  In a multi-context system the logging should really be dealt with centrally,
preferably as part of the UCC framework. */
extern LOG_CONTEXT_T GLB_eventLogCtx;

/* Enumerate logging categories.  Each category can be switched on or off as a group */
typedef enum
{
	/* Control state machine */
	SPECAN_LOG_CATEGORY_CTRL_STATES = 0,
	/* API state machine */
	//SPECAN_LOG_CATEGORY_API_STATES,
	/* Performance measurements */
	SPECAN_LOG_CATEGORY_PERFORMANCE,
	/* AGC sub-system */
	SPECAN_LOG_CATEGORY_AGC,
	/* Miscellaneous */
	//SPECAN_LOG_CATEGORY_MISC,
	/* Number of categories (keep this last) */
	SPECAN_NUM_LOG_CATEGORIES
} SPECAN_LOG_CATEGORY_T;


/* Enumerate all log events in all categories */
typedef enum
{
	/* Events in the SPECAN_LOG_CATEGORY_CTRL_STATES category */
	SPECAN_LOGEVENT_CTRLSTATE_IDLE = 0,
	SPECAN_LOGEVENT_CTRLSTATE_START,
	SPECAN_LOGEVENT_CTRLSTATE_TUNE,
	SPECAN_LOGEVENT_CTRLSTATE_AGC_UPDATE,
	SPECAN_LOGEVENT_CTRLSTATE_PROC_FRAGMENT,
	SPECAN_LOGEVENT_CTRLSTATE_PROC_SPECTRUM,

	/* Messages received by the control FSM: */
	SPECAN_LOGEVENT_FSM_MSG_RUN_SCAN,
	SPECAN_LOGEVENT_FSM_MSG_STOP_SCAN,
	SPECAN_LOGEVENT_FSM_MSG_CHANGE_STATE,
	SPECAN_LOGEVENT_FSM_MSG_TUNE_EVENT,

	/* Events in the SPECAN_LOG_CATEGORY_API_STATES category */

	/* Events in the SPECAN_LOG_CATEGORY_PERFORMANCE category */
//	SPECAN_LOGEVENT_FRONT_END_AGC,

	/* Events in the SPECAN_LOG_CATEGORY_MISC category */

	/* Events in the SPECAN_LOG_CATEGORY_AGC category */
	SPECAN_LOGVAL_FRONT_END_GAIN,

	/* Number of events (keep this last) */
	SPECAN_NUM_LOG_EVENTS
} SPECAN_LOG_EVENT_T;

/* Logging context structure */
typedef struct
{
	/* Log objects for each category */
	LOG_CATEGORY_OBJ_T 	logCat[SPECAN_NUM_LOG_CATEGORIES];
	/* Log objects for each event */
	LOG_EVENT_OBJ_T 	logObj[SPECAN_NUM_LOG_EVENTS];

} SPECAN_LOG_CTX;

/* Initialise our logging system */
void SPECAN_initLogging(SPECAN_LOG_CTX *logCtx);

/* convenience macro for readability */
#define SPECAN_LOG_EVENT(cat, obj)      LOG_event(&SA_ctx->logCtx.logCat[cat], &SA_ctx->logCtx.logObj[obj])
#define SPECAN_LOG_VALUE(cat, obj, val) LOG_value(&SA_ctx->logCtx.logCat[cat], &SA_ctx->logCtx.logObj[obj], val)

#endif /* SPECAN_LOGGING_H_ */
