#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

void fFAIL()
{
	getch();
	exit(-1);
}

#define	FAIL(Text,...)	printf(Text,__VA_ARGS__);	fFAIL();

typedef enum
{
	DISEQC_TEST_TOOL_TOKEN_NOPOWER = 0,
	DISEQC_TEST_TOOL_TOKEN_SILENCE,
	DISEQC_TEST_TOOL_TOKEN_TONE,
	DISEQC_TEST_TOOL_TOKEN_MESSAGE,
	DISEQC_TEST_TOOL_TOKEN_UNKNOWN
}DISEQC_TEST_TOOL_TOKEN_T;

typedef struct 
{
	DISEQC_TEST_TOOL_TOKEN_T	Token;
	union
	{
		BYTE					aui8Message[8];
		DWORD					ui32Lengthms;
	};
}DISEQC_TEST_TOOL_TOKEN;

void WriteSerial(HANDLE hSerial, char *pszData)
{
	DWORD	ui32Written;
	DWORD	ui32Size;
	char	*pszEnd;

	ui32Size = strlen(pszData);
	pszEnd = pszData + ui32Size;

	while(pszData<pszEnd)
	{
		Sleep(10);
		if((!WriteFile(hSerial,pszData++,1,&ui32Written,NULL))||(ui32Written!=1))
		{
			FAIL("ERROR: failed to write data to serial port: %s\n",pszData);
		}
	}
}

void ReadSerial(HANDLE hSerial, DISEQC_TEST_TOOL_TOKEN *psToken, BYTE ui8ExpectedHexLength)
{
	char		c;
	DWORD		ui32Read;
	BYTE		bReceivingSilence = 0;
	BYTE		bReceivingTone = 0;
	BYTE		bReceivingHex = 0;
	BYTE		ui8HexIndex;
	BYTE		bHexHigh;
	DWORD		ui32Temp;
	
	while(1)
	{
		if((!ReadFile(hSerial,&c,1,&ui32Read,NULL))||(ui32Read!=1))
		{
			FAIL("ERROR: failed to read data from serial port\n");
		}
		//printf("%c",c);
		switch(c)
		{
		case '~':
		case '^':
			psToken->Token = DISEQC_TEST_TOOL_TOKEN_TONE;
			psToken->ui32Lengthms = 1000;
			return;
		case '-':
		case '=':
			psToken->Token = DISEQC_TEST_TOOL_TOKEN_SILENCE;
			psToken->ui32Lengthms = 1000;
			return;
		case '_':
			psToken->Token = DISEQC_TEST_TOOL_TOKEN_NOPOWER;
			return;
		case '\'':
		case '\"':
			if(bReceivingTone)
			{
				psToken->Token = DISEQC_TEST_TOOL_TOKEN_TONE;
				psToken->ui32Lengthms = ui32Temp;
				return;
			}
			else
			{
				bReceivingTone = 1;
				ui32Temp = 0;
			}
			break;
		case '<':
		case '>':
			if(bReceivingSilence)
			{
				psToken->Token = DISEQC_TEST_TOOL_TOKEN_SILENCE;
				psToken->ui32Lengthms = ui32Temp;
				return;
			}
			else
			{
				bReceivingSilence = 1;
				ui32Temp = 0;
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(bReceivingSilence || bReceivingTone)
			{
				ui32Temp *= 10;
				ui32Temp += (c-'0');
				break;
			}
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if((c>='0')&&(c<='9'))
			{
				ui32Temp = c-'0';
			}
			else if((c>='a')&&(c<='f'))
			{
				ui32Temp = (c-'a')+10;
			}
			else
			{
				ui32Temp = (c-'A')+10;
			}

			if(!bReceivingHex)
			{
				bReceivingHex = 1;
				ui8HexIndex = 0;
				bHexHigh = 1;
			}
			if(bHexHigh)
			{
				psToken->aui8Message[ui8HexIndex] = ui32Temp<<4;
				bHexHigh = 0;
			}
			else
			{
				psToken->aui8Message[ui8HexIndex] |= ui32Temp;
				bHexHigh = 1;
				ui8HexIndex++;
				if(ui8HexIndex==ui8ExpectedHexLength)
				{
					psToken->Token = DISEQC_TEST_TOOL_TOKEN_MESSAGE;
					return;
				}
			}
			break;
		default:
			break;
		}
	}
}

void ReceiveToken(HANDLE hSerial, DISEQC_TEST_TOOL_TOKEN_T Token, DISEQC_TEST_TOOL_TOKEN *psToken, DWORD ui32Timeout, BYTE ui8ExpectedHexLength)
{
	DWORD	ui32Time = GetTickCount();

	while(1)
	{
		if(psToken->Token == Token)
		{
			break;
		}
		else if(psToken->Token != DISEQC_TEST_TOOL_TOKEN_SILENCE)
		{
			FAIL("ERROR: Received unexpected item\n");
		}

		if((GetTickCount() - ui32Time) > ui32Timeout)
		{
			FAIL("ERROR: Did not receive expected item\n");
		}

		ReadSerial(hSerial,psToken,ui8ExpectedHexLength);
	}
}

unsigned long long g_ui64RandX = 0;
unsigned long long g_ui64RandXC = 0;

void SRand(unsigned long ui32X)
{
	g_ui64RandX = ui32X;

	g_ui64RandXC = g_ui64RandX>>1;
}

DWORD Rand(unsigned long ui32Max)
{
	unsigned long long	ui64Temp;

	ui64Temp = ((g_ui64RandX * 1103515245) + g_ui64RandXC);
	g_ui64RandX = ui64Temp & 0xFFFFFFFF;
	g_ui64RandXC = ui64Temp >> 32;
	return (unsigned long)(g_ui64RandX % (ui32Max+1));
}

void Random(BYTE *pui8Buf, DWORD ui32Size)
{
	BYTE	*pui8End = pui8Buf + ui32Size;

	while(pui8Buf < pui8End)
	{
		*pui8Buf++ = Rand(255);
	}
}

void HexToText(unsigned char *pui8Data, unsigned long ui32DataSize, char *pszOutputText, unsigned long ui32SpaceFrequency, unsigned long ui32NewlineFrequency, BOOL bZeroX, char *pszSeparator)
{
	unsigned long	i;
	char			aszHex[17] = "0123456789abcdef";
	unsigned long	ui32Space = 0;
	unsigned long	ui32Newline = 0;
	char			*Ptr = pszOutputText;
	unsigned long	ui32SeparatorLen;

	if(pszSeparator)
	{
		ui32SeparatorLen = strlen(pszSeparator);
	}

	for(i=0;i<ui32DataSize;i++)
	{
		if(ui32Space == ui32SpaceFrequency){*Ptr++ = ' '; ui32Space = 0;}
		ui32Space++;
		if(ui32Newline == ui32NewlineFrequency){*Ptr++ = '\n'; ui32Newline = 0;}
		ui32Newline++;

		if(bZeroX)
		{
			*Ptr++ = '0';
			*Ptr++ = 'x';
		}
		*Ptr++ = aszHex[(pui8Data[i]>>4)&0xF];
		*Ptr++ = aszHex[(pui8Data[i])&0xF];
		if(pszSeparator)
		{
			memcpy(Ptr,pszSeparator,ui32SeparatorLen);
			Ptr += ui32SeparatorLen;
		}
	}
	if(pszSeparator && i)
	{
		Ptr -= ui32SeparatorLen;
	}
	*Ptr++ = 0;
}


int main()
{
	char					*pszSerial = "COM1";
	HANDLE					hSerial;
	DCB						sDCB;
	DISEQC_TEST_TOOL_TOKEN	sToken;
	DWORD					ui32Time;
	BYTE					bNoPowerMessageIssued = 0;
	BYTE					bSilenceMessageIssued = 0;
	DWORD					i;
	BYTE					ui8Len;
	BYTE					aui8SendBuf[255];
	char					aszSendBuf[513];
	BYTE					First;

	printf("\n");
	printf("~ DiSEqC autotest ~\n");
	printf("\n");

	printf("Initialising the DiSEqC test tool via serial %s...\n",pszSerial);
	
	hSerial = CreateFileA(pszSerial,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);

	if(hSerial==INVALID_HANDLE_VALUE)
	{
		FAIL("ERROR: Cannot access serial port %s\n",pszSerial);
	}

	memset(&sDCB,0,sizeof(DCB));
	if(!BuildCommDCBA("baud=9600 parity=n data=8 stop=1 rts=on",&sDCB))
	{
		FAIL("ERROR: Failed to initialise serial DCB\n");
	}

	WriteSerial(hSerial,"ZU8V0VEM0 ");

	do
	{
		ui32Time = GetTickCount();
		ReadSerial(hSerial,&sToken,0);
	}while((GetTickCount() - ui32Time) < 100);

	while(1)
	{
		if(sToken.Token == DISEQC_TEST_TOOL_TOKEN_NOPOWER)
		{
			bSilenceMessageIssued = 0;
			if(!bNoPowerMessageIssued)
			{
				printf("\nNo power on the DiSEqC bus (turn the power on).");
				bNoPowerMessageIssued = 1;
			}
			else
			{
				printf(".");
			}
		}
		else if(sToken.Token == DISEQC_TEST_TOOL_TOKEN_SILENCE)
		{
			bNoPowerMessageIssued = 0;
			if(!bSilenceMessageIssued)
			{
				printf("\nSilence on the DiSEqC bus (start the meta test).");
				bSilenceMessageIssued = 1;
			}
			else
			{
				printf(".");
			}
		}
		else
		{
			break;
		}
		ReadSerial(hSerial,&sToken,0);
	}

	printf("\n");
	printf("\n");

	printf("Receiving continuous tone (5s)...");

	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_TONE,&sToken,5000,0);

	ui32Time = GetTickCount();

	while(1)
	{
		if(sToken.Token == DISEQC_TEST_TOOL_TOKEN_SILENCE)
		{
			break;
		}
		else if(sToken.Token != DISEQC_TEST_TOOL_TOKEN_TONE)
		{
			FAIL("ERROR: Received unexpected item\n");
		}

		if((GetTickCount() - ui32Time) > 5500)
		{
			FAIL("ERROR: continuous tone too long\n");
		}

		ReadSerial(hSerial,&sToken,0);
	}

	printf("OK\n");
	printf("\n");

	printf("Receiving tone burst A (x100)...");

	for(i=0;i<100;i++)
	{
		ReadSerial(hSerial,&sToken,0);

		ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_TONE,&sToken,5000,0);

		if(sToken.ui32Lengthms<11)
		{
			FAIL("ERROR: tone burst A too short\n");
		}
		else if(sToken.ui32Lengthms==1000)
		{
			i--;
		}
		else if(sToken.ui32Lengthms>13)
		{
			FAIL("ERROR: tone burst A too long\n");
		}

		printf("%3u\b\b\b",i);
	}

	printf("OK \n");
	printf("\n");

	printf("Receiving tone burst B (x100)...");

	for(i=0;i<100;i++)
	{
		ReadSerial(hSerial,&sToken,0);

		ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,1);

		if(sToken.aui8Message[0]!=0xFF)
		{
			FAIL("ERROR: received unexpected item\n");
		}

		printf("%3u\b\b\b",i);
	}
	
	

	printf("OK \n");
	printf("\n");

	ReadSerial(hSerial,&sToken,0);

	printf("Receiving 100 messages...");

	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\x00\x00\x00\x00\x00\x00\x00\x00",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data received not equal to expected data\n");
	}

	ReadSerial(hSerial,&sToken,0);

	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data received not equal to expected data\n");
	}

	SRand(0x12345678);
	for(i=0;i<98;i++)
	{
		ui8Len = Rand(7)+1;
		Random(aui8SendBuf,ui8Len);

		ReadSerial(hSerial,&sToken,0);

		ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,ui8Len);

		if(memcmp(aui8SendBuf,sToken.aui8Message,ui8Len))
		{
			FAIL("ERROR: Data received not equal to expected data\n");
		}

		printf("%3u\b\b\b",i+2);
	}

	printf("OK \n");
	printf("\n");

	printf("Receiving and replying to 100 messages...");

	ReadSerial(hSerial,&sToken,0);
	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\x00\x00\x00\x00\x00\x00\x00\x00",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data received not equal to expected data\n");
	}

	WriteSerial(hSerial,"0000000000000000");

	ReadSerial(hSerial,&sToken,0);
	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\x00\x00\x00\x00\x00\x00\x00\x00",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data sent not equal to expected data\n");
	}

	ReadSerial(hSerial,&sToken,0);
	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data received not equal to expected data\n");
	}

	WriteSerial(hSerial,"FFFFFFFFFFFFFFFF");

	ReadSerial(hSerial,&sToken,0);
	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data sent not equal to expected data\n");
	}

	SRand(0x87654321);
	for(i=0;i<98;i++)
	{
		ui8Len = Rand(7)+1;
		Random(aui8SendBuf,ui8Len);

		ReadSerial(hSerial,&sToken,0);

		ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,ui8Len);

		if(memcmp(aui8SendBuf,sToken.aui8Message,ui8Len))
		{
			FAIL("ERROR: Data received not equal to expected data\n");
		}

		ui8Len = Rand(7)+1;
		Random(aui8SendBuf,ui8Len);

		HexToText(aui8SendBuf,ui8Len,aszSendBuf,0xFFFFFFFF,0xFFFFFFFF,FALSE,NULL);
		if(ui8Len!=8)
		{
			strcat(aszSendBuf,"\r");
		}
		WriteSerial(hSerial,aszSendBuf);

		ReadSerial(hSerial,&sToken,0);

		ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,ui8Len);

		if(memcmp(aui8SendBuf,sToken.aui8Message,ui8Len))
		{
			FAIL("ERROR: Data sent not equal to expected data\n");
		}

		printf("%3u\b\b\b",i+2);
	}

	printf("OK \n");
	printf("\n");

	Sleep(1000);

	printf("Sending 100 unexpected messages...");

	WriteSerial(hSerial,"0000000000000000");

	ReadSerial(hSerial,&sToken,0);
	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\x00\x00\x00\x00\x00\x00\x00\x00",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data sent not equal to expected data\n");
	}

	WriteSerial(hSerial,"FFFFFFFFFFFFFFFF");

	ReadSerial(hSerial,&sToken,0);
	ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,8);
	if(memcmp("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",sToken.aui8Message,8))
	{
		FAIL("ERROR: Data sent not equal to expected data\n");
	}

	SRand(0x55555555);
	for(i=0;i<98;i++)
	{
		ui8Len = Rand(7)+1;
		Random(aui8SendBuf,ui8Len);

		HexToText(aui8SendBuf,ui8Len,aszSendBuf,0xFFFFFFFF,0xFFFFFFFF,FALSE,NULL);
		if(ui8Len!=8)
		{
			strcat(aszSendBuf,"\r");
		}
		WriteSerial(hSerial,aszSendBuf);

		ReadSerial(hSerial,&sToken,0);

		ReceiveToken(hSerial,DISEQC_TEST_TOOL_TOKEN_MESSAGE,&sToken,5000,ui8Len);

		if(memcmp(aui8SendBuf,sToken.aui8Message,ui8Len))
		{
			FAIL("ERROR: Data sent not equal to expected data\n");
		}

		printf("%3u\b\b\b",i+2);

		Sleep(50);
	}

	printf("OK \n");
	printf("\n");

	printf("Check META log output to confirm SUCCESS.\n");

	getch();

	return 0;
}
