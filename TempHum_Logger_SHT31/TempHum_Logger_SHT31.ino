#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

bool enableHeater = true;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

#include "FS.h"
#include "SD.h"

float t;
float h;

String dataMessage;
String timeStamp;
unsigned long delayTime;

#include <WiFi.h>
#include <HTTPClient.h>
WiFiClient client;
#define CONNECTION_TIMEOUT 30


char ssid[] = "VM-CSJP";      //  network SSID (name)
char pass[] = "Vyoman@CleanAirIndia";   //  network password
String Device_Token = "1f1d4d21-223e-4032-b713-bf97e8cd1ba7";
int keyIndex = 0;         
char server[] = "api.tago.io";

#include "RTClib.h"
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  900
#define TIME_TO_WAIT  840000

void print_wakeup_reason()
{
esp_sleep_wakeup_cause_t wakeup_reason;

wakeup_reason = esp_sleep_get_wakeup_cause();
Serial.println();
Serial.println();
Serial.println();
switch(wakeup_reason)
{
case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
}
}


void setup() 
{

Serial.begin(115200);

print_wakeup_reason();
esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

print_wakeup_reason();
esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds"); 

if (!SD.begin(4)) 
{
Serial.println("SD Card Initialization Failed!");
}

uint8_t cardType = SD.cardType();
if(cardType == CARD_NONE) {
Serial.println("No SD card attached");
return;
}


if (! rtc.begin()) 
{
Serial.println("Couldn't find RTC");
}

Serial.println("SHT31 test");
if (! sht31.begin(0x44)) 
{   // Set to 0x45 for alternate i2c addr
Serial.println("Couldn't find SHT31");
while (1) delay(1);
}

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");


    //WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid, pass);
    Serial.println("\nConnecting");
    int timeout_counter = 0;

    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(200);
        timeout_counter++;
        if(timeout_counter >= CONNECTION_TIMEOUT*5){
        ESP.restart();
        }
    }
    
//rtc.adjust(DateTime(2022, 9, 29, 12, 36, 0));    

}



void loop() 
{


t = sht31.readTemperature();
h = sht31.readHumidity();

  
if (! isnan(t)) {  // check if 'is not a number'
Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
} 
else 
{ 
Serial.println("Failed to read temperature");
}
  
if (! isnan(h)) {  // check if 'is not a number'
Serial.print("Hum. % = "); Serial.println(h);
} 
else 
{ 
Serial.println("Failed to read humidity");
}
delay(1000);



if (++loopCnt == 10) 
{

printValues();

if (h>85) 
{
sht31.heater(true);
Serial.print("Heater Enabled State: ");
if (sht31.isHeaterEnabled())
Serial.println("ENABLED");
else
Serial.println("DISABLED");
delay(30000);
sht31.heater(false);
Serial.print("Heater Enabled State: ");
if (sht31.isHeaterEnabled())
Serial.println("ENABLED");
else
Serial.println("DISABLED");
}

Serial.flush(); 

delay(TIME_TO_WAIT);


}
}


void printValues() 
{

String variables[] = {"Temp", "Hum"};
float values[] = {t, h};


Serial.println("\nStarting connection to server...");

for (int i = 0; i < (sizeof(values) / sizeof(int)); i++)
{
  
// if you get a connection, report back via serial:
//String PostData = String("{\"variable\":\"temperature\", \"value\":") + String(t)+ String(",\"unit\":\"C\"}");
String PostData = String("{\"variable\":") + String("\"") + String(variables[i]) + String("\"") + String(",\"value\":") + String(values[i]) + String("}");

String Dev_token = String("Device-Token: ")+ String(Device_Token);
  

if (client.connect(server,80)) 
{                     
Serial.println("connected to server");
// Make a HTTP request:
client.println("POST /data? HTTP/1.1");
client.println("Host: api.tago.io");
client.println("_ssl: false");       // for non-secured connection, use this option "_ssl: false"
client.println(Dev_token);
client.println("Content-Type: application/json");
client.print("Content-Length: ");
client.println(PostData.length());
client.println();
client.println(PostData);

}
else 
{
// if you couldn't make a connection:
Serial.println("connection failed");
}
}


Serial.println();

DateTime now = rtc.now();

Serial.print(now.year(), DEC);
Serial.print('/');
Serial.print(now.month(), DEC);
Serial.print('/');
Serial.print(now.day(), DEC);
Serial.print(" (");
Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
Serial.print(") ");
Serial.print(now.hour(), DEC);
Serial.print(':');
Serial.print(now.minute(), DEC);
Serial.print(':');
Serial.print(now.second(), DEC);
Serial.println();


timeStamp = String(now.day()) + "/" + String(now.month())+ "/" + String(now.year()) + "," + String(now.hour())+ ":" + String(now.minute()) + ":" + String(now.second()) + ", " ;
    
logSDCard();


delay(2000);

client.stop();

delay(2000);

}

void logSDCard() 
{
  dataMessage =  timeStamp + String(t) + "," + String(h) + "\r\n";
  Serial.print("Saved data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());
}


// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) 
{
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) 
{
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
    delay(1000);

  } else {
    Serial.println("Append failed");
  }
  file.close();
}
