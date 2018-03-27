#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_TCP_PORT		7001	// Default port
#define BUFLEN			80  	// Buffer length

int clientSocket;
int connected;


void *SendChat(void *arg);
void InitializeClientSocket(char *host, int port);
void ReceiveChat();

char * name;




int main (int argc, char **argv)
{
	int port;
	char  *host;
	pthread_t inputThread;

	switch(argc)
	{
		case 2:
			host =	argv[1];	// Host name
			port =	SERVER_TCP_PORT;
		break;
		case 3:
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
		break;
		case 4:
			name = argv[3];
		default:
			fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
			exit(1);
	}

	InitializeClientSocket(host, port);

    pthread_create(&inputThread, NULL, SendChat, NULL);
   	
   	ReceiveChat();

   	pthread_join(inputThread, NULL);
	
	return (0);
}



void InitializeClientSocket(char *host,int port) {

	char **pptr;
	struct hostent	*hp;
	struct sockaddr_in server;
	char str[16];
	char clientName[BUFLEN];

	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Cannot create socket");
		exit(1);
	}
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL) {
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	
	if (connect (clientSocket, (struct sockaddr *)&server, sizeof(server)) == -1) {
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}


	strcpy(clientName, name);


	send (clientSocket, clientName, BUFLEN, 0);






	printf("Connected:    Server Name: %s\n", hp->h_name);
	pptr = hp->h_addr_list;
	printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));

	connected = 1;
}







void *SendChat(void *arg)
{
	char buf[BUFLEN];

	while (connected) {
		fgets(buf, BUFLEN, stdin);
		printf("SEND: %s", buf);
		fflush(stdout);
		send (clientSocket, buf, BUFLEN, 0);
	}
   
}




void ReceiveChat() {
	char *buf_ptr;
	char buf[BUFLEN];
	int bytes_to_read;
	int recv_bytes;

	buf_ptr = buf;
	bytes_to_read = BUFLEN;

	while (connected) {
		recv_bytes = 0;
		bytes_to_read = BUFLEN;
		while ((recv_bytes = recv (clientSocket, buf_ptr, bytes_to_read, 0)) < BUFLEN)
		{
			buf_ptr += recv_bytes;
			bytes_to_read -= recv_bytes;
		}
		printf ("RECV: %s", buf);
		fflush(stdout);
	}

	fflush(stdout);
	close (clientSocket);

}








































