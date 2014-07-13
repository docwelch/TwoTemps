#undef PROGMEM
#define PROGMEM __attribute__(( section(".progmem.data") ))
#undef PSTR
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];}))

#include <OneWire.h>
#include <WiFlyHQ.h>
#include <stdlib.h>

#include <SoftwareSerial.h>
SoftwareSerial wifiSerial(8,9);

WiFly wifly;
char buf[40];

OneWire ds(10);
float temps[]={0.0,0.0};
byte i;


void setup(){
  Serial.begin(115200);
  wifiSerial.begin(9600);
  if (!wifly.begin(&wifiSerial,&Serial)){
    Serial.println(F("Failed to start wifly"));
    wifly.terminal();
  }
  wifly.setBroadcastInterval(0);
  wifly.setDeviceID("Wifly-WebServer");
  
  wifly.setProtocol(WIFLY_PROTOCOL_TCP);
  if (wifly.getPort() !=80){
    wifly.setPort(80);
    wifly.save();
    Serial.println(F("Set port to 80, rebooting to make it work"));
    wifly.reboot();
    delay(3000);
  }
  Serial.println(F("Ready"));
}

void loop(){
    //getData();
    if (wifly.available()>0){
      getData();
      sendData();
      wifly.flushRx();
      temps[0]=0.0;
    }
}

void getData(){
  byte addr[8];
  byte data[12];
  byte present=0;
  Serial.println("Getting Data");
  while (ds.search(addr)){
  //if (!ds.search(addr)){
   // ds.reset_search();
   // delay(250);
   // return;
  //}
  
  if (OneWire::crc8(addr,7)!=addr[7]){
    Serial.println("Invalid CRC");
    return;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);
  delay(1000);
  present=ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for (i=0; i<9; i++){
    data[i]=ds.read();
  }
  //Serial.println("GOT DATA");
  
  unsigned int raw=(data[1]<<8)|data[0];
  raw=raw<<3;
  if (data[7]==0x10){
    raw=(raw&0xFFF0)+12-data[6];
  }
  if (temps[0]<0.1){
    temps[0]=((float)raw/16.0)*1.8+32.0;
    Serial.print("Temp[0] = ");
    Serial.println(temps[0]);
  }else{
    temps[1]=((float)raw/16.0)*1.8+32.0;
    Serial.print("Temp[1] = ");
    Serial.println(temps[1]);
  }
}
ds.reset_search();
delay(250);
}
    
void sendData(){
  Serial.println("Sending");
  char sup[7];
  char ret[7];
  /* Send the header direclty with print */
    wifly.println(F("HTTP/1.1 200 OK"));
    wifly.println(F("Content-Type: text/html"));
    wifly.println(F("Transfer-Encoding: chunked"));
    wifly.println();

    /* Send the body using the chunked protocol so the client knows when
* the message is finished.
* Note: we're not simply doing a close() because in version 2.32
* firmware the close() does not work for client TCP streams.
*/
    wifly.sendChunkln(F("<html>"));
    wifly.sendChunkln(F("<head><style>"));
    wifly.sendChunkln(F("h1{font-size: 64px;}</style>"));
    wifly.sendChunkln(F("<meta http-equiv=\"refresh\" content=\"5\">"));
    wifly.sendChunkln(F("<title>Delta T</title>"));
    wifly.sendChunkln(F("</head>"));
    wifly.sendChunkln(F("<h1>"));
    wifly.sendChunkln(F("<br>"));
    wifly.sendChunk(F("<p>Return Temp = "));
    dtostrf(temps[0],6,2,ret);
    wifly.sendChunk(ret);
    Serial.println(ret);
    wifly.sendChunkln(F("</p>"));
    wifly.sendChunk(F("<p>Supply Temp = "));
    dtostrf(temps[1],6,2,sup);
    wifly.sendChunk(sup);
    Serial.println(sup);
    wifly.sendChunkln(F("</p>"));
    //snprintf_P(buf, sizeof(buf), PSTR("<p>Return Temp = %f</p>"), temps[0]);
    //wifly.sendChunkln(buf);
    //snprintf_P(buf, sizeof(buf), PSTR("<p>Supply Temp = %3.2F</p>"), temps[1]);
    //wifly.sendChunkln(buf);
    wifly.sendChunkln(F("</h1>"));
    wifly.sendChunkln(F("</html>"));
    wifly.sendChunkln();
}


