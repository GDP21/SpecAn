// Window the SCP output buffer data.  The window data values have been pre-loaded
// by Meta into a vector of correct length according to the selected window shape.

void windowProc(int24 *SCPoutputData)
{
	int24 i;

	if (bypassWindowing)
		return;

	for (i = 0; i < FFTlen; i++)
	{
		// Note this is a multiplication of real and imaginary
		// parts separately (not a complex multiply)
		SCPoutputData[i] = SCPoutputData[i] * windowFunc[i];
	}
}
