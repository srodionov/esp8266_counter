unsigned long     btnPressStart   = 0;
unsigned long     btnPressEnd     = 0;

/** Is this an IP? */
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

//reset button
void resetButton(){
  if (digitalRead(PIN_BUTTON) == LOW){
    btnPressStart = millis();
  } 
  else 
  {
    if (btnPressStart > 0) btnPressEnd = millis();
  }

  if (btnPressStart != 0)
  {
    if (btnPressEnd != 0) ESP.restart();
    if ((millis() - btnPressStart) > 5000) 
    {
      Serial.println("RESET");
      clearCredentials();
      ESP.restart();
    }
  }
}
  

