// This function is run by MCPOS each time a new SCP buffer arrives.
// Processing of a spectral fragment is kicked off by Meta queuing 2 SCP jobs.
// This function has 2 arg chains, selected according to which SCP buffer has
// completed.

void SCPoutBufferProc()
{
	// Window the buffer data.  The window data values have been pre-loaded
	// by Meta into a vector according to the selected window shape.
	windowProc();

	// FFT the buffer data.  This function follows a function pointer
	// loaded from its argblock, to select the correct FFT size.
	FFTproc();

	// Calculate spectral power
	calcSpectralPower();

	// Inner loop processing - average results into an intermediate buffer
	innerLoopProc();

	// Outer loop processing (only occurs on wrapping of the
	// averagingPeriod_innerLoopCount counter) - scale and average into results buffer.
	outerLoopProc();

	// If there are less than 2 jobs to go then stop posting jobs, as 2 jobs have been
	// posted by Meta in advance.  This function deals with posting further SCP jobs

	// This function deals with decrementing the loop counters and also setting a flag
	// that controls whether the outer loop processing operations get performed
	// (when the inner loop counter wraps).  It also sets a flag used by processSCPqueues
	// to determine whether new SCP jobs are queued.
	// When we get to the end of the fragment, this function also interrupts Meta to
	// indicate that we are done (the interrupt is done by posting to irqgen final
	// queue)
	loopCtrl();

	// This function deals with removing the current job from the SCP output queue,
	// queueing a new job to the SCP and queuing an associated MCPOS job back to ourselves.
	// On the last 2 loop iterations, new jobs are not queued, causing us to stop after
	// the required number of iterations.
	// Note that we have finished using the SCP buffer now so it is safe to queue a
	// new job into the same buffer.
	processSCPqueues();

	return;
}
