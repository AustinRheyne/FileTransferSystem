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

// Prototypes
char* getListOfFiles();
void sendFile(int socket, char* path);
void getFile(int socket, char* path);


int main(int argc, char* argv[]) {
	// Check for valid arguments
	if(argc != 2) {
		printf("Usage: ./server <port>\n");
		exit(1);
	}
	
	// Validate the port
	int port = atoi(argv[1]);
	if(port < 1024 || port > 65535) {
		printf("Bad port: %s\n", argv[1]);
		exit(2);
	}
	
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	int value = 1;
	setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;
	
	// Bind the socket to the address
	bind(socketFD, (struct sockaddr*)&address, sizeof(address));
	
	// Setup the listen
	listen(socketFD, 1);
	
	// Accept a connection
	struct sockaddr_storage otherAddress;
	socklen_t otherSize = sizeof(otherAddress);
	
	
    char OK_MESSAGE[7] = "200 OK";
    char ERR_MESSAGE[8] = "404 ERR";
	while(1) {
		int otherSocket = accept(socketFD, (struct sockaddr *)&otherAddress, &otherSize);
		// Check socket
		if(otherSocket != -1) {
			printf("Client connected to FTP\n");
			char buffer[1024];
			int length = 1;
			// Check for a quit message or a disconnection
			length = read(otherSocket, buffer, sizeof(buffer)-1);
			buffer[length] = '\0';
			
			char protocol[16];
			char request[1024];
			int readLoc = 0;
			sscanf(buffer, "%s %s", protocol, request);
			
			// Update the pointer to match what has been read
			readLoc = strlen(protocol) + strlen(request) + 1;
			
			if(strcmp(protocol, "GET") == 0) {
				
				if(strcmp(request, "ls") == 0) {
					printf("\tClient requested: List of files\n");
					char* fileList = getListOfFiles();
					write(otherSocket, fileList, strlen(fileList));
					free(fileList);
				}
				
				if(strcmp(request, "file") == 0) {
					// Read filename and update read location	
					char fileName[1024];
					sscanf(buffer+readLoc, "%s", fileName);
					readLoc += strlen(fileName) + 1;
					printf("\tClient requested: %s\n", fileName);
					// Send file to socket
					sendFile(otherSocket, fileName);
				}
			}
			if(strcmp(protocol, "POST") == 0) {
				if(strcmp(request, "file") == 0) {
					printf("\tClient requested: Upload file\n");
					write(otherSocket, OK_MESSAGE, strlen(OK_MESSAGE));
					char fileName[1024];
					sscanf(buffer+readLoc, "%s", fileName);
					readLoc += strlen(fileName) + 1;
					
					getFile(otherSocket, fileName);
				}
				
				if(strcmp(request, "rm") == 0) {
					char fileName[1024];
					sscanf(buffer+readLoc, "%s", fileName);
					readLoc += strlen(fileName) + 1;
					
					char fullPath[2048];
					getcwd(fullPath, sizeof(fullPath));
					sprintf(fullPath, "%s/%s", fullPath, fileName);
					printf("\tClient requested: Remove %s\n", fullPath);
					int status = remove(fileName);
					if(status == 0) {
						 write(otherSocket, OK_MESSAGE, strlen(OK_MESSAGE));
					}
					else {
						write(otherSocket, ERR_MESSAGE, strlen(ERR_MESSAGE));
					}
				}
				
				if(strcmp(request, "cd") == 0) {
					char path[1024];
					sscanf(buffer+readLoc, "%s", path);
					readLoc += strlen(path) + 1;
					
					if(chdir(path) == 0) {
						 write(otherSocket, OK_MESSAGE, strlen(OK_MESSAGE));
					}
					else {
						write(otherSocket, ERR_MESSAGE, strlen(ERR_MESSAGE));
					}
				}
				
				if(strcmp(request, "mkdir") == 0) {
					char name[1024];
					sscanf(buffer+readLoc, "%s", name);
					readLoc += strlen(name) + 1;
					
					char fullPath[2048];
					getcwd(fullPath, sizeof(fullPath));
					sprintf(fullPath, "%s/%s", fullPath, name);
					printf("\tClient requested: Create %s\n", fullPath);
					
					// Create directory with read/write/search permissions for owner and group, and read/search permissions for others
					if(mkdir(fullPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
						 write(otherSocket, OK_MESSAGE, strlen(OK_MESSAGE));
					}
					else {
						write(otherSocket, ERR_MESSAGE, strlen(ERR_MESSAGE));
					}
				}
			}
		}
		printf("Client disconnected\n");
		// Flush the socket
		shutdown(otherSocket, SHUT_RDWR);	
		close(otherSocket);
		
	}
	close(socketFD);
}

void getFile(int socket, char* path) {
	// Placeholder fileFD, gaurenteed to not create an error
	int fileFD = -16;
	char buffer[1024];
    int bytes;
    int recievedData = 0;
	while((bytes = read(socket, buffer, 1024)) > 0) {
		// Create fileFD if it does not exist
		if(fileFD == -16){
			recievedData = 1;
			fileFD = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		}
		if(fileFD >= 0) {
			if(write(fileFD, buffer, bytes) != bytes) {
				fprintf(stderr, "Error writing to file");
				break;
			}
		}
	}		
	
	if(!recievedData) 
		printf("File does not exist!");
	close(fileFD);
}

void sendFile(int socket, char* path) {
	int fileFd = open(path, O_RDONLY);
	if(fileFd > 0) {
		printf("\tSending File\n");
		char buff[1024];
		int bytes;
		
		
		while((bytes = read(fileFd, buff, 1024)) != 0) {
			write(socket, buff, bytes);
		}
		
		printf("\tClient recieved: %s\n", path);
	} else {
		printf("\tRequested does not exist\n");
	}
}

char* getListOfFiles() {
    DIR* d;
    struct dirent* dir;
    d = opendir(".");
    char* files = malloc(sizeof(char)*16384);
    files[0] = '\0';
    // Check that directory exists
    if(d) {
        while((dir = readdir(d)) != NULL) {
            // Exclude the current and parent directory
            if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) { 
                struct stat fileStat;
                stat(dir->d_name, &fileStat);
                // Check if it is a file or a directory
                if (S_ISDIR(fileStat.st_mode)) {
                    sprintf(files + strlen(files), "%s/\n", dir->d_name);
                }
                else if (fileStat.st_mode & S_IXUSR) {
                    sprintf(files + strlen(files), "%s.exe\n", dir->d_name);
                
                } 
                else {
                    sprintf(files + strlen(files), "%s\n", dir->d_name);
                }
            }
        }
        closedir(d);
        return files;
    }
    else {
        return "Error getting list of files!\n";
    }
}
