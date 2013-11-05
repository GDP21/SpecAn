/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/fnx2/TUNER_fnx2.c,v $
**
** TITLE:       Tuner interface for DVB-H / DVB-T UCC based COFDM PHY for tuner FNX2
**
** AUTHOR:      Peter Cheung, Futurewave (HK)
**
** DESCRIPTION: tuner FNX2 with MeOS usage
**
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "PHY_tuner.h"
#include "TUNER_fnx2.h"

////////////////BUILD OPTIONS for FNX2 tuner driver //////
#define BBAGC_FIX	 	//COMMENT IN if fix Tx power level, BBAGC of FNX2 is fix from harness.c input
						//COMMENT OUT if BBAGC of FNX2 is controlled by AGC loop based on #ADC threshold
//#define FNX2_DEBUG		//COMMENT IN if 32 register of FNX2 is logged in global variable "g_spy_buffer"
						//COMMENT OUT if 32 register of FNX2 is NOT logged
#define FNX2_AUTOFILTERBUG	//COMMENT IN if FNX2 autofilter bug fix is used
							//COMMENT OUT if FNX2 autofilter bug fix is NOT used
///////////////////////////////////////////////////
//peter 10Nov2008 
//add xenif related header files, MeOs related and test harness files
#include "common_rf_cont.h"
#include "gpio_api.h"
#include "MeOS.h"
#ifdef BBAGC_FIX
// GJD Removed this. Just use a fixed value.
//#include "harness.h"  //peter 17Nov2008 //used input arguemnt from codescape script
//extern DBG_TEST_T dbgTest;   	//define in harness.c //peter 17Nov2008
#else
static unsigned char g_fnx2_BBAgc_current;
#endif
/////////////local variables////////peter 10Nov2008////////////////////////
KRN_TASKQ_T g_Fnx2Queue;	
//////define GPIO variables//////
typedef struct
{
	int		i32Handle;
	int		i32Pin;
	int		i32Mode;
	int		i32InitLevel;
	int		i32Pullup;
	int		i32IST;
} FNX2_GPIO_CONFIG_T;
FNX2_GPIO_CONFIG_T		g_Fnx2_standby_pin = { 0, XENIF_GPIO_0, 1, 0, 1, 0 }; //XENIF_GPIO_0=0
////////define tuner command variables///////
typedef struct
{
	LST_LINK;
	unsigned char state;
	long data1;
	unsigned char buff[10];
	TUNER_COMPLETION_FUNC_T pCompFunction;
}	FNX2_TUNER_CMD_T;
static FNX2_TUNER_CMD_T	FNX2_tuner_cmd;
#define FNX2_TUNER_STACKSIZE (256)
KRN_TASK_T fnx2_tuner_tcb;
unsigned int fnx2_tuner_stack[FNX2_TUNER_STACKSIZE];
KRN_MAILBOX_T Fnx2Tunermbox;
KRN_SEMAPHORE_T Fnx2Tuner_sem;
///////////////////////////////////////////
unsigned char g_fnx2_xo=0;
unsigned char g_fnx2_PowerSaveMode=0;
unsigned char g_fnx2_I2CSlaveAddr=0;
long g_fnx2_threshI_total=0;
unsigned char g_fnx2_agc_counter=0;
#define BUFFER_LEN	(10)
unsigned char g_fnx2_buffer[BUFFER_LEN];
unsigned char rfCmdArray[RF_CONTROL_SIZE]; //define I2C data in MTX core RAM
//variable to store previous BW [Hz] and freq [Hz] to avoid sending same I2C command
long prev_freqHz=0;
long prev_bwHz=0;
unsigned char prev_powerup=0;  //=0 (FNX2 currently power down), =1 (FNX2 currently power up)

#ifdef FNX2_DEBUG
unsigned char g_spy_buffer[32];			// for debug purpose ONLY
#endif


////////////tuner function prototypes/////////////peter 10Nov2008////////////////////////
static PHY_TUNER_RETURN_T   fnx2Tuner_Initialise(TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_Configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_tune(long frequencyHz, TUNER_COMPLETION_FUNC_T);
static long                 fnx2Tuner_readRFPower(TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_powerUp(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_powerDown(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_powerSave(PHY_RF_PWRSAV_T, unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_setIFAGCTimeConstant(long timeConstuS, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_setAGC(TUNER_AGCISR_HELPER_T *, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_initAGC(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T   fnx2Tuner_shutdown(TUNER_COMPLETION_FUNC_T);
//////////local function prototypes ///////////peter 10Nov2008//////////////
short FNX2_Reg_Mask(short startBit, short endBit);
unsigned char FNX2_I2C_read(unsigned char reg, short startBit, short endBit);
void FNX2_I2C_write(unsigned char reg, short startBit, short endBit, unsigned char data);
void FNX2SetPowerMode(unsigned char powersavemode);
void FNX2SendCommand(FNX2_TUNER_CMD_T *cmd,unsigned char cmd_type,long data1,unsigned char *buffer,TUNER_COMPLETION_FUNC_T pCompletionFunc);
static void FNX2TunerTask(void);
static void FNX2CmdProcess(FNX2_TUNER_CMD_T *tuner_command);
#ifndef BBAGC_FIX
unsigned char FNX2_GetHW_gain(long thresh_total);
#endif
////////////////////////////////////////////////////



TUNER_CONTROL_T fnx2Tuner_dvbh = {
    PHY_TUNER_VERSION_I32,  /* Version number - check that tuner and API are built with the same header */
    1,              /* The IF interface is complex baseband */
    0,              /* The final IF frequency of the tuner (Hz) */
    1,              /* Set true if the IF spectrum is inverted */
    166000,         /* The stepsize of the tuner PLL (Hz)      */
    166000,         /* The tuner PLL update margin (Hz)        */
    0,              /* Settling time in uS from power down to power up */
    0,              /* Settling time in uS from power save level1 to power up */
    0,              /* Settling time in uS from power save level2 to power up */
    0,              /* Settling time in uS from tune to stable output  */
    fnx2Tuner_Initialise,
    fnx2Tuner_Configure,
    fnx2Tuner_tune,
    fnx2Tuner_readRFPower,
    fnx2Tuner_powerUp,
    fnx2Tuner_powerDown,
    fnx2Tuner_powerSave,
    fnx2Tuner_setIFAGCTimeConstant,
    fnx2Tuner_setAGC,
    fnx2Tuner_initAGC,
    fnx2Tuner_shutdown,
    0,          /* No standard specific info */
    0           /* No tuner specific info */
};

////////////local function/////////////peter 10Nov2008////////////////////////
//=====================================================================================================================
/*
** FUNCTION:    FNX2_GetHW_gain
**
** DESCRIPTION: To provide gain decision based on the accumulated, averaged threshold
**
** RETURNS:     decision
**
*/
#ifndef BBAGC_FIX
unsigned char FNX2_GetHW_gain(long thresh_total)
{
		unsigned char decision=0;

		if (thresh_total > RFPOWER_MEAN_SATURATED)
				decision = 6;
		else if (thresh_total >= RFPOWER_MEAN_TOO_LOUD)
				decision = 5;
		else if (thresh_total >= RFPOWER_MEAN_UP_LIMIT)
				decision = 4;
		else if (thresh_total <= RFPOWER_MEAN_UP_SILENCE)
				decision = 3;
		else if (thresh_total <= RFPOWER_MEAN_TOO_QUIET)
				decision = 2;
		else if (thresh_total <= RFPOWER_MEAN_LOW_LIMIT)
				decision = 1;
		else
				decision = 0;

		return (decision);
}
#endif
//=====================================================================================================================
/*
** FUNCTION:    FNX2TunerTask
**
** DESCRIPTION: To handle all tuner control activities
**
**
** RETURNS:
**
*/

static void FNX2TunerTask(void)
{		
    FNX2_TUNER_CMD_T *tuner_cmd;    
    for(;;)
    {
    	KRN_testSemaphore(&Fnx2Tuner_sem, 1, KRN_INFWAIT);
       	if ((tuner_cmd = (FNX2_TUNER_CMD_T *)KRN_getMbox(&Fnx2Tunermbox, KRN_NOWAIT)) != NULL)
        {        		
        		FNX2CmdProcess(tuner_cmd);
        }
    }
}
//=======================================================================================================================
/*
** FUNCTION:    FNX2_Reg_Mask
**
** DESCRIPTION: 
** 				
** RETURNS:     
**
*/
short FNX2_Reg_Mask(short startBit, short endBit)
{
	short il;
	short result=0;

	if(startBit > 7)
		return 0;

	if(startBit < 0)
		startBit = 0;

	if(endBit > 7)
		endBit = 7;

	if(startBit > endBit)
		return 0;

	for(il = startBit; il <= endBit; il++)
		result |= (1 << il);

	return result;
}

//=================================================================================================================
/*
** FUNCTION:    FNX2SetPowerMode
**
** DESCRIPTION: 
** 				
** RETURNS:    
**
*/
void FNX2SetPowerMode(unsigned char powersavemode)
{
	switch(powersavemode)
  	{
		case FW_ACTIVE:
			g_fnx2_PowerSaveMode = 0;
  	  		break;
  	 	case FW_STBY:
  	 		g_fnx2_PowerSaveMode = 0x80;
  	 		break;
  	 	case FW_PEN1:
  	 		g_fnx2_PowerSaveMode = 0xA0;
  	 		break;
  	 	case FW_PEN2:
  	 		g_fnx2_PowerSaveMode = 0xC0;
  	 		break;
  		default:
  	  		__TBILogF("ERROR:  power saving mode UKNOWN!!\n");
			break;
  	}
	return;
}

//=================================================================================================================
/*
** FUNCTION:    FNX2_I2C_read
**
** DESCRIPTION: 
** 				
**  
**
** RETURNS:     
**
*/
unsigned char FNX2_I2C_read(unsigned char reg, short startBit, short endBit)
{
	unsigned char mask=0;
	unsigned char reg_value=0;

	rfCmdArray[0] = g_fnx2_I2CSlaveAddr;
    rfCmdArray[1] = reg | g_fnx2_PowerSaveMode;
    RFContolMessage(rfCmdArray, RF_CONTROL_SIZE, TRUE);
    reg_value = rfCmdArray[2];

	if((endBit - startBit) != 7)
  	{
    	mask = FNX2_Reg_Mask(startBit, endBit);
    	reg_value = reg_value & ~mask;
    	reg_value = reg_value >> startBit;
  	}

		return (reg_value);
}
//=================================================================================================================
/*
** FUNCTION:    FNX2_I2C_write
**
** DESCRIPTION: 
** 				
** RETURNS:     
**
*/
void FNX2_I2C_write(unsigned char reg, short startBit, short endBit, unsigned char data)
{
  	unsigned char reg_value=0;
  	unsigned char write_data=0;
  	unsigned char mask=0;

  	if((endBit - startBit) != 7)
  	{
				rfCmdArray[0] = g_fnx2_I2CSlaveAddr;
  	  	rfCmdArray[1] = reg | g_fnx2_PowerSaveMode;
  	  	RFContolMessage(rfCmdArray, RF_CONTROL_SIZE, TRUE);
  	  	reg_value = rfCmdArray[2];

  	  	mask = FNX2_Reg_Mask(startBit, endBit);
  	  	data = data << startBit;
  	  	write_data = (reg_value & ~mask) | (data & mask);
  	}
  	else
  	{
  	   	write_data = data;
  	}

  	rfCmdArray[0] = g_fnx2_I2CSlaveAddr;
  	rfCmdArray[1] = reg | g_fnx2_PowerSaveMode;
  	if (reg==0x00)  //used by power saving, so I2C write only 2 byte
  	{
  			RFContolMessage(rfCmdArray, RF_CONTROL_SIZE-1, FALSE);
  	}
  	else
  	{
  			rfCmdArray[2] = write_data;
  			RFContolMessage(rfCmdArray, RF_CONTROL_SIZE, FALSE);
		}
}
//=======================================================================================================================
/*
** FUNCTION:    FNX2SendCommand
**
** DESCRIPTION: To pass the Tuner command to Tuner Task
**
** RETURNS:
**
*/

void FNX2SendCommand(FNX2_TUNER_CMD_T *cmd,unsigned char cmd_type, long data1, unsigned char *buffer,TUNER_COMPLETION_FUNC_T pCompletionFunc)
{		
	unsigned char 	buffer_index;	  	
	
	cmd->state = cmd_type;
	cmd->data1 = data1;	
	for (buffer_index=0; buffer_index<BUFFER_LEN; buffer_index++)
	{
		cmd->buff[buffer_index] = buffer[buffer_index];
	}
	cmd->pCompFunction = pCompletionFunc;
	KRN_putMbox(&Fnx2Tunermbox,cmd);
	KRN_setSemaphore(&Fnx2Tuner_sem,1);         	  
	return;
}	

//=====================================================================================================================
/*
** FUNCTION:    FNX2CmdProcess
**
** DESCRIPTION: To process the tuner command retrived from mail box
**
**
** RETURNS:
**
*/

static void FNX2CmdProcess(FNX2_TUNER_CMD_T *tuner_command)
{
	KRN_TASKQ_T g_Fnx2Queue;
	unsigned char fnx2_reg_value,i;   
	unsigned char CmdProcess_result=PHY_TUNER_FAILURE; 
	unsigned char tune_result=PHY_TUNER_FAILURE;
	
	switch (tuner_command->state)
    {
    	case FNX2_TUNER_INIT:
    		//this is a placeholder ONLY //currently NOT call by any tuner function
    		CmdProcess_result=PHY_TUNER_SUCCESS;
    		break;

    	case FNX2_TUNER_CONFIG:
    		FNX2_I2C_write(FNX2_REG_DIG_CTRL, FNX2_RST_ALL_START, FNX2_RST_ALL_END, FNX2_RST_FULL_RESET);
    		DQ_init(&g_Fnx2Queue);
    		//this re-toggle rising edge of GPIO0 to ensure FNX2 is wakeup from power off
    		//can be removed, since already done in FNX2_TUNER_POWERUP
    		///////////////////////////////////////////////////////////
    		//GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_DISABLE);
  			//GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_ENABLE);
  			//KRN_hibernate(&g_Fnx2Queue, 10);
  			//GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_DISABLE);
  			//KRN_hibernate(&g_Fnx2Queue, 10);
  			//FNX2_I2C_write(FNX2_REG_DIG_CTRL, FNX2_RST_ALL_START, FNX2_RST_ALL_END, FNX2_RST_FULL_RESET);
  			/////////////////////////////////////////////////////////////////  						
  			//bit[2:0]=0x1, select broadcast standard DVB-T/H  
			//bit[4:3]=DVB_ISDN_BW //=3 (5MHz), =2 (6MHz), =1 (7MHz) =0 (8MHz)
			FNX2_I2C_write(0x01,0,4,tuner_command->buff[0]);
  			//setup synthesizer and autofilter
			FNX2_I2C_write(FNX2_REG_DIV_CTRL, FNX2_DIVP_START, FNX2_DIVP_END, FNX2_XO_24576);  //fix XO used on FNX2 to be 24.576MHz for now								  
  			FNX2_I2C_write(FNX2_REG_ANA_CTRL, FNX2_TUNE_FILTER_START, FNX2_TUNE_FILTER_END, FNX2_PERFORM_AUTOTUNE);
  			KRN_hibernate(&g_Fnx2Queue, 3);  //ensure 2.2ms wait time for FNX2 filter selection algorithm to complete
#ifdef FNX2_AUTOFILTERBUG
  			//below R and W of same 3 registers (0x2, 0x3, 0x4) is ONLY for temporarily bug fix in FNX2
  			FNX2_I2C_write(FNX2_REG_ANA_CTRL, FNX2_TUNE_FILTER_START, FNX2_TUNE_FILTER_END, FNX2_NO_TUNE);
  			fnx2_reg_value = FNX2_I2C_read(FNX2_REG_SYNTH_F_MSB,FNX2_REG_START_BIT,FNX2_REG_END_BIT);
  			FNX2_I2C_write(FNX2_REG_SYNTH_F_MSB, FNX2_REG_START_BIT, FNX2_REG_END_BIT, fnx2_reg_value);
			fnx2_reg_value = FNX2_I2C_read(FNX2_REG_SYNTH_F_LSB,FNX2_REG_START_BIT,FNX2_REG_END_BIT);
			FNX2_I2C_write(FNX2_REG_SYNTH_F_LSB, FNX2_REG_START_BIT, FNX2_REG_END_BIT, fnx2_reg_value);
			fnx2_reg_value = FNX2_I2C_read(FNX2_REG_SYNTH_INT,FNX2_REG_START_BIT,FNX2_REG_END_BIT);
			FNX2_I2C_write(FNX2_REG_SYNTH_INT, FNX2_REG_START_BIT, FNX2_REG_END_BIT, fnx2_reg_value);
			FNX2_I2C_write(FNX2_REG_ANA_CTRL, FNX2_TUNE_FILTER_START, FNX2_TUNE_FILTER_END, FNX2_PERFORM_AUTOTUNE);
  			KRN_hibernate(&g_Fnx2Queue, 3);  //ensure 2.2ms wait time for FNX2 filter selection algorithm to complete
			FNX2_I2C_write(FNX2_REG_ANA_CTRL, FNX2_TUNE_FILTER_START, FNX2_TUNE_FILTER_END, FNX2_NO_TUNE);
#endif
#ifdef VERBOSE	
    		__TBILogF("ConfigTunerRF OK: autoTune OK\n");
#endif
    		CmdProcess_result=PHY_TUNER_SUCCESS;
			break;

    	case FNX2_TUNER_INITIFAGC:
    		FNX2_I2C_write(FNX2_REG_BBAGC_VAL,FNX2_REG_START_BIT,FNX2_REG_END_BIT,(unsigned char)tuner_command->data1);    				
    		CmdProcess_result=PHY_TUNER_SUCCESS;
    		break;
    		
    	case FNX2_TUNER_SETFREQ:        
    		FNX2_I2C_write(0x01,6,7, tuner_command->buff[0]);  //BAND_SEL
    		if (tuner_command->buff[0]==FNX2_BAND45)  //UHF band
    		{
	    		FNX2_I2C_write(0x05,0,1,tuner_command->buff[2]);  //B45LNA_TANK
				FNX2_I2C_write(0x0A,0,0,tuner_command->buff[3]);  //LNA_TANK_TRIM
				FNX2_I2C_write(0x14,3,4,tuner_command->buff[4]);  //B45_NA_QTRIM
    		}
    		else
			{
				if (tuner_command->buff[0]==FNX2_BANDL) //L band
					FNX2_I2C_write(0x01,5,5,tuner_command->buff[1]); //LBII_SEL 
			}	
			FNX2_I2C_write(FNX2_REG_DIV_CTRL,FNX2_DIVN_START,FNX2_DIVN_END,tuner_command->buff[5]);    //DIVN_TEST
			FNX2_I2C_write(FNX2_REG_SYNTH_F_MSB,FNX2_REG_START_BIT,FNX2_REG_END_BIT,tuner_command->buff[6]);  //FRA_MSB
			FNX2_I2C_write(FNX2_REG_SYNTH_F_LSB,FNX2_REG_START_BIT,FNX2_REG_END_BIT,tuner_command->buff[7]);  //FRA_LSB
			FNX2_I2C_write(FNX2_REG_SYNTH_INT,FNX2_REG_START_BIT,FNX2_REG_END_BIT,tuner_command->buff[8]);    //REG_INT
#ifdef BBAGC_FIX
			// GJD Just use a fixed value (last argument to function below)
			FNX2_I2C_write(FNX2_REG_BBAGC_VAL, 0,7, 50);  //set fix BBAGC
#endif
#ifdef FNX2_DEBUG
			for (i=0;i<32;i++)			// for debug purpose ONLY
				g_spy_buffer[i]=FNX2_I2C_read(i,FNX2_REG_START_BIT,FNX2_REG_END_BIT);	
#endif
			for (i=0;i<20;i++)
			{										
				fnx2_reg_value = FNX2_I2C_read(FNX2_REG_STATUS,FNX2_REG_START_BIT,FNX2_REG_END_BIT);										
				if (fnx2_reg_value&0x80)										
				{	
#ifdef VERBOSE
					__TBILogF("SUCCESS: PLL_LOCK ok!!\n");	
#endif									
					tune_result = PHY_TUNER_SUCCESS;
					break;																																						
            	}
            }			
            if (tune_result==PHY_TUNER_FAILURE)
            {
            	__TBILogF("ERROR: tuner PLL_LOCK failed!!\n");	
        	}
            CmdProcess_result = tune_result;																							  									  																	
    		break;

    	case FNX2_TUNER_POWERUP:
    		DQ_init(&g_Fnx2Queue);
  			GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_DISABLE);  		
  			KRN_hibernate(&g_Fnx2Queue, 10);	//ensure GPIO pulse last for at least 10ms				  					
    		CmdProcess_result = PHY_TUNER_SUCCESS;	
  			break;

    	case FNX2_TUNER_POWERDOWN:    				
			GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_ENABLE);           			            									
    		CmdProcess_result = PHY_TUNER_SUCCESS;	
			break;

    	case FNX2_TUNER_SETIFAGC:            			            				
    		FNX2_I2C_write(FNX2_REG_BBAGC_VAL,FNX2_REG_START_BIT,FNX2_REG_END_BIT,(unsigned char)tuner_command->data1);    				
    		CmdProcess_result = PHY_TUNER_SUCCESS;	
    		break;

    	case FNX2_TUNER_POWERSAVE:            			
    		if (tuner_command->data1 == 0)  		//max power level	
  				GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_DISABLE);
    		else if (tuner_command->data1 == 1)  	//middle power level		
  				GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_ENABLE);
    		else if (tuner_command->data1 == 2)		//lowest power level
    		{		
    			__TBILogF("ERROR:  power saving level=%d is NOT used for now\n",tuner_command->data1);
				CmdProcess_result = PHY_TUNER_FAILURE;
    		}		
    		else 
    		{		
    			__TBILogF("ERROR:  power saving FAIL with UKNOWN level\n");
    			CmdProcess_result = PHY_TUNER_FAILURE;
    			break;
    		}	  	
    		CmdProcess_result = PHY_TUNER_SUCCESS;		    				    				            			            			    	
    		break;
    		
    	case FNX2_TUNER_GETRFPOWER:    
    		CmdProcess_result = PHY_TUNER_SUCCESS;			
    		break;    		
    	
    	case FNX2_TUNER_SETIFAGCTC:
    		CmdProcess_result = PHY_TUNER_SUCCESS;
    		break;	          
    	
    	default:
    		CmdProcess_result = PHY_TUNER_FAILURE;
    		break;
    }
	tuner_command->pCompFunction(CmdProcess_result);		    					
	return;
}		

//======================================//
//				TUNER FUNCTION          //
//======================================//
/*
** FUNCTION:    Init
**
** DESCRIPTION: Initialises the Tuner. (call once system startup)
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_Initialise(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{

	GPIO_RETURN_T	fnx2_gpio_return;
	KRN_TASKQ_T g_Fnx2Queue;
					  			        
#ifdef VERBOSE
	__TBILogF("InitTuner called -> init I2C, RST/STBY\n");
#endif
	FNX2SetPowerMode(FW_ACTIVE);
	g_fnx2_I2CSlaveAddr=FNX2_I2CADDR;		
    RFContolInit(166000); //fixed system CLK of DVB-H to 166MHz

	GPIOInit();		// Should be already been called in gpioCommand.c
	fnx2_gpio_return = GPIOAddDevice(
	    &(g_Fnx2_standby_pin.i32Handle),
		g_Fnx2_standby_pin.i32Pin,
		g_Fnx2_standby_pin.i32Mode,
		g_Fnx2_standby_pin.i32InitLevel,
		g_Fnx2_standby_pin.i32Pullup);

	if (fnx2_gpio_return != GPIO_OK)
	{
		__TBILogF("FAIL: register gpio for FNX2\n");
		pCompletionFunc(PHY_TUNER_FAILURE);
		return(PHY_TUNER_FAILURE);
	}									
	GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_DISABLE); //enable FNX2
  	GPIOSet(g_Fnx2_standby_pin.i32Handle, FNX2_STANDBY_ENABLE);  //disable FNX2
  	DQ_init(&g_Fnx2Queue);   
  	KRN_hibernate(&g_Fnx2Queue, 10);  //ensure GPIO pulse last for at least 10ms
	KRN_initMbox(&Fnx2Tunermbox);
    KRN_initSemaphore(&Fnx2Tuner_sem, 0);
	KRN_startTask(FNX2TunerTask, &fnx2_tuner_tcb, fnx2_tuner_stack, FNX2_TUNER_STACKSIZE, 1, NULL, "Delay no more tuner task");
		  	  	  	  	  	  			          	      
    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;	
}

//=====================================================================================================================
/*
** FUNCTION:    configTuner
**
** DESCRIPTION: sets the tuner bandwidth. (call when switch channel or reacquiring)
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_Configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	unsigned char BWmode=0;
	
#ifdef VERBOSE
		__TBILogF("Config tuner for DVB (ZIF only), with bandwidth=%d MHz\n",bandwidthHz/1000000);
#endif
	(void) standard;  //peter 12Dec2008  //current standard from tuner API is NOT used 
	if (!(prev_bwHz==bandwidthHz))   //calculated config related register if BW is changed
	{
		prev_bwHz=bandwidthHz;
  		//bit[2:0]=0x1, select broadcast standard DVB-T/H  
		//bit[4:3]=DVB_ISDN_BW //=3 (5MHz), =2 (6MHz), =1 (7MHz) =0 (8MHz)
		switch (bandwidthHz/1000000)
		{
			case 5:
				BWmode = 3;
				break;
			case 6:
				BWmode = 2;
				break;
			case 7:
				BWmode = 1;
				break;
			case 8:
				BWmode = 0;
				break;
			default:
				__TBILogF("ERROR:  BW=%dMHz is unknown!\n",BWmode);
				exit(-1);
				break;
		}	
		g_fnx2_buffer[0]=(BWmode<<3|0x1);  //0x1 to be derived from standard parameter from tuner API LATER (TBD) //peter 12Dec2008
		FNX2SendCommand(&FNX2_tuner_cmd, FNX2_TUNER_CONFIG, bandwidthHz, g_fnx2_buffer, pCompletionFunc);  	
	}
	else
		pCompletionFunc(PHY_TUNER_SUCCESS);
		
    return PHY_TUNER_SUCCESS;
}

//=================================================================================================================
/*
** FUNCTION:    SetFrequency
**
** DESCRIPTION: Send the strings of bytes to the RF, via the SPI to set the carrier Frequency.
** 				(call when switch channel or reacquiring)
**  Tune to a given RF frequency (Hz), called repeatedly and after each power cycle 
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_tune(long frequencyHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	unsigned char REG_INT;
	unsigned char FRA_MSB;
	unsigned char FRA_LSB;
	unsigned char DIVN_TEST=0;
	unsigned long FRA_MAIN;
	double TARGET_VCO;
	double INT_MAIN;	
	unsigned long DIV_OVERALL=0;
	long frequencykHz;
	//variables for band selection
	unsigned char BAND_SEL=0;	
	unsigned char LBII_SEL=0;
	//variables for band IV/V LNA tank freq
	unsigned char B45LNA_TANK=0;
	unsigned char LNA_TANK_TRIM=0;
	unsigned char B45_NA_QTRIM=0;
		
	//note:  DVB-T/H is only for band III, IV, V and L, NOT band II (FM only)
#ifdef VERBOSE
	__TBILogF("SetFrequency called:  tuner set to freq= %dHz\n", frequencyHz);
#endif

	if (!(prev_freqHz==frequencyHz))  //calculated tuning related register if carrier freq is changed
	{
		prev_freqHz=frequencyHz;
		frequencykHz=frequencyHz/1000;
		if ((frequencykHz >= 177000) && (frequencykHz <= 226500))
			BAND_SEL = FNX2_BAND3;
		else if ((frequencykHz >= 472000) && (frequencykHz <= 887200))		
			BAND_SEL = FNX2_BAND45;
		else if ((frequencykHz >= 1662500) && (frequencykHz <= 1682500))  //DVB-H L band is only for US	
		{			
			BAND_SEL = FNX2_BANDL;	
			//set LBII_SEL in case of L band
			if ((frequencykHz>=1450000) && (frequencykHz<=1492000))
				LBII_SEL=0x0;
			else if ((frequencykHz>=1660000) && (frequencykHz<=1685000))
				LBII_SEL=0x1;		
			g_fnx2_buffer[1]=LBII_SEL; 
		}
		g_fnx2_buffer[0]=BAND_SEL;	//bit[7:6]=0 (band III), =1 band IV/V, =2 band L, BAND_SEL
		switch (BAND_SEL)
		{
			case FNX2_BAND3:  //VHF band
				if ((frequencykHz >= 177000) && (frequencykHz < 201000)) 
				//for band 3, since final div=2 (fix), DIVN_TEST=[7, 8, 9], DIV_OVERALL=final div*DIVN_TEST=[14, 16, 18] 
				//BW=8MHz (178~194), BW=7MHz (177.5~198.5), BW=6MHz (177~195) 
				{
					DIVN_TEST = FNX2_DIVN_8;
					DIV_OVERALL = 16;							
				}
				else if ((frequencykHz >= 201000) && (frequencykHz < 226500))  
				//BW=8MHz (202~226), BW=7MHz (205.5~226.5), BW=6MHz (201~213)
				{
					DIVN_TEST = FNX2_DIVN_7;
					DIV_OVERALL = 14;				
				}		
				break;
			case FNX2_BAND45:  //UHF band
				if ((frequencykHz >= 472000) && (frequencykHz < 558000)) 
				//for band 4/5, since final div=1 (fix), DIVN_TEST=[4, 5, 6], DIV_OVERALL=final div*DIVN_TEST=[4, 5, 6] 	
				//BW=8MHz (474~554), BW=7MHz (529.5~557.5), BW=6MHz (473~557)
				//BW=8MHz, n=1 (474.167~554.167), BW=7MHz, n=1 (529.67~557.67), BW=6MHz, n=1 (473.167~557.167)
				//BW=8MHz, n=-1 (473.83~553.833), BW=7MHz, n=-1 (529.33~557.33), BW=6MHz, n=-1 (472.833~556.833)
				{							
					DIVN_TEST = FNX2_DIVN_6;
					DIV_OVERALL = 6;														
				}		
				else if ((frequencykHz >= 558000) && (frequencykHz < 699000)) 
				//BW=8MHz (562~698), BW=7MHz (564.5~697.5), BW=6MHz (563~695)		
				//BW=8MHz, n=1 (562.167~698.167), BW=7MHz, n=1 (564.67~697.67), BW=6MHz, n=1 (563.167~695.167)	
				//BW=8MHz, n=-1 (561.833~697.833), BW=7MHz, n=-1 (564.33~697.33), BW=6MHz, n=-1 (562.833~694.833)
				{	
					DIVN_TEST = FNX2_DIVN_5;
					DIV_OVERALL = 5;														
				}		
				else if ((frequencykHz >= 699000) && (frequencykHz <= 887200))	
				//BW=8MHz (706~858), BW=7MHz (704.5~802.5), BW=6MHz (701~887), 	
				//BW=8MHz, n=1 (706.167~858.167), BW=7MHz, n=1 (704.67~802.67), BW=6MHz, n=1 (701.167~887.167)	
				//BW=8MHz, n=-1 (705.833~857.833), BW=7MHz, n=-1 (704.33~802.33), BW=6MHz, n=-1	(700.833~886.833)				
				{	
					DIVN_TEST = FNX2_DIVN_4;
					DIV_OVERALL = 4;														
				}			
				//set LNA tank freq for band IV/V
				if 	((frequencykHz >=470000) && (frequencykHz<540000))
				{
					B45LNA_TANK = 0;
					LNA_TANK_TRIM = 0;
					B45_NA_QTRIM = 2;
				}
				else if ((frequencykHz >=540000) && (frequencykHz<550000))
				{
					B45LNA_TANK = 2;
					LNA_TANK_TRIM = 0;
					B45_NA_QTRIM = 1;
				}
				else if ((frequencykHz >=550000) && (frequencykHz<560000))
				{
					B45LNA_TANK = 0;
					LNA_TANK_TRIM = 1;
					B45_NA_QTRIM = 3;
			}	
				else if ((frequencykHz >=560000) && (frequencykHz<650000))
				{
					B45LNA_TANK = 3;
					LNA_TANK_TRIM = 0;
					B45_NA_QTRIM = 2;
				}			
				else if ((frequencykHz >=650000) && (frequencykHz<700000))
				{
					B45LNA_TANK = 1;
					LNA_TANK_TRIM = 0;
					B45_NA_QTRIM = 2;
				}		
				else if ((frequencykHz >=700000) && (frequencykHz<810000))
				{
					B45LNA_TANK = 3;
					LNA_TANK_TRIM = 1;
					B45_NA_QTRIM = 3;
				}		
				else if ((frequencykHz >=810000) && (frequencykHz<=870000))
				{
					B45LNA_TANK = 1;
					LNA_TANK_TRIM = 1;
					B45_NA_QTRIM = 2;
				}		
				g_fnx2_buffer[2] = B45LNA_TANK;
				g_fnx2_buffer[3] = LNA_TANK_TRIM;
				g_fnx2_buffer[4] = B45_NA_QTRIM;											
				break;
			case FNX2_BANDL: 
				//for L band, final div=2 (fix), DIVN block is bypassed (ie 1), DIV_OVERALL=final div*DIVN_TEST=2
				DIVN_TEST = FNX2_DIVN_1;
				DIV_OVERALL = 2;		
				break;
			default:
				__TBILogF("ERROR:  freq=%dkHz is NOT supported within DVB-H!!\n",frequencykHz);
				exit(-1);
				break;	
		}	
		TARGET_VCO = (double)(frequencykHz * DIV_OVERALL);
		INT_MAIN = (TARGET_VCO / 8192);			
		REG_INT = (unsigned char)(INT_MAIN - 256);
		FRA_MAIN = ((unsigned long)TARGET_VCO - ((unsigned long)INT_MAIN*8192));
		FRA_MAIN = ((double)FRA_MAIN/8192)*65536;							
		FRA_MSB = (unsigned char)(((unsigned long)FRA_MAIN >> 8) & 0xFF);	
		FRA_LSB = (unsigned char) ((unsigned long)FRA_MAIN & 0xFF);	
	
		g_fnx2_buffer[5]=DIVN_TEST;
		g_fnx2_buffer[6]=FRA_MSB;  
		g_fnx2_buffer[7]=FRA_LSB;  
		g_fnx2_buffer[8]=REG_INT;  		
		FNX2SendCommand(&FNX2_tuner_cmd, FNX2_TUNER_SETFREQ, frequencykHz, g_fnx2_buffer,pCompletionFunc);  	
	}
	else
		pCompletionFunc(PHY_TUNER_SUCCESS);
		
	return(PHY_TUNER_SUCCESS);	
}
//=====================================================================================================================
/*
** read total RF power
*/
static long fnx2Tuner_readRFPower(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
#ifdef VERBOSE
	__TBILogF("ReadRFPower called\n");	
#endif	
    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}
//=====================================================================================================================
/*
** FUNCTION:    PowerUp
**
** DESCRIPTION: power up tuner and load user parameters using muxID
**   Power up the tuner from either power-down or power-save state. Subsequently a tune() command is issued 
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_powerUp(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
#ifdef VERBOSE
	__TBILogF("RF PowerUp called -> send enable/powerup seq to RF\n");		
#endif
	if (prev_powerup==0)  //if NOT already power up, perform FNX2 power up command
	{
		prev_powerup=1;		
		FNX2SendCommand(&FNX2_tuner_cmd,FNX2_TUNER_POWERUP,(long)muxID,g_fnx2_buffer,pCompletionFunc);  
	}				
	else			 
		pCompletionFunc(PHY_TUNER_SUCCESS);
		
    return(PHY_TUNER_SUCCESS);
}
//=====================================================================================================================
/*
** FUNCTION:    PowerDown
**
** DESCRIPTION: .power down tuner and save user parameters using muxID
** 			(called in respose to PHY_cancel or after scan)
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_powerDown(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;
							
#ifdef VERBOSE	
	__TBILogF("RF PowerDown called -> PWROFF mode\n");		
#endif										
	prev_powerup=0;		
	FNX2SendCommand(&FNX2_tuner_cmd,FNX2_TUNER_POWERDOWN,(long)muxID,g_fnx2_buffer,pCompletionFunc);  			    
    
    return(PHY_TUNER_SUCCESS);
}
//=====================================================================================================================
/*
** FUNCTION:    PowerSave
**
** DESCRIPTION: switch tuner to power saving mode tuner and save user parameters using muxID
** Power down the tuner into a power-save state
**
**NOTE mapping of power level between MTX and FNX2 
**		MTX power level 0 = tuner power is max
**		MTX power level 1 = tuner power enable mode 1
**		MTX power level 2 = tuner power enable mode 2 (lowest power)
**		during disable call = FNX2 in STBY without power enable mode 
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_powerSave(PHY_RF_PWRSAV_T powerSaveMode, unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	(void)muxID;
    //__TBILogF("RF PowerSave called:  level=%d\n", level);        
	FNX2SendCommand(&FNX2_tuner_cmd,FNX2_TUNER_POWERSAVE,(long)powerSaveMode,g_fnx2_buffer,pCompletionFunc);  	
    
    return(PHY_TUNER_SUCCESS);
}
//=====================================================================================================================
/*
** FUNCTION:    setIFAGCTC
**
** DESCRIPTION: set IF AGC time constant (NOT USED NOW!!)
**		call when app requires a change in IF AGC time constant
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_setIFAGCTimeConstant(long timeConstuS, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)timeConstuS;      /* remove compiler warning about unused parameters */
    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}
//=====================================================================================================================
/*
** FUNCTION:    SetAGC
**
** DESCRIPTION: Send the strings of bytes to the RF, via the I2C/SPI to set the Gain.
**		calculate and send control value required for IF gain for tuner
**		(call every AGC update period, at the end of SCP config period)
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_setAGC(TUNER_AGCISR_HELPER_T *pAgcIsrHelper, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
#ifdef BBAGC_FIX
	(void)pAgcIsrHelper;     /* remove compiler warning about unused paramers */
    pCompletionFunc(PHY_TUNER_SUCCESS);
#else
	char gain_offset[7] = {0,2,4,8,-2,-4,-8};
	char agc_gain=0;
	unsigned char gain_decision=0;
	
	if (g_fnx2_agc_counter < RFPOWER_NUMOF_SAMPLE)
	{
		g_fnx2_agc_counter++;
		g_fnx2_threshI_total = g_fnx2_threshI_total + ((pAgcIsrHelper->AGCcount1I+pAgcIsrHelper->AGCcount1Q)>>1);
		pCompletionFunc(PHY_TUNER_SUCCESS);
	}
	else
	{
		gain_decision = FNX2_GetHW_gain(g_fnx2_threshI_total/RFPOWER_NUMOF_SAMPLE);
		g_fnx2_threshI_total = g_fnx2_agc_counter = 0;
		if (gain_decision == 0)
		{
			pCompletionFunc(PHY_TUNER_SUCCESS);
    		return(PHY_TUNER_SUCCESS);
    	}
		agc_gain = gain_offset[gain_decision] + (char)g_fnx2_BBAgc_current;

		if (agc_gain >= BBAGC_GAIN_MAX)
				agc_gain = BBAGC_GAIN_MAX;
		else if (agc_gain <= BBAGC_GAIN_MIN)
				agc_gain = BBAGC_GAIN_MIN;
		else
			g_fnx2_BBAgc_current = (unsigned char)agc_gain;																																				
			
		FNX2SendCommand(&FNX2_tuner_cmd,FNX2_TUNER_SETIFAGC,(long)g_fnx2_BBAgc_current,g_fnx2_buffer,pCompletionFunc);  													
    	return(PHY_TUNER_SUCCESS);
	}
#endif
    return(PHY_TUNER_SUCCESS);
}
//=====================================================================================================================
/*
** FUNCTION:    initIFAGC
**
** DESCRIPTION: initialises IF AGC
**			call at start of SCP config (if init is required)
**			in case of DVB-H, this is call for 1st burst and skep for all subsequent burst
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T fnx2Tuner_initAGC(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	(void)muxID;
		
#ifdef VERBOSE
	__TBILogF("Init IF AGC called:  dummy msg\n");         
#endif       
	// Check if using PDM control instead of I2C
    
#ifdef BBAGC_FIX
	pCompletionFunc(PHY_TUNER_SUCCESS);
#else
	g_fnx2_BBAgc_current = BBAGC_GAIN_INIT;  
	FNX2SendCommand(&FNX2_tuner_cmd,FNX2_TUNER_INITIFAGC,(long)g_fnx2_BBAgc_current,g_fnx2_buffer,pCompletionFunc);  	      
#endif
    return(PHY_TUNER_SUCCESS);
}
//=====================================================================================================================
/*
** FUNCTION:    shutdown
**
** DESCRIPTION: shutdown tuner
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T   fnx2Tuner_shutdown(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}
