#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
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
#include <float.h>

#define SERVER_PORT_NUM		5001
#define SERVER_MAX_CONNECTIONS	4
#define REQUEST_MSG_SIZE	1024 
#define BAUDRATE B9600                                                
//#define MODEMDEVICE "/dev/ttyS0"        //Conexió IGEP - Arduino
#define MODEMDEVICE "/dev/ttyACM0"         //Conexió directa PC(Linux) - Arduino                                   
#define _POSIX_SOURCE 1 /* POSIX compliant source */                       
                 

/* variable generale */                                          
struct termios oldtio,newtio;                                            
double min = 99.0;
double max = -9.0;
int comptador = 0;
float media;
char nbmeasureschar[2];
char timechar[2];
int start = 0;
int res;
int timetowait;
int fd;

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
        //the new element is assigned his prec and next elements
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
        //the new element is assigned his prec and next elements
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

void startAcquisition() {
	char buf2[255];
	char missatge[255];
	//int fd;
	int i;
	int tiempo = atol(timechar);
	int nbmeasures = atol(nbmeasureschar);
	timetowait = tiempo * nbmeasures;
	printf("%d \n", timetowait);
	
	
	sprintf(missatge,"AM1");
	strcat(missatge, timechar);
	strcat(missatge, nbmeasureschar);
	strcat(missatge,"Z");
	

	
	res = write(fd,missatge,strlen(missatge));

	if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }

	printf("Enviats %d bytes: ",res);
	for (i = 0; i < res; i++)
	{
		printf("%c",missatge[i]);
	}
	printf("\n");


	//read the message and store it in the buffer
	/*strcpy(buf2,*/ fonction(fd, buf2);
	//memset(buf,'\0',sizeof(buf));
	printf("\n");

}
                                                                                 
int mainArduino()                                                               
{                                                                          
	int /*fd,*/ i = 0/*, res*/;                                                           
	char buf[255];
	char buf2[255];
	char buf3[255];
	char missatge[255];
	//char timechar[2];
	//char nbmeasureschar[2];
	//int timetowait;
	int bytes;
	//int comptador = 0;

	char subbuf[10];
	chained_list* currentElem; //current elem of the list
	chained_list* root; //start of the list
	

	/*printf("Enter time : ");
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
*/


	//loop to get the temperature
	while(1){
		if(start == 1) {
			printf("\n");
			sleep(timetowait); //wait the number of second multiplied by the number of measures needed to do the average
      		//float media;
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
				if (comptador >= 3600){
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
		else {
			sleep(1);
		}		
	}
                                                   
	//TancarSerie(fd);
	
	return 0;
}





/* !!!!!!!!!!!!!!!!!!!! partie du server pour le client !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */


int mainClient() {
	struct sockaddr_in	serverAddr;
	struct sockaddr_in	clientAddr;
	int			sockAddrSize;
	int			sFd;
	int			newFd;
	int			nRead;
	int 		result;
	char		buffer[256];
	char		missatge[256];
	//int tab[3600];
	/*int counter = 0;
	float maximum = -9999.0;
	float minimum = -9999.0;*/
	char acquisition[10] = "stop";

	/*Preparar l'adreça local*/
	sockAddrSize=sizeof(struct sockaddr_in);
	bzero ((char *)&serverAddr, sockAddrSize); //Posar l'estructura a zero
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT_NUM);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/*Crear un socket*/
	sFd=socket(AF_INET, SOCK_STREAM, 0);
	
	/*Nominalitzar el socket*/
	result = bind(sFd, (struct sockaddr *) &serverAddr, sockAddrSize);
	
	/*Crear una cua per les peticions de connexió*/
	result = listen(sFd, SERVER_MAX_CONNECTIONS);
	
	/*Bucle s'acceptació de connexions*/
	while(1){
		printf("\nServer waiting for connections\n");

		/*Esperar conexió. sFd: socket pare, newFd: socket fill*/
		newFd=accept(sFd, (struct sockaddr *) &clientAddr, &sockAddrSize);
		printf("Connection with the client accepted: address %s, port %d\n",inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

		/*Rebre*/
		result = read(newFd, buffer, 256);
		
			
		printf("Message received from the client (bytes %d): %s\n",	result, buffer);
		
		/************************ Initialisation Array **********************/
	//float tab[3600];
	int i=0;
	//time_t t;
	//srand((unsigned) time(&t));
	//counter = 0;
	/*for(i=0; i<3600; i++){
		tab[i] = (double)rand() / (double)RAND_MAX*30; //((float) rand()) / (float) 30;
		counter++;
	}*/
	/********* HANDLE THE DATAS **************/

	char* mess = malloc(10*sizeof(char));
	char* acqu_state = malloc(10*sizeof(char));
	acqu_state = "stop"; //initialisation
		
	char order = buffer[1];
	//int n1, n2, n3, verif;
	char val[4];
	char subbuf[10];
	char subbuf2[10];
	char subbuf3[10];

	switch(order) {
		case 'U':
			memset(buffer, '\0', sizeof(buffer));
			memset(missatge, '\0', sizeof(missatge));
						
			printf("avg = %f\n", media);
			sprintf(val, "%.2f", media);
			strcpy(mess, "AU0");
			if (media < 10) strcat(val, "0");
			strcat(mess, val);
			strcat(mess, "Z");
			printf("mess = %s\n", mess);	
			break;
		case 'X':

			memset(buffer, '\0', sizeof(buffer));
			memset(missatge, '\0', sizeof(missatge));


			strcpy(mess, "AX0");			
			printf("maxi = %f\n", max);
			sprintf(val, "%.2f", max);
			if (max < 10) strcat(val, "0");
			strcat(mess, val);
			strcat(mess, "Z");
			printf("mess = %s\n", mess);
			break;
		case 'Y':

			memset(buffer, '\0', sizeof(buffer));
			memset(missatge, '\0', sizeof(missatge));

			strcpy(mess, "AY0");			
			printf("mini = %f\n", min);
			sprintf(val, "%.2f", min);
			if (min < 10) strcat(val, "0");
			strcat(mess, val);
			strcat(mess, "Z");
			printf("mess = %s\n", mess);
			break;
		case 'R':
			/*reset maxi et mini*/
			max = -9999.0;
			min = 9999.0;
			strcpy(mess, "AR0Z");
			break;
		case 'B':

			/*counter */
			memset(buffer, '\0', sizeof(buffer));
			memset(missatge, '\0', sizeof(missatge));

			printf("counter = %d\n", comptador);	
			sprintf(val, "%d", comptador);
			
			strcpy(mess, "AB0");
			strcat(mess, val);
			strcat(mess, "Z");	
			printf("mess = %s\n", mess);
			break;
		case 'M':
			/*start acqui*/
			
			//to get the differents datas from the message
			
			memcpy(subbuf, &buffer[2], 1);
			subbuf[2] = '\0';
			
			
			memcpy(timechar, &buffer[3], 2);
			timechar[2] = '\0';
			
			
			memcpy(nbmeasureschar, &buffer[5],2);
			nbmeasureschar[1] = '\0';
			
			
			printf("acquisition = %s\n", acquisition);

			//to start the acquisition of the temperature
			if (strcmp(subbuf,"1") == 0) {
				printf("\nAcquisition started witch %s seconds of interval.\n",timechar);
				printf("We calculate median with %s data\n\n. ",nbmeasureschar);
				strcpy(mess,"AM0Z");
				strcpy(acquisition,"start");
				printf("mess = %s\n", mess);
				startAcquisition();
				start = 1;
			}
			//stop the acquisition of the temperature
			else {
				strcpy(mess,"AM0Z");
				strcpy(acquisition,"stop");
				printf("mess = %s\n", mess);
				start = 0;
			}
			printf("acquisition = %s\n", acquisition);
			break;
		default:
			printf("invalid order\n");
			strcpy(mess,"erreuuuur");
	}
							


		/*Enviar*/
		
		strcpy(missatge,mess);
		memset(buffer, '\0', 256);
		strcpy(buffer,missatge); //Copiar missatge a buffer
		result = write(newFd, buffer, strlen(buffer)+1); //+1 per enviar el 0 final de cadena
		
		printf("Message sent to the client (bytes %d): %s\n\n",	result, missatge);
		
		/*Tancar el socket fill*/
		result = close(newFd);
	}
}


/* main complet */


int main (int argc, char **argv) {

	fd = ConfigurarSerie();

	pthread_t th_Client;
	pthread_create(&th_Client, NULL, mainClient, 0);
	pthread_t th_Arduino;
	pthread_create(&th_Arduino, NULL, mainArduino,0);

	pthread_join(th_Arduino, NULL);
	pthread_join(th_Client, NULL);
	
	TancarSerie(fd);
	
}