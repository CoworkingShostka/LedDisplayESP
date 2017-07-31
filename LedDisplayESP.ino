#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

#define DMDLib 0

#include <SPI.h> //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)

static uint32_t last;

#if DMDLib == 1

  #include <DMD.h>        //
 // #include <TimerOne.h>   //
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

#elif DMDlib == 2

  #include <DMD2.h>
  ///LED Panel Init block
  SoftDMD dmd(3,1);  // DMD2 controls the entire display

#endif

#include "RusSystemFont5x7.h"

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
  mqtt.subscribe("AS/FirstDoor/server_response");
  mqtt.subscribe("AS/FirstDoor/server_data");
  
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
  String data = res->popString();
  Serial.println(data);

  #if DMDLib == 1
    dmd.clearScreen( true );
    ScanDMD();
    
    char MSG1[16];
    data.toCharArray(MSG1, data.length()+1);
    
    dmd.drawString(0,0,MSG1,strlen(MSG1), GRAPHICS_NORMAL);

  #elif DMDLib == 2
    dmd.begin();
    dmd.drawString(0,0,data);

  #endif
    
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
  #if DMDLib == 1
//    initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
      //Timer1.initialize( 5000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
     // Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

      //clear/init the DMD pixels held in RAM
      dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
      dmd.selectFont(SystemFont5x7);
      
  #elif DMDLib == 2
    dmd.setBrightness(255);
    dmd.selectFont(SystemFont5x7);
    dmd.begin();
    
  #endif
   
}

void loop() {
  esp.Process();

  #if DMDLib == 1
    if((millis()- last) > 3)
    {
      ScanDMD();
    }
  #endif
}

//void serialEvent()
//{
//    #if DMDLib == 1
//    dmd.clearScreen(true);
//  
//  #elif DMDLib == 2
//    dmd.end();
//  #endif
  
//}

//ISR(USART_RXC_vect)
//{
//  // Code to be executed when the USART receives a byte here
//  #if DMDLib == 1
//    dmd.clearScreen(true);
//  
//  #elif DMDLib == 2
//    dmd.end();
//  #endif
//}
