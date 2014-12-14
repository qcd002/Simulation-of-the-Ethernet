#ifndef FRAME_D
#define FRAME_D

#define NUMBEROFSTATIONS 3  //Number of staion allows to connect with CBP is 3
#define COLLISIONMESSAGE 10 //Size of the collision message that CBP sends to stations
#define MESSAGESIZE 10
typedef int boolean;
#define true 1
#define false 0
        const char* s1 = "147.26.100.202";
        const char* s2 = "147.26.100.203";
        const char* s3 = "147.26.100.201";

		/* cbp_storage is a storage for CBP to store the frame and routing information of 
		the station that accquire first
		CBP can only hold messages of a single frame from a single station */
        struct cbp_storage{
				int source_station_sd;
                int source_station_number;
				int destination_station_number;
				int frame_number;
                char message1[MESSAGESIZE];
		char message2[MESSAGESIZE];

        };

		/* Frame struct that SP used to send message along with destination number to CBP */
        struct frame{
                int frame_number;
                char message1[MESSAGESIZE];
		char message2[MESSAGESIZE];
                char destination_addr[20];
                };
				
		/* desitinatiion struct that CBP used to store the destination station numbers
			and their corresponding socket descriptors */
		struct destination{
			int destination_sockfd;
			int destination_station_number;
		};
	

#endif

