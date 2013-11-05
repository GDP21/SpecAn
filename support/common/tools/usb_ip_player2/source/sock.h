typedef struct
{
	SOCKADDR_IN address;
	SOCKET handle;
} SOCK_T;

int SOCK_open(SOCK_T *connection, int address, unsigned short port);
int SOCK_write(SOCK_T *connection, unsigned char *buffer, int length);
void SOCK_close(SOCK_T *connection);
int SOCK_startup(void);
void SOCK_shutdown(void);