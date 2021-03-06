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

#define SERVER_TCP_PORT		7000	
#define SBUFLEN			80  	
#define RBUFLEN			100  	
#define NBUFLEN			20  	


int clientSocket;
int connected;
FILE *fp;


void *SendChat(void *arg);
void InitializeClientSocket(char *host, int port);
void ReceiveChat();

char * username;


int main (int argc, char **argv)
{
	int port;
	char  *host;
	pthread_t inputThread;
	char * fileName;
	fp = NULL;

	switch(argc)
	{
		case 3:
			username = argv[1];
			host = argv[2];	
			port = SERVER_TCP_PORT;
		break;
		case 4:
			username = argv[1];
			host = argv[2];
			port = atoi(argv[3]);	
		break;
		case 5:
			username = argv[1];
			host = argv[2];
			port = atoi(argv[3]);
			fileName = argv[4];
			strcat(fileName, ".txt");
			fp = fopen(fileName, "w+");
		break;	
		default:
			fprintf(stderr, "Usage: %s username host [port] [filename]\n", argv[0]);
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
	char clientName[SBUFLEN];

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
	
	//send server user name
	strcpy(clientName, username);
	send (clientSocket, clientName, SBUFLEN, 0);
	printf("Connected. Enter 'q' to exit chat.\n");
	connected = 1;
}




void *SendChat(void *arg)
{
	char buf[SBUFLEN];
	while (connected) {
		fgets(buf, SBUFLEN, stdin);
		if(strcmp(buf, "q\n") == 0) {
			printf("Disconnected.\n");
			connected = 0;
			break;
		}
		if (fp) {
			fprintf(fp, "%s\n", buf);
		}
		send (clientSocket, buf, SBUFLEN, 0);
	}
	close (clientSocket);
	if (fp) {
		fclose(fp);
	}
	exit(EXIT_SUCCESS);
   	return NULL;
}




void ReceiveChat() {
	char *buf_ptr;
	char buf[RBUFLEN];
	int bytes_to_read;
	int recv_bytes;
	buf_ptr = buf;

	//print anything received to screen
	while (connected) {
		printf("receiving\n");
		recv_bytes = 0;
		bytes_to_read = RBUFLEN;
		while ((recv_bytes = recv (clientSocket, buf_ptr, bytes_to_read, 0)) < RBUFLEN)
		{
			printf("recv_bytes\n");
			buf_ptr += recv_bytes;
			bytes_to_read -= recv_bytes;
		}
		fprintf(fp, "%s\n", buf);
		printf ("%s", buf);
		fflush(stdout);
	}
	
	fflush(stdout);
}








































