#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//string operations: strlen, strstr, strchr
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<time.h> //Time to start seed of rand()
#include<pthread.h> //Only works for POSIX systems | need separate solution for windows
#include<ctype.h> //tolower
#include<signal.h> //properly shutdown on cntrl-c
#include <gst/gst.h>
#include <glib.h>

#define MAX_STREAMS 2 //Max Number of OUTGOING (limited by device computational power)
#define MAX_LISTENS 2 //Max number of INCOMING streams (limited by device/mixing technique)
#define MAX_RETRY 5 //Max times to retry with a 
#define LISTEN 1 // 01
#define STREAM 2 // 10
#define REQUEST_PORT 4000 //Standardized port for client-to-client comms
#define HOST_POLL_PERIOD 5 //Polling period for client-server connection

//GLOBAL VARIABLES
int stream_cnt = 0;
int listen_cnt = 0;


int send_port = 0;//TODO REMOVE AFTER TESTING
int self_port = 0;//TODO REMOVE AFTER TESTING

typedef struct connection_info {
	int active;
	struct sockaddr_in sock_info;
	int sock_desc;
	int in_port;
	int out_port;
	GstElement *pipe_recv;
	GstElement *pipe_send;
}Connect_Info;

Connect_Info connection_list[MAX_STREAMS + MAX_LISTENS];

// FUNCTIONS
void print_active_connections(){
	char print_str[50] = {'\0'};

	puts("*************** BEGIN ACTIVE CONNECTIONS **************");
	for(int i = 0; i < (MAX_STREAMS + MAX_LISTENS); i++){
		if (connection_list[i].active){
			sprintf(print_str, "  Connection #%d", i);
			puts(print_str);
			if (connection_list[i].in_port > 0){
				sprintf(print_str, "    Incoming stream to port %d", connection_list[i].in_port);
				puts(print_str);
			}
			if (connection_list[i].out_port > 0){
				sprintf(print_str, "    Outgoing stream to %s:%d", inet_ntoa(connection_list[i].sock_info.sin_addr), connection_list[i].out_port);
				puts(print_str);
			}
		}
	}
	sprintf(print_str, "Listen Count: %d | Stream Count: %d", listen_cnt, stream_cnt);
	puts(print_str);
	puts("*************** END ACTIVE CONNECTIONS **************");
}

int find_new_connection_spot(){
	for(int i = 0; i < (MAX_STREAMS + MAX_LISTENS); i++){
		if (connection_list[i].active != 1 ){
			//puts("Empty Spot Found");
			connection_list[i].active = 1; //set just to make sure another thread doesnt grab this spot
			return i;
		}
	}
	puts("No more open spots");
	return -1;
}

int close_connection(Connect_Info *old_connect){
	//find which pointer it is in the list
	char print_str[50] = {'\0'};

	print_active_connections();

	int list_entry = -1;
	for(int i = 0; i < (MAX_STREAMS + MAX_LISTENS); i++){
		if (old_connect == &connection_list[i]){
			list_entry = i;
		}
	}
	if (list_entry == -1){
		puts("\tUnable to locate connection pointer");
		return 0;
	}

	sprintf(print_str, "Destroying Connection #%d", list_entry);
	puts(print_str);
	
	//Adjust listen and stream cnts accordingly
	if (old_connect->in_port > 0){
		puts("\tDecrementing LISTEN count");
		listen_cnt--;
	}
	if (old_connect->out_port > 0){
		puts("\tDecrementing STREAM count");
		stream_cnt--;
	}
	
	
	//set non null pipes to GST_STATE_NULL and unref
	if (old_connect->pipe_recv != NULL){
		puts("\tShutting down Recieve Pipeline");
		gst_element_set_state (old_connect->pipe_recv, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (old_connect->pipe_recv));
	}

	if (old_connect->pipe_send != NULL){
		puts("\tShutting down Recieve Pipeline");
		gst_element_set_state (old_connect->pipe_send, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (old_connect->pipe_send));
	}

	//Close socket
	puts("\tCLOSING SOCKET");
	close(old_connect->sock_desc);

	//free struct pointer
	//free(old_connect);

	//free struct pointer
	connection_list[list_entry].active = 0;
	
	puts("\tEverything went according to plan!");

	return 1;
	
}

Connect_Info initialize_connect_info(struct sockaddr_in sock_inf, int socket_desc){
	Connect_Info temp_info;

	temp_info.active = 1;
	temp_info.sock_info = sock_inf;
	temp_info.sock_desc = socket_desc;
	temp_info.in_port = 0;
	temp_info.out_port = 0;
	temp_info.pipe_recv = NULL;
	temp_info.pipe_send = NULL;

	return temp_info;

}
	

void cntrl_c_handle(int sig){
	char print_str[50] = {'\0'};
	puts("\n\nControl C caught");
	
	print_active_connections();
	//Close out all active connections
	int ret_val;
	for(int i = 0; i < (MAX_STREAMS + MAX_LISTENS); i++){
		if (connection_list[i].active){
			ret_val = close_connection(&connection_list[i]);
			if (ret_val){
				sprintf(print_str, "\tSuccessfully Closed out Connection #%d", i);
				puts(print_str);
			}
			else{
				sprintf(print_str, "\tFAILED TO CLOSE CONNECTION #%d", i);
				puts(print_str);
			}
		}
	}

	puts("All Tidied up --> Exiting now");
	exit(0);
	
}

void sigpipe_handle(){
	//SIGPIPE occurs when you try to write to server that does not exist
	//IMPORTANT ---> crashes program if caught
	puts("SIGPIPE CAUGHT! --> I aint quittin");
}

int assign_outgoing_port(int desired_port){
	if (stream_cnt >= MAX_STREAMS){
		puts("WARN: Too Many Outgoing Streams --> not setting up Gstreamer");
		return 0;
	}

	//Does not matter what port you send to
	stream_cnt++;
	return desired_port;
}

int assign_incoming_port(int desired_port){
	if (listen_cnt >= MAX_LISTENS){
		puts("WARN: Too Many Incoming Streams --> not setting up Gstreamer");
		return 0;
	}

	//puts("Good port found");
	listen_cnt++;
	return desired_port;
}

int find_good_incoming_port(int desired_port){
	if (listen_cnt >= MAX_LISTENS){
		puts("WARN: Cannot LISTEN to anymore streams");
		return 0;
	}

	if (desired_port == 0){
		desired_port = rand()%65534 + 1;
	}

	int port_listed = 1;

	while (port_listed){
		//Check to see if port is already in use, assuming no
		port_listed = 0;
		for(int i = 0; i < (MAX_STREAMS + MAX_LISTENS); i++){
			if (connection_list[i].active){
				if (desired_port == connection_list[i].in_port){
					//if port is already listed, break and set listed to true
					port_listed = 1;
					break;
				}

			}
		}
		if (port_listed){
			//if listed, generate a new random port and repeat process to see if listed
			//loop only breaks if port is not listed
			desired_port = rand()%65534 + 1; //Need to make sure 0 is not a returned port
		}
	}
	
	return desired_port;
	
}

int find_good_outgoing_port(int desired_port){
	if (stream_cnt >= MAX_STREAMS){
		puts("WARN: Cannot STREAM to anymore devices");
		return 0;
	}
	
	if (desired_port == 0){
		desired_port = rand()%65534 + 1;
	}

	return desired_port;
}

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
	
	search = strstr(mode_str, "print");
	if (search != NULL){
		print_active_connections();
	}
	
	return mode;
}

void *watch_connection(void *sock_inf){
	//Convert void into struct and extract pertinent info
	Connect_Info *new_sock = (Connect_Info *) sock_inf;

	int socket_desc = new_sock->sock_desc;
	struct sockaddr_in server = new_sock->sock_info;
	char *server_ip = inet_ntoa(server.sin_addr);
	char server_reply[10] = {'\0'}, print_str[100] = {'\0'};

	sprintf(print_str, "Socket Desc #%d at memory location %p in 'watch_connection", new_sock->sock_desc, new_sock);
	puts(print_str);

	//Spin and occasionally poll connected host
	sprintf(print_str, "Managing Connection to Socket %d", socket_desc);
	puts(print_str);

	while(1){
		sleep(HOST_POLL_PERIOD);
		if( write(socket_desc , "Ping" , strlen("Ping")) < 0){
			//puts("ERROR: Send failed --> Server Socket Closed");
			break;
		}

		//Receive a reply from the server
		if( recv(socket_desc, server_reply , sizeof(server_reply) , 0) < 0){
	
			//puts("Server Socket Closed!, Closing my connections");
			break;
		}

		if (strstr(server_reply, "Pong") != NULL){
			puts("\tPing Sent --> Pong Received!");
		}
		else{
			//puts("Something else received, closing");
			break;
		}
	}
	sprintf(print_str, "Closing connection to Socket %d and closing pipelines", socket_desc);
	puts(print_str);
	close_connection(new_sock);
	pthread_exit(NULL);
}


void connect_gstreamer(int reg_value){
	//extract pertinent info
	Connect_Info *new_sock = &connection_list[reg_value];
	int socket_desc = new_sock->sock_desc;
	struct sockaddr_in server = new_sock->sock_info;
	char *server_ip = inet_ntoa(server.sin_addr);

	char request[17] = {'\0'}, print_str[100] = {'\0'};
	char server_reply[10] = {'\0'};
	int mode, listen_port, stream_port;
	int ack = -1;
	pthread_t new_thread;

	sprintf(print_str, "Socket Desc #%d at memory location %p in 'connect_gstreamer", new_sock->sock_desc, new_sock);
	puts(print_str);

	while (ack == -1){
		char mode_str[30] = {'\0'};

		int valid_mode = 0;
		while (!valid_mode){
			valid_mode = 1;
			printf("\n\tEnter Mode: ");
			fgets(mode_str, 30, stdin);
			mode = parse_mode(mode_str);
			
			stream_port = find_good_outgoing_port(0);
		
			if (mode < 1 || mode > 3){
				valid_mode = 0;
			}
			else{
				if ((mode & LISTEN) == LISTEN){
					listen_port = find_good_incoming_port(0); //Setting to 0 generates a random, but untaken port
					if (!(listen_port)){
						puts("ERROR: Cannot support more listens --> try a different mode");
						valid_mode = 0;
					}
				}
				else{
					//If listening not involved, just set to a number > 0
					listen_port = 1;
				}
				if ((mode & STREAM) == STREAM){
					stream_port = find_good_outgoing_port(0); //Setting to 0 generates a random port
					if (!(stream_port)){
						puts("ERROR: Cannot support more streams --> try a different mode");
						valid_mode = 0;
					}
				}
				else{
					//If streaming not involved, just set to a number > 0
					stream_port = 1;
				}
				
			}
		}

		sprintf(request, "|%d|%d|%d", mode, listen_port, stream_port);
		if( send(socket_desc , request , strlen(request) , 0) < 0){
			puts("ERROR: Send failed");
			break;
		}
		sprintf(print_str, "\t\tRequest Message: %s", request);
		puts(print_str);
		
		//Receive a reply from the server
		if( recv(socket_desc, server_reply , sizeof(server_reply) , 0) < 0){
	
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
		sprintf(print_str, "Closing connection to Socket %d", socket_desc);
		puts(print_str);
		close_connection(new_sock);
	}
	if (ack >= 0){
		if (ack > 0){
			puts("\tStream Port Changed");
			stream_port = ack;
		}
		
		puts("\n\tEverything Good to Go");
		
		if ((mode & LISTEN) == LISTEN){
			/*TODO
				set up Gstreamer pipe
				set pipe_recv for Connect_Info
			*/
			//Requesting Client would like to listen to your audio
			if(assign_incoming_port(listen_port) > 0){
				new_sock->in_port = listen_port;
				sprintf(print_str, "\tLISTEN MODE: Setting up G729 RECV for Port %d", listen_port);
				puts(print_str);

			}
			else{
				puts("error assigning incoming port :/. Not setting up G729 RECV");
			}
		
			

		}
		if ((mode & STREAM) == STREAM){
			/*TODO
				set up Gstreamer pipe
				set pipe_send for Connect_Info
			*/
			//Requesting Client would like to listen to your audio
			if(assign_outgoing_port(stream_port) > 0){
				new_sock->out_port = stream_port;
				sprintf(print_str, "\tSTREAM MODE: Setting up G729 SEND to %s:%d", server_ip, stream_port);
				puts(print_str);

			}
			else{
				puts("error assigning outgoing port :/. Not setting up G729 SEND");
			}
		}
		
		//watch_connection((void *)&connection_list[reg_value]);
		//START CONNECTION WATCH THREAD
		if (pthread_create(&new_thread, NULL, watch_connection, (void *)&connection_list[reg_value]) ){
			puts("Error Creating Connection Watch thread");
		}
		else{
			puts("Connection Watch Thread Successfully Created!");
		}

	}
}

void run_client(){
	char print_str[100] = {'\0'}, server_ip[18] = {'\0'};
	int socket_desc;
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

	//Initialize connection struct
	int reg_value = find_new_connection_spot();
	connection_list[reg_value] = initialize_connect_info(server, socket_desc);
	sprintf(print_str, "Registered Socket Desc #%d in spot #%d, at memory location %p", socket_desc, reg_value, &connection_list[reg_value]);
	puts(print_str);
	print_active_connections();
	if (reg_value >= 0){
		connect_gstreamer(reg_value);
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

void *handle_new_connection(void *sock_inf){
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

	Connect_Info *new_sock = (Connect_Info *) sock_inf;

	int new_socket = new_sock->sock_desc;
	struct sockaddr_in client = new_sock->sock_info;
	
	char *client_ip = inet_ntoa(client.sin_addr);
	int client_port = ntohs(client.sin_port);
	char print_str[200] = {'\0'};
	int ack, mode, listen_port, stream_port;
	
	sprintf(print_str, "Socket Desc #%d at memory location %p in 'handle_new_connection", new_socket, new_sock);
	puts(print_str);
	
	sprintf(print_str, "\nConnection Accepted from %s:%d", client_ip,client_port);
	puts(print_str);

	sprintf(print_str, "\tNew Socket Value: %d", new_socket);
	puts(print_str);
	
	//Establish Gstreamer Connection
	for (int retry = 0; retry < MAX_RETRY; retry++){
		char setup[500] = {'\0'};
		int recv_val;
		
		if( (recv_val = recv(new_socket, setup , sizeof(setup) , 0)) < 0 ){
			sprintf(print_str, "Connection to socket %d failed (%d) or closed before Gstreamer established, closing socket", new_socket, recv_val);
			puts(print_str);
			close_connection(new_sock);
			return (void *)1;
		}
		
		if(!((int)strlen(setup) > 0)){
			sprintf(print_str, "0 Length Data recieved, closing socket ");
			puts(print_str);
			close_connection(new_sock);
			return (void *)1;
		}
 
		mode = extract_packet_value(setup,1);
		listen_port = extract_packet_value(setup,2);
		stream_port = extract_packet_value(setup,3);
		
		sprintf(print_str, "\t\tMode: %d | Listen: %d | Stream: %d", mode, listen_port, stream_port);
		puts(print_str);

		if ((mode < 1) || (mode > 4) || (listen_port < 1) || (listen_port > 65535) || (stream_port < 1) || (stream_port > 65535)){
			//BAD packet. Tell requesting client to try again
			puts("WARN: COULD NOT PARSE -> BAD PACKET or Improper values");
			ack = -1;
		}
		else{
			ack = 0;
			//Packet Was Good
			if ((mode & LISTEN) == LISTEN){
				//Requesting Client would like to listen to your audio
				if (!find_good_outgoing_port(listen_port)){
					//Too many outgoing ports
					ack = -2;
				}
			}
			if ((mode & STREAM) == STREAM){
				//Requesting Client would like to stream their audio to you
				int assigned_port;
				assigned_port = find_good_incoming_port(stream_port);

				if (!assigned_port){
					ack=-2;
				}
				else{
					if (!(assigned_port == stream_port)){
						//Desired port did not work
						//Send reply with port that was chosen
						ack = assigned_port;
						stream_port = ack;
						puts("\t\tAssigned port was different from desired port");
					}
					
				}
				
			}
		}

		//Send Back acknowledgement
		//If all good, send 0
		//If alteration to streaming from requesting client, send back adjusted port
		char ack_msg[10] = {'\0'};

		if ( (retry == (MAX_RETRY - 1)) && (ack == -1) ){
			ack = -2;
			puts("\tERROR: Unable to Connect after MAX_REPLY times");
		}

		sprintf(ack_msg, "%d", ack);
		write(new_socket , ack_msg , strlen(ack_msg));
		
		if (!(ack == -1)){
			//Provided ACK is not -1, leave loop
			puts("\tConnection Status Established! Leaving Loop");
			break;
		}

	}
	//Gstreamer Connection Navigated
	//IFF ack is greater than or equal to 0 (good or changed stream port), setup gstreamer accordingly
	if (ack >= 0 ){
		if ((mode & LISTEN) == LISTEN){
			/*TODO
				set up Gstreamer pipe
				set pipe_send for Connect_Info
			*/
			//Requesting Client would like to listen to your audio
			if(assign_outgoing_port(listen_port) > 0){
				new_sock->out_port = listen_port;
				sprintf(print_str, "\tLISTEN MODE: Setting up G729 SEND to %s:%d", client_ip, listen_port);
				puts(print_str);

			}
			else{
				puts("error assigning outgoing port :/. Not setting up G729 SEND");
			}
			
		}
		if ((mode & STREAM) == STREAM){
			/*TODO
				set up Gstreamer pipe
				set pipe_send for Connect_Info
			*/
			//Requesting Client would like to stream their audio to you
			
			if(assign_incoming_port(stream_port) > 0){
				new_sock->in_port = stream_port;
				sprintf(print_str, "\tSTREAM MODE: Setting up G729 RECV for Port %d", stream_port);
				puts(print_str);

			}
			else{
				puts("error assigning incoming port :/. Not setting up G729 RECV");
			}
		
		}
	}
	sprintf(print_str, "Managing Connection to Socket %d", new_socket);
	puts(print_str);

	int read_size;
	char nom_read[200];
	while ( (read_size = recv(new_socket, nom_read , sizeof(nom_read) , 0)) > 0){
		if (strstr(nom_read, "Ping") != NULL){
			write(new_socket, "Pong", strlen("Pong"));
			//puts("\tPing received --> sending Pong");
		}
	}
	sprintf(print_str, "CONNECTION TO SOCKET %d CLOSED, shutting down pipelines that were set up", new_socket);
	puts(print_str);
	close_connection(new_sock);
	
}

void *run_server(){
	int socket_desc , new_socket , c;
	struct sockaddr_in server , client;
	char print_str[100] = {'\0'};

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
		puts("WARN: Server Bind FAILED");
		return (void *)1;
	}
	puts("Server Binded Successfully");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while(1){
		pthread_t new_thread;
		new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (new_socket > 0){
			//Initialize connection struct
			//Initialize connection struct
			int reg_value = find_new_connection_spot();
			print_active_connections();
			if (reg_value >= 0){
				connection_list[reg_value] = initialize_connect_info(client, new_socket);
				sprintf(print_str, "Registered Socket Desc #%d in spot #%d, at memory location %p", socket_desc, reg_value, &connection_list[reg_value]);
				puts(print_str);
			
				if (pthread_create(&new_thread, NULL, handle_new_connection, (void *)&connection_list[reg_value]) ){
					puts("Error Creating Connection Handle thread");
				}
				else{
					puts("Handle Connection Thread Successfully Created!");
				}
			}
			else{
				puts("Error Registering Connection");
				close(new_socket);
			}

		}
	}
}	


int main(int argc , char *argv[]){
	
	time_t t;
	pthread_t server_thread;
	char port_input[10] = {'\0'};

	/*Attach Ctrl-C handle to close sockets*/
	signal(SIGINT, cntrl_c_handle);
	signal(SIGPIPE, sigpipe_handle);
	
	/*TODO REMOVE AFTER TESTING*/
	printf("Enter port you would like to bind server to: ");
	fgets(port_input, 10, stdin);
	self_port = atoi(port_input);

	printf("Enter port you would like connect with: ");
	fgets(port_input, 10, stdin);
	send_port = atoi(port_input);
	/*REMOVE AFTER TESTING */
	
	
	/* Assign standard client-to-client port so other client does not stream to it*/
	//assign_incoming_port(self_port); //TODO replace with REQUEST_PORT after testing
	
	/* Intializes random number generator */
	srand((unsigned) time(&t));
	
	if ( pthread_create(&server_thread, NULL, run_server, NULL) ){
		puts("Error Creating run server thread");
	}
	else{
		puts("Server Thread Successfully Created!");
	}
	
	sleep(1);

	while(1){
		run_client();
	}

	return 0;
}
