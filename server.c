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

#define N 512

void decode(char *command);

int main()
{
	int ser_fd,cli_fd;		//socket return file descriptor
	struct sockaddr_in srv,cli;	//used by bind and accept
	char command[N];		//used by read
	int nbytes;		//used by read
	char *cli_addr;
	int cli_port;

	/* socket */
	if((ser_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		printf("Socket error.\n");
		exit(1);
	}

	/* bind */
	bzero(&srv,sizeof(srv));
	srv.sin_family = AF_INET;	//IPv4
	srv.sin_port = htons(8989);	//setting port number
	srv.sin_addr.s_addr = htonl(INADDR_ANY);	//get IP address

	if(bind(ser_fd,(struct sockaddr*) &srv,sizeof(srv)) < 0)
	{
		printf("Bind Error.\n");
		exit(1);
	}
	
	/* listen */
	if(listen(ser_fd,5) < 0)
	{
		printf("Listen Error.\n");
		exit(1);
	}
	else
		printf("Waiting for connection.\n");
	/* accept */
	bzero(&cli,sizeof(cli));		//waiting for client to connect 
	socklen_t cli_len = sizeof(cli);
	cli_fd = accept(ser_fd,(struct sockaddr*) &cli, &cli_len);
	if(cli_fd < 0)
	{
		printf("Accept Error.\n");
		exit(1);
	}
	cli_addr = inet_ntoa(cli.sin_addr);	//ntohl
	cli_port = ntohs(cli.sin_port);
	printf("A client \"%s\" has connected via port num %d using SOCK_STREAM(TCP).\n",cli_addr,cli_port);		//if connect success
	
	while(1)
	{
		bzero(&command,sizeof(command));
		/* read */
		if((nbytes = read(cli_fd,command,sizeof(command))) < 0)		//read the command from the client
		{
			printf("Read Error.\n");
			exit(1);
		}
		if(strncmp(command,"leave",5) == 0)
		{
			printf("The client \"%s\" with port %d has terminated the connection.\n",cli_addr,cli_port);
			close(cli_fd);
			close(ser_fd);
			break;
		}
		else if(strncmp(command,"sendc",5) == 0)	//client send a compressed file
		{
			FILE *fp;
			char buf[N];
			int size;
			if((nbytes = read(cli_fd,&size,sizeof(&size))) < 0)		//receive the file size from client
			{
				printf("Read Error.\n");
				exit(1);
			}
			char dest[10] = "Coded_",src[10];
			strcpy(src,command+6);
			strcat(dest,src);
			fp = fopen(dest,"w");
			if(fp == NULL)
			{
				printf("File open failed.\n");
			}
			for(int i = size ; i > 0 ; i -= nbytes)
			{
				nbytes = read(cli_fd,buf,sizeof(buf));
				fwrite(buf,sizeof(char),nbytes,fp);
				bzero(&buf,sizeof(buf));
			}
			printf("The client sends a file \"%s\" with size of %d bytes.\nThe huffman coded data are stored in Code.txt.\nPlease use client to \"send\" it .\n",dest,size);		//remind the user to send Coded.txt to decode the file
			fclose(fp);
		}
		else if(strncmp(command,"send",4) == 0)		//the normal file
		{
			FILE *fp;
			char buf[N];
			int size;
			if((nbytes = read(cli_fd,&size,sizeof(&size))) < 0)		//receive the file size from client
			{
				printf("Read Error.\n");
				exit(1);
			}
			fp = fopen(command + 5,"w");
			if(fp < 0)
			{
				printf("File open failed.\n");
			}
			for(int i = size ; i > 0 ; i -= nbytes)		//read until the file end
			{
				nbytes = read(cli_fd,buf,sizeof(buf));
				fwrite(buf,sizeof(char),nbytes,fp);
				bzero(&buf,sizeof(buf));
			}

			printf("The client sends a file \"%s\" with size of %d bytes.\n",command + 5,size);
			fclose(fp);
		}
		else if(strncmp(command,"decode",6) == 0)	//decode by the Coded.txt
		{
			decode(command);
			printf("%s has decoded.\n",command+7);
		}
	}

	return 0;
}
void decode(char *command)	//decode
{
	FILE *fp_in,*fp_out;
	int index,count,freq[256],sum;
	for(int i = 0;i < 256;i ++)
	{
		freq[i] = 0;
	}
	float rate;
	char c,space,new_line;
	fp_in = fopen("Coded.txt","r");
	if(fp_in == NULL)
	{
		printf("File open failed.\n");
		exit(1);
	}
	fscanf(fp_in,"%d",&index);		//get all the informations we need
	fscanf(fp_in,"%c",&space);
	fscanf(fp_in,"%d",&count);
	fscanf(fp_in,"%c",&space);
	fscanf(fp_in,"%d",&sum);
	fscanf(fp_in,"%c",&new_line);
	char word1[count];
	for(int i = 0;i < count;i ++)
		word1[i] = '0';		//to initialize the characters' huffman number
	for(int i = 0;i < count;i ++)
	{
		int n,d=0,remainder,i = 0;
		fscanf(fp_in,"%c",&c);
		fscanf(fp_in,"%c",&space);
		fscanf(fp_in,"%d",&n);
		while(n != 0)
		{
			int s = 1;
			remainder = n % 10;
			n /= 10;
			for(int k = 0;k<i;k++)
				s *= 2;
			d += remainder * s;
			++ i;
		}
		word1[d] = c;
		fscanf(fp_in,"%c",&space);
		fscanf(fp_in,"%d",&freq[(int)c]);
		fscanf(fp_in,"%c",&space);
		fscanf(fp_in,"%f",&rate);
		fscanf(fp_in,"%c",&new_line);
	}
	fclose(fp_in);
	char dest[10] = "Coded_",src[10];		//after process with the informations, write the correct things th the file
	strcpy(src,command + 7);
	strcat(dest,src);
	fp_in = fopen(dest,"r");
	fp_out = fopen(command+7,"w");
	if(fp_in == NULL)
	{
		printf("File open failed.\n");
		exit(1);
	}
	for(int i = 0;i < sum;i ++)
	{
		int s,n = 0;
		for(int j = index - 1;j >= 0 ;j --)
		{	
			int d = 1;
			fscanf(fp_in,"%1d",&s);
			for(int k = 0;k<j;k++)
				d *= 2;
			n += s * d;
		}
		fprintf(fp_out,"%c",word1[n]);		//start write the things to file
	}
	fclose(fp_in);
	fclose(fp_out);
}
