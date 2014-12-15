/**
 * @karamvee_assignment1
 * @author  Karamveer Choudhary <karamvee@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

// Remote File Sharing System
//Implement: Develop a simple application for file sharing among remote hosts and observe some network characteristics using it.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include "../include/global.h"

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
#define CHUNKSIZE 1024 
struct IP_Table
{
	int connID;
	int fd;
	char connectionName[100];
	char connectionIP[100];
	char port[20];
	struct IP_Table *next;
};

struct stats
{
	char hostName[30];
	char hostName1[30];
	int  uploads;
	double avgUploadRate;
	int  downloads;
	double avgDownloadRate;
	struct stats *next;
};

typedef struct IP_Table* Table;
typedef struct stats* STATS;
char serverIp[INET6_ADDRSTRLEN];
char clientIp[INET6_ADDRSTRLEN];
int clientPort=0,serverPort=0,clientsockfd,serversockfd,size=100;
int downloadSock1=0,downloadSock2=0,downloadSock3=0;
bool server = false;
Table clientListHead =NULL;
fd_set master,read_fd;
fd_set clientMaster;
int downloading1=0,downloading2=0,downloading3=0,downloading=0;

int noOfClients = 0;
Table serverList = NULL;

STATS clientStat = NULL;
STATS serverStat = NULL;
STATS serverStatHead = NULL;

int mainMenu(char*,struct IP_Table*);
Table deserialize(char*,int);

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 *
 *char *getMyIP() 
 * This function retrieves the local IP address of the machine by connecting to google DNS.
 * It returns the IP of machine as character pointer.
 *
 */
//Google DNS method was suggested in recitations.
//Other API calls  are taken from beej's guide.  
char * getMyIP()
{ 
	struct addrinfo hints;
	struct addrinfo* info;
	struct sockaddr_in local_addr;
	socklen_t addr_len = sizeof(local_addr);
	char *google_ip= "8.8.8.8";
	char *g_port="53";
	int ret = 0,sockfd;
	char *myip = (char*) malloc(INET6_ADDRSTRLEN);

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo(google_ip, g_port, &hints, &info)) != 0) {
		printf("[ERROR]: getaddrinfo error: %s\n", gai_strerror(ret));
		return NULL;
	}
	sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sockfd <= 0) {
		perror("socket");
		return NULL;
	}

	if (connect(sockfd, info->ai_addr, info->ai_addrlen) < 0) {
		perror("connect");
		close(sockfd);
		return NULL;
	}

	if (getsockname(sockfd, (struct sockaddr*)&local_addr, &addr_len) < 0) {
		perror("getsockname");
		close(sockfd);
		return NULL;
	}
	if (inet_ntop(local_addr.sin_family, &(local_addr.sin_addr), myip, INET6_ADDRSTRLEN) == NULL) {
		perror("inet_ntop");
		return NULL;
	}
	return myip;
}

/*
 *
 *Function updatePort
 *This function is used to update port number of an existing peer by doing a name lookup in IP_List sent by server.
 *It returns interger to check if the value was sucessfully updated or not. 
 *ret value 1 means port is sucessfully updated.
 * 
 */
int updatePort( Table clientHead, Table serverHead, struct sockaddr* remoteaddr)
{

	int rv=0;
	Table temp,cur,cur1;
	temp = (Table) malloc( sizeof(struct IP_Table));
	if(getnameinfo(remoteaddr,sizeof(*remoteaddr),temp->connectionName,sizeof (temp->connectionName),temp->port,sizeof (temp->port),0)!=0)
		printf("Error in getnameinfo: %s\n",gai_strerror(rv));
	inet_ntop(remoteaddr->sa_family,get_in_addr(remoteaddr),temp->connectionIP, INET6_ADDRSTRLEN);

	if(serverHead == NULL)
	{
		printf("Server IP_Table is empty");
		return -1 ;
	}
	if(clientHead == NULL)
	{
		printf("Client IP_Table is empty");
		return -1;
	}
	cur  = serverHead;
	cur1 = clientHead;
	while(cur!=NULL)
	{
		if((strcmp(cur->connectionIP,temp->connectionIP)== 0) && (strcmp(cur->connectionName,temp->connectionName)==0))
			break;
		cur=cur->next;
	}
	while(cur1!=NULL)
	{
		if((strcmp(cur1->connectionIP,temp->connectionIP)== 0) && (strcmp(cur1->connectionName,temp->connectionName)==0))
			break;
		cur1=cur1->next;
	}
	if(cur!=NULL && cur1!=NULL)
	{
		strcpy(cur1->port,cur->port);
		return 1;
	}
	free(temp);
	return -1;
}

/*
 *saveAndCreate(struct sockaddr*, Table, int)
 *This function creates a new node and saves it in the Table linked list.
 *It reads connectionName, connectionIP from remoteaddr passed and uses int passed to save the sockfd of the node.
 * 
 */
Table saveAndCreate(struct sockaddr* remoteaddr,Table head,int filedesc)
{ 
	int rv=0;
	Table temp,itr;
	temp = (Table) malloc( sizeof(struct IP_Table));
	temp->fd = filedesc;
	if(getnameinfo(remoteaddr,sizeof(*remoteaddr),temp->connectionName,sizeof (temp->connectionName),temp->port,sizeof (temp->port),0)!=0)
		printf("Error in getnameinfo: %s\n",gai_strerror(rv));
	inet_ntop(remoteaddr->sa_family,get_in_addr(remoteaddr),temp->connectionIP, INET6_ADDRSTRLEN);

	if(head==NULL){
		temp->next = head;
		temp->connID = 1;
		return temp;
	}
	else
	{
		itr = head;
		while(itr->next!=NULL)
		{
			rv=rv+1;
			itr=itr->next;
		}
		itr->next=temp;
		temp->connID=(itr->connID)+1;
		temp->next=NULL;
		return head;
	}

	return head;

}

/*
 *
 *This functions is used to serializeStats on each client when server pings them for STATS.
 *This function takes the local stats structure head pointer and pointer to length. 2nd argument is used to return the len of buffer which is allocated
 *to write stats structure. 
 *The function returns a serialized character array containing stats pertaining to this particular node.
 *
 * */
char * serializeStats(struct stats *head, int *len)
{
	int totalsize=0,seek=0;
	STATS temp,var;
	char * buffer;
	char token[] = "INCOMINGSTATS";
	// Malloc  a buffer big enought to store full linked list.
	temp = head;

	while(temp!=NULL)
	{
		totalsize+= sizeof(struct stats);
		temp=temp->next;	
	}

	totalsize +=sizeof(token);	
	buffer = (char*) malloc(totalsize);
	// Copy Object wise to the allocated buffer.
	*len=totalsize;	
	var = head; 

	memcpy(&buffer[seek], token, sizeof(token));
	seek += sizeof(token); 

	while(var!=NULL)
	{
		memcpy(&buffer[seek], &var->uploads, sizeof(var->uploads));
		seek += sizeof(var->uploads); 

		memcpy(&buffer[seek], &var->hostName, sizeof(var->hostName));
		seek += sizeof(var->hostName); 

		memcpy(&buffer[seek], &var->hostName, sizeof(var->hostName));
		seek += sizeof(var->hostName); 

		memcpy(&buffer[seek], &var->avgUploadRate, sizeof(var->avgUploadRate));
		seek += sizeof(var->avgUploadRate); 

		memcpy(&buffer[seek], &var->downloads, sizeof(var->downloads));
		seek += sizeof(var->downloads); 

		memcpy(&buffer[seek], &var->avgDownloadRate, sizeof(var->avgDownloadRate));
		seek += sizeof(var->avgDownloadRate);

		var = var->next; 
	}
	return buffer; 
}

/*
 *
 *This functions is used to serialize Table data struture on server before sending the IP Table to each client.
 *This function takes the local IP_Table structure head pointer and pointer to length. 2nd argument is used to return the len of buffer which is allocated
 *to write Table structure. 
 *The function returns a serialized character array containing the information about all the connected clients.
 *
 * */
char * serializeTable(struct IP_Table *head,int *len)
{
	int totalsize=0,seek=0;
	Table temp,var;
	char * buffer;
	// Malloc  a buffer big enought to store full linked list.
	temp = head;
	while(temp!=NULL)
	{
		totalsize+= sizeof(struct IP_Table);
		temp=temp->next;	
	}

	buffer = (char*) malloc(totalsize);
	// Copy Object wise to the allocated buffer.
	*len=totalsize;	

	var = head;
	while(var!=NULL)
	{
		memcpy(&buffer[seek], &var->connID, sizeof(var->connID));
		seek += sizeof(var->connID); 

		memcpy(&buffer[seek], &var->fd, sizeof(var->fd));
		seek += sizeof(var->fd); 

		memcpy(&buffer[seek], &var->connectionName, sizeof(var->connectionName));
		seek += sizeof(var->connectionName); 

		memcpy(&buffer[seek], &var->connectionIP, sizeof(var->connectionIP));
		seek += sizeof(var->connectionIP); 

		memcpy(&buffer[seek], &var->port, sizeof(var->port));
		seek += sizeof(var->port);

		if(seek <= sizeof(struct IP_Table))
			seek=sizeof(struct IP_Table);
		else if(seek <= 2*sizeof(struct IP_Table))
			seek=2*sizeof(struct IP_Table);
		else if(seek <= 3*sizeof(struct IP_Table))
			seek=3*sizeof(struct IP_Table);
		else if(seek <= 4*sizeof(struct IP_Table))
			seek=4*sizeof(struct IP_Table);

		var = var->next; 
	}
	return buffer; 
}

/*
 *This function is used by all clients to create their own copy of Server's IP Table.
 *It takes a serialized buffer and Table head as argument.
 *The function returns Head node of Table
 * */
Table reconstructIP_Table(char* buffer,Table head)
{
	Table temp,cur;
	int seek = 0;
	temp = (Table) malloc(sizeof(struct IP_Table));
	cur = head;

	memcpy(&temp->connID,&buffer,sizeof(temp->connID));
	seek += sizeof(temp->connID);
	memcpy(&temp->fd,&buffer[seek],sizeof(temp->fd));
	seek += sizeof(temp->fd);
	memcpy(&temp->connectionName,&buffer[seek],sizeof(temp->connectionName));
	seek += sizeof(temp->connectionName); 
	memcpy(&temp->connectionIP, &buffer[seek],sizeof(temp->connectionIP));
	seek += sizeof(temp->connectionIP); 
	memcpy(&temp->port,&buffer[seek], sizeof(temp->port));
	seek += sizeof(temp->port);

	if(cur==NULL)
	{
		temp->next=NULL;
		temp->connID=1;
		return temp;
	}else
		while(cur->next!=NULL)
		{
			cur = cur->next;
		}
	cur->next = temp;
	temp->connID=(cur->connID)+1;
	temp->next=NULL;
	return head;

}

/*
 * listIPTable(Table) ,This function takes takes head node as parameter and prints the current list maintained by a machine.
 *
 * */
void listIPTable(Table head)
{
	Table itr;
	itr = head;
	printf("\n%-5s%-35s%-20s%-8s\n","ID", "HOSTNAME", "IP Address", "PORT"); 
	if(itr == NULL)
		printf("\nIP Table is empty\n");
	while(itr!=NULL)
	{
		printf("%-5d%-35s%-20s%-8s\n",itr->connID, itr->connectionName, itr->connectionIP, itr->port);
		itr=itr->next; 
	}
	fflush(stdout);	
}

/*
 * This function deletes a node from the link list head passed as 2nd argument. It looks up for the 1st argument passed in the current list and deletes it.
 *
 * */
Table deleteNode(int sockfd,Table first)
{
	Table prev,cur;
	if(first==NULL)
	{
		printf("IP_Server Table is emptry\n");
		return NULL;
	}
	if (first->fd == sockfd)
	{
		cur=first;
		first=first->next;
		free(cur);
		return first;
	} 
	prev=NULL;
	cur=first;
	while(cur!=NULL)
	{
		if(sockfd==cur->fd)break;
		prev=cur;
		cur=cur->next;
	}
	if(cur==NULL)
	{
		printf("This sock desciptor is not saved in IP_Table list");
		return first;
	}
	prev->next=cur->next;
	free(cur);
	return first;
}

/*
 * This functions looks for the arguments passed in the current maintained list. It returns true if it finds a valid node.
 * The 3rd argument Flag is used to add granularity while looking for the node.
 * */
int checkArg(Table first,char* ip,char* port,int flag)
{
	Table cur;
	if(first == NULL)
	{
		printf("IP_Table is empty");
		return -1;
	}
	cur = first;
	while(cur!=NULL)
	{
		if(flag)
		{
			if((strcmp(cur->connectionIP,ip)== 0 || strcmp(cur->connectionName,ip)==0))
				break;
		}
		else
		{
			if((strcmp(cur->connectionIP,ip)== 0 || strcmp(cur->connectionName,ip)==0) && (strcmp(cur->port,port)==0))
				break;
		}
		cur=cur->next;
	}
	if(cur!=NULL)
		return 1;
	return -1;
}

/*
 *This function is called by server to update the port of a particular registered client with its actual listening port when it recieves a message from client.
 *
 */
void populatePort(Table first,char* port,int fd,int len)
{
	Table cur;
	if(first == NULL)
	{
		printf("IP_Table is empty");
		exit(1);
	}
	cur = first;
	while(cur!=NULL)
	{
		if(fd == cur->fd)break;
		cur=cur->next;
	}
	if(cur!=NULL)
		strncpy(cur->port,port,len);
}

/*
 *This function is used by clients to deserialize the serialized buffer they recieve from Server. This buffer contains the updated list of all the registed clients to 
 server.
 *It takes a serialized buffer as 1st argument and returns the Head node of created Table.
 * This function calls reconstructIP_Table function in loop and passes it a buffer which is equivalent to size of 1 Table struct.  
 */
Table deserialize(char *buffer,int length)
{
	int done=0;
	Table head = NULL;
	char *tempBuffer;
	tempBuffer = (char*) malloc (length);

	while(done <length)
	{
		memcpy(tempBuffer,buffer+done,sizeof(struct IP_Table));
		head=reconstructIP_Table(tempBuffer,head);
		done= done+sizeof(struct IP_Table);
	}
	printf("\n******Got Updated IP_Table from Server******");
	listIPTable(head);
	return head;
}

/*
 *This function is used by server to reconstruct the STATS structure when it recieves a serialized buffer containing STATS data from a client.
 * */
STATS reconstructStats(char *buffer, STATS head) 
{
	STATS temp,cur;
	int seek = 0;
	temp = (STATS) malloc(sizeof(struct stats));
	cur = head;


	memcpy(&temp->uploads,&buffer[seek],sizeof(temp->uploads));
	seek += sizeof(temp->uploads);

	memcpy(&temp->hostName,&buffer[seek],sizeof(temp->hostName));
	seek += sizeof(temp->hostName);

	memcpy(&temp->hostName1,&buffer[seek],sizeof(temp->hostName1));
	seek += sizeof(temp->hostName1);

	memcpy(&temp->avgUploadRate,&buffer[seek],sizeof(temp->avgUploadRate));
	seek += sizeof(temp->avgUploadRate); 

	memcpy(&temp->downloads, &buffer[seek],sizeof(temp->downloads));
	seek += sizeof(temp->downloads); 

	memcpy(&temp->avgDownloadRate,&buffer[seek], sizeof(temp->avgDownloadRate));
	seek += sizeof(temp->avgDownloadRate);

	if(cur==NULL)
	{
		temp->next=NULL;
		return temp;
	}else
		while(cur->next!=NULL)
		{
			cur = cur->next;
		}
	cur->next = temp;
	temp->next=NULL;
	return head;

}

/*
 *This function is used by Server to deserialize the serialized buffer it recieves from Clients. This buffer contains the STATS list maintained by a particular client.
 *It takes a serialized buffer as 1st argument and returns the Head node of created STATS linked list. 
 * This function calls reconstructStats function in loop and passes it a buffer which is equivalent to size of 1 STATS in each iteration. 
 */
STATS deserializeStats(char *buffer,int length)
{
	int done=0;
	STATS head = NULL;
	char *tempBuffer;
	int counter = 1;
	tempBuffer = (char*) malloc (length);

	while(done < (length-counter*14))
	{
		memcpy(tempBuffer,buffer+done,sizeof(struct stats));
		head=reconstructStats(tempBuffer,head);
		done= done+84;
		counter +=1;
	}
	return head;
}

/*
 * This code is taken from beej's guide.
 * This function call makes sure that if there is some data which was not transferred in 1 send call will eventually get sent.
 * */
int sendUpdate(int sockD,char * buf,int *len)
{
	int total = 0;
	int bytesleft = *len; 
	int n;
	char str[20];
	sprintf(str, "%d", *len);

	while(total < *len) {
		n = send(sockD, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return n==-1?-1:0; 
}

/*
 *This function just prints the CREATOR command output as required.
 * */
void creator_function()
{
	printf("Karamveer Choudhary,karamvee,karamvee@buffalo,edu\n");
	printf("I have read and understood the course academic integrity policy located at \nhttp://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity");
}

/*
 * This function looks up for "sock" in the passed data structure. Flag variable is used to determine if the "sock" value passed is ConnID or FD
 * */
Table findFd(Table first,int sock,int flag)
{
	Table cur;
	if(first == NULL)
	{	
		printf("\nEmpty IP LIST AT CLIENT");
		return NULL;
	}
	cur = first;
	while(cur!=NULL)
	{
		if(!flag)
		{
			if(sock == cur->connID)
				break;
		}else
		{
			if(sock == cur->fd)
				break;
		}
		cur=cur->next;
	}
	if(cur == NULL)
	{
		printf("\n Descriptor %d not found in CLIENT IP LIST",sock);
		return NULL;
	}
	return cur;
}

/*
 *
 *This functions is invoked by clients to download files from their peers.
 * This function takes a char pointer as argument and notifies the remote nodes and asks them to upload a file to itself.
 *
 * */
int downloadFunction(char *command)
{
	char *path1 =NULL,*path2=NULL,*path3=NULL;
	char *fd1=NULL,*fd2=NULL,*fd3=NULL;
	FILE *fin1,*fin2,*fin3;	
	Table node1=NULL,node2=NULL,node3=NULL;
	char buffer[50];
	int sock;
	int nbytes;
	int ret = 0;

	fd1 = strtok(command," ");	
	fd1 = strtok(NULL," ");
	path1 = strtok(NULL," "); 	
	fd2 = strtok(NULL," "); 	
	path2 = strtok(NULL," "); 	
	fd3 = strtok(NULL," "); 	
	path3 = strtok(NULL," "); 	
	if(path3 !=NULL)
		*(path3+strlen(path3)-1) = '\0'; // Should actually check for space before trimming.

	if(path2 ==NULL)
		*(path1+strlen(path1)-1) = '\0'; // Should actually check for space before trimming.

	if(path3 ==NULL && path2 !=NULL)
		*(path2+strlen(path2)-1) = '\0'; // Should actually check for space before trimming.

	if(path1 != NULL && fd1 != NULL) 
	{
		sock = atoi(fd1);
		sprintf(buffer,"FILETRANSFER%s",path1);
		node1= findFd(clientListHead,sock,0);
		if(node1 == NULL)
			return -1;
		downloadSock1 = node1->fd;
		nbytes = strlen(buffer);
		if (sock != 1){
		downloading1 = 1;
			if(sendUpdate(downloadSock1,buffer,&nbytes) == -1)
				perror("\nsendUpdate Failed in  downloadFunction");

			fflush(stdout);
			ret = 1;
		}
		else
			ret = -2;

	}
	bzero(buffer,50);	
	if(path2 !=NULL && fd2 !=NULL)
	{
		sock = atoi(fd2);
		sprintf(buffer,"FILETRANSFER%s",path2);
		node2= findFd(clientListHead,sock,0);
		if(node2 == NULL)
			return -1;
		downloadSock2 = node2->fd;
		nbytes = strlen(buffer);
		if(sock!=1)
		{
			downloading2 = 1;
			if(sendUpdate(downloadSock2,buffer,&nbytes) == -1)
				perror("\nsendUpdate Failed in  downloadFunction");

			fflush(stdout);
			ret = 1;
		}
		else
			ret = -2;
	}
	bzero(buffer,50);	
	if(path3 != NULL && fd3 != NULL)
	{
		sock = atoi(fd3);
		sprintf(buffer,"FILETRANSFER%s",path3);
		node3= findFd(clientListHead,sock,0);
		if(node3 == NULL)
			return -1;
		downloadSock3 = node3->fd;
		nbytes = strlen(buffer);
		if(sock!=1)
		{
			downloading3 = 1;
			if(sendUpdate(downloadSock3,buffer,&nbytes) == -1)
				perror("\nsendUpdate Failed in  downloadFunction");
			fflush(stdout);
			ret = 1;
		}
		else
			ret = -2;
	}
	return ret;

}

/*
 * 
 * This function updates the maintained STATS structure and sets the upload/download speed of a node identified by the "sock" value passed as 1st argument.
 * If there is no current STATS linked list available then it creates one and after updating the list, it returns the head node.
 * counterType acts as a flag and is used to distinguish between upload/download
 *
 * */
STATS updateCounter(int sock, int counterType,STATS head,double speed)
{
	Table node;
	STATS temp,rev;		
	node = findFd(clientListHead,sock,1);
	if (node ==NULL)
		return NULL;
	if(head ==NULL && node !=NULL)
	{
		temp = (STATS)calloc(1,sizeof(struct stats));
		strcpy(temp->hostName,node->connectionName);
		temp->next = NULL;
		if(counterType)
		{
			temp->uploads =1;
			temp->avgUploadRate = speed;
			temp->avgDownloadRate = 0;
			temp->downloads = 0;
		}
		else
		{
			temp->avgDownloadRate = speed;
			temp->avgUploadRate = 0;
			temp->downloads=1;
			temp->uploads = 0;
		}
		return temp;
	}
	//if there are already elements in STATS linked list then search for the node with same connectionNAme and update its counters.
	temp = head;
	rev = NULL;
	while(temp!=NULL)
	{
		rev = temp;
		if(strcmp(temp->hostName, node->connectionName) == 0) break;
		temp= temp->next;
	}
	if(temp!=NULL) //FOUND THE STATS NODE FOR ALREADY EXISTING HOSTNAME
	{
		if(counterType){
			temp->uploads +=1;
			temp->avgUploadRate = speed;
		}
		else{
			temp->downloads +=1;
			temp->avgDownloadRate = speed;
		}
		return head;

	}
	if (temp ==NULL)
	{

		temp = (STATS)calloc(1,sizeof(struct stats));
		strcpy(temp->hostName,node->connectionName);
		temp->next = NULL;
		rev->next = temp;
		if(counterType){
			temp->uploads =1;
			temp->avgUploadRate += speed;
			temp->avgDownloadRate = 0;
			temp->downloads = 0;
		}
		else{
			temp->downloads =1;
			temp->avgDownloadRate += speed;
			temp->avgUploadRate = 0;
			temp->uploads = 0;
		}
		return head;
	}
}

/*
 *This function is used to terminate a connection. This is called by client when it recieves TERMINATE from user.
 * This function looks for the socket descriptor , closes it and removes the node from current list.
 * */
int terminateFunction(char *command)
{
	char * fd;
	int id;
	Table node;

	fd = strtok(command," ");
	fd = strtok(NULL," ");

	id = atoi(fd);

	if(id == 1)
		return -2;
	node = findFd(clientListHead,id,0);
	if(node == NULL)
		return -3;    

	close(node->fd);
	FD_CLR(node->fd,&clientMaster); 
	clientListHead = deleteNode(node->fd,clientListHead); 

	return 1;
}

/*
 *
 * This function is called by clients to upload file to its peers.
 *
 * */
int uploadFunction(char *command, int flag)
{
	char *path;
	char *fd;
	Table node=NULL;
	FILE *fin;
	int filesize,temp,nbytes;
	struct stat st;
	char buffer[CHUNKSIZE];
	int sock;
	struct timeval tv1,tv2;
	double time=0,speed=0;
	fd = strtok(command," ");
	if(flag == 0)
		fd = strtok(NULL," ");

	path = strtok(NULL," ");
	if(path !=NULL && flag!= 1)
		*(path+strlen(path)-1) = '\0'; // Should actually check for space before trimming.
	sock = atoi(fd);
	fin = fopen(path,"r");
	if(fin ==NULL)
	{
		perror("\nFile Opening failed");
		return -1;
	}		

	if(fstat(fileno(fin),&st))
	{
		fclose(fin);	
		return -1;
	}
	filesize = st.st_size;

	if(flag == 0)
	{
		if(sock == 1)
			return -2;
		node = findFd(clientListHead,sock,0);
		if(node == NULL)
			return -1;
	}

	//send info about file
	sprintf(buffer,"&&&&&&&&%d&&&&&&&&%s",filesize,path);
	nbytes = sizeof(buffer);

	if(flag)
	{
		if(sendUpdate(sock,buffer,&nbytes) == -1)
			perror("\nsendUpdate Failed in  uploadFunction");
	}
	else
	{
		if(sendUpdate(node->fd,buffer,&nbytes) == -1)
			perror("\nsendUpdate Failed in  uploadFunction");
	}

	//Calcuate Time
	gettimeofday(&tv1,NULL);

	bzero(buffer,CHUNKSIZE);	
	while((nbytes = fread(buffer,1,CHUNKSIZE,fin)) > 0)
	{
		if(nbytes == 0)
		{
			printf("\n ZERO BYTES READ");
			fflush(stdout);
		}
		else{
			if(flag)
			{
				if(sendUpdate(sock,buffer,&nbytes) == -1)
				{
					perror("\nsendUpdate Failed in  uploadFunction");
				}
			}
			else
			{
				if(sendUpdate(node->fd,buffer,&nbytes) == -1)
				{
					perror("\nsendUpdate Failed in  uploadFunction");
				}	
			}

		}
		bzero(buffer,CHUNKSIZE);	
	}

	fclose(fin);

	gettimeofday(&tv2,NULL);

	time = (tv2.tv_sec - tv1.tv_sec); 
	time += (tv2.tv_usec - tv1.tv_usec) / 1000000.0;
	if(time!=0)
		speed = (filesize*8/time);

	if(!flag)
		clientStat= updateCounter(node->fd,1,clientStat,speed); 
	else
		clientStat= updateCounter(sock,1,clientStat,speed);


	printf("\nFile size:%d Bytes,Time Taken:%5.5f seconds, Tx Rate:%f bits/second\n", filesize,time,speed);
	printf("\nFile %s sucessfully uploaded",path);	
}

/*
 *This function is called by Server when it recieves STATISTICS command from the user. 
 *This function sends an update to each connected client and requests them to send their currently maintained STATS
 * */
int getStats(Table head)
{
	//Iterate thru the server IP list and ping each host to get its stat table
	Table cur;
	cur = head;
	char buf[20] = "SENDSTATS";
	int len;
	len = strlen(buf);

	if (head == NULL)
		return 0;
	while(cur!=NULL)
	{
		if(sendUpdate(cur->fd,buf,&len) == -1)
			printf("\n sendUpdate Failed in getStats()");
		cur = cur->next;
		noOfClients+=1;
	}
	return noOfClients;
}

/*
 *This function is called by clients when they recieve STATISTICS command from user. It iterates thru client's STATS list and prints each value on screen.
 */
int printStats( STATS head)
{
	STATS itr;
	itr = head;

	double avgUpload = 0, avgDownload = 0;
	printf("\n%-30s %-20s %-30s %-20s %-30s\n","Hostname", "Total Uploads", "Average upload speed(bps)", "Total Downloads", "Average download speed (bps)"); 
	if(itr == NULL)
		printf("\nStats Table is empty\n");
	while(itr!=NULL)
	{
		if(itr->uploads != 0)
			avgUpload = (itr->avgUploadRate/itr->uploads);
		else
			avgUpload = 0;

		if(itr->downloads != 0)
			avgDownload = (itr->avgDownloadRate/itr->downloads);
		else avgDownload = 0;
		printf("%-30s %-20d %-30f %-20d %-30f\n",itr->hostName, itr->uploads, avgUpload, itr->downloads, avgDownload);
		itr=itr->next; 
	} 
}

/*
 *This function iterates thru the server's STATS linked list and prints it on screen.
 * */
int printServerStats(STATS head)
{
	STATS itr;
	itr = head;

	double avgUpload = 0, avgDownload = 0;
	printf("\n%-30s %-30s %-20s %-30s %-20s %-30s\n","HostName1", "Hostname2", "Total Uploads", "Average upload speed(bps)", "Total Downloads", "Average download speed (bps)"); 
	if(itr == NULL)
		printf("\nStats Table is empty\n");
	while(itr!=NULL)
	{
		if(itr->uploads != 0)
			avgUpload = (itr->avgUploadRate/itr->uploads);
		else
			avgUpload = 0.0;
		if(itr->downloads != 0)
			avgDownload = (itr->avgDownloadRate/itr->downloads);
		else
			avgDownload = 0.0;
		printf("%-30s %-30s %-20d %-30f %-20d %-30f\n",itr->hostName1, itr->hostName, itr->uploads, avgUpload, itr->downloads, avgDownload);
		itr=itr->next; 
	} 
}

/*
 * This functions is used by server to merge the currently recieved STATS from a client to and already exiting STATS list it has created with the data recieved from
 * other clients.
 * This functions takes head node of STATS list maintained by Server as 1st parameter and the head node of STATS linked list it has just created by deserialzing the
 * recieved buffer and merges them.
 *
 */
STATS createServerStat(STATS head,STATS current, int sock)
{
	STATS temp,prev,last;
	Table node;
	node = findFd(serverList,sock,1);
	if(node == NULL)
		return NULL;
	if(current == NULL)
		return head;
	temp = current;
	while(temp!=NULL)
	{	
		last = temp;
		strcpy(temp->hostName1,node->connectionName);
		temp = temp->next;
	}

	if(head == NULL)
	{
		return current;
	}
	else
	{
		temp = head;
		prev = NULL;
		while(temp!=NULL)
		{
			prev = temp;
			temp = temp->next;
		}
		prev->next = current;
		last->next = NULL;
		return head;
	}

}

/*
 * The code snippet in this function to "iterate thru addrinfo and creating a socket" is taken from beej's guide.
 *This function just creates a new socket and returns it.
 * */
int sockFunction(char *command,Table first)
{
	int numbytes,yes=1;
	char *ip;
	char *port;
	struct addrinfo hints, *servinfo, *p;
	Table head=NULL;
	struct sockaddr_storage remoteaddr;
	int rv;
	char localHostName[50];
	char s[INET6_ADDRSTRLEN];
	int fdmax,i;
	int sockfd;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ip=strtok(command," ");
	ip=strtok(NULL," ");


	gethostname(localHostName,sizeof(localHostName));

	port=strtok(NULL," "); //gives you extra space in the end.Need to trim it. 
	
	if(port!=NULL)
		*(port+strlen(port)-1) = '\0'; // Should actually check for space before trimming.
	
	if(ip == NULL || port == NULL || *ip == '\0' || *port == '\0'){
		printf("REGISTER/CONNECT Command should provide IP and PORT delimited with a space");
		return -1;
	}

	//check for self connection
	if(((strcmp(ip,clientIp) == 0) || (strcmp(localHostName,ip) == 0 )) && (clientPort == atoi(port)))
		return -2;	


	// Various combinations of self and duplicate connection for REGISTER AND CONNECT commands
	if(strstr(command,"CONNECT") !=NULL)
	{
		//check ip/hostname/port entry in Table
		if(checkArg(first,ip,port,0) !=1)
		{
			printf("\nIP %s and Port %s not present in Server_IP Table",ip,port);
			return -1;	
		}
	}
	//check Duplicate connections for CONNECT

	if(clientListHead !=NULL && checkArg(clientListHead,ip,port,1) == 1)
		return -3;	

	if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
		printf("Error in getaddrinfo: %s\n",gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd= socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

	if(strstr(command,"REGISTER") !=NULL)
	{
		clientListHead=saveAndCreate(p->ai_addr,clientListHead,sockfd);
	}
	if(strstr(command,"CONNECT") !=NULL)
	{
		clientListHead=saveAndCreate(p->ai_addr,clientListHead,sockfd);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	freeaddrinfo(servinfo); // all done with this structure
	return sockfd;
}

/*
 * This functions prints various command syntax the when user enters HELP
 */
void help_function()
{
	printf("Command: MYIP \n \t Display the IP address of this process");
	printf("\nCommand: MYPORT \n \t Display the port on which this process is listening for incoming connections");
	printf("\nCommand: REGISTER <server IP> <port_no> \n \t Registers this client with server");
	printf("\nCommand: CONNECT <destination> <port no> \n \t This command establishes a new TCP Connection to the specified destination");
	printf("\nCommand: LIST \n \t Display a numbered list of all the connections this process is part of.");
	printf("\nCommand: TERMINATE <connection id> \n \t This command will terminate the connection specified by connection id.");
	printf("\nCommand: EXIT \n \t Close all connections and terminate the process.");
	printf("\nCommand: UPLOAD <connection id> <file name> \n\t Uploads a file to a host specified by connection id.");
	printf("\nCommand: DOWNLOAD <connection id 1> <file1> <connection id 2> <file2> <connection id 3> <file3>\n\t This command downloads a file from each host specified in the command.");
	printf("\nCommand: STATISTICS \n\t Displays stats information about clients on server");
}

/*
 *This functions is called by both server and client to handle stdin events.
 */
int mainMenu(char * command,Table first)
{
	int size = 100;
	int ret= 0;
	fgets(command, size,stdin);
	if(strstr(command,"CREATOR")!= NULL)
	{                         
		creator_function();
		return -1;
	}
	else if(strstr(command,"HELP")!= NULL) 
	{
		help_function();
		return -1;
	}
	else if (strstr(command,"MYIP")!= NULL)
	{
		if(server)
			printf("IP address:%s",serverIp);
		else
			printf("IP address:%s",clientIp);
		return -1;
	}
	else if (strstr(command,"MYPORT")!= NULL)
	{
		if(server)
			printf("Port number:%d",serverPort);
		else
			printf("Port number:%d",clientPort);
		return -1;
	}
	else if (strstr(command,"REGISTER")!= NULL)
	{
		if(!server){
			ret = sockFunction(command,first);
			return ret;
		}else
			printf("\nCommand not supported");
		return -1;
	}
	else if (strstr(command,"CONNECT")!= NULL)
	{
		if(!server){
			ret=sockFunction(command,first);
			return ret;
		}else
			printf("\nCommand not supported");
		return -1;
	}

	else if (strstr(command,"LIST")!= NULL)
	{
		if(first!=NULL)
		{
			if(server == true)
			{
				listIPTable(first);
			}else
			{
				listIPTable(clientListHead);
			}
		}
	}
	else if (strstr(command,"TERMINATE")!= NULL)
	{
		if(server != true)
		{
			ret = terminateFunction(command);
			if(ret == -1)
				printf("\n Error in terminating connection");
			else if(ret == -2)
				printf("\n Connection to server cannot be terminated from client");
			else if(ret == -3)
				printf("\n No Peer associated with this ID");
			else	
				printf("\n Sucessfully terminated connection");
		}
		else
			printf("\n Command not supported on Server");

	}
	else if (strstr(command,"EXIT")!= NULL)
	{
		if(server == true)
		{
			//check if serverList is null.
			//EXIT only if serverList is NULL
			if(serverList == NULL)
				exit(EXIT_SUCCESS);
			else
				printf("\nCannot execute EXIT.Server is still connected to some clients");
		}
		else 
		{
			exit(EXIT_SUCCESS);
		}
	}
	else if (strstr(command,"UPLOAD")!= NULL)
	{
		if(server != true)
		{
			ret=uploadFunction(command,0);
			if(ret == -1)
				printf("\nUpload Failed");
			else if(ret == -2)
				printf("\nUploading to server is not allowed");
		}else
			printf("\nCommand not supported");
	}
	else if (strstr(command,"DOWNLOAD")!= NULL)
	{
		if(server != true)
		{
			ret=downloadFunction(command);
			if(ret == -1)
				printf("\nDownload Failed");
			else if (ret == -2)
				printf("Downloading file from SERVER is not allowed");
		}else
			printf("\nCommand not supported");
	}

	else if (strstr(command,"STATISTICS")!= NULL)
	{
		if(server != true)
		{
			printStats(clientStat);
		}
		else
		{
			if(getStats(serverList) == 0)
				printf(" None of the clients are connected to Server");
		}

	}
	else
	{	printf("\nInvalid Command");
		printf("\nNote: Commands are case sensitive");
	}
}

/*
 *main function()
 * Code snippets like basic select implementation,socket creation,binding etc in this function are taken from beej's guide.
 */
int main(int argc, char **argv)
{
	/*Start Here*/
	unsigned short port = 0;
	char *command, *hostname;
	command = (char *) malloc (size + 1);
	Table curr;

	if ( argc < 3)
	{
		printf("Usage: ./program (s/c) portNum\n");
		return 0;
	}
	if(strcmp(argv[1],"s")==0)
		server = true; 
	else if(strcmp(argv[1],"c")==0)
		server = false; 

	if(server == false)
	{
		struct addrinfo hints, *servinfo, *p;
		int rv,siz,filelength=0,yes=1,len,fdmax,i,nbytes,newfd,registerfd=0,connectfd=0,done = 0;
		int done1=0,done2=0,done3=0,filelength1=0,filelength2=0,filelength3=0,statlen = 0;
		struct sockaddr_storage their_addr;
		socklen_t sin_size;
		char remoteIP[INET6_ADDRSTRLEN];
		Table  serverListHead =NULL;
		char temp[] = "####";
		char buf[CHUNKSIZE];
		char buffer[CHUNKSIZE],cliPort[15],arg[20],*data;
		struct sockaddr_storage remoteaddr; // Peer address
		socklen_t addrlen;
		FILE *out1 = NULL,*out2=NULL,*out3=NULL;
		struct timeval tv1,tv2,tv3,tv4,tv5,tv6,tv7,tv8;
		double time=0,time1=0,time2=0,time3=0,speed=0,speed1=0,speed2=0,speed3=0;
		fd_set read_fds;
		char *fileSize,*path,*path3,*path1,*path2;
		memset(&hints,0,sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		hostname = getMyIP();
		strcpy(clientIp,hostname);
		if(hostname == NULL)
			return;
		if ((rv = getaddrinfo(hostname, argv[2], &hints, &servinfo)) != 0) {
			printf("Error in getaddrinfo: %d\n",rv);
			return 1;
		}

		//Basic socket creation,binding and listening code snippet taken from beej's guide.
		for(p = servinfo; p != NULL; p = p->ai_next) {
			clientPort = ntohs(((struct sockaddr_in *)p->ai_addr)->sin_port);
			printf(" Initialized client with IP %s ,port=%d\n", clientIp,clientPort);
			if ((clientsockfd= socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}
			if (setsockopt(clientsockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
						sizeof(int)) == -1) {
				perror("setsockopt");
				exit(1);
			}	
			if (bind(clientsockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(clientsockfd);
				perror("server: bind");
				continue;
			}
			if (p == NULL) {
				fprintf(stderr, "server: failed to bind\n");
				return 2;
			}

			break;
		}
		freeaddrinfo(servinfo); // all done with this structure
		if (listen(clientsockfd, 3) == -1) {   
			printf("listen error");	
			exit(3);
		}

		sprintf(cliPort,"CLIPORT%d",clientPort);
		FD_ZERO(&clientMaster);
		FD_SET(clientsockfd, &clientMaster);
		FD_SET(0, &clientMaster);
		fdmax = clientsockfd;
		for(;;) {
			read_fds = clientMaster; // copy it
			if(!downloading1 && !downloading2 && !downloading3 && !downloading)
				printf("\n>>");
			fflush(stdout);

			if (select(fdmax+1, &read_fds, NULL, NULL, 0) == -1) {
				perror("select");
				exit(4);
			}
			for(i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &read_fds)) { // we got one!!
					if (i == clientsockfd) {
						printf("\n SUCESSFULLY ACCEPTED CONNECTION from a new server on socket %d",i);
						addrlen = sizeof remoteaddr;
						newfd = accept(clientsockfd,
								(struct sockaddr *)&remoteaddr,
								&addrlen);
						if (newfd == -1) {
							perror("accept");
						} else {
							FD_SET(newfd, &clientMaster); // add to master set
							if (newfd > fdmax) {
								fdmax = newfd;
							}
							clientListHead = saveAndCreate((struct sockaddr *)&remoteaddr,clientListHead,newfd);
							if(updatePort(clientListHead,serverListHead,(struct sockaddr *)&remoteaddr) == -1)
								printf("\nError in updating client IP LIST");
						}
					}
					else if (FD_ISSET(0, &read_fds))
					{
						newfd = mainMenu(command,serverListHead);
						if(strstr(command,"REGISTER")!= NULL)
						{
							if(newfd == -1)
								printf("\n");
							else if(newfd == -2)
								printf("\n Self Connection is not allowed");
							else if(newfd == -3)
								printf("\n Duplicate Connections are not allowed");
							else{
								registerfd = newfd;
								FD_SET(registerfd,&clientMaster);
								if(registerfd >fdmax)
									fdmax = registerfd;
							}

						}
						else if(strstr(command,"CONNECT")!= NULL)
						{
							if(newfd == -1)
								printf("\n");
							else if(newfd == -2)
								printf("\n Self Connection is not allowed");
							else if(newfd == -3)
								printf("\n Duplicate Connections are not allowed");
							else{
								connectfd = newfd;
								FD_SET(connectfd,&clientMaster);
								if(connectfd >fdmax)
									fdmax = connectfd;
								printf("\n SUCESSFULLY CONNECTED to a server on socket %d",connectfd);
							}
							fflush(stdout);

						}
					}
					else if(FD_ISSET(registerfd,&read_fds))
					{	
						bzero(buffer,sizeof(buffer)); 	
						siz = recv(registerfd,buffer,sizeof buffer,0);
						if(strstr(buffer,"####") !=NULL)
							send(registerfd,cliPort,sizeof(cliPort),0);
						else if (strstr(buffer,"SENDSTATS")!=NULL)
						{
							data = serializeStats(clientStat,&statlen);
							if(sendUpdate(registerfd,data,&statlen) == -1)
								printf("\n SendUpdate () failed while sending serialized stats to server");	       
						}

						else 
							serverListHead=deserialize(buffer,siz);		
					}
					else if(FD_ISSET(downloadSock1,&read_fds) || FD_ISSET(downloadSock2,&read_fds) || FD_ISSET(downloadSock3,&read_fds))
					{
						bzero(buffer,sizeof(buffer)); 	
						if((siz = recv(i,buffer,sizeof buffer,0)) <= 0)
						{
							if (siz == 0)
							{	
								printf("\nConnection TERMINATED on socked %d\n", i);
								clientListHead=deleteNode(i,clientListHead);
							}
							else
								perror("recv");	
							close(i); // bye!
							FD_CLR(i, &clientMaster); // remove from master set
						}

						if( i == downloadSock1)
						{
							fflush(stdout);
							if(strstr(buffer,"&&&&&&&&")!=NULL)
							{
								fileSize = strtok(buffer,"&&&&&&&&");
								path1 = strtok(NULL,"&&&&&&&&");
								filelength1 = atoi(fileSize);	
								out1 = fopen(path1,"wb");
								if(out1 ==NULL)
								{
									perror("\nFile Opening failed");
								}	
								done1 = 0;
								downloading1 = 1;
								gettimeofday(&tv3,NULL);
							}
							else if(strstr(buffer,"FILETRANSFER")!=NULL)
							{
								printf("\n RECIEVED FILE DOWNLOAD REQ on SOCKET %d for file %s",i,(buffer+strlen("FILETRANSFER")));
								sprintf(arg,"%d %s",i,(buffer+strlen("FILETRANSFER")));
								//uploadFunction(arg,1);	
								if(uploadFunction(arg,1) == -1)
								 {
									bzero(buffer,sizeof buffer);
									strcpy(buffer,"ERRORINOPENINGFILE");
									send(i,buffer,sizeof(buffer),0);
								 }	 
							}	
							else if(strstr(buffer,"ERRORINOPENINGFILE") !=NULL)
							{
								printf("\n Error in accessing file at destination node");
									downloading1 = 0;
							}	
							else{
								if(done1 < filelength1)
								{
									if(siz > (filelength1-done1))
										len = fwrite(buffer,1,(filelength1-done1),out1);
									else
										len = fwrite(buffer,1,siz,out1);
									done1+=len;
								}	
								if ((done1 >= filelength1) && out1 !=NULL )
								{	
									if(out1 != NULL)
										fclose(out1);
									out1=NULL;
									fflush(stdout);
									gettimeofday(&tv4,NULL);	
									time1 = (tv4.tv_sec - tv3.tv_sec); 
									time1 += (tv4.tv_usec - tv3.tv_usec) / 1000000.0;
									if(time1!=0)
										speed1 = (filelength1*8/time1);
									printf("\nSucessfully downloaded file %s",path1); 
									printf("\nFile size:%d Bytes,Time Taken:%5.5f seconds, Rx Rate:%f bits/second\n", filelength1,time1,speed1);
									clientStat= updateCounter(i,0,clientStat,speed1);
									downloading1 = 0;
								}


							}
							fflush(stdout);

						}


						else if(i == downloadSock2)
						{
							fflush(stdout);
							if(strstr(buffer,"&&&&&&&&")!=NULL)
							{
								fileSize = strtok(buffer,"&&&&&&&&");
								path2 = strtok(NULL,"&&&&&&&&");
								filelength2 = atoi(fileSize);	
								fflush(stdout);
								out2 = fopen(path2,"wb");
								if(out2 ==NULL)
								{
									perror("\nFile Opening failed");
								}	
								done2 = 0;
								downloading2 = 1;
								gettimeofday(&tv5,NULL);	
							}
							else if(strstr(buffer,"FILETRANSFER")!=NULL)
							{
								printf("\n RECIEVED FILE DOWNLOAD REQ on SOCKET %d for file %s",i,(buffer+strlen("FILETRANSFER")));
								sprintf(arg,"%d %s",i,(buffer+strlen("FILETRANSFER")));
								//uploadFunction(arg,1);	
								if(uploadFunction(arg,1) == -1)
								 {
									bzero(buffer,sizeof buffer);
									strcpy(buffer,"ERRORINOPENINGFILE");
									send(i,buffer,sizeof(buffer),0);
								 }	 
							}
							else if(strstr(buffer,"ERRORINOPENINGFILE") !=NULL)
							{
								printf("\n Error in accessing file at destination node");
									downloading2 = 0;
							}	
							else{
								if(done2 < filelength2)
								{
									if(siz > (filelength2-done2))
										len = fwrite(buffer,1,(filelength2-done2),out2);
									else
										len = fwrite(buffer,1,siz,out2);
									done2+=len;
								}	
								if (done2 >= filelength2 && out2 != NULL)
								{	if(out2!=NULL)
									fclose(out2);
									out2 = NULL;
									fflush(stdout);
									gettimeofday(&tv6,NULL);	
									time2 = (tv6.tv_sec - tv5.tv_sec); 
									time2 += (tv6.tv_usec - tv5.tv_usec) / 1000000.0;
									if(time2!=0)
										speed2 = (filelength2*8/time2);
									printf("\nSucessfully downloaded file %s",path2); 
									printf("\nFile size:%d Bytes,Time Taken:%5.5f seconds, Rx Rate:%f bits/second\n", filelength2,time2,speed2);
									clientStat= updateCounter(i,0,clientStat,speed2);
									downloading2 = 0;
								}

							}
						}
						else if(i == downloadSock3)
						{
							fflush(stdout);
							if(strstr(buffer,"&&&&&&&&")!=NULL)
							{
								fileSize = strtok(buffer,"&&&&&&&&");
								path3 = strtok(NULL,"&&&&&&&&");
								fflush(stdout);
								filelength3 = atoi(fileSize);	
								out3 = fopen(path3,"wb");
								if(out3 ==NULL)
								{
									perror("\nFile Opening failed ");
								}	
								done3 = 0;
								downloading3 = 1;
								gettimeofday(&tv7,NULL);	
							}
							else if(strstr(buffer,"FILETRANSFER")!=NULL)
							{
								printf("\n RECIEVED FILE DOWNLOAD REQ on SOCKET %d for file %s",i,(buffer+strlen("FILETRANSFER")));
								sprintf(arg,"%d %s",i,(buffer+strlen("FILETRANSFER")));
								if(uploadFunction(arg,1) == -1)
								 {
									bzero(buffer,sizeof buffer);
									strcpy(buffer,"ERRORINOPENINGFILE");
									send(i,buffer,sizeof(buffer),0);
								 }	 
							}
							else if(strstr(buffer,"ERRORINOPENINGFILE") !=NULL)
							{
								printf("\n Error in accessing file at destination node");
									downloading3 = 0;
							}	
							else{
								if(done3 < filelength3)
								{
									if(siz > (filelength3-done3))
										len = fwrite(buffer,1,(filelength3-done3),out3);
									else
										len = fwrite(buffer,1,siz,out3);
									done3+=len;
								}	
								if (done3 >= filelength3 && out3 != NULL)
								{
									if(out3!=NULL)
										fclose(out3);
									out3 = NULL;
									fflush(stdout);
									gettimeofday(&tv8,NULL);	
									time3 = (tv8.tv_sec - tv7.tv_sec); 
									time3 += (tv8.tv_usec - tv7.tv_usec) / 1000000.0;
									if(time3!=0)
										speed3 = (filelength3*8/time3);
									printf("\nSucessfully downloaded file %s",path3); 
									printf("\nFile size:%d Bytes,Time Taken:%5.5f seconds, Rx Rate:%f bits/second\n", filelength3,time3,speed3);
									clientStat= updateCounter(i,0,clientStat,speed3);
									downloading3 = 0;
								}	

							}

						}
					}
					else {
						bzero(buf,CHUNKSIZE);	

						if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
						{

							if (nbytes == 0)
							{	
								printf("\n Peer at socket %d closed connection.\n", i);
								clientListHead=deleteNode(i,clientListHead);
							}
							else
								perror("recv");	
							close(i); // bye!
							FD_CLR(i, &clientMaster); // remove from master set

						}
						else{
							fflush(stdout);
							//RECV the incoming file
							if(strstr(buf,"&&&&&&&&")!=NULL)
							{
								fileSize = strtok(buf,"&&&&&&&&");
								path = strtok(NULL,"&&&&&&&&");
								filelength = atoi(fileSize);	
								out3 = fopen(path,"wb");
								if(out3 ==NULL)
								{
									perror("\nFile Opening failed");
								}	
								done = 0;
								downloading=1;
								gettimeofday(&tv1,NULL);	
							}
							else if(strstr(buf,"FILETRANSFER")!=NULL)
							{
								printf("\n RECIEVED FILE DOWNLOAD REQ on SOCKET %d for file %s",i,(buf+strlen("FILETRANSFER")));
								sprintf(arg,"%d %s",i,(buf+strlen("FILETRANSFER")));
								if(uploadFunction(arg,1) == -1)
								 {
									bzero(buf,sizeof buf);
									strcpy(buf,"ERRORINOPENINGFILE");
									send(i,buf,sizeof(buf),0);
								 }	 
							}
							else if(strstr(buf,"ERRORINOPENINGFILE") !=NULL)
							{
								printf("\n Error in accessing file at destination node");
									downloading=0;
							}	
							else
							{
								if(done < filelength)
								{
									len = fwrite(buf,1,nbytes,out3);
									done+=len;
								}
								if(done >= filelength) 
								{
									gettimeofday(&tv2,NULL);	
									time = (tv2.tv_sec - tv1.tv_sec); 
									time += (tv2.tv_usec - tv1.tv_usec) / 1000000.0;
									if(time!=0)
										speed = (filelength*8/time);
									printf("\nSucessfully downloaded file %s",path); 
									printf("\nFile size:%d Bytes,Time Taken:%5.5f seconds, Rx Rate:%f bits/second\n", filelength,time,speed);
									if(out3 != NULL)
										fclose(out3);
									out3 = NULL;
									clientStat= updateCounter(i,0,clientStat,speed);
									downloading=0;
									fflush(stdout);	
								}
							}
						}
					}

				}
			}
		}
	}
	else if (server == true)
	{
		int new_fd;
		struct addrinfo hints, *servinfo, *p;
		int rv,yes=1;
		struct sockaddr_storage their_addr;
		socklen_t sin_size;
		char remoteName[100],service[20];
		fd_set master;
		fd_set read_fds;
		int fdmax,len;
		int newfd,i,j;
		struct sockaddr_storage remoteaddr; // client address
		char remoteIP[INET6_ADDRSTRLEN];
		socklen_t addrlen;
		int nbytes,listeningPort,counter=0;
		char buf[CHUNKSIZE];
		struct timeval tv;
		char *serialize;
		char temp[] = "####";
		Table head2 = NULL; //TEMP VAR FOR TESTING


		memset(&hints,0,sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		hostname = getMyIP();
		strcpy(serverIp,hostname);
		if(hostname == NULL)
			return;

		if ((rv = getaddrinfo(hostname, argv[2], &hints, &servinfo)) != 0) {
			printf("Error in getaddrinfo: %d\n",rv);
			return 1;
		}


		//Basic socket creation,binding and listening code snippet taken from beej's guide.
		for(p = servinfo; p != NULL; p = p->ai_next) {
			serverPort = ntohs(((struct sockaddr_in *)p->ai_addr)->sin_port);
			printf(" Initialized Server with IP %s ,port=%d\n", serverIp,serverPort);
			if ((serversockfd= socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}
			//fcntl(serverSockfd, F_SETFL, O_NONBLOCK);
			if (setsockopt(serversockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
						sizeof(int)) == -1) {
				perror("setsockopt");
				exit(1);
			}
			if (bind(serversockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(serversockfd);
				perror("server: bind");
				continue;
			}
			if (p == NULL) {
				fprintf(stderr, "server: failed to bind\n");
				return 2;
			}

			break;
		}
		freeaddrinfo(servinfo); // all done with this structure



		// listen
		if (listen(serversockfd, 4) == -1) {
			printf("listen error");
			exit(3);
		}
		FD_ZERO(&master);
		FD_SET(serversockfd, &master);
		FD_SET(0, &master);
		fdmax = serversockfd;
		for(;;) {
			read_fds = master; // copy it
			printf("\n>>");
			fflush(stdout);

			if (select(fdmax+1, &read_fds, NULL, NULL, 0) == -1) {
				perror("select");
				exit(4);
			}
			for(i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &read_fds)) { // we got one!!
					if (i == serversockfd) {
						addrlen = sizeof remoteaddr;
						newfd = accept(serversockfd,
								(struct sockaddr *)&remoteaddr,
								&addrlen);
						if (newfd == -1) {
							perror("accept");
						} else {
							FD_SET(newfd, &master); // add to master set
							if (newfd > fdmax) {
								// keep track of the max
								fdmax = newfd;
							}
							serverList = saveAndCreate((struct sockaddr *)&remoteaddr,serverList,newfd);
							len=sizeof(temp);
							if(sendUpdate(newfd,temp,&len) == -1){
								perror("send");
								printf("\nSent only %d bytes",len);
							}
						}
					} 
					else if (FD_ISSET(0, &read_fds))
					{
						mainMenu(command,serverList);

					}
					else {

						if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
						{
							// got error or connection closed by client
							if (nbytes == 0){ 
								serverList=deleteNode(i,serverList);
								serialize = serializeTable(serverList,&len);
								for(j=1;j<=fdmax;j++)
								{
									if (FD_ISSET(j, &master) && j!=i && j!=serversockfd) {
										if(sendUpdate(j,serialize,&len) == -1){
											perror("send");
											printf("\nSent only %d bytes",len);
										}
									}

								}

							}
							else 
								perror("recv");	
							close(i); // bye!
							FD_CLR(i, &master); // remove from master set
						}
						else
						{      
							if(strstr(buf,"INCOMINGSTATS")!=NULL)
							{
								serverStat = deserializeStats(buf+sizeof("INCOMINGSTATS"),(nbytes-sizeof("INCOMINGSTATS")));
								serverStatHead = createServerStat(serverStatHead,serverStat,i);
								counter+=1;
								if(counter == noOfClients)
								{
									printServerStats(serverStatHead);
									free(serverStatHead);
									serverStatHead = NULL;
									free(serverStat);
									serverStat = NULL;
									counter = 0;
									noOfClients = 0;
								}	
							}
							else if(strstr(buf,"CLIPORT")!=NULL)
							{
								populatePort(serverList,buf+strlen("CLIPORT"),i,(nbytes-strlen("CLIPORT")));	
								serialize = serializeTable(serverList,&len);
								for(j=1;j<=fdmax;j++)
								{
									if (FD_ISSET(j, &master) && j!=serversockfd) {
										if(sendUpdate(j,serialize,&len) == -1){
											perror("send");
										}	
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
