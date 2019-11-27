#include <Artnet.h>
#include <Preferences.h>
#include "FastLED.h"
//#include "SPIFFS.h"

#include "esp_wifi.h" // needed for esp_wifi_set_ps ( power saving mode )

#include <WiFi.h>
#include <WiFiMulti.h>

//#include "ESPAsyncWebServer.h"
#include <WebServer.h>
#include <HTTPClient.h>

//#include <WiFi.h>      //ESP32 Core WiFi Library    
//#include <DNSServer.h> //Local WebServer used to serve the configuration portal (  https://github.com/zhouhan0126/DNSServer---esp32  )
//#include <WiFiManager.h>   // WiFi Configuration Magic (  https://github.com/zhouhan0126/DNSServer---esp32  ) >>  https://github.com/zhouhan0126/DNSServer---esp32  (ORIGINAL)


Preferences preferences;

unsigned int pref_led_first;
unsigned int pref_led_last;
unsigned int pref_config_mode;


#define FASTLED_INTERRUPT_RETRY_COUNT 3

#define FRAMES_PER_SECOND 60
#define LED_DATA_PIN 18 //D1

unsigned int pref_num_leds;

CRGB leds[256];

unsigned long last_LED_millis;
unsigned long millis_per_refresh;

unsigned long last_chase_millis;
unsigned long millis_per_chase;

unsigned long last_http_millis = 0;


CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

int brightness = 0;
int speed = 255;
int mode = 0;
uint8_t* dmx_data;
int debug_led;

uint8_t startIndex;

ArtnetReceiver artnet;
uint32_t pref_artnet_universe = 1;
uint8_t pref_artnet_startchannel = 1;

String pref_wifi_SSID;
String pref_wifi_Pass;

unsigned int ButtonStateMilli[] = {0,0};
boolean perform_wipe_wifi = false;

char ipaddress[16];


/* Sine Wave */
float x=0;
float y=0;
const float pi=3.14159;
int z=0;
float v=0;
int w=0;



unsigned int DMXChannel_Mode;
unsigned int DMXChannel_Brightness;

unsigned int DMXChannel_RGB_R;
unsigned int DMXChannel_RGB_G;
unsigned int DMXChannel_RGB_B;
unsigned int DMXChannel_RGB_StrobeSpeedA;
unsigned int DMXChannel_RGB_StrobeSpeedB;

unsigned int DMXChannel_Palet_Palet;
unsigned int DMXChannel_Palet_Speed;

unsigned int DMXChannel_Glitter_Count;
unsigned int DMXChannel_Glitter_Speed;
unsigned int DMXChannel_Glitter_R;
unsigned int DMXChannel_Glitter_G;
unsigned int DMXChannel_Glitter_B;

#define Mode_Channel_Value_RGB     0
#define Mode_Channel_Value_Pallet  50
#define Mode_Channel_Value_Glitter 100
#define Mode_Channel_Value_Fire    150
#define Mode_Channel_Value_Debug   240
#define Mode_Channel_Value_Max    255

WebServer server(80);

/*
 *  Channel 1 = Mode
 *  Channel 2 = Brightness
 *  
 *  RGB Mode
 *    Channel 3 = Red
 *    Channel 4 = Green
 *    Channel 5 = Blue
 * 
 *  Palet Mode
 *    Channel 3 = Palet Select
 *    Channel 4 = Speed
 *    Channel 5 = NA
 *    
 *  Glitter Mode
 *    Channel 3 = Star Count
 *    Channel 4 = Fade Speed
 *    Channel 5 = Red       \
 *    Channel 6 = Green      --- if all set to 0, then random color
 *    Channel 7 = Blue      /
 *  
 */

void setup () {
  dmx_data = NULL;
  Serial.begin(115200);

  // Load Preferences
  // **********************************************************************
  load_preferences();



  // Initialise SPIFFS
  // **********************************************************************

  
/*  
 *   if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

File file = SPIFFS.open("/index.html");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();  
*/
 
  // Initialise LED's
  // **********************************************************************
  FastLED.addLeds<WS2812, LED_DATA_PIN>(leds, pref_num_leds); 
  config_wifi();

  // Indicate connecting wifi
  sequence_WIFI_Connecting();
  FastLED.show(); 
  WiFi.mode (WIFI_STA);  
  
  // Connect to WIFI
  // **********************************************************************
  Serial.print("Connecting to WIFI ");

  // Disable powersaving mode on ESP32... this provides better ping times
  // https://github.com/espressif/arduino-esp32/issues/1484
  WiFi.setSleep(false);  
  esp_wifi_set_ps (WIFI_PS_NONE);
    
  WiFi.begin( pref_wifi_SSID.c_str(), pref_wifi_Pass.c_str());
  
  while (WiFi.status() != WL_CONNECTED) { 
     Serial.print("."); 
     delay(100); 
  }

  IPAddress ip = WiFi.localIP();
  
  sprintf(ipaddress, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  Serial.printf("\n  Connected; IP = %s\n", ipaddress );


  // Register with central service 
  register_esp32();

  // Start ArtNET
  // **********************************************************************
  sequence_ARTNET_Starting();
  FastLED.show(); 
  Serial.print("Starting ArtNET\n");
  delay(100); 

  set_DMX_channels( pref_artnet_startchannel );

  artnet.begin();
  artnet.subscribe(pref_artnet_universe, artnet_callback);


  // Start WebServer
  // **********************************************************************
  sequence_WebServer_Starting();
  FastLED.show(); 
  Serial.print("Starting Webserver\n");  
  delay(100);
   
  webserver_routine();
  Serial.printf("  Open http://%s in your browser\n", ipaddress);


  // Set BOOT Button to input
  pinMode(0, INPUT);
  attachInterrupt(0, int_buttonpress, CHANGE);



  // Execute the startup sequence
  // **********************************************************************
  sequence_BLACK();
  FastLED.show(); 
  sequence_startup();


  // Initialise timer and set per frame increment
  // **********************************************************************
  last_LED_millis = millis();
  millis_per_refresh = 1000 / FRAMES_PER_SECOND;

  last_chase_millis = millis();

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  startIndex = 0;
  Serial.print("Startup Complete\n");
}


void loop() {

  // Only run the webserver and wifi reset conditions for the first 120sec of operation..
  // after that, try and give more time to ArtNET and LED painting
//  if ( millis() <= last_http_millis + (60000 * 120) ) {
  
    // Only check webserver ... every 1 second ... 
    if ( millis() % 10 == 0 ) {
      //Serial.printf("%d WebServer Check\n", millis() );
      server.handleClient(); // check for webserver packet
    }
  
    if ( perform_wipe_wifi == true ) {
      wipe_wifi();  
    }
//  }

  artnet.parse(); // check for artnet packet 


  //int sine = sine_wave();
  //Serial.println(sine);


    
    if ( pref_config_mode == 1 ) {
      FastLED.setBrightness( 255 );
      sequence_config_mode();
    } else { 
        FastLED.setBrightness( brightness );
        if ( dmx_data != NULL ) {
          if ( mode < Mode_Channel_Value_Pallet ) {
            sequende_RGB( dmx_data[DMXChannel_RGB_B], dmx_data[DMXChannel_RGB_R], dmx_data[DMXChannel_RGB_G], dmx_data[DMXChannel_RGB_StrobeSpeedA], dmx_data[DMXChannel_RGB_StrobeSpeedB] ); 
          } else if ( mode > Mode_Channel_Value_Pallet && mode < Mode_Channel_Value_Glitter ) { 
            sequence_pallet( dmx_data[DMXChannel_Palet_Palet], dmx_data[DMXChannel_Palet_Speed] );
          } else if ( mode > Mode_Channel_Value_Glitter && mode < Mode_Channel_Value_Fire ) { 
            sequence_glitter( dmx_data[DMXChannel_Glitter_Count], dmx_data[DMXChannel_Glitter_Speed], dmx_data[DMXChannel_Glitter_R], dmx_data[DMXChannel_Glitter_G], dmx_data[DMXChannel_Glitter_B] );
          } else if ( mode > Mode_Channel_Value_Fire ) {    
            sequence_fire(dmx_data[DMXChannel_Glitter_Count], dmx_data[DMXChannel_Glitter_Speed], dmx_data[DMXChannel_Glitter_R]);
          }
        }    
    } 

  // If an LED update is due, then output 
  if ( millis() >= (last_LED_millis + millis_per_refresh) ) {
    FastLED.show(); 
    last_LED_millis = millis();
  }
}
// End main loop

void int_buttonpress() {
  int buttonState = digitalRead(0); // PIN 0 = BOOT Button

  ButtonStateMilli[ buttonState ] = millis();
  
  if ( buttonState == 0 ) {

    // If the WIFI isnt connected, and we press the button ... then wipe the wifi config ! 
    if ( WiFi.status() != WL_CONNECTED ) {
      perform_wipe_wifi = true;      
    }

    // TODO : somehow notify the user easily of the allocated IP Address...
    //    Maybe send a broadcast packet with the source IP so it can be seen in wireshark ??
    //    or does ARTNET have a way to notify other artnet apps ??
    Serial.printf("  Open http://%s in your browser\n", ipaddress);
  } else {
    // if we hold the button down for 2 seconds, then wipe wifi preferences
    if ( ButtonStateMilli[1] > (ButtonStateMilli[0]+2000) ) {
      perform_wipe_wifi = true;
    }    
  }
}



void artnet_callback(uint8_t* data, uint16_t size)
{
    //Serial.print("lambda : artnet data \n");

    for (size_t i = 0; i < size; ++i)
    {
        //Serial.print(data[i]); Serial.print(",");
        mode = data[ DMXChannel_Mode ];
        brightness = data[ DMXChannel_Brightness ];

        dmx_data = data;
    }
    //Serial.println();
}

int sine_wave()
{

  if (w==0){
    v=x*pi/180; // making deg in radians
    y=sin(v);   //calculate sine
    z=y*250;    // calculate duty cycle(250 not 255 because will help to turn off transistors)
    delay(100);
    x=x+0.57;// increase the angle
  }

  if (x>90){// we stop to calculate we have the duty cycle for angles smaller than 90deg
    // the other half is symetric
    x=0;
    w==1;
  }


}

void config_wifi(void){
  if ( pref_wifi_SSID == "" || pref_wifi_Pass ==  "" ) {
    Serial.print("Entering WIFI SmartConfig Mode ");
    
    sequence_WIFI_SmartConfig();
    FastLED.show();
     
    WiFi.mode(WIFI_AP_STA);
    /* start SmartConfig */
    WiFi.beginSmartConfig();
  
    /* Wait for SmartConfig packet from mobile */
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }

    /* Wait for WiFi to connect to AP */
    Serial.println("\nAttempting to connect to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    preferences.begin("my-app", false);
    preferences.putString("pref_wifi_SSID", WiFi.SSID() );
    preferences.putString("pref_wifi_Pass", WiFi.psk() );
    preferences.end();
      
    Serial.println("");
    Serial.printf("SmartConfig done, SSID:%s\n", WiFi.SSID());
    
    Serial.printf("Restarting in 2 sec\n");
    delay( 2000 );
    ESP.restart();
  }
}

void wipe_wifi(void) {
  Serial.printf("wipe_wifi. \n");
  
  preferences.begin("my-app", false);
  preferences.putString("pref_wifi_SSID", "" );
  preferences.putString("pref_wifi_Pass", "" );
  preferences.end();
    
  Serial.printf("Wifi Config cleared. \n");
  delay( 2000 );
  ESP.restart();  
}

void set_DMX_channels(unsigned int start ) {
  unsigned int DMXChannel_Start = start -1; // subtract one, because artnet channels actually start at 0 in our structs
  
  DMXChannel_Mode = DMXChannel_Start; 
  DMXChannel_Brightness = DMXChannel_Start+1;

  DMXChannel_RGB_R = DMXChannel_Start+2;
  DMXChannel_RGB_G = DMXChannel_Start+3;
  DMXChannel_RGB_B = DMXChannel_Start+4;
  DMXChannel_RGB_StrobeSpeedA = DMXChannel_Start+5;
  DMXChannel_RGB_StrobeSpeedB = DMXChannel_Start+6;

  DMXChannel_Palet_Palet = DMXChannel_Start+2;
  DMXChannel_Palet_Speed = DMXChannel_Start+3;

  DMXChannel_Glitter_Count = DMXChannel_Start+2;
  DMXChannel_Glitter_Speed = DMXChannel_Start+3;
  DMXChannel_Glitter_R = DMXChannel_Start+4;
  DMXChannel_Glitter_G = DMXChannel_Start+5;
  DMXChannel_Glitter_B = DMXChannel_Start+6;
}

void register_esp32() {
  HTTPClient http;
  
  char url[2048];
  sprintf(url, "http://180.214.92.162/esp32.php?register=true&mac=%s&ip=%s&universe=%d&start=%d", 
  WiFi.macAddress().c_str(), ipaddress, pref_artnet_universe, pref_artnet_startchannel );
  
  http.begin( url );
  int httpCode = http.GET();                                       

  if (httpCode > 0) { //Check for the returning code
      String payload = http.getString();
      Serial.println("Visit http://180.214.92.162/esp32.php to see all devices");
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end(); //Free the resources  
}

void load_preferences(){
  preferences.begin("my-app", true);
    pref_wifi_SSID  = preferences.getString("pref_wifi_SSID", "" );
    pref_wifi_Pass  = preferences.getString("pref_wifi_Pass", "" );
  
    // Load from preferences and convert to required type 
    pref_num_leds     = preferences.getUInt("pref_num_leds", 256 );
    pref_led_first    = preferences.getUInt("pref_led_first", 0 );
    pref_led_last     = preferences.getUInt("pref_led_last", 50 );
    pref_config_mode  = preferences.getUInt("pref_config_mode", 0 );
  
    pref_artnet_universe     = preferences.getUInt("pref_universe", 0 );
    pref_artnet_startchannel = preferences.getUInt("pref_startchan", 1 ); 
  preferences.end();

  Serial.printf("PREFERENCES SET ********\n");

  Serial.printf("  pref_wifi_SSID:    %s\n", pref_wifi_SSID);
  Serial.printf("  pref_wifi_Pass:    %s\n", (pref_wifi_Pass!="")?">>Set<<":"");
  
  Serial.printf("  pref_config_mode: %d\n", pref_config_mode);
  
  Serial.printf("  pref_num_leds:    %d\n", pref_num_leds);
  Serial.printf("  pref_led_first:   %d\n", pref_led_first);
  Serial.printf("  pref_led_last:    %d\n", pref_led_last);

  Serial.printf("  pref_artnet_universe:        %d\n", pref_artnet_universe);
  Serial.printf("  pref_artnet_startchannel:    %d\n", pref_artnet_startchannel);
  Serial.println("");
}
