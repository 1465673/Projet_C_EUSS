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
double min = 99.0;
double max = -9.0;

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

    double val;          //the temperature
    struct _e* prec;     //the pointer to the precedent element
    struct _e* next;     //the pointer to the next element

} chained_list;

/* create the list with just the root */
chained_list* creeListe () {

    chained_list* root = malloc ( sizeof *root );
    if ( root != NULL )  //verification if the list is well initialized
    {
        //the list is empty, the root point to itself
        root->prec = root;
        root->next = root;
    }
    return root;
}

/* parse the list and print the val */
void parsing (chained_list* root) {

	chained_list* it;
	for ( it = root->next; it != root; it = it->next ) {
	    printf("%f, ", it->val);
	}
}

/* add an element before an other one */
void addBefore (chained_list* element, double val) {
    chained_list* newElem = malloc ( sizeof *newElem );
    if ( newElem != NULL ) {
        newElem->val = val;
        //the new element is assigned is prec and next elements
        newElem->prec = element->prec;
        newElem->next = element;
        //the prec elem point to the new and the same for the next
        element->prec->next = newElem;
        element->prec = newElem;
    }
}

/* add an element after an other one */
void addAfter (chained_list* element, double val) {
    chained_list* newElem = malloc ( sizeof *newElem );
    if ( newElem != NULL ) {
        newElem->val = val;
        //the new element is assigned is prec and next elements
        newElem->prec = element;
        newElem->next = element->next;
        //the prec elem point to the new and the same for the next
        element->next->prec = newElem;
        element->next = newElem;
    }
}

/* add an element at the first place */
void addFirst (chained_list* root, double val) {
    addAfter (root, val);
}

/* add an element at the last place */
void addLast (chained_list* root, double val) {
    addBefore (root, val);
}

// permit to receipt the data from the arduino
// return the buffer with the message
char* fonction (int fd, char ret[]){
	int i = 0;
	int trigger = 0;
	int end = 0;
	int res = 0;
	int bytes = 0;

	memset(ret,'\0',sizeof(ret));
	//read the message byte by byte
	while(end == 0){
		ioctl(fd, FIONREAD, &bytes);

		if(bytes >= 0 && end == 0){
			res = res + read(fd,ret+i,1);

			if(ret[i]=='A'|| trigger ==1){
				i++;
				trigger = 1;
				res++;
						
				if(ret[i-1] == 'Z'){
					i++;
					ret[i] = '\0';
					end = 1;
				}		
			}
		}
				
	}

	//print the message
	for (i = 0; i < res; i++) {
		printf("%c", ret[i]);
	}
	
	
	return ret;
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
	int comptador = 0;

	char subbuf[10];
	chained_list* currentElem; //current elem of the list
	chained_list* root; //start of the list
	

	printf("Enter time : ");
	scanf("%s",timechar);
	
	int tiempo = atol(timechar);
	
	printf("Enter number of measures : ");
	scanf("%s",nbmeasureschar);
	int nbmeasures = atol(nbmeasureschar);
	timetowait = tiempo * nbmeasures;
	printf("%d \n", timetowait);
	
	
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

	

	//read the message and store it in the buffer
	strcpy(buf2, fonction(fd, buf2));
	memset(buf,'\0',sizeof(buf));
	printf("\n");



	//loop to get the temperature
	while(1==1){
			printf("\n");
			sleep(timetowait); //wait the number of second multiplied by the number of measures needed to do the average
      		float media;
      		sprintf(missatge,"ACZ");
			res = write(fd,missatge,strlen(missatge));
			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
			
			//get one example of temperature
			strcpy(buf3, fonction(fd, buf3));
			memcpy(subbuf, &buf3[3], 4); //to take the valor of the code V
      		subbuf[4] = '\0';
      		media = atof(subbuf);
      		printf("\n");
      		printf("Media : %f \n",media ); //print the temperature
      		memset(buf3,'\0',sizeof(buf3));

      		//assign the min and max
      		double valeur = media;
			if (valeur > max) max = valeur;
			if (valeur < min) min = valeur;
			printf("Min : %f\n",min);
			printf("Max : %f\n",max);
			
      		//create the list if none
      		//create the new elements and put them on the right place
      		//change the valor of elements if we already have 3600 elements
			if (valeur >=0.0 && valeur <=30.0){
				if (root == NULL){
					root = creeListe();
					currentElem = root;
				}				
				if (comptador>= 3600){
					if(currentElem == root){
						currentElem = root->next;
					}
					currentElem->val = valeur;
					currentElem = currentElem->next;
				}
				else{
					addLast(root,valeur);
					currentElem = currentElem->next;
				}
					comptador++;
			//print the whole list
			parsing(root);
			}

			printf("\n");
			//the counter of element in the list
			printf("comptador : %d",comptador);
			printf("\n");
			sleep(1);



			sprintf(missatge,"AS131Z");
			//send message to the LED to switch it on
			res = write(fd,missatge,strlen(missatge));
			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
			
			//read the returned code by the LED
			strcpy(buf2, fonction(fd, buf2));
			printf("\n");
			memset(buf2,'\0',sizeof(buf2));



			sprintf(missatge,"AS130Z");
			//turn the LED OFF
			res = write(fd,missatge,strlen(missatge));
			if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
			
			i = 0;
			int trigger = 0;
			int end = 0;
			res = 0;

			//read the returned code by the LED
			while(end == 0){
				ioctl(fd, FIONREAD, &bytes);

				if(bytes >= 0 && end == 0){
					res = res + read(fd,buf2+i,1);

					if(buf2[i]=='A'|| trigger ==1){
						i++;
						trigger = 1;
						res++;
						
						if(buf2[i-1] == 'Z'){
							i++;
							buf2[i] = '\0';
							end = 1;
						}		
					}
				}
				
			}


			for (i = 0; i < res; i++)
			{
				printf("%c", buf2[i]);
			}
			printf("\n");
			memset(buf2,'\0',sizeof(buf2));
			sleep(1);
			
			}
                                                   
	TancarSerie(fd);
	
	return 0;
}
