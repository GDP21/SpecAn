# Script to activate and run a host-port build of the Spectrum Analyser, normally used
# for emulator testing of customer releases.

from __future__ import print_function



######################################
#### Select as appropriate below: ####
######################################
isEmulator = 0


if isEmulator == 1:
    print("********************")
    print("Running for EMULATOR")
    print("********************")

    # Set genericTuner to 1 if using a generic or dummy tuner driver e.g. on the emulator
    genericTuner = 1
    
    # ADC clock freq should be set to 90MHz on emulator, 78.64MHz on Saturn 2
    ADCclkFreq = 90000000
    
    sleepTime = 5
else:
    print("********************")
    print("Running for Saturn 2")
    print("********************")
    genericTuner = 0
    thread1Tuner = 0
    #genericTuner = 1
    #thread1Tuner = 1
    ADCclkFreq = 78640000
    sleepTime = 1
    

startFreq      = 950e6
scanRange      = 1100e6
scanResolution = 200e3
bandwidth      = 52e6
tunerGridBase  = 0
tunerGridInc   = 1e6
avgPeriodN     = 5      # 64 Averages
avgPeriodM     = 8      # Gives a total of 8*64=512 averages
tuningStep     = 20e6   # 20MHz Manual tuning step
peakWidth      = 4      # 4 bin peak width
dcComp         = 1      # DC compensation enabled
windowType     = 1      # Hamming window


# Generally we set runUnderCodescape to 1, so the code must first be manually loaded and run
# under Codescape.  If running without Codescape, set this to 0.
runUnderCodescape = 1


#### Edit as appropriate if running without Codescape ####
if runUnderCodescape == 0:
    DA_ID = 'DA-net 00175'
    load_script = '../../../support/specAn/loaders/hostPortApp/build/smake/test_METAGCC/SPECAN_hostPortApp.py'

from transactions import *
from commonTvRegisters import CommonTvRegisters
from commonTvRegisters import SpecAnRegisters
import thread
from thread1Ctrl import tunerMonitor

messages_thread = Messages(0)

if runUnderCodescape > 0:
    messages_thread.ToggleTimeouts()
    messages_thread.GetConnection()
else:
    messages_thread.load_code(load_script,DA_ID)

###########################
#### ACTIVATE
###########################
print('Activating..')
target = messages_thread.img_tv_activate(0, 0)
print('Target ID is ' + str(target))

time.sleep(sleepTime);

# Get the Demod ID and version number
demodId = messages_thread.img_tv_getreg(target, CommonTvRegisters.TV_REG_DEMOD_ID)
demodId = demodId>>24
buildId = messages_thread.img_tv_getreg(target, CommonTvRegisters.TV_REG_BUILD_ID)
if demodId != 0:
    print('ERROR: INVALID DEMOD ID ' + str(demodId))
    exit(0)
print('Activating Demod ID ' + str(demodId) + '. Version: ' + str(buildId))

###########################
#### Set up front end.  This is necessary when testing customer
#### releases using the generic tuner driver.
###########################
if genericTuner > 0:
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_CTRL, 0x142)
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_ADCSAMP, ADCclkFreq)
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_IF, 0)
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_RS, 0x80000000)
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_AGC_NORMAL, 65535) # 0x3FFFF/4
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_AGC_FAST, 22500) # 90000/4
    result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_FE_GROUP, 0)
    print('Set up front end')

############################
###### Tune
############################

if thread1Tuner == 1:
    # Start the thread 1 tuner monitor
    thread.start_new_thread ( tunerMonitor, (messages_thread, target) )

############################
###### SET bandwidth
############################
result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_TUNER_BW, bandwidth)
print('bandwidth set, return val:  ' + hex(result))

############################
###### SET frequency
############################
result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_TUNER_FREQ, startFreq)
print('Start frequency set, return val:  ' + hex(result))

time.sleep(sleepTime);

############################
##### Configure the Spectrum Analyser
############################
result = messages_thread.img_tv_setreg(target, SpecAnRegisters.SA_SCAN_RANGE, scanRange)
result = messages_thread.img_tv_setreg(target, SpecAnRegisters.SA_SCAN_RESOLUTION, scanResolution)
result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_TUNER_GRID_BASE, tunerGridBase)
result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_TUNER_GRID_INCR, tunerGridInc)
result = messages_thread.img_tv_setreg(target, SpecAnRegisters.SA_AVERAGING_PERIOD, (avgPeriodN + (avgPeriodM << 8)))

# If the tuning step is zero, use auto tuning step
if tuningStep == 0:
    tuningStep = 1<<31
result = messages_thread.img_tv_setreg(target, SpecAnRegisters.SA_TUNING_STEP, tuningStep)

# Set the control register
val = (peakWidth) + (windowType << 6) + (dcComp << 9)
result = messages_thread.img_tv_setreg(target, SpecAnRegisters.SA_MEASUREMENT_CONTROL, val)

############################
##### DETECT command
############################
result = messages_thread.img_tv_setreg(target, CommonTvRegisters.TV_REG_CONTROL, 2)
print('Detect command, return val: ' + hex(result)  )

############################
##### Monitor the state until it completes
############################
state = messages_thread.img_tv_getreg(target, CommonTvRegisters.TV_REG_STATE)
while state != 3:
    if thread1Tuner == 0:
        print('Current state = ' + str(state), end='\r')
    time.sleep(1)
    state = messages_thread.img_tv_getreg(target, CommonTvRegisters.TV_REG_STATE)


############################
##### Current state
############################
print('Current state = ' + str(state))

############################
##### Extract the results
############################
nSpectrumWords = messages_thread.img_tv_getreg(target, SpecAnRegisters.SA_OUT_SPECTRUM_LEN)
spectrumAddr   = messages_thread.img_tv_getreg(target, SpecAnRegisters.SA_REG_OUT_SPECTRUM_PTR)

if (spectrumAddr >> 30) == 0:
    spectrumAddr = ((spectrumAddr & 0x03ffffff) | 0xb4000000);      # Data in GRAM
else:
    spectrumAddr = (((spectrumAddr & 0x03ffffff)<<2) | 0xb0000000); # Data in External RAM

print(str(nSpectrumWords) + ' spectrum words created at ' + hex(spectrumAddr))
