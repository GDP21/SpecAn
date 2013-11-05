/*!
******************************************************************************

 @file msi_tuner.h

 @brief MSI tuner driver

 @Author Ensigma

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for MSI tuner driver.

******************************************************************************/
/*! \mainpage Mirics tuner driver
*******************************************************************************
 \section intro Introduction

 This is a driver for Mirics multi-standard tuner (MSI001 and MSI002)

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef MSI_TUNER_H
/* @cond DONT_DOXYGEN */
#define MSI_TUNER_H

#include "uccrt.h"
/* @endcond */

/*! Output of tuner is about an IF Frequency of 0Hz */
#define MSI_ZEROIF_IF_FREQ          (0)
/*! Output of tuner is about an IF Frequency of 450kHz */
#define MSI_LOWIF_450kHz_IF_FREQ    (450000)
/*! Output of tuner is about an IF Frequency of 1620kHz */
#define MSI_LOWIF_1620kHz_IF_FREQ   (1620000)
/*! Output of tuner is about an IF Frequency of 2048kHz */
#define MSI_LOWIF_2048kHz_IF_FREQ   (2048000)

/*! The array of bytes to send to the tuner is at most this size */
#define MSI_RF_CONTROL_SIZE			4

/*!
*******************************************************************************
 Supported DVBH modes.
 Identifying the modulation mode is use will all the tuner driver to optimise
 its operation for that modulation scheme.
*******************************************************************************/
typedef enum msi_dvbh_mode_t
{
    /*! DVB-H operating in QPSK mode */
    MSI_DVBH_QPSK = 0,
    /*! DVB-H operating in 16 QAM mode */
    MSI_DVBH_16QAM,
    /*! DVB-H operating in 64 QAM mode */
    MSI_DVBH_64QAM
} MSI_DVBH_MODE_T;


/*!
*******************************************************************************
 MSI tuner driver configuration.
 Various aspect of the driver that can be configured at run time.
*******************************************************************************/
typedef struct
{
	/*! Space in the bulk ram that the driver can use to hold messages that
		get DMAed to peripherals */
	unsigned char *bulkRamArray;
	/*! Channel number of the DMAC that the tuner driver can assign to peripheral driver */
	int	dmacChannel;
	/*! Serial port number. Note the SPI counts from port 0 whereas the I2C counts from 1.
	    Hence with SPI port numbers 0, 1 and 2 corresponds to SPIM_DEVICE0, SPIM_DEVICE1 and
	    SPIM_DEVICE2 respectively.
	    With SCBM port numbers 1 and 2 correspond to SCBM_PORT_1 and SCBM_PORT_2.
	*/
	int portNumber;
	/*! Factor by which to ignore AGC updates.
		The implements an only update 1 in N on the AGC.
	*/
	int agcUpdateDecimateFactor;
}MSI_CONFIG_T;

/*! Size in bytes of the workspace that needs to be allocated for this tuner driver. */
#define MSI_TUNER_DRIVER_WORKSPACE_SIZE	4800

/*! Exported tuner control structure for MSI tuner driver */
extern TDEV_CONFIG_T MSITuner;

#endif
