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
#define BUFLEN	80		//Buffer length
#define TRUE	1
#define MAXLINE 4096
#define MAXCLIENTS 10


// Function Prototypes
static void SystemFatal(const char* );
int CreateServerSocket();
int CheckSockets();
void SendToAll(char * buf);



int clientSockets[FD_SETSIZE];
int clients;
fd_set rset, allset;



int main (int argc, char **argv)
{

	int serverSocket;
	int maxSockNum;
	int newConnection;
	int nready;
	int client_len;
	struct sockaddr_in client_addr;
   	int i;

	serverSocket = CreateServerSocket();

	for (i = 0; i < FD_SETSIZE; i++) {
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
   		printf("triggered\n");

  		if (FD_ISSET(serverSocket, &rset)) 
  		{		
  			
  			client_len = sizeof(client_addr);
  			if ((newConnection = accept(serverSocket, (struct sockaddr *) &client_addr, &client_len)) == -1) {
  				SystemFatal("accept error");
  			}

  			char temp[100];
  			sprintf(temp, "New User Connected: %d", newConnection);
 			SendToAll(temp);

  			fcntl(newConnection, F_SETFL, O_NONBLOCK);

  			printf(" Remote Address:  %s\n", inet_ntoa(client_addr.sin_addr));

  			for (i = 0; i < FD_SETSIZE; i++) {
				if (clientSockets[i] < 0) {
					clientSockets[i] = newConnection;	// save descriptor
					break;
				}
				
				if (i == FD_SETSIZE) {
					printf ("Too many clients\n");
					exit(1);
				}
  			}
  				

			FD_SET(newConnection, &allset);     // add new descriptor to set
			if (newConnection > maxSockNum) {
				maxSockNum = newConnection;	
			}

			if (i > clients) {
				clients = i;	// new max index in client[] array
			}

			if (--nready <= 0) {
				continue;	// no more readable descriptors
			}
		} else {
			nready = CheckSockets(nready);
		}

		

		
	}

	return 1;
}



int CheckSockets(int nready) {
	int count = 0;
	char *bufp, buf[BUFLEN];
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
			bytes_to_read = BUFLEN;
			
			while ((n = recv(sockfd, bufp, bytes_to_read, 0)) < BUFLEN && n != 0) {
				printf(" hi %d\n", n);
				//n = read(sockfd, bufp, bytes_to_read);
				bufp += n;
				bytes_to_read -= n;
			}

			// printf("bytes %d\n", bytes_to_read);

			if (n != 0) {
				printf("RECV: %s\n", bufp);
				SendToAll(bufp);
			}
			
			// printf("RECV: %s\n", bufp);
			// SendToAll(bufp);

			if (n == 0) {
				char temp[100];
	  			sprintf(temp, "User Exit: %d", sockfd);
	 			SendToAll(temp);
				close(sockfd);
				FD_CLR(sockfd, &allset);
				clientSockets[i] = -1;
			}
 
			if (--nready <= 0){
				break;  
			}
		}
	}
	return nready;

}



void SendToAll(char * buf) {

	int socket;
	for (int i = 0; i <= clients; i++) {

		if ((socket = clientSockets[i]) < 0) {
			continue;
		}
		write(socket, buf, BUFLEN);   // echo to client

	}

	printf("Sent: %s\n", buf);
}













int CreateServerSocket() {

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

	if (bind(socketID, (struct sockaddr *)&server, sizeof(server)) == -1)
		SystemFatal("bind error");
	
	listen(socketID, 10);

	return socketID;
}
















// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message)
{
	perror (message);
	exit (EXIT_FAILURE);
}
