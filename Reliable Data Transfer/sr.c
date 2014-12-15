#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

struct info {
int starttime;
int ack;
int pktno;
};

struct node
{
 char data[20];
 int ACK;
 int seq;
 int ind;
 float tick;
 struct node *link;
};
typedef struct node * NODE;

float time = 0.000;
void tolayer3(int,struct pkt);
void tolayer5(int,char*);
void starttimer(int,float);
void stoptimer(int);

int base=0;
int rcv_base=0;
int expectedseqnum = 0;
int nextseqnum = 0;
int TIMEDOUTPKT= 0;
float diff = 0;

struct pkt **sndpkt = NULL;
struct info *state=NULL;
struct info *rcv_state=NULL;

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Statistics 
 * Do NOT change the name/declaration of these variables
 * You need to set the value of these variables appropriately within your code.
 * */
int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;

/* Globals 
 * Do NOT change the name/declaration of these variables
 * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 * */
float TIMEOUT = 15.0;
int WINSIZE;         //This is supplied as cmd-line parameter; You will need to read this value but do NOT modify it; 
int SND_BUFSIZE = 0; //Sender's Buffer size
int RCV_BUFSIZE = 0; //Receiver's Buffer size

struct msg sendbuffer[1000];
struct pkt rcvbuffer[1000];
int timerflag=0;

NODE first;
NODE last;
// Function to allocate memory for a new node.
NODE getnode()
{
        NODE x;
        x = (NODE) malloc(sizeof(struct node));
        if (x != NULL)
                return x;
}

//Function to delete the 1st element from the linkedlist
NODE pop(NODE first)
{
        NODE temp;
        if(first == NULL)
                return first;
        temp = first;
        temp = temp->link;
        free(first);
        return temp;
}
//function to insert an element in the rear side of the linked list.
NODE insert(NODE first,int seq,char *data)
{
        NODE temp,cur;
        temp = getnode();
        strcpy(temp->data,data);
        temp->link=NULL;
        temp->seq= seq;
        temp->ind= 0;
        temp->ACK= 0;

        if(first == NULL)
         {
           last=temp;
                return temp;
         }
       cur = first;
        while(cur->link !=NULL)
                cur = cur->link;
        cur->link = temp;
        last = temp;
        return  first;

}
//function to create checksum for a packet
int createCheckSum(packet)
    struct pkt packet;
{
        int i,temp=0;
        temp = packet.seqnum + packet.acknum;
        for(i=0;i<20;i++)
                temp +=packet.payload[i];
        return temp;
}

//function to check if the packet is corrupt or not
int isCorrupt(packet)
 struct pkt packet;
{
        int temp = createCheckSum(packet);
        if(temp == packet.checksum)
                return 0;
        else
                return 1;
}

//function to make a new packet using char * from messaged and nextseqnum
struct pkt* make_pkt(nextseqnum,ack,data)
int nextseqnum;
int ack;
char *data;
{
  int chcksum = 0;
  struct pkt *pk;
  pk = (struct pkt*)malloc(sizeof(struct pkt));
  strcpy(pk->payload,data);
  pk->seqnum = nextseqnum;
  pk->acknum = ack;
  pk->checksum=createCheckSum(*pk);
  return pk;
}



/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
// sendbuffer[A_application] = message;
 struct pkt *pk=NULL;
        int chksum;
        first=insert(first,A_application,message.data);
        A_application++;
printf("\n SIDE A: Got packet from Application,current count %d",A_application);

 if(nextseqnum < base+WINSIZE && last->ind ==0)
 {
	sndpkt[nextseqnum%WINSIZE] = make_pkt(nextseqnum,0,last->data);
        last->tick = time+TIMEOUT;
	last->ind = 1;
	tolayer3(0,*sndpkt[nextseqnum%WINSIZE]);
 	printf("\n Side A: Sending packed %d from A_output",nextseqnum);
	A_transport++;
	if(timerflag == 0)
	{
		starttimer(0,TIMEOUT);
		timerflag = 1;
	}
	nextseqnum++;
 }
}

void B_output(message)  /* need be completed only for extra credit */
  struct msg message;
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int ackno,i=0;
	NODE temp;
	if(isCorrupt(packet) == 0)
	{
		ackno = packet.acknum;
		temp = first;
		while(temp!=NULL)	//Mark the just recieved packet as ACKED
		{
			if (temp->seq == ackno)
			{
				temp->ACK =1;
				break;
			}
			temp = temp->link;
		}

		printf("\n Side A:Recieved ackno=%d and last packet with timer =%d",ackno,TIMEDOUTPKT); //TIMEDOUTPKT variable keeps track of last packet which timedout

		if(timerflag == 1 && ((TIMEDOUTPKT == packet.acknum) || (TIMEDOUTPKT == 9999))) //Check if current timer is associated with this packet before cancelling it
		{
			stoptimer(0);
			timerflag = 0;
			printf("\n Side A: Cancelled timer for %d",TIMEDOUTPKT);
		}
		printf("\n Side A: In A_Input base is = %d and nextseqnum is = %d A_application is = %d ",base,nextseqnum,A_application);

		temp = first;

		while(temp != NULL)  //If there are some ACKED inorder packets then move base pointer forward.
		{ 
			if(temp->ACK==1)
				base =base+1;
			else
				break;
			temp = temp->link;
		}
		printf("\n Side A: Updated base is %d",base);
		temp = first;
		while(temp !=NULL)   // Loop just to print the currently Acked packet. Not needed for actual computation though.
		{
			if(temp->ACK == 1)
				printf("\n Currently Acked packet = %d",temp->seq);
			temp = temp->link;
		}
		temp = first;
		while(temp!=NULL && temp->seq < base)	//Move first pointer ahead to reduce the elements in linked list
		{
			printf("\n SIDE A:deleting acked packets from buffer %d",temp->ACK);
			temp= temp->link;
			first=temp;
		}

		temp = first;
		if(timerflag == 0)
		{
			while(temp != NULL)
			{
				if(temp->ACK !=1)
				{
					starttimer(0,diff);
					timerflag = 1;
					break;
				}
				temp=temp->link;
			}
		}

		temp = first;
		while(temp!=NULL && nextseqnum < base+WINSIZE) //Check if there is space to send more packets and if yes then read from buffer.
		{
			if(temp->ind !=1){
				sndpkt[nextseqnum % WINSIZE] = make_pkt(nextseqnum,0,temp->data);
				printf("\n SIDE A: sending a packet %d from A_input",sndpkt[nextseqnum % WINSIZE]->seqnum);
				tolayer3(0,*sndpkt[nextseqnum % WINSIZE]);
				temp->ind= 1;
				temp->tick = time+TIMEOUT;
				A_transport++;
				nextseqnum++;
			}
			temp=temp->link;
		}

	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("\n Inside A_Interrupt()");
	int i,j;
	NODE temp,min,failed;
	float now = time;
	timerflag =0;
       if(first !=NULL)
{
	temp= first;
	failed = first;
	while(temp!=NULL)
	{
		if(temp->ACK != 1 && temp->tick == now)	// Find the packed whose timer just interrupted
		{
			TIMEDOUTPKT = temp->seq;	
			failed = temp;
			break;
		}
		temp = temp->link;
	}
	
	failed->tick = time+TIMEOUT;		//Update timer for failed packet.
	min = first;
	temp = first;
	while(temp !=NULL)
	{
		if(temp->ACK != 1 && temp->tick < min->tick )
			min = temp;
		temp = temp->link;
	} 							//Find packet with minimum time in current list of unacked packets
	printf("Side A: Found packet %d with min timer %f",min->seq,min->tick);
		
        diff = min->tick - time;			// Store the difference of current time and time for minimum timer packet.
	
	printf("\n Side A: diff here is %f",diff);
	if(diff <= 0)
		diff = TIMEOUT;
       
	if(timerflag == 0)		//send the current timedout packet
	{
		A_transport++;
		timerflag = 1;
		tolayer3(0,*sndpkt[(failed->seq)%WINSIZE]);
		starttimer(0,diff);
	printf("\n Side A: Recent Packet %d whose timer expired",failed->seq);
	}
	
	printf("\n Side A: check for other packets");
	
	temp = first;
	while(temp !=NULL)
	{
		if(temp != failed && temp->ACK !=1 && (temp->tick-now) <= 0)		//check for any other packets which have timeout less than current time.
		{
			
			tolayer3(0,*sndpkt[(temp->seq)%WINSIZE]);
			A_transport++;
			temp->tick=time+TIMEOUT;
		}
		temp = temp->link;
		
	}
}
else 	
	printf("\n LIST IS EMPTY");

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
TIMEDOUTPKT =9999;
sndpkt=(struct pkt**)malloc(sizeof(struct pkt*)*WINSIZE);
state = (struct info *)malloc(sizeof(struct info) *1000);
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	int temp;
	struct pkt *pk;
	int full = 1,newbase=0;
	B_transport++;
	if(isCorrupt(packet) == 0)
	{      
		if(packet.seqnum >= rcv_base && packet.seqnum < rcv_base+WINSIZE)		//Check if uncorrupted packet has come in allowed window at reciever
		{
			printf("\nSide B:  Received packet within allowed WINDOW %d",packet.seqnum);
			printf("\n Side B: Current window is %d to %d",rcv_base,rcv_base+WINSIZE);
			rcvbuffer[packet.seqnum] = packet; 
			rcv_state[packet.seqnum].ack = 1;
			pk = make_pkt(0,packet.seqnum,"CORRECTDATA");
			tolayer3(1,*pk);
			if(packet.seqnum == rcv_base)
			{
				rcv_base++;
				B_application++;
				tolayer5(1,rcvbuffer[packet.seqnum].payload);
				printf("\n Side B: Delivered packet %d to application %s and rcv_base = %d",packet.seqnum,rcvbuffer[packet.seqnum].payload,rcv_base);
				for(;;)
				{
					newbase = rcvbuffer[rcv_base].seqnum;
					if(newbase == rcv_base)
					{
						//this means there are already inorder acked packets; We need to move rcv_base ahead
						B_application++;
						tolayer5(1,rcvbuffer[rcv_base].payload);
						rcv_base += 1;
						printf("\n Side B: Incremented rcv_base to %d in while loop",rcv_base);
					}
					else
					break;
				}
					
			}
		}
		else if(packet.seqnum < rcv_base) 
		{
			printf("\n SIDE B: Value of rcv_base in else part =%d",rcv_base);	
			printf("\n Side B: Recived packet out of boundary %d",packet.seqnum);
			pk = make_pkt(0,packet.seqnum,"OUTOFBOUNDARY");
			tolayer3(1,*pk);
		}
	}
	else
		printf("\n CORRUPTED PACKET %d",packet.seqnum);

}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
rcv_base = 0;
rcv_state = (struct info *)malloc(sizeof(struct info)* 1000);
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

//forward declarations
void init();
void generate_next_arrival();
void insertevent(struct event*);

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float lossprob = 0.0;	   /* probability that a packet is dropped */
float corruptprob = 0.0;   /* probability that one bit is packet is flipped */
float lambda = 0.0; 	   /* arrival rate of messages from layer 5 */
int ntolayer3 = 0; 	   /* number sent into layer 3 */
int nlost = 0; 	  	   /* number lost in media */
int ncorrupt = 0; 	   /* number corrupted by media*/

/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 */
int isNumber(char *input)
{
    while (*input){
        if (!isdigit(*input))
            return 0;
        else
            input += 1;
    }

    return 1;
}

int main(int argc, char **argv)
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c;

   int opt;
   int seed;

   //Check for number of arguments
   if(argc != 5){
   	fprintf(stderr, "Missing arguments\n");
	printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
   	return -1;
   }

   /* 
    * Parse the arguments 
    * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html 
    */
   while((opt = getopt(argc, argv,"s:w:")) != -1){
       switch (opt){
           case 's':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -s\n");
                           return -1;
                       }
                       seed = atoi(optarg);
                       break;

           case 'w':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -w\n");
                           return -1;
                       }
                       WINSIZE = atoi(optarg);
                       break;

           case '?':   break;

           default:    printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
                       return -1;
       }
   }
  
   init(seed);
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
	//Do NOT change any of the following printfs
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
	
	printf("\n");
	printf("Protocol: SR\n");
	printf("[PA2]%d packets sent from the Application Layer of Sender A[/PA2]\n", A_application);
	printf("[PA2]%d packets sent from the Transport Layer of Sender A[/PA2]\n", A_transport);
	printf("[PA2]%d packets received at the Transport layer of Receiver B[/PA2]\n", B_transport);
	printf("[PA2]%d packets received at the Application layer of Receiver B[/PA2]\n", B_application);
	printf("[PA2]Total time: %f time units[/PA2]\n", time);
	printf("[PA2]Throughput: %f packets/time units[/PA2]\n", B_application/time);
	return 0;
}



void init(int seed)                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(seed);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


void insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 //char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
