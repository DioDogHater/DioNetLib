// samples/message-exchange/server.c

/*
	This sample shows how to create a simple message exchange server/client program
	with DioDogHater's Networking Library (dionetlib.h).
	This code is written by DioDogHater, github: github.com/DioDogHater
	Please read the documentation in dionetlib.h before using/compiling a program with the library.
*/

#include "dionetlib.h"

#define PORT "8080"

int main(int argc, char* argv[]){
	// Initialise WinSock2 if on windows
	s_init();
	
	// Create the server socket, bind it the the device's address and port 8080
	create_server_socket(PORT);
	bind_server_socket;
	listen_server_socket;
	
	// Try to accept the first client connection
	s_socket client_socket=s_socket_default;
	accept_server_socket(client_socket);
	if(!s_success) { closesocket(listen_socket); s_quit(); }
	printf("client successfully connected!\n");
	
	// Close the listening socket, for we dont need it anymore
	closesocket(listen_socket);
	
	// Receive the message sent by the client (blocking)
	char buffer[256]; bzero(buffer,sizeof(buffer));
	s_recv(client_socket,buffer,sizeof(buffer)-1);
	if(s_recv_result > 0) printf("received message \"%s\"!\n",buffer);
	else { printf("connection error!\n"); closesocket(client_socket); s_quit(); }
	
	// Send "Hello World!" to the client
	char msg[]="Hello World!";
	s_send(client_socket,msg,strlen(msg));
	if(s_success) printf("message successfully sent!\n");
	
	// Close the socket and exit the program, cleaning up WinSock2 if on windows
	closesocket(client_socket);
	s_quit();
	return 0;
}