/*H*******************************************************************************************************
*
* File:        UDPClient.c
*
* Author1:     Mani Bhargavi Ketha
* Author2:     Misha Yalavarthy
*
* Description: The program uses the Simple File Transfer Protocol to send ASCII and binary files  
*	          from the client to the server.
*	          The client accepts five arguments which include the client executable code       
*	          name,<input file name>,<output file name>,<IP address of server><port number>
*	          The client must be able to send the output file name to the server as well as the 	
*	          contents of the input file in chunk sizes of 10 bytes.
*	          The client must provide a way to inform the server of the completion of file transfer
*	          through the socket and must close the socket using UDP sockets.
*		The client must implement the stop and wait protocol and it must send sequence numbers along with the calculated checksum
*		It must wait for acknowledgment before sending the next chunk of data and it implements a timer.
*
* Date:	      November 18,2015     
*
*
*
*H*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>  // Header defines the socket address stucture
#include<sys/types.h>
#include<netdb.h>	 // Header defines the hostent structure
#include<string.h>
typedef struct packet{
    char data[1024];
}Packet;

typedef struct frame{
    int frame_kind; //ACK:0, SEQ:1 FIN:2
    int sq_no;
    unsigned int len; 
    unsigned int checksum; 
    int ack;
    Packet packet;
}Frame;

unsigned int checksum(void *buffer, size_t len, unsigned int seed)//function for calculation checksum
{
      unsigned char *buf = (unsigned char *)buffer;
      size_t i;

      for (i = 0; i < len; ++i)
            seed += (unsigned int)(*buf++);
      return seed;
}

int main(int argc, char **argv)
{
	int sd,c,s,i;//sd is the socket descriptor returned by the socket function

	/*fname & fnamout are the two buffers used to store outputfilename and input file contents*/

	char fname[50],fnameout[50],buff[10],*temp,*temp1;//buff used as buffer for sending data to socket
	int state;
    	int n;
    	int sum;
    	int recv_result;
	int randerror = 0;
    	int frame_id = 0;
struct timeval tv;//used for setting sockopt for setting timer
    	Packet packet;//Creating a packet structure
    	Frame frame;//creating a frame structure to initialize
   	Frame recv_frame;
	struct sockaddr_in caddr;
	int length;
	FILE *fp;//creating/opening file
	socklen_t clen=sizeof(caddr);
	/*must check whether four arguments are entered with first argument being ./STPClient*/	

	if(argc != 5)//checking whether five arguments are inputted
	{
		printf("\nUSAGE: $client<input file name><output file name><ipaddress><PORT>\n");
		exit(0);
	}
	/*socket function, AF_INET is for ipv4 type and SOCK_DGRAM is used for UDP protocol*/

	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if(sd!=-1)//checking whether socket is created or not
	//socket descriptor returns two values 0(socket created) and -1(socket not created)
	{
		printf("Socket is created\n");
	}
	else
	{
		printf("Socket is not created\n");
	}
	//socket has been created

	//Initializing the socket address structure
	
	caddr.sin_family=AF_INET;
	caddr.sin_addr.s_addr=inet_addr(argv[3]);
	caddr.sin_port=htons(atoi(argv[4]));//Converting argument 4 from command line into string format and then into network format
	c=connect(sd,(struct sockaddr *) &caddr,sizeof(caddr));
	if(c==0)
		printf("Connected to server\n");
	else
	{
		printf("Not connected\n");
		return -1;
	}

	temp = argv[1];
	for(i=0;temp[i]!='\0';i++)
	{
	   fname[i] = temp[i];//fname buffer now holds the input file(argument1) name
        }
        fname[i] = '\0';

	temp1 = argv[2];
       //using buffer fnameout to send argument 2 (name of output file)
	for(i=0;temp1[i]!='\0';i++)
	{
	   fnameout[i] = temp1[i];
	//fnameout buffer now holds the output file name(argument2)
        }
        fnameout[i] = '\0';
	printf("\nOutput file : %s",fnameout);
	
        sendto(sd, fnameout, sizeof(fnameout), 0, (struct sockaddr*)&caddr, clen);
         //sending the contents of buffer(input file name) fnameout
	/*
	if(send(sd,fname,sizeof(fname),0)!=0)
		printf("file %s is transferring to server\n",fname);
	*/

	
	//Output file(argument 2) has been sent to server
	//Now input file contents will be sent to the server through fread and sendto functions   
	
	printf("\n\nInput file %s is transferring to server\n",fname);

	if((fp = fopen(fname,"rb"))==NULL)//checking whether file is empty or unable to open the file
	{
           printf("Sorry can't open %s",fname);    
           return -1;   
        }
        
	while(!feof(fp))
	{       
                if(fread(buff,sizeof(char),10,fp))	
			/*Using fread function the contents of the input file are read in chunk size of 10 bytes and loaded into buffer*/
		{
                 strcpy(packet.data, buff);//copy contents of buff into packet.data
        	 memcpy(&(frame.packet), &packet, sizeof(Packet));
        	 frame.frame_kind = 1;
                 if(frame_id > 1) 
			frame_id = 0;
        	 frame.sq_no = frame_id;
	         frame.len = sizeof(frame.packet.data);

	         frame.checksum  = checksum(frame.packet.data, frame.len, 0);//calculating checksum of data
        	 frame.ack = frame_id;
		 tv.tv_sec = 1;//setting timer for 1000 milliseconds
    		 tv.tv_usec = 0;
  		 setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));//setting the sockoption for the timeval struture

		 //sendto(sd, buff,sizeof(buff),0, (struct sockaddr*)&caddr, clen);
          	 	while(1)
        	 	{
				start:
           	  		sendto(sd, &frame, sizeof(frame), 0, (struct sockaddr*)&caddr, clen);//start sending chunks of data to server
		  
            	 		 printf("Frame %d sent\n", frame_id);//frame is sent
				//waiting for the acknowledgment before sending the next chunk of data
            	  		recv_result = recvfrom(sd, &recv_frame, sizeof(recv_frame), 0,(struct sockaddr*)&caddr, (socklen_t *) &clen);

            	 	 	if(recv_result > 0 )//condition checking whether acknowledgment is received
		 		 {
                   		printf("Ack %d received\n", recv_frame.sq_no);
                   		break;//ack received jump outside the while(1) loop and update frame_id
                 	 	}
		  		else
		 	 	{
                   		printf("Frame %d time expired\n", frame_id);
				wait(5);

			        goto start;//if ack fram has not been received and timer expires, resend the frame
                    		}
          		}//closing while(1)
          	
		
		frame_id++;//updating the frame_id
              
		}//closing if

 	}//closing while(!feof(fp)

	if(write(sd,"success",sizeof("success"),0)!=0)
	/*we are sending a success message to the server to indicate file transfer has been completed*/

		printf("\nfile transfer completed from client side\n");
	fclose(fp);	/*closing the input file*/

	close(sd);/*closing the socket connection*/
	return 0;
}
