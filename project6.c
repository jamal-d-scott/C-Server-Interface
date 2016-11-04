/*===================
Jamal Scott         =
Project 6           =
Dr. Wittman         =
CS222               =
====================*/

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>

//Prototypes for functions used.
int fileNotFound(char* fileBuffer, char* filePath, char* protocol);
void sendHeader(char* ctype, int fileSize, int otherSocket);
void processFilePath(char* filePath);
char* contentType(char* filePath);

int main(int argc, char** argv)
{
	//Closes the program given there isn't the appropriate command line arguments.
	if(argc != 3)
	{
		printf("Usage: ./server <port> <path to root>\n");
		return 1;
	}
	
	int file;
	int otherSocket;
	int fileSize;
	int readFile = 1;
	int received = 1;
	
	//Hold's all of the client's sent information. 
	char buffer[1024];
	//Holds the client's request (GET/HEAD).
	char request[255];
	//Holds the specified file path/name.
	char filePath[255];
	//Holds the client's protocol.
	char protocol[255];
	//Holds the file's information.
	char fileBuffer[1024];
	
	char* ctype;
	
	//Retrieves the arguments from the command line.
	char* portNum = argv[1];
	const char* path = argv[2];
	
	//Declaration of a struct 'fstat' for gathering file size info.
	struct stat st;

	//Closes the program if the file directory cannot be changed.
	if(chdir(path))
	{
		printf("Could not change to directory: %s\n", path);
		return 1;
	}
	
	//Takes the port number (retrieved from the command line arguments)
	// and converts it from a string to an integer.
	int port = atoi(portNum);
	
	//Creates the server-side socket
	int sockFD = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;
	
	//Enables port re-use.
	int value = 1;
	setsockopt(sockFD, SOL_SOCKET,SO_REUSEADDR, &value, sizeof(value));
	
	//Binds a port to a socket.
	if(bind(sockFD, (struct sockaddr*)&address, sizeof(address)) == -1)
	{	
		//Bind returns -1 signaling a bad port. This notifies of a bad port.
		printf("Bad Port: %d\n", port);
		return 1;
	}
	
	//Listens for incoming requests.
	listen(sockFD, 1);
	
	//Continuously runs the server.
	while(1)
	{		
		//Continuously accepts incoming requests as long as the server is running.
		struct sockaddr_storage otherAddress;
		socklen_t otherSize = sizeof(otherAddress);
		otherSocket = accept( sockFD, (struct sockaddr *) &otherAddress, &otherSize);
		
		//Notifies the server if a client has been accepted.
		if(otherSocket != 0)
				printf("\nGot a client: \n");
				
		//Receives requests and store them into 'buffer.'
		received = recv(otherSocket,buffer,sizeof(buffer)-1,0);
		
		//Adds the null character to the end of the client's sent information.
		buffer[received] = '\0';
		
		//Reads in the request, file path, and protocol from the buffer	
		//and stores them.	
		sscanf(buffer, "%s %s %s", request,filePath,protocol);
		
		if(strcmp("GET",request) == 0 || strcmp("HEAD",request) == 0)
		{
			processFilePath(filePath);
			printf("\t %s %s %s\n",request,filePath,protocol);
			
			//Opens the file for reading only using low-level i/o
			file = open(filePath, O_RDONLY);
			
			//Calls the fstat function to get file info.
			fstat(file, &st);
			
			//Retrieves the file size stored in the stat struct.
			fileSize = st.st_size;
	
			if(file != -1)
			{	
				//Determines and stores the file's content type.
				ctype = contentType(filePath);
				
				//Only sends the 'HEAD' information upon request.
				if(strcmp("HEAD",request) == 0)
				{
					//Only sends the header information upon request.
					sendHeader(ctype,fileSize,otherSocket);
					printf("Sent file: %s\n" , filePath);
				}
				else
				{	
					//Sending the header information first for the requested file.
					sendHeader(ctype,fileSize,otherSocket);
						
					do
					{
						//Reads the bytes from the file into 'fileBuffer' and
						//sends it to the client's socket. 
						readFile = read(file, fileBuffer,sizeof(fileBuffer));
						send(otherSocket, fileBuffer,readFile,0);
						
					//Executes until the size of the file has been reached.
					}while(readFile == sizeof(fileBuffer));

					printf("Sent file: %s\n" , filePath);
				}
					//Closes the file after reading
					close(file);
			}
			else
			{
				//Gives a 404 error if the file does not exist.
				printf("File not found: %s\n", filePath);
				
				/*Calls the 'fileNotFound' function which
				*stores the the HTTP 404 status code information in 
				'fileBuffer' which then sends to the client.
				*/
				send(otherSocket, fileBuffer, fileNotFound(fileBuffer, filePath,protocol),0);
			}
		}
			//Closes the socket connection after all processing was done.
			close(otherSocket);
	}

	return 0;
}

//Function for appropriate file path processing.
void processFilePath(char* filePath)
{	
	//Checks to see if the file path/name ends with the '/' character.
	//If so, adds 'index.html' to the file path. 	
	if(filePath[strlen(filePath)-1] == '/')
		strcat(filePath,"index.html");	
	
	//Removes the character '/' from the beginning of the file name.		
	if(filePath[0] == '/')
		memmove(filePath,filePath+1,strlen(filePath));
}

//Function for giving a client a 404 error.
int fileNotFound(char* fileBuffer, char* filePath, char* protocol)
{	
	//Only sends the HTTP format given there is an HTTP protocol 
	if(strstr(protocol,"HTTP/"))
	{
		sprintf(fileBuffer,  "HTTP/1.0 404 Not Found\nContent-type: text/html\n\n<html>\n<body>\n<h1>Not Found</h1>\n<p>The requested URL %s was not found on this server.</p>\n</body>\n</html>\n" , filePath);
	}
	else
		sprintf(fileBuffer,  "HTTP/1.0 404 Not Found\n");
	
	//Returns the appropriate length for the parameter of the send function.
	return strlen(fileBuffer);
}

char* contentType(char* filePath)
{
	//Finds the last occurrence of the dot character signaling the file
	//extension.
	char* fileExtension = strrchr(filePath, '.');
	
	//Determines the MIME type.
	if(!strcmp(fileExtension,".html") || !strcmp(fileExtension,".htm"))
		return "text/html";
	else if(!strcmp(fileExtension,".jpg") || !strcmp(fileExtension,".jpeg"))
		return "image/jpeg";
	else if(!strcmp(fileExtension,".gif"))
		return "image/gif";
	else if(!strcmp(fileExtension,".png"))
		return "image/png";
	else if(!strcmp(fileExtension,".txt") || !strcmp(fileExtension,".c") || !strcmp(fileExtension,".h"))
		return "text/plain";
	else if(strstr(fileExtension,".pdf"))
		return "application/pdf";
}

//Function for sending a file's header informaion.
void sendHeader(char* ctype, int fileSize, int otherSocket)
{
	//Make a file buffer to hold the HTTP exchange information.
	char fileBuffer[255];
	
	//Writes the HTTP exchange information into the file buffer including the 
	//Content-type and Content-Length.
	sprintf(fileBuffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", ctype,fileSize);
	
	//Sends the header informaion.
	send(otherSocket, fileBuffer, strlen(fileBuffer),0);
}
