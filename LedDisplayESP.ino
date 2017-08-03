#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

#include <SPI.h> //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //

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
      dmd.selectFont(Arial_14);
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
    case 4:
    {
      dmd.clearScreen( true );
      
      pch = strtok(NULL, "#\n");
      
      printImg(pch);
      flagM = false;
      
      break;
    }
  }

return 0; 
}

void printImg(char* buf)
{
//  Serial.println(buf);  //debug
//  Serial.println(strlen(buf));  //debug
  int i = 0;
  int len = strlen(buf); 
  
  for (byte y = 0; y < DMD_PIXELS_DOWN; y++) {
    for (byte x = 0; x < DMD_PIXELS_ACROSS*DISPLAYS_ACROSS; x++) {
      if (buf[i] == '1' && i <= len) {
//        Serial.print("i");  //debug
//        Serial.println(i);  //debug
        dmd.writePixel(x, y, GRAPHICS_NORMAL, true);
        
      }
      i++;
    }
  }
}
