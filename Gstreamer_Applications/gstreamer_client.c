#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//string operations: strlen, strstr, strchr
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<time.h> //Time to start seed of rand()
#include<pthread.h> //Only works for POSIX systems | need separate solution for windows
#include<ctype.h> //tolower

#define MAX_STREAMS 10 //Max Number of Ports Receiving Data on (limited by device/mixing technique)
#define MAX_LISTENS 10 //Max number of Streams sending to other clients (limited by computation power)
#define MAX_RETRY 3 //Max times to retry with a 
#define LISTEN 1 // 01
#define STREAM 2 // 10
#define REQUEST_PORT 4000 //Standardized port for client-to-client comms

//GLOBAL VARIABLES
int stream_ports[MAX_STREAMS];
int stream_cnt = 0;
int listen_cnt = 0;


int send_port = 0;//TODO REMOVE AFTER TESTING
int self_port = 0;//TODO REMOVE AFTER TESTING

// FUNCTIONS
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
	server.sin_port = htons( send_port ); //TODO replace with REQUEST_PORT after testing

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


int assign_stream_port(int desired_port){
	int port_listed = 1;
	while (port_listed){
		//Check to see if port is already in use, assuming no
		port_listed = 0;
		for(int i = 0; i < stream_cnt; i++){
			if (desired_port == stream_ports[i]){
				//if port is already listed, break and set listed to true
				//puts("\t\tPORT ALready Taken!");
				port_listed = 1;
				break;
			}
		}
		if (port_listed){
			//if listed, generate a new random port and repeat process to see if listed
			//loop only breaks if port is not listed
			desired_port = rand()%65534 + 1; //Need to make sure 0 is not a returned port
		}
	}

	//Found a port that's not listed
	if (stream_cnt < MAX_STREAMS){
		//Can Still make connections
		//Increase connection cnt, add desired_port to list, return port set
		stream_ports[stream_cnt] = desired_port;
		stream_cnt++;
		return desired_port;
	}
	else{
		//If stream_cnt = (MAX-1), return -2 because cannot connect
		//puts("WARN: CANNOT CONNECT: MAX CONNECTED ALREADY");
		return -2;
	}

}

int extract_packet_value(char *packet, int val_ind){
	char *temp_str = packet;
	const char delim = '|';
	int good_packet = 1;
	for(int i = 0; i < val_ind; i++){
		//Search for first occurance of delim, increase pointer by one and repeat
		temp_str = strchr(temp_str, delim);
		if (temp_str == NULL){
			good_packet = 0;
			break;
		}
		temp_str++; //increas pointer value to discard found character
	}
	if (good_packet){
		return atoi(temp_str);
	}
	else{
		return -1;
	}
}

void handle_new_connection(struct sockaddr_in client, int new_socket){
	/*Setup String Format
	From the client requesting
		|mode|ListenPort|StreamPort
	Mode:
		01: LISTEN
		10: STREAM
		11: STREAM & LISTEN

	ListenPort:
		 - The requesting client would like to listen to your audio
		 - the integer port requesting client will listen on (the port this client will need to stream to
	StreamPort:
		 - The requesting client would like to stream audio to you
		 - the integer port the requesting client would like to stream to you
		 - Need to verfiy this port is available, and if not send back a proposed port
	Setup String Format*/

	/*ACK CODE
		 - -2 -> Max Connections reached
		 - -1 -> Bad Request Packet
		 -  0 -> All Good
		 -  1-65535 -> Changed Port
	ACK CODE*/


	char *client_ip = inet_ntoa(client.sin_addr);
	int client_port = ntohs(client.sin_port);
	char print_str[250] = {'\0'};
	
	sprintf(print_str, "\nConnection Accepted from %s:%d", client_ip,client_port);
	//puts(print_str);

	sprintf(print_str, "\tNew Socket Value: %d", new_socket);
	//puts(print_str);

	for (int retry = 0; retry < MAX_RETRY; retry++){
		char setup[500] = {'\0'};
		int ack, mode, listen_port, stream_port;
		
		if( recv(new_socket, setup , 2000 , 0) < 0)
		{
			//puts("recv failed");
		}
		
		mode = extract_packet_value(setup,1);
		listen_port = extract_packet_value(setup,2);
		stream_port = extract_packet_value(setup,3);
		
		sprintf(print_str, "\t\tMode: %d | Listen: %d | Stream: %d", mode, listen_port, stream_port);
		//puts(print_str);

		if ((mode < 1) || (mode > 4) || (listen_port == -1) || (listen_port > 65535) || (stream_port == -1) || (stream_port > 65535)){
			//BAD packet. Tell requesting client to try again
			//puts("WARN: COULD NOT PARSE -> BAD PACKET or Improper values");
			ack = -1;
		}
		else{
			ack = 0;
			//Packet Was Good
			if ((mode & LISTEN) == LISTEN){
				//Listen Mode requested (Requesting Client would like to listen to your audio)
			
				listen_cnt++;
				if (listen_cnt > MAX_LISTENS){
					//Check to make sure resources are not being strained
					//puts("WARN: Already sending out too many streams");
					ack = -2;
					
				}
				else{
					
					sprintf(print_str, "\tLISTEN MODE: Setting up G729 SEND to %s:%d", client_ip, listen_port);
					//puts(print_str);
					//TODO Set Up GSTREAMER G729_SEND code to client IP and listen_port
				}
				
			}
			if ((mode & STREAM) == STREAM){
				//Stream Mode Requested (Requesting Client would like to stream their audio to you)
				int assigned_port;
				assigned_port = assign_stream_port(stream_port);
				if (!(assigned_port == stream_port)){
					//Desired port did not work
					//Send reply with port that was chosen
					ack = assigned_port;
					//puts("\t\tAssigned port was different from desired port");
				}
				if (assigned_port > 0){
					//TODO Set Up GSTREAMER G729_RECV code for assigned Port
					sprintf(print_str, "\tSTREAM MODE: Setting up G729 RECV for Port %d", assigned_port);
					//puts(print_str);
				}
			}
			//Send Back acknowledgement
			//If all good, send 0
			//If alteration to streaming from requesting client, send back adjusted port
		}

		char ack_msg[10] = {'\0'};
		if ( (retry == (MAX_RETRY - 1)) && (ack == -1) ){
			//Send -2 if connection failed MAX_RETRY times
			sprintf(ack_msg, "%d", -2);
			write(new_socket , ack_msg , strlen(ack_msg));
			//puts("\tERROR: Unable to Connect! Leaving");
		}
		else{
			sprintf(ack_msg, "%d", ack);
			write(new_socket , ack_msg , strlen(ack_msg));
		}
		
		if (!(ack == -1)){
			//Provided ACK is not -1, leave loop
			//puts("\tConnection Status Established! Leaving");
			return;
		}

	}
	
}

void *run_server(){
	int socket_desc , new_socket , c;
	struct sockaddr_in server , client;

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( self_port ); //TODO replace with REQUEST_PORT after testing
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
		puts("Server bind failed");
		return (void *)1;
	}
	puts("Server Binded Successfully");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while(1){
		new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (new_socket > 0){
			handle_new_connection(client, new_socket);
		}
		else{
			//puts("Connection failed with new socket");
		}
	}
}	


int main(int argc , char *argv[]){
	
	time_t t;
	pthread_t server_thread;
	char port_input[10] = {'\0'};
	
	/*TODO REMOVE AFTER TESTING*/
	printf("Enter port you would like to bind server to: ");
	fgets(port_input, 10, stdin);
	self_port = atoi(port_input);

	printf("Enter port you would like connect with: ");
	fgets(port_input, 10, stdin);
	send_port = atoi(port_input);
	/*REMOVE AFTER TESTING */

	/* Assign standard client-to-client port so other client does not stream to it*/
	assign_stream_port(self_port); //TODO replace with REQUEST_PORT after testing
	
	/* Intializes random number generator */
	srand((unsigned) time(&t));
	
	if ( pthread_create(&server_thread, NULL, run_server, NULL) ){
		puts("Error Creating run server thread");
	}
	else{
		puts("Server Thread Successfully Created!");
	}
	
	sleep(2);

	while(1){
		connect_gstreamer();
	}

	return 0;
}
