Common Mirics Tuner Driver.

This is a common Mirics tuner driver common to the requirements of the all the mobile TV (and FM) standards.

The common Mirics driver current provides the following features:
-A common driver that supports bands 2, 3, 4-5 and L with a common PLL grid size.
	-PLL grid size and update margin can be overridden with externally defined values 
	 (see Stephen Smith for more details if you require them).
	
-It is expected that some standards may need to add small standard specific modifications as necessary. 
 This will not produce the smallest tuner driver for each standard, but that’s the cost of common code.

-Extra ‘DVB-H’ support has been added to get the best performance out of the Emmy. This is conditional compilable.

-L-band calibration (required to achieve decent L-band sensitivity) is only performed once upon the first tune to an 
 L-band frequency. This is ok for T-DMB as calibration within the standard DAB range of L-band frequencies will be 
 pretty good across all of that limited L-band range. This is sufficient for the present needs of the driver in L-band

-Channel filter is configured to match (as closely as we can) the bandwidth of the signal we are configured to.

-Basic control of RF is implemented for power up, power down and power save functions.

-Support for both Emmy (Oscar) and MSI001 eval boards  on Avnet/Kuroi

-MSI001 and MSI002 are supported using SPIM and I2C respectively.

-Handles the requirement that the tuner callbacks may be in interrupt context, hence a task is required to talk to the 
 device drivers (as they require to be able to block).

-4KB stack for the tuner task. Can only be reduced when all supported standards guarantee lower usage.

-MeOS is required. Included in the driver are use of a pool and a mailbox. These are commonly used by device drivers 
 and QIO hence I believe (without further investigation to prove me wrong) that is doesn't add significant parts of 
 MeOS to the build that are otherwise unneeded.


