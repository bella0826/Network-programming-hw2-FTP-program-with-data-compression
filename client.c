#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h> 

#define N 512

void compression(char *command);

int main()
{
	int cli_fd;
	int nbytes;
	char command[N];
	struct sockaddr_in addr;
	char *ip,*port;		//to get the ip_addr and port number by string
	char s[2] = " ";
	int port1;

	printf("Hello, this is FTP client.\nGetting more informations, type help.\nBut first type \"link ip port\" to get link to the server.\n$ ");
	if(fgets(command,N,stdin) == NULL)
	{
		printf("Fgets Error.\n");
		exit(1);
	}
	command[strlen(command) - 1] = '\0'; 	//let '\n' become '\0'
	ip = strtok(command,s);
	ip = strtok(NULL,s);		//to separate the ip address and port number
	port = strtok(NULL,s);
	port1 = atoi(port);
	if(strncmp(command,"link",4) == 0)
	{
		if((cli_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)	//create socket
		{
			printf("Socket Error.\n");
			exit(1);
		}
		bzero(&addr,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port1);		//change the long number to network format
		addr.sin_addr.s_addr = inet_addr(ip);	//change the string to network binary format
		if(connect(cli_fd,(struct sockaddr*) &addr,sizeof(addr)) < 0)	//ask to connect to server
		{
			printf("Connect Error.\n");
			exit(1);
		}
		else
		{
			printf("The server with IP address \"%s\" has accepted your connection.\n",command + 5);
		}
	}
	else
	{
		printf("fail command.\n");
	}
		
	while(1)
	{
		bzero(command,N);	//clean the command, to prevent it from rewriting
		printf("$ ");
		if(fgets(command,N,stdin) == NULL)
		{
			printf("Fgets Error.\n");
			exit(1);
		}
		command[strlen(command) - 1] = '\0';
		if(strncmp(command,"leave",5) == 0)
		{
			printf("Bye bye ~~\n");
			if((nbytes = write(cli_fd,command,N)) < 0)	//told the server that the connection has terminated
			{
				printf("Write Error.\n");
				exit(1);
			}
			close(cli_fd);		//close the file descriptor
			exit(1);
		}
		else if(strncmp(command,"sendc",5) == 0)	//sending compressed file
		{
			compression(command);		//compression
			char a[10],b[10] = "Coded_";		//used to name the compressed file
			FILE *fp;
			char buf[N];
			int size;	//file size

			time_t timer;		//in order to print the time information
			struct tm* tm_info; 
			char tm_buf[26];
			timer = time(NULL);
			tm_info = localtime(&timer);
			strftime(tm_buf,26," %Y//%m//%d %H:%M",tm_info);

			if((nbytes = write(cli_fd,command,N)) < 0)		//write commnad to the server
			{
				printf("Write Error.\n");
				exit(1);
			}
			strcpy(a,command+6);		//make the new file name
			strcat(b,a);
			fp = fopen(b,"r");
			if(fp == NULL)
			{
				printf("File open failed.\n");
				exit(1);
			}
			fseek(fp,0,SEEK_END);		//get the file size from pointer
			size = ftell(fp);
			fseek(fp,0,SEEK_SET);

			if((nbytes = write(cli_fd,&size,sizeof(&size))) < 0)		//transmit the file size to the server
			{
				printf("Write Error.\n");
				exit(1);
			}
			while(!feof(fp))		//write the file to the server
			{
				nbytes = fread(buf,sizeof(char),sizeof(buf),fp);
				if(write(cli_fd,buf,nbytes) < 0)
				{
					printf("Write Error.\n");
					fclose(fp);
					exit(1);
				}
			}
			fclose(fp);
			printf("Time to upload: ");
			printf("%s\n",tm_buf);
		}
		else if(strncmp(command,"send",4) == 0)		//normal sending
		{
			FILE *fp;
			char buf[N];
			int size;	//file size

			time_t timer;		//in order to print the time information
			struct tm* tm_info; 
			char tm_buf[26];
			timer = time(NULL);
			tm_info = localtime(&timer);
			strftime(tm_buf,26," %Y//%m//%d %H:%M",tm_info);	//to get the transmission time

			if((nbytes = write(cli_fd,command,N)) < 0)
			{
				printf("Write Error.\n");
				exit(1);
			}
			fp = fopen(command + 5,"r");
			if(fp == NULL)
			{
				printf("File open failed.\n");
				exit(1);
			}
			fseek(fp,0,SEEK_END);		//get the file size from pointer
			size = ftell(fp);
			fseek(fp,0,SEEK_SET);

			if((nbytes = write(cli_fd,&size,sizeof(&size))) < 0)		//transmit the file size to the server
			{
				printf("Write Error.\n");
				exit(1);
			}
			while(!feof(fp))		//write the file to the server
			{
				nbytes = fread(buf,sizeof(char),sizeof(buf),fp);	//read from the file
				if(write(cli_fd,buf,nbytes) < 0)		//write to the server
				{
					printf("Write Error.\n");
					fclose(fp);
					exit(1);
				}
			}
			fclose(fp);
			printf("Sending a file \"%s\" with size of %d bytes.\n",command + 5,size);
			printf("Time to upload: ");
			printf("%s\n",tm_buf);
			
		}
		else if(strncmp(command,"help",4) == 0)		//some instructions to help user
		{
			printf("Type \"send\" to send file to the server.\n");
			printf("Type \"sendc\" to send compressed file to the server.");
			printf("Type \"leave\" to terminate the connection with the server.\n");
		}
		else if(strncmp(command,"decode",6) == 0)	//decode the compressed file
		{
			if((nbytes = write(cli_fd,command,N)) < 0)
			{
				printf("Write Error.\n");
				exit(1);
			}
		}
	}

	return 0;
}
void compression(char *command)		//fixed_length huffamn encoding
{
	FILE *fp_in,*fp_out,*fp;
	char c,dest[10],src[10] = "Coded_";
	int freq[256],word[256],num = 0,a,index = 0,sum = 0;
	int file_size,coded_size;
	fp_in = fopen(command + 6,"r");
	printf("%s\n",command+6);
	strcpy(dest,command+6);
	if(fp_in == NULL)
	{
		printf("File open failed1.\n");
		exit(1);
	}
	for(int i = 0;i < 256;i ++)		
	{
		freq[i] = 0;
	}
	while((c = fgetc(fp_in)) != EOF)	//count the frequency of the character
	{
		if(freq[(int)c] == 0)
			num ++;		//count how many different characters in the file
		else
			num = num;
		freq[(int)c] ++;
		sum ++;		//how many characters in the file in total 
	}
	a = num;
	while(a!= 0)		
	{
		index ++;
		a /= 2;
	}
	int str[index],count = num;
	for(int i = 0 ; i < 256 ;i ++)		//let every characters in this file have a huffman number
	{
		if(freq[i] != 0)
		{
			word[i] = count - num;
			num --;
		}
		else
			word[i] = 0;
	}
	fseek(fp_in,0,SEEK_SET);
	strcat(src,dest);
	fp_out  = fopen(src,"w");		//coded version of file
	fp = fopen("Coded.txt","w");		//information about coded
	if(fp_out == NULL)
	{
		printf("File open failed2.\n");
		exit(1);
	}
	int n,j = 0;
	fprintf(fp,"%d",index);
	fprintf(fp," %d",count);
	fprintf(fp," %d\n",sum);
	while((c = fgetc(fp_in)) != EOF)
	{
		j = 0;
		for(int i = 0; i < index ;i++)
			str[i] = 0;
		n = word[(int)c];
		while(n > 0)		//become the binary format
		{
			str[j] = n % 2;
			j ++ ;
			n /= 2;
		}
		j = index-1;
		for(;j>=0;j--)		//write to the file
		{
			fprintf(fp_out,"%d",str[j]);
		}
	}
	for(int i = 0;i< 256;i++)		//to printf coding informations in the Coded.txt
	{
		if(freq[i] != 0)
		{
			float rate = (double)freq[i]/sum;
			j = 0;
			fprintf(fp,"%c ",i);
			for(int i = 0; i < index ;i++)
				str[i] = 0;
			n = word[i];
			while(n > 0)
			{
				str[j] = n % 2;
				j ++;
				n /= 2;
			}
			j = index - 1;
			for(;j>=0;j--)
			{
				fprintf(fp,"%d",str[j]);
			}
			fprintf(fp," %d %3.1f",freq[i],rate * 100);
			fprintf(fp,"\n");
		}
	}
	fseek(fp_in,0,SEEK_END);
	file_size = ftell(fp_in);
	fseek(fp_out,0,SEEK_END);
	coded_size = ftell(fp_out);
	fclose(fp);
	fclose(fp_out);
	fclose(fp_in);
	int output_size = (coded_size%8 ? (coded_size/8) + 1 : coded_size/8);
	double ratio = (double)output_size/file_size ;
	printf("Original file length : %d bytes,",file_size);		//printf the compression rate on the terminal
	printf(" compressed file length : %d bytes(ratio: %2f)\n",output_size,ratio * 100);
	printf("Use fixed-length code word %d bits.\n",index);
}
