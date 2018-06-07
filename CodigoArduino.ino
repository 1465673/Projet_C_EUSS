#include <SoftwareSerial.h> 
#include <avr/interrupt.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <float.h>

float tabtemperature[100];
int i = 0;
int nb;
int n;
int counter = 0; //to know how many temperature we have put in the array
float avg = 0.00;
int current = 0; //to know where we are in the array
int tiempo;
int seconds;

bool start = false; // aquisition false au démarrage

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  // initialize Timer1
  cli(); // disable global interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B

  // set compare match register to desired timer count:
  OCR1A = 15624;

  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);

  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);

  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);

  // enable global interrupts:
  sei();

}

/* simulate the captor of temperature */
ISR(TIMER1_COMPA_vect) {

  //if we have set the ordre to ON
  if (start == true) {
    seconds++;
    if (seconds == tiempo) {
      seconds = 0;
      //the seed the randomazer to generate temperature
      randomSeed(analogRead(0));
      //generate temperature between 0.0 and 30.0
      tabtemperature[i] = random(0.0, 30.0);

      
      //if the array is full we go back to the begin
      if (i == 99) {
        i = 0;
        current = 0;
      }
      //else we just increment 
      else {
        i++;
        current++;
        counter++;
      }

      //if we have put enough data, we calculate the average
      if (counter == nb) {
        avg = getAverage(tabtemperature, nb);
        //Serial.println("avg : ");
        //Serial.println(avg);
        counter = 0;
      }

    }
  }

}



void loop() {

  char acquisition[10] = "stop";
  char mess[10];
  String buffer;
  char val[4];
  char subbuf[10];
  char subbuf2[10];
  char subbuf3[10];
  char buf[10];
  int entry;
  int ret;
  int i = 0;

  //the message we receive from the console
  buffer = Serial.readString();
  
  char* ret2 = new char [buffer.length()+1];
  strcpy(ret2, buffer.c_str());
  
  char order = buffer[1]; //to take the order from the message
  if (buffer != NULL) {

    //Serial.print("I received something: ");
    //Serial.println("message : ");
    //Serial.println(buffer);


    char ch = 'A';
    ret2 = strchr(ret2, ch);
    int i= 0;  
    for(i = 0; i < strlen(ret2); i++) {
      
      if (ret2[i] == 'Z') {
        ret2[i+1] = '\0';
      }
    }

    buffer = ret2;

    char order = buffer[1];

    switch (order) {
    case 'S':

      //take and convert the valor for the entry
      memcpy(buf, &buffer[2], 2);
      buf[2] = '\0';
      entry = atoi(buf);

      //if there is an error in the protocol
      if (buffer[0] != 'A' || buffer[5] != 'Z') {
        strcpy(mess, "AS1Z");
      }
      //if everything is good 
      else if ((entry >= 0 && entry <= 13) || (buffer[4] == 0 || buffer[4] == 1) || buffer[1] == "S") {
        strcpy(mess, "AS0Z");
        memcpy(buf, &buffer[4], 1);
        buf[1] = '\0';
        n = atoi(buf);
        ret = setEntry( &entry, &n);
      }
      //if there is an error in the parameters
      else {
        strcpy(mess, "AS2Z");
      }

      break;

    case 'E':

      //take and convert the valor for the entry
      memcpy(buf, &buffer[2], 2);
      buf[2] = '\0';
      entry = atoi(buf);

      //if there is an error in the protocol
      if (buffer[0] != 'A' || buffer[4] != 'Z') {
        strcpy(mess, "AE1Z");
      }
      //if everything is good
      else if (entry >= 0 && entry <= 13 || buffer[1] == 'E') {
        strcpy(mess, "AE0");
        ret = readEntry( &entry);
        if (ret == 0) {
          strcat(mess, "0");
        } else {
          strcat(mess, "1");
        }
        strcat(mess, "Z");

      }
      //if there is an error with the parameters
      else {
        strcpy(mess, "AE2Z");
      }

      break;

    case 'C':
      
      //if there is an error with the protocol
      if (buffer[0] != 'A' || buffer[2] != 'Z') {
        strcpy(mess, "AC1Z");
      }
      //if there is an error with the parameters
      else if (buffer[1] != 'C') {
        strcpy(mess, "AC2Z");
      }
      //if everything is good
      else {
        strcpy(mess, "AC0");
        dtostrf(avg, 4, 1, subbuf);

        if (avg < 10) {
          dtostrf(avg, 4, 2, subbuf);
        } else {
          dtostrf(avg, 4, 1, subbuf);
        }
        strcat(mess, subbuf);
        strcat(mess, "Z");
      }
      break;

    case 'M':
      
      //to get the differents datas from the message
      memcpy(subbuf, &buffer[2], 1); //to take the valor of the code V
      subbuf[1] = '\0';

      memcpy(subbuf2, &buffer[3], 2); //to take the valor of the time
      subbuf2[2] = '\0';

      memcpy(subbuf3, &buffer[5], 1); //to take the valor of the number of datas
      subbuf3[1] = '\0';

      //if there is a problem in the protocol
      if (buffer[0] != 'A' || buffer[6] != 'Z') {
        strcpy(mess, "AM1Z");
      }
      //if everything is OK
      else if ((buffer[2] == '0' || buffer[2] == '1') && (atoi(subbuf2) >= 1 && atoi(subbuf2) <= 20) && (atoi(subbuf3) >= 1 && atoi(subbuf3) <= 9)) {
        //to start the acquisition of the temperature
        if (strcmp(subbuf, "1") == 0) {
    tiempo = atoi(subbuf2);
    seconds = 0;

          //Serial.print("Acquisition started with an interval in seconds of : ");
          //Serial.println(subbuf2);
          //Serial.print("We calculate median with a number of data of : ");
          //Serial.println(subbuf3);
          nb = atoi(subbuf3);
          strcpy(acquisition, "start");
          start = true;
          strcpy(mess, "AM0Z");

        }
        //stop the acquisition of the temperature
        else {
          strcpy(mess, "AM0Z");
          strcpy(acquisition, "stop");
          start = false;

          counter = 0;
        }
      }
      //if there is a problem in the parameters
      else {
        strcpy(mess, "AM2Z");
      }

      //Serial.print("acquisition is now = ");
      //Serial.println(acquisition);
      break;

    default:
      //Serial.println("invalid order \n");
      strcpy(mess, "AD1Z");
    }

    //print the answer
    //Serial.print("mess : ");
    Serial.println(mess);

  }

}

/* permit to change the state of an entry */
/* *entry is the entry to change */
/* *val is the new state of the entry */
int setEntry(int *entry, int *val) {
  int ret = 0;
  bool state;

  if ( *val == 0) {
    state = LOW;
  } else if ( *val == 1) {
    state = HIGH;
  }

  pinMode(entry, OUTPUT);
  digitalWrite( *entry, state);

  return ret;
}

/* permit to read the entry */
/* int *entry is the number of the entry to read */
/* return the state of the entry */
int readEntry(int *entry) {
  pinMode(entry, INPUT);
  return digitalRead( *entry);

}

/*permit to get the average of the datas in the array */
/* *tab is the array of datas */
/* n is the number of datas to take to calculate the average */
float getAverage(float *tab, int n) {
  float ret;
  int count = 0;
  float sum = 0.0;
  int i = current - n; //to take only the new datas

  //to take the datas in the end of the array if we need it
  if (current - n < 0) {
    i = 100 - current - n;
  }

  while (count < n) {
    //to return to the start of the array
    if (i > 99) {
      i = 0;
    }
    sum = sum + tab[i];
    count++;
    i++;
  }

  ret = sum / count;

  return ret;
}
