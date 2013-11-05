/*
** FILE NAME:   TUNER_fnx2.h       --(exported header)--
**
** TITLE:       Tuner interface for DVB-H / DVB-T UCC based COFDM PHY
**
** AUTHOR:      Peter Cheung, Futurewave (HK)
**
** DESCRIPTION: tuner FNX2 with MeOS usage
**
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
**
*/

extern TUNER_CONTROL_T fnx2Tuner_dvbh;


#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#define FINAL_IF    0		//ZIF
#define SIGFORMAT	TRUE  // Complex IQ
#define FNX2_I2CADDR	0x30  

#define RF_CONTROL_SIZE 3	//buffer to W/R via I2C is 3 byte long 

#define FW_ACTIVE 	0
#define FW_STBY   	1
#define FW_PEN1		2
#define FW_PEN2		3

#define XENIF_GPIO_0	0

//=======================================================================================================================
// Fnx2 Register address
#define FNX2_REG_BANDMODE				0x01
#define FNX2_REG_SYNTH_F_MSB		0x02
#define FNX2_REG_SYNTH_F_LSB		0x03
#define FNX2_REG_SYNTH_INT			0x04
#define FNX2_REG_DIV_CTRL				0x05
#define FNX2_REG_DIG_CTRL				0x06
#define FNX2_REG_RFAGC_SETUP		0x07
#define FNX2_REG_RFAGC_VAL			0x08
#define FNX2_REG_BBAGC_VAL			0x09
#define FNX2_REG_ANA_CTRL				0x0A
#define FNX2_REG_PEN1_SETUP			0x0B
#define FNX2_REG_PEN2_SETUP			0x0C
#define FNX2_REG_STATUS					0x0D
#define FNX2_REG_ZIF_STARTUP		0x0E
#define FNX2_REG_SPARE					0x0F
#define FNX2_REG_TEST_PDN3			0x10
#define FNX2_REG_TEST_PDN2			0x11
#define FNX2_REG_TEST_PDN1			0x12
#define FNX2_REG_TEST_PDN0			0x13
#define FNX2_REG_TESTMODE_SETUP	0x14
#define FNX2_REG_TESTMUX_SETUP	0x15
#define FNX2_REG_AGC_TST				0x16
#define FNX2_REG_VCO_CTRL3			0x17
#define FNX2_REG_VCO_CTRL2			0x18
#define FNX2_REG_VCO_CTRL1			0x19
#define FNX2_REG_VCO_CTRL0			0x1A
#define FNX2_REG_CAT_SETUP			0x1B
#define FNX2_REG_CAT_CTRL				0x1C
#define FNX2_REG_FILTERTUNE_MSB 0x1D
#define FNX2_REG_FILTERTUNE_LSB	0x1E
#define FNX2_REG_I2CAT_CTRL			0x1F

//=======================================================================================================================
// Register 0x01 Band Mode setting
#define FNX2_BAND3		0
#define FNX2_BAND45		1
#define FNX2_BANDL		2
#define FNX2_BAND2		3
#define FNX2_BAND_SEL	(FNX2_BAND3)		// User define option
#define FNX2_BAND_SEL_START	6
#define FNX2_BAND_SEL_END		7

#define FNX2_LBAND_1	0
#define FNX2_LBAND_2	1
#define FNX2_LBII	(FNX2_LBAND_1)
#define FNX2_LBII_START	5
#define FNX2_LBII_END	5

#define FNX2_DVBISDB_BW_8	0
#define FNX2_DVBISDB_BW_7	1
#define FNX2_DVBISDB_BW_6	2
#define FNX2_DVBISDB_BW_5	3
#define FNX2_DVBISDB_BW	(FNX2_DVBISDB_BW_8)
#define FNX2_DVBISDB_BW_START	3
#define FNX2_DVBISDB_BW_END		4

#define FNX2_SEL_STANDARD_DABTDMB		0
#define FNX2_SEL_STANDARD_DVBTH			1
#define FNX2_SEL_STANDARD_WSPACE		2
#define FNX2_SEL_STANDARD_ISDBT_13	3
#define FNX2_SEL_STANDARD_ISDBT_3		4
#define FNX2_SEL_STANDARD_ISDBT_1		5
#define FNX2_SEL_STANDARD_FMMODE1		6
#define FNX2_SEL_STANDARD_FMMODE2		7
#define FNX2_SEL_STANDARD (FNX2_SEL_STANDARD_DABTDMB)
#define FNX2_SEL_STANDARD_START		0
#define FNX2_SEL_STANDARD_END		2

//Register 0x05 Divider Control
#define FNX2_DIVN_9		0
#define FNX2_DIVN_1		1
#define FNX2_DIVN_4		2
#define FNX2_DIVN_5		3
#define FNX2_DIVN_6		4
#define FNX2_DIVN_7		5
#define FNX2_DIVN_8		6
#define FNX2_DIVN_RESERVE		7
#define FNX2_DIVN	(FNX2_DIVN_9)
#define FNX2_DIVN_START	5
#define FNX2_DIVN_END		7

#define FNX2_DIVP_3		0
#define FNX2_DIVP_15	1
#define FNX2_DIVP_2		2
#define FNX2_DIVP_25	3
#define FNX2_DIVP_1		4
#define FNX2_DIVP_35	5
#define FNX2_DIVP_4		6
#define FNX2_DIVP	(FNX2_DIVP_3)
#define FNX2_DIVP_START	2
#define FNX2_DIVP_END		4

// XO Clock Frequency
#define FNX2_XO_24576	0		// Divide Ratio = 3
#define FNX2_XO_13000	1		// Divide Ratio = 1.5
#define FNX2_XO_16384	2		// Divide Ratio = 2
#define FNX2_XO_19200	3		// Divide Ratio = 2.5
#define	FNX2_XO_6_10	4		// Divide Ratio = 1
#define FNX2_XO_26000	5		// Divide Ratio = 3.5
#define FNX2_XO_36571	6		// Divide Ratio = 4
#define FNX2_XO	(FNX2_XO_24576)
#define FNX2_XO_START	2
#define FNX2_XO_END		4


#define FNX2_B45LNA_TANK_FREQ0	0		// LNA_TANK_TRIM_SET1=510 LAN_TANK_TRIM_SET2=600
#define FNX2_B45LNA_TANK_FREQ1	1		// LNA_TANK_TRIM_SET1=660 LAN_TANK_TRIM_SET2=910
#define FNX2_B45LNA_TANK_FREQ2	2		// LNA_TANK_TRIM_SET1=600 LAN_TANK_TRIM_SET2=770
#define FNX2_B45LNA_TANK_FREQ3	3		// LNA_TANK_TRIM_SET1=605 LAN_TANK_TRIM_SET2=780
#define FNX2_B45LNA_TANK (FNX2_B45LNA_TANK_FREQ0)
#define FNX2_B45LNA_TANK_START	0
#define FNX2_B45LNA_TANK_END		1

//Register 0x06 Operation of Digital subsystem
#define FNX2_GPIO_DIRECT		0
#define FNX2_GPIO_MAPGPIO2	1		// EN_GPIO=1, GPIO2=0 PLL_LOCK=pin state, GPIO2=1 PK_DET=pin state,
#define FNX2_GPIO_MAPGPIO1	2		// LNA mode EN_GPIO=1 GPIO1=0, pin state 0=low gain 1=high gain
#define FNX2_GPIO_MAPBOTH		3
#define FNX2_GPIO_CTRL (FNX2_GPIO_DIRECT)
#define FNX2_GPIO_CTRL_START	6
#define FNX2_GPIO_CTRL_END		7

#define FNX2_RST_NORMAL 		0
#define FNX2_RST_FULL_RESET	1
#define FNX2_RST_ALL	(FNX2_RST_NORMAL)
#define FNX2_RST_ALL_START	5
#define FNX2_RST_ALL_END		5

#define FNX2_XTALON_DIGINPUT		0		// Clk source = external or xtal
#define FNX2_XTALOFF_DIGINPUT		1		// Clk source = external
#define FNX2_XTALON_DIGOUT_MAIN	2		// Clk source = xtal
#define FNX2_XTALON_DIGIOUT_REF	3		// Clk source = xtal
#define FNX2_REFCLK_CTRL (FNX2_XTALON_DIGINPUT)
#define FNX2_REFCLK_CTRL_START	3
#define FNX2_REFCLK_CTRL_END		4

#define FNX_GPIO1_HIGH	1
#define FNX_GPIO1_LOW		0
#define FNX_GPIO1_START	1
#define FNX_GPIO1_END		1

#define FNX_GPIO2_HIGH	1
#define FNX_GPIO2_LOW		0
#define FNX_GPIO2_START	2
#define FNX_GPIO2_END		2

#define FNX2_EN_GPO_ENABLE	1
#define FNX2_EN_GPO_DISABLE	0
#define FNX2_EN_GPO_START		0
#define FNX2_EN_GPO_END			0

//Register 0x07 RF AGC Setup
#define FNX2_RFAGC_AUTO_PIN		0
#define FNX2_RFAGC_MAN_PIN		1
#define FNX2_RFAGC_AUTO_I2C		2
#define FNX2_RFAGC_MAN_I2C		3
#define FNX2_RFAGC_OPMODE (FNX2_RFAGC_AUTO_PIN)
#define FNX2_RFAGC_OPMODE_START	6
#define FNX2_RFAGC_OPMODE_END		7

#define FNX2_LNA_GAIN_AUTO		0
#define FNX2_LNA_GAIN_HIGH		2	// I2C control RF AGC must set 2 or 3
#define FNX2_LNA_GAIN_LOW			3
#define FNX2_LNA_GAINMODE (FNX2_LNA_GAIN_AUTO)
#define FNX2_LNA_GAINMODE_START	4
#define FNX2_LNA_GAINMODE_END		5

#define FNX2_TIMEOUT_1_5		0		// Timeout period (ms) for slow decay/fast attack RF AGC system
#define FNX2_TIMEOUT_0_25		1
#define FNX2_TIMEOUT_1			2
#define FNX2_TIMEOUT_10			3
#define FNX2_TIMEOUT_50			4
#define FNX2_TIMEOUT_100		5
#define FNX2_TIMEOUT_500		6
#define FNX2_TIMEOUT_1000		7
#define FNX2_TIMEOUT (FNX2_TIMEOUT_1_5)
#define FNX2_TIMEOUT_START	1
#define FNX2_TIMEOUT_END		3

#define FNX2_SET_LNA_GAIN			0		// To choose which one to be updated in RFAGC_VAL registers
#define FNX2_SET_AMP1_GAIN		1
#define FNX2_SEL_AMP1 (FNX2_SET_LNA_GAIN)
#define FNX2_SEL_AMP1_START	0
#define FNX2_SEL_AMP1_END	0

//Register 0x0A LPF Tuning
#define FNX2_NO_TUNE					0
#define FNX2_PERFORM_AUTOTUNE	1
#define FNX2_TUNE_FILTER	(FNX2_NO_TUNE)
#define FNX2_TUNE_FILTER_START	7
#define FNX2_TUNE_FILTER_END		7

#define FNX2_INTERNAL_LUT		0		// default
#define FNX2_CURRENT_REG		1	
#define FNX2_IGNORE_TUNE_LUT	(FNX2_INTERNAL_LUT)
#define FNX2_IGNORE_TUNE_LUT_START	6
#define FNX2_IGNORE_TUNE_LUT_END		6

#define FNX2_POWER_OFF	1		// Need Standby pin to re-start
#define FNX2_POWER_ON		0
#define FNX2_POWER_OFF_START	5
#define FNX2_POWER_OFF_END	5

#define FNX2_XO_BIAS_MAX_CURRENT			0
#define FNX2_XO_BIAS_REDUCED_CURRENT1	1
#define FNX2_XO_BIAS_REDUCED_CURRENT2	2
#define FNX2_XO_BIAS_MIN_CURRENT			3
#define FNX2_XO_BIAS_SEL (FNX2_XO_BIAS_MAX_CURRENT)
#define FNX2_XO_BIAS_SEL_START	3
#define FNX2_XO_BIAS_SEL_END		4

#define FNX2_BBAGC_I2C_ASYNC	0
#define FNX2_BBAGC_I2C_TIMED	1
#define FNX2_BBAGC_PIN_ASYNC	2
#define FNX2_BBAGC_MODE	(FNX2_BBAGC_I2C_ASYNC)
#define FNX2_BBAGC_MODE_START	1
#define FNX2_BBAGC_MODE_END		2

#define FNX2_LNA_TANKTRIM_SET1	0
#define FNX2_LNA_TANKTRIM_SET2	1
#define FNX2_LNA_TANK_TRIM	(FNX2_LNA_TANKTRIM_SET1)
#define FNX2_LNA_TANK_TRIM_START	0
#define FNX2_LNA_TANK_TRIM_END	1

#define FNX2_REG_START_BIT		0
#define FNX2_REG_END_BIT			7

#define FNX2_STANDBY_ENABLE			1
#define FNX2_STANDBY_DISABLE		0

//=======================================================================================================================
#define RFPOWER_MEAN_NORMAL			(8192)
#define RFPOWER_MEAN_SATURATED		(10600)
#define RFPOWER_MEAN_TOO_LOUD		(9600)
#define RFPOWER_MEAN_UP_LIMIT		(9100)
#define RFPOWER_MEAN_LOW_LIMIT		(7100)
#define RFPOWER_MEAN_TOO_QUIET		(6600)
#define RFPOWER_MEAN_UP_SILENCE		(5600)

#define RFPOWER_NUMOF_SAMPLE		(16)  // Try to increase the sample size for more stable operation

#define BBAGC_GAIN_MIN	(0)
#define BBAGC_GAIN_MAX	(255)
#define BBAGC_GAIN_INIT	(10)	


//used in process tuner command function "FNX2CmdProcess"
#define FNX2_TUNER_INIT				0
#define FNX2_TUNER_CONFIG			1
#define FNX2_TUNER_INITIFAGC		2
#define FNX2_TUNER_SETFREQ			3
#define FNX2_TUNER_POWERUP			4
#define FNX2_TUNER_POWERDOWN		5
#define FNX2_TUNER_SETIFAGC			6
#define FNX2_TUNER_POWERSAVE		7
#define FNX2_TUNER_GETRFPOWER		8
#define FNX2_TUNER_SETIFAGCTC 		9

//=======================================================================================================================


