void SunUpDown (void) {
  if (day() != PreDay) {
    //PreDay = day();
    Heute[0] = 0; Heute[1] = 0; Heute[2] = 12; Heute[3] = day(); Heute[4] = month(); Heute[5] = year() - 2000; // store today's date (at noon) in an array for TimeLord to use
    int JetztZeit = hour() * 60 + minute();
    int SonneAufZeit, SonneUnterZeit;
    if (Dresden.SunRise(Heute)) {
      if (LTZ.locIsDST(now())) {
        Heute[tl_hour] += 1;
        Serial.print("Sommer, ");
      }
      else
        Serial.print("Winter, ");
      Serial.print("Sonnenaufgang heute um ");
      Serial.print(Heute[tl_hour]);
      printDigits(Heute[tl_minute]);
      Serial.println();
      SonneAufZeit = Heute[tl_hour] * 60 + Heute[tl_minute];
    }
    if (Dresden.SunSet(Heute)) {
      if (LTZ.locIsDST(now())) {
        Heute[tl_hour] += 1;
        Serial.print("Sommer, ");
      }
      else
        Serial.print("Winter, ");
      Serial.print("Sonnenuntergang heute um ");
      Serial.print(Heute[tl_hour]);
      printDigits(Heute[tl_minute]);
      Serial.println();
      SonneUnterZeit = Heute[tl_hour] * 60 + Heute[tl_minute];
    }
    //Serial.print("Jetz = "); Serial.print(JetztZeit); Serial.print(" Auf = "); Serial.print(SonneAufZeit); Serial.print(" Unter = "); Serial.println(SonneUnterZeit);
    if ((JetztZeit > SonneAufZeit) && (JetztZeit < SonneUnterZeit)) {
      SonneDa = true;
      if (!PreSonneDa)
        ClearStrip();
      PreSonneDa = SonneDa;
    }
    else if ((JetztZeit < SonneAufZeit) || (JetztZeit > SonneUnterZeit)) {
      SonneDa = false;
      if (PreSonneDa)
        ClearStrip();
      PreSonneDa = SonneDa;
    }
    Serial.println();
  }
}

