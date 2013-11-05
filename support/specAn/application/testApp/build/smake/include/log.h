/*!
 * \file    log.h
 *
 * \brief   This module implements generic event logging functionality.
 *
 *
 */
#ifndef LOG_H
#define LOG_H

#ifndef NULL
#define NULL (void *)0x0 /*!< Define NULL */
#endif

#ifndef TRUE
#define TRUE 1  /*!< Define TRUE */
#endif

#ifndef FALSE
#define FALSE 0 /*!< Define FALSE */
#endif

    
/* Macro definitions */
	
#define  LOG_MAX_EVENT_ID    (262144) /*!< Maximum event Id. */
#define  LOG_MAX_CATEGORY_ID (1024)   /*!< Maximum category Id. */
#define  LOG_MAX_STRING_SIZE (2048)   /*!< Maximum string size. */

/* Type definitions */
	
/*! \brief Return code enumeration. */
typedef enum {
    LOG_RTN_STRING_BUFFER_FULL = -3,   /*!< String buffer is full. */
    LOG_RTN_EVENT_BUFFER_FULL  = -2,   /*!< Event buffer is full. */
    LOG_RTN_FAIL               = -1,   /*!< Fail. */
    LOG_RTN_SUCCESS            = 0     /*!< Success. */
}LOG_RTN_E ;

/*! \brief Switch on/off enumeration. */
typedef enum {
    LOG_SWITCH_OFF = 0, /*!< Logging disabled. */
    LOG_SWITCH_ON  = 1  /*!< Logging enabled. */
}LOG_SWITCH_E ;

/*! \brief Wrap on/off enumeration. */
typedef enum {
    LOG_WRAP_OFF = 0, 			/*!< Wrapping disabled. */
    LOG_WRAP_ON  = 1,  			/*!< Wrapping enabled. */
    LOG_WRAP_CONSUMED_ONLY = 2, /*!< Wrap, but only overwrite consumed data. */
}LOG_WRAP_E ;

/*! \brief Initialised enumeration. */
typedef enum {
    LOG_INIT_YES = 0xC90FDAB2,  /*!< Initialised. */
    LOG_INIT_NO  = 0xCE0EDEBE   /*!< Un-initialised. */
}LOG_INIT_E ; 

/*! \brief Log record type enumeration. */
typedef enum {
    LOG_TYPE_UNUSED = 0,        /*!< Unused. */
    LOG_TYPE_EVENT,             /*!< Event. */
    LOG_TYPE_EVENT_STRING,      /*!< Event string. */
    LOG_TYPE_CATEGORY_STRING,   /*!< Category string. */
    LOG_TYPE_RESOLUTION,        /*!< Resolution. */
    LOG_TYPE_VALUE,             /*!< Value. */
    LOG_TYPE_PERFORMANCE_START, /*!< Performance start. */
    LOG_TYPE_PERFORMANCE_STOP,  /*!< Performance stop. */
    LOG_TYPE_MAX,               /*!< Maximum type. */
} LOG_TYPE_E ; 

/*! \brief Log management structure. */
typedef struct {
    LOG_INIT_E    eInit;             /*!< Initialisation flag. */
    const char   *pStatus;           /*!< Status. */
    LOG_WRAP_E    eWrap;             /*!< Wrapping on/off. */
    unsigned int *pBase;             /*!< Base address of log. */
    unsigned int  sizeToSave;        /*!< Size of log to save (bytes). */
	unsigned int  sizeMax;			 /*!< Size of log to save maximum (bytes). */
    unsigned int *pEvent;            /*!< Event partition pointer. */
    unsigned int *pEventStart;		 /*!< Pointer to start of event partition. */
    unsigned int *pEventEnd;		 /*!< Pointer to end of event partition. */
    unsigned int  *pConsumed;		 /*!< Pointer to end of consumed data. */
    unsigned int *pString;           /*!< String partition pointer. */
    unsigned int  eventSizeMax;      /*!< Size of event partition (bytes). */
    unsigned int  eventSizeMaxWords; /*!< Size of event partition (words) */
    unsigned int  eventSizeFree;     /*!< Size of event partition free (bytes). */
    unsigned int  stringSizeMax;     /*!< Size of string partition (bytes). */
    unsigned int  stringSizeFree;    /*!< Size of string partition free (bytes). */
    unsigned int  recordCount;       /*!< Log record count. */
    unsigned int  categoryCount;     /*!< Category ID generator. */
    unsigned int  eventCount;        /*!< Event ID generator. */
    unsigned int  wrapCount;         /*!< Number of times buffer has wrapped. */
    unsigned int  consumedWrapCount; /*!< The number of times buffer had wrapped
         	 	    	 	 	 	 	  last time data was consumed. */
    unsigned int  clockFreq;         /*!< Clock frequency. */
    unsigned int  previousStamp;     /*!< Previous time stamp. */
    unsigned int  maxTimeStamp;      /*!< Maximum possible value of time stamp. */
    unsigned int  notifyThreshold;	 /*!< The notification callback is called when
     	 	 	 	 	 	 	 	 	  this many words of data are ready to be
     	 	 	 	 	 	 	 	 	  consumed. */
    unsigned int  haveNotified; 	 /*!< Flag indicating that notification callback
     	 	 	 	 	 	 	 	   has been called.*/
    void (*notifyCallback)(void*);   /*!< To be called when notifyThreshold words of
     	 	 	 	 	 	 	 	 	  data are ready to be consumed. */
    void (*overflowCallback)(void*); /*!< To be called when unconsumed data is
    									  overwritten. */

} LOG_CONTEXT_T ;

/*! \brief Log 'event' object. */
typedef struct {
    LOG_INIT_E    eInit;    /*!< Initialisation flag. */
    unsigned int  id;       /*!< Event ID. */
    LOG_SWITCH_E  eOnOff;   /*!< On/Off flag. */
    const char   *pTag;     /*!< Tag. */
} LOG_EVENT_OBJ_T ;

/*! \brief Log 'category' object. */
typedef struct {
    LOG_INIT_E    eInit;    /*!< Initialisation flag. */
    unsigned int  id;       /*!< Category ID. */
    LOG_SWITCH_E  eOnOff;   /*!< On/Off flag. */
    const char   *pTag;     /*!< Tag. */
} LOG_CATEGORY_OBJ_T;

/* Function Prototypes */
#ifdef __STDC__


/*!
 * <b>Description:</b>\n
 * 
 * This function initialises the log context.
 * 
 * \param[in,out] pContext             Event log context pointer.
 * \param[in]     pLogBase             Base of log buffer.
 * \param[in]     sizeMax              Size of buffer in bytes.
 * \param[in]     stringPartitionSize  Number of bytes to reserve for strings.
 * \param[in]     clockFreq            Clock frequency (Hz).
 * \param[in]     maxTimeStamp         Maximum possible value of time stamp. 
 *
 */
void LOG_init (
    LOG_CONTEXT_T *pContext, 
    unsigned int  *pLogBase, 
    unsigned int   sizeMax, 
    unsigned int   stringPartitionSize, 
    unsigned int   clockFreq, 
    unsigned int   maxTimeStamp
);

/*!
 * <b>Description:</b>\n
 * 
 * This function initialises a category object.
 * 
 * \param[in,out] pCategoryObj      Pointer to category object.
 * \param[in]     pCategoryString   String associated with this category.
 * \param[in]     eOnOff            Default on/off state of category.
 *
 */
void LOG_categoryInit (
    LOG_CATEGORY_OBJ_T *pCategoryObj, 
    const char         *pCategoryString, 
    LOG_SWITCH_E        eOnOff
);
    
/*!
 * <b>Description:</b>\n
 * 
 * This function initialises an event object.
 * 
 * \param[in,out] pEventObj         Pointer to event object.
 * \param[in]     pEventString      String associated with this event.
 * \param[in]     eOnOff            Default on/off state of event.
 *
 */
void LOG_eventInit (
    LOG_EVENT_OBJ_T *pEventObj, 
    const char      *pEventString, 
    LOG_SWITCH_E     eOnOff
);

/*!
 * <b>Description:</b>\n
 *
 * This function registers a callback which will be called when a certain
 * amount of log data is ready to be consumed. The callback will only be called
 * once until some data is marked as consumed (see LOG_markConsumed).
 *
 * \param[in]     callback      The function to be called. It will be passed the current log context.
 * \param[in]     threshold     The number of unconsumed words that will trigger a notification.
 *
 */
void LOG_addNotificationCallback(void (*callback)(void*), unsigned int threshold);

/*!
 * <b>Description:</b>\n
 *
 * This function registers a callback which will be called when unconsumed
 * log data is overwritten. The callback will be called each time new data
 * is overwritten.
 *
 * \param[in]     callback      The function to be called. It will be passed the current log context.
 *
 */
void LOG_addOverflowCallback(void (*callback)(void*));

/*!
 * <b>Description:</b>\n
 *
 * This function marks the log data up to the point indicated as having been consumed.
 *
 * \param[in]     pConsumed      Pointer to the end of the consumed data.
 *
 */
void LOG_markConsumed(unsigned int *pConsumed);

/*!
 * <b>Description:</b>\n
 * 
 * Switch logging ON.
 *
 */
void LOG_on ();

/*!
 * <b>Description:</b>\n
 * 
 * Switch logging OFF.
 *
 */
void LOG_off ();

/*!
 * <b>Description:</b>\n
 * 
 * Enable wrapping.
 *
 */
void LOG_wrapOn (
    LOG_CONTEXT_T *pContext
);

/*!
 * <b>Description:</b>\n
 * 
 * Disable wrapping.
 *
 */
void LOG_wrapOff (
    LOG_CONTEXT_T *pContext
);

/*!
 * <b>Description:</b>\n
 * 
 * Enable wrapping over consumed data only.
 *
 */
void LOG_wrapConsumedOnly (
    LOG_CONTEXT_T *pContext
);

/*!
 * <b>Description:</b>\n
 *
 * Enable this event.
 *
 * \param[in,out] pEventObj         Pointer to event object.
 * 
 */
void LOG_eventOn (
    LOG_EVENT_OBJ_T *pEventObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Disable this event.
 *
 * \param[in,out] pEventObj         Pointer to event object.
 * 
 */
void LOG_eventOff (
    LOG_EVENT_OBJ_T *pEventObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Enable this category.
 *
 * \param[in,out] pCategoryObj      Pointer to event object.
 * 
 */
void LOG_categoryOn (
    LOG_CATEGORY_OBJ_T *pCategoryObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Disable this category.
 *
 * \param[in,out] pCategoryObj      Pointer to event object.
 * 
 */
void LOG_categoryOff (
    LOG_CATEGORY_OBJ_T *pCategoryObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Write an event record to the event log.
 *
 * \param[in] pCategoryObj          Pointer to category object.
 * \param[in] pEventObj             Pointer to event object.
 * 
 */
void LOG_event (
    LOG_CATEGORY_OBJ_T *pCategoryObj, 
    LOG_EVENT_OBJ_T    *pEventObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Write a value record to the event log.
 *
 * \param[in] pCategoryObj          Pointer to category object.
 * \param[in] pEventObj             Pointer to event object.
 * \param[in] value                 32 bit value.
 * 
 */
void LOG_value (
    LOG_CATEGORY_OBJ_T *pCategoryObj, 
    LOG_EVENT_OBJ_T    *pEventObj,
    int                 value
);

/*!
 * <b>Description:</b>\n
 * 
 * Write a performance start record to the event log.
 *
 * \param[in] pCategoryObj          Pointer to category object.
 * \param[in] pEventObj             Pointer to event object.
 * 
 */
void LOG_performanceStart (
    LOG_CATEGORY_OBJ_T *pCategoryObj, 
    LOG_EVENT_OBJ_T    *pEventObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Write a performance stop record to the event log.
 *
 * \param[in] pCategoryObj          Pointer to category object.
 * \param[in] pEventObj             Pointer to event object.
 * 
 */
void LOG_performanceStop (
    LOG_CATEGORY_OBJ_T *pCategoryObj, 
    LOG_EVENT_OBJ_T    *pEventObj
);

/*!
 * <b>Description:</b>\n
 * 
 * Dump the entire contents of the log to a binary file.
 * For use in regression tests.
 *
 * \param[in] pContext         Event log context pointer.
 * \param[in] fileName         Name of binary file to dump to.
 * 
 */
LOG_RTN_E LOG_dumpBinaryLog (
    LOG_CONTEXT_T *pContext,
	char *fileName	
);
		
#endif /* __STDC__ */



#endif /* #ifndef LOG_H */

