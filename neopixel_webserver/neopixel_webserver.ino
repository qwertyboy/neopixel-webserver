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
const int MAX_PAGENAME_LEN = 8; //max characters in page name 
char buffer[MAX_PAGENAME_LEN+1]; // additional character for terminating null
EthernetServer server(80);

//html command variables
char cmdBuffer[129];
String cmd = ""; 
String modeCmd = "";
String color1Str = "";
String color2Str = "";
char color1StrBuf[7];
char color2StrBuf[7];
int color1Cmd = 0;
int color2Cmd = 0;
int colorCombo = 0;

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
int pulseInterval = 50;
int spinInterval = 10;
int spinInterval2 = 15;
int chaseInterval = 50;
int sparkleInterval = 1300;  //Tune this to get the desired update speed for sparkle effect (1300 default)

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
              memset(cmdBuffer, 0, 129);
              memset(color1StrBuf, 0, 7);
              memset(color2StrBuf, 0, 7);
              
              cmd = "";
              modeCmd = "";
              color1Str = "";
              color2Str = "";
              color1Cmd = 0;
              color2Cmd = 0;
              colorCombo = 0;
              
              byte bytesRead = client.readBytesUntil('\n\r', cmdBuffer, 129);
              
              //put the command buffer into a string
              for(int i = 0; i < bytesRead; i++){
                cmd = cmd + cmdBuffer[i];
              }
              
              modeCmd = cmd.substring(0, cmd.indexOf('&'));
              
              //get the string for each color and convert to a number
              color1Str = cmd.substring((cmd.indexOf("r=%23") + 5), cmd.indexOf("&customColor2"));
              color2Str = cmd.substring(cmd.indexOf("2=%23") + 5);
              
              color1Str.toCharArray(color1StrBuf, 7);
              color2Str.toCharArray(color2StrBuf, 7);
              
              color1Cmd = strtol(color1StrBuf, NULL, 16);
              color2Cmd = strtol(color2StrBuf, NULL, 16);
              colorCombo = addColors(color1Cmd, color2Cmd);
              
              Serial.printf("bytesRead:\t%d\n", bytesRead);
              Serial.println("cmd:\t\t" + cmd);
              Serial.println("modeCmd:\t" + modeCmd);
              //Serial.println("color1Str:\t" + color1Str);
              //Serial.println("color2Str:\t" + color2Str);
              Serial.printf("color1Cmd:\t%x\n", color1Cmd);
              Serial.printf("color2cmd:\t%x\n", color2Cmd);
              Serial.printf("colorCombo:\t%x\n", colorCombo);
              
              //pick effect mode
              if(modeCmd == "staticColor"){
                ledMode = 1;
              }else if(modeCmd == "slowRainbow"){
                ledMode = 2;
              }else if(modeCmd == "pulse"){
                ledMode = 3;
              }else if(modeCmd == "spin"){
                ledMode = 4;
              }else if(modeCmd == "spinBounce"){
                ledMode = 5;
              }else if(modeCmd == "doubleSpin"){
                ledMode = 6;
              }else if(modeCmd == "theaterChase"){
                ledMode = 7;
              }else if(modeCmd == "rainbowChase"){
                ledMode = 8;
              }else if(modeCmd == "sparkle"){
                ledMode = 9;
              }else if(modeCmd == "random"){
                ledMode = 10;
              }else if(modeCmd == "weatherClock"){
                ledMode = 11;
              }else if(modeCmd == "off"){
                ledMode = 0;
              }else{
                ledMode = -1;
              }
              
              Serial.printf("ledMode:\t%d\n", ledMode);
            }
          }
          
          //send html for control page
          sendHeader(client,"LED Control Panel");

          client.println("<div align='left'>");
          client.println("<h3>Select a mode:</h3>");
          //client.println("Static Color, Spin, Spin Bounce, Theater Chase, and Sparkle require a color selection.");
          client.println("<form action='/' method='POST'>");
          client.println("<select name='mode'>");
          client.println("<option value='staticColor'>Static Color (Requires color selection)</option>");
          client.println("<option value='slowRainbow'>Slow Rainbow</option>");
          client.println("<option value='pulse'>Pulse</option>");
          client.println("<option value='spin'>Spin (Requires color selection)</option>");
          client.println("<option value='spinBounce'>Spin Bounce (Requires color selection)</option>");
          client.println("<option value='doubleSpin'>Double Spin (Requires 2 color selections)</option>");
          client.println("<option value='theaterChase'>Theater Chase (Requires color selection)</option>");
          client.println("<option value='rainbowChase'>Rainbow Theater Chase</option>");
          client.println("<option value='sparkle'>Thparkle (Requires color selection)</option>");
          client.println("<option value='random'>Random Animations</option>");
          client.println("<option value='weatherClock'>Weather Clock</option>");
          client.println("<option value='off'>Off</option>");
          client.println("</select><br>");
          client.println("Color 1: <input type='color' name='customColor'>");
          client.println("Color 2: <input type='color' name='customColor2'>");
          client.println("<input type='submit'>");
          client.println("</form>");
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
    
    if(millis1 - prevMillis1 > rainbowInterval){
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
    
    if(millis1 - prevMillis1 > pulseInterval){
      prevMillis1 = millis1;
      
      //every update, set a pixel to a random value and fade them
      for(int j = 0; j < 334; j++){
        leds.setPixel(j, fadeVals[index] & random(0xFFFFFF));
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
      
    if(millis1 - prevMillis1 > spinInterval){
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
      
      if(millis1 - prevMillis1 > spinInterval){
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
      
      if(millis1 - prevMillis1 > spinInterval){
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
    if(millis1 - prevMillis1 > spinInterval){
      prevMillis1 = millis1;
      
      leds.setPixel(segment1[0], 0);
      leds.show();
      
      if(pos1 < 333){pos1++;}else{pos1 = 0;}
    }
    
    //update segment 2
    if(millis2 - prevMillis2 > spinInterval2){
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
    
    if(millis1 - prevMillis1 > chaseInterval){
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
    
    if(millis1 - prevMillis1 > chaseInterval){
      prevMillis1 = millis1;
      
      //set every third pixel with a rainbow color
      for(int i=0; i < 334; i=i+3){
        int index = (color + i) % 180;
        leds.setPixel(i+j, rainbowColors[index]);
      }
        
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
      
      if(!random(sparkleInterval)){
        leds.setPixel(i, brightness);
      }
    }
    
    leds.show();
    leds.show();
    
    lastMode = 9;
  }
  
  //Random Animations
  else if(ledMode == 10){
    if(lastMode != 10){
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
    
    lastMode = 10;
  }
  
  //Weather clock
  else if(ledMode == 11){
    if(lastMode != 11){
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
    
    lastMode = 11;
  }
  
  //ledMode not valid or zero, so turn off all but last one
  else{
    lastMode = -1;
    
    for(int i = 0; i < 334; i++){
      leds.setPixel(i, 0x000000);
    }
    
    leds.setPixel(333, 0x010101);
    //leds.show();
  }
}

//send a standard http response header
void sendHeader(EthernetClient client, char *title){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.print("<html><head><title>");
  client.print(title);
  client.println("</title><body>");
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
