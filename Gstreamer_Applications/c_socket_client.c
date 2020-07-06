#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr

void get_connection_str(char *request){
	
}

void connect_gstreamer(int socket_desc){
	char request[17] = {'\0'};
	char server_reply[2000] = {'\0'};
	int listen_port, stream_port;
	int ack = -1;

	while (ack == -1){
		char mode[2];

		printf("Enter Mode: ");
		gets(mode);
		
		listen_port = rand()%65534 + 1;
		stream_port = rand()%65534 + 1;

		sprintf(request, "|%s|%d|%d", mode, listen_port, stream_port);
		if( send(socket_desc , request , strlen(request) , 0) < 0)
		{
			puts("Send failed");
			break;
		}
		puts(request);
		
		//Receive a reply from the server
		if( recv(socket_desc, server_reply , 2000 , 0) < 0)
		{
			puts("recv failed");
		}
		puts("Reply Code:");
		puts(server_reply);
		ack = atoi(server_reply);
	}
	if (ack == -2){
		puts("CONNECTION ERROR");
	}
	if (ack >= 0){
		if (ack > 0){
			puts("Stream Port Changed");
			stream_port = ack;
		}
		
		puts("Everything Good to Go");
	}
	
	close(socket_desc);
}

int main(int argc , char *argv[])
{
	int socket_desc;
	struct sockaddr_in server;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 4000 );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	
	puts("Connected\n");
	
	//Send some data
	connect_gstreamer(socket_desc);
	
	return 0;
}
