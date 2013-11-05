/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tools/usb_ip_player2/source/sock.c,v $
**
** TITLE:       Sock
**
** AUTHOR:      Ensigma.
**
** DESCRIPTION: Socket handling functions
**
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "sock.h"
#include "usb_ip.h"

int SOCK_startup(void)
{
WSADATA data;

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
	{
		dbg_printf(SEVERE_LOG, "WSAStartup failed!\n");
		return 0;
	}
	return 1;
}


/*
** FUNCTION:	SOCK_open
**
** DESCRIPTION:	Opens a datagram socket at a specified address and port
**
** RETURNS:		Boolean success.
*/
int SOCK_open(SOCK_T *connection, int address, unsigned short port)
{

	if ((connection->handle = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		dbg_printf(SEVERE_LOG, "socket() failed: %d\n", WSAGetLastError());
		return 0;
	}

	connection->address.sin_family = AF_INET;
	connection->address.sin_addr.s_addr = htonl(address);
	connection->address.sin_port = htons(port);

	return 1;
}

/*
** FUNCTION:	SOCK_write
**
** DESCRIPTION:	Writes a specified number of bytes to a previously opened socket
**
** RETURNS:		Boolean success.
*/
int SOCK_write(SOCK_T *connection, unsigned char *buffer, int length)
{
	int tx;

	tx = sendto(connection->handle, buffer, length, 0, (SOCKADDR *)&(connection->address), sizeof(connection->address));

	if (tx == SOCKET_ERROR)
	{
		dbg_printf(SEVERE_LOG, "sendto() failed; %d\n", WSAGetLastError());
	}
	else if (tx == 0)
	{
		dbg_printf(SEVERE_LOG, "sendto() is 0\n");
	}

	return tx;
}

/*
** FUNCTION:	SOCK_close
**
** DESCRIPTION:	Close a socket
**
** RETURNS:		None.
*/

void SOCK_close(SOCK_T *connection)
{
	closesocket(connection->handle);
}

void SOCK_shutdown(void)
{
	WSACleanup();
}
