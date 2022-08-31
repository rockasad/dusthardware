// DHT 21
#include "DHT.h"
#define DHTPIN 15
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

// RTC
#include <ThreeWire.h>
#include <RtcDS1302.h>
ThreeWire myWire(25, 33, 26); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

// SPIFFS
#include "FS.h"
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true
