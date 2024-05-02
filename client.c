// Standard Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// Network Includes
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

// File I/O
#include <fcntl.h>
#include <sys/stat.h>

void getCharFromServer(int sockFD, char* response);
void getFileFromServer(int sockFD, char* fileName);
void uploadFileToServer(int sockFD, char* fileName);
int main(int argc, char* argv[]) {
	// Check for valid arguments
	if(argc != 3) {
		printf("Usage: ./server <address> <port>\n");
		exit(1);
	}
	
	// Validate the port
	int port = atoi(argv[2]);
	if(port < 1024 || port > 65535) {
		printf("Bad port: %s\n", argv[1]);
		exit(2);
	}
	
	// Setup socket
    int sockFD;
    struct sockaddr_in address;
    
    char input[1024];
    char message[2048];
    char response[16384];
    
    char OK_MESSAGE[7] = "200 OK";
    char ERR_MESSAGE[8] = "404 ERR";
	
	while(strcmp(input, "quit\n") != 0) {
		
		// Initialize socket
		sockFD = socket(AF_INET, SOCK_STREAM, 0);
		
		// Setup the address
		char* serverAddress = argv[1];
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		inet_pton(AF_INET, serverAddress, &(address.sin_addr));
	
		// Clear messages
		input[0] = '\0';
		message[0] = '\0';
		response[0] = '\0';
		
		printf("Command: ");
		fgets(input, sizeof(input), stdin);
		
		if(strcmp(input, "ls\n") == 0) {
			
		    // Attempt to connect to the server    
		    connect(sockFD, (struct sockaddr *)&address, sizeof(address));
		    
			sprintf(message, "GET ls\n%s", input);
			write(sockFD, message, strlen(message));
			getCharFromServer(sockFD, response);
			
			close(sockFD);
			// Read server response
			printf("\n%s\n", response);
		}
		
		if(strcmp(input, "download\n") == 0) {
			char fileName[1024];
			char newFileName[1024];
			printf("Enter filename: ");
			fgets(fileName, sizeof(fileName), stdin);
			
			// Remove new line from fileName
			fileName[strlen(fileName)-1] = '\0';
			
			printf("Enter new name for file: ");
			fgets(newFileName, sizeof(newFileName), stdin);
			newFileName[strlen(newFileName)-1] = '\0';
			
			printf("Downloading %s as %s\n\n", fileName, newFileName);
			
		    // Attempt to connect to the server    
		    connect(sockFD, (struct sockaddr *)&address, sizeof(address));
		    
			
			// Ask the server for the file
			sprintf(message, "GET file\n%s", fileName);
			write(sockFD, message, strlen(message));
			
			// Attempt to recieve the file from the server
			getFileFromServer(sockFD, newFileName);
			
			close(sockFD);
		}
		
		
		if(strcmp(input, "upload\n") == 0) {
			char fileName[1024];
			char newFileName[1024];
			printf("Enter filename: ");
			fgets(fileName, sizeof(fileName), stdin);
			
			// Remove new line from fileName
			fileName[strlen(fileName)-1] = '\0';
			
			printf("Enter new name for file: ");
			fgets(newFileName, sizeof(newFileName), stdin);
			newFileName[strlen(newFileName)-1] = '\0';
			
			printf("\tUploading %s as %s\n\n", fileName, newFileName);
			
			
		    // Attempt to connect to the server    
		    connect(sockFD, (struct sockaddr *)&address, sizeof(address));
        
			
			// Ask the server for the file
			sprintf(message, "POST file\n%s", newFileName);
			write(sockFD, message, strlen(message));
			
			// Upload the file to the server
			uploadFileToServer(sockFD, fileName);
			
			close(sockFD);
		}
		
		if(strcmp(input, "rm\n") == 0) {
			char fileName[1024];
			printf("Enter filename: ");
			fgets(fileName, sizeof(fileName), stdin);
			
			// Remove new line from fileName
			fileName[strlen(fileName)-1] = '\0';
			
			
		    // Attempt to connect to the server    
		    connect(sockFD, (struct sockaddr *)&address, sizeof(address));
		    
		    // Ask server to remove file
			sprintf(message, "POST rm\n%s", fileName);
			write(sockFD, message, strlen(message));
			
			// Wait for repsonse
			getCharFromServer(sockFD, response);
			
			if(strcmp(response, ERR_MESSAGE) == 0) {
				printf("Error removing file from server!\n");
			} else {
				printf("File removed from server\n");
			}
			close(sockFD);
		}
	}

}

void uploadFileToServer(int sockFD, char* fileName) {
	int fileFd = open(fileName, O_RDONLY);
	if(fileFd > 0) {
		printf("\tSending File\n");
		char buff[1024];
		int bytes;
		
		
		while((bytes = read(fileFd, buff, 1024)) != 0) {
			write(sockFD, buff, bytes);
		}
		
		printf("\tServer recieved: %s\n", fileName);
	} else {
		printf("\tRequested does not exist\n");
	}
}

void getFileFromServer(int sockFD, char* fileName) {

	// Placeholder fileFD, gaurenteed to not create an error
	int fileFD = -16;
	char buffer[1024];
    int bytes;
    int recievedData = 0;
	while((bytes = read(sockFD, buffer, 1024)) > 0) {
		// Create fileFD if it does not exist
		if(fileFD == -16){
			recievedData = 1;
			fileFD = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		}
		if(fileFD >= 0) {
			if(write(fileFD, buffer, bytes) != bytes) {
				fprintf(stderr, "Error writing to file\n");
				break;
			}
		}
	}		
	
	if(!recievedData) 
		printf("File does not exist!\n");
	
	close(fileFD);
}

void getCharFromServer(int sockFD, char* response) {
    char buffer[1024];
    int bytes;
	while((bytes = read(sockFD, buffer, 1024)) > 0) {
		buffer[bytes] = '\0';
		strcat(response, buffer);
	}		
}
