// Shared data declarations for spectrum analyser MCP pseudo-code

// These counters are set non-zero by Meta prior to running; they count down
// through the number of buffers which are to be averaged.
int24 averagingPeriod_innerLoopCount;
int24 averagingPeriod_innerLoop;
int24 averagingPeriod_outerLoopCount;

bool doOuterLoopProc; // flag controls outer loop processing
bool queueNewSCPJob; // flag controls queueing of new SCP jobs
bool bypassWindowing; // flag set if rectangular window is specified

int24 FFTlen; // FFT length

cplx24 windowFunc[SA_MAX_FFT_LEN]; // Window function, pre-loaded by Meta
