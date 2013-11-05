/*!
******************************************************************************

 @file fpga_config.h

 @brief FPGA SOC config

 @Author Ensigma

	<b>Copyright (C) 2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for FPGA SOC configuration driver

******************************************************************************/
#ifndef FPGA_CONFIG_H
#define FPGA_CONFIG_H



/*!
*******************************************************************************
  Structure used to control the configuration of the UCC ECP TSO interface
  (Transport Stream Output) block. For a more detailed decription see the
  UCC ECP Technical Reference Manual and FPGA manual.
*******************************************************************************/
typedef struct
{
        unsigned long dataOrder;        /*!< Selects data order of the current byte in bitstream mode and endianism in bytestream mode.\n\n
                                             Byte mode:\n
                                             0: TS_DATA(7:0) = output byte (7:0);\n
                                             1: TS_DATA(7:0) = output byte (0:7);\n
                                             Bit mode:\n
                                             0: TS_DATA(0) = output order 7 down to 0\n
                                             1: TS_DATA(0) = output order 0 up to 7
                                        */
        unsigned long clockGate;        /*!< Used to gate TS_CLK, the output transport stream clock, with the TS_VALID signal.\n
                                             0: Gate clock; clock turned off when TS_VALID is de-asserted\n
                                             1: Do not Gate clock; output data clock is turned on at all times\n
                                            Note that when operating in bitstream mode (see ::streamSelect) this bit must be set to 0 to gate TS_CLK.
                                        */
        unsigned long clockInvert;      /*!< Used to set the active polarity of the Transport Stream clock.\n
                                             0: Data is clocked out on TS_CLK positive edge\n
                                             1: Data is clocked out on TS_CLK negative edge
                                        */
        unsigned long validInvert;      /*!< Used to set the active polarity of TS_VALID.\n
                                            0: TS_VALID is active high\n
                                            1: TS_VALID is active low
                                        */
        unsigned long errorInvert;      /*!< Used to set the active polarity of TS_ERR.\n
                                            0: TS_ERR is active high\n
                                            1: TS_ERR is active low
                                        */
        unsigned long streamSelect;     /*!< Used to select between a bytestream output and a bitstream output on the Transport Stream output bus.\n
                                            When bitstream output is selected, the bitstream is presented on the TS_DATA(0) pin.\n
                                            When using bitstream output the clock must be gated (see ::clockGate).\n
                                            0: Bytestream output\n
                                            1: Bitstream output
                                        */
} TSO_FPGA_CONFIG_T;


/*!
*******************************************************************************

 @Function              @FPGAConfig

 <b>Description:</b>\n
 This function configures SOC specific aspects of the system at startup.

*******************************************************************************/
void FPGAConfig(void);

/*!
*******************************************************************************

 @Function              @FPGA_TSOConfig

 <b>Description:</b>\n
 This function configures the TSO interface in the FPGA that passes the data
 onto the USB.

*******************************************************************************/
void FPGA_TSOConfig(const TSO_FPGA_CONFIG_T *config);

#endif
