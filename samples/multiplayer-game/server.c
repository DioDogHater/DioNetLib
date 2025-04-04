#include "../../dionetlib.h"
#include <stdint.h>
#include <time.h>

#define PORT "8888"

#define MAX_CLIENTS 30

#define BUFFER_SIZE 256

// Structure to represent a player's data
struct player{
	int8_t id; // if player doesnt exist, id=-1
	int16_t x; // position of the player (updated instantaneously)
	int16_t y;
};
struct player players[MAX_CLIENTS];

// Package types
#define PKG_PLAYER_UPDATE 0x01 // uint8_t pkg_type+(struct player player_info) -> 6 bytes (if player didn't exist, player now exists)
#define PKG_PLAYER_QUIT 0x02   // uint8_t pkg_type+(uint8_t id) -> 2 bytes (sets the player's id to -1)
#define PKG_PLAYER_SETID 0x03  // uint8_t pkg_type+(uint8_t id) -> 2 bytes (sends the player it's assigned id)

char recv_buffer[BUFFER_SIZE];
int recv_buffer_pos=0;
char send_buffer[BUFFER_SIZE];
int send_buffer_pos=0;

// Updates the new socket with the newest information and updates other clients
void update_new_socket(s_socket new_client, s_socket* clients){
	// Get the smallest id with no socket connected
	int new_id=-1;
	for(int i=0; i<MAX_CLIENTS; i++){
		if(!s_isvalid(clients[i])){
			new_id=i;
			clients[i]=new_client;
			players[new_id]=(struct player){new_id,rand()%800-400,0};
			printf("new client has id #%d!\n",i);
			break;
		}
	} if(new_id == -1) { closesocket(new_client); printf("server is full!\n"); return; }
	// Setup the buffer for the update package of the new player
	send_buffer[0]=(char)PKG_PLAYER_UPDATE;
	memcpy(send_buffer+1,&players[new_id],sizeof(struct player));
	send_buffer_pos+=sizeof(struct player)+1;
	// Get all other players' data to send to the new player
	bzero(recv_buffer,BUFFER_SIZE);
	recv_buffer_pos=0;
	recv_buffer[recv_buffer_pos++]=(char)PKG_PLAYER_SETID;
	recv_buffer[recv_buffer_pos++]=(char)new_id;
	for(int i=0; i<MAX_CLIENTS; i++){
		if(players[i].id != -1){ // Gather player's info
			recv_buffer[recv_buffer_pos++]=(char)PKG_PLAYER_UPDATE;
			memcpy(recv_buffer+recv_buffer_pos,&players[i],sizeof(struct player));
			recv_buffer_pos+=sizeof(struct player);
		}
	}s_send(new_client,recv_buffer,recv_buffer_pos);
	if(!s_success) { fprintf(stderr,"failed to send update message to new client!\n"); return; }
}

int main(int argc, char* argv[]){
	s_init();
	
	// Create server, bind it and start listening
	create_server_socket(PORT);
	bind_server_socket;
	listen_server_socket;
	
	srand(time(NULL));
	
	// Client sockets + maxfd for select()
	s_socket maxfd;
	s_socket clients[MAX_CLIENTS];
	for(int i=0; i<MAX_CLIENTS; i++) { clients[i]=s_socket_default; players[i]=(struct player){-1,0,0}; }
	
	// fd_set that will store all sockets to check for activity with select()
	fd_set readfds;
	
	printf("Server started!\nWaiting for activity...\n");
	
	while(true){
		// Reset and put all sockets in the fd_set for select()
		FD_ZERO(&readfds);
		FD_SET(listen_socket,&readfds);
		for(int i=0; i<MAX_CLIENTS; i++){
			if(s_isvalid(clients[i])){
				FD_SET(clients[i],&readfds);
				if(clients[i] > maxfd)
					maxfd=clients[i];
			}
		}
		
		// Wait until there is activity on at least one socket
		//printf("Waiting for packets...");
		int activity=select(maxfd+1,&readfds,NULL,NULL,NULL);
		if((activity < 0) && (s_errno != s_EINTR)) { perror("select() error!\n"); continue; }
		
		// If a new pending connection arrives on the listening socket, add it to the players
		bzero(send_buffer,BUFFER_SIZE);
		send_buffer_pos=0;
		if(FD_ISSET(listen_socket,&readfds)){
			s_socket new_client=s_socket_default;
			accept_server_socket(new_client);
			if(!s_success) fprintf(stderr,"failed to accept new client!\n");
			else{
				printf("new client connected!\n");
				update_new_socket(new_client,clients);
			}
		}
		
		// Now gather all data from clients in the send buffer
		for(int i=0; i<MAX_CLIENTS; i++){
			if(FD_ISSET(clients[i],&readfds)){
				//printf("received packet from client #%d!\n",i);
				bzero(recv_buffer,BUFFER_SIZE);
				s_recv(clients[i],recv_buffer,BUFFER_SIZE-1);
				if(s_recv_result <= 0){
					printf("client #%d disconnected!\n",i);
					closesocket(clients[i]);
					clients[i]=s_socket_default;
					// Add the quit package to send buffer
					send_buffer[send_buffer_pos++]=(char)PKG_PLAYER_QUIT;
					send_buffer[send_buffer_pos++]=(char)i;
					// And reset the player...
					players[i]=(struct player){-1,0,0};
				}else{
					// If the packet does not start with the PKG_PLAYER_UPDATE constant, it is not valid
					if(recv_buffer[0] != (char)PKG_PLAYER_UPDATE) { printf("invalid packet received!\npacket does not start with PKG_PLAYER_UPDATE!\n"); continue; }
					// Add the updated data to the send buffer
					send_buffer[send_buffer_pos++]=recv_buffer[0];
					memcpy(send_buffer+send_buffer_pos,recv_buffer+1,sizeof(struct player));
					send_buffer_pos+=sizeof(struct player);
					// Store the newest data of the player
					memcpy(&players[i],recv_buffer+1,sizeof(struct player));
					//printf("new data: %d, %d, %d, %d\n",players[i].id,players[i].x,players[i].y,players[i].state);
				}
			}
		}
		
		// Send to remaining connected clients the newest updates
		for(int i=0; i<MAX_CLIENTS; i++){
			if(s_isvalid(clients[i])){
				s_send(clients[i],send_buffer,send_buffer_pos);
				if(!s_success) fprintf(stderr,"failed to send packets to client #%d!\n",i);
			}
		}
	}
	
	s_quit();
}