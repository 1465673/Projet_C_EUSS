//      linux_serie_demo.c
//
//This document is copyrighted (c) 1997 Peter Baumann, (c) 2001 Gary Frerking
//and is distributed under the terms of the Linux Documentation Project (LDP)
//license, stated below.
//
//Unless otherwise stated, Linux HOWTO documents are copyrighted by their
//respective authors. Linux HOWTO documents may be reproduced and distributed
//in whole or in part, in any medium physical or electronic, as long as this
//copyright notice is retained on all copies. Commercial redistribution is
//allowed and encouraged; however, the author would like to be notified of any
//such distributions.
//
//All translations, derivative works, or aggregate works incorporating any
//Linux HOWTO documents must be covered under this copyright notice. That is,
//you may not produce a derivative work from a HOWTO and impose additional
//restrictions on its distribution. Exceptions to these rules may be granted
//under certain conditions; please contact the Linux HOWTO coordinator at the
//address given below.
//
//In short, we wish to promote dissemination of this information through as
//many channels as possible. However, we do wish to retain copyright on the
//HOWTO documents, and would like to be notified of any plans to redistribute
//the HOWTOs.
//
//http://www.ibiblio.org/pub/Linux/docs/HOWTO/Serial-Programming-HOWTO

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>                                                        
#include <termios.h>       
#include <sys/ioctl.h>   
#include <time.h> 
 
#define BAUDRATE B9600                                                
//#define MODEMDEVICE "/dev/ttyS0"        //Conexió IGEP - Arduino
#define MODEMDEVICE "/dev/ttyACM0"         //Conexió directa PC(Linux) - Arduino                                   
#define _POSIX_SOURCE 1 /* POSIX compliant source */                       
                                                           
struct termios oldtio,newtio;                                            


int	ConfigurarSerie(void)
{
	int fd;                                                           


	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );                             
	if (fd <0) {perror(MODEMDEVICE); exit(-1); }                            

	tcgetattr(fd,&oldtio); /* save current port settings */                 

	bzero(&newtio, sizeof(newtio));                                         
	//newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;             
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;             
	newtio.c_iflag = IGNPAR;                                                
	newtio.c_oflag = 0;                                                     

	/* set input mode (non-canonical, no echo,...) */                       
	newtio.c_lflag = 0;                                                     

	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */         
	newtio.c_cc[VMIN]     = 4;   /* blocking read until 1 chars received */ 

	tcflush(fd, TCIFLUSH);                                                  
	tcsetattr(fd,TCSANOW,&newtio);
	
		
 	sleep(2); //Per donar temps a que l'Arduino es recuperi del RESET
		
	return fd;
}               

void TancarSerie(fd)
{
	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
}
                                                                                 
int main(int argc, char **argv)                                                               
{                                                                          
	int fd, i = 0, res;                                                           
	char buf[255];
	char buf2[255];
	char buf3[255];
	char missatge[255];
	char timechar[2];
	char nbmeasureschar[2];
	int timetowait;
	

	printf("Enter time : ");
	scanf("%s",timechar);
	
	int tiempo = atol(timechar);
	
	printf("Enter number of measures : ");
	scanf("%s",nbmeasureschar);
	int nbmeasures = atol(nbmeasureschar);
	timetowait = tiempo * nbmeasures;
	printf("%d \n", timetowait);
	
	

	// Enviar el missatge 1
	sprintf(missatge,"AM1");
	strcat(missatge, timechar);
	strcat(missatge, nbmeasureschar);
	strcat(missatge,"Z");
	

	fd = ConfigurarSerie();
	
	res = write(fd,missatge,strlen(missatge));

	if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }

	printf("Enviats %d bytes: ",res);
	for (i = 0; i < res; i++)
	{
		printf("%c",missatge[i]);
	}
	printf("\n");

	res = read(fd,buf,4);

	
	printf("Rebuts %d bytes: ",res);
	for (i = 0; i < res; i++)
	{
		printf("%c",buf[i]);
	}
	printf("\n");

	
	while(1==1){
			sleep(timetowait);
      		char subbuf[10];
      		double media;
      		sprintf(missatge,"ACZ");
			
      		fd = ConfigurarSerie();
			res = write(fd,missatge,strlen(missatge));

			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }

			//fd = ConfigurarSerie();	
			res = read(fd,buf,4);
			res = res + read(fd,buf+4,4);
			

			printf("Rebuts %d bytes: ",res);
			for (i = 0; i < res; i++)
			{
				printf("%c", buf[i]);
			}
			printf("\n");
			memcpy(subbuf, &buf[3], 4); //to take the valor of the code V
      		subbuf[4] = '\0';
      		media = atof(subbuf);
      		printf("Media : %1.f \n",media );



			
      		
			if (media >=0 && media <=30)
			{
			fd = ConfigurarSerie();	
			sprintf(missatge,"AS131Z");

			res = write(fd,missatge,strlen(missatge));

			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
	
			
			res = read(fd,buf2,4);
			for (i = 0; i < res; i++)
			{
				printf("on %c\n", buf2[i]);
			}


			
			sprintf(missatge,"AS130Z");

			res = write(fd,missatge,strlen(missatge));
			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
			
			res = read(fd,buf3,4);
			for (i = 0; i < res; i++)
			{
				printf("off %c\n", buf3[i]);
			}


			}


      	}





		





                                                                   
	TancarSerie(fd);
	
	return 0;
}

