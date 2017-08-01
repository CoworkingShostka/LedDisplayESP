#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

#include <SPI.h> //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //

 #define Test 0
  //Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 3
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
  /*--------------------------------------------------------------------------------------
    Interrupt handler for Timer1 (TimerOne) driven DMD refresh scanning, this gets
    called at the period set in Timer1.initialize();
  --------------------------------------------------------------------------------------*/
void ScanDMD()
  { 
    dmd.scanDisplayBySPI();
  }

#include "RusSystemFont5x7.h"
#include "RusArial14.h"
#include "Arial_black_16.h"

static uint32_t last;
long timer;
bool flagM = false;

///
///esp-link init block
///
// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages.
ELClient esp(&Serial, &Serial);

// Initialize CMD client (for GetTime)
ELClientCmd cmd(&esp);

// Initialize the MQTT client
ELClientMqtt mqtt(&esp);

// Callback made from esp-link to notify of wifi status changes
// Here we just print something out for grins
void wifiCb(void* response) {
  ELClientResponse *res = (ELClientResponse*)response;
  if (res->argc() == 1) {
    uint8_t status;
    res->popArg(&status, 1);

    if(status == STATION_GOT_IP) {
      Serial.println("WIFI CONNECTED");
    } else {
      Serial.print("WIFI NOT READY: ");
      Serial.println(status);
    }
  }
}

bool connected;

// Callback when MQTT is connected
void mqttConnected(void* response) {
  Serial.println("MQTT connected!");
  //mqtt.subscribe("AS/FirstDoor/server_response");
 // mqtt.subscribe("AS/FirstDoor/server_data");
  mqtt.subscribe("/ipanel/command");
  
  connected = true;
}

// Callback when MQTT is disconnected
void mqttDisconnected(void* response) {
  Serial.println("MQTT disconnected");
  connected = false;
}

// Callback when an MQTT message arrives for one of our subscriptions
void mqttData(void* response) {
  ELClientResponse *res = (ELClientResponse *)response;

  Serial.print("Received: topic=");
  String topic = res->popString();
  Serial.println(topic);

  Serial.print("data=");

  char data[res->argLen()+1];
  res->popChar(data);
  
  //String data = res->popString();
  Serial.println(data);

  modeSwitch(data);
    
}

void mqttPublished(void* response) {
  Serial.println("MQTT published");
}

void setup() {
  Serial.begin(115200);
  Serial.println("EL-Client starting!");

   // Sync-up with esp-link, this is required at the start of any sketch and initializes the
  // callbacks to the wifi status change callback. The callback gets called with the initial
  // status right after Sync() below completes.
  esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
  bool ok;
  do {
    ok = esp.Sync();      // sync up with esp-link, blocks for up to 2 seconds
    if (!ok) Serial.println("EL-Client sync failed!");
  } while(!ok);
  Serial.println("EL-Client synced!");

  // Set-up callbacks for events and initialize with es-link.
  mqtt.connectedCb.attach(mqttConnected);
  mqtt.disconnectedCb.attach(mqttDisconnected);
  mqtt.publishedCb.attach(mqttPublished);
  mqtt.dataCb.attach(mqttData);
  mqtt.setup();

  Serial.println("EL-MQTT ready");

  //LED panel setup
  //clear/init the DMD pixels held in RAM
  dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
//      dmd.selectFont(SystemFont5x7);
      
  #if Test == 1
byte testB[] = {0x00, 0x08, 0xC0, 0x60, 0x30, 0x18, 0x00, 0x00, 0x01, 0x03, 0x06, 0x04, 0x0C, 0x18};


  
  #endif
   
}

void loop() {
  esp.Process();


    if((millis()- last) > 3)
    {
      ScanDMD();
    }

    if(flagM && ((timer + 30) < millis()))
    {
      dmd.stepMarquee(-1,0);
      
      timer = millis();
    }
}

void modeSwitch(char* str)
{
  char * pch;
  pch = strtok(str, "#");

  //Serial.println(pch);  //debug
//  while (pch!= NULL)
//      {
//        Serial.println(pch);
//        pch = strtok(NULL, "#");
//      }
     
  int mode = atoi(pch);
 
  switch (mode)
  {
    case 1:
    {
      dmd.selectFont(Arial_Black_16);
      dmd.clearScreen( true );
      
      pch = strtok(NULL, "#");
      //Serial.println(pch);  //debug
      dmd.drawString(0,0,pch,strlen(pch), GRAPHICS_NORMAL);

      flagM = false;
      
      break;
    }
    case 2:
    {
      dmd.selectFont(SystemFont5x7);
      dmd.clearScreen( true );

      
      pch = strtok(NULL, "#");
      //Serial.println(pch);  //debug
      dmd.drawString(0,0,pch,strlen(pch), GRAPHICS_NORMAL);

      pch = strtok(NULL, "#");
      //Serial.println(pch);  //debug
      dmd.drawString(0,8,pch,strlen(pch), GRAPHICS_NORMAL);

      flagM = false;
      
      break;
    }
    case 3:
    {
      dmd.selectFont(Arial_14);
      dmd.clearScreen( true );

      pch = strtok(NULL, "#");
      dmd.drawMarquee(pch, strlen(pch), 0, 0);

      timer = millis();
      flagM = true;

      break;
    }
  }
  
//  String str1 = getValue(str, '#', 1);
//  
//  char MSG1[16];
//  str1.toCharArray(MSG1, str1.length()+1);
//  
//  switch (mode){
//    case 1:
//    {
//      dmd.selectFont(Arial_14);
//      dmd.clearScreen( true );
//      
//      dmd.drawString(2,2,MSG1,strlen(MSG1), GRAPHICS_NORMAL);
//      
//      break;
//    }
//    case 2:
//    {    
//      dmd.selectFont(SystemFont5x7);
//      dmd.clearScreen( true );
//
//      String str2 = getValue(str, '#', 2);
//      char MSG2[16];
//      str2.toCharArray(MSG2, str2.length()+1);
//      
//      dmd.drawString(0,0,MSG1,strlen(MSG1), GRAPHICS_NORMAL);
//      dmd.drawString(0,8,MSG2,strlen(MSG2), GRAPHICS_NORMAL);
//
//      Serial.println(str1);
//      Serial.println(MSG1);
//            
//      break;
//    }
//    case 3:
//    {
//      dmd.selectFont(Arial_14);
//      dmd.clearScreen( true );
//
//      dmd.drawMarquee(MSG1, strlen(MSG1), 0, 0);
//
//      break;
//    }
//  }

return 0; 
}

//split string fnctn
//String getValue(char data, char separator, int index)
//{
//    int found = 0;
//    int strIndex[] = {0, -1};
//    int maxIndex = sizeof(data) - 1;
//
//    for (int i = 0; i <= maxIndex && found <= index; i++)
//    {
//        if (data[i] == separator || i == maxIndex)
//        {
//            found++;
//            strIndex[0] = strIndex[1] + 1;
//            strIndex[1] = (i == maxIndex) ? i + 1 : i;
//        }
//    }
//
//    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
//}

