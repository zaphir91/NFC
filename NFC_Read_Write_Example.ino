/*
Contact :
RANDRIA Solofo
solofo.randria91@gmail.com
reddit.com/u/zaphir3

This example shows the read and write function to a NFC tag.
It communicates with a tag via a MFRC522 over SPI communication
This example have been tested with Ntag 213-215-216

Just plug your arduino, wire it to your MFRC522, launch the serial monitor and have fun !'


Thanks to this post that prompted me to make this example
https://forum.arduino.cc/t/reading-writing-ntag215-with-rc522/1111130

NFC Ntag21x documentation
https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf
*/
///////////////////Libraries
#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>

///////////////////Definitions
#define RST_PIN 9   // Pin connected to RST on the RFID module
#define SS_PIN 10   // Pin connected to SDA on the RFID module


///////////////////Variables
byte Incomming_message[64];        //message to read/write
byte Read_pages[16];               //content of read pages x to x+4
byte Read_size = 18;                 // 4 pages (16 = 4x4bytes) pages + crc


bool Serial_input_detected;          //Sends a pulse when an input from serial teminal is detected
bool Do_Read;                     //cycle start to do an action
bool Tag_Present;                   //return if tag is present
bool Tag_Ok;                        //return if tag is readable
bool Help;                          //Bring the help section

int Page_to_process;                   //Used in read functions
int Number_of_incomming_Byte;      //calculated after read serial
int Number_taken_pages;             //calculated after read serial


short G7 = -1;                      //Current function
short G7_memo = -1;                 //Allow creation of rising edge
short G7_function = 0;               //Current step of the function
short G7_function_memo = -1;                 //Allow creation of rising edge

char index;                          //for differents for loop
char Page_offset = 0;               //offset or processed page

///////////////////Timer target
unsigned long TON_Helper_message = 1000;
unsigned long TON_Scan_tag_message = 1000;



///////////////////Sub functions references
void Serial_Message();
void Helper();
void Home();
void G7Test();
void G7Navigaion();
void G7home();
void G7Read();
void Contact();
void G7Write();

///////////////////Instances
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance



//////////////////Program
void setup() {
  SPI.begin();          //start SPI communication
  Serial.begin(9600);   //start serial communication
  mfrc522.PCD_Init();   // Init MFRC522 module
  G7 = 0;               // Initiate program
}

void loop() {
  //Check if a card is present
  Tag_Present = mfrc522.PICC_IsNewCardPresent(); // store the value of tag Present to not call the function multiple times
  Tag_Ok = mfrc522.PICC_ReadCardSerial();       // store the value of tag OK to not call the function multiple times



  //Card_Detection();
  Helper();           //Message every 5min to show that it is alive. Display possible command at the same time
  Serial_Message();   //Read serial buffer
  G7Navigaion();       //process user input
  
  switch (G7) { //Grafcet architecture to navigate through the functions                                                                                 (sorry, I come from the PLC wolrd :p)
    case 0: // Homepage
        G7home();
        
    break;

    case 1: // Homepage
        G7Read();
    break;

    case 2: // Write
        G7Write();
    break;

    case 3: // tag detection
        G7Test();
    break;
  }



  G7_memo = G7; // allow the creation of rising trigger for messages
  Serial_input_detected = false; //Reset pulse from input serial  
  Help = false; // reset the action to bring up the help section
  G7_function_memo = G7_function; // allow the creation of rising trigger for messages
}





void G7home(){ //Home page, display what you can do
  if (G7_memo != G7 || Help == true) { // Display message on rising edge
    Serial.println("Here is a list of command you can do : ");
    Serial.println("!Read : Read chunks of 4 pages of NFC tags ");
    Serial.println("!Write : Write chunk of multiple pages of NFC tags ");
    Serial.println("!Test : Test if a tag is detected ");
    Serial.println("!Home : Display the home page ");
  }
}


void G7Read(){// Read 4 pages
  if (G7_memo != G7) { // Display message upon entering the function
    Serial.println("Which pages do you want to read ?");
    Serial.println("max values are :");
    Serial.println("NTAG213 : 44");
    Serial.println("NTAG215 : 134");
    Serial.println("NTAG216 : 230");
    Serial.println("");
    Do_Read = false; // reset any incoming action on entering the new function
  }
  if (Help == true) { // Display message on help demmand
    Serial.println("The read function allow to read 4 pages at the same time");
    Serial.println("1 Pages = 4 bytes. Values are written in Hex for ease of use");
    Serial.println("'ABCD' would be equal to 1 page containing 0x41 0x42 0x43 0x44");
    Serial.println("Enter the adress of the first page you want to read");
    Serial.println("max pages are :");
    Serial.println("NTAG213 : 44");
    Serial.println("NTAG215 : 134");
    Serial.println("NTAG216 : 230");
    Serial.println("");
    Do_Read = false; // allow the user to enter a new line after displaying the help section
  }
  
  if (G7_memo == G7) {//begin the scaning once the opening message have been displayed

    if (Serial_input_detected) {
      Do_Read = true; // raise a flag to say that a reading has been demanded
    }

    if (Do_Read && !Tag_Present && !Tag_Ok && millis() > TON_Scan_tag_message) { // print a message to scan tag if the flag to read is up
      Serial.println("Waiting for a tag");
      TON_Scan_tag_message = millis() + 10000; //only print the message every 10sec
    }

    

    if (Tag_Present && Tag_Ok && Do_Read) { //If the read flag is up, and a card is present, read following 4 pages
      Page_to_process = atoi((char*)Incomming_message);      // convert user input to int
      
      MFRC522::StatusCode status = mfrc522.MIFARE_Read(Page_to_process, Read_pages, &Read_size);   //get where to read

      if (status == MFRC522::StatusCode::STATUS_OK) { // read was successful
        Page_offset = 0; // offset of the current page read
        for (index = 0; index<16; index++) {//scan all the received bytes

          if ((index % 4 == 0)) {// new lines every 4 byte (new page)
            if(index > 0){//increase index of shown page, just for feedback, does not do anything with the tag itself
              Page_offset++;
            }
            Serial.println();
            Serial.print("Page ");
            Serial.print(Page_to_process + Page_offset);
            Serial.print(" : ");
          }
          Serial.print(Read_pages[index], HEX); //print a byte of the page
          Serial.print(" ");              //add a space between each byte

        }
        //mfrc522.PICC_HaltA(); // stop communication with card
        Do_Read = false;  //reset Read flag
        Serial.println();
        Serial.println("Enter a new page to read :");
      }
          
    }
  }

}

void G7Test(){  //trigger is a card has been detected
  if (G7_memo != G7 || Help == true) { // Display message on rising edge
    Serial.println("Put a tag near your MFRC522");
    Do_Read = false; // reset any incoming action on entering the new function
  }

  if (Tag_Present && Tag_Ok) {  //message on rising trigger card detected
    Serial.println("System : tag is detected");

    Serial.print(F("Card UID:")); //Display the UID of the read tag. UID is equivalent to serial number of the tag
    for (byte i = 0; i < mfrc522.uid.size; i++) {/*this part was taken from "change UID" example. File>Example>MFRC522>Change UID*/
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    } 
    Serial.println();
  }
}

void G7Write(){ // write x pages
  if (G7_memo != G7) { // Display message upon entering the function
    Serial.println("Message must be less than 64 charateres");
    Serial.println("Which page do you want to write ?");
    Serial.println("Max page are :");
    Serial.println("NTAG213 : 44");
    Serial.println("NTAG215 : 134");
    Serial.println("NTAG216 : 230");
    Serial.println("");
    Do_Read = false; // reset any incoming action on entering the new function
    G7_function = 0;   // reset any incoming ongoing inner grafcet
    G7_function_memo = -1;   // reset any incoming ongoing inner grafcet
  }

  if (Help == true) { // Display message on rising edge
    Serial.println("The write function allow to write any number of pages at the same time");
    Serial.println("1 charater = 1 byte. 4 bytes = 1 Page");
    Serial.println("'ABCD' would be equal to 1 page containing 0x41 0x42 0x43 0x44");
    Serial.println("Your input must be less than 64 charateres (Byte)");
    Serial.println("page 0-3 is reserved for system value of tag, Hence why you cannot write in them in this project");
    Serial.println("Page 2 contain element to lock the tag permanently. becarful on you own project");
    Serial.println("Enter the adress of the first page you want to write");
    Serial.println("max pages are :");
    Serial.println("NTAG213 : 44");
    Serial.println("NTAG215 : 134");
    Serial.println("NTAG216 : 230");
    Serial.println("");
    G7_function = 0;   // reset the inner grafcet to allow user to enter a new pages to write
    G7_function_memo = -1;
  }

  if ((G7_function == 0) && (G7_function != G7_function_memo)) {//step 0 adress
  Serial.println("Enter the adress of page you want to write");
  }

  if (G7_function == 0 && (G7_function == G7_function_memo) && Serial_input_detected == true) { //step 0, user enter adress of first page to write
    Page_to_process = atoi((char*)Incomming_message);      // convert user input to int

    if (Page_to_process < 4) {// page 0-3 is reserved for system tag
      G7_function = 99;
    }
    else {
      G7_function = 1;
    }
    
  }

  if ((G7_function == 1)  && (G7_function != G7_function_memo)) {//step 1 message
  Serial.println("Enter your message");
  }

  if (G7_function == 1 && (G7_function == G7_function_memo)  && Serial_input_detected == true) { //step 1, user enter his message to be written
    Serial.print("You wrote : ");
    for (index = 0; index < 64; index++) {// read byte by byte to print what was entered
    Serial.print((char)Incomming_message[index]);
    }
    Serial.println("");
    Serial.print("Your message is ");
    Serial.print(Number_of_incomming_Byte);
    Serial.print(" bytes long, and takes ");
    Serial.print(Number_taken_pages);
    Serial.println(" Pages");
    Serial.print("It will be written starting at page ");
    Serial.println(Page_to_process);
    G7_function = 2;
    Page_offset = 0;
  }

  if (G7_function == 2 && (G7_function == G7_function_memo) && !Tag_Present && !Tag_Ok && millis() > TON_Scan_tag_message) { // print a message to scan tag if the step is currently to the write step
    Serial.println("Waiting for a tag");
    TON_Scan_tag_message = millis() + 10000; //only print the message every 10sec
  }


  if (G7_function == 2 && (G7_function == G7_function_memo) && Tag_Present && Tag_Ok) { // step 2 waiting step
      G7_function = 3;
  }

  if (G7_function == 3 && (G7_function == G7_function_memo)) { //step 3 write to tag
  
  MFRC522::StatusCode status = mfrc522.MIFARE_Ultralight_Write(Page_to_process + Page_offset, &Incomming_message[0 + (Page_offset * 4)], 4); // always write 4 bytes at a time

    if (status == MFRC522::StatusCode::STATUS_OK) {
      if (Page_offset < Number_taken_pages) {//Check if there is still pages to be written
        Page_offset++;
        G7_function = 2;
      }
      else {
        Serial.println("Message has been written to tag");
        G7_function = 0;
        for (index = 0; index < 64 ; index++) {//clear buffer after writing
          Incomming_message[index] = 0;
        }
      }
      
            

    }
  }


  /*error section*/

  if (G7_function == 99) {//step 99 user tried to write in system pages
    Serial.println("page 0-3 is reserved for system values");
    G7_function = 0;
  }


}

void Helper(){  // Print instruction over serial;
  if (millis() >= TON_Helper_message) {           //resend welcome message every 5 min
    Serial.println("If you have any issues, don't hesitate do to !help to get a list of command");
    Serial.println("You can also do !Home at any time to get back to the selection menu");
    Serial.println("");
    TON_Helper_message = millis() + 300000;       //300 seconds (5min)
  }
}

void Serial_Message(){  //reading incomming message
  if (Serial.available() > 0) {
    for (index = 0; index < 64 ; index++) {           //clear buffer before reading
      Incomming_message[index] = 0;
    }
    Serial.readBytesUntil(0x0A,Incomming_message,64);     //Read up to line feed 
    Serial_input_detected = true;                         //generate a pulse when a new input have been detected     


    for (index = 0; index < 64; index++) { // find the number of byte in message
      if (Incomming_message[index] == 0) { 
        Number_of_incomming_Byte = index; //Size of the message
        Number_taken_pages = (Number_of_incomming_Byte / 4); // number of potential pages the message will take
        if ((Number_of_incomming_Byte % 4) > 0) { //round to the celling the number of page
          Number_taken_pages++;
        }
        return;
      }
    }
  }
   
}

void G7Navigaion(){  // process the differents commands
  if (!strcmp(Incomming_message, "!Help")) {//Send to home
    Help = true; //Bring up the help section
    Serial_input_detected = false; //put this flag to 0 to not trigger any action
    for (index = 0; index < 64 ; index++) {           //clear buffer text
      Incomming_message[index] = 0;
    }
  }
  if (!strcmp(Incomming_message, "!Home")) {//Send to home
    G7 = 0;
  }
  else if (!strcmp(Incomming_message, "!Read")) {// Send to read command
    G7 = 1;
  }
  else if (!strcmp(Incomming_message, "!Write")) {// Send to write command
    G7 = 2;
  }
  else if (!strcmp(Incomming_message, "!Test")) {// Send to write command
    G7 = 3;
  }
}

