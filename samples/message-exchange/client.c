// samples/message-exchange/client.c

/*
	This sample shows how to create a simple message exchange server/client program
	with DioDogHater's Networking Library (dionetlib.h).
	This code is written by DioDogHater, github: github.com/DioDogHater
	Please read the documentation in dionetlib.h before using/compiling a program with the library.
*/

#include "dionetlib.h"

#define PORT "8080"
#define ADDRESS "127.0.0.1"

int main(int argc, char* argv[]) {
	// Initialise WinSock2 if on windows
	s_init();
	
	// Create the client socket with the address set to 127.0.0.1 (localhost) and port set to 8080
	create_client_socket(ADDRESS,PORT);
	
	// Try to connect to the server
	connect_client_socket;
	if(s_success) printf("successfully connected to server!\n");
	else { printf("failed to connect!\n"); closesocket(client_socket); s_quit(); }
	
	// Prompt the message in the console
	char buffer[256]; bzero(buffer,sizeof(buffer));
	printf("Please enter your message:\n> ");
	fgets(buffer,sizeof(buffer)-1,stdin);
	buffer[strlen(buffer)-1]='\0';// <-- This line simply removes the '\n' at the end of the message (fgets doesnt remove it)
	
	// Send the message written by the user
	s_send(client_socket,buffer,strlen(buffer));
	if(s_success) printf("sent message successfully!\n");
	else { printf("failed to send!\n"); closesocket(client_socket); s_quit(); }
	
	// Receive the response from the server
	bzero(buffer,sizeof(buffer));
	s_recv(client_socket,buffer,sizeof(buffer)-1);
	if(s_recv_result > 0) printf("received message \"%s\"\n",buffer);
	else { printf("connection error!\n"); closesocket(client_socket); s_quit(); }
	
	// Close the client socket and exit, cleaning up WinSock2 if on windows
	closesocket(client_socket);
	s_quit();
	return 0;
}