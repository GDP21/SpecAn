/*!****************************************************************************
 @File          hostmsgproc.c

 @Title         UCCP common TV API wrapper

 @Date          15 April 2011

 @Copyright     Copyright (C) Imagination Technologies Limited

 @Description   A common UCC TV application wrapper. Accepts messages from a
 host interface and responds.

 ******************************************************************************/
/******************************************************************************
 *
 * NOTE regarding build variants.
 *
 * By default, the host port message buffers are dynamically allocated from
 * the GRAM pool. However, some customers have a non-standard requirement to place
 * the message buffers at known locations in GRAM. To satisfy these requirements
 * we have a build option to statically declare the buffers rather than allocate
 * them from the pool. To build static buffers, define the macro STATIC_MSGBUF
 * at build time.
 *
 *****************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <uccrt.h>

#include "uccframework.h"
#include "hostmsgproc.h"
#include "img_tv_msg.h"
#include "hostif.h"

/*
 * Constants
 */
#define MAX_TV_CORE_INSTANCES	1	/* For now we only handle a single TV core at a time */

#define NUMBER_AUTO_STATUS_MESSAGES	10

/* We have a request buffer for each TV core and one for the system */
#define NUMBER_REQUEST_BUFFERS	(MAX_TV_CORE_INSTANCES + 1)

/*  GRAM buffer sizing.
 * GRAM buffer sizes are a multiple of 24 bytes so that we can ensure that they are always aligned
 * on both GRAM word boundaries and Meta 8-byte boundaries. This allows us to ensure that, when we have
 * static message buffers, Meta build tools don't place them at a location which doesn't map
 * to a GRAM word boundary
 */
#define GRAM_BUFF_GRANULE       24
#define GRAM_BUFFER_BYTES ((GRAM_BUFF_GRANULE * ((sizeof(IMG_TV_LONG_MSG_T) + GRAM_BUFF_GRANULE - 1) / GRAM_BUFF_GRANULE)))
#define GRAM_BUFFER_WORDS (GRAM_BUFFER_BYTES / 3)
/*
 * Local typedefs
 */

/*
 * Our local per TV core instance.
 * Used to keep the Core instance and whatever we need to keep track of per TV core.
 */
typedef struct
{
    bool activated;
    uint32_t numberOfStdSpecificRegisters;
    uint32_t tvInstanceId;
    uint32_t *updateSourceId;
    UFW_COREINSTANCE_T coreInstance;
} TV_CORE_INSTANCE_T;

/*
 * static data
 */

/* Count of the auto status update messages that we couldn't send out */
static uint32_t missedMSGCount;
/* Input mailbox for task to block on */
static KRN_MAILBOX_T imgtv_mbox;
/* A statically allocated message that is always available to use to send IMG_TV_READY messages */
static POOLABLE_IMG_TV_MSG_T readyMessage;
/* List to act as simple pool of auto status messages into handler function */
static KRN_POOL_T autoStatusMessagePool;
/* All the auto status messages we have to add to pool */
static POOLABLE_IMG_TV_MSG_T autoStatusMessage[NUMBER_AUTO_STATUS_MESSAGES];
/* A number of buffers to hold request messages, passed into host port driver and
 ** reused as status messages, before being recycled into driver for more requests */
static POOLABLE_IMG_TV_MSG_T requestMessage[NUMBER_REQUEST_BUFFERS];

/* Keep a record of the following for use by the activate/deactivate functions */
static UFW_COREDESC_T *arrayOfTVDescriptors;
static unsigned sizeofTVDescriptorArray;
static TV_ACTIVATION_PARAMETER_T *listOfTuners;

static TV_CORE_INSTANCE_T tvCoreContexts[MAX_TV_CORE_INSTANCES];

/*-----------------------------------------------------------------------------*/
/*
 * Validate that a primary register Id is actually supported by a TV instance
 */
static bool
validPrimaryRegisterId(TV_CORE_INSTANCE_T *instance, uint32_t regId)
{
    return (regId < TV_REG_NUM_COMMON_REG)
            || (regId >= TV_REG_FIRST_STD_ID
                    && ((regId - TV_REG_FIRST_STD_ID)
                            < instance->numberOfStdSpecificRegisters));

}
/*-----------------------------------------------------------------------------*/

/* Initialises static data, messages, pools etc.
 ** The host interface must be initialised before this is called */
static void
IMGTV_init(UFW_COREDESC_T tvDescriptors[], unsigned numDesc,
           TV_ACTIVATION_PARAMETER_T *tunerList)
{
    int i;
    POOLABLE_IMG_TV_MSG_T *mp;

    arrayOfTVDescriptors = tvDescriptors;
    sizeofTVDescriptorArray = numDesc;
    listOfTuners = tunerList;

    KRN_initMbox(&imgtv_mbox);

    missedMSGCount = 0;

    /* Initialise pool of automatic status message buffers */
    assert(sizeof(autoStatusMessage)
            == (NUMBER_AUTO_STATUS_MESSAGES * sizeof(POOLABLE_IMG_TV_MSG_T)));
    KRN_initPool(&autoStatusMessagePool, autoStatusMessage,
                 NUMBER_AUTO_STATUS_MESSAGES, sizeof(POOLABLE_IMG_TV_MSG_T));

    /* Set up all the request message descriptors and queue them on to the driver */
    for (i = 0, mp = requestMessage; i < NUMBER_REQUEST_BUFFERS; i++, mp++)
    {
        /* check design assumptions about structure layout */
        assert((void *)&mp->desc == (void *)&mp->desc.iocb);
        assert((void *)&mp->desc == (void *)mp);
        /* set up message descriptor */
        mp->desc.msgLen = sizeof(mp->tvAPIMessage);
        mp->desc.msgPtr = (uint8_t *)(&mp->tvAPIMessage);
        mp->autoUpdateMessage = 0;
        /* queue to driver */
        HSTIF_queuetoRx(&mp->desc, &imgtv_mbox);
    }

    /* ensure the ready message is clear */
    memset(&readyMessage, 0, sizeof(readyMessage));
    readyMessage.tvAPIMessage.messageFunction = IMG_TV_ERROR;

    /* Clear the whole context before we start */
    memset(&tvCoreContexts, 0, sizeof(tvCoreContexts));

    return;
}

/*-----------------------------------------------------------------------------*/

static void
IMGTV_sendReady(POOLABLE_IMG_TV_MSG_T *message)
{
    KRN_IPL_T oldipl;

    if ((message->tvAPIMessage.messageFunction == IMG_TV_PING) ||
        (message->tvAPIMessage.messageFunction == IMG_TV_SHUTDOWN))
    {
        message->tvAPIMessage.targetId = message->tvAPIMessage.sourceId;
        /* Don't update messageId, keep it from PING/SHUTDOWN request */
    }
    else
    {
        message->tvAPIMessage.targetId = 0;
        message->tvAPIMessage.messageId = AUTOMATIC_STATUS_MESSAGE;
    }

    message->tvAPIMessage.length = IMG_TV_READY_LENGTH;
    message->tvAPIMessage.sourceId = 0;
    message->tvAPIMessage.messageFunction = IMG_TV_READY;
    message->tvAPIMessage.datalength = IMG_TV_READY_DATA_LENGTH;

    /* protect reading/clearing of count from async access in handler function */
    oldipl = KRN_raiseIPL();
    message->tvAPIMessage.payload[0] = missedMSGCount;
    missedMSGCount = 0;
    KRN_restoreIPL(oldipl);

    /* Send to host port - message length field isn't self inclusive */
    HSTIF_send((uint8_t *)&message->tvAPIMessage,
               message->tvAPIMessage.length + 4);

    return;
}

/*-----------------------------------------------------------------------------*/

static void
IMGTV_sendError(POOLABLE_IMG_TV_MSG_T *message)
{
    uint32_t temp;

    message->tvAPIMessage.length = IMG_TV_ERROR_LENGTH;
    message->tvAPIMessage.messageFunction = IMG_TV_ERROR;
    message->tvAPIMessage.datalength = IMG_TV_ERROR_DATA_LENGTH;

    /* Swap over source/target Ids from request to status */
    temp = message->tvAPIMessage.sourceId;
    message->tvAPIMessage.sourceId = message->tvAPIMessage.targetId;
    message->tvAPIMessage.targetId = temp;

    /* Send to host port - message length field isn't self inclusive */
    HSTIF_send((uint8_t *)&message->tvAPIMessage,
               message->tvAPIMessage.length + 4);

    return;
}

/*-----------------------------------------------------------------------------*/

static void
registerUpdateHandler(VREG_T *reg, void *parameter)
{
    TV_CORE_INSTANCE_T *instance = (TV_CORE_INSTANCE_T *)parameter;
    POOLABLE_IMG_TV_MSG_T *updateMSG;
    UFW_COREINSTANCE_T *coreInstance;
    uint32_t regId;
    KRN_IPL_T oldipl;

    assert(instance != NULL);

    /* Try and get a message to fill in with this register update */
    updateMSG = KRN_takePool(&autoStatusMessagePool, KRN_NOWAIT);

    if (updateMSG == NULL)
    {
        /* Pool empty, hence we can't handle this message, so count it */

        /* protect incrementing count from clearing in sendReady function */
        oldipl = KRN_raiseIPL();
        missedMSGCount++;
        KRN_restoreIPL(oldipl);

        return;
    }

    coreInstance = &(instance->coreInstance);
    regId = TVREG_PTR2ID(coreInstance,reg);
    assert(validPrimaryRegisterId(instance, regId));

    /* Fill in auto status update message */
    updateMSG->autoUpdateMessage = 1;
    updateMSG->tvAPIMessage.length = IMG_TV_REGVALUE_LENGTH;
    updateMSG->tvAPIMessage.sourceId = instance->tvInstanceId;
    updateMSG->tvAPIMessage.targetId = instance->updateSourceId[TVREG_PTR2OFFSET(coreInstance, reg)];
    updateMSG->tvAPIMessage.messageId = AUTOMATIC_STATUS_MESSAGE;
    updateMSG->tvAPIMessage.messageFunction = IMG_TV_REGVALUE;
    updateMSG->tvAPIMessage.datalength = IMG_TV_REGVALUE_DATA_LENGTH;

    updateMSG->tvAPIMessage.payload[0] = regId;
    updateMSG->tvAPIMessage.payload[1] = VREG_read(reg);

    /* Send onto message handler task, via mailbox. */
    KRN_putMbox(&imgtv_mbox, updateMSG);

    return;
}

/*-----------------------------------------------------------------------------*/

/* All the register access message handling is very much the same,
 ** so this is a common function for all of them */
static void
IMGTV_registerAccess(POOLABLE_IMG_TV_MSG_T *message)
{
    uint32_t priRegId, secRegId, tvInstanceId;
    uint32_t requiredDataLength;
    TV_CORE_INSTANCE_T *instance;
    UFW_COREINSTANCE_T *coreInstance;

    switch (message->tvAPIMessage.messageFunction)
    {
    case IMG_TV_SETREG:
        requiredDataLength = IMG_TV_SETREG_DATA_LENGTH;
        break;

    case IMG_TV_GETREG:
        requiredDataLength = IMG_TV_GETREG_DATA_LENGTH;
        break;

    case IMG_TV_AUTO_ON:
        requiredDataLength = IMG_TV_AUTO_ON_DATA_LENGTH;
        break;

    case IMG_TV_AUTO_OFF:
        requiredDataLength = IMG_TV_AUTO_OFF_DATA_LENGTH;
        break;

    default:
        /* Set to invalid length, so we will signal an error */
        requiredDataLength = MAX_IMG_TV_MSG_DATA_LENGTH + 1;
        break;
    }

    if (message->tvAPIMessage.datalength != requiredDataLength)
    { /* Incorrect data length */
        IMGTV_sendError(message);
        return;
    }

    tvInstanceId = message->tvAPIMessage.targetId;
    if (tvInstanceId > MAX_TV_CORE_INSTANCES || tvInstanceId == 0)
    { /* unknown instance */
        IMGTV_sendError(message);
        return;
    }

    instance = &tvCoreContexts[tvInstanceId - 1]; /* Note tvInstanceId starts at 1 */

    if (!(instance->activated))
    { /* not activated instance, hence we can't use it */
        IMGTV_sendError(message);
        return;
    }
    coreInstance = &(instance->coreInstance);

    priRegId = message->tvAPIMessage.payload[0] & 0xFFFFU;
    secRegId = (message->tvAPIMessage.payload[0] >> 16) & 0xFFFFU;
    if (!validPrimaryRegisterId(instance, priRegId))
    { /* not a valid register for this TV core */
        IMGTV_sendError(message);
        return;
    }

    if (secRegId >= TVREG_getValueCount(coreInstance, priRegId))
    { /* not a valid index for this register */
        IMGTV_sendError(message);
        return;
    }

    switch (message->tvAPIMessage.messageFunction)
    {
    case IMG_TV_SETREG:
        /* Write 'value' from second payload word */
        TVREG_wrapperWriteIndexed(coreInstance, priRegId, secRegId,
                                  message->tvAPIMessage.payload[1]);
        break;

    case IMG_TV_GETREG:
        /* Nothing, the reading of the register is performed below in the common status code. */
        break;

    case IMG_TV_AUTO_ON:
        /* record the source Id for this message, so we know where to target any auto status update messages */
        instance->updateSourceId[TVREG_OFFSET(priRegId)] = message->tvAPIMessage.sourceId;

        /* install our common handler for this register */
        TVREG_installWrapperHandler(coreInstance, priRegId,
                                    registerUpdateHandler, instance);
        break;

    case IMG_TV_AUTO_OFF:
        /* Clear handler for this register */
        TVREG_installWrapperHandler(coreInstance, priRegId, NULL, NULL);
        break;

    default:
        assert(0);
        /* really should never have this case, such an error will have already been caught */
        break;
    }

    /* Fill in our status for this successful request */

    /* Keep the messageId and regId (payload[0]) from request */
    message->tvAPIMessage.length = IMG_TV_REGVALUE_LENGTH;
    message->tvAPIMessage.targetId = message->tvAPIMessage.sourceId;
    message->tvAPIMessage.sourceId = tvInstanceId;
    message->tvAPIMessage.messageFunction = IMG_TV_REGVALUE;
    message->tvAPIMessage.datalength = IMG_TV_REGVALUE_DATA_LENGTH;
    message->tvAPIMessage.payload[1] = TVREG_readIndexed(coreInstance, priRegId,
                                                         secRegId);

    /* Send to host port - message length field isn't self inclusive */
    HSTIF_send((uint8_t *)&message->tvAPIMessage,
               message->tvAPIMessage.length + 4);

    return;
}

/*-----------------------------------------------------------------------------*/

static void
IMGTV_readMemory(POOLABLE_IMG_TV_MSG_T *message)
{
//    uint32_t i;

    POOLABLE_IMG_TV_LONG_MSG_T returnMessage;

    uint32_t requestedBuffAddress = message->tvAPIMessage.payload[0];
    uint32_t requestedByteCount   = message->tvAPIMessage.payload[1];

    /* Check we have a valid ReadMemory request */
    if (message->tvAPIMessage.datalength != IMG_TV_READMEM_DATA_LENGTH)
    { /* not properly formed request */
        IMGTV_sendError(message);
        return;
    }

    /* Check that byteCount is greater than zero */
    if (!requestedByteCount)
    { /* not properly formed request */
        IMGTV_sendError(message);
        return;
    }

    /* all is good... construct Memory Status Message... */

    /* Address in UCCP memory - Copied from ReadMemory request  */
   	returnMessage.tvAPIMessage.bufAdress = requestedBuffAddress;
   	returnMessage.tvAPIMessage.byteCount =
    		(requestedByteCount > MAX_IMG_TV_LONG_MSG_DATA_LENGTH)
    					 ? MAX_IMG_TV_LONG_MSG_DATA_LENGTH : requestedByteCount;
   	returnMessage.tvAPIMessage.sourceId   = 0;
    /* targetId  = sourceId from ReadMemory request
     * messageId = Bits[16:31]=0. Bits[0:15]: Copied from ReadMemory request. */
   	returnMessage.tvAPIMessage.targetId   = message->tvAPIMessage.sourceId;
   	returnMessage.tvAPIMessage.messageId  =
   			                           message->tvAPIMessage.messageId & 0xffff;
   	returnMessage.tvAPIMessage.messageFunction = IMG_TV_MEM;
   	/* dataLength = 8 + byteCount, rounded up to a multiple of 4) */
   	returnMessage.tvAPIMessage.datalength =
   			       IMG_TV_MEM_DATA_LENGTH(returnMessage.tvAPIMessage.byteCount);
   	/* length = dataLength + header_length) */
   	returnMessage.tvAPIMessage.length     =
   			           IMG_TV_MEM_LENGTH(returnMessage.tvAPIMessage.datalength);

	memcpy(&returnMessage.tvAPIMessage.payload[0],
                       &(*(uint32_t*)requestedBuffAddress),
                                               MAX_IMG_TV_LONG_MSG_DATA_LENGTH);

    /* Send to host port - message length field isn't self inclusive */
    HSTIF_send((uint8_t *)&returnMessage.tvAPIMessage,
    		returnMessage.tvAPIMessage.length + 4);

    return;
}

/*-----------------------------------------------------------------------------*/

static void
IMGTV_activate(POOLABLE_IMG_TV_MSG_T *message)
{
    uint32_t tvInstanceId = 0;
    uint32_t demodId;
    TV_CORE_INSTANCE_T *instance;
    bool error = false;

    /* Check we have a valid activation request */
    if (message->tvAPIMessage.datalength != IMG_TV_ACTIVATE_DATA_LENGTH)
    { /* not properly formed request */
        IMGTV_sendError(message);
        return;
    }

    do
    { /* do checking/processing until we complete or hit an error */

        demodId = message->tvAPIMessage.payload[0];
        if (demodId >= sizeofTVDescriptorArray)
        { /* unknown TV core */
            error = true;
            break;
        }

        /* find first unactivated instance */
        for (tvInstanceId = 0; tvInstanceId < MAX_TV_CORE_INSTANCES;
                tvInstanceId++)
        {
            if (!(tvCoreContexts[tvInstanceId].activated))
                break;
        }

        /* Move to starting id at 1 */
        tvInstanceId++;

        if (tvInstanceId > MAX_TV_CORE_INSTANCES || tvInstanceId == 0)
        { /* unsupported instance: so couldn't find one to use */
            error = true;
            break;
        }

        /* Note we are ignoring the channel Id in the request, just passing it onto the status message */

        instance = &tvCoreContexts[tvInstanceId - 1]; /* Note tvInstanceId starts at 1 */

        if (instance->activated)
        { /* already activated */
            error = true;
            break;
        }

        if (!UFW_activateCore(&arrayOfTVDescriptors[demodId], 1,
                              &(instance->coreInstance), listOfTuners))
        {
            /* Failed to activate */
            error = true;
            break;
        }
    }
    while (0);

    if (error)
    {
        /* error in activation signaled by a sourceId of zero in the status */
        tvInstanceId = 0;
    }
    else
    {
        /* set up instance for this TV core */
        instance->activated = true;
        instance->numberOfStdSpecificRegisters =
                instance->coreInstance.coreDesc->numRegisters - TV_REG_NUM_COMMON_REG;
        instance->tvInstanceId = tvInstanceId;
        instance->updateSourceId = UFW_memAlloc(
                (instance->coreInstance.coreDesc->numRegisters * sizeof(uint32_t)),
                UFW_MEMORY_TYPE_NORMAL);

    }

    /* send activated status */

    /* Keep the messageId and payload from request */
    message->tvAPIMessage.length = IMG_TV_ACTIVATED_LENGTH;
    message->tvAPIMessage.targetId = message->tvAPIMessage.sourceId;
    message->tvAPIMessage.sourceId = tvInstanceId;
    message->tvAPIMessage.messageFunction = IMG_TV_ACTIVATED;
    message->tvAPIMessage.datalength = IMG_TV_ACTIVATED_DATA_LENGTH;
    /* Payload is just the same as the request */

    /* Send to host port - message length field isn't self inclusive */
    HSTIF_send((uint8_t *)&message->tvAPIMessage,
               message->tvAPIMessage.length + 4);

    return;
}

/*-----------------------------------------------------------------------------*/
bool
IMGTV_activateReg(UFW_COREINSTANCE_T *activeInstance)
{
    uint32_t tvInstanceId = 0;
    TV_CORE_INSTANCE_T *instance;

    /* find first unactivated instance */
    for (tvInstanceId = 0; tvInstanceId < MAX_TV_CORE_INSTANCES; tvInstanceId++)
	{
		if (!(tvCoreContexts[tvInstanceId].activated))
			break;
	}

    /* Move to starting id at 1 */
    tvInstanceId++;

    if (tvInstanceId > MAX_TV_CORE_INSTANCES || tvInstanceId == 0)
    { /* unsupported instance: so couldn't find one to use */
    	return false;
    }

    instance = &tvCoreContexts[tvInstanceId - 1]; /* Note tvInstanceId starts at 1 */

    if (instance->activated)
    { /* already activated */
    	return false;
    }

    /* set up instance for this TV core */
    instance->coreInstance.coreDesc 			 = activeInstance->coreDesc;
    instance->coreInstance.instanceExtensionData = activeInstance->instanceExtensionData;
    instance->coreInstance.registerBlock 		 = activeInstance->registerBlock;
    instance->coreInstance.priority 			 = activeInstance->priority;
    instance->activated = true;
    instance->numberOfStdSpecificRegisters =
                instance->coreInstance.coreDesc->numRegisters - TV_REG_NUM_COMMON_REG;
    instance->tvInstanceId = tvInstanceId;
    instance->updateSourceId = UFW_memAlloc(
                (instance->coreInstance.coreDesc->numRegisters * sizeof(uint32_t)),
                UFW_MEMORY_TYPE_NORMAL);

    return true;
}

/*-----------------------------------------------------------------------------*/

static void
IMGTV_deactivate(POOLABLE_IMG_TV_MSG_T *message)
{
    uint32_t i;
    uint32_t n;
    uint32_t tvInstanceId = message->tvAPIMessage.targetId;
    TV_CORE_INSTANCE_T *instance;
    UFW_COREINSTANCE_T *coreInstance;
    bool error = false;

    /* Check we have a valid deactivation request */
    if (message->tvAPIMessage.datalength != IMG_TV_DEACTIVATE_DATA_LENGTH)
    { /* not properly formed request */
        IMGTV_sendError(message);
        return;
    }

    do
    { /* do checking/processing until we complete or hit an error */

        if (tvInstanceId > MAX_TV_CORE_INSTANCES || tvInstanceId == 0)
        { /* unsupported instance */
            error = true;
            break;
        }

        instance = &tvCoreContexts[tvInstanceId - 1]; /* Note tvInstanceId starts at 1 */

        if (!(instance->activated))
        { /* not activated */
            error = true;
            break;
        }

        coreInstance = &instance->coreInstance;

        if (TV_STATE_INITIALISED != TVREG_read(coreInstance, TV_REG_STATE))
        { /* TV core is not in the correct state */
            error = true;
            break;
        }

        /* Clear any installed handler functions on the registers.
         ** This will stop the deactivation triggering auto status updates */
        for (i = 0; i < TV_REG_NUM_COMMON_REG; i++)
            TVREG_installWrapperHandler(coreInstance, i, NULL, NULL);
        for (n = 0, i = TV_REG_FIRST_STD_ID; n < instance->numberOfStdSpecificRegisters; i++, n++)
            TVREG_installWrapperHandler(coreInstance, i, NULL, NULL);

        /* de-activate the software instance */
        if (!UFW_deactivateCore(&(instance->coreInstance), listOfTuners))
        {
            /* Failed to deactivate */
            error = true;
            break;
        }
    }
    while (0);

    if (error)
    {
        /* error in deactivation signaled by a sourceId of zero in the status */
        tvInstanceId = 0;
    }
    else
    {
        /* clear instance for this TV core */
        instance->activated = false;
        instance->numberOfStdSpecificRegisters = 0;
        instance->tvInstanceId = 0;
        /* Note the memory previously allocated for this will be de-alloced with the deactivate */
        instance->updateSourceId = NULL;
    }

    /* send deactivated status */
    /* Keep the messageId from request */
    message->tvAPIMessage.length = IMG_TV_DEACTIVATED_LENGTH;
    message->tvAPIMessage.targetId = message->tvAPIMessage.sourceId;
    message->tvAPIMessage.sourceId = tvInstanceId;
    message->tvAPIMessage.messageFunction = IMG_TV_DEACTIVATED;
    message->tvAPIMessage.datalength = IMG_TV_DEACTIVATED_DATA_LENGTH;

    /* Send to host port - message length field isn't self inclusive */
    HSTIF_send((uint8_t *)&message->tvAPIMessage,
               message->tvAPIMessage.length + 4);

    return;
}

/*-----------------------------------------------------------------------------*/

/* Sanity check the message for obvious errors, invalid headers etc.
 ** Returns true if an error has been found */
static bool
IMGTV_badMessage(POOLABLE_IMG_TV_MSG_T *message)
{
    /* Too small for a header */
    if (message->tvAPIMessage.length < IMG_TV_HEADER_LENGTH)
    {
        /* Clear as we can't trust any of it */
        memset(&(message->tvAPIMessage), 0, sizeof(IMG_TV_MSG_T));
        return (true);
    }

    /* Too big for our buffers, NB length excludes itself */
    if (message->tvAPIMessage.length
            > (sizeof(IMG_TV_MSG_T) - sizeof(uint32_t)))
        return (true);

    /* if the two lengths don't make sense */
    if ((message->tvAPIMessage.length - message->tvAPIMessage.datalength)
            != IMG_TV_HEADER_LENGTH)
        return (true);

    return (false);
}

/*-----------------------------------------------------------------------------*/

static void
IMGTV_handleAutoMessage(POOLABLE_IMG_TV_MSG_T *message)
{
    /* Decide if this is an auto status update from a register update, or a request that needs handling */
    if (message->tvAPIMessage.messageFunction == IMG_TV_REGVALUE)
    { /* Auto status updates */
        TV_CORE_INSTANCE_T *instance =
                &tvCoreContexts[message->tvAPIMessage.sourceId - 1]; /* Note TV Instance Id starts at 1 */

        /* Only send out the auto status update if the TV core instance is active.
         ** This catches any messages formed before a core is deactivated,
         ** but doesn't get sent out until after the deactivation completes */
        if (instance->activated)
        {
            /* If we have missed any auto status update messages then inform the host of this */
            if (missedMSGCount)
                IMGTV_sendReady(&readyMessage);

            /* Send to host port - message length field isn't self inclusive */
            HSTIF_send((uint8_t *)&message->tvAPIMessage,
                       message->tvAPIMessage.length + 4);
        }
    }
    else
    {
        assert(0);
        /* should never happen */
    }
}

/*-----------------------------------------------------------------------------*/
static void
IMGTV_handleRequestMessage(POOLABLE_IMG_TV_MSG_T *message)
{
    bool shutdown = false;
    /* Ensure that message Id only has the least significant 16 bits used */
    /* This allows us to echo it back without any confusion */
    message->tvAPIMessage.messageId &= 0xFFFF;

#if IMG_TV_PING_DATA_LENGTH != IMG_TV_SHUTDOWN_DATA_LENGTH
#error "Implementation assumes PING and shutdown messages are the same length"
#endif

    switch (message->tvAPIMessage.messageFunction)
    {
    case IMG_TV_SHUTDOWN:
        shutdown = true;
        /* deliberate drop through */
    case IMG_TV_PING:
        if (message->tvAPIMessage.datalength != IMG_TV_PING_DATA_LENGTH)
        {
            shutdown = false; /* only shutdown in response to an error free request */
            IMGTV_sendError(message);
        }
        else
        {
            IMGTV_sendReady(message);
        }
        break;

    case IMG_TV_ACTIVATE:
        IMGTV_activate(message);
        break;

    case IMG_TV_DEACTIVATE:
        IMGTV_deactivate(message);
        break;

    case IMG_TV_READMEM:
    	IMGTV_readMemory(message);
    	break;
    case IMG_TV_SETREG:
    case IMG_TV_GETREG:
    case IMG_TV_AUTO_ON:
    case IMG_TV_AUTO_OFF:
        /* All register access requests handled by single function */
        IMGTV_registerAccess(message);
        break;

    default:
        IMGTV_sendError(message);
        break;
    }
    if (shutdown)
        UCCP_shutdown();
}

/*-----------------------------------------------------------------------------*/

/*
 * This is the top level (public) function for the IMG common TV API application.
 * It is intended that this is called from main and takes over the start up task.
 */
#ifdef STATIC_MSGBUF
/*
 * Message buffer declared as 8 byte integers to ensure that meta tools always achieve
 * no less than 8-byte alignment. We do a runtime check to ensure that we have actually
 * placed on a GRAM boundary as well
 */
uint64_t IMGTV_messageBuffers[(2 * GRAM_BUFFER_BYTES) / 8];
#endif
void
IMGTV_MessageApp(UFW_COREDESC_T tvDescriptors[], unsigned numDesc,
                 TV_ACTIVATION_PARAMETER_T *tunerList, bool initEnvironment)
{
    UCCP_GRAM_ADDRESS_T ui32BufBaseH;
    UCCP_GRAM_ADDRESS_T ui32BufBaseU;
#ifdef STATIC_MSGBUF
    uint32_t temp;
#endif

    if(initEnvironment)
    {
    	/* Init the environment we need */
    	UFW_init();
    	UCCP_init();
    	UCCP_reset();
    }

#ifdef STATIC_MSGBUF
    /* Calculate and verify the GRAM equivalent addresses of the static message buffers */
    temp = ((uint32_t)IMGTV_messageBuffers) - MEMGBL_PKD;
    ui32BufBaseH = temp / 3;

    if (((3 * (ui32BufBaseH)) != temp) || /* Not GRAM word aligned */
            (ui32BufBaseH & 0xff000000) || /* bottom of buffer outside GRAM range */
            ((ui32BufBaseH + 2 * GRAM_BUFFER_WORDS - 1) & 0xff000000) /* top of buffer outside GRAM range*/
    )
    {
        assert("Badly placed static message buffer" == NULL);
        exit(0); /* Don't rely on assert - release build could get this wrong so we want an obvious failure" */
    }

#else
    /* Allocate two contiguous buffers in GRAM for (one each way to/from host) */

    ui32BufBaseH = UFW_gramAlloc(2 * GRAM_BUFFER_WORDS);
#endif
    ui32BufBaseU = ui32BufBaseH + GRAM_BUFFER_WORDS;

    /* Init host port driver */

    HSTIF_init(ui32BufBaseH, ui32BufBaseU, GRAM_BUFFER_WORDS,
               KRN_LOWEST_PRIORITY);

    /* Init the internal state of the IMG TV message handling wrapper */
    IMGTV_init(tvDescriptors, numDesc, tunerList);

    /* Just before we start handling requests and automatic status updates send an IMG_TV_READY */
    IMGTV_sendReady(&readyMessage);

    /*
     * Forever accept messages and process them.
     * The imgtv_mbox receives messages from the host and locally generated async messages.
     * (Synchronous replies are sent immediately and not routed via this processing loop)
     */
    for (;;)
    {
        POOLABLE_IMG_TV_MSG_T *message = KRN_getMbox(&imgtv_mbox, KRN_INFWAIT);

        /* Handle obvious errors or handle the message */
        if (IMGTV_badMessage(message))
            IMGTV_sendError(message);
        else if (message->autoUpdateMessage)
            IMGTV_handleAutoMessage(message);
        else
            IMGTV_handleRequestMessage(message);

        if (message->autoUpdateMessage)
        {
            /* Auto message buffers are returned to their pool */
            KRN_returnPool(message);
        }
        else
        {
            /*
             * Request message buffers are re-queued on the driver.
             * First fix up the descriptor in case it has been modified during processing.
             */
            message->desc.msgLen = sizeof(message->tvAPIMessage);
            message->desc.msgPtr = (uint8_t *)&message->tvAPIMessage;
            HSTIF_queuetoRx(&message->desc, &imgtv_mbox);
        }
    }
    return;
}

