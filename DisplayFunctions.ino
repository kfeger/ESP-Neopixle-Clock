void drawTime(void) {
#if defined WITH_OLED
  String TimeString = "";
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Droid_Sans_Bold_50);
  
  if (hour(now()) < 10)
    TimeString = "0" + String(hour(now()));
  else
    TimeString = String(hour(now()));
  if (minute(now()) < 10)
    TimeString += ":0" + String(minute(now()));
  else
    TimeString += ":" + String(minute(now()));
  display.drawString(TIME_POS, TimeString);

  display.setFont(Droid_Sans_12);
  if (SyncFail)
    TimeString = "Fehler! Neu um ";
  else
    TimeString = "NTP-Sync um ";
  if (hour(NextSync) < 10)
    TimeString += "0" + String(hour(NextSync));
  else
    TimeString += String(hour(NextSync));
  if (minute(NextSync) < 10)
    TimeString += ":0" + String(minute(NextSync));
  else
    TimeString += ":" + String(minute(NextSync));
  if (second(NextSync) < 10)
    TimeString += ":0" + String(second(NextSync));
  else
    TimeString += ":" + String(second(NextSync));
  display.drawString(SYNC_POS, TimeString);
  display.display();
#endif
}

void drawWiFiMessage (String ThisSSID) {
#if defined WITH_OLED
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Droid_Sans_16);
  display.drawString(64, 0, "Bitte WLAN über");
  display.drawString(64, 20, ThisSSID);
  display.drawString(64, 40, "konfigurieren");
  display.display();

#endif
}

