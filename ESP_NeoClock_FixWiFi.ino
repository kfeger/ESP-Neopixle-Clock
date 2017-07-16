/*
 * ESP_NeoClock ohne WiFiManager
 */
#include <TimeLib.h>
#include <Timezone.h>
#include <TimeLord.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#define NEXT_SYNC 3600  //1h zwischen Sync

//Stunden hell
#define HOUR_C   0x400000
#define HOUR_CL  0x040000
#define HOUR_CLL 0x020000
//Stunden dunkel
#define DARK_HOUR_C   0x080000
#define DARK_HOUR_CL  0x020000
#define DARK_HOUR_CLL 0x010000

//Minuten hell
#define MIN_C  0x4000
#define MIN_CL 0x0400
//Minuten dunkel
#define DARK_MIN_C  0x0400
#define DARK_MIN_CL 0x0100

//Sekunden hell
#define SEC_C      0x60
//Sekunden dunkel
#define DARK_SEC_C 0x06

//Stundenmarker hell
#define MARK_C      0x101010
//Stundenmarker dunkel
#define DARK_MARK_C 0x020202

#define HOUR_OFF 0x00ffff
#define MIN_OFF  0xff00ff
#define SEC_OFF  0xffff00

#define NEXT_SYNC 3600  //1h zwischen Sync
#define FAULT_WAIT 120  //2m nach NTP-Fehler warten
#define WIFI_WAIT 20000 //20s auf WLAN warten
bool NoSync = false;


int NeoPin = 0;
int NeoLength = 60;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NeoLength, NeoPin, NEO_GRB + NEO_KHZ800);

int NowSecond = 0;
int NowMinute = 0;
bool NextMinute = false;
int NowHour = 0;
bool NextHour = false;
const int Offset = 7;
uint32_t PrevColor = 0;
#define SyncPin 2

byte HourC = 64, MinC = 64, SecC = 64;
byte hourval, minuteval, secondval;
bool TimeUpdate = false;

//time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
//void sendNTPpacket(IPAddress &address);

// what is our longitude (west values negative) and latitude (south values negative)
// in Briesnitz, Dresden
TimeLord Dresden;
float const LONGITUDE = 13.658098;
float const LATITUDE = 51.062220;
byte Heute[6];
byte PreDay = 0;
bool SonneDa = true;
bool PreSonneDa = false;

// local time zone definition
// Central European Time (Frankfurt, Paris) from Timezone/examples/WorldClock/WorldClock.pde
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone LTZ(CEST, CET);    // this is the Timezone object that will be used to calculate local time
TimeChangeRule *tcr;        //pointer to the time change rule, use to get the TZ abbrev

time_t utc, local;
uint32_t LastSync = 0, NextSync = 0;


const char ssid[] = "xxxxxxxx";       //  your network SSID (name)
const char pass[] = "yyyyyyyy";       // your network password

/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP, LocalIP; // time.nist.gov NTP server address
const char* ntpServerName = "de.pool.ntp.org";
#define NTP_PACKET_SIZE 48  // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
boolean recalculate = 1;
String text = "";

const int timeZone = 1;     // Central European Time

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
unsigned int localPort = 2930;  // local port to listen for udp packets

void setup() {
  Serial.begin(115200);
  pinMode(SyncPin, OUTPUT);
  digitalWrite(SyncPin, HIGH);
  pinMode(NeoPin, OUTPUT);
  digitalWrite(NeoPin, LOW);
  Serial.println();
  strip.begin();
  strip.show();
  SetMarker(1);

  WiFi.mode(WIFI_STA);
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());

  digitalWrite(SyncPin, LOW);

  Serial.print("IP address assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  Serial.println(F("Hole NTP-Zeit"));
  setSyncProvider(getNTPTime);
  //setTime(getNTPTime());
  if (timeStatus() != timeSet) {
    Serial.println("Uhr nicht mit NTP synchronisiert");
    while (1); //wdt provozieren
  }
  else
    Serial.println("NTP hat die Systemzeit gesetzt");
  LastSync = now();
  NextSync = now() + NEXT_SYNC;

  ClearStrip();
  SunUpDown();
  PreSonneDa = ~SonneDa;

  // Sonnenauf- und -untergang bestimmen
  Dresden.TimeZone(1 * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  // as long as the RTC never changes back and forth between DST and non-DST
  Dresden.DstRules(3, 4, 10, 4, 60); //Umschaltung WZ/SZ am 4. Sonntag im ärz, zurück am 4. Sonntag im Oktober. SZ 60 min plus
  Dresden.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are

  WiFi.disconnect();

}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  if ((now() >= NextSync) && minute() && (minute() != 30)) {  //kein sync bei hh:00 oder hh:30
    time_t NowTime = now();
    long ConnectWait = millis();
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (( millis() - ConnectWait) > WIFI_WAIT) {
        break;  //WLAN-Fehler
      }
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {  //WLAN ist gut....
      setTime(getNTPTime());
      if ((timeStatus() != timeSet) || (year() <= 2000)) {  //ntp-Fehler, nach FAULT_WAIT nochmal
        Serial.println(F("NTP-Fehler!"));
        setTime(NowTime); //bisherige Zeit wieder herstellen
        NextSync = NowTime + FAULT_WAIT; //in zwei Minuten nochmal versuchen
        NoSync = true;
      }
      else {  //npt ist gut, normal weiter nach NEXT_SYNC
        NextSync = now() + NEXT_SYNC;
        NoSync = false;
      }
      WiFi.disconnect();
    }
    else {  //WLAN-Fehler...
      Serial.println(F("WLAN nicht verbunden!"));
      setTime(NowTime); //bisherige Zeit wieder herstellen
      NextSync = NowTime + WIFI_WAIT; //in zwei Minuten nochmal versuchen
      NoSync = true;
    }
  }
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
      NowSecond = second();
      NowMinute = minute();
      NowHour = hour();
      if (NowHour >= 12)
        NowHour -= 12;
      SunUpDown();
      /*if (LTZ.locIsDST(now()))
        digitalWrite(SyncPin, HIGH);
      else
        digitalWrite(SyncPin, LOW);*/
      SetHands();
    }
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

