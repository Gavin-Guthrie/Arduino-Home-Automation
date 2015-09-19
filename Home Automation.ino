#define WEBDUINO_FAIL_MESSAGE "<h1>Request Failed</h1>"
#define WEBDUINO_SERVER_HEADER "Server: Switch/"
#define WEBDUINO_AUTH_REALM "Switch"
#define WEBDUINO_SERIAL_DEBUGGING 0
#define PREFIX ""
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"
#include <EEPROM.h>

  
 /* CHANGE THIS TO MATCH YOUR HOST NETWORK. Most home networks are in
 * the 192.168.0.XXX or 192.168.1.XXX subrange. Pick an address
 * that's not in use and isn't going to be automatically allocated by
 * DHCP from your router. */
static uint8_t ip[] = {192,168,0,16};
static uint8_t gateway[] = {192,168,3,1};
static uint8_t subnet[] = {255,255,255,0};
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
 /* The following should be updated as required
 * The relayUse array human object names for the Relay Function
 */
String relayUse[] = {"Undefined","Undefined","Undefined","Undefined","Undefined","Undefined","Undefined"}; //defaults for pin names. EEPROM write not emplimented for this yet.
boolean invertOO = true; //if your using different hardware setup or relay boards your HIGH might be ON or OFF(or NO/NC) just switch this (global settings)
#define THEME "b" //options a-e see http://jquerymobile.com/demos/1.2.1/docs/pages/pages-themes.html
 //the following variables are normally not changed proceed undocumented
 //
const int PulseTime = 500; // time in miliseconds for pulse high width:global setting
WebServer webserver(PREFIX, 80); // port webserver will listen on HTTP default is port 80
const char* AUTHENTICATION = "YWRtaW46YWRtaW4="; //admin:admin Base64-encoded
boolean AUTHENABLED = true; //use Auth on Switching and settings Page
#define MAXrelay 3 //the upper limit of PINS that will be used (memory constraints)
int TotalRelay = 9;
boolean relayAtBoot = LOW; // for high or low state at boot of code, typically not changed as invert deals
boolean HardwareControl = true; // enable hardware buttons for ON off
#define AnalogONpin A0 //pin for analog on button
#define AnalogOFFpin A1 //pin for analog off button
boolean AnalogEnabled = false; // enable printing the output of analog pins to the root webpage used for reading device LED woth optioresistor for "on off" status etc..
#define Analog1Pin A2 //pin for analog reading
#define Analog2Pin A3 //pin for analog reading
#define Analog1UpperValue 900 //the upper value for analog reading after this point will flip to "on" vs off
#define Analog2UpperValue 900 //the upper value for analog reading after this point will flip to "on" vs off
boolean Analog1 = false; //false = off
boolean Analog2 = false; //false = off
unsigned long previousMillis = 0;
P(htmlHead) =
"<!DOCTYPE html><html><head>"
"<title>PowerSwitch</title>"
"<meta name='viewport' content='width=device-width, initial-scale=3'>"
"<META HTTP-EQUIV='CACHE-CONTROL' CONTENT='NO-CACHE'>"
"<link rel='stylesheet' href='http://marvinstuart.com/switch/jquery.mobile.min.css' />"
"<script src='http://marvinstuart.com/switch/jquery.min.js'></script>"
"<script src='http://marvinstuart.com/switch/jquery.mobile.min.js'></script>"
"</head><body>"
"<div data-role='page' data-theme='"THEME"'>"
"<div data-role='header'><center></center><a href='/switch'>SwitchPower</a></h1></div><!-- /header -->"
"<div data-role='content'>";
P(htmlFooter) = "<div data-role='footer' data-position='fixed'><h6>Remote Switch</h6></div><!-- /footer -->";// add footer information
P(endrefresh)="<button id='PageRefresh'>Refresh</button><script type='text/javascript'>$('#PageRefresh').click(function() {location.reload();});</script>";
// the following are system variables
static uint8_t relayPinAddressStart = 255; //default pis status for EEPROM
static uint8_t relayTypeAddressStart = 0; //default pin type for EEPROM
uint8_t relayPin[] = {9,8,7,6,5,4,3}; //the default pins to relay (stored in EEPROM after you change in web)
uint8_t relayType[] = {0,0,0,0,0,0,0}; //the default pin type relay-0 or PWM-1 (stored in EEPROM after you change in web)
uint8_t aio = 1;
uint8_t aif = 1;
P(analog1ON) = "Analog Sensor 1 <font color='green'>ON</font>";
P(analog2ON) = "Analog Sensor 2 <font color='green'>ON</font>";
P(formStart) = "<form action='" PREFIX "/switch' method='post'><center>";
P(formEnd) = "<input type='submit' value='Submit'/><input type='reset' value='Reset'></form>";
P(spacer) = "<fieldset data-role='controlgroup' data-type='horizontal'>";
 
template<class T> // no-cost stream operator as described at http://arduiniana.org/libraries/streaming/
inline Print &operator <<(Print &obj, T arg)
{
 obj.print(arg);
 return obj;
}
  
void outputPins(WebServer &server, WebServer::ConnectionType type, bool addControls = false) //main web routine
{
server.httpSuccess();
server.printP(htmlHead);
  
if (addControls)
 {
 server.printP(formStart);
 }
 for (int i = 1; i <= MAXrelay; ++i)
 {
 if (relayPin[i-1] >= 0 && relayPin[i-1] <= 53)
 {
 if (relayType[i-1] == 0)
 {
 String displayName;
 if (relayUse[i-1].equalsIgnoreCase("Undefined")){
 displayName = String("Relay-" + String(i,DEC) + " ");
 }
 else {
 displayName = String(relayUse[i-1] + " ");
 }
 displayPins(server, relayPin[i-1], relayType[i-1], displayName, addControls);
 }
 else if (relayType[i-1] == 1)
 {
 String displayName;
 if (relayUse[i-1].equalsIgnoreCase("Undefined")){
 displayName = String("PulseRelay-" + String(i,DEC) + " ");
 }
 else {
 displayName = String(relayUse[i-1] + "* ");
 }
 displayPins(server, relayPin[i-1], relayType[i-1], displayName, addControls);
 }
 }
 }
//display analog controls
if (AnalogEnabled && Analog1 ) {
 server.printP(analog1ON);
}
if (AnalogEnabled && Analog2 ) {
 server.printP(analog2ON);
}
server << "</center></p>";
  
if (addControls)
 {
 server.printP(formEnd);
 }
 else
 {
 server.printP(endrefresh);
 }
server.printP(htmlFooter);
}
  
void settingsPage(WebServer &server, WebServer::ConnectionType type) //settings only page
{
 if(!server.checkCredentials(AUTHENTICATION) && AUTHENABLED)
 {
 server.httpUnauthorized(); //not authenticated
 }
 else //authenticated
 {
server.httpSuccess();
server.printP(htmlHead);
server << "<form action='" PREFIX "/settings' method='post'>";
char pinName[4];
  
for (int i = 1; i <= MAXrelay; ++i)
 {
 server << "<label>Relay Port " << i << "<input type='text' name='u" << i << "' value='" << relayUse[i-1] << "'>" << " Pin: <input type='text' name='h" << i << "' value='" << relayPin[i-1] << "'></label><br/>";
 pinName[0] = 'e';
 itoa(i, pinName + 1, 10);
 server.radioButton(pinName, "0", "Latched Relay(LR)", !relayType[i-1]);
 server << " ";
 server.radioButton(pinName, "1", "Pulse Relay(PR)", relayType[i-1]);
 server << "<br/>";
 }
server << "<input type='submit' value='Submit'/></form>";
server.printP(htmlFooter);
 }
}
void settingsCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) //settings post handler
{
 if (type == WebServer::POST)
 {
 bool repeat;
 char name[MAXrelay*3], value[MAXrelay*3];
 do
 {
 repeat = server.readPOSTparam(name, MAXrelay*3, value, MAXrelay*3);
 int pin = strtoul(name + 1, NULL, 10);
 int val = strtoul(value, NULL, 10);
 String valueString = String(value);
  
 if (name[0] == 'h') //pinNumber
 {
 if (EEPROM.read(relayPinAddressStart+pin-1) == val)
 {
 //do nothing no change in value
 } else {
 if (relayPin[pin-1] > 0 && relayPin[pin-1] <=53)
 {
 digitalWrite(relayPin[pin-1], LOW); //make sure pin is off before overwritting it
 }
 relayPin[pin-1] = val;
 EEPROM.write(relayPinAddressStart+pin-1,val);
 }
 }
 if (name[0] == 'e') //relay type
 {
 if (EEPROM.read(relayTypeAddressStart+pin-1) == val)
 {
 //do nothing no change in value
 } else {
 relayType[pin-1] = val;
 EEPROM.write(relayTypeAddressStart+pin-1,val);
 }
 }
 if (name[0] == 'u') //relay common name
 {
 relayUse[pin-1] = valueString;
 }
  
}
 while (repeat);
  
server.httpSeeOther(PREFIX "/settings");
 }
 else
 settingsPage(server, type);
}
  
void switchCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) // generic post handler and pin writer
{
 if (type == WebServer::POST)
 {
 bool repeat;
 char name[MAXrelay], value[MAXrelay];
 do
 {
 repeat = server.readPOSTparam(name, MAXrelay, value, MAXrelay);
 if (name[0] == 'd')
 {
 int pin = strtoul(name + 1, NULL, 10);
 int val = strtoul(value, NULL, 10);
 int pinindex = -1;
  
 for( int a = 0; a<MAXrelay ; a++ )
 {
 if (pin == relayPin[a]) {
 pinindex = a;
 break;
 }
 }
 if (relayType[pinindex] == 0)
 {
 if (invertOO){ digitalWrite(pin, !val); } else { digitalWrite(pin, val); }
 }
 else
 {
 if (val){
 if (invertOO){ digitalWrite(pin, !val); } else { digitalWrite(pin, val); }
 delay(PulseTime);
 if (invertOO){ digitalWrite(pin, val); } else { digitalWrite(pin, !val); }
 }
 }
 }
 }
 while (repeat);
  
server.httpSeeOther(PREFIX "/switch");
 }
 else
  
 if(!server.checkCredentials(AUTHENTICATION) && AUTHENABLED)
 {
 server.httpUnauthorized(); //not authenticated
 }
 else //authenticated
 {
 outputPins(server, type, true);
 }
}
  
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) //default web page loader
{
 outputPins(server, type, false);
}
  
void displayPins(WebServer &server, int pin, int type, String label, bool addControls) //print the pin status
{
 int val;
 if (invertOO){val = !digitalRead(pin); } else { val = digitalRead(pin);}
 server << label;
 if (addControls)
 {
 char pinName[4];
 server.printP(spacer);
 pinName[0] = 'd';
 itoa(pin, pinName + 1, 10);
  
 if (type == 0)
 {
 //normal relay
 server.radioButton(pinName, "1", "On", val);
 server << " ";
 server.radioButton(pinName, "0", "Off", !val);
 server << "</fieldset>";
 }
 else if (type == 1)
 {
 //latch relay
 server.radioButton(pinName, "1", "Switch", val);
 server << "</fieldset>";
 }
  
 }
 else
 {
  
 if (type == 0)
 {
 //normal relay
 if (invertOO){
 server << (!val ? "<font color='red'>Off</font>" : "<font color='green'>On</font>");
 }
 else
 {
 server << (val ? "<font color='red'>On</font>" : "<font color='green'>Off</font>");
 }
 }
 else if (type == 1)
 {
 //latch relay - recomended that you monitor the analog sensor here and use to print status
server << "<font color='red'>n/a</font>";
 //if (AnalogEnabled && Analog1 ) {server.printP(analog1ON);}
 }
  
}
 server << "<br/>";
}
  
void setup()
{
 Serial.begin(9600);
 // set pins 0-9 for digital input
 for (int i = 2; i <= TotalRelay; ++i)
 {
 pinMode(i, OUTPUT);
 }
 
//Initialize pins - this should be changed if other then UNO
 for (int i = 2; i <= TotalRelay; ++i)
 {
 if (invertOO){
 digitalWrite(i, !relayAtBoot);
 }
 else {
 digitalWrite(i, relayAtBoot);
 }
  
 }
 //start web
 Ethernet.begin(mac, ip);
 webserver.begin();
 webserver.setDefaultCommand(&defaultCmd);
 webserver.addCommand("switch", &switchCmd);
 webserver.addCommand("settings", &settingsCmd);
 Serial.print("server address "); Serial.println(Ethernet.localIP()); //debug to serial if server is 0.0.0.0 web boot failed check sheld connections
 for (int i = 0; i < sizeof(relayPin); i++){
 relayPin[i] = EEPROM.read(relayPinAddressStart + i);
 }
 for (int i = 0; i < sizeof(relayType); i++){
 relayType[i] = EEPROM.read(relayTypeAddressStart + i);
 }
 if (HardwareControl) {
 digitalWrite(AnalogONpin, LOW);
 digitalWrite(AnalogOFFpin, LOW);
 }
 if (AnalogEnabled) {
 digitalWrite(Analog1Pin, INPUT);
 digitalWrite(Analog2Pin, INPUT);
 }
}
  
void loop()
{
 // process incoming connections one at a time forever
 webserver.processConnection();
  
//physical on/off control
 if (HardwareControl) {
 int aon = analogRead(AnalogONpin);
 int aoff = analogRead(AnalogOFFpin);
 if (aon > 1015){;
 //Turn On relays incrementally
 if (relayPin[aio-1] > 0 && relayPin[aio-1] <= 53){
 if (invertOO){ digitalWrite(relayPin[aio-1], LOW); } else { digitalWrite(relayPin[aio-1], HIGH); }
 delay(400); //cheap antibounce
 aif = aio; //keep them synched
 if (aio == sizeof(relayPin)){aio=1;} else { aio++;}
 }
 }
 if (aoff > 1015){;
 //if long delay press power down all relays
 if(previousMillis > 8) //3 second press of button (400 second delay below*8) bug here for over 8 relays
 {
 Serial.println("do");
 previousMillis = 0;
 for (int i = 0; i < sizeof(relayPin); i++){
 {
 if (invertOO){ digitalWrite(relayPin[aif-1], HIGH); } else { digitalWrite(relayPin[aif-1], LOW); }
 aif=1;aio=1; //reset hardware switching
 }
 }
 }
 else { previousMillis++;}
 //Turn Off relays incrementally
 if (relayPin[aif-1] > 0 && relayPin[aif-1] <= 53){
 if (invertOO){ digitalWrite(relayPin[aif-1], HIGH); } else { digitalWrite(relayPin[aif-1], LOW); }
 delay(400); //cheap antibounce
 aio= aif; //keep them synched
 if (aif == sizeof(relayPin)){aif=1;previousMillis = 0;} else { aif--;}
 }
 if(aif<1){aif=1;}
 }
 }
  
//analog reading
 if (AnalogEnabled) {
 int a1 = analogRead(Analog1Pin);
 int a2 = analogRead(Analog2Pin);
 //Serial.println(a1 + ":" + a2); //debug print for tuning the AnalogUpperValue settings
 if (a1 > Analog1UpperValue){;Analog1 = true;}else {Analog1 = false;}
 if (a2 > Analog2UpperValue){;Analog2 = true;}else {Analog2 = false;}
 }
}
