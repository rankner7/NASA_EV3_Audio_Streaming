#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write

#define MAX_STREAMS 10 //Max number of ports to be recieving
#define MAX_RETRY 3 //Max times to retry with a 
#define STREAM 1 // 01
#define LISTEN 2 // 10

//GLOBAL VARIABLES
int stream_ports[MAX_STREAMS];
int connection_cnt = 0;

//Initialize random kernel
//time_t t;
//srand((unsigned) time(&t) );

void print_strs(char *target, char **str_arr){
	
	/***** Example Use ***********/
	//combines[0] = "hey "; combines[1] = "it "; combines[2] = "worked!!!";combines[3] = NULL; 
	//print_strs(target_str, combines);

	char *current_str = str_arr[0];
	int cnt = 0;
	while (current_str != NULL){
		strcat(target, current_str);
		cnt++;
		current_str = str_arr[cnt];
	}
	puts(target);
	
	//Clean up
	target[0] = '\0';
	str_arr[0] = NULL;
}

void print_new_connection(struct sockaddr_in client, int new_socket){
	char *client_ip = inet_ntoa(client.sin_addr);
	int client_port = ntohs(client.sin_port);
	char target_str[250] = {'\0'};
	char *combine[10]; //Printing with puts dynamically
	
	char int_str[10];
	sprintf(int_str, "%d", client_port);
	combine[0]="\nConnection Accepted from ";combine[1]=client_ip;combine[2]=":";combine[3]=int_str;combine[4]=NULL;
	print_strs(target_str, combine);

	
	sprintf(int_str, "%d", new_socket);
	combine[0]="\tNew Socket Value: ";combine[1]=int_str;combine[2]=NULL;
	print_strs(target_str, combine);
}

int assign_stream_port(int desired_port){
	int port_listed = 1;
	while (port_listed){
		//Check to see if port is already in use, assuming no
		port_listed = 0;
		for(int i = 0; i < connection_cnt; i++){
			if (desired_port == stream_ports[i]){
				//if port is already listed, break and set listed to true
				puts("\t\tPORT ALready Taken!");
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
	if (connection_cnt < MAX_STREAMS){
		//Can Still make connections
		//Increase connection cnt, add desired_port to list, return port set
		stream_ports[connection_cnt] = desired_port;
		connection_cnt++;
		return desired_port;
	}
	else{
		//If connection_cnt = (MAX-1), return -1 because cannot connect
		puts("WARN: CANNOT CONNECT: MAX CONNECTED ALREADY");
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
		01: STREAM
		10: LISTEN
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

	print_new_connection(client, new_socket);

	for (int retry = 0; retry < MAX_RETRY; retry++){
		char setup[500] = {'\0'};
		int ack, mode, listen_port, stream_port;
		
		if( recv(new_socket, setup , 2000 , 0) < 0)
		{
			puts("recv failed");
		}
		
		mode = extract_packet_value(setup,1);
		listen_port = extract_packet_value(setup,2);
		stream_port = extract_packet_value(setup,3);

		if ((mode == -1) || (mode > 4) || (listen_port == -1) || (listen_port > 65535) || (stream_port == -1) || (stream_port > 65535)){
			//BAD packet. Tell requesting client to try again
			puts("WARN: COULD NOT PARSE -> BAD PACKET or Improper values");
			//TODO add accountability for invalid mode or ports
			ack = -1;
		}
		else{
			ack = 0;
			//Packet Was Good
			if (!(mode & LISTEN)){
				//Listen Mode requested (Requesting Client would like to listen to your audio)
				//TODO Set Up GSTREAMER G729_SEND code to client IP and listen_port
				char *client_ip = inet_ntoa(client.sin_addr);
				int client_port = listen_port;
				char target_str[250] = {'\0'};
				char *combine[10]; //Printing with puts dynamically
				char int_str[10];
				sprintf(int_str, "%d", client_port);
				combine[0]="\tLISTEN MODE: Setting up G729 SEND to  ";combine[1]=client_ip;combine[2]=":";combine[3]=int_str;combine[4]=NULL;
				print_strs(target_str, combine);
				
			}
			if (!(mode & STREAM)){
				//Stream Mode Requested (Requesting Client would like to stream their audio to you)
				int assigned_port;
				assigned_port = assign_stream_port(stream_port);
				if (!(assigned_port == stream_port)){
					//Desired port did not work
					//Send reply with port that was chosen
					ack = assigned_port;
					puts("\t\tAssigned port was different from desired port");
				}
				if (assigned_port > 0){
					//TODO Set Up GSTREAMER G729_RECV code for assigned Port
					char target_str[250] = {'\0'};
					char *combine[10]; //Printing with puts dynamically
					char int_str[10];
					sprintf(int_str, "%d", assigned_port);
					combine[0]="\tSTREAM MODE: Setting up G729 RECV for Port ";combine[1]=int_str;combine[2]=NULL;
					print_strs(target_str, combine);
				}
			}
			//Send Back acknowledgement
			//If all good, send 0
			//If alteration to streaming from requesting client, send back adjusted port
		}

		char ack_msg[10];
		if (retry == (MAX_RETRY - 1)){
			//Send -2 if connection failed more than 3 times
			sprintf(ack_msg, "%d", -2);
			write(new_socket , ack_msg , strlen(ack_msg));
			puts("\tERROR: Unable to Connect! Leaving");
		}
		else{
			sprintf(ack_msg, "%d", ack);
			write(new_socket , ack_msg , strlen(ack_msg));
		}
		
		if (!(ack == -1)){
			//Provided ACK is not -1, leave loop
			puts("\tConnection Established! Leaving");
			return;
		}

	}
	
}
	


int main(int argc , char *argv[]){
	
	int socket_desc , new_socket , c, connection_cnt;
	struct sockaddr_in server , client;
	char *message;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 4000 );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	//connection_cnt=0;
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
		handle_new_connection(client, new_socket);
		
	}
	if (new_socket<0)
	{
		perror("accept failed");
		return 1;
	}

	return 0;
}
