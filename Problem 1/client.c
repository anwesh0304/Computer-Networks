#include<stdlib.h> //exit(0);
#include<stdio.h> //printf
#include<string.h> //memset
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/wait.h>
#include "dataDef.h"

#define BUFLEN 2  //Max length of buffer
#define SERVER_PORT 8888  // The port on which to send data

void die(char *s)
{
    perror(s);
    exit(1);
}

int setServer (struct sockaddr_in *server_addr, fd_set* readfds)
{
	int sockfd ;

	if ((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1)
        die("socket\n");

    FD_ZERO(readfds); 
    FD_SET (sockfd, readfds) ;
 
    memset((char *) server_addr, 0, sizeof(struct sockaddr_in));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
     
    if (connect (sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in)) < 0)
    	die ("Connection even failed!\n") ;

	return sockfd ;
}

// resendPrep (&readfds, sockfd, &timeout, &sndCount) ;

int resendPrep (fd_set *readfds, int sockfd, struct timeval *timeout, int *sndCount)
{
    (*sndCount)++ ;
	FD_ZERO(readfds); 
	FD_SET (sockfd, readfds) ;
	timeout->tv_sec = TIMEOUT ;
	timeout->tv_usec = 0 ;
}

int main(void)
{
	struct sockaddr_in server_addr;
    int sockfd, i, slen = sizeof(server_addr), sndCount ;
    struct timeval timeout = {TIMEOUT,0} ;
    fd_set readfds ;

    data *datBuf, *ackPkt ;
	data *datCache [10] ;
	ackPkt = (data *) malloc (sizeof(data)) ;
	ackPkt->pktType = DATA ;

	if (fork())
	{
		sockfd = setServer (&server_addr, &readfds) ;

	    for (int i = 0 ; i < 10 ; i += 2)
	    {
	    	datBuf = (data *) malloc (sizeof(data)) ;
	    	datBuf->payload = PACKET_SIZE ;
	    	datBuf->offset = i*PACKET_SIZE ;
	    	datBuf->last = NO ;
	    	datBuf->pktType = DATA ;
	    	datBuf->channel = EVEN ;
	    	strcpy (datBuf->stuff , "abcd") ;
	    	datCache[i] = datBuf ;

	    	sndCount = 1 ;
	        while (1)
	        {
	        	printf ("%d : SENDING %d\n", i, sndCount) ;
	        	send (sockfd, (char *)datBuf, sizeof(data), 0) ;
	        	select (sockfd + 1, &readfds, NULL, NULL, &timeout) ;

	        	if (FD_ISSET (sockfd, &readfds))
	        	{
	        		read (sockfd, (char *)ackPkt, sizeof(data)) ;
	        		printf ("%d : ACK\n", i) ;
	        		break ;	
	        	}
	        	else
	        		;//printf ("%d : REAL_TIMEOUT\n", i) ;

	        	resendPrep (&readfds, sockfd, &timeout, &sndCount) ;

	        }
	    }

	    wait (NULL) ;
	    printf ("Uploading done!\n") ;
	    close(sockfd);

	    return 0 ;
	}
	else
	{
		sockfd = setServer (&server_addr, &readfds) ;

	    for (int i = 1 ; i < 10 ; i += 2)
	    {
	    	datBuf = (data *) malloc (sizeof(data)) ;
	    	datBuf->payload = PACKET_SIZE ;
	    	datBuf->offset = i*PACKET_SIZE ;
	    	datBuf->last = (i == 9)?YES:NO ;
	    	datBuf->pktType = DATA ;
	    	datBuf->channel = ODD ;
	    	datCache[i] = datBuf ;
	    	strcpy (datBuf->stuff , "efgh") ;

	    	sndCount = 1 ;
	        while (1)
	        {
	        	printf ("\t%d : SENDING %d\n", i, sndCount) ;
	        	send (sockfd, (char *)datBuf, sizeof(data), 0) ;
	        	select (sockfd + 1 , &readfds, NULL, NULL, &timeout) ;

	        	if (FD_ISSET (sockfd, &readfds))
	        	{
	        		read (sockfd, (char *)ackPkt, sizeof(data)) ;
	        		printf ("\t%d : ACK\n", i) ;
	        		break ;	
	        	}
	        	else
	        		;//printf ("\t%d : REAL_TIMEOUT\n", i) ;

				resendPrep (&readfds, sockfd, &timeout, &sndCount) ;
	        }
	    }
	    close(sockfd);
	}
}