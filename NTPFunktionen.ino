/*-------- NTP code ----------*/
/*

  This is from RFC 5905:

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0  |LI | VN  |Mode |    Stratum     |     Poll      |  Precision   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4  |                         Root Delay                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8  |                         Root Dispersion                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  12  |                          Reference ID                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  16  |                                                               |
  +                     Reference Timestamp (64)                  +
  20  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  24  |                                                               |
  +                      Origin Timestamp (64)                    +
  28  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  32  |                                                               |
  +                      Receive Timestamp (64)                   +
  36  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  40  |                                                               |
  +                      Transmit Timestamp (64)                  +
  44  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  .                                                               .
  .                    Extension Field 1 (variable)               .
  .                                                               .
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  .                                                               .
  .                    Extension Field 2 (variable)               .
  .                                                               .
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Key Identifier                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                            dgst (128)                         |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

//holt ein NTP-Paket und gibt die Daten als unsigned long UNIX-epoch zurück

// send an NTP request to the time server at the given address

//#define DEBUG 1

long int sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

}

//const unsigned int assumedDelay = 25;
#define assumedDelay 25

long int getNTPTime() {
  //Wire.endTransmission(true);
  Serial.println("Getting NTP-Time!");
  if (WiFi.status() != WL_CONNECTED) { //WLAN verloren?
    Serial.println(F("WLAN verloren, ESP macht Reset"));
    delay(2000);
    digitalWrite(0, HIGH);
    digitalWrite(2, HIGH);  //Dann klappts auch mit nem Restart...
    ESP.restart();  //ja, restart ESP
    delay(2000);
  }
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  while (udp.parsePacket() > 0) ; // discard any previously received packets
#if DEBUG
  Serial.println("Transmit NTP Request");
#endif
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
#if DEBUG
      Serial.println("Receive NTP Response");
#endif
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      unsigned long secFraction;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      // convert four bytes starting at location 44 to a long integer
      secFraction =  (unsigned long)packetBuffer[44] << 24;
      secFraction |= (unsigned long)packetBuffer[45] << 16;
      secFraction |= (unsigned long)packetBuffer[46] << 8;
      secFraction |= (unsigned long)packetBuffer[47];
      time_t utc = secsSince1900 - 2208988799UL; // 2208988800 seconds between 1970-01-01 and 1900-01-01, we add one second to the result because we wait until the next second to return
      int secFractionMs = (secFraction / 4294967) + assumedDelay; // this produces milliseconds out of the 32-bit fractional part of the ntp timestamp; rounding errors should be negligible
      if (secFractionMs > 1000) {
        secFractionMs -= 1000;
        utc++;
      }
      delay(1000 - secFractionMs);
      recalculate = 1;
      return LTZ.toLocal(utc, &tcr);
    }
  }
#if DEBUG
  Serial.println("No NTP Response : -(");
#endif
  /*digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);  //Dann klappts auch mit nem Restart...
  ESP.restart();  //Keine ntp-Zeit, reset*/
  return 0; // return 0 if unable to get the time
}

