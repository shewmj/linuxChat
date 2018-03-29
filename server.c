#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define SERVER_TCP_PORT 7005	// Default port
#define SBUFLEN	100		//Buffer length
#define RBUFLEN	80	
#define TRUE	1
#define MAXLINE 4096
#define MAXCLIENTS 10


// Function Prototypes
static void SystemFatal(const char* );
int InitializeServerSocket();
int AcceptConnection(int serverSocket);
char * GetClientName(int clientSocket);
int CheckSockets();
void SendToAll(char * buf, int client);



char * clientNames[FD_SETSIZE];
int clientSockets[FD_SETSIZE];
int clients;
fd_set rset, allset;



int main (int argc, char **argv)
{

	int serverSocket;
	int maxSockNum;
	int newConnection;
	int nready;

	serverSocket = InitializeServerSocket();

	for (int i = 0; i < FD_SETSIZE; i++) {
		clientSockets[i] = -1;
	}
	FD_ZERO(&allset);
	FD_SET(serverSocket, &allset);
	fcntl(serverSocket, F_SETFL, O_NONBLOCK);
	maxSockNum = serverSocket;
	clients = -1;

	while (TRUE)
	{
   		rset = allset;
   		nready = select(maxSockNum + 1, &rset, NULL, NULL, NULL);
   		
  		if (FD_ISSET(serverSocket, &rset)) 
  		{		
  			newConnection = AcceptConnection(serverSocket);
			if (newConnection > maxSockNum) {
				maxSockNum = newConnection;	
			}
			if (--nready <= 0) {
				continue;	
			}
		} else {
			nready = CheckSockets(nready);
		}
	}


	free(clientNames);
	return 1;
}



int InitializeServerSocket() {

	struct sockaddr_in server;
	int socketID;
	int arg = 1;
	if ((socketID = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		SystemFatal("Cannot Create Socket!");
	}
	if (setsockopt (socketID, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
		SystemFatal("setsockopt");
	}
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_TCP_PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client
	if (bind(socketID, (struct sockaddr *)&server, sizeof(server)) == -1) {
		SystemFatal("bind error");
	}
	listen(socketID, MAXCLIENTS);

	return socketID;
}





int AcceptConnection(int serverSocket) {

	int newConnection;
	int client_len;
	struct sockaddr_in client_addr;
	int i;
	char * clientName;
	client_len = sizeof(client_addr);

	if ((newConnection = accept(serverSocket, (struct sockaddr *) &client_addr, &client_len)) == -1) {
		SystemFatal("accept error");
	}
	fcntl(newConnection, F_SETFL, O_NONBLOCK);
	for (i = 0; i < FD_SETSIZE; i++) {
		if (clientSockets[i] < 0) {
			clientSockets[i] = newConnection;	
			break;
		}
		if (i == FD_SETSIZE) {
			printf ("Too many clients\n");
			exit(1);
		}
	}
	if (i > clients) {
		clients = i;	
	}
	FD_SET(newConnection, &allset);    
	return newConnection;
}



int CheckSockets(int nready) {
	int count = 0;
	char *bufp, buf[RBUFLEN];
	char echo[SBUFLEN];
	char chatUpdate[SBUFLEN];
	int bytes_to_read;
	int sockfd;
	ssize_t n;

	for (int i = 0; i <= clients; i++) {
		if ((sockfd = clientSockets[i]) < 0) {
			continue;
		}

		if (FD_ISSET(sockfd, &rset))
		{
			count++;
			bufp = buf;
			bytes_to_read = RBUFLEN;
			
			while ((n = recv(sockfd, bufp, bytes_to_read, 0)) < RBUFLEN && n != 0) {
				bufp += n;
				bytes_to_read -= n;
			}

			if (n != 0) {
				//first message from client is their name
				if (clientNames[i] == NULL) {
					clientNames[i] = strdup(buf);
					sprintf(chatUpdate, "*%s* connected.\n", clientNames[i]);
					SendToAll(chatUpdate, i);
				} else {
					strcpy(echo, clientNames[i]);
					strcat(echo, ": ");
					strcat(echo, bufp);
					SendToAll(echo, i);
				}
			}
			if (n == 0) {
	  			sprintf(chatUpdate, "*%s* left.\n", clientNames[i]);
	 			SendToAll(chatUpdate, i);
				close(sockfd);
				FD_CLR(sockfd, &allset);
				//clients--;
				free(clientNames[i]);
				clientNames[i] = NULL; 
				clientSockets[i] = -1;
			}
 
			if (--nready <= 0){
				break;  
			}
		}
	}
	return nready;

}


//send to all except one client
void SendToAll(char * buf, int client) {
	int socket;
	for (int i = 0; i <= clients; i++) {
		if ((socket = clientSockets[i]) < 0 || i == client) {
			continue;
		}
		write(socket, buf, SBUFLEN); 
	}
	printf("%s", buf);
	fflush(stdout);
}






static void SystemFatal(const char* message)
{
	perror (message);
	exit (EXIT_FAILURE);
}
