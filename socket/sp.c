#include "unp.h"
#include "frame.h"
#include <string.h>
#include <stdio.h>


// getRand function return a random number from 0 to max-1
int getRand(int max){
       
        return (int)((float)max * rand( ) /(RAND_MAX+1.0)); 
        
}


//  setDestNo return a string of IP address of a station when knowing its station number
char* setDestNo(int DestNo){
        char dest[20];
        if(DestNo == 1){
                strcpy(dest,s1);
                return dest;
        }
        if(DestNo == 2){
                strcpy(dest,s2);
                return dest;
        }
        if(DestNo == 3){
                strcpy(dest,s3);
                return dest;
        }
}

// Exponential computation of base 2
int Pow(int base, int exponent) {
    int n = 1;
        int i;
    for (i = 0; i < exponent; i++) {
        n *= base;
    }
    return n;
}

int main(){

        int    sockfd;
        struct sockaddr_in      servaddr;

        int                     maxfdp1;

        struct timeval timeout;
        int     timeout_firstpart, timeout_slottime,ctCollision;
        char            sendline[MAXLINE], recvline[MAXLINE];
        ssize_t         n, nwritten;
        fd_set          rset, wset;
	char 	coli[COLLISIONMESSAGE];
        const char* strptr= "147.26.100.200";
	int frame_number; int station_number;
        struct frame sendingframe;
        void *sendingframePtr = &sendingframe;
	int  val;
		
		
        sockfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        Inet_pton(AF_INET, strptr, &servaddr.sin_addr);
		
        Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
		
		/* Set to non-blocking using fcntl 
		*/
		
		/* Set the socket to and from CBP */
        val = Fcntl(sockfd, F_GETFL, 0);
        Fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

		/*Set the stdin*/
        val = Fcntl(fileno(stdin), F_GETFL, 0);
        Fcntl(fileno(stdin), F_SETFL, val | O_NONBLOCK);

		/*Set the sdtout*/
        val = Fcntl(fileno(stdout), F_GETFL, 0);
        Fcntl(fileno(stdout), F_SETFL, val | O_NONBLOCK);


        maxfdp1 = max(max(fileno(stdin), fileno(stdout)), sockfd) + 1;
        for ( ; ; ) {
                FD_ZERO(&rset);
                FD_ZERO(&wset);
				FD_SET(fileno(stdin), &rset);    /* read from stdin */
				FD_SET(sockfd, &rset);                  /* read from socket */
				FD_SET(sockfd, &wset);                  /* data to write to socket */
				FD_SET(fileno(stdout), &wset);   /* data to write to stdout */

                Select(maxfdp1, &rset, &wset, NULL, NULL);
				
				//read first line of the file
				if (FD_ISSET(fileno(stdin), &rset)) {
					while(Fgets(sendline,MAXLINE,stdin)!=NULL){
					
						// parse line from in put, assign to frame struct in frame.h
						sscanf(sendline,"%d%d",&frame_number, &station_number);
						sendingframe.frame_number= frame_number; 				// get frame number
						strcpy(sendingframe.message1,"part 1");					//copy first message from frame	
						char *dest = setDestNo(station_number);  				// convert destination number to destination address(string dot representaiton)
						strcpy(sendingframe.destination_addr,dest);		

						/*read_part1_succesfully intialized to 1: attempt to send first message of the frame
						  Set to 0 if a station successfully sent first message and starts sending second message*/		
						int read_part1_successfully = 1; 
						while(read_part1_successfully){
							/*	Call select function to turn on the write-to-socket bit, no time out.
							 *	The function will check for the writability of the socket to CBP 
							 * 	The second parameter set to NULL, e.i not allowing any reading*/
						
							Select(maxfdp1, NULL, &wset, NULL, NULL);
							if (FD_ISSET(sockfd, &wset)) {
									if ( (nwritten = write(sockfd, sendingframePtr, sizeof(sendingframePtr))) < 0) {
											if (errno != EWOULDBLOCK)
													err_sys("write error to socket");				
									} 
									else{
										/*	Call select function to turn on the read-from-socket bit
										 *	Set timeout to 0.1 ms
										 *	The third parameter set to NULL, not for writing
										 *	Waiting response from the server, either collision or time out
										 * 	If collision -> perform BEBO
										 *	If time-out(first message successfully sent) -> sending second message
										*/
										FD_SET(sockfd, &rset);
										timeout.tv_sec = 0;
										timeout.tv_usec = 100000;										
										int response = Select(maxfdp1, &rset, NULL, NULL, &timeout);
										if (FD_ISSET(sockfd, &rset)){
											if(n = read(sockfd,coli,COLLISIONMESSAGE)>0){
												//when receive message from CBP that there is a collision, inform collision, and perform BEBO
												if(strcmp(coli,"Collision") == 0){
													/* BEBO */
															
													ctCollision++; //increase number of collision by 1
													int numofslot = Pow(2,ctCollision); //divides to 2^ct Collision time lot
													int slot_time = getRand(numofslot);// get a random number from 0 to numofslot
													
													// Write to stdout for collision acknowledgement 
													FD_SET(fileno(stdout),&wset);
													if(FD_ISSET(fileno(stdout),&wset))
														fprintf(stdout,"A collision informed, wait for %d time slot\n",slot_time);
													//wait for numofslot time slots
													timeout.tv_sec = 0;
													timeout.tv_usec = slot_time * 100000;
													timeout_slottime  = Select(maxfdp1, NULL, NULL, NULL,&timeout);		
													if(ctCollision > 16){
														fprintf(stdout,"After 16 collision, station suspends restransmission. Terminated!\n");
														Close(sockfd);
														FD_CLR(sockfd, &rset);
														FD_CLR(sockfd, &wset);
													}
												}
											}
										}
										if(response == 0){
											FD_SET(fileno(stdout),&wset);
											if(FD_ISSET(fileno(stdout),&wset))
												fprintf(stdout,"Send part 1 of frame %d to Station %d\n",frame_number,station_number);
											read_part1_successfully = 0;
										}
									}
							}//end if: turn on write-to-socket bit
						}
						/* 	Start sending part 2 after part 1 of the frame successfully sent
						 *	Call select function to turn on the write-to-socket bit, no time out.
						 *	The function will check for the writability of the socket to CBP 
						 * 	The second parameter set to NULL, e.i not allowing any reading*/
						FD_SET(sockfd, &wset); 
						strcpy(sendingframe.message2,"part 2");	
						Select(maxfdp1, NULL, &wset, NULL, NULL);
						if (FD_ISSET(sockfd, &wset)) {
								if ( (nwritten = write(sockfd, sendingframePtr, sizeof(sendingframePtr))) < 0) {
										if (errno != EWOULDBLOCK)
												err_sys("write error to socket");				
								} 
								else{
									FD_SET(fileno(stdout), &wset);
									if(FD_ISSET(fileno(stdout),&wset))
										fprintf(stdout,"Send part 2 of frame %d to Station %d\n",frame_number,station_number);				
								}	
								
						}
						
						
						
					}//while loop read line
				}//read first line
				
				
		

        }


}

