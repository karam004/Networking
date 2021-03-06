/**
 * @karamvee_assignment3
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

#include "../include/global.h"
#include "../include/logger.h"

#define INF 65534 
int totalneighbors=0,totalservers=0,neigh= 0,thisnode=0,globalRecvCounter=0,crash=0;
struct topoNode *topoPointer;
struct topoNode
{
	uint16_t selfno;
	uint16_t otherno;
	char ip[20];
	uint16_t port;
	uint16_t cost;
	int timeoutCount;
	int disabled;
	uint16_t isneighbor; 
};

struct routingNode
{
	uint16_t cost;
	int nextHop;
	uint16_t nodeId;
	char nodeIp[4];
	char ipAddr[20];
	uint16_t nodePort;
	uint16_t isneighbor;
};

struct routingNode routingUpdate[5];

struct fulltable
{
	uint16_t owner;
	struct routingNode table[5];
};

struct fulltable bigTable[5];

/*
 * This funtion is used to sort the routingNode populated from a routing update
 */
void sortRoutingTable(struct routingNode *temp)
{
	//sort the routingTable on the basis of nodeId
	int i,j;
	struct routingNode test;
	for(i=1;i<totalservers;i++)
	{
		for (j=0;j<totalservers-i;j++)
		{
			if(temp[j].nodeId > temp[j+1].nodeId)
				{
					test = temp[j];
					temp[j]=temp[j+1];
					temp[j+1] = test;
				}
		}
	}

}

/*
 * This function reads the topology structure populated after reading topology file and copies information in bigTable structure
 * The entry created by createInitialSetup1 is the actual routing Table for a particular server. 
 */
void createInitialSetup1()
{
//Read topo strucutre and fill Routing Table and Neighbours
int i=0;
char ip[4];
 for(i=0;i<totalservers;i++)
 {
	if(!topoPointer[i].disabled)
	{
		if(topoPointer[i].isneighbor ==1)
	    bigTable[thisnode].table[i].isneighbor = 1;
	 else
	    bigTable[thisnode].table[i].isneighbor = 0;

         inet_pton(AF_INET,&topoPointer[i].ip,ip);
         strcpy(bigTable[thisnode].table[i].nodeIp,ip);
         strcpy(bigTable[thisnode].table[i].ipAddr,topoPointer[i].ip);
	 bigTable[thisnode].table[i].nodePort = topoPointer[i].port;
	 bigTable[thisnode].table[i].cost = topoPointer[i].cost;
	 bigTable[thisnode].table[i].nodeId = topoPointer[i].otherno;
	 if(topoPointer[i].otherno == topoPointer[i].selfno)
		 bigTable[thisnode].table[i].nextHop = topoPointer[i].selfno;  //not set yet
	 else
		 bigTable[thisnode].table[i].nextHop = -1;  //not set yet
	 if(topoPointer[i].isneighbor == 1)
		 bigTable[thisnode].table[i].nextHop = bigTable[thisnode].table[i].nodeId; 
	}
 }
}

/*
 * This function is used to serialize the routing table strucutre into a char array
 * This function adheres to the condition imposed for creating the update packet
 */ 
char * constructMsg()
{
 	int seek=0,i=0;	
       	char *buffer; //initialize a big buffer to accumulate all the packets
	int16_t temp=0;
	int16_t JUNK=0;
	char ip[4];
	char ip1[20];
	buffer = malloc(68);   //8 byte header + 5 * 12 byte Updates for 5 routes at max.
       	memcpy(&buffer[seek],&totalservers,2);
	seek += 2;

	memcpy(&buffer[seek],&topoPointer[thisnode].port,2);
	seek += 2;
         
        inet_pton(AF_INET,&topoPointer[thisnode].ip,ip);
	memcpy(&buffer[seek],ip,4);
	seek += 4;
	inet_ntop(AF_INET,ip,ip1,INET_ADDRSTRLEN);
        	
	//copy all routing table info for the neighbours
	
	while(i<totalservers)
	{
	memcpy(&buffer[seek],&bigTable[thisnode].table[i].nodeIp,4);
	seek +=4;
	
        temp = htons(bigTable[thisnode].table[i].nodePort);
	memcpy(&buffer[seek],&temp,2);
	seek +=2;

	memcpy(&buffer[seek],&JUNK,2);
	seek +=2;
  
        temp = htons(bigTable[thisnode].table[i].nodeId);
	memcpy(&buffer[seek],&temp,2);
	seek +=2;
 
        temp = htons(bigTable[thisnode].table[i].cost);	
	memcpy(&buffer[seek],&temp,2);
	seek +=2;
	i++;
 	}
       return buffer;
}

/*
 * This function is used to send periodic updates to neighbors
 */
int sendPeriodicUpdate(int sockfd)
{
  char *message;
  int  i=0;
  struct sockaddr_in addr;
  message = constructMsg();
  
  for(;i<totalservers;i++)
  {
   if(topoPointer[i].disabled !=1 && topoPointer[i].isneighbor == 1)
   {     //from beej guide , This is the old way. 
	addr.sin_addr.s_addr = inet_addr(bigTable[thisnode].table[i].ipAddr);
	addr.sin_family  = AF_INET;
	addr.sin_port = topoPointer[i].port;
  	//printf("\n IN sendPeriodicUpdate:Sendin Periodic Update to IP %s \n ",bigTable[thisnode].table[i].ipAddr);
	if(sendto(sockfd,(void*)message,68,0,(struct sockaddr*)&addr,sizeof(addr)) == -1)
	{
		perror("sendTo error");
		return -1;
	}
   }
  }
 return 1;
}

/* 
 * This function is used at each timeout to check if a particular server has timed out
 */ 
void checkTimeOut()
{
   int i=0,j;
   struct routingNode *temp; 
   for(;i<totalservers;i++)
   {
	if(topoPointer[i].timeoutCount != -1) 
	{
	  if(topoPointer[i].timeoutCount == -2)
		topoPointer[i].timeoutCount = 1;
	  else
		topoPointer[i].timeoutCount +=1;
	  if(topoPointer[i].timeoutCount == 3)
	   {
            printf("\nIN checkTimeOut:TimeoutCount has reached 3 for %s",topoPointer[i].ip);
	    topoPointer[i].disabled = 1;
	    topoPointer[i].cost = INF;
	    for(j=0;j<totalservers;j++)
	    {
		//mark all servers's cost to INF where this server was nextHop
		if(bigTable[thisnode].table[j].nextHop == topoPointer[i].otherno)
		{
			printf("\n updating cost to INF for router %d since its nexthop %d has been disabled",bigTable[thisnode].table[j].nodeId,topoPointer[i].otherno);
			bigTable[thisnode].table[j].cost = INF;
			bigTable[thisnode].table[j].nextHop = -1;
		}
	    }
	    bellmanFord();
	    topoPointer[i].timeoutCount = -1;    //marked for no monitoring.	     
           } 
	}
   }
}

/*
 * this function is used to find the id of the server using its IPaddress
 */ 
int findId(char *node)
{
	int i=0;
	for(;i<totalservers;i++)
	{
		if(strcmp(topoPointer[i].ip,node) == 0)
			return topoPointer[i].otherno;
	}
}

/*
 * This function is used to return the index of an entry where key is nodeId
 */
int findIndex(int n)
{
	int i=0;
	for(;i<totalservers;i++)
	{
		if(n== topoPointer[i].otherno)
			return i;
	}
}

/*
 * This Function is used to find cost of a link using IP address string as key
 */ 
int findCost(char *node)
{
	int i=0;
	for(;i<totalservers;i++)
	{
		if(strcmp(topoPointer[i].ip,node) == 0)
			return topoPointer[i].cost;
	}
}

/*This function is used to recvUpdates from diffrent nodes
 * It function recieves the packet and deserializes it
 */
struct routingNode* recvUpdate(int sockfd)
{
	struct routingNode *temp;
	char buffer[68],nextnodeStr[17];
	int nexthop,seek=0,i=0,nOfUpdate=0,costTothis=0,noOfUpdate=0;
	char nextnode[4],node[4];
	int16_t test=0;
	struct sockaddr_storage addr;	
	
	int fromlen = sizeof addr;
	int byte_count = recvfrom(sockfd, buffer,sizeof(buffer), 0, &addr, &fromlen);
	//printf("\n Recieved %d bytes",byte_count);
	//recvUpdate and create a new routing table
	//read from the buffer and fll the new routing table
         globalRecvCounter++;	
       	memcpy(&noOfUpdate,&buffer[seek],2);
	seek += 4;

       	memcpy(&nextnode,&buffer[seek],4);
	seek += 4;
	
	inet_ntop(AF_INET,&nextnode,nextnodeStr,INET_ADDRSTRLEN);
        nexthop = findId(nextnodeStr);
        cse4589_print_and_log("RECEIVED A MESSAGE FROM SERVER %d\n", nexthop);
	topoPointer[findIndex(nexthop)].timeoutCount = -2;
	// No need to check previous value of timeoutCount here because if control has come to this function then there can be 2 conditions before this
	// Either timeoutCount was -1 , so we need to mark it for monitoring
	// Else It was 1,2,or -2 , its ok to reset this value to -2 in all those cases.
	//this indicates that we have recived 1st update from this server and will now monitor this server for updates.
	costTothis = findCost(nextnodeStr);
	while(i<noOfUpdate)
	{
		memcpy(&routingUpdate[i].nodeIp,&buffer[seek],4);
		seek +=8;

		inet_ntop(AF_INET,&routingUpdate[i].nodeIp,nextnodeStr,INET_ADDRSTRLEN);
         	strcpy(routingUpdate[i].ipAddr,nextnodeStr);
		routingUpdate[i].nodeId= findId(nextnodeStr);

		memcpy(&test,&buffer[seek],2);
		seek +=2;
		routingUpdate[i].nodeId = ntohs(test);

		memcpy(&test,&buffer[seek],2);
		seek +=2;
		routingUpdate[i].cost=ntohs(test);		

		routingUpdate[i].nextHop = nexthop;

		i++;
	}
	
	sortRoutingTable(routingUpdate);
	for(i=0;i<noOfUpdate;i++)
	{
		cse4589_print_and_log("%-15d%-15d\n",routingUpdate[i].nodeId,routingUpdate[i].cost);
	}
	return routingUpdate;
}

/*This function is used to store routing updates received from all the neighbour into a big strucutre whicch will be used during bellman ford calculations
 */
void saveInFullTable(struct routingNode *node,int flag)
{
	int ownerid,i,temp;
	if(flag)
		ownerid = node->nextHop;
	else
		ownerid = topoPointer[thisnode].selfno;  
	temp = findIndex(ownerid);
	bigTable[temp].owner = ownerid;
        for(i=0;i<totalservers;i++)
	{
		bigTable[temp].table[i].cost = node[i].cost;
		bigTable[temp].table[i].nodeId= node[i].nodeId;
		bigTable[temp].table[i].nextHop= ownerid;
	}	
}

/*
 * This function is used to calculate minimum distance to each node in the topology
 */ 
void bellmanFord()
{
int i,j,k,costToNeigh=0,friend=0,temp=0,min[4];
createInitialSetup1(); 
for(i=0;i<totalservers;i++)
  min[i]=0; 
  for(i=0;i<totalservers;i++)
  {
	
	//pick a router whose cost we need to find
 	if(i != thisnode)   //cost to self is 0
	{
	 min[i] = topoPointer[i].cost;
	 //look at each neighbors table and search for destination i.
	 for(j=0;j<totalservers;j++)
	 {
		 //pick one of the neighbor whose routing update I need to look into
		 friend = bigTable[j].owner;
		 if(topoPointer[friend-1].disabled)
		 {
			 continue;
		 }
		 if(topoPointer[friend-1].isneighbor)
		 {
			
			 costToNeigh = topoPointer[friend-1].cost;
			for(k=0;k<totalservers;k++)
			{
				if(bigTable[j].table[k].nodeId == topoPointer[i].otherno)
				{
					costToNeigh = costToNeigh+bigTable[j].table[k].cost;
					if(costToNeigh < min[i])
					{
						min[i] = costToNeigh;
						bigTable[thisnode].table[k].nextHop = bigTable[j].owner;
						bigTable[thisnode].table[k].cost = min[i];	
					}
					break;
				}		
		        }
	        }
	}	
      }
 
  }

}

/*
 *This functions is called by main function to handle stdin events.
 */
int mainMenu(int sockfd)
{
	int size = 50,i=0;
	int ret= 0;
	char command[50];
	char *temp,*temp1,*temp2,*temp3;
	fgets(command, size,stdin);
	if((strstr(command,"update")!= NULL) || (strstr(command,"UPDATE") !=NULL))
	{
  		temp = strtok(command," ");
		temp1 = strtok(NULL," ");         
		temp2 = strtok(NULL," ");         
		temp3 = strtok(NULL," ");
		if(topoPointer[thisnode].selfno == atoi(temp1))
		{
			for(i=0;i<totalservers;i++)
			{
				if(topoPointer[i].otherno == atoi(temp2) && topoPointer[i].isneighbor == 1)
				{
					if(strstr(temp3,"inf") !=NULL)
					{
						//printf("\n Setting Cost to inf");
						topoPointer[i].cost = INF; 
						bigTable[thisnode].table[i].cost = INF;
						bigTable[thisnode].table[i].nextHop= -1;
					}
					else
					{
						//printf("\n Updating cost between node %s and %s to %s",temp1,temp2,temp3);
						topoPointer[i].cost = atoi(temp3);
						bigTable[thisnode].table[i].cost = atoi(temp3);
						bigTable[thisnode].table[i].nextHop= atoi(temp2);
					}
				}	
			}	
				
	  	if(temp!=NULL)
	  		*(temp+strlen(temp)-1) = '\0';
	  	cse4589_print_and_log("%s:SUCCESS\n",temp);
	       	bellmanFord();	
		}
		else
		{
			cse4589_print_and_log("%s:%s\n",temp,"This ID doesnt not correspond to any neighbor");
		}
	}
	else if((strstr(command,"step")!= NULL) || (strstr(command,"STEP") !=NULL))
	{                        
		temp = strtok(command," ");
		if(sendPeriodicUpdate(sockfd) == -1)
			cse4589_print_and_log("%s:%s\n",temp,"Failed to send Periodic Update");
		else
		{
			if(temp!=NULL)
				*(temp+strlen(temp)-1) = '\0';
			cse4589_print_and_log("%s:SUCCESS\n",temp); 
		}
	}
	else if((strstr(command,"packets")!= NULL) || (strstr(command,"PACKETS") !=NULL))
	{                         
          temp = strtok(command," ");
          cse4589_print_and_log("%d",globalRecvCounter);  
	  if(temp!=NULL)
	 	*(temp+strlen(temp)-1) = '\0';
	  cse4589_print_and_log("%s:SUCCESS\n",temp); 
	  globalRecvCounter = 0;
	}
	else if((strstr(command,"display")!= NULL) || (strstr(command,"DISPLAY") !=NULL))
	{                         
          temp = strtok(command," ");
	  if(temp!=NULL)
	  	*(temp+strlen(temp)-1) = '\0';
	  
	  for(i=0;i<totalservers;i++)
	 {
		cse4589_print_and_log("%-15d%-15d%-15d\n",bigTable[thisnode].table[i].nodeId,bigTable[thisnode].table[i].nextHop,bigTable[thisnode].table[i].cost);
         }         
	  cse4589_print_and_log("%s:SUCCESS\n",temp); 
	}
	else if((strstr(command,"disable")!= NULL) || (strstr(command,"DISABLE") !=NULL))
	{                         
		temp = strtok(command," ");
		temp1 = strtok(NULL," ");         
		if(temp!=NULL && *(temp+strlen(temp)) != '\0')
			*(temp+strlen(temp)-1) = '\0';
		if(topoPointer[findIndex(atoi(temp1))].isneighbor)
		{
			topoPointer[findIndex(atoi(temp1))].disabled = 1;
			topoPointer[findIndex(atoi(temp1))].cost= INF;
			for(i=0;i<totalservers;i++)
			{
				if(bigTable[thisnode].table[i].nextHop == atoi(temp1))// && topoPointer[i].disabled == 0)
				{
					bigTable[thisnode].table[i].cost = INF;		
					bigTable[thisnode].table[i].nextHop= -1;		
				}

			}	 
			bellmanFord();
			cse4589_print_and_log("%s:SUCCESS\n",temp);
		}
		else
		{
			cse4589_print_and_log("%s:%s\n",temp,"This ID doesnt not correspond to any neighbor");
		}

	}
	else if((strstr(command,"crash")!= NULL) || (strstr(command,"CRASH") !=NULL))
	{                         
          temp = strtok(command," ");
	  crash = 1;         
	  if(temp!=NULL)
	  	*(temp+strlen(temp)-1) = '\0';
	 cse4589_print_and_log("%s:SUCCESS\n",temp); 
	}
	else if((strstr(command,"dump")!= NULL) || (strstr(command,"DUMP") !=NULL))
	{                         
          temp = strtok(command," ");
	  temp2 = constructMsg();
          cse4589_dump_packet(temp2,68);
	  if(temp!=NULL)
	  	*(temp+strlen(temp)-1) = '\0';
	  cse4589_print_and_log("%s:SUCCESS\n",temp); 
	}
	else if((strstr(command,"academic")!= NULL) || (strstr(command,"ACADEMIC") !=NULL))
	{                         
          temp = strtok(command," ");
	  temp2 = "(I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity)"; 
	 cse4589_print_and_log("%s\n",temp2);
	  if(temp!=NULL)
	  	*(temp+strlen(temp)-1) = '\0';
	 cse4589_print_and_log("%s:SUCCESS\n",temp); 
	}
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	float timeout;
	FILE *in;
	int i=0,j;
	int count = 0;
	char line[256];
	char *f1,*f2,*f3,*filename;
	fd_set master;
	fd_set read_fds;
	int sockfd,index,fdmax=0,interval=0;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	struct timeval tv;
	struct routingNode *tempNode;
	/*Init. Logger*/
	cse4589_init_log();

	/*Clear LOGFILE and DUMPFILE*/
	fclose(fopen(LOGFILE, "w"));
	fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/

	if ( argc < 5)
	{
		printf("Usage:./server -t <path�to�topology�file> i �i<routing�update�interval>\n");
		return 0;
	}

	if(strcmp(argv[1],"-t") == 0)
	{
		printf("\n Topology file is %s",argv[2]);
		filename = argv[2];
	}
	else if(strcmp(argv[1],"-i") == 0)
		{
		printf("\n Timeout is  %s",argv[2]);
		interval = atoi(argv[2]);
		}
	else
	{
		printf("\n Wrong format");
		exit(0);
	}		

	if(strcmp(argv[3],"-i") == 0)
	{
		printf("\n Timeout is  %s",argv[4]);
		interval = atoi(argv[4]);
		
	}
	else if(strcmp(argv[3],"-t") == 0)
	{
		printf("\n Topology file is %s",argv[4]);
		filename = argv[4];
	}
	else
	{
		printf("\n Wrong format");	
		exit(0);
	}

	tv.tv_sec = interval;//atoi(argv[4]);
	tv.tv_usec = 0;//atoi(argv[4]);
	//Read content from topology file
	in = fopen(filename,"r");
	if( in == NULL)
	{
		printf("\n File pointer is NULL");
		exit(0);
	}
	while(fgets(line,sizeof(line),in) !=NULL)
	{	
		if(count==0)
		{
			totalservers = atoi(line);
			topoPointer = (struct topoNode*)malloc(sizeof(struct topoNode)*totalservers);
		}
		else if(count == 1)
			neigh= atoi(line);
		else
		{
			if(strstr(line,".")!=NULL && i != totalservers)  //this means there is IP address in the line
			{
				f1 = strtok(line," ");
				topoPointer[i].otherno = atoi(f1);	
				f2 = strtok(NULL," ");
				strcpy(topoPointer[i].ip,f2);
				f3 = strtok(NULL," ");
				topoPointer[i].port = atoi(f3);
				topoPointer[i].cost= INF;
				topoPointer[i].isneighbor= 0;
				topoPointer[i].timeoutCount= -1;
	 			topoPointer[i].disabled = 0;	  
				i++;
			}
			else
			{
				f1 = strtok(line," "); 
				index= atoi(f1); 
				thisnode = findIndex(index);
				f2 = strtok(NULL," ");
				f3 = strtok(NULL," ");
				int temp = atoi(f2);
				int temp1 = atoi(f2);
				for(j=0;j<i;j++)
				{
					topoPointer[j].selfno =  index;
					if(topoPointer[j].otherno == temp) // found an existing entry for this server, ie this is one of the neighbour
					{
						topoPointer[j].cost= atoi(f3);
						topoPointer[j].isneighbor=1;
						totalneighbors++;
					}
					if(topoPointer[j].otherno == topoPointer[j].selfno)
						topoPointer[j].cost = 0;     // distance to itself should be 0
				}
			}

		}
		count++;

	}
	fclose(in);
	//Done with reading topology file
	createInitialSetup1();
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	char *c = (char*)&topoPointer[thisnode].port;
	char port[5];
	
	memcpy(port,&topoPointer[thisnode].port,4);
	sprintf(port,"%u",topoPointer[thisnode].port);
	int yes =1;	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd == -1)
		printf("Error");       
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}	
	
	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof serv_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = topoPointer[thisnode].port;
	serv_addr.sin_addr.s_addr = inet_addr(topoPointer[thisnode].ip);
	int a;
	a = bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	if(a == -1)
	{
		perror("listener: bind");
		exit(0);
	}

	FD_ZERO(&master);
	FD_SET(sockfd, &master);
	FD_SET(0, &master);
	fdmax = sockfd;
	int ret;	
	for(;;) 
	{
		read_fds = master; // copy it
		printf("\n>>");
		fflush(stdout);

		if (ret=select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select");
			exit(4);
		}
		if (FD_ISSET(sockfd, &read_fds) && crash !=1)
		{
			tempNode = recvUpdate(sockfd);
			saveInFullTable(tempNode,1);
			bellmanFord();
		}
		else if(FD_ISSET(0,&read_fds))
		{
			mainMenu(sockfd);	
		}
		else
		{
		//	printf("\n This should Happen after every %d secs",interval); 
			if(crash !=1)
			{	checkTimeOut();
				if(sendPeriodicUpdate(sockfd) == -1)
					exit(1);
			}
			if(crash)
				FD_CLR(sockfd,&master);
			tv.tv_sec = interval;
			tv.tv_usec = 0;
		}

	}
	return 0;
}
