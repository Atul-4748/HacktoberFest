#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
	
#define TRUE 1
#define FALSE 0
#define MAX_INPUT_SIZE 256
// #define PORT 8888
	
int main(int argc , char *argv[])
{
	int opt = TRUE;
	int master_socket, addrlen, new_socket, client_socket[10],max_clients = 10 , activity, i , valread , sd;
	int max_sd;
	struct sockaddr_in address;
		
	char msg[MAX_INPUT_SIZE]; 
	fd_set readfds;
	int port = atoi(argv[1]);
	if (argc < 2) {
		fprintf(stderr,"usage %s <server-port>\n",argv[0]);
		exit(0);
	}

	//a message
	// char *message = "ECHO Daemon v1.0 \r\n";
	
	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
	}
		
	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
	{
		printf("socket failed");
		exit(1);
	}
	
	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,sizeof(opt)) < 0 )
	{
		printf("setsockopt failed\n");
		exit(1);
	}
	
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
		
	//bind the socket to given port
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed\n");
		exit(1);
	}
	printf("Listening on port %d \n", port);
		
	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		printf("listening failed\n");
		exit(1);
	}
		
	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...\n");
		
	while(TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);
	
		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;
			
		//add child sockets to set
		for ( i = 0 ; i < max_clients ; i++)
		{
			//socket descriptor
			sd = client_socket[i];
				
			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);
				
			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}
	
		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
			
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				printf("Failed to accept");
				exit(1);
			}
			
			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
			
			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if( client_socket[i] == 0 )
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n" , i);
						
					break;
				}
			}
		}
			
		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];
				
			if (FD_ISSET(sd , &readfds))
			{
				//Check if it was for closing , and also read the
				//incoming message
				if ((valread = read( sd , msg, 1024)) == 0)
				{
					//Somebody disconnected , get his details and print
					// 	(socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n" ,inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
				}
					
				//Echo back the message that came in
				else
				{
					//set the string terminating NULL byte on the end
					//of the data read
					// msg[valread] = '\0';
					char op = '+';
			     	int l = 0;
			     	int a = 0,b = 0;

			     	for(int i=0;i<strlen(msg)-1;i++){
			     		if(msg[i]=='+' || msg[i]=='-' || msg[i]=='*' || msg[i]=='/'){
			     			op = msg[i];
			     			l = 1;
			     		}else{
			     			if(l==0){
			     				int d = msg[i]-'0';
			     				a = (a*10)+d;
			     			}else{
			     				int d = msg[i]-'0';
			     				b = (b*10)+d;
			     			}
			     		}
			     	} 

			     	int res = 0;

			     	if(op=='+'){
			     		res = a+b;
			     	}else if(op == '-'){
			     		res = a-b;
			     	}else if(op == '*'){
			     		res = a*b;
			     	}else if(op == '/' && b!=0){
			     		res = a/b;
			     	}else res = -1;

			     	printf("sending reply to client %d:%i\n",ntohs(address.sin_port), res);
			     	bzero(msg, MAX_INPUT_SIZE);
                    sprintf(msg, "%d", res);
					send(sd , msg , strlen(msg) , 0);
					bzero(msg, sizeof(msg));
				}
			}
		}
	}
		
	return 0;
}
