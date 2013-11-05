/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tools/usb_ip_player2/source/ip.c,v $
**
** TITLE:       IP
**
** AUTHOR:      Ensigma.
**
** DESCRIPTION: IP and UDP Header extraction and check sum validation
**
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#include "stdint.h"
#include "ip.h"


/*
** FUNCTION:	IP_readHeader
**
** DESCRIPTION:	Reads IP header, and validates checksum
**
** PARAMETERS:  
**              buffer - contains header to be read 
**              length - number of bytes in buffer
**             *header - structure to hold parsed header 
**
** RETURNS:		0 if checksum fails or length is too short .
*/
int IP_readHeader(uint8_t *buffer, int length, IP_HEADER_T *header)
{
	unsigned char *word16;
	unsigned char *word32;

	//Check for valid header by length
	if (length < 20) 
		return 0;

	//Validate checksum
	if (!ValidatePacketChecksum(buffer, length, (char *)0, 0))
		return 0;

	header->version = (buffer[0] & 0xF0) >> 4;
	
	header->IHL = (buffer[0] & 0x0F);

	header->type_of_service = buffer[1];

	word16 = (unsigned char *)&(header->total_length);
	word16[0] = buffer[3];
	word16[1] = buffer[2];

	word16 = (unsigned char *)&(header->identification);
	word16[0] = buffer[5];
	word16[1] = buffer[4];

	header->flags = (buffer[6] & 0xE0) >> 5;

	word16 = (unsigned char *)&(header->fragment_offset);
	word16[0] = buffer[7];
	word16[1] = buffer[6]; word16[1] &= 0x1F;

	header->time_to_live = buffer[8];

	header->protocol = buffer[9];

	word16 = (unsigned char *)&(header->header_checksum);
	word16[0] = buffer[11];
	word16[1] = buffer[10];

	word32 = (unsigned char *)&(header->source_address);
	word32[0] = buffer[15];
	word32[1] = buffer[14];
	word32[2] = buffer[13];
	word32[3] = buffer[12];

	word32 = (unsigned char *)&(header->destination_address);
	word32[0] = buffer[19];
	word32[1] = buffer[18];
	word32[2] = buffer[17];
	word32[3] = buffer[16];

	header->data = &(buffer[4 * (header->IHL)]);

	return 1;
}

/*
** FUNCTION:	UDP_readHeader
**
** DESCRIPTION:	Reads UDP header
**
** PARAMETERS:  
**              buffer - contains header to be read 
**              length - number of bytes in buffer
**              *header - structure to hold parsed header 
**
** RETURNS:		Boolean status (only fails if length is too short)
*/
int UDP_readHeader(uint8_t *buffer, int length, UDP_HEADER_T *header)
{
	unsigned char *word16;

	if (length < 8) return 0;

	word16 = (unsigned char *)&(header->source_port);
	word16[0] = buffer[1];
	word16[1] = buffer[0];

	word16 = (unsigned char *)&(header->destination_port);
	word16[0] = buffer[3];
	word16[1] = buffer[2];

	word16 = (unsigned char *)&(header->length);
	word16[0] = buffer[5];
	word16[1] = buffer[4];

	word16 = (unsigned char *)&(header->checksum);
	word16[0] = buffer[7];
	word16[1] = buffer[6];

	header->data = &(buffer[8]);

	return 1;
}

/*
** FUNCTION:	ValidatePacketChecksum
**
** DESCRIPTION:	Computes IP checksum and confirms it is 0.
**              Up to two buffers can be provided and checsum is performed across both buffers
**              Buffers must be zero padded to 16-bits
**
** PARAMETERS:  
**              buffer1 - contains first chunk of data to be check-summed 
**              length1 - number of bytes in buffer1
**              buffer2 - contains second to be check-summed, 0 if it is to be skipped 
**              length2 - number of bytes in buffer2 (0 if no data)
**
** RETURNS:		Boolean status - 1 if chesum valid, 0 if fails
*/
int ValidatePacketChecksum(unsigned char *buffer,  int length, 
						   unsigned char *buffer2, int length2)	
{
	unsigned char *pByte;
	int            i;
	unsigned short checksum;
	unsigned long  sum;
	unsigned char *word16;
	unsigned short temp;

	//compute checksum across first chunk
	word16 = (unsigned char *)&(temp);
	for (i = length, sum = 0, pByte = buffer;  i > 1; i -= 2, pByte +=2)
	{
			if (i >= 2)
			    word16[0] = *(pByte+1);
			else
				word16[0] = 0;
		word16[1] = *pByte;
		sum  += (temp);
    }

	//Extend checksum across second chuck if valid
	if (length2 != 0)
	{
		for (i = length2, pByte = buffer2;  i > 0; i -= 2, pByte +=2)
		{
			if (i >= 2)
			    word16[0] = *(pByte+1);
			else
				word16[0] = 0;
			word16[1] = *pByte;
			sum  += (temp);
		}
	}

    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16)
       sum = (sum & 0xffff) + (sum >> 16);
    checksum = (unsigned short)(~sum);

	//Compare checksum to 0 and return result
	return (checksum == 0 ? 1 : 0);
}


