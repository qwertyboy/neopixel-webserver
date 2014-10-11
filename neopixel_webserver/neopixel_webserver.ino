/*
  -------------------- Network-Controlled Neopixel Driver --------------------
  
  This program is intended to be run on a Teensy 3.1 connected to an Arduino
  ethernet shield. Slight modification may be needed for other ehternet
  adapters.
  
  This program hosts a webserver that can be used to control Adafruit Neopixel
  led strips.
  
  ---------------------------- Nathan Duprey 2014 ----------------------------
*/
  

#include <SPI.h>
#include <Ethernet.h>
#include <OctoWS2811.h>

#define Host_name nathan-ledctrl

byte mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x81, 0x8D};
byte ip[] = {192, 168, 2, 13};
int dhcpSuccess = 0;
int retry = 0;
int maxRetries = 4;

const int MAX_PAGENAME_LEN = 8; //max characters in page name 
char buffer[MAX_PAGENAME_LEN+1]; // additional character for terminating null
EthernetServer server(80);

char cmdBuffer[129];
String cmd = "";
String modeCmd = "";
String colorStr = "";
char colorStrBuf[7];
int colorCmd = 0;

const int ledsPerStrip = 45;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

unsigned long millis1 = 0;
unsigned long millis2 = 0;
unsigned long prevMillis1 = 0;
unsigned long prevMillis2 = 0;

int rainbowInterval = 56;
int pulseInterval = 50;
int spinInterval = 10;
int chaseInterval = 50;
long timeout = 15000;

int ledMode = 0;
int rainbowColors[] = {16711680, 16713728, 16716032, 16718080, 16720384, 16722432, 16724736, 16726784, 16729088, 16731136, 16733440,
                       16735488, 16737792, 16739840, 16742144, 16744192, 16746496, 16748544, 16750848, 16752896, 16755200, 16757248,
                       16759552, 16761600, 16763904, 16765952, 16768256, 16770304, 16772608, 16774656, 16776960, 16187136, 15662848,
                       15073024, 14548736, 13958912, 13434624, 12844800, 12320512, 11730688, 11206400, 10616576, 10092288, 9502464,
                       8978176, 8388352, 7864064, 7274240, 6749952, 6160128, 5635840, 5046016, 4521728, 3931904, 3407616, 2817792,
                       2293504, 1703680, 1179392, 589568, 65280, 65288, 65297, 65305, 65314, 65322, 65331, 65339, 65348, 65356,
                       65365, 65373, 65382, 65390, 65399, 65407, 65416, 65424, 65433, 65441, 65450, 65458, 65467, 65475, 65484,
                       65492, 65501, 65509, 65518, 65526, 65535, 63231, 61183, 58879, 56831, 54527, 52479, 50175, 48127, 45823,
                       43775, 41471, 39423, 37119, 35071, 32767, 30719, 28415, 26367, 24063, 22015, 19711, 17663, 15359, 13311,
                       11007, 8959, 6655, 4607, 2303, 255, 524543, 1114367, 1638655, 2228479, 2752767, 3342591, 3866879, 4456703,
                       4980991, 5570815, 6095103, 6684927, 7209215, 7799039, 8323327, 8913151, 9437439, 10027263, 10551551, 11141375,
                       11665663, 12255487, 12779775, 13369599, 13893887, 14483711, 15007999, 15597823, 16122111, 16711935, 16711926,
                       16711918, 16711909, 16711901, 16711892, 16711884, 16711875, 16711867, 16711858, 16711850, 16711841, 16711833,
                       16711824, 16711816, 16711807, 16711799, 16711790, 16711782, 16711773, 16711765, 16711756, 16711748, 16711739,
                       16711731, 16711722, 16711714, 16711705, 16711697, 16711688};
                       
int fadeVals[] = {0x010101, 0x010101, 0x020202, 0x030303, 0x050505, 0x080808, 0x0b0b0b, 0x0f0f0f, 0x141414, 0x191919, 0x1e1e1e,
                  0x242424, 0x2b2b2b, 0x313131, 0x383838, 0x404040, 0x484848, 0x505050, 0x585858, 0x616161, 0x696969, 0x727272,
                  0x7b7b7b, 0x848484, 0x8d8d8d, 0x969696, 0x9e9e9e, 0xa7a7a7, 0xafafaf, 0xb7b7b7, 0xbfbfbf, 0xc7c7c7, 0xcecece,
                  0xd4d4d4, 0xdbdbdb, 0xe1e1e1, 0xe6e6e6, 0xebebeb, 0xf0f0f0, 0xf4f4f4, 0xf7f7f7, 0xfafafa, 0xfcfcfc, 0xfdfdfd,
                  0xfefefe, 0xffffff, 0xfefefe, 0xfdfdfd, 0xfcfcfc, 0xfafafa, 0xf7f7f7, 0xf4f4f4, 0xf0f0f0, 0xebebeb, 0xe6e6e6,
                  0xe1e1e1, 0xdbdbdb, 0xd4d4d4, 0xcecece, 0xc7c7c7, 0xbfbfbf, 0xb7b7b7, 0xafafaf, 0xa7a7a7, 0x9e9e9e, 0x969696,
                  0x8d8d8d, 0x848484, 0x7b7b7b, 0x727272, 0x696969, 0x616161, 0x585858, 0x505050, 0x484848, 0x404040, 0x383838,
                  0x313131, 0x2b2b2b, 0x242424, 0x1e1e1e, 0x191919, 0x141414, 0x0f0f0f, 0x0b0b0b, 0x080808, 0x050505, 0x030303,
                  0x020202, 0x010101, 0x000000};

void setup()
{
  Serial.begin(57600);
  leds.begin();
  leds.show();
  
  delay(2000);
  
  dhcpSuccess = Ethernet.begin(mac);
  while(dhcpSuccess == 0){
    retry += 1;
    if(retry > maxRetries){
      Serial.println("Maximum retries exceeded, system halting!");
      while(1){}
    }
    
    Serial.println("DHCP configuration failed! Trying again in 15 seconds.");
    dhcpSuccess = Ethernet.begin(mac);
  }
  
  Serial.print("DHCP IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  
  server.begin();
}

void loop()
{
  millis1 = millis();
  millis2 = millis();
  
  EthernetClient client = server.available();
  if(client){
    while (client.connected()){
      if (client.available()){
        memset(buffer, 0, 9);
        
        if(client.readBytesUntil('/', buffer, 9)){
          Serial.println(buffer);
          
          if(strcmp(buffer,"POST ") == 0){
            client.find("\n\r");
            
            while(client.findUntil("mode=", "\n\r")){
              memset(cmdBuffer, 0, 129);
              memset(colorStrBuf, 0, 7);
              cmd = "";
              modeCmd = "";
              colorStr = "";
              colorCmd = 0;
              
              byte bytesRead = client.readBytesUntil('\n\r', cmdBuffer, 129);
              
              for(int i = 0; i < bytesRead; i++){
                cmd = cmd + cmdBuffer[i];
              }
              
              modeCmd = cmd.substring(0, cmd.indexOf('&'));
              colorStr = cmd.substring(cmd.indexOf('%23') + 1);
              colorStr.toCharArray(colorStrBuf, 7);
              colorCmd = strtol(colorStrBuf, NULL, 16);
              
              Serial.println(bytesRead);
              //Serial.println(cmdBuffer);
              Serial.println(modeCmd);
              Serial.println(colorCmd);
              
              if(modeCmd == "Static+Color"){
                ledMode = 1;
              }else if(modeCmd == "Slow+Rainbow"){
                ledMode = 2;
              }else if(modeCmd == "Pulse"){
                ledMode = 3;
              }else if(modeCmd == "Spin"){
                ledMode = 4;
              }else if(modeCmd == "Spin+Bounce"){
                ledMode = 5;
              }else if(modeCmd == "Theater+Chase"){
                ledMode = 6;
              }else if(modeCmd == "Rainbow+Theater+Chase"){
                ledMode = 7;
              }else if(modeCmd == "Random+Animations"){
                ledMode = 8;
              }else if(modeCmd == "Weather+Clock"){
                ledMode = 9;
              }else if(modeCmd == "Off"){
                ledMode = 0;
              }else{
                ledMode = -1;
              }
              
              Serial.println(ledMode);
            }
          }
          sendHeader(client,"LED Control Panel");

          client.println("<div align='left'>");
          client.println("<h2>Select a mode:</h2>");
          client.println("Static Color, Spin, Spin Bounce, and Theater Chase require a color selection.");
          client.println("<p><form action='/' method='POST'>");
          client.println("<input list='modes' name='mode'>");
          client.println("<datalist id='modes'>");
          client.println("<option value='Static Color'>");
          client.println("<option value='Slow Rainbow'>");
          client.println("<option value='Pulse'>");
          client.println("<option value='Spin'>");
          client.println("<option value='Spin Bounce'>");
          client.println("<option value='Theater Chase'>");
          client.println("<option value='Rainbow Theater Chase'>");
          client.println("<option value='Random Animations'>");
          client.println("<option value='Weather Clock'>");
          client.println("<option value='Off'>");
          client.println("</datalist><br>");
          client.println("Pick a color: <input type='color' name='customColor'>");
          client.println("<input type='submit'>");
          client.println("</form></p>");
          client.println("</div>");
          client.println("<div align='center'>");
          client.println("<h2>Live Camera</h2>");
          client.println("<img src='http://nathanspi.no-ip.org:8082'></img>");
          client.println("</div>");
          client.println("</body>");
          client.println("</html>");
          
          client.stop();
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
  
  //-------------------------------------------------- Effects --------------------------------------------------//

  //Static Color
  if(ledMode == 1){
    for(int i = 0; i < (leds.numPixels() - 26); i++){
      leds.setPixel(i, colorCmd);
    }
    
    leds.show();
    leds.show();
  }
  
  //Rainbow Mode
  else if(ledMode == 2){
    static int color;
    
    if(millis1 - prevMillis1 > rainbowInterval){
      prevMillis1 = millis1;
        
      for(int x = 0; x < (leds.numPixels() - 26); x++){
        int index = (color + x) % 180;
        leds.setPixel(x, rainbowColors[index]);
      }
        
      if(color < 178){color++;}else{color = 0;}
    }
    
    leds.show();
    while(leds.busy()){
    }
  }
  
  //Pulse
  else if(ledMode == 3){
    static int i;
    
    if(millis1 - prevMillis1 > pulseInterval){
      prevMillis1 = millis1;
      
      for(int j = 0; j < (leds.numPixels() - 26); j++){
        leds.setPixel(j, fadeVals[i]);
      }
      
      if(i < 90){i++;}else{i = 0;}
    }
    
    leds.show();
    while(leds.busy()){
    }
  }
  
  //Spin
  else if(ledMode == 4){
    static int i;
    
    for(int j = 0; j < 30; j++){
        if((i + j) > 333){
          leds.setPixel(((334 - (i+j)) * -1), colorCmd);
        }
        leds.setPixel(i + j, colorCmd);
      }
      
    leds.show();
      
    if(millis1 - prevMillis1 > spinInterval){
      prevMillis1 = millis1;
      
      leds.setPixel(i - 1, 0);
      leds.show();
      
      if(i < 334){i++;}else{i = 0;}
    }
  }
  
  //Spin Bounce
  else if(ledMode == 5){
    static int dir, i;
    
    if(dir == 0){
      for(int j = 0; j < 30; j++){
        leds.setPixel(i + j, colorCmd);
      }
      
      leds.show();
      
      if(millis1 - prevMillis1 > spinInterval){
        prevMillis1 = millis1;
      
        leds.setPixel(i - 1, 0);
        leds.show();
      
        if(i <= 303){i++;}else{i = 333; dir = !dir;}
      }
    }else{
      for(int j = 0; j < 30; j++){
        leds.setPixel(i - j, colorCmd);
      }
      
      leds.show();
      
      if(millis1 - prevMillis1 > spinInterval){
        prevMillis1 = millis1;
      
        if(i > 29){i--;}else{i = 0; dir = !dir;}
      
        leds.setPixel(i + 1, 0);
        leds.show();
      }
    }
  }
  
  //Theater Chase
  else if(ledMode == 6){
    static int j;
    
    if(millis1 - prevMillis1 > chaseInterval){
      prevMillis1 = millis1;
      
      for(int i=0; i < (leds.numPixels() - 26); i=i+3){
        leds.setPixel(i+j, colorCmd);
      }
        
      leds.show();
      leds.show();
     
      for(int i=0; i < (leds.numPixels() - 26); i=i+3){
        leds.setPixel(i+j, 0);
      }
      
      if(j < 2){j++;}else{j = 0;}
    }
  }
  
  //Rainbow Theater Chase
  else if(ledMode == 7){
    static int color, j;
    
    if(millis1 - prevMillis1 > chaseInterval){
      prevMillis1 = millis1;
      
      for(int i=0; i < (leds.numPixels() - 26); i=i+3){
        int index = (color + i) % 180;
        leds.setPixel(i+j, rainbowColors[index]);
      }
        
      leds.show();
     
      for(int i=0; i < (leds.numPixels() - 26); i=i+3){
        leds.setPixel(i+j, 0);
      }
        
      if(color < 178){color++;}else{color = 0;}
      if(j < 2){j++;}else{j = 0;}
    }
  }
  
  //Random Animations
  else if(ledMode == 8){
    for(int i = 0; i < (leds.numPixels() - 26); i++){
      leds.setPixel(i, 0x333333);
    }
    
    leds.show();
  }
  
  //Weather clock
  else if(ledMode == 9){
    for(int i = 0; i < (leds.numPixels() - 26); i++){
      leds.setPixel(i, 0x010101);
    }
    
    leds.show();
    while(leds.busy()){
    }
  }
  
  //ledMode not valid or zero, so turn off
  else{
    for(int i = 0; i < (leds.numPixels() - 26); i++){
      leds.setPixel(i, 0x000000);
    }
    
    leds.setPixel(333, 0x010101);
    leds.show();
  }
}

void sendHeader(EthernetClient client, char *title){
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.print("<html><head><title>");
  client.print(title);
  client.println("</title><body>");
}   
