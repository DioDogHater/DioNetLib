// samples/multiclient/server.c

/*
	This is a replacement for people who do not have the telnet command installed on their device.
	Please use telnet instead of this, for it is only capable of sending a message once, then listening.
*/

#include "../../dionetlib.h"

#define ADDRESS "127.0.0.1"
#define PORT "8787"

int main(int argc, char* argv[]){
	s_init();
	
	// Connect to the server
	create_client_socket(ADDRESS,PORT);
	connect_client_socket;
	if(!s_success) { fprintf(stderr,"connection failed!\n"); s_quit(); }
	
	// Receive the greeting message
	char buffer[513]; bzero(buffer,sizeof(buffer));
	s_recv(client_socket,buffer,sizeof(buffer)-1);
	if(s_recv_result <= 0){
		fprintf(stderr,"recv failed!\n");
		closesocket(client_socket);
		s_quit();
	}
	printf("received message: \"%s\"\n",buffer);
	
	// Ask message to user
	printf("Please enter your message:\n> ");
	bzero(buffer,sizeof(buffer));
	fgets(buffer,sizeof(buffer)-1,stdin);
	buffer[strlen(buffer)-1]='\0';
	printf("sending %s\n",buffer);
	
	// Send our message
	s_send(client_socket,buffer,strlen(buffer));
	if(!s_success) {
		fprintf(stderr,"failed sending message!\n");
		closesocket(client_socket);
		s_quit();
	}
	
	// Listen for messages
	while(true){
		bzero(buffer,sizeof(buffer));
		s_recv(client_socket,buffer,sizeof(buffer)-1);
		if(s_recv_result <= 0) {
			fprintf(stderr,"recv failed!\n");
			break;
		}
		printf("received: \"%s\"\n",buffer);
	}
	
	closesocket(client_socket);
	s_quit();
}