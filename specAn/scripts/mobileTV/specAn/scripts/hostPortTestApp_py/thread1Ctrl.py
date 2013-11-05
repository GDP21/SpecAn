# Task to communicate between an application on Thread0 and a tuner on Thread1
#

from commonTvRegisters import CommonTvRegisters
from CSUtils import DA

def tunerMonitor(messager, target):

    print("Tuner Monitor Thread Started")
    
#    thread1 = DA.GetNextThread(DA.GetFirstThread()) 
#    with DA.TargetSelector(thread1):
#        print DA.GetTargetInfo(DA.GetCurrentTarget())
#        bwAddr = DA.EvaluateSymbol("manualTunerCtrl");
#        print(bwAddr)
    bwAddr   = int("0x39888c58", 0)
    freqAddr = int("0x39888c5c", 0)
    
    
    # This very simple task monitors for a change in the tuner frequency, and then applies the updates
    lastFrequency = messager.img_tv_getreg(target, CommonTvRegisters.TV_REG_ACTIVE_TUNER_FREQ)
    
    # Loop forever, checking for tuner requests
    while 1:
        frequency = messager.img_tv_getreg(target, CommonTvRegisters.TV_REG_ACTIVE_TUNER_FREQ)
        if frequency != lastFrequency:
            # Get the requested bandwidth
            bandwidth = messager.img_tv_getreg(target, CommonTvRegisters.TV_REG_ACTIVE_TUNER_BW)
            
            # Set the frequency and bandwidth on Thread 1
            print( '\b   Tuner request ' + str(frequency) + ' ' + str(bandwidth))
            
            DA.WriteLong(bwAddr, bandwidth);
            DA.WriteLong(freqAddr,frequency);
            
            messager.img_tv_setreg(target, CommonTvRegisters.TV_REG_TUNER_BW, bandwidth)
            messager.img_tv_setreg(target, CommonTvRegisters.TV_REG_TUNER_FREQ, frequency)
            lastFrequency = frequency