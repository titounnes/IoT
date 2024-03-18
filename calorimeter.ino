#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


#define THERMISTORPIN A0 
const char* ssid = "micro-project";
const char* passphrase = "12345678";
const char* token= "6469982677:AAFNCb5QMDuPqi8gWYQ4SjsOCCoo1WZKsvU"; // token bot iotProjectTtoBot
const String  headHtml = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head>";

int lcdColumns = 16;
int lcdRows = 2;
int backLight = 0;
int connectInt = 0;
int startTime =0;  

String st;
String content;
String data;
int statusCode;
char c;
float galat;
int report = 0;
bool testWifi(void);
void launchWeb(int webtype);
void setupAP();
void createWebServer(int webtype);
String mac;
const char* host = "https://iot.e-project-tech.com/";
const long interval = 10000;
unsigned long previousMillis = 0;
int idle =0;

ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

void showDisplay(String line1, String line2){
  lcd.clear();
  if(backLight == 0){
    lcd.backlight();
  }else{
    lcd.noBacklight();
  }
  if(line1 != ""){
    lcd.setCursor(0, 0);
    lcd.print(line1);
  }
  if(line2 != ""){
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
} 

bool connectFromCache(){
  String esid;
  mac = WiFi.macAddress();
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        return true;
      } 
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(THERMISTORPIN, INPUT);

  EEPROM.begin(512);
  delay(10);
  lcd.init();
  lcd.backlight();
  showDisplay("Startup.","Reading data...");
  bool conected = connectFromCache();
  if(conected){
    launchWeb(0);
  }else{
    setupAP();
  }
}


void launchWeb(int webtype) {
  if(webtype==1){
    showDisplay("Wifi not conected.",WiFi.softAPIP().toString());  
  }else{
    showDisplay("Wifi conected.",WiFi.localIP().toString());       
  }
  delay(3000);
  createWebServer(webtype);
  server.begin();
  Serial.println("Server started"); 
  Serial.println(WiFi.softAPIP().toString()); 
  Serial.println(WiFi.localIP().toString()); 
}

String getAp(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  connectInt = 0;
  int n = WiFi.scanNetworks();
  if (n == 0){
    return "";
  }
  for (int i = 0; i < n; ++i){
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
  }
  String opt = "<select name='ssid'>";
  for (int i = 0; i < n; ++i){
      opt += "<option value='";
      opt += WiFi.SSID(i);
      opt += "'>";
      opt += WiFi.SSID(i);
      opt += "</option>";
  }
  opt += "</select>";  
  return opt;
}

void setupAP(void) {
  st = getAp();
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

String getHtml(String segment){
  HTTPClient https;    //Declare object of class HTTPClient
  WiFiClientSecure client;
  
  https.begin(client, host + segment+"/index.html");              
  https.addHeader("Content-Type", "text/html");    
  String payload = https.getString();    //Get the response payload

  Serial.println(payload);    //Print request response payload
  https.end();  //Close connection
  return payload;
}

void createWebServer(int webtype)
{
  Serial.println(webtype);
  if ( webtype == 1 ) {
    server.on("/", []() {
        content = headHtml+ "<h1>Setting WiFi</h1>";
        content += "<form method='get' action='setting'><label>SSID: </label>";
        content += st;
        content += "<br><label>Password: </label><input name='pass' length=64><input type='submit' value='Connect'></form>";
        content += "</html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        int len_qsid = qsid.length();
        String qpass = server.arg("pass");
        int len_pass = qpass.length(); 
        content = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head><body>";
        if (len_qsid > 0 && len_pass > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < len_qsid; ++i)
            {
              EEPROM.write(i, qsid[i]);
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < len_pass; ++i)
            {
              EEPROM.write(32+i, qpass[i]);
            }    
          EEPROM.commit();

          content += "<p>SSID berhasil set. Silahkan reset perangkat...</p>";
        } else {
          content += "<p>SSID gagal diset. Silahkan ulangi...</p>";
          content += "<p><script>setTimeout(function(){window.location.href='/'},5000)</script></body></html>";
        }
      content += "</html>";
      server.send(statusCode, "text/html", content);
      bool conected = connectFromCache();
      if(conected){
        launchWeb(0);
      }else{
        setupAP();
      }
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head>";
      content += "<h3>Control Panel</h3><p>url: <a href=\""+String(host)+"/api/calorimeter/"+String(mac)+"\" target=\"_blank\">Check Data</a></p>";
      if(connectInt==0){
        content += "<p>Start Measuring: <a href=\"/start\">Go</a></p>";
      }else{
        content += "<p>Stop Measuring: <a href=\"/stop\">Stop</a></p>";
      }
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/start", []{
      connectInt = 1;
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head><body>";
      content += "<p>Start meausuring ....</p>";
      content += "<p><script>setTimeout(function(){window.location.href='/'},5000)</script></body></html>";
      server.send(200, "text/html", content);
    });
    server.on("/stop", []{
      connectInt = 0;
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head><body>";
      content += "<p>Stop meausuring ....</p>";
      content += "<p><script>setTimeout(function(){window.location.href='/'},5000)</script></body></html>";
      server.send(200, "text/html", content);
    });
    server.on("/reset", []() {
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'></head>";
      content += "<p>Clearing the data.<br> Please wait...</p><script>setTimeout(function(){window.location.href='/'},5000)</script></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
      bool conected = connectFromCache();
      if(conected){
        launchWeb(0);
      }else{
        setupAP();
      }
    });
  }
}


float getTemperature() { 
  float Vin = 5.0;
  int adc = analogRead(THERMISTORPIN);
  float R_ratio = 0.972 * pow(adc/1024.0,  1.06);
  float Ro = 10000.0/R_ratio - 10000.0;
  float Tinv = 0.001194513474+0.000219973381200282*log(Ro)+0.0000001729919901*log(Ro)*log(Ro)*log(Ro);
  float T = 1/Tinv-273.15;
  return T;
}


void sendData(int period, float temperature){
  HTTPClient https;    //Declare object of class HTTPClient
  WiFiClientSecure client;
  client.setInsecure();
  
  String segment = "/api/calorimeter/";
  String postData;

  postData = "mac=" + mac + "&temperature=" + temperature +"&period="+period ;
  
  https.begin(client, host + segment);              //Specify request destination
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
  int httpCode = https.POST(postData);   //Send the request
  String payload = https.getString();    //Get the response payload

  Serial.println(httpCode);   //Print HTTP return code
  https.end();  //Close connection
}

void loop() {
  int now;
  unsigned long currentMillis = millis();
  server.handleClient();
  if(connectInt==1){
    idle = 1;
    if(startTime==0){
      startTime = millis();
      now = startTime; 
    }else{
      now = millis();
    }
    if(currentMillis - previousMillis > interval){
      previousMillis = currentMillis;
      float temperature = getTemperature();
      int period = (now-startTime)/1000;
      if(period > 0){
        period  = period  + 1;
      }
      showDisplay("Detik ke: "+String(period), "Temp: "+String(temperature)+" C");  
      sendData(period, temperature);
      Serial.print("Detik ke- : ");
      Serial.println(period);
      Serial.print("Temperatur : ");
      Serial.print(temperature);
      Serial.println("C");    
    }
  }else{
    if(idle>0){
      Serial.println("Stop measauring");
      showDisplay("Go to:", WiFi.localIP().toString());
      startTime=0;
      idle = 0;
    }
  }
}
