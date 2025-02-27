#include "dionetlib.h"

#define MAX_CLIENTS

int main(int argc, char* argv[]){
	s_init();
	
	create_server_socket("8787");
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
			if(clients[i] > s_socket_default)
				FD_SET(clients[i],&readfds);
			if(clients[i] > maxfd)
				maxfd=clients[i];
		}
		
		// check for activity in all sockets, if error, handle it
		int activity=select(maxfd+1,&readfds,NULL,NULL,NULL);
		if((activity < 0) && (s_errno != s_EINTR)) printf("select error!\n");
		
		// check if new incoming client
		if(FD_ISSET(listen_socket, &readfds)){
			// accept the client
			s_socket new_socket=s_socket_default;
			accept_server_socket(new_socket);
			if(!s_success) { perror("failed to accept client!\n"); s_quit(); }
			printf("new client connected!\n");
			
			// greet the client
			char greet_msg[]="Connected to server! :)";
			s_send(new_socket,greet_msg,sizeof(greet_msg));
			if(!s_success) { perror("failed to send greet msg!\n"); }
			printf("Greeting sent!\n");
			
			// find empty space to put the new client in
			for(int i=0; i<MAX_CLIENTS; i++){
				if(clients[i] == s_socket_default){
					clients[i]=new_socket;
					printf("Set new client to id: %d\n",i);
				}
			}
		}
	}
	
	s_quit();
}