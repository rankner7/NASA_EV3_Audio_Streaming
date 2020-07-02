#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write

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
	
	


int main(int argc , char *argv[])
{
	char target_str[256], *combine[10]; //Printing with puts dynamically

	int socket_desc , new_socket , c;
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
	new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (new_socket<0)
	{
		perror("accept failed");
		return 1;
	}
	
	char *client_ip = inet_ntoa(client.sin_addr);
	int client_port = ntohs(client.sin_port);
	
	combine[0]="Connection Accepted from ";combine[1]=client_ip;combine[2]=NULL;
	print_strs(target_str, combine);
	
	//Reply to the client
	message = "Hello Client , I have received your connection. But I have to go now, bye\n";
	write(new_socket , message , strlen(message));
	return 0;
}
