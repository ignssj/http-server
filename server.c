#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<pthread.h>

#define CONNMAX 20
#define BYTES 1024
int slot=0;

char *ROOT;
int listenfd, clients[CONNMAX], contador=0;
void error(char *);
void startServer(char *);
void respond(int);
void *cback(void *void_ptr);

int main(int argc, char* argv[])
{
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;    
	
	//Default Values PATH = ~/ and PORT=10000
	char PORT[6];
	ROOT = getenv("PWD");
	strcpy(PORT,"8080");

	//Parsing the command line arguments
	while ((c = getopt (argc, argv, "p:r:")) != -1)
		switch (c)
		{
			case 'r':
				ROOT = malloc(strlen(optarg));
				strcpy(ROOT,optarg);
				break;
			case 'p':
				strcpy(PORT,optarg);
				break;
			case '?':
				fprintf(stderr,"Parametros errados cpx\n");
				exit(1);
			default:
				exit(1);
		}
	
	printf("SERVER TA TIPO NEYMAR. %s%s%s PATH: %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");
	// Setting all elements to -1: signifies there is no client connected
	int i;
	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	startServer(PORT);
	pthread_t thread;
	// ACCEPT connections
	 for (int y=0; y<CONNMAX; y++)                //while(1)
	{
		
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
			error ("Erro ao aceitar a conexao");
		else
		{
				
				if(pthread_create(&thread, NULL, cback, NULL) ==0){
				respond(slot);
				}
				
					
				
			
		}
		close(clients[slot]);
		 slot = (slot+1)%CONNMAX;
	} 
	pthread_exit(NULL);
	
	return 0;
}

void *cback(void *void_ptr)
{
}

//inicia o server
void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

	// getaddrinfo do host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("Erro ao receber o enderecamento");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) break;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("Erro no socket() ou bind()");
		close(listenfd);
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if ( listen (listenfd, 20) != 0 )
	{
		perror("Erro na espera por novas conexoes");
		exit(1);
	}
	
	

}

//client connection
void respond(int n)
{
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
	int rcvd, fd, bytes_read;

	memset( (void*)mesg, (int)'\0', 99999 );

	rcvd=recv(clients[n], mesg, 99999, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("Erro no recebimento\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Conexão encerrada, socket desconectado\n");
	else    // message received
	{
		printf("%s", mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \r\n");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.1", 8)!=0 && strncmp( reqline[2], "HTTP/1.0", 8)!=0 )
			{
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			}
			else
			{
				if ( strncmp(reqline[1], "/\0", 2)==0 )
					reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

				strcpy(path, ROOT);
				strcpy(&path[strlen(ROOT)], reqline[1]);
				printf("file: %s\n", path);

				if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
				{
					send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
					//printf("HTTP/1.0 200 OK\n\n");
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
						write (clients[n], data_to_send, bytes_read);
				}
				else    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
			}
		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
}
