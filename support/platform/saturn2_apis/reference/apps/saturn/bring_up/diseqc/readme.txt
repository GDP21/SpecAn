

Run codescape, File->Load via script..  debug/runwithargs.js
 
  
Arguments:
  
  	diseqctest.js			Script to execute with arguments. Must be the 1st argument and must be the diseqc test script (diseqctest.js)
	-test N				Test Number (Default 1)\n");
	-block N			Block Index (Default 0)\n");
	-wait				Wait (modify variables to perform test) (Default No Wait)\n");
	-rxtonetolerance N		Tone Tolerance (0-1000) (Default 500)\n");
	-rxmindetections N		Min Detections Per Chunk (Default 9)\n");
	-rxchunkwidth N			Receive Chunk Width (Default 12 (0.5ms))\n");
	-rxpostmessagesilence N		Receive Post Message Silence (Default 0x84 (6ms))\n");
	-slaveresponsetimeout N		Slave Response Timeout (Default 0xCE4 (150ms))\n");
	-txshortchunkwidth N		Send Short Chunk Width (Default 11 (0.5ms))\n");
	-txlongchunkwidth N		Send Long Chunk Width (Default 22 (1ms))\n");
	-masteremptytimeout N		Master Empty Timeout (Default 0x84 (6ms))\n");
	-posttransactionsilence N	Post Transaction Silence (Default 0x14A (15ms))\n");
	-targetfreq N			Target Frequency Q12.20 mhz (Default 0x5A1D (22khz))\n");
	-receiveexpected		Receive Only When Data Is Expected (Default Always Receive)\n");
	-tonelogic			Input/Output Is Polarity (Default Input/Output Is Tone Wave)\n");
	-inputpolaritylow 		Input Polarity Low (Default Input Polarity High)\n");
	-outputpolaritylow		Output Polarity Low (Default Output Polarity High)\n");
	-loopback			Loopback On (Default Off)\n");
	-messages X ...			Messages (Variable Number Of Messages In Hex eg -messages E00000 E03168)\n");
	-iter N				Iterations For Test (Default 0xFFFFFFFF (Infinite))");
	
   
 Tests:
 
 	1 - DISEQC_TEST_AUTO				- Automated test of all functions.
	2 - DISEQC_TEST_CONTINUOUS			- Continuous tone
	3 - DISEQC_TEST_CONTINUOUS_END			- Intermittant tone
	4 - DISEQC_TEST_TONE_BURST_A			- Tone burst A
	5 - DISEQC_TEST_TONE_BURST_B			- Tone burst B
	6 - DISEQC_TEST_SEND_MESSAGES			- Send messages
	7 - DISEQC_TEST_SEND_RECEIVE_MESSAGES		- Send messages with replies
	8 - DISEQC_TEST_TONE_BURST_A_B			- Alternating A and B tone bursts
	9 - DISEQC_TEST_CONTINUOUS_TONE_BURST_A		- Continuous tone suspended for tone burst A then continued
	10 - DISEQC_TEST_CONTINUOUS_TONE_BURST_B	- Continuous tone suspended for tone burst B then continued
	
	
 	Test 1 is the most comprehensive and simplest to execute. This should be used if the hardware is available. To run the test, do the following:
 	- Locate the DiSEqC test tool. It is a grey device with DiSEqC 2.0 TEST TOOL written on the front.
 	- Connect the power supply for the test tool. There are no LEDs to indicate the device is on, the automated test will fail to connect via serial if the device is not powered.
 	- Connect the DiSEqC interface to be tested to the test tool using a coaxial cable (use the port closest to the power connection). 
 	- Connect the tool to the PC that will run the test using a serial cable.
 	- Run the windows automated test. 
 		-If it fails to connect via serial check the serial port is the correct one and the device is powered. 
 		-If it complains about the DiSEqC bus being powered, ensure the connection is made and an external 12V power supply is not required.
 		-Run the META test when it asks.
   
  
 
example arguments: 	diseqctest.js -test 1 
			run the automated test

			diseqctest.js -test 6 -posttransactionsilence 22000 -messages E03100 E03166 E03167 E0316800
			send messages E03100, E03166, E03167, E0316800 with 1 second wait in between messages.