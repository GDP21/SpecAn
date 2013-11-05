/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tools/usb_ip_player2/source/app.c,v $
**
** TITLE:       USB IP Player
**
** AUTHOR:      Ensigma.
**
** DESCRIPTION: USB IP playout program to be used primarily with Vidi board
**              Includes options for:
**               - USB->file       (raw)
**               - USB->file       (IP parsed)
**               - USB->UDP socket (IP verified)
**               - USB->UDP socket (UDP verified)
**               - file->file       (raw)
**               - file->file       (IP parsed)
**               - file->UDP socket (IP verified)
**               - file->UDP socket (UDP verified)
**
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/


#define DIAGNOSTICS

//Include files - note need to be careful about the ordering of winsok2 and windows
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <winsock2.h> /* required by sock.h */
#include <windows.h>
#include <stdarg.h>

#include "stdint.h"
#include "ip.h"
#include "sock.h"
#include "usb_ip.h"

//Defaults if options not over ridden on cmd line
#define  DEF_INPUT_MODE      (eUSB_IN)
#define  DEF_OUTPUT_MODE     (eSOCKET_OUT)
#define  DEF_OUTPUT_FORMAT   (eRAW_OUT)
#define  DEF_DELAY           (40)
#define  DEF_LOCALHOST       (1)
#define  DEF_LENGTH          (0)
#define  DEF_INP_BLOCK_SIZE  (8192)
#define  DEF_LOG_LEVEL       (INFO_LOG)
#define  PACKETS_PER_INFO    (200)

//Defines
#define MTU					16000	/* maximum size of IP - used for sizing/checking */
#define IP_HEADER_SIZE		20		/* from RFC 791 */
#define UDP_HEADER_SIZE		8		/* from RFC 768 */
#define MAX_UDP_DATA_SIZE	16000	/* max size of UDP packet - some what arbitary limit*/
#define CHUNK_SIZE			8192	/* size of file/USB data reads                      */

//Local funtcion to extract options
static int ExtractOptions(int argc, char *argv[], OPTIONS_S *args);
static void PrintHelp();

//Function pointer types to simply overloading 
typedef int (*FN_READ_T)(unsigned char *buffer, size_t size, int no_items, OPTIONS_S *opt);
typedef int (*FN_DELIVER_T)(FN_READ_T pFread, OPTIONS_S *opt);

//Functions for File Input
int ReadFileInput(unsigned char *buffer, size_t size, int no_items, OPTIONS_S *opt);

//Functions for USB Input
int ReadUSBInput(unsigned char *pBuffer, size_t size, int no_items, OPTIONS_S *opt);

//Functions for socket output
static int DeliverDataToSocket(FN_READ_T pFread, OPTIONS_S *opt);

//Functions for file output
static int DeliverRawDataToFile(FN_READ_T pFread, OPTIONS_S *opt);
static int DeliverIPDataToFile(FN_READ_T pFread, OPTIONS_S *opt);

//Logging level, can be overridden on command line
static int log_level = INFO_LOG;


static SOCK_T sockets[32];
static unsigned short sockets_port[8],sockets_index;
unsigned int port_connected(unsigned short port, int *index);
static int connected=0;


/*
** FUNCTION:	main
**
** DESCRIPTION:	Main program loop.
**
** RETURNS:		Exit status.
*/

int __cdecl main(int argc, char *argv[])
{
	FN_READ_T      pFnReadInput;
	FN_DELIVER_T   pFnDeliverData;
	OPTIONS_S      options;

	//Check data-sizes are as we expect
	assert(sizeof(uint8_t) == 1);
	assert(sizeof(uint16_t) == 2);
	assert(sizeof(uint32_t) == 4);


	//Extract operations (defaults + overriden from cmd line)
	if (ExtractOptions(argc, argv, &options) != 1)
	{
		PrintHelp();
		exit(0);
	}

	//Startup message
	dbg_printf(SEVERE_LOG,"usb_ip_player >\n");

	//Set up overloaded functions based on operation mode
	if (options.input_mode == eUSB_IN)
	{
		//USB mode, so set up function pointers appropriately
		pFnReadInput = (FN_READ_T)ReadUSBInput;
	}
	else
	{
		//Not USB mode, so set up function pointers for file input
		pFnReadInput = (FN_READ_T)ReadFileInput;
	}
	if (options.output_mode == eSOCKET_OUT)
	{
		//Stream out to socket
		pFnDeliverData = (FN_DELIVER_T)DeliverDataToSocket;
		if (!SOCK_startup()) exit(0);
	}
	else
	{
		//Dump data to file
		if (options.output_format == eIP_OUT)
		   pFnDeliverData = (FN_DELIVER_T)DeliverIPDataToFile;
		else
		   pFnDeliverData = (FN_DELIVER_T)DeliverRawDataToFile;
	}

	/* continuous loop playback */
	while ((*pFnDeliverData)(pFnReadInput, &options) == 0)
		;


	while (connected) {
		SOCK_close(&sockets[--connected]);
	}
	if (pFnDeliverData == (FN_DELIVER_T)DeliverDataToSocket) SOCK_shutdown();

	return EXIT_SUCCESS;
}


/*
** FUNCTION:	deliverData
**
** DESCRIPTION:	Handles fragmented IP datagrams which are reassembled and then
**				forwarded as complete UDP packets.
**
** PARAMETERS:  pFread - pointer to input data read function
**
** RETURNS:		Boolean status.
*/
static int DeliverDataToSocket(FN_READ_T pFread, OPTIONS_S *opt)
{
	uint8_t datagram[MTU];
	uint8_t chunk[CHUNK_SIZE];
	uint8_t *buffer;
	int length;
	IP_HEADER_T ip;
	UDP_HEADER_T udp;
	long bytes_required;
	long bytes_in_chunk;
	unsigned char *pBytes;
	uint8_t data[MAX_UDP_DATA_SIZE];
	uint8_t *offset = NULL;
	int total = 0;
	int id = 0;
	int header_search;

	unsigned char pseudo_header[20];
	unsigned char *word32;

//	static SOCK_T player;
//	static int connected;
	static unsigned short port;
	static unsigned long bytes_written = 0;
	static unsigned long packet_count = 0;
	

	//Loop reading and parsing data
    if ( (*pFread)(chunk, CHUNK_SIZE, 1, opt) != 1)
		return 0;
	bytes_in_chunk  = CHUNK_SIZE;
	pBytes = chunk;
	header_search = 1;
	while (((opt->length == 0) || (bytes_written < opt->length)) )
	{
		//Data parsing all written to work on contiguous data, so once we have header we copy
		if (bytes_in_chunk < IP_HEADER_SIZE)
		{
			memcpy(chunk, pBytes, bytes_in_chunk);
		    if ( (*pFread)(&chunk[bytes_in_chunk], CHUNK_SIZE-bytes_in_chunk, 1, opt) != 1)
				return 0;
			bytes_in_chunk  = CHUNK_SIZE;
			pBytes = chunk;
		}
		//Parse header and check if it is valid (rely on checksum and length checks)
		if ( (!IP_readHeader(pBytes, IP_HEADER_SIZE, &ip))
			||(ip.total_length > MTU))
		{
			//Failed to find valid header, so move on byte and retry
			bytes_in_chunk--;
			pBytes++;
			header_search = 1;
			continue;
		}
		if (header_search)
		{
			//Resync, so print debug message
			dbg_printf(INFO_LOG, "Synced to IP header\n");
			header_search = 0;
		}
        bytes_in_chunk -= IP_HEADER_SIZE;
		pBytes += IP_HEADER_SIZE;

#ifdef DIAGNOSTICS
		printf("IP: %d ", ip.version);
		printf("%d ", ip.IHL);
		printf("%d ", ip.type_of_service);
		printf("%d ", ip.total_length);
		printf("%d ", ip.identification);
		printf("%d ", ip.flags);
		printf("%d ", ip.fragment_offset);
		printf("%d ", ip.time_to_live);
		printf("%d ", ip.protocol);
		printf("%d ", ip.header_checksum);
		printf("0x%08X ", ip.source_address);
		printf("0x%08X\n", ip.destination_address);
#endif

		//Read payload (may require two reads) - to provide contigous data, we copy from chunk buffer 
		//to datagram
		bytes_required = ip.total_length - (4 * ip.IHL);
		if (bytes_in_chunk >= bytes_required)
		{
			//already gor enough data so just copy across
			memcpy(datagram, pBytes, bytes_required);
			bytes_in_chunk -= bytes_required;
			pBytes += bytes_required;
		}
		else
		{
			//Copy remainer, then read read and copy chunks
			memcpy(datagram, pBytes, bytes_in_chunk);
			bytes_required -= bytes_in_chunk;
		    if ( (*pFread)(chunk, CHUNK_SIZE, 1, opt) != 1)
				return 0;
			memcpy(&datagram[bytes_in_chunk], chunk, bytes_required);
			bytes_in_chunk = CHUNK_SIZE - bytes_required;
			pBytes = chunk + bytes_required;
		}

		//Bail out if it's not UDP
		if (ip.protocol != UDP_PROTOCOL)
		{
			dbg_printf(TRACE_LOG, "TRACE: IP payload not UDP.\n");	
			continue;
		}

		//Extract UDP header and payload
		//This is complicated by the that that UDP dgarm may be fragmented across two or more IP
		//packets
		buffer = datagram; /* skip IP header */
		length = ip.total_length - (4 * ip.IHL);
		if (ip.fragment_offset == 0)
		{
			//Start, so extract header and build psuedo header required later for checksum check
			if (!UDP_readHeader(buffer, length, &udp))
			{
				dbg_printf(TRACE_LOG, "TRACE: Unable to parse UDP header.\n");
				continue;
			}

			//Fill in pseudo header required later for checksum 
			//This is actually Pseudo header + UDP header (this is stripped off)
			word32 = (unsigned char *)&(ip.source_address);
			pseudo_header[3] = word32[0];
			pseudo_header[2] = word32[1];
			pseudo_header[1] = word32[2];
			pseudo_header[0] = word32[3];
			word32 = (unsigned char *)&(ip.destination_address);
			pseudo_header[7] = word32[0];
			pseudo_header[6] = word32[1];
			pseudo_header[5] = word32[2];
			pseudo_header[4] = word32[3];
			pseudo_header[9] = ip.protocol;
			pseudo_header[8] = 0;
			word32 = (unsigned char *)&(udp.length);
			pseudo_header[11] = word32[0];
			pseudo_header[10] = word32[1];
			memcpy(&pseudo_header[12], buffer, UDP_HEADER_SIZE);

#ifdef DIAGNOSTICS
			printf("\tUDP: %d ", udp.source_port);
			printf("%d ", udp.destination_port);
			printf("%d ", udp.length);
			printf("%d ", udp.checksum);
			buffer=udp.data;
			printf("%02x ",buffer[1]);
			printf("%02x%02x%02x%02x ",buffer[4],buffer[5],buffer[6],buffer[7]);
			printf("%02x\n",buffer[12]);
#endif
			port = udp.destination_port;
			buffer = udp.data; /* skip UDP header */
			length -= UDP_HEADER_SIZE;

			offset = data; /* reset */
			total = 0;
			id = ip.identification;
		}


		//Run various checks to make sure that we have valid fragmenst of DFGRAM etc
		if (offset == NULL) 
			continue; /* still waiting for first UDP header after startup */

		//Check that if we are on part of fragmented DGRAM the ip.id matches
		//(if are on first framgment the it will have been assigned above)
		if (ip.identification != id) /* corrupt? */
		{
			dbg_printf(TRACE_LOG, "TRACE: IP datagram fragments out of sequence? %hd %hd\n", id, ip.identification);
			continue;
		}

		if (((total > 0) && ((8 * ip.fragment_offset - UDP_HEADER_SIZE) != total)) ||
			((total == 0) && (ip.fragment_offset != 0)))
		{
			/* missed fragment - assume off-air fragments will not be received out of order
			therefore this means we have missed a fragment so ignore any further fragments
			with same ID */
			dbg_printf(TRACE_LOG, "TRACE: IP datagram fragment(s) missed: %d vs %d\n", ip.fragment_offset, total);
			continue;
		}

		if ((total + length) > MAX_UDP_DATA_SIZE)
		{
			dbg_printf(SEVERE_LOG, "ERROR: UDP data too large for internal buffer!\n");
			continue;
		}

		//Add new data into dgram (either at start or at offset if second fragment)
		memcpy(offset, buffer, length); /* rebuild fragmented IP datagram */

		offset += length; /* next IP fragment will go here */
		total += length;

		//Check to see if we now have all the data, and if so verify and output
		if ((ip.flags & 1) == 0) /* last fragment */
		{
			if (opt->localhost)
			{
				ip.destination_address = 0x7F000001; /* override IP header when not connected to network */
			}

			//Open socket if not current connected
			if (!(port_connected(port,&sockets_index))) {
				sockets_port[connected]=port;
				if ((connected==32) || (!SOCK_open(&sockets[connected], ip.destination_address, port)))
				{
					dbg_printf(SEVERE_LOG, "ERROR: Unable to connect to player (socket).");
					return 0;
				}
				dbg_printf(SEVERE_LOG, "Opened Port %d\n",port);
				connected++;
			}






#ifdef DIAGNOSTICS
			printf("\t\taddr=0x%08X/port=%d: %d\n", ip.destination_address, port, total);
#endif

			//If in UDP validated mode, check UDP checksum and ditch packet if it is invalid
//			if (opt->output_format == eUDP_OUT)
			{
				//Check checksum
				if (!ValidatePacketChecksum(pseudo_header, sizeof(pseudo_header), data, total))
				{
					dbg_printf(TRACE_LOG, "TRACE: Invalid UDP checksum\n");
					continue;
				}
			}
			//Send UDP palyload to socket connect to WMP
			//if ((buffer[1] & 0x7f)==96) {
				if (!SOCK_write(&sockets[sockets_index], data, total)) /* send rebuilt UDP data (no headers) */
				{
					dbg_printf(SEVERE_LOG, "ERROR: Unable to deliver to player (socket).\n");
					return 0;
				}
			//}
			bytes_written += total;
			if (++packet_count % PACKETS_PER_INFO == 0)
				dbg_printf(INFO_LOG,"Output %d udp packets %lf Mbytes\n", packet_count, ((double)bytes_written)/1.0e6);

			/* fine tune playout rate to match broadcast material */
			if (opt->input_mode == eFILE_IN)
				Sleep(opt->delay);
		}
	}

	return 1;
}

/*
** FUNCTION:	ReadFileInput
**
** DESCRIPTION:	Reads data from input file according to mode settings.
**              Contains static file pointer, so only supports one file 
**
** RETURNS:		number of items read.
*/
static int ReadFileInput(unsigned char *buffer, size_t size, int no_items, OPTIONS_S *opt)
{
	static FILE *fp = NULL;
	size_t       num_read;

	//Open input file if not already open
	if (fp == NULL)
	{
		fp = fopen(opt->in_fname,"rb");
		if (fp == NULL)
		{
			dbg_printf(FATAL_LOG, "ERROR: Failed to open inpfile %s\n", opt->in_fname);
			exit (-1);
		}
	}

	//Read data. If read fails close file and NUL pointer so it will be reopened next time
	num_read = fread(buffer, size, no_items, fp);
	if ((num_read != no_items) || feof(fp) || ferror(fp))
	{
		fclose(fp);
		fp = NULL;
	}
	return (num_read);
}



/*
** FUNCTION:	DeliverRawDataToFile
**
** DESCRIPTION:	Reads data from input device (USb or file) and writes out to outputfile.
**
** RETURNS:		boolean status, exits if fials to open file.
*/
static int DeliverRawDataToFile(FN_READ_T pFread, OPTIONS_S *opt)
{
	static   FILE *fp = NULL;
	unsigned char  buffer[DEF_INP_BLOCK_SIZE];
	unsigned long  bytes_written;


	//Open output file if not already open
	if (fp == NULL)
	{
		fp = fopen(opt->out_fname,"wb");
		if (fp == NULL)
		{
			dbg_printf(FATAL_LOG, "ERROR: Failed to open outfile %s\n", opt->out_fname);
			exit (-1);
		}
	}

	//Loop continuously read data from input source and writing to output file
	bytes_written = 0;
	while ( ((*pFread)(buffer, DEF_INP_BLOCK_SIZE, 1, opt) == 1)
		   && ((opt->length == 0) || (bytes_written < opt->length)) )
	{
		fwrite(buffer, DEF_INP_BLOCK_SIZE, 1, fp);
		fflush(fp);
		bytes_written += DEF_INP_BLOCK_SIZE;
	}

	fclose (fp);
	exit (-1);
	return 1;
}


/*
** FUNCTION:	DeliverIPDataToFile
**
** DESCRIPTION:	Reads data from input device (USb or file, parses to find valid IP packets
**              and finally writes the packets to file
**
** RETURNS:		boolean status, exits if fials to open file.
*/
static int DeliverIPDataToFile(FN_READ_T pFread, OPTIONS_S *opt)
{
	static   FILE *fp = NULL;
	uint8_t datagram[MTU];
	uint8_t chunk[CHUNK_SIZE];
	uint8_t *pPayload;
	IP_HEADER_T ip;
	long bytes_required;
	long bytes_in_chunk;
	unsigned char *pBytes;
	int header_search;

	unsigned long  bytes_written = 0;

	//Open output file if not already open
	if (fp == NULL)
	{
		fp = fopen(opt->out_fname,"wb");
		if (fp == NULL)
		{
			dbg_printf(FATAL_LOG, "ERROR: Failed to open outfile %s\n", opt->out_fname);
			exit (-1);
		}
	}

	//Loop reading and parsing data
    if ( (*pFread)(chunk, CHUNK_SIZE, 1, opt) != 1)
		return 0;
	bytes_in_chunk  = CHUNK_SIZE;
	pBytes = chunk;
	header_search = 1;
	while (((opt->length == 0) || (bytes_written < opt->length)) )
	{
		//Data parsing all written to work on contiguous data, so once we have header we copy
		if (bytes_in_chunk < IP_HEADER_SIZE)
		{
			memcpy(chunk, pBytes, bytes_in_chunk);
		    if ( (*pFread)(&chunk[bytes_in_chunk], CHUNK_SIZE-bytes_in_chunk, 1, opt) != 1)
				break;
			bytes_in_chunk  = CHUNK_SIZE;
			pBytes = chunk;
		}
		//Parse header and check if it is valid (rely on checksum and length checks)
		if ( (!IP_readHeader(pBytes, IP_HEADER_SIZE, &ip))
			||(ip.total_length > MTU))
		{
			//Failed to find valid header, so move on byte and retry
			bytes_in_chunk--;
			pBytes++;
			header_search = 1;
			continue;
		}
		if (header_search)
		{
			//Resynced, so print debug message
			dbg_printf(INFO_LOG, "Synced to IP header\n");
			header_search = 0;
		}
        bytes_in_chunk -= IP_HEADER_SIZE;
		memcpy(datagram, pBytes, 4*ip.IHL);
		pPayload = &(datagram[4*ip.IHL]);
		pBytes += IP_HEADER_SIZE;

		//Read payload (may require two reads) - to provide contigous data, we copy from chunk buffer 
		//to datagram
		bytes_required = ip.total_length - (4 * ip.IHL);
		if (bytes_in_chunk >= bytes_required)
		{
			//already gor enough data so just copy across
			memcpy(pPayload, pBytes, bytes_required);
			bytes_in_chunk -= bytes_required;
			pBytes += bytes_required;
		}
		else
		{
			//Copy remainer, then read and copy chunks
			memcpy(pPayload, pBytes, bytes_in_chunk);
			bytes_required -= bytes_in_chunk;
		    if ( (*pFread)(chunk, CHUNK_SIZE, 1, opt) != 1)
				break;
			memcpy(&pPayload[bytes_in_chunk], chunk, bytes_required);
			bytes_in_chunk = CHUNK_SIZE - bytes_required;
			pBytes = chunk + bytes_required;
		}

		//Write IP packet to file
		fwrite(datagram, ip.total_length, 1, fp);
		fflush(fp);
		bytes_written += ip.total_length;

	}
	fclose (fp);
	exit (-1);
	return 1;
}

/*
** FUNCTION:	ExtractOptions
**
** DESCRIPTION:	Parses the command line and extracts options.
**              Defaults (where applicable) are provided for parameters that are not supplied
**
** RETURNS:		boolean status
*/
static int ExtractOptions(int argc, char *argv[], OPTIONS_S *args)
{
	int   i;
	char  *cp;
	
	//Set up defaults (no defaults for filenames)
	args->input_mode    = DEF_INPUT_MODE;
	args->output_mode   = DEF_OUTPUT_MODE;
	args->output_format = DEF_OUTPUT_FORMAT;
	args->delay         = DEF_DELAY;
	args->localhost     = DEF_LOCALHOST;
	args->length        = DEF_LENGTH;
	log_level           = DEF_LOG_LEVEL;

	//Walk command line extracting arguments
	i = 1; //Skip first arg
	while (i < argc)
	{
		//check for - at start of option - error if not present
		cp = argv[i];
		if (*cp++ != '-')
		{
			dbg_printf(SEVERE_LOG, "invalid option  %s\n", argv[i]);
			return 0;
		}

		//Process options
		switch (*cp)
		{
		case 'i':
			//Input mode
			++cp;
			if ((*cp == 'u') || (*cp == 'U'))
			{
				args->input_mode = eUSB_IN;
			}
			else if ((*cp == 'f') || (*cp == 'F'))
			{
				//File input, next argument should be filename
				args->input_mode = eFILE_IN;
				if (++i >= argc)
				{
					dbg_printf(FATAL_LOG, "no input file specified\n");
					exit (-1);
				}
				strcpy(args->in_fname, argv[i]);
			}
			else
			{
				dbg_printf(SEVERE_LOG, "invalid input mode %c\n", *cp);
				return 0;
			}
			break;
		case 'o':
			//Output mode
			++cp;
			if ((*cp == 's') || (*cp == 'S'))
			{
				args->output_mode = eSOCKET_OUT;
			}
			else if ((*cp == 'f') || (*cp == 'F'))
			{
				//File output, next argument should be filename
				args->output_mode = eFILE_OUT;
				if (++i >= argc)
				{
					dbg_printf(SEVERE_LOG, "no output file specified\n");
					return 0;
				}
				strcpy(args->out_fname, argv[i]);
			}
			else
			{
				dbg_printf(SEVERE_LOG, "invalid output mode %c\n", *cp);
				return 0;
			}
			break;
		case 'f':
			//Output mode
			++cp;
			if ((*cp == 'r') || (*cp == 'R'))
			{
				args->output_format = eRAW_OUT;
			}
			else if ((*cp == 'i') || (*cp == 'I'))
			{
				//File output, next argument should be filename
				args->output_format = eIP_OUT;
			}
			else if ((*cp == 'u') || (*cp == 'U'))
			{
				//File output, next argument should be filename
				args->output_format = eUDP_OUT;
			}
			else
			{
				dbg_printf(SEVERE_LOG, "invalid output format %c\n", *cp);
				return 0;
			}
			break;
		case 'd':
			//Delay in ms when reading from file
			if (++i >= argc)
			{
				dbg_printf(SEVERE_LOG, "no delay specified\n");
				return 0;
			}
			sscanf(argv[i], "%d", &args->delay);
			break;
		case 'h':
			//Local host flag
			if (++i >= argc)
			{
				dbg_printf(SEVERE_LOG, "no local host flag\n");
				return 0;
			}
			sscanf(argv[i], "%d", &args->localhost);
			break;
		case 'l':
			//Length (in bytes) to play for
			if (++i >= argc)
			{
				dbg_printf(SEVERE_LOG, "no length\n");
				return 0;
			}
			sscanf(argv[i], "%ld", &args->length);
			break;
		case 'v':
			//verbose mode
			log_level = ALL_LOG;
			break;
		case 'q':
			//quiet mode
			log_level = SEVERE_LOG;
			break;
		case '?':
			//Help so just return 0 (help printed above)
			return 0;
		default:				
			printf("unknown option %s\n", argv[i]);
			return 0;
		}
		++i;
	}

	return 1;
}


/*
** FUNCTION:	PrintHelp
**
** DESCRIPTION:	Displays command line options
**
** RETURNS:		none
*/
static void PrintHelp()
{
	printf("usb_ip_player [options]\n");
	printf("-iF <filename>    read input from file <filename>\n");
	printf("-iU               read input from USB port\n");
	printf("-oF <filename>    write output to file <filename>\n");
	printf("-oS               write output as UDP to socket\n");
	printf("-fR               write file output as raw bytes (no IP parsing\n");
	printf("-fI               write file output as parsed IP\n");
	printf("-v                verbose mode. All information messages displayed\n");
	printf("-q                quiet mode. Only serious warnings displayed\n");
	printf("-d <delay in ms>  playback delay to throttle file playback\n");
	printf("-h <1|0>          local host address used if 1\n");
	printf("-l <0|bytes>      playback length (bytes), infinite is 0\n\n");
	printf("default options if not specified:\n");
	printf("  -iU -oS -fR -d 40 -h 1 -l 0\n");

	return;
}

/*
** FUNCTION:	dbg_printf
**
** DESCRIPTION:	Debug printf which selectivity writes log messages based on severity
**
** RETURNS:		number of chararcters written out
*/
int dbg_printf(int severity_level, char *formatstring, ...) 
{
   va_list args;
   va_start(args, formatstring);

   if (severity_level >= log_level)
      return(vfprintf(stderr, formatstring, args));
   else
      return 0;
}

unsigned int port_connected(unsigned short port, int *index)
{
	int i;
	if (connected==0) {
		*index=0;	
		return 0;
	}
	for (i=0;i<connected;i++) {
		if (sockets_port[i]==port) {
		*index=i;
		return 1;
		}
	}
	*index=connected;
	return 0;
}