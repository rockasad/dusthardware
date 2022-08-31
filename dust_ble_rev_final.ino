
bool saveSuccess = false;
String yyyy = "";
String mm = "";
String dd = "";
String HH = "";
String MM = "";
String Sec = "";
#include "Header.h"
#include "RTCs.h"
#include "SPIFFSs.h"
#include "Configs.h"
#include <WiFi.h>

unsigned long postDHT = millis();
unsigned long postPMS = millis();
unsigned long postRTC = millis();
unsigned long postSampling = millis();

unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

#define RXD2 16
#define TXD2 17


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

String name_bt = "";
//String name_bt = "Dust_30:AE:A4:07:0D:64" + WiFi.macAddress();
String command = "";
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
bool _sendLog = false;
unsigned long delaySendLog = millis();

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        command = "";
        for (int i = 0; i < rxValue.length(); i++) {
          command += rxValue[i];
        }
        if (command.indexOf("AT+REQ") != -1) {
          _sendLog = true;
          delaySendLog = millis();
        } else if (command.indexOf("AT+LOGAPP=OK") != -1) {
          deleteFile(SPIFFS, "/datalogs.txt");
        }

        Serial.println(command);
        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  dht.begin();
  initsRTC();
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  name_bt = "Dust_01";
  BLEDevice::init(name_bt.c_str());
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                          );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
}
float h = 0;
float t = 0;

String stringFormat(int num) {
  if (num >= 10 && num < 100) {
    return "0" + String(num);
  } else if (num >= 1000) {
    return "999";
  }
  return "00" + String(num);
}

void loop() {
  if (millis() - postDHT >= 2000) {
    float hum = dht.readHumidity();
    float tem = dht.readTemperature();
    if (isnan(hum) || isnan(tem)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      postDHT = millis();
      return;
    }
    h = hum;
    t = tem;
    postDHT = millis();
    //    Serial.print(F("Humidity: "));
    //    Serial.print(h);
    //    Serial.print(F("%  Temperature: "));
    //    Serial.print(t);
    //    Serial.print(F("°C"));
    //    Serial.println();
  }

  if (millis() - postPMS >= 1000) {
    int index = 0;
    char value;
    char previousValue;
    while (Serial2.available()) {
      value = Serial2.read();
      if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d)) {
        //        Serial.println("Cannot find the data header.");
        break;
      }

      if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
        previousValue = value;
      }
      else if (index == 5) {
        pm1 = 256 * previousValue + value;
        //        Serial.print("{ ");
        //        Serial.print("\"pm1\": ");
        //        Serial.print(pm1);
        //        Serial.print(" ug/m3");
        //        Serial.print(", ");
      }
      else if (index == 7) {
        pm2_5 = 256 * previousValue + value;
        //        Serial.print("\"pm2_5\": ");
        //        Serial.print(pm2_5);
        //        Serial.print(" ug/m3"); AQI
        //        Serial.print(", ");
      }
      else if (index == 9) {
        pm10 = 256 * previousValue + value;
        //        Serial.print("\"pm10\": ");
        //        Serial.print(pm10);
        //        Serial.print(" ug/m3");
        //        Serial.println(" }");
      } else if (index > 15) {
        break;
      }
      index++;
    }
    while (Serial2.available()) Serial2.read();
    postPMS = millis();
  }

  if (millis() - postRTC >= 1000) {
    RtcDateTime now = Rtc.GetDateTime();

    printDateTime(now);
    //    Serial.println();

    //    Serial.print(yyyy);
    //    Serial.print(mm);
    //    Serial.print(dd);
    //    Serial.print(HH);
    //    Serial.print(MM);
    //    Serial.print("    ");
    if (!now.IsValid())
    {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
    postRTC = millis();
  }
  if (!deviceConnected && oldDeviceConnected) {
    pServer->startAdvertising(); // restart advertising
    Serial.println("DisConnected");
    _sendLog = false;
    delaySendLog = millis();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Connected");
    delaySendLog = millis();
    _sendLog = false;

    oldDeviceConnected = deviceConnected;
  }
  if (_sendLog) {
    if (millis() - delaySendLog >= 5000) {
      //          readFile(SPIFFS, "/datalogs.txt");
      File file = SPIFFS.open("/datalogs.txt");
      if (!file || file.isDirectory()) {
        delaySendLog = millis();
        _sendLog = false;
        Serial.println("- failed to open file for reading");
        return;
      }

      Serial.println("================= START READ FILE =================");
      String strslog = "";
      while (file.available()) {
        char c = file.read();
        strslog += String(c);
        //        Serial.write(c);

        //      c.toCharArray(vals, 1);
        //        pTxCharacteristic->setValue(a.c_str());
        //        pTxCharacteristic->notify();
      }
      Serial.print("1> Length : ");
      Serial.print(strslog.length());
      int count = 0;
      String strSendlog = "";
      for (int i = 0; i < strslog.length(); i++) {
        char c = strslog.charAt(i);
        if (c != '\n')
          strSendlog += String(c);
        if (c == '#') {
          //          H202208111100E00100300332.7#
          String data1 = strSendlog.substring(strSendlog.indexOf("H"), strSendlog.indexOf("E"));
          String data2 = strSendlog.substring(strSendlog.indexOf("E") , strSendlog.indexOf("#") + 1);
          Serial.print("data1 = ");
          Serial.print(data1);
          Serial.print("      data2 = ");
          Serial.println(data2);
          //          strSendlog = "";
          char valsTEMP [13];
          int lenTEMP = data1.length() + 1;
          data1.toCharArray(valsTEMP, lenTEMP);
          pTxCharacteristic->setValue(valsTEMP);
          pTxCharacteristic->notify();
          //                    strSendlog = "";
          delay(200);
          char valsTEMP2 [15];
          int lenTEMP2 = data2.length() + 1;
          data2.toCharArray(valsTEMP2, lenTEMP2);
          pTxCharacteristic->setValue(valsTEMP2);
          pTxCharacteristic->notify();
          strSendlog = "";
          delay(200);
        }
        //        if (strSendlog.length() >= 15) {
        //          char valsTEMP [15];
        //          int lenTEMP = strSendlog.length() + 1;
        //          strSendlog.toCharArray(valsTEMP, lenTEMP);
        //          pTxCharacteristic->setValue(valsTEMP);
        //          pTxCharacteristic->notify();
        //          strSendlog = "";
        //          delay(100);
        //        }
      }
      pTxCharacteristic->setValue("AT+LOG=OK");
      pTxCharacteristic->notify();
      //      pTxCharacteristic->setValue("AT+LOG=OK");
      //      pTxCharacteristic->notify();
      //      pTxCharacteristic->setValue("AT+LOG=OK");
      //      pTxCharacteristic->notify();
      //      pTxCharacteristic->setValue("AT+LOG=OK");
      //      pTxCharacteristic->notify();
      Serial.print("2> Length : ");


      Serial.println();
      Serial.println("================= END =================");

      delaySendLog = millis();
      _sendLog = false;
    }
  }
  if (millis() - postSampling >= 1000) {
    char vals[20];
    //      String val =  String(pm1) + "," + String(pm2_5) + "," + String(pm10) + "," + String(t, 2) + "," + String(h, 2);
    //    String val =  "&" + String(pm1) + "," + String(pm2_5) + "," + String(pm10) + "," + String(t, 1) + "#";
    //      String val = "#PM1=" + String(pm2_5, 0) + "$";
    //      String val = "#PM25=" + String(pm2_5, 0) + "$";
    //      String val = "#PM10=" + String(pm2_5, 0) + "$";
    //      String val = "#TEMP=" + String(t, 1) + "$";
    String tempString = "";
    if (t < 10.0) {
      tempString = "0" + String(t, 1);
    } else if (t < 100.0) {
      tempString = String(t, 1);
    } else {
      tempString = "99.9";
    }
    String val1  = "H" + yyyy + mm + dd + HH + MM;
    String val2  = "E" + stringFormat(pm1) + stringFormat(pm2_5) + stringFormat(pm10) + tempString + "#";
    String datalogsTxt = val1 + val2 + "\r\n";
    if (deviceConnected) {
      String valPM1 = "#PM1=" + String(pm1) + "$";
      char valsPM1 [20];
      int lenPM1 = valPM1.length() + 1;
      valPM1.toCharArray(valsPM1, lenPM1);
      pTxCharacteristic->setValue(valsPM1);
      pTxCharacteristic->notify();

      String valPM25 = "#PM25=" + String(pm2_5) + "$";
      char valsPM25 [20];
      int lenPM25 = valPM25.length() + 1;
      valPM25.toCharArray(valsPM25, lenPM25);
      pTxCharacteristic->setValue(valsPM25);
      pTxCharacteristic->notify();

      String valPM10 = "#PM10=" + String(pm10) + "$";
      char valsPM10 [20];
      int lenPM10 = valPM10.length() + 1;
      valPM10.toCharArray(valsPM10, lenPM10);
      pTxCharacteristic->setValue(valsPM10);
      pTxCharacteristic->notify();

      String valTEMP = "#TEMP=" + String(t, 1) + "$";
      char valsTEMP [20];
      int lenTEMP = valTEMP.length() + 1;
      valTEMP.toCharArray(valsTEMP, lenTEMP);
      pTxCharacteristic->setValue(valsTEMP);
      pTxCharacteristic->notify();

      //      int len = val.length() + 1;
      //      val.toCharArray(vals, len);
      //      pTxCharacteristic->setValue(vals);
      //      pTxCharacteristic->notify();
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //    if ((MM.toInt() == 0 || MM.toInt() == 30) && (Sec.toInt() == 0 || Sec.toInt() == 1)) { ทุกๆ ครึ่งชั่วโมง
    if (MM.toInt() == 0 && (Sec.toInt() == 0 || Sec.toInt() == 1)) { // ทุกๆ 1 ชั่วโมง
      //    if ((MM.toInt() == 0 ||
      //         MM.toInt() == 5 ||
      //         MM.toInt() == 10 ||
      //         MM.toInt() == 15 ||
      //         MM.toInt() == 20 ||
      //         MM.toInt() == 25 ||
      //         MM.toInt() == 30 ||
      //         MM.toInt() == 35 ||
      //         MM.toInt() == 40 ||
      //         MM.toInt() == 45 ||
      //         MM.toInt() == 50 ||
      //         MM.toInt() == 55)
      //        && (Sec.toInt() == 0 || Sec.toInt() == 1)) {
      appendFile(SPIFFS, "/datalogs.txt", datalogsTxt.c_str());
      delay(2000);
    }

    Serial.print(datalogsTxt);
    postSampling = millis();
  }
}
