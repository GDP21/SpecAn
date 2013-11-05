#ifndef _USB_IP_H
#define _USB_IP_H

//Structure and enums for various options and modes
typedef enum		//Input mode
{
	eUSB_IN,
	eFILE_IN
} INPUT_MODE_E;
typedef enum		//Output mode
{
	eSOCKET_OUT,
	eFILE_OUT
} OUTPUT_MODE_E;
typedef enum		//Output format
{
	eRAW_OUT,
	eIP_OUT,
	eUDP_OUT
} OUTPUT_FORMAT_E;

typedef struct		//options
{
	INPUT_MODE_E	input_mode;
	OUTPUT_MODE_E   output_mode;
	OUTPUT_FORMAT_E output_format;
	char			in_fname[BUFSIZ];
	char			out_fname[BUFSIZ];
	int			    delay;
	int			    localhost;
	unsigned long   length;
} OPTIONS_S;

//Logging defs
#define ALL_LOG    0
#define TRACE_LOG  1
#define INFO_LOG   2
#define WARN_LOG   3
#define SEVERE_LOG 4
#define FATAL_LOG  5

int dbg_printf(int severity_level, char *formatstring, ...);

#endif