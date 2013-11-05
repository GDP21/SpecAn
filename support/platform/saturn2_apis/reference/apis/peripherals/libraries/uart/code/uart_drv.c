/*!
*******************************************************************************
  file   uart_drv.c

  brief  UART Device Driver

         This file contains the functions that make up the UART Device driver.

  author Imagination Technologies

         <b>Copyright 2006 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

*******************************************************************************/

/* ---------------------------- INCLUDE FILES ---------------------------- */

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
** TBI_NO_INLINES is needed for use of TBI_CRITON() and TBI_CRITOFF()
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#define TBI_NO_INLINES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* MeOS Library */
#include <MeOS.h>

#include <img_defs.h>

/* System */
#include <ioblock_defs.h>
#include <ioblock_utils.h>

/* Serial Port Driver */
#include "ser_api.h"
#include "uart_reg.h"
#include "uart_hw.h"
#include "uart_drv.h"



/* --------------------------- DATA STRUCTURES --------------------------- */

/* Internal state of the UART Driver */
typedef struct
{
    QIO_DEVICE_T  *		tx_dev;              //pointer to TX device descriptor
    QIO_DEVICE_T  *		rx_dev;              //   "    "  RX   "        "
    QIO_IOPARS_T  *		tx_current;          //pointer to iopars stored in IOCB
    QIO_IOPARS_T  *		rx_current;          //   "    "    "      "    "    "
    unsigned long		tx_byte_count;
    unsigned long		rx_byte_count;
    unsigned char		cts;
    unsigned char		ctsEnabled;
    unsigned char		divisorMsb;
    unsigned char		divisorLsb;
    unsigned char		wordFmt;
    unsigned char		rxThresh;
    unsigned char		inUse;
    unsigned char		errorFlag;
    unsigned long		fifoCtrlReg;
} UART_STATE_T;


/* ------------------------- FUNCTION PROTOTYPES ------------------------- */

static int init_tx(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass,
                                int *devRank, unsigned intMasks[QIO_MAXVECGROUPS]);
static int init_rx(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass,
                                int *devRank, unsigned intMasks[QIO_MAXVECGROUPS]);

static void start_tx(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
static void start_rx(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
static void isr(QIO_DEVICE_T *dev);
static void cancel_tx(QIO_DEVICE_T *dev);
static void cancel_rx(QIO_DEVICE_T *dev);


/* ----------------------------- GLOBAL DATA ----------------------------- */

/* The transmit driver object */
const QIO_DRIVER_T STX_driver =
{
    isr,   		/* ISR                       */
    init_tx,  	/* init function             */
    start_tx, 	/* start function            */
    cancel_tx, 	/* cancel function        	 */
    NULL,  		/* no power control function */
    NULL,  		/* no sim start function     */
    NULL   		/* no shut function          */
};

/* The receive driver object */
const QIO_DRIVER_T SRX_driver =
{
    isr,   		/* ISR                       */
    init_rx,  	/* init function             */
    start_rx, 	/* start function            */
    cancel_rx, 	/* cancel function        	 */
    NULL,  		/* no power control function */
    NULL,  		/* no sim start function     */
    NULL   		/* no shut function          */

};


/* ----------------------------- STATIC DATA ----------------------------- */

/* Allocate state for the maximum number of UART interfaces */
// MAX-NUM_UART_BLOCKS defined in uart_drv.h
ioblock_sBlockDescriptor	*	g_apsUARTBlock[ MAX_NUM_UART_BLOCKS ] =
{
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL
};

#ifdef DEBUG
/* Debug counters */
static int DRVprogress = 0xFFFF;
static int DRVprogress2 = 0;
static int DRVprogress3 = 0;
static int DRV_NONE = 0;
static int DRV_ERR = 0;
static int DRV_MODEM = 0;
static int DRV_RLS = 0;
static int DRV_RXDATA = 0;
static int DRV_CHARTIME = 0;
static int DRV_TXFIFO = 0;
static int DRVinitProg = 0;
static volatile int rxdataEmpty = 0;
static volatile int rxdataEmpty2 = 0;
#endif


/* --------------------------- STATIC FUNCTIONS -------------------------- */

/*
**  FUNCTION:       init_tx
**
**  DESCRIPTION:    Initialises the transmit device driver and serial port device
**
**  INPUTS:         intNumber : interrupt number to associate with the device
**
**  OUTPUTS:        *pwrClass : power saving capabilities of device
**                  *devRank  : rank (register buffering depth) of device
*/
static int init_tx(	QIO_DEVICE_T		*	dev, 
					QIO_DEVPOWER_T		*	pwrClass,
					int					*	devRank, 
					unsigned				intMasks[QIO_MAXVECGROUPS] )
{
    SER_REG_T					*	regPtr;
	UART_STATE_T				*	psContext;
	unsigned long					ui32Reg;
    int								lockState;
	ioblock_sBlockDescriptor	*	psBlockDesc;

#ifdef DEBUG
DRVprogress = 0;
#endif

	// Check we have enough memory to "allocate" our context
	IMG_ASSERT( sizeof( UART_STATE_T ) <= INTERNAL_SER_MEM_SIZE );

	// Get context structure from defined block descriptor
	psBlockDesc = g_apsUARTBlock[ dev->id ];
	psContext	= (UART_STATE_T *)psBlockDesc->pvAPIContext;

    //Asynchronous serial port has no power saving
    *pwrClass = QIO_POWERNONE;

    //Port has only a single rank
    *devRank = 1;

    //Record 'TX device' handle
    //As the ISR is called from both devices, when we complete a TX job
    //we must report to QIO using the 'TX device' handle
    psContext->tx_dev = dev;

    psContext->cts = SERReadCTS(psBlockDesc->ui32Base);
    DummyWrite(psBlockDesc->ui32Base);
	
	//Set up serial port registers
    regPtr = (SER_REG_T *)psBlockDesc->ui32Base;

    //Set DLAB = 1 to configure baudrate
    WRITE(&regPtr->line_control, (unsigned long)SER_LCR_DLAB_MASK);
    DummyWrite(psBlockDesc->ui32Base);

    WRITE(&regPtr->interrupt_en, (unsigned long)psContext->divisorMsb);
    DummyWrite(psBlockDesc->ui32Base);

    WRITE(&regPtr->txrx_holding, (unsigned long)psContext->divisorLsb);
    DummyWrite(psBlockDesc->ui32Base);

    //Set data format (word length, parity, stop bits)
    WRITE(&regPtr->line_control, (unsigned long)psContext->wordFmt);
    DummyWrite(psBlockDesc->ui32Base);

    //Clear DLAB (sets RTS high as by-product)
    WRITE(&regPtr->modem_control, 0);
    DummyWrite(psBlockDesc->ui32Base);

    //Set up the FIFOs
    psContext->fifoCtrlReg = (unsigned long)((psContext->rxThresh << SER_FCR_RXTR_SHIFT) |
                                                 SER_FCR_ENFIFO_MASK |
                                                 SER_FCR_RSTXF_MASK  |
                                                 SER_FCR_RSRXF_MASK);
    WRITE(&regPtr->fcr_iid, psContext->fifoCtrlReg);
    DummyWrite(psBlockDesc->ui32Base);

    //Initialise interrupts
    WRITE(&regPtr->interrupt_en,
                   (unsigned long)(SER_IER_EDSSI_MASK |
                   SER_IER_ELSI_MASK |
                   SER_IER_ETBFI_MASK));
    DummyWrite(psBlockDesc->ui32Base);

    //Set up interrupt mapping (edge sensitive trigger)

	IOBLOCK_CalculateInterruptInformation( psBlockDesc );

	IMG_MEMCPY( intMasks, psBlockDesc->ui32IntMasks, sizeof( unsigned int ) * QIO_MAXVECGROUPS );
	
	TBI_LOCK( lockState );
	
	// HWLEVELEXT for UART
	ui32Reg = READ( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress );
	if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		ui32Reg &= ~(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	else if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
	{
		ui32Reg |= psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask;
	}
	WRITE( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress, ui32Reg );

	TBI_UNLOCK( lockState );

    #ifdef DEBUG
    DRVinitProg++;
    DRVprogress++;
    #endif

    return 0;
}

/*
**  FUNCTION:       init_rx
**
**  DESCRIPTION:    Initialises the receive device driver and serial port device
**					This function MUST be called AFTER init_tx.
**
**  INPUTS:         intNumber : interrupt number to associate with the device
**
**  OUTPUTS:        *pwrClass : power saving capabilities of device
**                  *devRank  : rank (register buffering depth) of device
*/
static int init_rx( QIO_DEVICE_T	*	dev, 
					QIO_DEVPOWER_T	*	pwrClass,
					int				*	devRank, 
					unsigned			intMasks[QIO_MAXVECGROUPS] )
{
	UART_STATE_T				*	psContext;
	ioblock_sBlockDescriptor	*	psBlockDesc;

	// Get context
	psBlockDesc = g_apsUARTBlock[ dev->id ];
	psContext = (UART_STATE_T *)psBlockDesc->pvAPIContext;

    //Asynchronous serial port has no power saving
    *pwrClass = QIO_POWERNONE;

    //Port has only a single rank
    *devRank = 1;

    //Record 'RX device' handle
    //As the ISR is called from both devices, when we complete a RX job
    //we must report to QIO using the 'RX device' handle
    psContext->rx_dev = dev;

#ifdef DEBUG
DRVprogress++;
#endif

    //Reset error flag
    psContext->errorFlag = FALSE;

	IMG_MEMCPY( intMasks, psBlockDesc->ui32IntMasks, sizeof( unsigned int ) * QIO_MAXVECGROUPS );

    //level sensitive interrupt is already configured in TX init

    return 0;
}

/*
**  FUNCTION:       FillTXFifo
**
**  DESCRIPTION:    Fill the transmit FIFO with as many bytes as we can.
**
**  INPUTS:          base_address  -  base address for device register block
**                  *data          -  pointer to data for transmission
**                   max_bytes     -  maximum number of bytes to be transmitted
**
**  OUTPUTS:        None
**
**  RETURNS:        Number of bytes transmitted.
*/
static unsigned long FillTXFifo(unsigned long base_address, unsigned char *data, unsigned long max_bytes)
{
    unsigned int i;
    unsigned long n_bytes;

    if (max_bytes > TX_FIFO_DEPTH)
        n_bytes = TX_FIFO_DEPTH;
    else
        n_bytes = max_bytes;

    for (i = 0; i < n_bytes ; i++)
    {
        SERTx(base_address, *data++);
        DummyWrite(base_address);
    }

#ifdef DEBUG
DRVprogress2 += n_bytes;
#endif

    return(n_bytes);
}

/*
**  FUNCTION:       start_tx
**
**  DESCRIPTION:
**
**  INPUTS:         *ioPars : transaction parameter and data structure
**
**  OUTPUTS:        None
*/
static void start_tx(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars)
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsUARTBlock[ dev->id ];
    UART_STATE_T				*	statePtr	= (UART_STATE_T *)psBlockDesc->pvAPIContext;
    unsigned long 					interruptReg;
    unsigned long 					num_bytes_left;
    unsigned long 					num_transmitted;
    unsigned char 				*	dataPtr;
    unsigned char 					cts;
    SER_REG_T					*	regPtr;

    //Copy job parameters into device state:
    //size = ioPars->count;
    //data = ioPars->pointer;
    statePtr->tx_current = ioPars;

    //Reset transmitted byte count
    statePtr->tx_byte_count = 0;

    //If CTS/RTS is not enabled, ignore the CTS state
    if (statePtr->ctsEnabled)
        cts = statePtr->cts;
    else
        cts = 1;

    //Start data transfer if CTS and Tx FIFO empty
    if (cts && SERTxFifoEmpty(psBlockDesc->ui32Base))
    {
#ifdef DEBUG
DRVprogress+=0x10000;
#endif
        //Fill transmit fifo
        num_bytes_left = statePtr->tx_current->counter;
        dataPtr = (unsigned char *)(statePtr->tx_current->pointer);
        num_transmitted = FillTXFifo(psBlockDesc->ui32Base, dataPtr, num_bytes_left);

        //Update status
        statePtr->tx_byte_count = num_transmitted;
    }
    else
    {
#ifdef DEBUG
DRVprogress+=0x100000;
#endif
    }

    //Disable interrupt enable register
    regPtr = (SER_REG_T *)psBlockDesc->ui32Base;

    interruptReg = READ(&regPtr->interrupt_en);
    DummyWrite(psBlockDesc->ui32Base);

    WRITE(&regPtr->interrupt_en, 0);
    DummyWrite(psBlockDesc->ui32Base);

    //Re-enable interrupt enable register
    WRITE(&regPtr->interrupt_en,
                   (unsigned long)(interruptReg |
                   SER_IER_EDSSI_MASK |
                   SER_IER_ELSI_MASK |
                   SER_IER_ETBFI_MASK));
    DummyWrite(psBlockDesc->ui32Base);
}

/*
**  FUNCTION:       start_rx
**
**  DESCRIPTION:
**
**  INPUTS:         *ioPars : transaction parameter and data structure
**
**  OUTPUTS:        None
*/
static void start_rx(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars)
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsUARTBlock[ dev->id ];
    UART_STATE_T				*	statePtr	= (UART_STATE_T *)psBlockDesc->pvAPIContext;
    unsigned long					interruptReg;
    SER_REG_T					*	regPtr;

    //Copy job parameters into device state:
    statePtr->rx_current = ioPars;

	if (statePtr->errorFlag == TRUE)
	{
		//Overrun Error previously detected
		//Set error flag in job parameters
		statePtr->rx_current->opcode = SER_STATUS_OVERRUN_ERROR;

		//Cancel operation by completing it
		statePtr->rx_current = NULL;
		QIO_complete(statePtr->rx_dev, QIO_IOCOMPLETE);
		QIO_start(statePtr->rx_dev);
	}
	else
	{

		//Reset received byte count and error flag
		statePtr->rx_byte_count = 0;
		statePtr->rx_current->opcode = SER_STATUS_SUCCESS;

		//Set RTS
		SERSetRTS(psBlockDesc->ui32Base);
		DummyWrite(psBlockDesc->ui32Base);

		SEREnableRxTrig(psBlockDesc->ui32Base);
		DummyWrite(psBlockDesc->ui32Base);

		//Disable interrupt enable register
		regPtr = (SER_REG_T *)psBlockDesc->ui32Base;

		interruptReg = READ(&regPtr->interrupt_en);
		DummyWrite(psBlockDesc->ui32Base);

		WRITE(&regPtr->interrupt_en, 0);
		DummyWrite(psBlockDesc->ui32Base);

		//Re-enable interrupt enable register
		WRITE(&regPtr->interrupt_en, interruptReg);
		DummyWrite(psBlockDesc->ui32Base);
	}

#ifdef DEBUG
DRVprogress+=0x1000;
#endif
}

/*
**  FUNCTION:       isr
**
**  DESCRIPTION:    Handles interrupts to the serial port device driver.
**
**  INPUTS:         None
**
**  OUTPUTS:        None
**
**  RETURNS:        void
*/
static void isr(QIO_DEVICE_T *dev)
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsUARTBlock[ dev->id ];
    UART_STATE_T				*	statePtr	= (UART_STATE_T *)psBlockDesc->pvAPIContext;
	unsigned long  					num_bytes_left;
    unsigned long  					num_transmitted = 0;
    unsigned char				*	dataPtr;
    unsigned char  					cts;
    int								trig_source;
    int								lsr;
    unsigned long					interruptReg;
    SER_REG_T					*	regPtr;

#ifdef DEBUG
DRVprogress3++;
#endif

    //Handle a single cause for the trigger before returning
    //We handle only a single trigger cause at a time because this limits the
    //time during which interrupts are disabled. If multiple trigger causes
    //occur at the same time, the level-sensitive trigger will not be deasserted
    //and another interrupt will be generated.
    trig_source = SERReadTrigSource(psBlockDesc->ui32Base);
    DummyWrite(psBlockDesc->ui32Base);

    regPtr = (SER_REG_T *)psBlockDesc->ui32Base;

    switch(trig_source)
    {
        case(TX_FIFO_EMPTY):
            //Transmit FIFO is empty...

            //If CTS/RTS is not enabled, ignore the CTS state
            if (statePtr->ctsEnabled == 1)
                cts = statePtr->cts;
            else
                cts = 1;

            if (cts && (statePtr->tx_current != NULL))
            {
#ifdef DEBUG
DRV_TXFIFO++;
#endif
                //CTS is set, so transmit more data if necessary
                num_bytes_left = (statePtr->tx_current->counter - statePtr->tx_byte_count);

                if (num_bytes_left > 0)
                {
                    //Assign local dataPtr to ioPars->pointer to access TX data
                    dataPtr = (unsigned char *)(statePtr->tx_current->pointer);

                    //Fill transmit fifo
                    num_transmitted = FillTXFifo(psBlockDesc->ui32Base, (dataPtr + statePtr->tx_byte_count), num_bytes_left);

                    //Update status
                    statePtr->tx_byte_count += num_transmitted;
                }

                if ((num_transmitted == num_bytes_left) || (num_bytes_left == 0))
                {
                    //We have transmitted all the data - notify QIO, invalidate tx_current
                    statePtr->tx_current = NULL;
#ifdef DEBUG
DRV_TXFIFO+=0x10000;
#endif
                    QIO_complete(statePtr->tx_dev, QIO_IOCOMPLETE);
                    QIO_start(statePtr->tx_dev);
                }
            }
            else
            {
                //CTS is not set, or no TX job is queued
#ifdef DEBUG
DRV_TXFIFO+=0x1000000;
#endif
            }
            break;

        case(RX_DATA):
#ifdef DEBUG
DRV_RXDATA++;
#endif
        case(CHAR_TIMEOUT):
#ifdef DEBUG
DRV_CHARTIME++;
#endif
            //First check that a valid RX job is in progress
            if (statePtr->rx_current != NULL)
            {
                if (SERRxFifoEmpty(psBlockDesc->ui32Base))
                {
#ifdef DEBUG
                    rxdataEmpty++;
#endif
                }
                //Assign local dataPtr to ioPars->pointer for received data
                dataPtr = (unsigned char *)(statePtr->rx_current->pointer);

                //Data has been received - read any bytes in FIFO
                do
                {
                    DummyWrite(psBlockDesc->ui32Base);

                    if (SERRxFifoEmpty(psBlockDesc->ui32Base))
                    {
#ifdef DEBUG
                        rxdataEmpty2++;
#endif
                    }
                    DummyWrite(psBlockDesc->ui32Base);

                    dataPtr[statePtr->rx_byte_count++] = SERRxData(psBlockDesc->ui32Base);
                    DummyWrite(psBlockDesc->ui32Base);

                    if (statePtr->rx_byte_count == statePtr->rx_current->counter)
                    {
#ifdef DEBUG
DRV_RXDATA+=0x10000;
#endif
                        //All expected data has been received - notify QIO and invalidate rx_current
                        statePtr->rx_current = NULL;
                        QIO_complete(statePtr->rx_dev, QIO_IOCOMPLETE);
                        QIO_start(statePtr->rx_dev);
                        break;
                    }
                }
                while(!SERRxFifoEmpty(psBlockDesc->ui32Base));

            }
            else
            {
#ifdef DEBUG
DRV_RXDATA+=0x1000000;
#endif
                //No job in progress - unexpected RX data
                //Disable trigger and RTS (re-enabled in start_rx())
                SERClearRTS(psBlockDesc->ui32Base);
                DummyWrite(psBlockDesc->ui32Base);

                SERDisableRxTrig(psBlockDesc->ui32Base);
                DummyWrite(psBlockDesc->ui32Base);
            }
            break;

        case(RECEIVER_LINE_STATUS):
            //First check that a valid RX job is in progress
            if (statePtr->rx_current != NULL)
            {
#ifdef DEBUG
DRV_RLS++;
#endif
                //Handle Errors
                lsr = SERReadLSR(psBlockDesc->ui32Base);
                DummyWrite(psBlockDesc->ui32Base);

                //Assign local dataPtr to ioPars->pointer for received data
                dataPtr = (unsigned char *)(statePtr->rx_current->pointer);

                if (lsr & (SER_LSR_PE_MASK | SER_LSR_FE_MASK | SER_LSR_BI_MASK))
                {
                    //Parity Error, Framing Error or Break Interrupt Error detected
                    //Read byte from FIFO to clear error flag
                    dataPtr[statePtr->rx_byte_count++] = SERRxData(psBlockDesc->ui32Base);
                    DummyWrite(psBlockDesc->ui32Base);

                    //Set error flag in job parameters
                    statePtr->rx_current->opcode = SER_STATUS_ERROR;

                    if (statePtr->rx_byte_count == statePtr->rx_current->counter)
                    {
#ifdef DEBUG
DRV_RLS+=0x10000;
#endif
                        //All expected data has been received - notify QIO and invalidate rx_current
                        statePtr->rx_current = NULL;
                        QIO_complete(statePtr->rx_dev, QIO_IOCOMPLETE);
                        QIO_start(statePtr->rx_dev);
                    }
                }
                else if (lsr & SER_LSR_OE_MASK)
                {
#ifdef DEBUG
DRV_RLS+=0x100000;
#endif
                    //Overrun Error detected
                    //Set error flag in job parameters
                    statePtr->rx_current->opcode = SER_STATUS_OVERRUN_ERROR;
                    //Set error flag in driver state
                    statePtr->errorFlag = TRUE;

					//Cancel operation by completing it
					statePtr->rx_current = NULL;
					QIO_complete(statePtr->rx_dev, QIO_IOCOMPLETE);
					QIO_start(statePtr->rx_dev);
                }
            }
            else
            {
#ifdef DEBUG
DRV_RLS+=0x100;
#endif
                //No job in progress - unexpected RX error

                //read LSR register to reset interrupt
                lsr = SERReadLSR(psBlockDesc->ui32Base);
                DummyWrite(psBlockDesc->ui32Base);

                //Disable trigger and RTS (re-enabled in start_rx())
                SERClearRTS(psBlockDesc->ui32Base);
                DummyWrite(psBlockDesc->ui32Base);

                SERDisableRxTrig(psBlockDesc->ui32Base);
                DummyWrite(psBlockDesc->ui32Base);
            }
            break;

        case(MODEM_STATUS_CHANGE):
#ifdef DEBUG
DRV_MODEM++;
#endif
            //CTS has changed state
            //Update local record of CTS
            statePtr->cts = SERReadCTS(psBlockDesc->ui32Base);
            DummyWrite(psBlockDesc->ui32Base);

            //Start transmitting data:
            //iff CTS is true
            //AND data is queued
            //AND FIFO is empty
            if ((statePtr->cts) &&
                (statePtr->tx_current != NULL) &&
                (SERTxFifoEmpty(psBlockDesc->ui32Base)))
            {
#ifdef DEBUG
DRV_MODEM+=0x1000;
#endif
                //CTS is set, so transmit more data if necessary
                num_bytes_left = (statePtr->tx_current->counter - statePtr->tx_byte_count);

                if (num_bytes_left > 0)
                {
                    //Assign local dataPtr to ioPars->pointer to access TX data
                    dataPtr = (unsigned char *)(statePtr->tx_current->pointer);

                    //Fill transmit fifo
                    num_transmitted = FillTXFifo(psBlockDesc->ui32Base, (dataPtr + statePtr->tx_byte_count), num_bytes_left);

                    //Update status
                    statePtr->tx_byte_count += num_transmitted;
                }

                if ((num_transmitted == num_bytes_left) || (num_bytes_left == 0))
                {
#ifdef DEBUG
DRV_MODEM+=0x10000;
#endif
                    //We have transmitted all the data - notify QIO and invalidate tx_current
                    statePtr->tx_current = NULL;
                    QIO_complete(statePtr->tx_dev, QIO_IOCOMPLETE);
                    QIO_start(statePtr->tx_dev);
                }
            }
            break;

        case(NONE):
#ifdef DEBUG
DRV_NONE++;
#endif
        default:
#ifdef DEBUG
DRV_ERR++;
#endif
            //Unexpected behaviour...
            break;
    }

    //Disable interrupt enable register
    interruptReg = READ(&regPtr->interrupt_en);
    DummyWrite(psBlockDesc->ui32Base);

    WRITE(&regPtr->interrupt_en, 0);
    DummyWrite(psBlockDesc->ui32Base);

    if ((statePtr->rx_current != NULL) || (statePtr->tx_current != NULL))
    {
        //Interrupt enable register is cleared at start of ISR and re_enabled at the end (here).
        //This generates the necessary Meta trigger if another interrupt is pending (i.e. if it
        //occurs while the isr is running).
        WRITE(&regPtr->interrupt_en, interruptReg);
        DummyWrite(psBlockDesc->ui32Base);
    }

    return;
}


/*
**  FUNCTION:       cancel_tx
**
**  DESCRIPTION:    QIO cancel function for the TX driver
**
**  INPUTS:         *dev : QIO device descriptor
**
**  OUTPUTS:        None
*/
static void cancel_tx(QIO_DEVICE_T *dev)
{
    UART_STATE_T *statePtr = (UART_STATE_T *)g_apsUARTBlock[ dev->id ]->pvAPIContext;

    statePtr->tx_current = NULL;
}


/*
**  FUNCTION:       cancel_rx
**
**  DESCRIPTION:    QIO cancel function for the RX driver
**
**  INPUTS:         *dev : QIO device descriptor
**
**  OUTPUTS:        None
*/
static void cancel_rx(QIO_DEVICE_T *dev)
{
    UART_STATE_T *statePtr = (UART_STATE_T *)g_apsUARTBlock[ dev->id ]->pvAPIContext;

    statePtr->rx_current = NULL;
}


/* -------------------------- EXPORTED FUNCTIONS ------------------------- */

/*!
*******************************************************************************

 @Function              @UARTConfig

 <b>Description:</b>\n
 This function configures the UART port. It is passed a set of parameters, as
 contained within a ::UART_PARA_TYPE structure. It uses these to set the
 equivalent parameters in the static 'state' variable.

 \param     *port               Pointer to port descriptor.
 \param     *parameters         Parameters to configure.

 \return                        This function returns as follows:\n
                                ::SER_OK        Port configured successfully.\n
                                ::SER_ERR_PORT  Unable to configure port.\n

*******************************************************************************/
unsigned long UARTConfig(unsigned char port, UART_PARA_TYPE *parameters)
{
    unsigned short		div;
    int  				lock;
	UART_STATE_T	*	psState = (UART_STATE_T *)g_apsUARTBlock[ port ]->pvAPIContext;

    //Decode baud rate divisor
    div = parameters->divisor;

    TBI_CRITON(lock);

    //Store parameters
    psState->divisorMsb = ((div >> 8) & 0xFF);
    psState->divisorLsb = (div & 0xFF);
    psState->wordFmt    = parameters->wordFormat;
    psState->rxThresh   = parameters->rxTrigThreshold;
    psState->ctsEnabled = parameters->flowControl;
    psState->inUse      = 1;

    TBI_CRITOFF(lock);

    return SER_OK;
}

/*!
*******************************************************************************

 @Function              @UARTResetError

 <b>Description:</b>\n
 This function resets the driver error flag and flushes the hardware receiver FIFO.

 \param     *port               Pointer to port descriptor.

 \return                        This function returns as follows:\n
                                ::SER_OK        Error resets successfully.\n

*******************************************************************************/
unsigned long UARTResetError(unsigned char port)
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsUARTBlock[ port ];
	UART_STATE_T				*	psState		= (UART_STATE_T *)psBlockDesc->pvAPIContext;
    int  lock;
	SER_REG_T *regPtr;

    regPtr = (SER_REG_T *)psBlockDesc->ui32Base;

	//Disable RTS
	SERClearRTS(psBlockDesc->ui32Base);
	DummyWrite(psBlockDesc->ui32Base);

    //Clear hardware receiver FIFO by setting again the FIFO Control Register
    WRITE(&regPtr->fcr_iid, psState->fifoCtrlReg);
    DummyWrite(psBlockDesc->ui32Base);

	//read LSR register to reset register
	SERReadLSR(psBlockDesc->ui32Base);
	DummyWrite(psBlockDesc->ui32Base);

	//read RBR register to clear last data remaining
	SERRxData(psBlockDesc->ui32Base);
	DummyWrite(psBlockDesc->ui32Base);

    TBI_CRITON(lock);

    //Reset error flag
    psState->errorFlag = FALSE;

    TBI_CRITOFF(lock);

    return SER_OK;
}
