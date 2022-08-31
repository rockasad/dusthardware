

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  //  snprintf_P(datestring,
  //             countof(datestring),
  //             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
  //             dt.Month(),
  //             dt.Day(),
  //             dt.Year(),
  //             dt.Hour(),
  //             dt.Minute(),
  //             dt.Second() );
  //  Serial.print(datestring);
  int yearInt = int(dt.Year());
  int monthInt = int(dt.Month());
  int dayInt = int(dt.Day());
  int hrInt = int(dt.Hour());
  int mnInt = int(dt.Minute());
  yyyy = "";
  mm = "";
  dd = "";
  HH = "";
  MM = "";
  Sec = "";
  yyyy = dt.Year();
  (monthInt < 10) ? mm = "0" + String(monthInt) : mm = String(monthInt);
  (dayInt < 10) ? dd = "0" + String(dayInt) : dd = String(dayInt);
  (hrInt < 10) ? HH = "0" + String(hrInt) : HH = String(hrInt);
  (mnInt < 10) ? MM = "0" + String(mnInt) : MM = String(mnInt);

  //  yyyy = dt.Year();
  //  mm = int(dt.Month());
  //  dd = int(dt.Day());
  //  HH = int(dt.Hour());
  //  MM = int(dt.Minute());
    Sec = int(dt.Second());
}

void initsRTC() {
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
}
