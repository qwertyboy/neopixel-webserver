/*
 * WebServerPost sketch
 * Turns pin 8 on and off using HTML form
 */

#include <SPI.h>
#include <Ethernet.h>
#include <OctoWS2811.h>

#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF
#define OFF    0x000000

byte mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x81, 0x8D};
byte ip[] = {192, 168, 2, 13};
int dhcpSucess = 0;
int retry = 0;
int maxRetries = 4;

const int MAX_PAGENAME_LEN = 8; // max characters in page name 
char buffer[MAX_PAGENAME_LEN+1]; // additional character for terminating null
char cmdBuffer[33];
EthernetServer server(80);

const int ledsPerStrip = 45;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);
long previousMillis = 0;
long timeout = 15000;

void setup()
{
  Serial.begin(57600);
  delay(2000);
  
  dhcpSucess = Ethernet.begin(mac);
  while(dhcpSucess == 0){
    retry += 1;
    if(retry > maxRetries){
      Serial.println("Maximum retries exceeded, system halting!");
      while(1){}
    }
    
    Serial.println("DHCP configuration failed! Trying again in 15 seconds.");
    dhcpSucess = Ethernet.begin(mac);
  }
  
  Serial.print("DHCP IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  
  server.begin();
  
  leds.begin();
  leds.show();
}

void loop()
{
  unsigned long currentMillis = millis();
  
  EthernetClient client = server.available();
  if (client) {
    int type = 0;
    while (client.connected()) {
        if (client.available()) {
        // GET, POST, or HEAD
        memset(buffer,0, sizeof(buffer)); // clear the buffer
        if(client.readBytesUntil('/', buffer,sizeof(buffer))){
          Serial.println(buffer);
          if(strcmp(buffer,"POST ") == 0){
            client.find("\n\r");
            while(client.findUntil("color=%23", "\n\r")){
              memset(cmdBuffer, 0, sizeof(cmdBuffer));
              byte bytesRead = client.readBytesUntil('\n\r', cmdBuffer, sizeof(cmdBuffer));
              int cmdNumber = (int)strtol(cmdBuffer, NULL, 16);
              Serial.println(bytesRead);
              Serial.println(cmdBuffer);
              Serial.println(cmdNumber);
              
              for(int i = 0; i < leds.numPixels(); i++){
                leds.setPixel(i, cmdNumber);
              }
              
              leds.show();
              previousMillis = currentMillis;
            }
          }
          sendHeader(client,"LED Control Panel");
          
          client.println("<div align='left'><h3>Select a LED color or choose your own:</h3>");
          
          client.println("<form action='/' method='POST'>");
          client.println("<input type='radio' name='color' value='#ff0000'> Red<br>");
          client.println("<input type='radio' name='color' value='#00ff00'> Green<br>");
          client.println("<input type='radio' name='color' value='#0000ff'> Blue<br>");
          client.println("<input type='radio' name='color' value='#ffff00'> Yellow<br>");
          client.println("<input type='radio' name='color' value='#ff1088'> Pink<br>");
          client.println("<input type='radio' name='color' value='#e05800'> Orange<br>");
          client.println("<input type='radio' name='color' value='#ffffff'> White<br>");
          client.println("Choose your own color: <input type='color' name='color'><br>");
          client.println("<input type='submit' value='Submit'></form>");
          
          client.println("<form action='/' method='POST'>");
          client.println("<input type='hidden' name='color' value='#000000'>");
          client.println("<input type='submit' value='Off'></form></div>");
          
          client.println("<div align='center'><img src='http://nathanspi.no-ip.org:8082'></img></div>");
          
          /*
          Use this code if running Yawcam on a pc host
          
          client.print("<div align='center'><applet code='YawApplet.class' archive='YawApplet.jar'");
          client.println(" codebase='http://nathanspi.no-ip.org:8081' width='640' height='480'>");
          client.println("<param name='Host' value='nathanspi.no-ip.org'>");
          client.println("<param name='Port' value='8081'>");
          client.println("<param name='Zoom' value='true'></applet></div>");
          */
          
          client.print("</body></html>");
          client.stop();
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
  
  if((currentMillis - previousMillis > timeout) && (leds.getPixel(0) != 0x000000)){
    previousMillis = currentMillis;
    Serial.println("Timeout reached! Turning off.");
    
    for(int i = 0; i < leds.numPixels(); i++){
      leds.setPixel(i, 0x000000);
    }
    leds.show();
  }
}

void sendHeader(EthernetClient client, char *title)
{
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.print("<html><head><title>");
  client.print(title);
  client.println("</title><body>");
}   
