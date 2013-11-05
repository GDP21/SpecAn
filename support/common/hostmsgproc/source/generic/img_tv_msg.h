/*!****************************************************************************
* @File          img_tv_msg.h
*
* @Title         IMG TV API message definitions
*
* @Date          15 April 2011
*
* @Copyright     Copyright (C) Imagination Technologies Limited
*
* @Description   Constants and types for common IMG TV API messages.
*
******************************************************************************/


#ifndef _IMG_TV_MSG_H_
#define _IMG_TV_MSG_H_
#include <stdint.h>
#include <MeOS.h>

/**
 *
 */
/* Maximum length (in bytes) of payload supported */
#define MAX_IMG_TV_MSG_DATA_LENGTH		8
/* Length of header excluding the length field itself */
#define IMG_TV_HEADER_LENGTH			20


/* For each message the required 'length' and 'datalength' */
#define IMG_TV_READY_DATA_LENGTH		4
#define IMG_TV_READY_LENGTH				(IMG_TV_HEADER_LENGTH + IMG_TV_READY_DATA_LENGTH)

#define IMG_TV_PING_DATA_LENGTH			0
#define IMG_TV_PING_LENGTH				(IMG_TV_HEADER_LENGTH + IMG_TV_PING_DATA_LENGTH)

#define IMG_TV_ACTIVATE_DATA_LENGTH		8
#define IMG_TV_ACTIVATE_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_ACTIVATE_DATA_LENGTH)

#define IMG_TV_ACTIVATED_DATA_LENGTH	8
#define IMG_TV_ACTIVATED_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_ACTIVATED_DATA_LENGTH)

#define IMG_TV_DEACTIVATE_DATA_LENGTH	0
#define IMG_TV_DEACTIVATE_LENGTH		(IMG_TV_HEADER_LENGTH + IMG_TV_DEACTIVATE_DATA_LENGTH)

#define IMG_TV_DEACTIVATED_DATA_LENGTH	0
#define IMG_TV_DEACTIVATED_LENGTH		(IMG_TV_HEADER_LENGTH + IMG_TV_DEACTIVATED_DATA_LENGTH)

#define IMG_TV_ERROR_DATA_LENGTH		0
#define IMG_TV_ERROR_LENGTH				(IMG_TV_HEADER_LENGTH + IMG_TV_ERROR_DATA_LENGTH)


#define IMG_TV_SETREG_DATA_LENGTH		8
#define IMG_TV_SETREG_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_SETREG_DATA_LENGTH)

#define IMG_TV_REGVALUE_DATA_LENGTH		8
#define IMG_TV_REGVALUE_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_REGVALUE_DATA_LENGTH)

#define IMG_TV_GETREG_DATA_LENGTH		4
#define IMG_TV_GETREG_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_GETREG_DATA_LENGTH)

#define IMG_TV_AUTO_ON_DATA_LENGTH		4
#define IMG_TV_AUTO_ON_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_AUTO_ON_DATA_LENGTH)

#define IMG_TV_AUTO_OFF_DATA_LENGTH		4
#define IMG_TV_AUTO_OFF_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_AUTO_OFF_DATA_LENGTH)


/* Maximum length (in bytes) of payload supported */
#define MAX_IMG_TV_LONG_MSG_DATA_LENGTH	        128
#define IMG_TV_READMEM_DATA_LENGTH		8
#define IMG_TV_MEM_MULTIPLE			4
#define IMG_TV_READMEM_LENGTH			(IMG_TV_HEADER_LENGTH + IMG_TV_READMEM_DATA_LENGTH)

#define IMG_TV_MEM_DATA_LENGTH( x )		((8 + x) + (IMG_TV_MEM_MULTIPLE-1)) & ~(IMG_TV_MEM_MULTIPLE-1)
#define IMG_TV_MEM_LENGTH( x )		        (IMG_TV_HEADER_LENGTH + x )

/* types of status messageId */
#define NORMAL_STATUS_MESSAGE			(0<<16)
#define AUTOMATIC_STATUS_MESSAGE		(1<<16)


/**
 *  Enumerate the messageFunctions
 */
enum
{
    /* Test messaging system. This request does nothing except solicit a status response */
    IMG_TV_PING = 0,
    /* Ready status message. Provided in response to a ping request and automatically at system startup. Also signals lost message count */
    IMG_TV_READY,
    /* Activate an instance of a TV demodulator */
    IMG_TV_ACTIVATE = 0x02,
    /* TV demodulator activation status */
    IMG_TV_ACTIVATED,
    /* Deactivate an instance of a TV demodulator */
    IMG_TV_DEACTIVATE = 0x04,
    /* TV demodulator deactivation status */
    IMG_TV_DEACTIVATED,
    /* Set a virtual register in the interface of a TV demodulator instance */
    IMG_TV_SETREG = 0x06,
    /* Register value */
    IMG_TV_REGVALUE,
    /* Request the value of a virtual register in the interface of a TV demodulator instance */
    IMG_TV_GETREG = 0x08,
    /* Memory block */
    IMG_TV_MEM = 0x09,
    /* Activate automatic transmission of updates to a virtual register in the interface of a TV demodulator instance */
    IMG_TV_AUTO_ON = 0x0A,
    /* Deactivate automatic transmission of updates to a virtual register in the interface of a TV demodulator instance */
    IMG_TV_AUTO_OFF = 0x0C,
    /* Read a block of data from UCCP memory */
    IMG_TV_READMEM = 0x0E,
    /* Error status message. This is transmitted in response to an invalid request */
    IMG_TV_ERROR = 0xFFFFFFFF
};


/**
 *  Data structure for the common TV API messages.
 */
typedef struct
{
	/* Total length of the message in bytes, excluding the 4 bytes of the length field itself */
	uint32_t length;
	/* Identity of the software entity sending the message sender */
	uint32_t sourceId;
	/* Identity of the software object to which the message should be delivered */
	uint32_t targetId;
	/* Host define Id for message, also indicates Automatic status message */
	uint32_t messageId;
	/* Nature of the request or report in the message */
	uint32_t messageFunction;
	/* Defines the length (in bytes) of the data payload which follows the header */
	uint32_t datalength;
	/* Payload data, message specific */
	uint32_t payload[MAX_IMG_TV_MSG_DATA_LENGTH/sizeof(uint32_t)];
} IMG_TV_MSG_T;

typedef struct
{
	/* Total length of the message in bytes, excluding the 4 bytes of the length field itself */
	uint32_t length;
	/* Identity of the software entity sending the message sender */
	uint32_t sourceId;
	/* Identity of the software object to which the message should be delivered */
	uint32_t targetId;
	/* Host define Id for message, also indicates Automatic status message */
	uint32_t messageId;
	/* Nature of the request or report in the message */
	uint32_t messageFunction;
	/* Defines the length (in bytes) of the data payload which follows the header */
	uint32_t datalength;
	/* Payload data, message specific */
	uint32_t bufAdress;
	/* Payload data, message specific */
	uint32_t byteCount;
	/* Payload data, message specific */
	uint32_t payload[MAX_IMG_TV_LONG_MSG_DATA_LENGTH/sizeof(uint32_t)];
} IMG_TV_LONG_MSG_T;

/**
 *  A MeOS poolable structure, containing message descriptor, body and management data.
 */
typedef struct
{
	/* start off with a host port message descriptor to make it poolable and QIOable */
	HP_MSG_DESCRIPTOR_T desc;
	/* IMG TV message structure */
	IMG_TV_MSG_T	tvAPIMessage;
	/*
	 * Flag to indicate whether this is an automatic message or a host request.
	 * We manage this explicitly, rather than inferring it from the message content,
	 * to eliminate the possibility of a badly formed request from the host confusing
	 * our message processing.
	 */
	uint32_t autoUpdateMessage;
} POOLABLE_IMG_TV_MSG_T;

/**
 *  A MeOS poolable structure, containing message descriptor, body and management data.
 */
typedef struct
{
	/* start off with a host port message descriptor to make it poolable and QIOable */
	HP_MSG_DESCRIPTOR_T desc;
	/* IMG TV message structure */
	IMG_TV_LONG_MSG_T	tvAPIMessage;
	/*
	 * Flag to indicate whether this is an automatic message or a host request.
	 * We manage this explicitly, rather than inferring it from the message content,
	 * to eliminate the possibility of a badly formed request from the host confusing
	 * our message processing.
	 */
	uint32_t autoUpdateMessage;
} POOLABLE_IMG_TV_LONG_MSG_T;

#endif /* _IMG_TV_MSG_H_ */
