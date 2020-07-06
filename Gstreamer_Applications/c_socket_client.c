#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<time.h>
#include<ctype.h> //tolower

#define MAX_STREAMS 10 //Max number of ports to be recieving
#define MAX_RETRY 3 //Max times to retry with a 
#define LISTEN 1 // 01
#define STREAM 2 // 10
#define REQUEST_PORT 4000

void parse_reply_code(int reply, char *copy_str){
	if (reply == -2){
		sprintf(copy_str, "\t\tConnection Error: Either Attempts Maxed out or Connecting device has maxed out connections");
	}
	if (reply == -1){
		sprintf(copy_str, "\t\tBad Packet Format: Either Packet was jumbled or inputs were out of range");
	}
	if (reply == 0){
		sprintf(copy_str, "\t\tALL GOOD: Packet was parsed and configuration was accepted!");
	}
	if (reply > 0){
		sprintf(copy_str, "\t\tStream Port Change: Code value is port to be used");
	}
}

void trim_user_input(char *input_str){
	//fgets always includes '\n' if full input is captured
	//put ending delimeter at that location
	char *search = strchr(input_str, '\n');
	if ( search != NULL){
		*search = '\0';
	}
	
}

int parse_mode(char *mode_str){
	int mode = 0;
	char listen[] = "listen", stream[] = "stream";
	char *search = NULL;
	char print_str[50] = {'\0'};

	//Get Rid of trailing
	trim_user_input(mode_str);

	//lowercase input
	for(int i = 0; mode_str[i]; i++){
 		mode_str[i] = tolower(mode_str[i]);
	}

	search = strstr(mode_str, "listen");
	if (search != NULL){
		mode = mode | LISTEN;
	}

	search = strstr(mode_str, "stream");
	if (search != NULL){
		mode = mode | STREAM;
	}
	
	return mode;
}

void connect_gstreamer(){
	char request[17] = {'\0'}, print_str[100] = {'\0'};
	char server_reply[10] = {'\0'}, server_ip[18] = {'\0'};
	int mode, listen_port, stream_port, socket_desc;
	int ack = -1;
	struct sockaddr_in server;
	
	//Create socket
	printf("\nEnter Connecting Device IP: ");
	fgets(server_ip, 18, stdin);
	trim_user_input(server_ip);
	

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		puts("ERROR: Could not create socket");
		return;
	}
		
	server.sin_addr.s_addr = inet_addr(server_ip);
	server.sin_family = AF_INET;
	server.sin_port = htons( REQUEST_PORT );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0){
		puts("ERROR: Could not connect to Device");
		return;
	}
	
	puts("\tConnected to Device!");

	while (ack == -1){
		char mode_str[30] = {'\0'};

		printf("\n\tEnter Mode: ");
		fgets(mode_str, 30, stdin);
		mode = parse_mode(mode_str);
		
		listen_port = rand()%65534 + 1;
		stream_port = rand()%65534 + 1;

		sprintf(request, "|%d|%d|%d", mode, listen_port, stream_port);
		if( send(socket_desc , request , strlen(request) , 0) < 0){
			puts("ERROR: Send failed");
			break;
		}
		sprintf(print_str, "\t\tRequest Message: %s", request);
		puts(print_str);
		
		//Receive a reply from the server
		if( recv(socket_desc, server_reply , 2000 , 0) < 0){
	
			puts("ERROR: Receive failed");
			break;
		}
		ack = atoi(server_reply);
		sprintf(print_str, "\tReply Code: %d", ack);
		puts(print_str);

		parse_reply_code(ack, print_str);
		puts(print_str);
	}
	if (ack == -2){
		puts("CONNECTION ERROR");
	}
	if (ack >= 0){
		if (ack > 0){
			puts("\tStream Port Changed");
			stream_port = ack;
		}
		
		puts("\n\tEverything Good to Go");
		
		if ((mode & LISTEN) == LISTEN){
			//Listen Mode requested (Requesting Client would like to listen to your audio)
			//TODO Set Up GSTREAMER G729_RECV code to listen_port
			sprintf(print_str, "\tLISTEN MODE: Setting up G729_RECV for Port %d", listen_port);
			puts(print_str);

		}
		if ((mode & STREAM) == STREAM){
			//TODO Set Up GSTREAMER G729_SEND code to server_ip and stream_port
			sprintf(print_str, "\tSTREAM MODE: Setting up G729_SEND to %s:%d", server_ip, stream_port);
			puts(print_str);
		}
	}
	
	close(socket_desc);
}

int main(int argc , char *argv[])
{
	/* Intializes random number generator */
	time_t t;
	srand((unsigned) time(&t));

	//Send some data
	while (1){
		connect_gstreamer();
	}
	
	return 0;
}
