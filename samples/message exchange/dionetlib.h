// This library was made by DioDogHater, github.com/DioDogHater
// Please credit me if you use this in a commercial environement

#ifndef DIODOGHATER_NETLIB_H
#define DIODOGHATER_NETLIB_H

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
//	|   A crucial part of networking is assuring that your buffers are holding all zeros
//	|	Instead of using ZeroMemory or any other function, use this one for it is OS dependant
//	|-------->	memset(b,0,sz)

/*

========== LIST OF COMMON REFERENCES BETWEEN WINDOWS AND LINUX ==========

#-------- Constants / Variables: -----------

- s_socket
	macro that gives the type used for the socket objects. WinSock2 uses SOCKET typedefs (unsigned integers) whereas linux uses a signed integer file descriptor

- s_socket_default
	macro that gives the value that needs to be set to a newly created socket. WinSock2 has default INVALID_SOCKET and linux has a default of 0 (integer)

- s_errno
	macro that will give you the errno (replaces the default errno with WSAGetLastError() in windows)

- s_EWOULDBLOCK
	macro that will give you the EWOULDBLOCK error number, OS-dependant (WSAEWOULDBLOCK on winsock2 instead of the default EWOULDBLOCK)

- s_socket_block_mode / s_socket_block_flags
	unsigned long / int variable that will be used in set_socket_block(s_sock,s_block_state)

- s_success
	bool variable that holds wether the last macro has been successfully executed or not (vital for sending, connecting, accepting, etc.)

- s_recv_result
	int variable that holds the result returned from the last s_read(s_sock,s_data,s_sz) called
	normally holds the amount of bytes of data stored in the buffer, but will be 0 if connection lost and -1 if an error occurs

#---------- Functions/Macros: --------------

- s_init()
	will initialize the winsock2 .dll files necessary for the usage of the library, does nothing in linux
	NEEDS TO BE CALLED BEFORE ANY OTHER FUNCTION/MACRO TO WORK WITH WINSOCK2

- s_stop()
	quits the winsock2 libraries, does nothing in linux
	NEEDS TO BE CALLED AT THE END OF PROGRAM TO WORK WITH WINSOCK2

- s_quit()
	will quit winsock2 and exit the program, will only exit the program in linux
	ONLY CALL IF YOU WANT YOUR PROGRAM TO STOP WORKING! i.e. when fatal errors occur

- closesocket(s)
	closes the socket connection and frees the port connection for other programs (very important in linux)
	note: with WinSock2, this is not actually a macro in this header, but the actual function used by WinSock2

- set_socket_block(s_sock,s_block_state)
	sets the socket to either (s_block_state=true) blocking mode or (s_block_state=false) non-blocking mode

- bzero(b,sz) -> ALTERNATIVE TO memset(b,0,sz)
	will set sz bytes of buffer b to 0
	use memset since it is meant to be OS-dependant
	this is just to accomodate people who normally use the linux socket libraries, and lazy people like myself
	
- s_send(s_sock,s_data,s_sz)
	will send s_data through s_sock's connection with size s_sz
	please check if s_success is false in case of an error!

- s_recv(s_sock,s_data,s_sz)
	will receive s_sz bytes of data from s_sz's connection and store it in s_data
	note: normally, if s_recv_result is 0 or <0, there has been a connection error, so please check before reading s_data

- create_server_socket(s_port)
	creates the server's listen socket, as the variable listen_socket (if you need to use it)
	port is a string holding the port number where the server will establish

- bind_server_socket
	will bind the server listen socket to the port and the device's address

- listen_server_socket
	starts listening to client connections

- accept_server_socket(s_client_socket)
	will wait until client socket connects to accept it
	please check if s_success is false in case of an error!
	s_client_socket needs to be a variable of type s_socket

- close_server_socket(s_client_socket)
	will cut of the output stream to the s_client_socket's connection.
	does not in fact stop the connection entirely, for the client can still send data through s_client_socket
	to fully close the connection, please use closesocket()

- create_client_socket(s_addr,s_port)
	creates the client's socket to connect to the server
	VERY IMPORTANT: you will need the client socket for later functions for data transfer !!!
	THE NAME OF THE s_socket VARIABLE HOLDING THE CLIENT SOCKET IS client_socket

*/

// ---------------------------- WINDOWS ------------------------------------
#if defined(_WIN32) // If code is compiled on any Windows version, it will use windows.h and the WinSock2 API

// Link using necessary libraries

// >>>>>>>>>>>>>>>>> IMPORTANT PLEASE READ <<<<<<<<<<<<<<<<<<<
// PLEASE ADD -lws2_32 -lmswsock -ladvapi32 TO YOUR COMPILATION COMMAND IF YOU ARE COMPILING ON WINDOWS !!!

// Necessary for good functioning of WinSock2
#define WIN32_LEAN_AND_MEAN

// Quick hack to get code working on mingw32
// Put this part of the code in comments if libraries keep missing on mingw64:
#define OS_WIN32
#ifndef WINVER
#define WINVER 0x0501
#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
// -----

// All necessary libraries
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define s_socket SOCKET
#define s_socket_default INVALID_SOCKET
#define s_errno WSAGetLastError()
#define s_EWOULDBLOCK WSAEWOULDBLOCK

unsigned long s_socket_block_mode;
bool s_success;
int s_recv_result;
int s_iresult;

#define bzero(b,sz) memset((b),0,(sz))
#define set_socket_block(s_sock, s_block_state) \
	s_socket_block_mode=s_block_state?0:1;\
	s_success=(ioctlsocket(s_sock,FIONBIO,&s_socket_block_mode) == 0);
void s_init() { WSADATA WsaData; int result; if((result=WSAStartup(MAKEWORD(2,2),&WsaData)) != 0) { fprintf(stderr,"WinSock2 init error: %d\n",result); } }
void s_stop() { WSACleanup(); }
void s_quit() { WSACleanup(); exit(0); } // Change this if you want it to not exit
#define s_send(s_sock, s_data, s_sz) \
	if(send(s_sock,s_data,(int)s_sz,0) == SOCKET_ERROR) { fprintf(stderr,"send() error: %d\n",WSAGetLastError()); s_success=false; }\
	else s_success=true;
#define s_recv(s_sock, s_data, s_sz) \
	s_recv_result=recv(s_sock,s_data,(int)s_sz,0);\
	if(s_recv_result < 0) fprintf(stderr,"recv() error: %d\n",WSAGetLastError());
#define create_server_socket(s_port) \
	struct addrinfo *s_result_ptr=NULL, s_hints;\
	s_socket listen_socket=s_socket_default;\
	ZeroMemory(&s_hints, sizeof(s_hints));\
	s_hints.ai_family=AF_INET;\
	s_hints.ai_socktype=SOCK_STREAM;\
	s_hints.ai_protocol=IPPROTO_TCP;\
	s_hints.ai_flags=AI_PASSIVE;\
	if((s_iresult=getaddrinfo(NULL, s_port, &s_hints, &s_result_ptr)) != 0) { fprintf(stderr,"getaddrinfo() error: %d\n",s_iresult); s_quit(); }\
	listen_socket=socket(s_result_ptr->ai_family, s_result_ptr->ai_socktype, s_result_ptr->ai_protocol);\
	if(listen_socket == s_socket_default){ fprintf(stderr,"socket() error: %d\n",WSAGetLastError()); s_quit(); }
#define bind_server_socket \
	if(bind(listen_socket, s_result_ptr->ai_addr, (int)s_result_ptr->ai_addrlen) == SOCKET_ERROR){\
		fprintf(stderr,"bind() error: %d\n",WSAGetLastError());\
		freeaddrinfo(s_result_ptr);\
		closesocket(listen_socket);\
		s_quit();\
	}freeaddrinfo(s_result_ptr);
#define listen_server_socket \
	if(listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) { fprintf(stderr,"listen() error: %d\n",WSAGetLastError()); closesocket(listen_socket); s_quit(); }
#define accept_server_socket(s_client_socket) \
	s_client_socket=s_socket_default;\
	s_client_socket=accept(listen_socket,NULL,NULL);\
	if(s_client_socket == s_socket_default) { fprintf(stderr,"accept() error: %d\n",WSAGetLastError()); s_success=false; }\
	else s_success=true;
#define close_server_socket(s_client_socket) \
	if(shutdown(s_client_socket,SD_SEND) == SOCKET_ERROR) { fprintf(stderr,"shutdown() error: %d\n"); closesocket(s_client_socket); s_quit(); }\
	closesocket(s_client_socket);
#define create_client_socket(s_addr, s_port) \
	struct addrinfo *s_result_ptr=NULL, *s_ptr=NULL, s_hints;\
	s_socket client_socket=s_socket_default;\
	ZeroMemory(&s_hints, sizeof(s_hints));\
	s_hints.ai_family=AF_UNSPEC;\
	s_hints.ai_socktype=SOCK_STREAM;\
	s_hints.ai_protocol=IPPROTO_TCP;\
	if((s_iresult=getaddrinfo(s_addr, s_port, &s_hints, &s_result_ptr))!= 0){ fprintf(stderr,"getaddrinfo() error: %d\n",s_iresult); s_quit(); }\
	s_ptr=s_result_ptr;\
	client_socket=s_socket_default;\
	client_socket=socket(s_ptr->ai_family, s_ptr->ai_socktype, s_ptr->ai_protocol);\
	if(client_socket == s_socket_default){ fprintf(stderr,"socket() error: %d\n",WSAGetLastError()); freeaddrinfo(s_result_ptr); s_quit(); }
#define connect_client_socket \
	if(connect(client_socket,s_ptr->ai_addr,(int)s_ptr->ai_addrlen) == SOCKET_ERROR) { closesocket(client_socket); client_socket=s_socket_default; s_success=false; }\
	else s_success=true;
#define close_client_socket \
	if(shutdown(client_socket,SD_SEND) == SOCKET_ERROR) { fprintf(stderr,"shutdown() error: %d\n"); closesocket(client_socket); s_quit(); }\
	closesocket(client_socket);
// ---------------------------------------------------------------------------


// ----------------------------- LINUX ---------------------------------------
#elif defined __linux__ // If code is compiled on linux, it will use the default linux socket API

// Necessary libraries
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define s_socket int
#define s_socket_default 0
#define s_errno errno
#define s_EWOULDBLOCK EWOULDBLOCK

int s_socket_block_flags;
bool s_success;
int s_recv_result;

#define closesocket(s) close((s))
#define set_socket_block(s_sock, s_block_state) \
	s_socket_block_flags=fcntl(s_sock, F_GETFL, 0);\
	if(s_socket_block_flags == -1)s_success=false;\
	s_socket_block_flags = s_block_state ? (s_socket_block_flags & ~O_NONBLOCK) : (s_socket_block_flags | O_NONBLOCK);\
	s_success=(fcntl(s_sock, F_SETFL, s_socket_block_flags) == 0);
void s_init() {} // They both do absolutely nothing. Blame windows and their weird ahh system ok.
void s_stop() {}
void s_quit() { exit(0); }
#define s_send(s_socket, s_data, s_sz) \
	if(write(s_socket,s_data,s_sz) < 0) { perror("write() (s_send()) error!\n"); s_success=false; }\
	else s_success=true;
#define s_recv(s_socket, s_data, s_sz) \
	s_recv_result=read(s_socket,s_data,s_sz);\
	if(s_recv_result < 0) perror("read() (s_recv()) error!\n");
#define create_server_socket(s_port) \
	struct sockaddr_in s_serv_addr;\
	s_socket listen_socket=socket(AF_INET,SOCK_STREAM,0);\
	if(listen_socket < 0) { perror("socket() error!\n"); s_quit(); }\
	bzero((char*)&s_serv_addr,sizeof(s_serv_addr));\
	s_serv_addr.sin_family=AF_INET;\
	s_serv_addr.sin_port=htons(atoi(s_port));\
	s_serv_addr.sin_addr.s_addr=INADDR_ANY;
#define bind_server_socket \
	char s_true_val=1;\
	setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR,&s_true_val,sizeof(int));\
	if(bind(listen_socket,(struct sockaddr*)&s_serv_addr,sizeof(s_serv_addr)) < 0) { perror("bind() error!\n"); s_quit(); }
#define listen_server_socket \
	listen(listen_socket,SOMAXCONN);
#define accept_server_socket(s_client_socket) \
	s_client_socket=accept(listen_socket,NULL,NULL);\
	if(s_client_socket < 0) { perror("accept() error!\n"); s_success=false; }\
	else s_success=true;
#define close_server_socket(s_client_socket) \
	if(shutdown(s_client_socket,SHUT_WR) < 0) { perror("shutdown() error!\n"); closesocket(s_client_socket); s_quit(); }\
	closesocket(s_client_socket);
#define create_client_socket(s_address, s_port) \
	struct hostent* s_server;\
	struct sockaddr_in s_serv_addr;\
	s_socket client_socket=socket(AF_INET,SOCK_STREAM,0);\
	if(client_socket < 0) { perror("socket() error!\n"); s_quit(); }\
	s_server=gethostbyname(s_address);\
	if(s_server == NULL) { perror("gethostbyname() error! Invalid address!\n"); closesocket(client_socket); s_quit(); }\
	bzero((char*)&s_serv_addr,sizeof(s_serv_addr));\
	s_serv_addr.sin_family=AF_INET;\
	bcopy((char*) s_server->h_addr, (char*) &s_serv_addr.sin_addr.s_addr, s_server->h_length);\
	s_serv_addr.sin_port=htons(atoi(s_port));
#define connect_client_socket \
	if(connect(client_socket,(struct sockaddr*)&s_serv_addr,sizeof(s_serv_addr)) < 0) { perror("connect() error!\n"); s_success=false; }\
	else s_success=true;
#define close_client_socket \
	if(shutdown(client_socket,SHUT_WR) < 0) { perror("shutdown() error!\n"); closesocket(client_socket); s_quit(); }\
	closesocket(client_socket);

// ---------------------------------------------------------------------------


// --------------- IN CASE YOUR OS ISN'T SUPPORTED ---------------------------
#else
#error "Sadly, your OS isn't supported by dionetlib.h! Please check if you are using mingw for Windows and not cygwin."
#endif


#endif