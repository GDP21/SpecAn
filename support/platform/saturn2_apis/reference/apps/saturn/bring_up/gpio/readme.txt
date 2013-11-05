1) locate the block index and the pin number of the pins to be tested (list in API header file) - there are no GPIO only pins, rather a number of pins that can be
   set to act as GPIO pins rather than being used in the interface they belong to.

2) Run codescape, File->Load via script..  debug/runwithargs.js.

Arguments:

	gpiotest.js		Script to execute with arguments. Must be the 1st argument and must be the gpio test script (gpiotest.js)
	-test N			Test number
	-blockA N		Block Index (first pin)
	-pinA N			Pin Number (first pin)
	-blockB N		Block Index (second pin)
	-pinB N			Pin Number (second pin)
	-blockC N		Block Index (third pin)
	-pinC N			Pin Number (third pin)
	
Tests:

	1 - GPIO_TEST_AUTO				- Automated test of all functions. Pin A and Pin B must be connected together. No interaction is required for this test
	2 - GPIO_TEST_OUTPUT_LOW			- Low output on first pin
	3 - GPIO_TEST_OUTPUT_HIGH			- High output on first pin
	4 - GPIO_TEST_OUTPUT_OSCILLATE			- Oscillate between low and high output on the first pin
	5 - GPIO_TEST_OUTPUT_OSCILLATE_REMOVE		- Oscillate between low and high output on the first pin with removing and re-adding the pin in-between
	6 - GPIO_TEST_FOLLOW_POLL			- Second pins output will follow the input of the first pin (using polling)
	7 - GPIO_TEST_FOLLOW_INTERRUPT			- Second pins output will follow the input of the first pin (using interrupts that are reconfigured after each interrupt)
	8 - GPIO_TEST_FOLLOW_INTERRUPT_TRANSITION	- Second pins output will follow the input of the first pin (using the dual edge interrupt)
	9 - GPIO_TEST_DISABLE_INTERRUPT			- Second pins output will follow the input of the first pin when the input of the third pin is high (using the dual edge interrupt)
					
	Test 1 is the most comprehensive and simplest to execute. Two GPIO pins (Pin A and Pin B) must be connected together to perform the test.
	Other tests will require the ability to set and read levels on the pins externally (eg aardvark,LEDs,etc).
   

example arguments: gpiotest.js -test 1 -blockA 0 -pinA 7 -blockB 0 -pinB 8
		   = automated test using SPI1_CS0 and SPI1_CS1 (these pins must be connected together)
