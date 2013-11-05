// This function deals with removing the current job from the SCP output queue,
// queueing a new job to the SCP and queuing an associated MCPOS job back to ourselves.
// On the last 2 loop iterations, new jobs are not queued, causing us to stop after
// the required number of iterations.  This is controlled by the queueNewSCPJob flag
// set within loopCtrl.

void processSCPqueues(int24 *nextMCPOSjobPtr, int24 *nextSCPaddressPtr)
{
	// Peripheral bus write to head of SCP output address queue pops our job
	// off the queue (the other two queues are not instantiated)
	*SCPOUT_addressQheadPtr = 1;

	if (!queueNewSCPJob)
		return;

	// Posting a new SCP jobs involves writing to the capture length, discard length and
	// address queues
	*SCPIN_captureLenQtailPtr = SCPcaptureLen;
	*SCPIN_discardLenQtailPtr = 0;
	*SCPIN_addressQtailPtr = *nextSCPaddressPtr;

	// Then queue an MCPOS job that will be activated when the SCP job completes
	*mcposSCPjobQtailPtr = nextMCPOSjobPtr;

	return;
}

