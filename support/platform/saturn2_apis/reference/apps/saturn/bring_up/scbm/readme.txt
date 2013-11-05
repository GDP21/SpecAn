
1.) Plug aardvark into eval board/connector (For the Saturn bring up board, this is CN38).

2.) Install aardvark USB driver from http://www.totalphase.com/support/product/aardvark_i2cspi/

3.) Download aardvark control center software from http://www.totalphase.com/support/product/aardvark_i2cspi/

4.) Run Aardvark control center and configure as I2C Slave. Configure the slave address as the same one used 
in the runwithargs.js script and load the test file (eg large.bin). Select 'Enable', then 'Set Resp.'.

5.) Use load via script in codescape and select runwithargs.js (this file will need to be in the same directory
	as the .elf file). For a list of options, consult the Test Harness source code. Example here:

scbmtest.js -test 19 -size 80 -addrA 0x7A -br 400 -out large.bin -ref large.bin -block 0 -clksrc 0 -coreclockfreq 24576
	
6.)	In Codescape select Project->Set Fileserver root directory. Select the path of the data files
	(e.g.: large.bin).

7.) Run program on the bring up board and follow instructions printed in Codescape's log window. This may entail 
	checking data that the Aardvark has received.



