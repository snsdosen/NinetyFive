//NinetyFive - Arduino automotive EEPROM reader
//snsdosen 2023
//This project is primary aimed at reading and writing ST95P08 EEPROMs found in various ECUs
//Other ST EEPROMS might be supported with little or no modification.

#include <SPI.h>

#define APP_NAME  "NinetyFive"
#define VERSION   "0.1a"

//Some other EEPROMS use dual byte addressing
//#define MODE_DUAL_BYTE_ADDR

//Hardware specific
#define CS_PIN      10        //Slave Select
#define PAGE_SIZE   16        //64 pages of 16 bytes for ST95P08
#define PAGE_COUNT  64

//SPI opcodes
#define WREN    0b00000110    //Set Write Enable Latch
#define WRDI    0b00000100    //Reset Write Enable Latch
#define RDSR    0b00000101    //Read Status Register
#define WRSR    0b00000001    //Write Status Register
#define READ    0b00000011    //Read Data from Memory Array
#define WRITE   0b00000010    //Write Data to Memory Array

//User commands
#define READ_CMD  "read"
#define WRITE_CMD "write"
#define CLS_CMD   "cls"
#define TEST_CMD  "test"

//Console messages
#define DONE_MSG      "DONE!"
#define ERR_MSG       "ERROR!"
#define TIMEOUT_MSG   "TIMEOUT!"

//Writing mode, data must not be parsed by console
bool writeMode = false;

//Input buffer for serial console (32 bytes)
unsigned char serialBuffer[32];
byte serialIndex = 0;

//Input buffer for data write
byte inputBuffer[PAGE_SIZE];
byte charBuffer[3];
int inputIndex = 0;
int charIndex = 0;
int inputCounter = 0;
int pageCounter = 0;

//Write timeout
unsigned long startTime = 0;

//Zero out input buffer
void clearInputBuffer(){
  serialIndex = 0;
  memset(serialBuffer, 0, sizeof(serialBuffer));
}

void setup() {
  Serial.begin(115200);

  //Wait for serial to be available
  while(!Serial);

  //Set up SPI comms
  SPISettings spiSettings(F_CPU / 8, MSBFIRST, SPI_MODE0);
  SPI.begin();
  SPI.beginTransaction(spiSettings);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // disable slave device
}

//This function is ran when "test" is written in command line
//If you (the user) need to test something, place it here and fire test from command line
//EG. you are porting this code for another EEPROM and want to test out various SPI commands
void test(){
  Serial.println("TESTED!");
}

//Write a single page to EEPROM
void writePage(int address){
  //Write enable
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(WREN);
  digitalWrite(CS_PIN, HIGH);

  delay(5);

  digitalWrite(CS_PIN, LOW);

  //Write page
#ifdef MODE_DUAL_BYTE_ADDR
  SPI.transfer(WRITE);
  SPI.transfer(address >> 8);
  SPI.transfer(address);
#else
  SPI.transfer(WRITE | (address >> 5) & 24);
  SPI.transfer(address);
#endif

  //Transfer page size worth of data
  for(int i = 0; i < PAGE_SIZE; i++){
    SPI.transfer(inputBuffer[i]);
  }

  digitalWrite(CS_PIN, HIGH);

  delay(10);
}

//Write data from pasted ASCII HEX dump
void writeData(){
  byte activeByte = 0;

  //Read all data from the serial buffer
  while(Serial.available() > 0){
    charBuffer[charIndex] = Serial.read();
    inputCounter++;

    if(inputCounter % 3 == 0){
      activeByte = strtol(charBuffer, NULL, 16);

      if(activeByte < 16) Serial.print("0");
      Serial.print(activeByte, HEX);

      //Place data to input buffer
      inputBuffer[inputIndex] = activeByte;

      if(inputIndex < PAGE_SIZE - 1) inputIndex++;
      else inputIndex = 0;

      //Single page has been fetched
      if(inputCounter % PAGE_SIZE == 0) {
        Serial.println();

        //Write buffer to EEPROM
        writePage(pageCounter * PAGE_SIZE);

        pageCounter++;
      }
      else {
        Serial.print(" ");
      }
    }

    if(charIndex < 2) charIndex++;
    else charIndex = 0;
  }

  //Check if timeout condition occured (prevents lockout of Arduino)
  if(inputCounter < (PAGE_SIZE * PAGE_COUNT * 3) - 1 && millis() > startTime + 5000){
    Serial.println(TIMEOUT_MSG);

    //Exit write mode
    writeMode = false;
    inputIndex = 0;
    charIndex = 0;
    inputCounter = 0;
    pageCounter = 0;
  }

  //Check if data input is complete
  if(inputCounter == (PAGE_SIZE * PAGE_COUNT * 3) - 1){
    //Write last page
    inputBuffer[PAGE_SIZE-1] = activeByte;
    writePage(PAGE_SIZE * (PAGE_COUNT - 1));

    writeMode = false;
    inputIndex = 0;
    charIndex = 0;
    inputCounter = 0;
    pageCounter = 0;

    Serial.println();
    Serial.println(DONE_MSG);
  }
}

//Very direct ASCII HEX dump of the chip data to the screen
void readData(){
  byte buff = 0;
  digitalWrite(CS_PIN, LOW);

  //Send read command to EEPROM
  SPI.transfer(READ);

  //Address (0)
  SPI.transfer(0);

#ifdef MODE_DUAL_BYTE_ADDR
  //Additional address byte needed
  SPI.transfer(0);
#endif

  int lineCounter = 0;

  //Read all pages and dump them on the screen
  for(int j = 0; j < 64;j++){
    for(int i = 0; i < PAGE_SIZE;i++){
      buff = SPI.transfer(0);

      if(buff < 16) Serial.print("0");
      Serial.print(buff, HEX);

      //Print either a newline or space
      if(lineCounter == 15) {
        Serial.println();
        lineCounter = 0;
        }
      else {
        Serial.print(" ");
        lineCounter++;
      }
    }

    //Artificial delay to slow down the printing on the screen
    //and to give EEPROM more than enough time to switch to next page if need be
    delay(30);
  }

  digitalWrite(CS_PIN, HIGH);

  Serial.println();
}

void loop() {
  byte readByte = 0;

  //Ignore console in write mode
  if(writeMode){
    writeData();
    return;
  }

 if(Serial.available() > 0){
   readByte = Serial.read();
   
  Serial.write(readByte);

    if(readByte == '\r') {
      Serial.println();

      if(strcmp(serialBuffer, READ_CMD) == 0){
        Serial.println("READING...");
        readData();
        Serial.println(DONE_MSG);
      }
      else if(strcmp(serialBuffer, WRITE_CMD) == 0){
        Serial.println("WAITING DATA INPUT");
        startTime = millis();
        writeMode = true;
      }
      else if(strcmp(serialBuffer, CLS_CMD) == 0){
        Serial.write(12);
      }else if(strcmp(serialBuffer, TEST_CMD) == 0){
        test();
      }else{
        Serial.println(ERR_MSG);
      }

    //Accept data again
    clearInputBuffer();

    }
    else{
      //Append character to the circular buffer
      serialBuffer[serialIndex] = readByte;
      if(serialIndex < sizeof(serialBuffer))serialIndex++;
      else serialIndex = 0;
    }
 }
}
