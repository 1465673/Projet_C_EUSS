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
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */ 

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

/* define the structure of an element for the list */
typedef struct _e {

    int val;            /* données quelconques - ici un entier */
    struct _e* prec;    /* pointeur sur l'élément précédent */
    struct _e* next;    /* pointeur sur l'élément suivant */

} chained_list;

/* create the list with just the root */
chained_list* creeListe () {

    chained_list* root = malloc ( sizeof *root );
    if ( root != NULL )  /* si la racine a été correctement allouée */
    {
        /* pour l'instant, la liste est vide, 
           donc 'prec' et 'suiv' pointent vers la racine elle-même */
        root->prec = root;
        root->next = root;
    }
    return root;
}

/* parse the list and print the val */
void parsing (chained_list* root) {

	chained_list* it;
	for ( it = root->next; it != root; it = it->next ) {
	    printf("%d, ", it->val);
	}
}

/* add an element before an other one */
void ajouterAvant (chained_list* element, int val) {
    chained_list* nouvel_element = malloc ( sizeof *nouvel_element );
    if ( nouvel_element != NULL ) {
        nouvel_element->val = val;
        /* on définit les pointeurs du nouvel élément */
        nouvel_element->prec = element->prec;
        nouvel_element->next = element;
        /* on modifie les éléments de la liste */
        element->prec->next = nouvel_element;
        element->prec = nouvel_element;
    }
}

/* add an element after an other one */
void ajouterApres (chained_list* element, int val) {
    chained_list* nouvel_element = malloc ( sizeof *nouvel_element );
    if ( nouvel_element != NULL ) {
        nouvel_element->val = val;
        /* on définit les pointeurs du nouvel élément */
        nouvel_element->prec = element;
        nouvel_element->next = element->next;
        /* on modifie les éléments de la liste */
        element->next->prec = nouvel_element;
        element->next = nouvel_element;
    }
}

/* add an element at the first place */
void ajouterEnTete (chained_list* racine, int val) {
    ajouterApres (racine, val);
}

/* add an element at the last place */
void ajouterEnQueue (chained_list* racine, int val) {
    ajouterAvant (racine, val);
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
	int bytes;
	int comptador;
	int min;
	int max;
	char subbuf[10];
	chained_list* currentElem;

	

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

	printf("Sent %d bytes: ",res);
	for (i = 0; i < res; i++)
	{
		printf("%c",missatge[i]);
	}
	printf("\n");
	sleep(1);

	i = 0;
	int trigger = 0;
	int end = 0;
	res = 0;
		printf("avant while 1");
	ioctl(fd, FIONREAD, &bytes);
	printf("avant while");
	printf("bytes : %d",bytes);
	while(bytes == 0 && end == 0){
		printf("rentré while ");
		res = res + read(fd,buf+i,1);
		//printf("buf %s \n",buf[i]);
		
		/*if(buf[i]=='A'|| trigger ==1){
			printf("rentré if 1");
			i++;
			trigger =1;
			res++;
			if(buf[i]=='Z'){
				printf("rentré if 2");
				i++;
				buf[i]='\0';
				end = 1;
			}		
		}*/
	}

	printf("Received %d bytes: ",res);
	for (i = 0; i <= 4; i++)
	{
		printf("%c",buf[i]);
	}


	/*printf("bytes en attente %d",bytes);
	for (i = 0; i<bytes ;i++){
		res = res + read(fd,buf+i,1);
	}

	
	printf("Rebuts %d bytes: ",res);
	for (i = 0; i < res; i++)
	{
		printf("%c",buf[i]);
	}
	printf("\n");
*/
	
	/*while(1==1){

			sleep(timetowait);

      		double media;
      		sprintf(missatge,"ACZ");
			
      		//fd = ConfigurarSerie();
			res = write(fd,missatge,strlen(missatge));

			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }

			//fd = ConfigurarSerie();	
			//res = read(fd,buf,4);
			//res = res + read(fd,buf+4,4);
			res = read(fd,buf,1);
			res = res + read(fd,buf+1,1);
			res = res + read(fd,buf+2,1);
			res = res + read(fd,buf+3,1);
			res = res + read(fd,buf+4,1);
			res = res + read(fd,buf+5,1);
			res = res + read(fd,buf+6,1);
			res = res + read(fd,buf+7,1);
			

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
			comptador++;
			//fd = ConfigurarSerie();	
			sprintf(missatge,"AS131Z");

			res = write(fd,missatge,strlen(missatge));

			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
	
			
			//res = read(fd,buf2,4);
			
			res = read(fd,buf2,1);
			res = res + read(fd,buf2+1,1);
			res = res + read(fd,buf2+2,1);
			res = res + read(fd,buf2+3,1);

			for (i = 0; i < res; i++)
			{
				printf("on %c\n", buf2[i]);
			}


			
			sprintf(missatge,"AS130Z");

			res = write(fd,missatge,strlen(missatge));
			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
			
			//res = read(fd,buf3,4);
			
			res = read(fd,buf3,1);
			res = res + read(fd,buf3+1,1);
			res = res + read(fd,buf3+2,1);
			res = res + read(fd,buf3+3,1);

			for (i = 0; i < res; i++)
			{
				printf("off %c\n", buf3[i]);
			}


			}


      	}*/

                                                                   
	TancarSerie(fd);
	
	return 0;
}

