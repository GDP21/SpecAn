// This function manages loop counters and flags processing when outer loop
// counter increments, it also manages posting of new SCP jobs

void loopCtrl()
{
	queueNewSCPJob = 0; // default

	// decrement inner loop counter
	averagingPeriod_innerLoopCount--;

	// When the decremented inner loop count hits 0, we will be wrapping
	// it on the next iteration.  Flag outer loop
	// processing which will happen at the end of the next iteration.
	doOuterLoopProc = 0; // default
	if (averagingPeriod_innerLoopCount == 0)
	{
		doOuterLoopProc = 1;
		if (averagingPeriod_outerLoopCount == 0)
		{
			// Last-but-one iteration, don't post to SCP
			return;
		}
	}

	if (averagingPeriod_innerLoopCount < 0)
	{
		// Inner loop count is wrapping, process below (will not
		// post to SCP)
		goto innerLoopWrap
	}

	// Inner loop not wrapping; all there is left to do is post a new
	// job back to outselves
	goto postNewJob;

	// Inner loop count wrapping: outer loop count decrements
innerLoopWrap:
	// Re-load inner loop count
	averagingPeriod_innerLoopCount = averagingPeriod_innerLoop;
	// decrement outer loop count
	averagingPeriod_outerLoopCount--;
	// If outer loop counter hasn't gone below zero, we
	// are still going so don't process end of fragment
	if (averagingPeriod_outerLoopCount >= 0)
		goto postNewJob;

	// End of fragment, interrupt Meta to flag completion
endofOfFragment:
	interrupt_meta;
	return;

	// Set flag so processSCPqueues will post new job to SCP
postNewJob:
	queueNewSCPJob = 1;

	return;
}
