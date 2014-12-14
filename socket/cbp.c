#include "unp.h"
#include <string.h>
#include <stdio.h>
#include "frame.h"


// This function will convert a string representaion of IP address
// to a station number
int ston(char *sourceIP){
        if(strcmp(sourceIP,s1) == 0)
                return 2;
        if(strcmp(sourceIP,s3) == 0)
                return 3;
}

int main(int argc, char ** argv){
	int  i, maxi, maxfd, listenfd, connfd, sockfd,val,nwritten;
	int  nready, client[NUMBEROFSTATIONS];
	ssize_t  n;
	fd_set   rset, allrset, wset,allwset;
	socklen_t    clilen;
	struct sockaddr_in    cliaddr, servaddr;
	int frame_number;
	int source_station_number;
	int destination_sockfd; //used to assign destination station socket number, 
	char *message1; //used to store first message
	char *message2; //used to store second message
	char source[INET_ADDRSTRLEN]; //used to the  third parameter of function inet_ntop
	char *sourceIP; //used to stored IP address of SP
	
	/* dest is an array that will store the destination station numbers
	and their corresponding socket descriptors. This will later be used to 
	send frame to destination station after receiving both messages of frame */	
	char *destinationIP; 
	char *destination_addr;
	int stored_destination_number;
	struct destination dest[NUMBEROFSTATIONS];
	/* initilize each station's sockfd to -1 */
	for (i = 0; i < NUMBEROFSTATIONS; i++){
		(dest + i)->destination_sockfd = -1;
	}
	
	struct frame * receiving_framePtr;
	struct cbp_storage  cbs;
	char col_mes[COLLISIONMESSAGE] = "Collision";
	char *collision_message = col_mes;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	maxfd = listenfd;                       /* initialize */
	maxi = -1;                                      /* index into client[] array */
	
	/* Set to non-blocking mode using Fcntl for non-blocking IO*/
	
	/* Set socket descriptor that write to CBP output */
	val = Fcntl(fileno(stdout), F_GETFL, 0);
	Fcntl(fileno(stdout), F_SETFL, val | O_NONBLOCK);
	
	val = Fcntl(fileno(stdin), F_GETFL, 0);
	Fcntl(fileno(stdin), F_SETFL, val | O_NONBLOCK);
	for (i = 0; i < NUMBEROFSTATIONS; i++)
			client[i] = -1;                 /* -1 indicates available entry */
	FD_ZERO(&allrset);
	FD_ZERO(&allwset);
	FD_SET(listenfd, &allrset);
	
	maxfd = listenfd;
	
	int ctClient = 0;
	


    for ( ;; ) {
			
	rset = allrset; 
	//wset = allwset;/* structure assignment */
	nready = Select(maxfd+1, &rset, NULL, NULL, NULL);
	if (ctClient == NUMBEROFSTATIONS){			
		if (FD_ISSET(listenfd, &rset)) {        /* new client connection */
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
			
			printf("New connection, sockfd: %d\n",connfd);	
			/* Save the just-connected station, e.i. its socket descriptor and its station number
			This information will later be used to send frame to destination station after
			receiving both message of frame */
			destinationIP = Inet_ntop(AF_INET, &cliaddr.sin_addr,source,INET_ADDRSTRLEN);
			stored_destination_number = ston(destinationIP);
			for (i = 0; i < NUMBEROFSTATIONS; i++){
				if((dest+i)->destination_sockfd < 0){
					(dest+i)->destination_sockfd = connfd;
					(dest+i)->destination_station_number = stored_destination_number;
					break;
				}
			}			
			/* Set socket descriptor to and from SP using Fcntl for non-blocking IO*/	
			val = Fcntl(connfd, F_GETFL, 0);
			Fcntl(connfd, F_SETFL, val | O_NONBLOCK);

			for (i = 0; i < NUMBEROFSTATIONS; i++)
				if (client[i] < 0) {
					client[i] = connfd; 
					ctClient++; 
					break;
				}
			FD_SET(connfd, &allrset);
			FD_SET(connfd, &allwset);

			if (connfd > maxfd)	
					maxfd = connfd;                
			if (i > maxi)
					maxi = i;                           

			if (--nready <= 0)
					continue;                          
	
		}       
		else //Clients full
		{
			break;
		}
	}
	}
	
	/*All Sps are now connect to the CBP, ready for sending frames*/	
	
	/*buffer_acquired is set to:   0 when first part of frame received																       1 when the second part of the frame sent, making the buffer ready for new frame 
					 or when a collision happens, discards the remaining part of the frame, allows stations to acquire the CBP storage
	*/
	char response;	
	printf("All stations are connected, ready to exchage data (Y/N):  \n");
        scanf("%*c %c",&response);
        if( (response == 'y') || (response == 'Y') ){	
	int buffer_acquired = 1;
	for( ; ; ){
	
		
		nready = Select(maxfd+1, &rset, &wset, NULL, NULL);

        for (i = 0; i <= maxi; i++) {   /* check all clients for data */
			sockfd = client[i];
			FD_SET(sockfd, &rset);
			sourceIP = Inet_ntop(AF_INET, &cliaddr.sin_addr,source,INET_ADDRSTRLEN);
			source_station_number = ston(sourceIP);
			
			if(buffer_acquired){	
				if(FD_ISSET(sockfd, &rset)){
					buffer_acquired = 0;
					if(n = Read(sockfd, receiving_framePtr,MAXLINE) == 0){
							Close(sockfd);
							FD_CLR(sockfd,&allrset);
							client[i] = -1;
					}
					else{
						/* Get information from struct frame type variable pointed by receiving_framePtr pointer
						that is received from SP */
						frame_number = receiving_framePtr->frame_number;						
						message1 = receiving_framePtr->message1;
						destination_addr = receiving_framePtr->destination_addr;
						int destination_station_number = ston(destination_addr);
						
						//inspect the message1					
						cbs.source_station_sd = sockfd;
						cbs.source_station_number = source_station_number;
						cbs.destination_station_number = destination_station_number;
						cbs.frame_number = frame_number;			
						strcpy(cbs.message1,message1);

						/*Turn on the bit to write to stdout*/
						FD_SET(fileno(stdout), &wset);
						if(FD_ISSET(fileno(stdout), &wset))
							fprintf(stdout,"Receive %s of frame %d from Station %d,to Station %d\n",
											message1,frame_number,source_station_number,destination_station_number);														
					}
					if (--nready <= 0)
						break;				/* no more readable descriptors */
				}
			}
			else{ 
				/* Buffer already store first message of a frame of a station
				Check if the receiving message is the second frame of that frame or
				the first message of a frame from another station */
				if (FD_ISSET(sockfd, &rset)){
					if(n = Read(sockfd, receiving_framePtr,MAXLINE) == 0){
						Close(sockfd);
						FD_CLR(sockfd,&allrset);
						client[i] = -1;
					}				
					else{/* Inspect the message if it is the second message 
							or the first message from another frame of another station*/
	
						/* Get information from struct frame type variable pointed by receiving_framePtr pointer
						that is received from SP */
						frame_number = receiving_framePtr->frame_number;			//get frame number				
						message2 = receiving_framePtr->message2;						//get the second message
						destination_addr = receiving_framePtr->destination_addr;	//get the destination address (string)
						int destination_station_number = ston(destination_addr);	//convert to number using ston function

						//Inspect the message
						//Check if the current source station number is the one whose first message has already been saved to CBP storage
						//If yes, save the second message
						if(cbs.source_station_number == source_station_number && 
									cbs.frame_number == frame_number){
							if(strcmp(message2,"part 2"==0)){
								strcpy(cbs.message2,message2);
							/*Turn on the bit to write to stdout*/
							FD_SET(fileno(stdout), &wset);
							if(FD_ISSET(fileno(stdout), &wset))
								fprintf(stdout,"Receive %s of frame %d from Station %d,to Station %d\n",
												message1,frame_number,source_station_number,destination_station_number);
								/* Turn on the bit in the wset to write to desination station the first message 
									Get the destination socket number from the dest array
								*/
								
								for( i = 0; i<NUMBEROFSTATIONS; i++){
									if((dest+i)->destination_station_number == cbs.destination_station_number)
										break;
								}
								destination_sockfd = (dest+i)->destination_sockfd;
								FD_SET(destination_sockfd, &wset);
							}
						}
						else{ /* Receiving frame from another station, inform both station a collision*/
						
							 /* Sending collision message to station that accquired the bus, discards the remaing parts of frame */						 
							buffer_acquired = 1;
							FD_SET(cbs.source_station_sd, &wset);		
							if(FD_ISSET(cbs.source_station_sd, &wset))
								Writen(cbs.source_station_sd, collision_message, strlen(COLLISIONMESSAGE));
								
							/*Sending collision message to another station*/
							if(FD_ISSET(sockfd, &wset))
								Writen(sockfd, collision_message, strlen(COLLISIONMESSAGE));
						}
					}
					if (--nready <= 0)
						break;				/* no more readable descriptors */
				}	
			}
			/* Sending the first message of the frame to destinaition SP*/

				
			} 
					

        }

    }




}
