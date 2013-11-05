This test requires the 'Aaardvark' SPI analyser to be connected to the board under test.
It also requires the 'runwithargs.js' to be copied into the build directory once the test has been built.
'RunWithArgs.js' should then be loaded via Codescape, with the arguments appropriate for the test being run specified.

An example set of arguments is:
spimtest.js -test 10 -size 20 -dev 0 -mode 0 -br 1 -dma 0 -data 0 -cs 0 -hold 255 -setup 255 -delay 255 -write1 mosi_1.bin -write2 mosi_2.bin -read miso_1.bin -block 1 -bytedelay 1

If the test harness is run with no arguments, usage information will be displayed.

Prior to running the tests, the data files provided in the 'spim\thread0\data' directory should be copied to your Codescape fileserver directory.

To set up Aardvark slave on PC, run Aardvark GUI, click 'configure aardvark' and select connected device. On RHS of screen (SPI control) go to 'slave' tab and select 
polarity and phase to match SPI test mode (this varies in some tests). Load MISO (Master In, Slave Out) message (from 'spim\test\thread0\data' directory), click
'Set MISO' and then 'enable'. 'Bit order' should always be set to 'MSB'.

SPI modes (quick reference):
Mode	Polarity	  	Phase
		(0=rise/fall)	(0=sample/setup)
		(1=fall/rise)	(1=setup/sample)
0			0				0
1			0				1
2			1				0
3			1				1


Troubleshooting:

If there is no response seen in the aardvark window, the chip select jumpers may need to be modified - check SW14 bit 7 is on.

