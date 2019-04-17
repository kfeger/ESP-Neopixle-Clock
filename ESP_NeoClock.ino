/*
   Benutzt 60-LED-Ringe mit
   DIN -> DOUT im Uhrzeigersinn
   oder entgegen.
   Kann OLED-Displays SSD1306 (128x64) ansteuern
   Siehe "Compiler-Steuerung" unten
*/
/* Siehe
   https://github.com/kfeger/NeoPixelClock

   Eine Uhr mit 60 Neopixel auf Basis ESP-01
   Beinhaltet WiFiManager zur Konfiguration des lokalen WLAN.

   Da kein Pin für die Anwendung eines einfachen LDR verfügbar ist,
   wird Sonnenauf- und -untergang für die Helligkeitssteuerung
   verwendet (TimeLord.h). Nachdem die Neopixel über Pin 3 (Rx) gesteuert werden,
   sind Pins 0 und 2 wieder verfügbar. Es könnten weitere Interfaces
   über I2C angebunden werden (hier umgesetzt SSD1306).
   Sommer-/Winterzeit wird über TimeLib.h berechnet, die Zeit selbst liefert
   NTP (de.pool.ntp.org)

   Für mehr Pins kann ESP-12 verwendet werden. dazu
   "#define ESP-12 1" unkommentieren.
 * */

/*
   Compiler-Seuerung
*/

//OLED-Display
#define WITH_OLED 1

//Orte....
#define DD 1    //ottO
//#define HB 1  //Manni
//#define WW 1    //Silke
//#define KO 1  //Sarah

//Plattform
//#define ESP12 1

//LED-Ring-Typ
#define CW_RING 1

/*
   Ende Compiler-Steuerung
*/

#include <TimeLib.h>  //https://github.com/PaulStoffregen/Time
#include <Timezone.h> //https://github.com/JChristensen/Timezone
#include <TimeLord.h> //https://github.com/probonopd/TimeLord
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>  //https://github.com/adafruit/Adafruit_NeoPixel
//needed for WiFi-Manager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include "images.h"
#ifdef WITH_OLED
  #include "SSD1306.h"  //https://github.com/ThingPulse/esp8266-oled-ssd1306
#endif
#include "fonts.h"


#define NEXT_SYNC 3600  //1h zwischen Sync

//Stunden hell
#define HOUR_C   0x400000
#define HOUR_CL  0x080000
#define HOUR_CLL 0x010000
//Stunden dunkel
#define DARK_HOUR_C   0x0B0000
#define DARK_HOUR_CL  0x010000
#define DARK_HOUR_CLL 0x010000

//Minuten hell
#define MIN_C  0x1a00
#define MIN_CL 0x0200
//Minuten dunkel
#define DARK_MIN_C  0x0600
#define DARK_MIN_CL 0x0100

//Sekunden hell
#define SEC_C      0x60
//Sekunden dunkel
#define DARK_SEC_C 0x06

//Stundenmarker hell
#define MARK_C_0      0x101010  //weiß
#define MARK_C_1      0x101010  //weiß
#define MARK0_C     0x181808    //gelblich
#define MARK1_C     0x180818    //lila-ich
//Stundenmarker dunkel
#define DARK_MARK_C_0   0x020202  //weiß
#define DARK_MARK_C_1   0x020202  //weiß
#define DARK_MARK0_C  0x030301    //gelblich
#define DARK_MARK1_C  0x030103    //lila-ich

#define HOUR_OFF 0x00ffff
#define MIN_OFF  0xff00ff
#define SEC_OFF  0xffff00

#ifdef ESP12
int NeoPin = 14;
#define HOSTNAME "NeoPixel12Clock"
#define AP_NAME "NeoPixel-Uhr"
#else
int NeoPin = 3; //eigentlich Rx-Pin, mal sehen...
#define HOSTNAME "NeoPixel01Clock"
#define AP_NAME "NeoPixel-Uhr"
#endif

int NeoLength = 60;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NeoLength, NeoPin, NEO_GRB + NEO_KHZ800);

#ifdef WITH_OLED
  SSD1306  display(0x3c, 0, 2); //ESP-01, SDA = 0, SCLK = 2
#endif

#define LOC_POS 64, 0
#define LOC1_POS  64, 20
#define TIME_POS 64, 0
#define SYNC_POS 64, 52


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
tmElements_t Ts;

//time_t getNtpTime();
//void digitalClockDisplay();
//void printDigits(int digits);
//void sendNTPpacket(IPAddress &address);

// what is our longitude (west values negative) and latitude (south values negative)
TimeLord City;

#if defined DD
#pragma message "ottO - Briesnitzr Hoehe 19a, Dresden"
float const LONGITUDE = 13.658055;
float const LATITUDE = 51.062369;
#elif defined WW
#pragma message "Silke - Jahnstrasse 1, Ransbach-Baumbach"
float const LONGITUDE = 7.735337;
float const LATITUDE = 50.467175;
#elif defined KO
#pragma message "Sarah - Loehrstrasse 42, Koblenz"
float const LONGITUDE = 7.593056;
float const LATITUDE = 50.359111;
#elif defined HB
#pragma message "Manni.42 - Postbrookstrasse 2f, Bremerhaven"
float const LONGITUDE = 8.624611;
float const LATITUDE = 53.530929;
#else
#error "Ort nicht definiert"
#endif

byte Heute[6];
byte PreDay = 0;
bool SonneDa = true;
bool PreSonneDa = false;
time_t TempTime = 0;
bool SyncFail = false;

// local time zone definition
// Central European Time (Frankfurt, Paris) from Timezone/examples/WorldClock/WorldClock.pde
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone LTZ(CEST, CET);    // this is the Timezone object that will be used to calculate local time
TimeChangeRule *tcr;        //pointer to the time change rule, use to get the TZ abbrev

time_t utc, local;
uint32_t LastSync = 0, NextSync = 0, TempSync = 0;
bool NewSync = false;
typedef struct tm ThisTime;

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
#if defined WITH_OLED
#pragma message "Mit OLED-Display"
  display.init();
  display.flipScreenVertically();
  display.clear();
#else
#pragma message "Ohne OLED-Display"
#endif
  pinMode(NeoPin, OUTPUT);
  digitalWrite(NeoPin, LOW);
  Serial.println();
  Serial.println("Connecting via WiFiManager ");
  strip.begin();
  strip.show();
  SetMarkerColor(); //irgendwie lila...
  //SetMarker(1);
  delay(500);
#if defined WITH_OLED
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 48, "Verbinde WLAN");
  display.display();
#endif
  WiFi.hostname(HOSTNAME);
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect(AP_NAME)) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

#if defined WITH_OLED
  display.clear();
  display.drawString(64, 48, "WLAN verbunden");
  display.drawXbm(34, 5, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display.display();
  delay(1000);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(LOC_POS, "Standort ist");
  display.setFont(Droid_Sans_Bold_36);
#if defined DD
  display.drawString(LOC1_POS, "ottO");
#elif defined HB
  display.drawString(LOC1_POS, "Manni.42");
#elif defined WW
  display.drawString(LOC1_POS, "Silke");
#elif defined KO
  display.drawString(LOC1_POS, "Sarah");
#else
  display.drawString(LOC1_POS, "Fehler");
#endif
  display.display();
  delay(2000);
#endif
  delay(2000);

  ClearStrip();
  SetMarker(1);

  Serial.print("IP address assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  Serial.println(F("Hole NTP-Zeit"));
  //setSyncProvider(getNTPTime);
  setTime(getNTPTime());
  if (timeStatus() != timeSet) {
    Serial.println("Uhr nicht mit NTP synchronisiert");
    while (1); //wdt provozieren
  }
  else
    Serial.println("NTP hat die Systemzeit gesetzt");
  LastSync = now();
  NextSync = now() + NEXT_SYNC;
  breakTime(NextSync, Ts);
  Ts.Second = 30;
  NextSync = makeTime(Ts);
  PrintSync(NextSync);
  ClearStrip();
  SunUpDown();
  PreSonneDa = ~SonneDa;

  // Sonnenauf- und -untergang bestimmen
  City.TimeZone(1 * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  // as long as the RTC never changes back and forth between DST and non-DST
  City.DstRules(3, 4, 10, 4, 60); //Umschaltung WZ/SZ am 4. Sonntag im ärz, zurück am 4. Sonntag im Oktober. SZ 60 min plus
  City.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are
  drawTime();
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  if (now() >= NextSync) {
    TempTime = getNTPTime();
    if (TempTime >= now()) {
      setTime(TempTime);
      SyncFail = false;
    }
    else
      SyncFail = true;
    NextSync = now() + NEXT_SYNC;
    NewSync = true;
    breakTime(NextSync, Ts);
    Ts.Second = 30;
    NextSync = makeTime(Ts);
    PrintSync(NextSync);
    Serial.print(now());
    Serial.print(" ");
    Serial.print(NextSync);
    Serial.println("  In if");
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
