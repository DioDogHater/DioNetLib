// samples/multiclient/server.c

/*
	This sample shows how we can use dionetlib to make a multi-client server without using threads.
	It uses the select() function and fd_set structures as is, because they are the same in both Windows and Linux.
	If you want to test the server more accurately, please use the telnet command instead of client.c, since it can only send one message, then listen.
*/

#include "dionetlib.h"

#define MAX_CLIENTS 20

#define PORT "8787"

int main(int argc, char* argv[]){
	s_init();
	
	create_server_socket(PORT);
	bind_server_socket;
	listen_server_socket;
	
	// array to hold clients and maxfd which will be used as the nfds argument in select()
	s_socket clients[MAX_CLIENTS];
	s_socket maxfd;
	
	// set all clients sockets to 0
	for(int i=0; i<MAX_CLIENTS; i++) { clients[i]=s_socket_default; }
	
	// fd_set used to check which socket has activity
	fd_set readfds;
	
	char buffer[513];
	
	while(true){
		// set again all valid sockets in readfds
		FD_ZERO(&readfds);
		FD_SET(listen_socket,&readfds);
		for(int i=0; i<MAX_CLIENTS; i++){
			if(s_isvalid(clients[i])){
				FD_SET(clients[i],&readfds);
				if(clients[i] > maxfd)
					maxfd=clients[i];
			}
		}
		
		// check for activity in all sockets, if error, handle it
		int activity=select(maxfd+1,&readfds,NULL,NULL,NULL);
		if((activity < 0) && (s_errno != s_EINTR)) perror("select error!\n");
		
		// check if new incoming client
		if(FD_ISSET(listen_socket, &readfds)){
			// accept the client
			s_socket new_socket=s_socket_default;
			accept_server_socket(new_socket);
			if(!s_success) fprintf(stderr,"failed to accept client!\n");
			else{
				printf("new client connected!\n");
				// greet the client
				char greet_msg[]="connected to server! :)";
				s_send(new_socket,greet_msg,sizeof(greet_msg));
				if(!s_success) { fprintf(stderr,"failed to send greet msg!\n"); }
				printf("greeting sent!\n");
				
				// find empty space to put the new client in
				for(int i=0; i<MAX_CLIENTS; i++){
					if(clients[i] == s_socket_default){
						clients[i]=new_socket;
						printf("set new client to id: %d\n",i);
						break;
					}
				}
			}
		}
		// check if we get a message from clients
		for(int i=0; i<MAX_CLIENTS; i++){
			if(FD_ISSET(clients[i],&readfds)){
				bzero(buffer,sizeof(buffer));
				s_recv(clients[i],buffer,sizeof(buffer)-1);
				if(s_recv_result <= 0){
					printf("client #%d disconnected!\n",i);
					closesocket(clients[i]);
					clients[i]=s_socket_default;
				}else{
					buffer[s_recv_result] = '\0';
					printf("client #%d: %s\n",i,buffer);
					// send message to all other clients
					for(int y=0; y<MAX_CLIENTS; y++){
						if(y == i) continue;
						if(s_isvalid(clients[y])) { s_send(clients[y],buffer,strlen(buffer)); }
					}
				}
			}
		}
	}
	
	closesocket(listen_socket);
	for(int i=0; i<MAX_CLIENTS; i++){
		if(s_isvalid(clients[i]))
			closesocket(clients[i]);
	}
	
	s_quit();
}