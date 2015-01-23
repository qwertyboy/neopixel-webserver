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

//effect update intervals
int rainbowInterval = 56;
int pulseInterval = 30;
int spinInterval = 10;
int spinInterval2 = 15;
int chaseInterval = 50;
int sparkleInterval = 1300;  //Tune this to get the desired update speed for sparkle effect (1300 default)
int colorInterval = 30000;

//effect modes and color arrays
int ledMode = 0;
int lastMode = 0;
int randPos[100];
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
                       
int fadeVals[] =      {0x010101, 0x010101, 0x020202, 0x030303, 0x050505, 0x080808, 0x0b0b0b, 0x0f0f0f, 0x141414, 0x191919, 0x1e1e1e,
                       0x242424, 0x2b2b2b, 0x313131, 0x383838, 0x404040, 0x484848, 0x505050, 0x585858, 0x616161, 0x696969, 0x727272,
                       0x7b7b7b, 0x848484, 0x8d8d8d, 0x969696, 0x9e9e9e, 0xa7a7a7, 0xafafaf, 0xb7b7b7, 0xbfbfbf, 0xc7c7c7, 0xcecece,
                       0xd4d4d4, 0xdbdbdb, 0xe1e1e1, 0xe6e6e6, 0xebebeb, 0xf0f0f0, 0xf4f4f4, 0xf7f7f7, 0xfafafa, 0xfcfcfc, 0xfdfdfd,
                       0xfefefe, 0xffffff, 0xfefefe, 0xfdfdfd, 0xfcfcfc, 0xfafafa, 0xf7f7f7, 0xf4f4f4, 0xf0f0f0, 0xebebeb, 0xe6e6e6,
                       0xe1e1e1, 0xdbdbdb, 0xd4d4d4, 0xcecece, 0xc7c7c7, 0xbfbfbf, 0xb7b7b7, 0xafafaf, 0xa7a7a7, 0x9e9e9e, 0x969696,
                       0x8d8d8d, 0x848484, 0x7b7b7b, 0x727272, 0x696969, 0x616161, 0x585858, 0x505050, 0x484848, 0x404040, 0x383838,
                       0x313131, 0x2b2b2b, 0x242424, 0x1e1e1e, 0x191919, 0x141414, 0x0f0f0f, 0x0b0b0b, 0x080808, 0x050505, 0x030303,
                       0x020202, 0x010101, 0x000000};

int gammaCurve[] =    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                       1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8,
                       8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
                       20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 38, 39,
                       39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67,
                       68, 69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89, 90, 92, 93, 95, 96, 98, 99, 101, 102, 104,
                       105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
                       144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177, 180, 182, 184, 186, 189,
                       191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244,
                       247, 249, 252, 255};

void setup()
{
  Serial.begin(57600);
  leds.begin();
  leds.show();
  
  pinMode(0, OUTPUT);
  
  //generate random starting positions
  for(int i = 0; i < 100; i++){
    randPos[i] = random(333);
  }
  
  delay(2000);
  
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
                ledMode = -1;
              }else{
                ledMode = strtol(cmd, NULL, 10);
              }
              memset(cmd, 0, 32);
              
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
              Serial.printf("effectSpeed:\t%f\n", effectSpeed);
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
          client.println("<option value=03>Red Alert!</option>");
          client.println("<option value=04>Spin (Requires color selection)</option>");
          client.println("<option value=05>Spin Bounce (Requires color selection)</option>");
          client.println("<option value=06>Double Spin (Requires 2 color selections)</option>");
          client.println("<option value=07>Theater Chase (Requires color selection)</option>");
          client.println("<option value=08>Rainbow Theater Chase</option>");
          client.println("<option value=09>Thparkle (Requires color selection)</option>");
          client.println("<option value=10>Random Thparkle</option>");
          client.println("<option value=11>Random Animations</option>");
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
    
    if((!cleared) || (lastMode != 1)){
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
    leds.show();
    
    lastMode = 1;
  }
  
  //Rainbow Mode
  else if(ledMode == 2){
    static int color;
    
    //clear leds if coming from different mode
    if(lastMode != 2){
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
        leds.setPixel(x, rainbowColors[index]);
      }
        
      if(color < 178){color++;}else{color = 0;}
    }
    
    leds.show();
    leds.show();
    
    lastMode = 2;
  }
  
  //Pulse
  else if(ledMode == 3){
    static int index;
    
    //clear leds if coming from different mode
    if(lastMode != 3){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    if(millis1 - prevMillis1 > pulseInterval / effectSpeed){
      prevMillis1 = millis1;
      
      //every update, set a pixel to a random value and fade them
      for(int j = 0; j < 334; j++){
        leds.setPixel(j, fadeVals[index] & 0xFF0000);
      }
      
      if(index < 90){index++;}else{index = 0;}
    }
    
    leds.show();
    leds.show();
    
    lastMode = 3;
  }
  
  //Spin
  else if(ledMode == 4){
    static int pos;
    
    //clear leds if coming from different mode
    if(lastMode != 4){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    for(int i = 0; i < 30; i++){
        if((pos + i) > 333){
          leds.setPixel(((334 - (pos+i)) * -1), color1Cmd);
        }
        leds.setPixel(pos + i, color1Cmd);
      }
      
    leds.show();
      
    if(millis1 - prevMillis1 > spinInterval / effectSpeed){
      prevMillis1 = millis1;
      
      leds.setPixel(pos - 1, 0);
      leds.show();
      
      if(pos < 334){pos++;}else{pos = 0;}
    }
    
    lastMode = 4;
  }
  
  //Spin Bounce
  else if(ledMode == 5){
    static int dir, pos;
    
    if(lastMode != 5){
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
    
    lastMode = 5;
  }
  
  //Double Spin
  else if(ledMode == 6){
    static int pos1 = randPos[77];
    static int pos2 = randPos[32];
    static int segment1[30], segment2[30];
    
    //clear leds if coming from different mode
    if(lastMode != 6){
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
    lastMode = 6;
  }
  
  //Theater Chase
  else if(ledMode == 7){
    static int j;
    
    //clear leds if changing modes
    if(lastMode != 7){
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
      leds.show();
      
      //clear every third pixel
      for(int i=0; i < 334; i=i+3){
        leds.setPixel(i+j, 0);
      }
      
      if(j < 2){j++;}else{j = 0;}
    }
    
    lastMode = 7;
  }
  
  //Rainbow Theater Chase
  else if(ledMode == 8){
    static int color, j;
    
    //clear leds if changing modes
    if(lastMode != 8){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    if(millis1 - prevMillis1 > chaseInterval / effectSpeed){
      prevMillis1 = millis1;
      
      //set every third pixel with a rainbow color
      for(int i=0; i < 334; i=i+3){
        int index = (color + i) % 180;
        leds.setPixel(i+j, gammaCorrect(rainbowColors[index]));
      }
        
      leds.show();
      leds.show();
      leds.show();
      leds.show();
      
      //clear every third pixel
      for(int i=0; i < 334; i=i+3){
        leds.setPixel(i+j, 0);
      }
        
      if(color < 178){color++;}else{color = 0;}
      if(j < 2){j++;}else{j = 0;}
    }
    
    lastMode = 8;
  }
  
  //Sparkle
  else if(ledMode == 9){
    //clear leds if changing modes
    if(lastMode != 9){
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
    leds.show();
    
    lastMode = 9;
  }
  
  //color change sparkle
  else if(ledMode == 10){
    int randColor = random(179);
    
    //clear leds if changing modes
    if(lastMode != 10){
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
    leds.show();
    
    lastMode = 10;
  }
  
  //Random Animations
  else if(ledMode == 11){
    if(lastMode != 11){
      for(int i = 0; i < 334; i++){
        leds.setPixel(i, 0);
      }
      
      leds.show();
      Serial.println("Cleared LEDs");
    }
    
    for(int i = 0; i < 334; i++){
      leds.setPixel(i, 0x333333);
    }
    
    leds.show();
    leds.show();
    
    lastMode = 11;
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
    
    for(int i = 0; i < 334; i++){
      leds.setPixel(i, 0x000000);
    }
    
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
