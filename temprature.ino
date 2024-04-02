#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DHTesp.h>
#include <DHT.h>
#include <DHT_U.h>
#include <string>

#define DHTPIN 0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 5, 4);

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const char *ssid = "Default";
const char *password = "Default";
String st;
String content;
int i = 0;
int statusCode;

const long utcOffsetInSeconds = 19800;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);

int timeCounter = 0;

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

// server info
char serverAddress[] = "";  // server address
int port = 8080;

void setup() {
  
  Serial.begin(115200);
  Serial.begin(9600);
  
  // set up the LCD's number of columns and rows:
  lcd.init();       // Initialize the lcd
  lcd.backlight();  // turn on the backlight
  dht.begin();
  
  delay(1000); /// Minor delay
  initWifi();
  while (WiFi.status() != WL_CONNECTED)
  {
    lcd.clear();
    lcd.print("Connecting...");
    delay(1000);
    lcd.cursor();
    lcd.blink();
  }
  timeClient.begin();
  lcd.clear();
  lcd.print("IP Address:");
  lcd.setCursor(0, 1); ///  Move to Next line
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();
  lcd.noBlink();
  lcd.noCursor();
}

void loop() {

  delay(1000);
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  String currentMonthName = months[currentMonth-1];
  int currentYear = (ptm->tm_year+1900)-2000;
  String currentDate = String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear);
  // DHT vars
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);


  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(daysOfTheWeek[timeClient.getDay()]);
  lcd.print(",");
  lcd.print(currentDate);
  lcd.setCursor(0, 1);
  lcd.print(timeClient.getHours());
  lcd.print(":");
  lcd.print(timeClient.getMinutes());
  lcd.print(":");
  lcd.print(timeClient.getSeconds());
  delay(2000);

  lcd.clear();
  lcd.noBlink();
  lcd.noCursor();
  lcd.setCursor(0, 0);

   
  lcd.print("humidity: ");
  lcd.print(h);
  lcd.setCursor(0, 1);
  lcd.print("temp: ");
  lcd.print(t);
  lcd.print(" C");
  delay(3000);
  
  if(timeCounter >= 60){
    getRequest(h,t);
    timeCounter = 1;
  }
  if(timeCounter == 0){
    getRequest(h,t);
    timeCounter = 1;
  }
  timeCounter = timeCounter + 1;
}

void getRequest(float h, float t){
  HTTPClient http;
  WiFiClient client;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GET request");
  
  std::string urlpath= "http:///data/"+ std::to_string(t)+"/"+ std::to_string(h)+"/"+"prod-test";
  Serial.println(String(urlpath.c_str()));
  http.begin(client, String(urlpath.c_str()));
  int httpCode = http.GET();
  if (httpCode > 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    // HTTP header has been send and Server response header has been handled
    lcd.print(httpCode);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(http.errorToString(httpCode).c_str());
  }

  http.end();
}

void displayLongText(String text, int col, int row){
  lcd.setCursor(col, row);
  int   ArrayLength  =text.length()+1;    //The +1 is for the 0x00h Terminator
  char  array2[ArrayLength];
  text.toCharArray(array2,ArrayLength);
  int counter = (int) ArrayLength;
  Serial.println(ArrayLength);
  Serial.println( sizeof(char));
  Serial.println( counter);
  Serial.println();

  lcd.print(text);

  // char array2[] = text;
  for (int positionCounter = 0; positionCounter < counter; positionCounter++) {
    lcd.scrollDisplayLeft(); 
                // stretch the content of the message on the display from right to left.
      // show the message on the display
    delay(300);                          // wait 500 ms
  }
  delay(1000);
  lcd.clear();
}

void lcd0(){
  lcd.setCursor(0, 0);
  lcd.clear();
}

void lcd1(){
  lcd.setCursor(0, 1);
  lcd.clear();
}

void initWifi(){
  
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  delay(1000);
  WiFi.disconnect();
  EEPROM.begin(512);
   //Initialasing EEPROM
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);

  delay(1000);
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  lcd0();
  lcd.print("SSID: ");
  lcd.setCursor(0, 1);
  lcd.print(esid);
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  lcd0();
  lcd.print("PASS: ");
  lcd.setCursor(0, 1);
  lcd.print(epass);

  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    lcd.clear();
    lcd.print("Connected!!!");
    return;
  }
  else
  {
    lcd.clear();
    lcd.print("Hotspot on");
    launchWeb();
    setupAP();// Setup HotSpot
    delay(1000);
  }
  lcd0();
  lcd.print("Waiting.");

  while ((WiFi.status() != WL_CONNECTED))
  {
    lcd0();
    lcd.print(WiFi.softAPIP());
    lcd.setCursor(0, 1);
    lcd.print("Waiting...");
    delay(500);
    server.handleClient();
  }
}

//----------------------------------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void)
{
  int c = 0;
  lcd.clear();
  lcd.print("Waiting to connect");
  while ( c < 40 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    lcd0();
    lcd.print("*");
    lcd0();
    c++;
  }
  lcd0();
  lcd.print("timed out");
  lcd.setCursor(0,1);
  lcd.print("opening AP");
  return false;
}

void launchWeb()
{
  if (WiFi.status() == WL_CONNECTED)
    lcd0();
    lcd.print("WiFi connected");
  createWebServer();
  // Start the server
  server.begin();
  // lcd.print("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n == 0){
    lcd.print("no networks found");
    delay(1000);
  }

  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("ElectronicsInnovation", "");
  lcd.clear();
  launchWeb();
  lcd0();
  lcd.print("over");
}

void createWebServer()
{
  {
    server.on("/", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Welcome to Wifi Credentials Update page";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><br/><label>SSID: </label><br/>";
      
      content +="<label for='ssid'>select wifi networks:</label><br/>";
      content += "<select name='ssid' id='ssid'>";
      content += "</select>";
      
      
      content += "<br/><input name='pass' length=64><br/><br/><input type='submit'></form><br/><br/>";
      content += "<script>";
      content += "select = document.getElementById('ssid');";
      int n = WiFi.scanNetworks();
      if (n > 0)
      {
        for (int i = 0; i < n; ++i)
        {
          content += "var opt = document.createElement('option');opt.value =\""+ WiFi.SSID(i)+"\";opt.innerHTML =\""+ WiFi.SSID(i)+"\";select.appendChild(opt);";
        }
      }
      content += "</script>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        lcd.print("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        lcd0();
        lcd.print(qsid);
        lcd.print("");
        lcd1();
        lcd.print(qpass);
        lcd.print("");

        lcd.print("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          lcd0();
          lcd.print("Wrote: ");
          lcd.print(qsid[i]);
        }
        lcd.print("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          lcd1();
          lcd.print("Wrote: ");
          lcd.print(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        displayLongText("saved to eeprom... reset to boot into new wifi", 15, 0);
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        lcd0();
        lcd.print("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

    });
  }
}
