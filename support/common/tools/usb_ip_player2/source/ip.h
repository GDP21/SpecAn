#ifndef _IP_H_LOADED
#define _IP_H_LOADED

//Include int defs used in header structures
#include "stdint.h"

//Defines
#define UDP_PROTOCOL	0x11

//IP header structure
typedef struct
{
	uint8_t version;
	uint8_t IHL;
	uint8_t type_of_service;
	uint16_t total_length;
	uint16_t identification;
	uint8_t flags;
	uint16_t fragment_offset;
	uint8_t time_to_live;
	uint8_t protocol;
	uint16_t header_checksum;
	uint32_t source_address;
	uint32_t destination_address;
	void *data;
} IP_HEADER_T;

//UDP header strcuture
typedef struct
{
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
	void *data;
} UDP_HEADER_T;

//Function prototypes
int IP_readHeader(uint8_t *buffer, int length, IP_HEADER_T *header);
int UDP_readHeader(uint8_t *buffer, int length, UDP_HEADER_T *header);
int ValidatePacketChecksum(unsigned char *buffer,  int length, 
									 unsigned char *buffer2, int length2);

#endif
