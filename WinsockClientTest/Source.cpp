#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <conio.h>


#pragma comment(lib, "Ws2_32.lib")

using namespace std;

void pause();

int __cdecl main(int argc, char* argv[]) {

	#define DEFAULT_PORT "27015"
	#define DEFAULT_BUFLEN 512

	//Create a SOCKET object
	SOCKET ConnectSocket = INVALID_SOCKET;

	//Message data being sent
	const char* sendbuf = "This is a test message from client ";

	//WSADATA structure contains info about the Windows Sockets Implementation.
	WSADATA wsadata;

	//WSAStartup function is called to initiate the use of WS2_32.dll
	//The MAKEWORD parameter of WSAStartup makes a request for version 2.2 of WinSock on the system, and sets the passed version of Windows Sockets support that the caller can use.
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (wsaResult != 0) {
		cout << "WSAStartup Failed: " << wsaResult << "\n";
		pause();
		return 1;
	}

	/*
	After initialization, a SOCKET object must be instantiated for use by the client (seen above).
	For this application, the internet address family is unspecified so that either an IPv6 or IPv4 address can be returned.
	The application requests the socket type to be a stream socket for the TCP protocol.
	*/
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	/*
	Call getaddrinfo function requesting the IP address for the server name passed on the command line.
	The TCP port on the server that the client will connect to is definded by DEFAULT_PORT as 27015.
	The getaddrinfo function returns its value as an integer that is checked for errors.
	*/

	//Resolve Server Address and Port
	//WSACleanup is used to terminate the use of the WS2_32 DLL
	int lookupResult;

	if (argv[1] == NULL) {
		const char* ipaddr = "127.0.0.1";
		lookupResult = getaddrinfo(ipaddr, DEFAULT_PORT, &hints, &result);
	}
	else
		lookupResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);

	if (lookupResult != 0) {
		cout << "Getaddrinfo Failed: " << lookupResult << "\n";
		WSACleanup();
		pause();
		return 1;
	}



	/*
	Next call the socket function and return its value to the ConnectSocket variable.
	For this application, use the first IP address returned by the call to getaddrinfo that matched the address family, socket type, and protocol specified in the hints parameter.
	In this example, a TCP stream socket was specified with a socket type of SOCK_STREAM and protocol of IPPROTO_TCP.
	The address family was left unspecified (AF_UNSPEC), so the returned IP address could be either an IPv6 or IPv4 address for the server.
	*/

	//Attempt to connect to the first address returned by the call to getaddrinfo
	ptr = result;

	//Create a SOCKET for connecting to the server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Error at Socket(): " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		pause();
		return 1;
	}

	/*
	Connect to the server
	NOTE: If the connect call failed, we should really try the next address returned by getaddrinfo
	but for this example we just free the resources returned by the getaddrinfo and print an error message
	*/
	int connectResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (connectResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Unable to connect to the server!\n";
		WSACleanup();
		pause();
		return 1;
	}

	/*
	The getaddrinfo function is used to determine the values in the sockaddr structure.
	In this example the first IP address returned by the getaddriunfoi function is used to specifity the sockaddr structure passed to the connect.
	If the connect call fails to the first IP address, then try the next addrinfo structure in the linked list returned from the getaddrinfo function.
	The info specified in the sockaddr structure includes:
		- the IP address of the server that the client will try to connect to
		- the port number of the server that the client will connect to. This port was specified as port 27015 when the client called the getaddrinfo function.
	*/

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN] = "";

	//Send an Initial Buffer
	int sendResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (sendResult == SOCKET_ERROR) {
		cout << "Send Failed: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		pause();
		return 1;
	}

	cout << "Bytes Sent: " << sendResult << "\n";

	//Shutdown the connection for sending since no more data will be sent, the client will continue to use the ConnectSocket  for receiving data
	int shutdownResult = shutdown(ConnectSocket, SD_SEND);
	if (shutdownResult == SOCKET_ERROR) {
		cout << "Shutdown Failed" << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		pause();
		return 1;
	}

	/*
	Receive data until the server closes the connection
	The send and recv functions both return an integer value of the number of bytes sent or received, or an error.
	Each function also takes the same parameters: the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.
	*/
	int receiveResult;
	do {
		receiveResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (receiveResult > 0) {
			cout << "Bytes Received: " << receiveResult << "\n";
			cout << "Received Buffer: " << recvbuf << " \n";
		}
		else if (receiveResult == 0)
			cout << "Connection Closed\n";
		else
			cout << "Receive Failed: " << WSAGetLastError();
	} while (receiveResult > 0);

	/* 
	Once the client is completed sending and receiving data, the client disconnects from the server and shutdowns the sockets.
	When the client is done sending data to the server, the shutdown function can be called specifying SD_SEND to shutdown the sending side of the socket.
	This allows the server to release some of the resources for this socket. The client application can still receive data on the socket.
	*/

	//Shutdown the send half of the connection since no more data will be sent
	int shutdownSendResult = shutdown(ConnectSocket, SD_SEND);
	if (shutdownSendResult == SOCKET_ERROR) {
		cout << "ShutdownSend Failed: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		pause();
		return 1;
	}

	//When client application is done receiving data, the closesocket function is called to close the socket.
	//After the client application is completed using the Windows Sockets DLL, the WSACleanup function is called to release resources.
	closesocket(ConnectSocket);
	WSACleanup();

	pause();
	return 0;
}

void pause() {
	char a;
	cout << string(2, '\n') << "Press any Key to Continue... ";
	a = _getch();
}