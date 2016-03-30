/* H*************************************************************************************************
*
* File:        UDPServer.c
*
* Author1:     Mani Bhargavi Ketha
* Author2:     Misha Yalavarthy
*
* Description: The program uses the Simple File Transfer Protocol to send ASCII and binary files 
*	          from the client to the server using UDP socket.
*	         The server accepts two arguments which include the server executable code name 
* 	         and <port number>.
*	         The server must first receive the output file name from the client and create that file       
*                    and then write into that file the contents of
*	          the input file which the client sends.The server must write into the output file in  
*                     chunk sizes of 5 bytes. The server must calculate the checksum and check it with the one from client and
*		if correct must send back an ack or do nothing.
*	          The server must be able to identify when the file transfer from the client side is 
*                     completed and it must terminate its connection after 
*	          closing the file.
*
* Date:	      November 18,2015     
*
*
*
*H*/



//header files
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>  // Header defines the socket address stucture
#include<sys/types.h>
#include<netdb.h>// Header defines the hostent structure
#include<string.h>


typedef struct packet
{
    char data[1024]; //contains data to be sent
}Packet;


typedef struct frame
{
    //declare and initiate variables
    int frame_kind; //ACK:0, SEQ:1 FIN:2
    int sq_no;
    unsigned int len; //length variable
    unsigned int checksum; //checksum variable
    int ack; //acknowledge
    Packet packet;
}Frame;


unsigned int checksum(void *buffer, size_t len, unsigned int seed)
{
      //calculating the checksum of data being sent
      unsigned char *buf = (unsigned char *)buffer;
      size_t i;
    
      for (i = 0; i < len; ++i)
            seed += (unsigned int)(*buf++);
      return seed; //value is returned as seed
}




int main(int argc, char **argv)
{
    //initiate variables
	int sd,b,cd,i=0,r;//sd is the socket descriptor returned by the socket function
	int str_len; //string length
    	socklen_t adr_sz;
    	int randerror =0; //random error set to 0
    	Frame ack_frame, recv_frame; //initialize acknowledge frame
	struct timeval tv; //timeout value structure declaration
   	 int frame_id; //frame id
    	int recv_result;
	char fname[50],buff[10],temp,temp1;
	/*fname and buff are buffers which hold the value of the output file name and contents on inputfile respectively*/

	char *ptr; //pointer
	unsigned int csum =0; //checksum initiated
	struct sockaddr_in caddr,saddr; //socket address structure declared

	FILE *fp;//declare file pointer
	socklen_t clen=sizeof(caddr);

    
    
	if(argc != 2)//checking whether two arguments have been entered

		{
		printf("\nUSAGE: $Server<PORT>\n"); //print out server port number for client
		exit(0);
		}
	
   	 //create socket
    	sd=socket(AF_INET, SOCK_DGRAM, 0);
	
    	//if statement to check status of socket. if sd does not equal -1, then it was successful, otherwise -1 is used as an error check
    	if(sd!=-1)
		{
		printf("Socket is created\n");
		}
	else
		{
		printf("Socket is not created\n"); //debugging for socket
		}
	
    
    
        //initializing address structures
    	saddr.sin_family=AF_INET;
	saddr.sin_addr.s_addr=htonl(INADDR_ANY);
	saddr.sin_port=htons(atoi(argv[1]));
	
   	 //bind socket 
   	 b=bind(sd,(struct sockaddr *) &saddr,sizeof(saddr));

   	 /*socket has been binded successfully if b = 0. If not, then -1 is again used as an error check. Binding a socket will assign the local ends 		 port number*/
    if(b==0)
		printf("Bind socket\n");
	else
	{
		printf("Socket not binded\n"); //debugging
		return -1;
	}
	
	
    //file name and size is getting processed. Initiation of file receiving
	if(recvfrom(sd,fname,sizeof(fname),0,(struct sockaddr *) &caddr,(socklen_t *) &clen) != 0)
		printf("File receiving initiated\n");
       
	printf("\nOutput file : %s",fname); //print the output file name
	
    //created file with filename sent by client
    fp = fopen(fname,"w");
    tv.tv_sec = 1; // timeout declared
    tv.tv_usec = 0;
    //setting the sockopt for timeval for the timer
    setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
    
   
    frame_id = 0; 
    while(1)
    {
        
       //receiving data from client
        recv_result = recvfrom(sd, &recv_frame, sizeof(recv_frame), 0,(struct sockaddr *) &caddr,(socklen_t *) &clen);
	
    	
	wait(5);
	
   
   	 if(frame_id > 1)
		frame_id = 0;
       
        
        printf("\ndata: %s\n", recv_frame.packet.data);
        printf("recv result %d\n", recv_result);
        printf("recv frame=%d, frame_kind = %d\n", recv_frame.sq_no, recv_frame.frame_kind);
       
        //producing the effect of error in checksum by giving a random erro setting checksum to 0
        if(randerror%7==0)
        {
            csum = 0; //checksum is set to 0
        }
        else
        {
	       csum  = checksum(recv_frame.packet.data, recv_frame.len, 0); //calculating the checksum for data received
        }
    
        if(recv_result > 0 && recv_frame.frame_kind == 1 && recv_frame.sq_no==frame_id && recv_frame.checksum == csum)
		//checking whether the checksum received and checksum calculated are same
        	{
           	 //print out that the frame has been received by the server
           	 printf("Frame %d received\n", recv_frame.sq_no);
           	 //fprintf(fp, "%s", recv_frame.packet.data);
            	ptr = recv_frame.packet.data;

            	fwrite(ptr,sizeof(char),5,fp); //writing first 5 chunks data to file
            
            	if(recv_frame.packet.data[5] != '\0')
            		{
               		 ptr = &recv_frame.packet.data[5]; //packet will go through frame to write
               	 	 fwrite(ptr,sizeof(char),5,fp); //writing second 5 chunks data
           		 }
            
 
            ack_frame.frame_kind = 1;
            ack_frame.ack = recv_frame.sq_no;
            printf("Ack %d sent\n", ack_frame.ack); //print statement to show ack frame has been sent
	    
            
            sendto(sd, &ack_frame, sizeof(ack_frame), 0,(struct sockaddr *) &caddr,clen);         //ack frame is sent from server to client
	    
        }//if loop closed
	
   	 //when the checksum condition does not hold true the timer expires and no ack is sent and frame id is decremented
    	else if(recv_result < 0 || csum == 0)
        {
            printf("Frame %d time expired\n", frame_id);
	    frame_id--; //
            //break;
        }
        else
        {
             return -1; //returns -1 when frame time has not expired
        }

	frame_id++; //frame id iterated
	randerror++; //random error is iterated
    } //while loop closed

fclose(fp); //file is closed
printf("\nThe file has been recieved\n");
close(sd);//closing the socket connection
return 0;
}
