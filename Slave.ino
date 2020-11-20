//-----------------------------------------------------------------------------------------------------------//
//  This entire file contains code from various sources. Credit is given in the README.md file of the GitHub project.
//                                                                                                           //
//  Slave_ELEC1601_Student_2019_v3                                                                           //
//  The Instructor version of this code is identical to this version EXCEPT it also sets PIN codes           //
//  20191008 Peter Jones                                                                                     //
//                                                                                                           //
//  Bi-directional passing of serial inputs via Bluetooth                                                    //
//  Note: the void loop() contents differ from "capitalise and return" code                                  //
//                                                                                                           //
//  This version was initially based on the 2011 Steve Chang code but has been substantially revised         //
//  and heavily documented throughout.                                                                       //
//                                                                                                           //
//  20190927 Ross Hutton                                                                                     //
//  Identified that opening the Arduino IDE Serial Monitor asserts a DTR signal which resets the Arduino,    //
//  causing it to re-execute the full connection setup routine. If this reset happens on the Slave system,   //
//  re-running the setup routine appears to drop the connection. The Master is unaware of this loss and      //
//  makes no attempt to re-connect. Code has been added to check if the Bluetooth connection remains         //
//  established and, if so, the setup process is bypassed.                                                   //
//                                                                                                           //
//-----------------------------------------------------------------------------------------------------------//

#include <SoftwareSerial.h>   //Software Serial Port
#include <Servo.h>    // Include servo library
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

Servo servoLeft;   // Declare left and right servos
Servo servoRight;

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################

int shieldPairNumber = 11;

// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// #######################################################

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);

void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}

void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400

    servoLeft.attach(13);               // Attach left signal to pin 13
    servoRight.attach(12); 

    pinMode(10, INPUT);  pinMode(9, OUTPUT);   // Left IR LED & Receiver
    pinMode(3, INPUT);  pinMode(2, OUTPUT);    // Right IR LED & Receiver

    servoLeft.writeMicroseconds(1300); 
    servoRight.writeMicroseconds(1300); 
    delay(3000);
    
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
}

void forward(int time)
{
    //append(&head, 1);
    servoLeft.writeMicroseconds(1700);
    servoRight.writeMicroseconds(1300);
    delay(time);
}
  
void backward(int time)
{
    //append(&head, 2);
    servoLeft.writeMicroseconds(1300);
    servoRight.writeMicroseconds(1700);
    delay(time);
}
  
void leftTurn(int time) 
{
    //append(&head, 3);
    servoLeft.writeMicroseconds(1300);
    servoRight.writeMicroseconds(1300);
    delay(time); //
}
  
void rightTurn(int time) 
{
    //append(&head, 4);
    servoLeft.writeMicroseconds(1700);
    servoRight.writeMicroseconds(1700);
    delay(time); //
}

void stop()
{
    servoLeft.writeMicroseconds(1500);
    servoRight.writeMicroseconds(1500);
}

int irDetect(int irLedPin, int irReceiverPin, long frequency)
{
    tone(irLedPin, frequency, 8);              // IRLED 38 kHz for at least 1 ms
    delay(1);                                  // Wait 1 ms
    int ir = digitalRead(irReceiverPin);       // IR receiver -> ir variable
    delay(1);                                  // Down time before recheck
    return ir;                                 // Return 1 no detect, 0 detect
}  
  
int irDeviate()
{
    while(1){
      int irLeft = irDetect(9, 10, 38000);       // Check for object on left
      int irRight = irDetect(2, 3, 38000);       // Check for object on right
      
      if(irLeft == 0 && irRight == 1) 
      {
          Serial.println("left sensor detect only");
          backward(100);
          leftTurn(100);
      }
      else if(irRight == 0 && irLeft == 1)
      {
          Serial.println("right sensor detect only");
          backward(100);
          rightTurn(100);
      }
      else if(irLeft == 0 && irRight == 0)
      {
          Serial.println("both sensors detect");
          stop();
          break;
      }
      else
      {
          forward(500);
      }    
    }
    
    
}

void keyboardControl(char data){
    Serial.println(data);
    switch (data) { // Switch-case of the signals in the serial port 
      case 'a' : // Case Number 1 is received; robot will turn left
        Serial.println("left");
        leftTurn(500);
        stop();
        //servoLeft.writeMicroseconds(1300); 
        //servoRight.writeMicroseconds(1300); 
        break;
      case 's' : // Case Number 2 is received; robot will move backward
        Serial.println("backward");
        backward(500);
        stop();
        //servoLeft.writeMicroseconds(1300); 
        //servoRight.writeMicroseconds(1700);
        break;
      case 'd' : // Case Number 3 is received; robot will turn right
        Serial.println("right"); 
        rightTurn(500);
        stop();
        //servoLeft.writeMicroseconds(1700); 
        //servoRight.writeMicroseconds(1700); 
        break;
      case 'w': // Case Number 4 is received; robot will move forward
        Serial.println("forward"); 
        forward(500);
        stop();
        //servoLeft.writeMicroseconds(1700); 
        //servoRight.writeMicroseconds(1300); 
        break;
      case 'z': // Case Number 5 is received; robot has completed obstacle course and will now begin automatic IR navigation
        irDeviate();
      case '0': // Case Number 6 is received; robot will cease all movement
        Serial.println("stop");
        servoLeft.writeMicroseconds(1500); 
        servoRight.writeMicroseconds(1500); 
      default : 
        break;
    }
}
  
//struct Node()
//{
//    int data;
//    struct Node* next; // Pointer to next node in DLL 
//    struct Node* prev; // Pointer to previous node in DLL $
//}

void loop()
{
    char recvChar;

    while(1)
    {
        if(blueToothSerial.available())   // Check if there's any data sent from the remote Bluetooth shield
        {
            recvChar = blueToothSerial.read();
            Serial.print(recvChar);
            keyboardControl(recvChar);
        }

        if(Serial.available())            // Check if there's any data sent from the local serial terminal
        {
            recvChar  = Serial.read();
            Serial.print(recvChar);
            blueToothSerial.print(recvChar);
        }
    }
}

/*
void append(struct Node** head_ref, int new_data)
{
*/
/*
void printList(struct Node* node)
{
    struct Node* last;

    while (node != NULL)
    {
        printf("%d", node->data);
        last = node;
        node = node->next;
    }
}
  
*/

