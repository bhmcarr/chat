// Brandon Carr
// CSE 489 - PA1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h> //hostent
#include <arpa/inet.h>
//Prototypes ------------------------*
struct Command tokenize(char * line);
void displayHelp();
int getPort(char * argv[]);
//-----------------------------------*
uint LQ_SIZE = 10;

struct Command{	// used for tokenizer
	char * args[4];
};

struct listEntry{
	char * host;
	char * ip;
	char * port;
	char * id;
	int in_use;
};


int main(int argc, char * argv[]){
	int i; 		// iterator for later
	int err;	// errors
	if(argc < 2){
		printf("Incorrect usage.\n");	// too few args from argv, exit
		return 1;
	}

//	[----------------------SERVER MODE-------------------------]
	if(*argv[1] == 's'){
		struct sockaddr_in s_listen_info; 	// server listening socket struct declared
		int s_listen_socket;				// fd for server listen sock goes here
		int new_socket;						// holder for new incoming connections
		int port = getPort(argv);			// get the listening port from argv
		char s_buffer[1000];				// server-side buffer
		bzero(&s_buffer, sizeof(s_buffer));
		char ntopbuff[20];					// buffer for ntop conversions
		int new_port;
		char * new_host;
		

		if((s_listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			printf("[!] Socket creation error.\n");
			return 1;						// create the listening socket
		}

		// Now fill in some information about the listening socket
		s_listen_info.sin_family = AF_INET;					// family 
		s_listen_info.sin_addr.s_addr = htonl(INADDR_ANY);	// accept any address
		s_listen_info.sin_port = htons(port);				// convert port to network format and store in struct

		// Bind the listening socket
		bind(s_listen_socket, (struct sockaddr*)&s_listen_info, sizeof(s_listen_info));

		// Listen on that socket for incoming client connections
		listen(s_listen_socket, LQ_SIZE);

		printf("Listening on port %d..\n", port);

		//--- FD VARIABLES HERE ---
		fd_set fds;							// watch list for client sockets
		int client_sockets[4];				// client socket ids go here
		bzero(&client_sockets, sizeof(client_sockets));	// zero it out
		int num_ready;
		int addrlen = sizeof(s_listen_info);

		//--- INITIALIZE THE SERVER_IP_LIST ---
		struct listEntry server_ip_list[5];
		server_ip_list[0].host = "timberlake.cse.buffalo.edu";
		server_ip_list[0].ip = "128.205.36.8";
		char  int_to_string[15];
		sprintf(int_to_string, "%d", port);
		server_ip_list[0].port = int_to_string;
		server_ip_list[0].id = "-";
		server_ip_list[0].in_use = 1;

		for(i = 1; i < 5; i++){
			server_ip_list[i].in_use = 0;
		}

		while(1){
			// Clear the fds
			FD_ZERO(&fds);

			// Re-add listening socket to set
			FD_SET(s_listen_socket, &fds);

			// Add all current sockets to set
			for(i = 0; i < 4; i++){
				if(client_sockets[i] > 0){
					FD_SET(client_sockets[i], &fds);
				}
			}

			// Wait for activity on sockets
			num_ready = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
			if(num_ready < 0){
				printf("[!] Select failure.\n");
				return 1;
			}

			// Activity on listening socket = new connection
			if(FD_ISSET(s_listen_socket, &fds)){
				// Accept the new connection and create a socket for it
				new_socket = accept(s_listen_socket,(struct sockaddr*)&s_listen_info, &addrlen);
				if(new_socket < 0){
					printf("[!] Accept error.\n");
					return 1;
				}
				printf("A new client has connected.\n");

				// read host name from client
				err = read(new_socket, s_buffer, sizeof(s_buffer));
				s_buffer[err] = '\0';

				// Add the socket fd to the client socket set
				for(i = 0; i < 4; i++){
					if(client_sockets[i] == 0){
						client_sockets[i] = new_socket;
						printf("Added client (fd:%d) to client socket list.\n", new_socket);
						break;
					}			
				}

				//update list
				for(i = 0; i < 5; i++){
					if(server_ip_list[i].in_use == 0){
					// copy id
					server_ip_list[i].id = "-";
					
					// copy ip
					inet_ntop(AF_INET, &s_listen_info.sin_addr, ntopbuff, sizeof(server_ip_list));
					server_ip_list[i].ip = malloc(sizeof(ntopbuff));
					memcpy(server_ip_list[i].ip, ntopbuff, sizeof(ntopbuff));
					
					// copy port number
					new_port = ntohs(s_listen_info.sin_port);
					sprintf(int_to_string, "%d", new_port);
					server_ip_list[i].port = malloc(sizeof(int_to_string));
					memcpy(server_ip_list[i].port, int_to_string, sizeof(int_to_string));

					// copy host
					server_ip_list[i].host = malloc(sizeof(s_buffer));
					memcpy(server_ip_list[i].host, s_buffer, sizeof(s_buffer));
					
					// mark as in use
					server_ip_list[i].in_use = 1;

					break;
					}
				}

				// prepare list to be sent
					//strcat(s_buffer, "ID...HOST...............................IP..........PORT\n");
					for(i = 0; i < 5; i++){
						if(server_ip_list[i].in_use == 1){
							strcat(s_buffer, server_ip_list[i].id);
							strcat(s_buffer, " ");
							strcat(s_buffer, server_ip_list[i].host);
							strcat(s_buffer, " ");
							strcat(s_buffer, server_ip_list[i].ip);
							strcat(s_buffer, " ");
							strcat(s_buffer, server_ip_list[i].port);
							strcat(s_buffer, "\n");
						}
						if(i == 4){
							strcat(s_buffer, "\0");
						}
					}

					//printf("%s",s_buffer);
					write(new_socket,s_buffer,1000);

					//write to all connected clients
					for(i = 0; i < 5; i++){
						if(client_sockets[i] != 0 && client_sockets[i] != new_socket){
							write(client_sockets[i], s_buffer, 1000);
						}
					}
			}

			// Activity on existing socket - service all sockets
			for(i = 0; i < 4; i++){
				if(FD_ISSET(client_sockets[i], &fds)){
					// read from the socket
					err = read(client_sockets[i], s_buffer, 100);
					s_buffer[err] = '\0';	//null termination

					// check for registration request
					if(strcmp(s_buffer,"exit") == 0){
						printf("Client on fd:%d has disconnected.", client_sockets[i]);

						close(client_sockets[i]);

						client_sockets[i] = 0;

					}


				}
			}



			sleep(1);
		}

	}
//	[----------------------CLIENT MODE-------------------------]
	else if(*argv[1] == 'c'){
		char *input_buffer;		// user input buffer
		size_t n = 0;			// used for getline
		struct Command cmd;		// used for tokenizer (holds args)
		char read_buffer[1000];
		char client_buffer[500];
		bzero(&client_buffer, sizeof(client_buffer));
		int c_listen_socket;				// client listening socket
		struct sockaddr_in c_listen_info;	// client listening socket info
		int port = getPort(argv);
		int num_ready;
		int new_socket;
		char * whoami;

		// C L SOCKET INITIALIZATION
		c_listen_info.sin_family = AF_INET;
		c_listen_info.sin_addr.s_addr = htonl(INADDR_ANY);
		c_listen_info.sin_port = htons(port);	// short? long?

		// create listening socket
		c_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
		if(c_listen_socket < 0){
			printf("Socket creation error.\n");
			return 1;
		}

		// bind socket
		bind(c_listen_socket, (struct sockaddr*)&c_listen_info, sizeof(c_listen_info));

		// listen on socket
		listen(c_listen_socket, LQ_SIZE);
		//printf("chat>>");

		// FD SET INITIALIZATION
		fd_set fds;							// watch list for client sockets
		int peer_list[4];				// client socket ids go here
		bzero(&peer_list, sizeof(peer_list));	// zero it out
		//int num_ready;
		int addrlen = sizeof(c_listen_info);

		while(1){

			// Clear the fds
			FD_ZERO(&fds);

			// Re-add listening socket to set
			FD_SET(c_listen_socket, &fds);

			// Add STDIN to set
			FD_SET(0, &fds);

			// Add all current sockets to set
			for(i = 0; i < 4; i++){
				if(peer_list[i] > 0){
					FD_SET(peer_list[i], &fds);
				}
			}

			// Wait for activity on sockets
			num_ready = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
			if(num_ready < 0){
				printf("[!] Select failure.\n");
				return 1;
			}

			// incoming connection
			if(FD_ISSET(c_listen_socket, &fds)){
				new_socket = accept(c_listen_socket,(struct sockaddr*)&c_listen_info, &addrlen);

				printf("A new client has connected.\n");

				// Add the socket fd to the peer list
				for(i = 0; i < 4; i++){
					if(peer_list[i] == 0){
						peer_list[i] = new_socket;
						printf("Added client (fd:%d) to peer list.\n", new_socket);
						break;
					}			
				}


			}

			//incoming data from existing socket
			for(i = 0; i < 4; i++){
				if(FD_ISSET(peer_list[i], &fds) && peer_list[i] != 0){
					// read from the socket
					err = read(peer_list[i], client_buffer, 1000);
					client_buffer[err] = '\0';	//null termination

					printf("%s\n",client_buffer);

				}
			}



//			-----------------------------------------------------
			//printf("chat>>");
			getline(&input_buffer,&n, stdin);	// get command from stdin

			cmd = tokenize(input_buffer);		// tokenize command into args

			if(strcmp(cmd.args[0],"myip") == 0){	// MYIP - displays user IP

				struct sockaddr_in address_info;
				memset(&address_info, '0', sizeof(address_info));
	

				int c_socket;
				if((c_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
					printf("Cannot create socket.\n");
					return 1;
				}



				address_info.sin_family = AF_INET;
				address_info.sin_port = htons(5000);

				if(inet_pton(AF_INET, "8.8.8.8", &address_info.sin_addr) <=0){
					printf("inet_pton error \n");
					return 1;	//set up to connect to Google DNS server to get a generic response
				}

				if(connect(c_socket, (struct sockaddr*)&address_info, sizeof(address_info)) < 0){
					printf("Couldn't connect...\n");
					return 1;	// actual connecting
				}

				char* my_ip;
				my_ip = malloc(50);
				memset(my_ip, '0', sizeof(my_ip));
				struct sockaddr_in dummy;
				memset(&dummy, '0', sizeof(dummy));
				int dummylen = sizeof(dummy); // create dummy addr struct

				if(getsockname(c_socket, (struct sockaddr*)&dummy, &dummylen) != 0){
					printf("getsockname error %d\n",errno);
					return 1;	// extract socket info to dummy struct
				}

				snprintf(my_ip, 50, "%s", inet_ntoa(dummy.sin_addr)); // convert to readable format and print
				printf("MY IP: %s \n",my_ip);

				//inet_ntop(AF_INET, &dummy, &my_ip, sizeof(dummy));

				//printf("%s \n",my_ip);

			}
			else if(strcmp(cmd.args[0],"exit") == 0){
				if(peer_list[i] != 0){
					write(peer_list[0], "exit", 5);
				}
				exit(1);
			}

			else if(strcmp(cmd.args[0],"help") == 0){
				displayHelp();
			}
			else if(strcmp(cmd.args[0],"register") == 0){
								
				int server_socket;						// server socket fd
				struct sockaddr_in server_socket_info;	// server socket info

				server_socket = socket(AF_INET, SOCK_STREAM, 0); // create the socket
				if(server_socket < 0){
					printf("[!] Socket Error.\n");
					return 1;
				}

				// Fill in socket connection info

				char * endptr;					// convert port from string to long
		 		long converted_port = strtol(cmd.args[2], &endptr, 10);	

				server_socket_info.sin_family = AF_INET;
				server_socket_info.sin_port = htons(converted_port);	// port goes here

				// convert ip to network format and store in struct
				if(inet_pton(AF_INET, cmd.args[1], &server_socket_info.sin_addr) < 0){
					printf("[!] inet_pton error.\n");
					return 1;
				}

				printf("Connecting with %s on port %ld\n",cmd.args[1],converted_port);

				err = connect(server_socket,(struct sockaddr*)&server_socket_info, sizeof(server_socket_info));
				if(err < 0){
					printf("[!] Connection error %d\n", errno);
					return 1;
				}
				printf("Connection successful.\n");

				// get own host name
				err = gethostname(client_buffer, sizeof(client_buffer));
				if(err < 0){
					printf("Could not resolve host");
				}

				//store hostname
				whoami = malloc(100);
				memcpy(whoami, client_buffer, 100);

				// send host name to server
				write(server_socket, client_buffer, sizeof(client_buffer));
				bzero(&client_buffer, sizeof(client_buffer));

				sleep(1);

				// read list from server
				read(server_socket, read_buffer, 1000);
				printf("%s\n",read_buffer);

				// add connection to socket list
				for(i = 0; i < 4; i ++){
					if(peer_list[i] == 0){
						peer_list[i] = server_socket;
						break;
					}
				}



			}
			else if(strcmp(cmd.args[0],"creator") == 0 || strcmp(cmd.args[0],"CREATOR") == 0){
				printf("[Chat Client]\nBrandon Carr - CSE 489\nSpring 2015\n");
			}

			else if(strcmp(cmd.args[0],"connect") == 0){
				// set up a new socket
				int new_client_socket;
				struct sockaddr_in new_client_info;

				// fill in struct
				new_client_info.sin_family = AF_INET;
				inet_pton(AF_INET, cmd.args[1], &new_client_info.sin_addr);
				char * endptr;					// convert port from string to long
		 		long converted_port = strtol(cmd.args[2], &endptr, 10);	
				new_client_info.sin_port = htons(converted_port);

				// create socket
				new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
				if(new_client_socket < 0){
					printf("Socket creation error.\n");
					return 1;
				}

				// connect
				err = connect(new_client_socket, (struct sockaddr*)&new_client_info, sizeof(new_client_info));
				if (err < 0){
					printf("Cannot connect\n");
					return 1;
				}
				printf("Connection to %s successful. (fd = %d)\n", cmd.args[1], new_client_socket);

				// update peer list
				for(i = 0; i < 4; i ++){
					if(peer_list[i] == 0){
						peer_list[i] = new_client_socket;
						break;
					}
				}
				printf("You may now send messages to the following socket numbers: ");
				for(i = 0; i < 4; i++){
					if(peer_list[i] != 0){
						printf("%d, ",peer_list[i]);
					}
				}
				printf("\nNOTE: If messages won't send, try hitting enter on the other end.\nSTDIN halts once for some reason, then it should be fine.\n");

			}

			else if(strcmp(cmd.args[0],"send") == 0){
				char * endptr;
				char * line;
				n = 0;
				char send_buff[100];
				bzero(&send_buff, sizeof(send_buff));
				strcat(send_buff, whoami);
				strcat(send_buff, " says: ");
				printf("Please enter a message: ");
				getline(&line, &n, stdin);
				strcat(send_buff,line);
				
				//strcat(send_buff, cmd.args[2]);

		 		long converted_fd = strtol(cmd.args[1], &endptr, 10);

				write(converted_fd, send_buff, 100);
			}

			else if(strcmp(cmd.args[0],"myport") == 0){
				printf("MY PORT: %d\n",port);
			}

			else if(strcmp(cmd.args[0],"terminate") == 0){
				char * endptr;

				long converted_fd = strtol(cmd.args[1], &endptr, 10);

				close(converted_fd);
				for(i = 0; i < 4; i++){
					if(peer_list[i] == converted_fd){
						peer_list[i] = 0;
						break;
					}
				}

				printf("Closed connection on fd:%ld", converted_fd);
			}

			else if(strcmp(cmd.args[0],"list") == 0){
				printf("You can use these fd's in your 'send' commands.\nUse 'send <fd>' and a prompt will appear to enter a message.\n");
				for(i = 0; i < 4; i++){
					if(peer_list[i] != 0){
						printf("Client %d: USE FD %d\n", i, peer_list[i]);
					}
				}
			}

			
		}
	}


	return 0;
}

void displayHelp(){
	printf("[--------HELP--------]\nhelp - display this help screen\nmyip - display your own ip\nmyport - display your current listening port\n");
	printf("register <Server IP> <Port Number> - register self with the chat server\nconnect <Destination> - connects to another user via their ID number\n");
	printf("list - display current connected users and their connection informaton\nterminate <Connection ID> - terminated the user session with specified connection ID\n");
	printf("exit - close all connections and terminate process\nsend <fd> <Message> - sends message to specified client via connection ID\n");
}

int getPort(char * argv[]){
	int port = 0;
	if(argv[2]){ 		// get port number
			sscanf(argv[2], "%i", &port);
		}
	printf("Using port %d \n", port);

	return port;
}

struct Command tokenize(char * line){
	struct Command cmd;
	//cmd = zeroCommand();
	int arg = 0;
	char * start = line;
	for (char *iter = line; *iter != 0; iter++){
		if(*iter == ' ' || *iter == '\n' || *iter == '\r'){
			if(arg < 4){
				cmd.args[arg] = malloc(iter-start);
				strcat(cmd.args[arg],"\0");
				memcpy(cmd.args[arg], start, iter-start);
				start = iter;
				start++;
				
				arg++;
			}
		}

	}



	// printf("arg0: %s\n", cmd.args[0]);
	// printf("arg1: %s\n", cmd.args[1]);
	// printf("arg2: %s\n", cmd.args[2]);
	// printf("arg3: %s\n", cmd.args[3]);

	return cmd;
}