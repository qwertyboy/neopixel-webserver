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

//server setup
byte mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x81, 0x8D};
byte ip[] = {192, 168, 2, 13};
int dhcpSuccess = 0;
int retry = 0;
int maxRetries = 4;
char buffer[9]; // additional character for terminating null
EthernetServer server(80);

//html command variables
char cmdBuffer[128];
int color1Cmd = 0;
int color2Cmd = 0;
int colorCombo = 0;
float effectSpeed = 1.0;

//led setup
const int ledsPerStrip = 45;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

//loop counters for timing effects
unsigned long millis1 = 0;
unsigned long millis2 = 0;
unsigned long prevMillis1 = 0;
unsigned long prevMillis2 = 0;
unsigned long millis3 = 0;
unsigned long prevMillis3 = 0;

//effect update intervals
int rainbowInterval = 56;//56
int pulseInterval = 30;
int spinInterval = 10;
int spinInterval2 = 15;
int chaseInterval = 50;
int sparkleInterval = 1300;  //Tune this to get the desired update speed for sparkle effect (1300 default)
int colorInterval = 30000;
int randInterval = 10000;
int fadeInterval = 10;

//effect modes and color arrays
int ledMode = 0;
int lastMode = 0;
int newMode = random(1,10);
int randPos[100];
int rainbowColors[] = {0xff0000, 0xff0800, 0xff1100, 0xff1900, 0xff2200, 0xff2a00, 0xff3300, 0xff3b00, 0xff4400, 0xff4c00, 0xff5500, 0xff5d00, 0xff6600, 0xff6e00, 0xff7700, 0xff7f00,
                       0xff8800, 0xff9000, 0xff9900, 0xffa100, 0xffaa00, 0xffb200, 0xffbb00, 0xffc300, 0xffcc00, 0xffd400, 0xffdd00, 0xffe500, 0xffee00, 0xfff600, 0xffff00, 0xf6ff00,
                       0xeeff00, 0xe5ff00, 0xddff00, 0xd4ff00, 0xccff00, 0xc3ff00, 0xbbff00, 0xb2ff00, 0xaaff00, 0xa1ff00, 0x99ff00, 0x90ff00, 0x88ff00, 0x7fff00, 0x77ff00, 0x6eff00,
                       0x66ff00, 0x5dff00, 0x55ff00, 0x4cff00, 0x44ff00, 0x3bff00, 0x33ff00, 0x2aff00, 0x22ff00, 0x19ff00, 0x11ff00, 0x08ff00, 0x00ff00, 0x00ff08, 0x00ff11, 0x00ff19,
                       0x00ff22, 0x00ff2a, 0x00ff33, 0x00ff3b, 0x00ff44, 0x00ff4c, 0x00ff55, 0x00ff5d, 0x00ff66, 0x00ff6e, 0x00ff77, 0x00ff7f, 0x00ff88, 0x00ff90, 0x00ff99, 0x00ffa1,
                       0x00ffaa, 0x00ffb2, 0x00ffbb, 0x00ffc3, 0x00ffcc, 0x00ffd4, 0x00ffdd, 0x00ffe5, 0x00ffee, 0x00fff6, 0x00ffff, 0x00f6ff, 0x00eeff, 0x00e5ff, 0x00ddff, 0x00d4ff,
                       0x00ccff, 0x00c3ff, 0x00bbff, 0x00b2ff, 0x00aaff, 0x00a1ff, 0x0099ff, 0x0090ff, 0x0088ff, 0x007fff, 0x0077ff, 0x006eff, 0x0066ff, 0x005dff, 0x0055ff, 0x004cff,
                       0x0044ff, 0x003bff, 0x0033ff, 0x002aff, 0x0022ff, 0x0019ff, 0x0011ff, 0x0008ff, 0x0000ff, 0x0800ff, 0x1100ff, 0x1900ff, 0x2200ff, 0x2a00ff, 0x3300ff, 0x3b00ff,
                       0x4400ff, 0x4c00ff, 0x5500ff, 0x5d00ff, 0x6600ff, 0x6e00ff, 0x7700ff, 0x7f00ff, 0x8800ff, 0x9000ff, 0x9900ff, 0xa100ff, 0xaa00ff, 0xb200ff, 0xbb00ff, 0xc300ff,
                       0xcc00ff, 0xd400ff, 0xdd00ff, 0xe500ff, 0xee00ff, 0xf600ff, 0xff00ff, 0xff00f6, 0xff00ee, 0xff00e5, 0xff00dd, 0xff00d4, 0xff00cc, 0xff00c3, 0xff00bb, 0xff00b2,
                       0xff00aa, 0xff00a1, 0xff0099, 0xff0090, 0xff0088, 0xff007f, 0xff0077, 0xff006e, 0xff0066, 0xff005d, 0xff0055, 0xff004c, 0xff0044, 0xff003b, 0xff0033, 0xff002a,
                       0xff0022, 0xff0019, 0xff0011, 0xff0008};
                       
int fadeVals[] =      {0x010101, 0x010101, 0x020202, 0x030303, 0x050505, 0x080808, 0x0b0b0b, 0x0f0f0f, 0x141414, 0x191919, 0x1e1e1e, 0x242424, 0x2b2b2b, 0x313131, 0x383838, 0x404040,
                       0x484848, 0x505050, 0x585858, 0x616161, 0x696969, 0x727272, 0x7b7b7b, 0x848484, 0x8d8d8d, 0x969696, 0x9e9e9e, 0xa7a7a7, 0xafafaf, 0xb7b7b7, 0xbfbfbf, 0xc7c7c7,
                       0xcecece, 0xd4d4d4, 0xdbdbdb, 0xe1e1e1, 0xe6e6e6, 0xebebeb, 0xf0f0f0, 0xf4f4f4, 0xf7f7f7, 0xfafafa, 0xfcfcfc, 0xfdfdfd, 0xfefefe, 0xffffff, 0xfefefe, 0xfdfdfd,
                       0xfcfcfc, 0xfafafa, 0xf7f7f7, 0xf4f4f4, 0xf0f0f0, 0xebebeb, 0xe6e6e6, 0xe1e1e1, 0xdbdbdb, 0xd4d4d4, 0xcecece, 0xc7c7c7, 0xbfbfbf, 0xb7b7b7, 0xafafaf, 0xa7a7a7,
                       0x9e9e9e, 0x969696, 0x8d8d8d, 0x848484, 0x7b7b7b, 0x727272, 0x696969, 0x616161, 0x585858, 0x505050, 0x484848, 0x404040, 0x383838, 0x313131, 0x2b2b2b, 0x242424,
                       0x1e1e1e, 0x191919, 0x141414, 0x0f0f0f, 0x0b0b0b, 0x080808, 0x050505, 0x030303, 0x020202, 0x010101, 0x000000};

int gammaCurve[] =    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
                       0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09,
                       0x09, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14,
                       0x15, 0x15, 0x16, 0x16, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1a, 0x1b, 0x1b, 0x1c, 0x1d, 0x1d, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x23, 0x23, 0x24, 0x25, 0x26,
                       0x27, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
                       0x40, 0x42, 0x43, 0x44, 0x45, 0x46, 0x48, 0x49, 0x4a, 0x4b, 0x4d, 0x4e, 0x4f, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57, 0x59, 0x5a, 0x5c, 0x5d, 0x5f, 0x60, 0x62,
                       0x63, 0x65, 0x66, 0x68, 0x69, 0x6b, 0x6d, 0x6e, 0x70, 0x72, 0x73, 0x75, 0x77, 0x78, 0x7a, 0x7c, 0x7e, 0x7f, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8a, 0x8c, 0x8e,
                       0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa7, 0xa9, 0xab, 0xad, 0xaf, 0xb1, 0xb4, 0xb6, 0xb8, 0xba, 0xbd, 0xbf, 0xc1, 0xc4, 0xc6,
                       0xc8, 0xcb, 0xcd, 0xd0, 0xd2, 0xd5, 0xd7, 0xda, 0xdc, 0xdf, 0xe1, 0xe4, 0xe7, 0xe9, 0xec, 0xef, 0xf1, 0xf4, 0xf7, 0xf9, 0xfc, 0xff};
                       
float randSpeeds[] =  {0.03, 0.09, 0.11, 0.11, 0.13, 0.16, 0.18, 0.22, 0.24, 0.25, 0.26, 0.27, 0.27, 0.30, 0.31, 0.32, 0.34, 0.37, 0.42, 0.43, 0.44, 0.45, 0.45, 0.45, 0.45, 0.48,
                       0.49, 0.50, 0.51, 0.51, 0.52, 0.52, 0.53, 0.53, 0.53, 0.53, 0.56, 0.57, 0.59, 0.61, 0.61, 0.61, 0.61, 0.61, 0.62, 0.63, 0.64, 0.64, 0.65, 0.66, 0.67, 0.67,
                       0.68, 0.69, 0.69, 0.70, 0.71, 0.71, 0.72, 0.73, 0.73, 0.74, 0.76, 0.77, 0.77, 0.78, 0.78, 0.79, 0.79, 0.79, 0.80, 0.80, 0.81, 0.81, 0.82, 0.82, 0.82, 0.83,
                       0.84, 0.84, 0.85, 0.85, 0.85, 0.88, 0.88, 0.88, 0.88, 0.88, 0.89, 0.89, 0.90, 0.90, 0.91, 0.91, 0.92, 0.92, 0.93, 0.94, 0.95, 0.95, 0.96, 0.97, 0.99, 0.99,
                       0.99, 1.01, 1.01, 1.01, 1.03, 1.03, 1.03, 1.04, 1.04, 1.05, 1.05, 1.06, 1.06, 1.06, 1.07, 1.08, 1.08, 1.09, 1.09, 1.09, 1.10, 1.11, 1.11, 1.11, 1.12, 1.12,
                       1.13, 1.13, 1.13, 1.14, 1.14, 1.15, 1.15, 1.16, 1.16, 1.17, 1.18, 1.18, 1.19, 1.19, 1.20, 1.21, 1.21, 1.21, 1.22, 1.23, 1.23, 1.24, 1.26, 1.27, 1.27, 1.28,
                       1.28, 1.29, 1.32, 1.33, 1.35, 1.36, 1.36, 1.37, 1.38, 1.39, 1.39, 1.42, 1.42, 1.43, 1.43, 1.43, 1.45, 1.46, 1.47, 1.47, 1.48, 1.50, 1.53, 1.53, 1.55, 1.55,
                       1.56, 1.58, 1.59, 1.59, 1.65, 1.65, 1.70, 1.71, 1.71, 1.71, 1.73, 1.74, 1.83, 1.87, 1.89, 1.90, 1.93, 1.98};

void setup()
{
  Serial.begin(57600);
  leds.begin();
  leds.show();
  
  pinMode(0, OUTPUT);
  
  delay(2000);
  
  //generate random starting positions
  int randSeed = analogRead(3);
  randomSeed(randSeed);
  Serial.printf("Random seed: %d\n", randSeed);
  for(int i = 0; i < 100; i++){
    randPos[i] = random(333);
  }
  
  //get ip address and start server
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
  millis3 = millis();
  
  //handle http request
  EthernetClient client = server.available();
  if(client){
    while(client.connected()){
      if(client.available()){
        memset(buffer, 0, 9);
        
        if(client.readBytesUntil('/', buffer, 9)){
          Serial.printf("\n\nbuffer:\t\t%s\n", buffer);
          
          //if response is of type POST
          if(strcmp(buffer,"POST ") == 0){
            client.find("\n\r");
            
            while(client.findUntil("mode=", "\n\r")){
              //clear the buffers and command strings
              memset(cmdBuffer, 0, 128);
              char cmd[32] = "";
              char * sptr;
              
              //read commands
              byte bytesRead = client.readBytesUntil('\n\r', cmdBuffer, 128);
              
              //parse the commands
              //get led mode and check if valid
              strncpy(cmd, cmdBuffer, 2);
              if(!isdigit(cmd[0]) || !isdigit(cmd[1])){
                newMode = -1;
              }else{
                newMode = strtol(cmd, NULL, 10);
              }
              memset(cmd, 0, 32);
              ledMode = newMode;
              lastMode = -1;
              
              //get first custom color
              sptr = strstr(cmdBuffer, "customColor=\%23");
              strncpy(cmd, sptr + 15, 6);
              color1Cmd = strtol(cmd, NULL, 16);
              memset(cmd, 0, 32);
              
              //get second custom color
              sptr = strstr(cmdBuffer, "customColor2=\%23");
              strncpy(cmd, sptr + 16, 6);
              color2Cmd = strtol(cmd, NULL, 16);
              memset(cmd, 0, 32);
              
              //get effect speed
              sptr = strstr(cmdBuffer, "speed=");
              strncpy(cmd, sptr + 6, 4);
              effectSpeed = strtof(cmd, NULL);
              
              colorCombo = gammaCorrect(addColors(color1Cmd, color2Cmd));
              color1Cmd = gammaCorrect(color1Cmd);
              color2Cmd = gammaCorrect(color2Cmd);
              
              Serial.printf("bytesRead:\t%d\n", bytesRead);
              Serial.printf("cmdBuffer:\t%s\n", cmdBuffer);
              Serial.printf("ledMode:\t%d\n", ledMode);
              Serial.printf("effectSpeed:\t%.2f\n", effectSpeed);
              Serial.printf("color1Cmd:\t%x\n", color1Cmd);
              Serial.printf("color2cmd:\t%x\n", color2Cmd);
              Serial.printf("colorCombo:\t%x\n", colorCombo);
            }
          }
          
          //send html for control page
          sendHeader(client,"LED Control Panel");

          client.println("<div align='left'>");
          client.println("<h3>Select a mode:</h3>");
          client.println("<form action='/' method='POST'>");
          client.println("<select name='mode'>");
          client.println("<option value=01>Static Color (Requires color selection)</option>");
          client.println("<option value=02>Slow Rainbow</option>");
          client.println("<option value=03>Red Alert! (Requires color selection)</option>");
          client.println("<option value=04>Spin (Requires color selection)</option>");
          client.println("<option value=05>Spin Bounce (Requires color selection)</option>");
          client.println("<option value=06>Double Spin (Requires 2 color selections)</option>");
          client.println("<option value=11>Rainbow Spin</option>");
          client.println("<option value=07>Theater Chase (Requires color selection)</option>");
          client.println("<option value=08>Rainbow Theater Chase</option>");
          client.println("<option value=09>Thparkle (Requires color selection)</option>");
          client.println("<option value=10>Random Thparkle</option>");
          client.println("<option value=12>Random Animations</option>");
          client.println("<option value=12>Weather Clock</option>");
          client.println("<option value=00>Off</option>");
          client.println("</select><br>");
          client.println("Color 1: <input type='color' name='customColor'>");
          client.println("Color 2: <input type='color' name='customColor2'><br>");
          client.println("Speed (Precentage): <input type='range' name='speed' min=0.01 max=1.99 step=0.01>&nbsp;&nbsp;&nbsp;&nbsp;<input type='submit'>");
          client.println("</form>");
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
    static int cleared;
    
    //clear leds if not all the same or changing modes
    for(int i = 0; i < 334; i++){
      if(leds.getPixel(i) != color1Cmd){
        cleared = 0;
      }
    }
  
    if(!cleared || (lastMode != 1 && lastMode != 12)){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
    
      leds.show();
      cleared = 1;
      Serial.println("Cleared LEDs");
    }
    
    //set all leds to the new color
    for(int i = 0; i < 334; i++){
      leds.setPixel(i, color1Cmd);
    }
    
    leds.show();
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 1;
    }
  }
  
  //Rainbow Mode
  else if(ledMode == 2){
    static int color;
    
    //clear leds if coming from different mode
    if(lastMode != 2 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
    
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    if(millis1 - prevMillis1 > rainbowInterval / effectSpeed){
      prevMillis1 = millis1;
      
      //every update, get a new value from the table  
      for(int x = 0; x < 334; x++){
        int index = (color + x) % 180;
        leds.setPixel(x, gammaCorrect(rainbowColors[index]));
      }
      
      if(color < 178){color++;}else{color = 0;}
    }
    
    leds.show();
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 2;
    }
  }
  
  //Pulse
  else if(ledMode == 3){
    static int index;
    
    //clear leds if coming from different mode
    if(lastMode != 3 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    //get the component of the color
    int blue = color1Cmd & 0x0000FF;
    int green = (color1Cmd & 0x00FF00) >> 8;
    int red = (color1Cmd & 0xFF0000) >> 16;
    
    int brightness = 0;
    //get a random brightness and pass it to the color component
    red = (fadeVals[index] & 0xFF) * red / 0xFF;
    green = (fadeVals[index] & 0xFF) * green / 0xFF;
    blue = (fadeVals[index] & 0xFF) * blue / 0xFF;
    brightness = red << 16 | green << 8 | blue;
    
    if(millis1 - prevMillis1 > pulseInterval / effectSpeed){
      prevMillis1 = millis1;
      
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, brightness);
      }
      
      leds.show();
      leds.show();
      
      if(index < 90){index++;}else{index = 0;}
    }
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 3;
    }
  }
  
  //Spin
  else if(ledMode == 4){
    static int pos = 0;
    
    //clear leds if coming from different mode
    if(lastMode != 4 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    for(int i = 0; i < 30; i++){
      if((pos + i) > 333){
        leds.setPixel((pos + i) - 334, color1Cmd);
      }
      leds.setPixel(pos + i, color1Cmd);
    }
    
    if(millis1 - prevMillis1 > spinInterval / effectSpeed){
      prevMillis1 = millis1;
      
      leds.setPixel(pos - 1, 0);
      leds.show();
      
      if(pos < 334){pos++;}else{pos = 0;}
    }
    
    leds.show();
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 4;
    }
  }
  
  //Spin Bounce
  else if(ledMode == 5){
    static int dir, pos;
    
    if(lastMode != 5 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    if(dir == 0){
      for(int i = 0; i < 30; i++){
        leds.setPixel(pos + i, color1Cmd);
      }
      
      leds.show();
      
      if(millis1 - prevMillis1 > spinInterval / effectSpeed){
        prevMillis1 = millis1;
      
        leds.setPixel(pos - 1, 0);
        leds.show();
      
        if(pos <= 303){pos++;}else{pos = 333; dir = !dir;}
      }
    }else{
      for(int i = 0; i < 30; i++){
        leds.setPixel(pos - i, color1Cmd);
      }
      
      leds.show();
      
      if(millis1 - prevMillis1 > spinInterval / effectSpeed){
        prevMillis1 = millis1;
      
        if(pos > 29){pos--;}else{pos = 0; dir = !dir;}
      
        leds.setPixel(pos + 1, 0);
        leds.show();
      }
    }
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 5;
    }
  }
  
  //Double Spin
  else if(ledMode == 6){
    static int pos1 = randPos[random(100)];
    static int pos2 = randPos[random(100)];
    static int segment1[30], segment2[30];
    
    //clear leds if coming from different mode
    if(lastMode != 6 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    //calculate the position of each led and its respecitve segment
    for(int i = 0; i < 30; i++){
      //if the position is greater than end, start at beginning
      if((pos1 + i) > 333){
        segment1[i] = (pos1 + i) - 334;
      }else{
        segment1[i] = pos1 + i;
      }
      
      //if the position is less than the beginning, start at the end
      if((pos2 - i) < 0){
        segment2[i] = (pos2 - i) + 334;
      }else{
        segment2[i] = pos2 - i;
      }
      
      leds.setPixel(segment1[i], color1Cmd);
      leds.setPixel(segment2[i], color2Cmd);
    }
    
    //look for overlap
    for(int i = 0; i < 30; i++){
      for(int j = 0; j < 30; j++){
        if(segment1[29-i] == segment2[29-j]){
          leds.setPixel(segment1[29-i], colorCombo);
          leds.setPixel(segment2[29-i], colorCombo);
        }
      }
    }
    
    leds.show();
    
    //update segment 1
    if(millis1 - prevMillis1 > spinInterval / effectSpeed){
      prevMillis1 = millis1;
      
      leds.setPixel(segment1[0], 0);
      leds.show();
      
      if(pos1 < 333){pos1++;}else{pos1 = 0;}
    }
    
    //update segment 2
    if(millis2 - prevMillis2 > spinInterval2 / effectSpeed){
      prevMillis2 = millis2;
      
      leds.setPixel(segment2[0], 0);
      leds.show();
      
      if(pos2 > 0){pos2--;}else{pos2 = 333;}
    }

    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 6;
    }
  }
  
  //Theater Chase
  else if(ledMode == 7){
    static int j;
    
    //clear leds if changing modes
    if(lastMode != 7 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    if(millis1 - prevMillis1 > chaseInterval / effectSpeed){
      prevMillis1 = millis1;
      
      //set every third pixel
      for(int i=0; i < 334; i=i+3){
        leds.setPixel(i+j, color1Cmd);
      }
        
      leds.show();
      
      //clear every third pixel
      for(int i=0; i < 334; i=i+3){
        leds.setPixel(i+j, 0);
      }
      
      if(j < 2){j++;}else{j = 0;}
    }
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 7;
    }
  }
  
  //Rainbow Theater Chase
  else if(ledMode == 8){
    static int color, j;
    
    //clear leds if changing modes
    if(lastMode != 8 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    if(millis1 - prevMillis1 > chaseInterval / effectSpeed){
      prevMillis1 = millis1;
      
      //set every third pixel with a rainbow color
      for(int i = 0; i < 334; i = i + 3){
        int index = (color + i) % 180;
        leds.setPixel(i + j, gammaCorrect(rainbowColors[index]));
      }
        
      leds.show();
      
      //clear every third pixel
      for(int i = 0; i < 334; i = i + 3){
        leds.setPixel(i + j, 0);
      }
        
      if(color < 178){color++;}else{color = 0;}
      if(j < 2){j++;}else{j = 0;}
    }
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 8;
    }
  }
  
  //Sparkle
  else if(ledMode == 9){
    //clear leds if changing modes
    if(lastMode != 9 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    //get the component of the color
    int blueOct = color1Cmd & 0x0000FF;
    int greenOct = (color1Cmd & 0x00FF00) >> 8;
    int redOct = (color1Cmd & 0xFF0000) >> 16;
    
    for(int i = 0; i < 334; i++){
      int brightness = 0;
      int rand = random(0xFF);
      rand = (rand * rand) / 0xFF;
      
      //get a random brightness and pass it to the color component
      brightness += rand * blueOct / 0xFF;
      brightness += (rand * greenOct / 0xFF) << 8;
      brightness += (rand * redOct / 0xFF) << 16;
      
      if(!random(sparkleInterval / effectSpeed)){
        leds.setPixel(i, brightness);
      }
    }
    
    leds.show();
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 9;
    }
  }
  
  //color change sparkle
  else if(ledMode == 10){
    int randColor = random(179);
    
    //clear leds if changing modes
    if(lastMode != 10 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      color1Cmd = gammaCorrect(rainbowColors[randColor]);
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    //get the component of the color
    int blueOct = color1Cmd & 0x0000FF;
    int greenOct = (color1Cmd & 0x00FF00) >> 8;
    int redOct = (color1Cmd & 0xFF0000) >> 16;
    
    for(int i = 0; i < 334; i++){
      int brightness = 0;
      int rand = random(0xFF);
      rand = (rand * rand) / 0xFF;
      
      //get a random brightness and pass it to the color component
      brightness += rand * blueOct / 0xFF;
      brightness += (rand * greenOct / 0xFF) << 8;
      brightness += (rand * redOct / 0xFF) << 16;
      
      if(!random(sparkleInterval / effectSpeed)){
        leds.setPixel(i, brightness);
      }
      
      if(millis1 - prevMillis1 > colorInterval / effectSpeed){
        prevMillis1 = millis1;
        
        color1Cmd = gammaCorrect(rainbowColors[randColor]);
      }
    }
    
    leds.show();
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 10;
    }
  }
  
  //rainbow spin
  else if(ledMode == 11){
    static int pos, color = 0;
    
    //clear leds if coming from different mode
    if(lastMode != 11 && lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    for(int i = 0; i < 30; i++){
      //int index = color % 180;
      
      if((pos + i) > 333){
        leds.setPixel((pos + i) - 334, gammaCorrect(rainbowColors[color]));
      }
      leds.setPixel(pos + i, gammaCorrect(rainbowColors[color]));
    }
    
    if(millis1 - prevMillis1 > spinInterval / effectSpeed){
      prevMillis1 = millis1;
      
      leds.setPixel(pos - 1, 0);
      leds.show();
      
      if(pos < 334){pos++;}else{pos = 0;}
    }
    
    if(millis2 - prevMillis2 > rainbowInterval / effectSpeed){
      prevMillis2 = millis2;
      if(color < 178){color++;}else{color = 0;}
    }
    
    leds.show();
    
    if(lastMode == 12){
      ledMode = 12;
    }else{
      lastMode = 11;
    }
  }
  
  //Random Animations
  else if(ledMode == 12){
    if(millis3 - prevMillis3 > randInterval){
      prevMillis3 = millis3;
      
      newMode = random(1, 11);
      color1Cmd = gammaCorrect(rainbowColors[random(179)]);
      color2Cmd = gammaCorrect(rainbowColors[random(179)]);
      colorCombo = gammaCorrect(addColors(color1Cmd, color2Cmd));
      effectSpeed = randSpeeds[random(199)];
      
      Serial.printf("\nnewMode:\t%d\n", newMode);
      Serial.printf("color1Cmd:\t%x\n", color1Cmd);
      Serial.printf("color2Cmd:\t%x\n", color2Cmd);
      Serial.printf("colorCombo:\t%x\n", colorCombo);
      Serial.printf("effectSpeed:\t%.2f\n", effectSpeed);
      
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0x000000);
      }
      leds.show();
      leds.show();
      
      Serial.println("Cleared LEDs");
    }
    
    ledMode = newMode;
    
    lastMode = 12;
  }
  
  //Weather clock
  else if(ledMode == 12){
    if(lastMode != 12){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    for(int i = 0; i < 334; i++){
      leds.setPixel(i, 0x010101);
    }
    
    leds.show();
    
    lastMode = 12;
  }  
  
  //turn leds off
  else if(ledMode == 0){
    lastMode = 0;
    
    for(int i = 0; i < 334; i++){
      leds.setPixel(i, 0x000000);
    }
    
    leds.show();
  }
  
  //ledMode not valid, so turn off all but last one
  else{
    lastMode = -1;
    
//    for(int i = 0; i < 334; i++){
//      leds.setPixel(i, 0x000000);
//    }
    
    leds.setPixel(333, 0x0F0000);
    leds.show();
  }
}

//send a standard http response header
void sendHeader(EthernetClient client, char *title){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.print("<html><head><title>");
  client.print(title);
  client.println("</title><body background='http://apod.nasa.gov/apod/image/1403/heic1404b1920.jpg'><font color='white'>");
}

//function to add colors
int addColors(int color1, int color2){
  int color1Red = color1 & 0xFF0000;
  int color1Green = color1 & 0x00FF00;
  int color1Blue = color1 & 0x0000FF;
  
  int color2Red = color2 & 0xFF0000;
  int color2Green = color2 & 0x00FF00;
  int color2Blue = color2 & 0x0000FF;
  
  int newRed = (color1Red + color2Red) / 2;
  int newGreen = (color1Green + color2Green) / 2;
  int newBlue = (color1Blue + color2Blue) / 2;
  
  newRed = newRed & 0xFF0000;
  newGreen = newGreen & 0x00FF00;
  newBlue = newBlue & 0x0000FF;
  
  int result = newRed | newGreen | newBlue;
  return result;
}

int gammaCorrect(int color){
  int red = (color & 0xFF0000) >> 16;
  int green = (color & 0x00FF00) >> 8;
  int blue = color & 0x0000FF;
  
  int result = (gammaCurve[red] << 16) | (gammaCurve[green] << 8) | gammaCurve[blue];
  return result;
}

//int fadeOn(int led, int color){
//  static int index = 0;
//  //get the component of the color
//  int blue = color & 0x0000FF;
//  int green = (color & 0x00FF00) >> 8;
//  int red = (color & 0xFF0000) >> 16;
//  
//  int brightness = 0;
//  //get a random brightness and pass it to the color component
//  red = (fadeVals[index] & 0xFF) * red / 0xFF;
//  green = (fadeVals[index] & 0xFF) * green / 0xFF;
//  blue = (fadeVals[index] & 0xFF) * blue / 0xFF;
//  brightness = red << 16 | green << 8 | blue;
//  
//  if(millis2 - prevMillis2 > fadeInterval / effectSpeed){
//    prevMillis2 = millis2;
//    
//    leds.setPixel(led, brightness);
//    leds.show();
//    
//    if(index < 45){index++;}else{index = 0;}
//  }
//}
//
//int fadeOff(int color){
//  static int index = 45;
//  //get the component of the color
//  int blue = color & 0x0000FF;
//  int green = (color & 0x00FF00) >> 8;
//  int red = (color & 0xFF0000) >> 16;
//  
//  int brightness = 0;
//  //get a random brightness and pass it to the color component
//  red = (fadeVals[index] & 0xFF) * red / 0xFF;
//  green = (fadeVals[index] & 0xFF) * green / 0xFF;
//  blue = (fadeVals[index] & 0xFF) * blue / 0xFF;
//  brightness = red << 16 | green << 8 | blue;
//  
////  if(millis2 - prevMillis2 > fadeInterval / effectSpeed){
////    prevMillis2 = millis2;
////    
////    leds.setPixel(led, brightness);
////    leds.show();
////    
//    if(index < 90){index++;}else{index = 45;}
////  }
//  return brightness;
//}
