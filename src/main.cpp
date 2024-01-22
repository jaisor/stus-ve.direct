#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>
#include <SPI.h>

#include "config.h"
#include "Configuration.h"

#if defined(ESP32)
#elif defined(ESP8266)
  #include <SoftwareSerial.h> 
#elif defined(SEEED_XIAO_M0)
  #include <ArduinoLowPower.h>
#else
  #error Unsupported platform
#endif


#if defined(ESP32)
  #define VE_RX GPIO_NUM_16
  #define VE_TX GPIO_NUM_17
#elif defined(ESP8266)
  #define VE_RX D3
  #define VE_TX D4
#elif defined(SEEED_XIAO_M0)
  #define VE_RX D7
  #define VE_TX D6
#endif

unsigned long tsMillisBooted;

#if defined(DISABLE_LOGGING) && defined(SEEED_XIAO_M0)
  #define LED_SETUP PIN_LED_TXL
#else
  #define LED_SETUP INTERNAL_LED_PIN
#endif

Stream *VEDirectStream;

//SoftwareSerial victronSerial(rxPin, txPin);         // RX, TX Using Software Serial so we can use the hardware serial to check the ouput
                                                    // via the USB serial provided by the NodeMCU.
char receivedChars[buffsize];                       // an array to store the received data
char tempChars[buffsize];                           // an array to manipulate the received data
char recv_label[num_keywords][label_bytes]  = {0};  // {0} tells the compiler to initalize it with 0. 
char recv_value[num_keywords][value_bytes]  = {0};  // That does not mean it is filled with 0's
char value[num_keywords][value_bytes]       = {0};  // The array that holds the verified data
static byte blockindex = 0;
bool new_data = false;
bool blockend = false;


void RecvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    while (VEDirectStream->available() > 0 && new_data == false) {
        rc = VEDirectStream->read();
        Serial.write(rc);
        /*
        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= buffsize) {
                ndx = buffsize - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            new_data = true;
        }
        yield();
        */
    }
}

void ParseData() {
    char * strtokIndx; // this is used by strtok() as an index
    strtokIndx = strtok(tempChars,"\t");      // get the first part - the label
    // The last field of a block is always the Checksum
    if (strcmp(strtokIndx, "Checksum") == 0) {
        blockend = true;
    }
    strcpy(recv_label[blockindex], strtokIndx); // copy it to label
    
    // Now get the value
    strtokIndx = strtok(NULL, "\r");    // This continues where the previous call left off until '/r'.
    if (strtokIndx != NULL) {           // We need to check here if we don't receive NULL.
        strcpy(recv_value[blockindex], strtokIndx);
    }
    blockindex++;

    if (blockend) {
        // We got a whole block into the received data.
        // Check if the data received is not corrupted.
        // Sum off all received bytes should be 0;
        byte checksum = 0;
        for (int x = 0; x < blockindex; x++) {
            // Loop over the labels and value gotten and add them.
            // Using a byte so the the % 256 is integrated. 
            char *v = recv_value[x];
            char *l = recv_label[x];
            while (*v) {
                checksum += *v;
                v++;
            }
            while (*l) {
                checksum+= *l;
                l++;
            }
            // Because we strip the new line(10), the carriage return(13) and 
            // the horizontal tab(9) we add them here again.  
            checksum += 32;
        }
        // Checksum should be 0, so if !0 we have correct data.
        if (!checksum) {
            // Since we are getting blocks that are part of a 
            // keyword chain, but are not certain where it starts
            // we look for the corresponding label. This loop has a trick
            // that will start searching for the next label at the start of the last
            // hit, which should optimize it. 
            int start = 0;
            for (int i = 0; i < blockindex; i++) {
              for (int j = start; (j - start) < num_keywords; j++) {
                if (strcmp(recv_label[i], keywords[j % num_keywords]) == 0) {
                  // found the label, copy it to the value array
                  strcpy(value[j], recv_value[i]);
                  start = (j + 1) % num_keywords; // start searching the next one at this hit +1
                  break;
                }
              }
            }
        }
        // Reset the block index, and make sure we clear blockend.
        blockindex = 0;
        blockend = false;
    }
}

void HandleNewData() {
    // We have gotten a field of data 
    if (new_data == true) {
        //Copy it to the temp array because parseData will alter it.
        strcpy(tempChars, receivedChars);
        ParseData();
        new_data = false;
    }
}

void PrintValues() {
    for (int i = 0; i < num_keywords; i++){
        Serial.print(keywords[i]);
        Serial.print(",");
        Serial.println(value[i]);
    }
}

void PrintEverySecond() {
    static unsigned long prev_millis;
    if (millis() - prev_millis > 1000) {
        //PrintValues();
        prev_millis = millis();
    }
}


void setup() {

  pinMode(INTERNAL_LED_PIN, OUTPUT);
  
  pinMode(LED_SETUP, OUTPUT);
  digitalWrite(LED_SETUP, LOW);

  #ifndef DISABLE_LOGGING
  Serial.begin(19200);  while (!Serial); delay(100);
  randomSeed(analogRead(0));
  Log.begin(LOG_LEVEL, &Serial);
  Log.infoln("Initializing...");
  #endif

  //device = new CDevice();
  //rf24Manager = new CRF24Manager(device);
  tsMillisBooted = millis();

#if defined(ESP32)
  Serial2.begin(19200, SERIAL_8N1, VE_RX, VE_TX);
  VEDirectStream = &Serial2;
#elif defined(ESP8266)
  pinMode(VE_RX, INPUT);
  pinMode(VE_TX, OUTPUT);
  SoftwareSerial *ves = new SoftwareSerial(VE_RX, VE_TX);
  ves->begin(19200, SWSERIAL_8N1);
  VEDirectStream = ves;
#elif defined(SEEED_XIAO_M0)
  Serial1.begin(19200, SERIAL_8N1);
  VEDirectStream = &Serial1;
#endif
  
  delay(1000);
  Log.infoln("Initialized");
  digitalWrite(LED_SETUP, HIGH);
}

void loop() {

  // TODO: Eventually use https://github.com/giacinti/VeDirectFrameHandler/blob/master/example/ReadVEDirectFramehandler.ino
  // Protocol https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.33.pdf

  //Log.infoln("VE_SOC = %i", ved->read(VE_SOC));
  /*
  VESerial->begin(VED_BAUD_RATE);
	if (VESerial) {
    Log.infoln("VESerial began RX=%i; TX=%i;", rxPin, txPin);
		delay(500);
		while(VESerial->available()) {
      Log.info("%i", VESerial->read());
			//VESerial->flush();
			//VESerial->end();
		}
	}
  */

  RecvWithEndMarker();
  HandleNewData();
  PrintEverySecond();
	
  yield();
}


