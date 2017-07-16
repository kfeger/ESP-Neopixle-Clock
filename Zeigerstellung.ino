void ClearStrip(void) {
  for (int i = 0; i < 60; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

int GetPixelAddress (int Number) {
  if ((Number >= 0) && (Number < 8))   // Nummer größer 0 und kleiner 8
    return (7 - Number);
  else if (Number && (Number < 60)) // Nummer größer 0 und kleiner 60
    return (67 - Number);
  else    // Fehler, Nummer ausserhalb des Berichs 0...59
    return (0);
}

void ResetPixel (int Position, uint32_t Farbe) {
  uint32_t OldColor, NewColor;
  OldColor = strip.getPixelColor(Position);
  NewColor = OldColor & Farbe;
  strip.setPixelColor(Position, NewColor);
  //strip.setPixelColor(Position, 0);
}

void SetPixel (int Position, uint32_t Farbe) {
  uint32_t OldColor, NewColor;
  OldColor = strip.getPixelColor(Position);
  NewColor = OldColor | Farbe;
  strip.setPixelColor(Position, NewColor);
}

void disp(int val) {
  Serial.print(val);
  Serial.print("...");
}

void SetHands(void) {
  // Laufzeit mit Int-Aus ca. 2ms
  noInterrupts();
  Serial.println(SonneDa);
  ClearStrip();
  int OnPos = 0;
  int OnPosH = 0, OnPosL = 0;
  int OnPosHH = 0, OnPosLL = 0;
  int OffPos = 0;
  int OffPosL = 0, OffPosLL = 0;
  int OffPosH = 0, OffPosHH = 0;

  int ShowHour;

  uint32_t OldColor = 0, NewColor = 0;

  // Sekunden
  OnPos = GetPixelAddress(NowSecond);
  if (NowSecond == 0)
    OffPos = GetPixelAddress(59);
  else
    OffPos = GetPixelAddress(NowSecond - 1);
  ResetPixel(OffPos, SEC_OFF);
  if (SonneDa) {
    ResetPixel(OffPos, SEC_OFF);
    SetPixel(OnPos, SEC_C);
  }
  else {
    ResetPixel(OffPos, SEC_OFF);
    SetPixel(OnPos, DARK_SEC_C);
  }

  // Minuten
  OnPos = GetPixelAddress(NowMinute);
  switch (NowMinute) {
    case 0:
      OffPosL = GetPixelAddress(58);
      OffPos = GetPixelAddress(59);
      OffPosH = GetPixelAddress(0);
      OnPosL = GetPixelAddress (59);
      OnPosH = GetPixelAddress(1);
      break;
    case 1:
      OffPosL = GetPixelAddress(59);
      OffPos = GetPixelAddress(0);
      OffPosH = GetPixelAddress(1);
      OnPosL = GetPixelAddress (0);
      OnPosH = GetPixelAddress(2);
      break;
    case 59:
      OffPosL = GetPixelAddress(57);
      OffPos = GetPixelAddress(58);
      OffPosH = GetPixelAddress(59);
      OnPosL = GetPixelAddress (58);
      OnPosH = GetPixelAddress(0);
      break;
    default:
      OffPosL = GetPixelAddress(NowMinute - 2);
      OffPos = GetPixelAddress(NowMinute - 1);
      OffPosH = GetPixelAddress(NowMinute);
      OnPosL = GetPixelAddress (NowMinute - 1);
      OnPosH = GetPixelAddress(NowMinute + 1);
      break;
  }
  if (SonneDa) {
    ResetPixel(OffPosL, MIN_OFF);
    ResetPixel(OffPos, MIN_OFF);
    ResetPixel(OffPosH, MIN_OFF);
    SetPixel(OnPosL, MIN_CL);
    SetPixel(OnPos, MIN_C);
    SetPixel(OnPosH, MIN_CL);
  }
  else {
    ResetPixel(OffPosL, MIN_OFF);
    ResetPixel(OffPos, MIN_OFF);
    ResetPixel(OffPosH, MIN_OFF);
    SetPixel(OnPosL, DARK_MIN_CL);
    SetPixel(OnPos, DARK_MIN_C);
    SetPixel(OnPosH, DARK_MIN_CL);
  }

  // Stunden
  ShowHour = (NowHour * 5) + (NowMinute / 12);
  OnPos = GetPixelAddress(ShowHour);
  if (!ShowHour)
    OffPos = GetPixelAddress(59);
  else
    OffPos = GetPixelAddress(ShowHour - 1);

  switch (ShowHour) {
    case 0:
      OffPosLL = GetPixelAddress(57);
      OffPosL = GetPixelAddress(58);
      OffPosH = GetPixelAddress(0);
      OffPosHH = GetPixelAddress(1);
      OnPosLL = GetPixelAddress (58);
      OnPosL = GetPixelAddress (59);
      OnPosH = GetPixelAddress(1);
      OnPosHH = GetPixelAddress (2);
      break;
    case 1:
      OffPosLL = GetPixelAddress(58);
      OffPosL = GetPixelAddress(59);
      OffPosH = GetPixelAddress(1);
      OffPosHH = GetPixelAddress(2);
      OnPosLL = GetPixelAddress (59);
      OnPosL = GetPixelAddress (0);
      OnPosH = GetPixelAddress(2);
      OnPosHH = GetPixelAddress (3);
      break;
    case 2:
      OffPosLL = GetPixelAddress(59);
      OffPosL = GetPixelAddress(0);
      OffPosH = GetPixelAddress(2);
      OffPosHH = GetPixelAddress(3);
      OnPosLL = GetPixelAddress (0);
      OnPosL = GetPixelAddress (1);
      OnPosH = GetPixelAddress(3);
      OnPosHH = GetPixelAddress (4);
      break;
    case 57:
      OffPosLL = GetPixelAddress(54);
      OffPosL = GetPixelAddress(55);
      OffPosH = GetPixelAddress(57);
      OffPosHH = GetPixelAddress(58);
      OnPosLL = GetPixelAddress (55);
      OnPosL = GetPixelAddress (56);
      OnPosH = GetPixelAddress(58);
      OnPosHH = GetPixelAddress (59);
      break;
    case 58:
      OffPosLL = GetPixelAddress(55);
      OffPosL = GetPixelAddress(56);
      OffPosH = GetPixelAddress(58);
      OffPosHH = GetPixelAddress(59);
      OnPosLL = GetPixelAddress (56);
      OnPosL = GetPixelAddress (57);
      OnPosH = GetPixelAddress(59);
      OnPosHH = GetPixelAddress (0);
      break;
    case 59:
      OffPosLL = GetPixelAddress(56);
      OffPosL = GetPixelAddress(57);
      OffPosH = GetPixelAddress(59);
      OffPosHH = GetPixelAddress(0);
      OnPosLL = GetPixelAddress (57);
      OnPosL = GetPixelAddress (58);
      OnPosH = GetPixelAddress(0);
      OnPosHH = GetPixelAddress (1);
      break;
    default:
      OffPosLL = GetPixelAddress(ShowHour - 3);
      OffPosL = GetPixelAddress(ShowHour - 2);
      OffPosH = GetPixelAddress(ShowHour);
      OffPosHH = GetPixelAddress(ShowHour + 1);
      OnPosLL = GetPixelAddress (ShowHour - 2);
      OnPosL = GetPixelAddress (ShowHour - 1);
      OnPosH = GetPixelAddress(ShowHour + 1);
      OnPosHH = GetPixelAddress(ShowHour + 2);
      break;
  }

  if (SonneDa) {
    ResetPixel(OffPosLL, HOUR_OFF);
    ResetPixel(OffPosL, HOUR_OFF);
    ResetPixel(OffPos, HOUR_OFF);
    ResetPixel(OffPosH, HOUR_OFF);
    ResetPixel(OffPosHH, HOUR_OFF);
    SetPixel(OnPosLL, HOUR_CLL);
    SetPixel(OnPosL, HOUR_CL);
    SetPixel(OnPos, HOUR_C);
    SetPixel(OnPosH, HOUR_CL);
    SetPixel(OnPosHH, HOUR_CLL);
  }
  else {
    ResetPixel(OffPosLL, HOUR_OFF);
    ResetPixel(OffPosL, HOUR_OFF);
    ResetPixel(OffPos, HOUR_OFF);
    ResetPixel(OffPosH, HOUR_OFF);
    ResetPixel(OffPosHH, HOUR_OFF);
    SetPixel(OnPosLL, DARK_HOUR_CLL);
    SetPixel(OnPosL, DARK_HOUR_CL);
    SetPixel(OnPos, DARK_HOUR_C);
    SetPixel(OnPosH, DARK_HOUR_CL);
    SetPixel(OnPosHH, DARK_HOUR_CLL);
  }
  SetMarker(0);
  strip.show();
  interrupts();
}

void SetMarker (byte slow) {
  int i;
  for (i = 0; i < 60; i += 5) {
    if (SonneDa)
      SetPixel(GetPixelAddress(i), MARK_C);
    else
      SetPixel(GetPixelAddress(i), DARK_MARK_C);
    if (slow) {
      strip.show();
      delay (84);
    }
  }
}

