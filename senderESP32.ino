
#define ARDUINO_ARCH_AVR

#ifndef HAS_WIFI
#define HAS_WIFI 1
#endif

#define USE_SH1106 1

#define EXT_NOTIFY_OUT 22
#define BUTTON_PIN 17

#define BATTERY_PIN 26
// ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION

#define USE_SX1262

// Definiere SPI-Pins für ESP32
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5

#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET 22
#define LORA_DIO1 4
#define LORA_BUSY 21

#include <WiFi.h>
//#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#include <WebServer.h>
#include <SoftwareSerial.h>

#include <RadioLib.h>

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI, RADIOLIB_DEFAULT_SPI_SETTINGS);

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent or received a packet, set the flag
  operationDone = true;
}

// ID für Identifikation
bool deviceidIsOn = false;
const byte deviceid[] = { 0x00, 0x00, 0x00, 0x00 };

bool useWiFiMode = false;
bool useLoRaMode = true;

// WiFi-Zugangsdaten
const char* ssid = "";
const char* password = "";

// Serverdaten
const char* server = "192.168.0.24";
const int port = 80;

// HTTP-Anfrage
String secret = "";

//WebServer server(80);

HardwareSerial SmlSerial(1);  // UART1 verwenden

String irDataBuffer; // String zur Speicherung der empfangenen IR-Daten

unsigned long previousMillis = 0;
const long interval = 1000;

byte bytesmltemp; //Speichern aktueller Wert
const byte StartSequence[] = { 0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01 }; //start sequence of SML
const byte StopSequence[]  = { 0x1B, 0x1B, 0x1B, 0x1B, 0x1A }; //end sequence of SML protocol
const byte PKwhSequence[] = { 0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xFF, 0x65, 0x00, 0x1C, 0x01, 0x04, 0x01, 0x62, 0x1E, 0x52, 0xFF, 0x69, 0x00, 0x00, 0x00, 0x00 }; //Sequence current "Gesamtverbrauch"
const byte PWSequence[] = { 0x77, 0x07, 0x01, 0x00, 0x10, 0x07, 0x00, 0xFF, 0x01, 0x01, 0x62, 0x1B, 0x52, 0x00, 0x55 }; //Sequence current "Wirkleistung"
int SequenceStep = 1; //Sequenzschritt
int SequenceIndex = 0; //Sequenzindex
byte SmlPW[4]; //Speichern der aktuellen Messwerte PW in HEX
byte SmlPKwh[4]; //Speichern der aktuellen Messwerte PKWH in HEX
//Variablen Berechnung Leistung
unsigned long currentPW = 0; //Aktuelle Leistung W Ausgabe DEC
unsigned long currentPKwh = 0; //Gesamtleistung Kwh Ausgabe DEC
double dblcurrentPW = 0; //currentPW/Count
double dblcurrentPKwh = 0; //currentPKwh/10000
int CountLoop[] = { 0, 20 }; //Alle X-Loops upload to DB
//Led`s

int changeTime = 2000;
int sleepTime = 20000;

#define uS_TO_mS_FACTOR 1000ULL  /* Conversion factor for milliseconds to microseconds */

void setup() {
  if (useLoRaMode == true) {
   setCpuFrequencyMhz(40);
  }
  if (useWiFiMode == true) {
   setCpuFrequencyMhz(80);
  }

  //pinMode(IR_RECEIVE_PIN, INPUT);
  //radio.standby();
  Serial.begin(9600);
  delay(2000);

  if (useLoRaMode == true) {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    Serial.print(F("[SX1262] Initializing ... "));
    int state = state = radio.begin(869.4, 250.0, 7, 5, 0x12, 1);;
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true);
    }

    // set the function that will be called
    // when new packet is received
    radio.setDio1Action(setFlag);

    #if defined(INITIATING_NODE)
      // send the first packet on this node
      Serial.print(F("[SX1262] Sending first packet ... "));
      transmissionState = radio.startTransmit("Hello World!"); //Ist das noch nötig für den Test?
      transmitFlag = true;
    #else
      // start listening for LoRa packets on this node
      Serial.print(F("[SX1262] Starting to listen ... "));
      state = radio.startReceive(); //Ist das noch nötig für den Test?
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
      } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
      }
    #endif

    SmlSerial.begin(9600, SERIAL_8N1, 16, 17); // Initialisierung der Hardware-Seriellen Schnittstelle 
  }
}
 String hexValue = "";

bool WiFiiConnecting = false;
unsigned long scriptTimer = 0;
//RTC_DATA_ATTR unsigned long scriptTimerAfterSleep = 0; // Wenn ich weiß wie ich das mit RTC mache.
unsigned long scriptTimerAfterSleep = 0;
void loop() {
  //server.handleClient();
  unsigned long currentMillis = millis();
  if(scriptTimerAfterSleep == 0)  {
    scriptTimer = currentMillis - 3200;
  } else {
    scriptTimer = scriptTimerAfterSleep;
  }

  if (WiFiiConnecting == false && useWiFiMode == true) {
     WiFiiConnecting = true;
     WiFi.mode(WIFI_STA);
     WiFi.begin(ssid, password);

    if ((SequenceStep == 1) && (useLoRaMode == true || (WiFi.status() == WL_CONNECTED))) { // Ich will das Später vielleicht wieder hinzufügen.
      SequenceStep ++; //Sequenzschritt 1-->2
    }
  }

  if (SequenceStep == 1 && useLoRaMode == true) {
    SequenceStep ++; //Sequenzschritt 1-->2
  }

  if (SmlSerial.available()) {
    bytesmltemp = SmlSerial.read(); //Temp-Speicher der SML Nachricht
    switch (SequenceStep) {
      case 2: // Suche Start-Sequence
        if (bytesmltemp == StartSequence[SequenceIndex]) {
          SequenceIndex ++;
          if (SequenceIndex == 8) {
            SequenceIndex = 0;
            SequenceStep ++; //Sequenzschritt 2-->3
            //digitalWrite(LedOkPin, HIGH); //LED Prozess SML Running OK
          }
        } else if (bytesmltemp != StartSequence[SequenceIndex]) {
          SequenceIndex = 0;
        }
        break;
      case 3: // Suche PKwh-Sequence
        if (bytesmltemp == PKwhSequence[SequenceIndex]) {
          SequenceIndex ++;
          if (SequenceIndex == 23) {
            SequenceIndex = 0;
            SequenceStep ++; //Sequenzschritt 3-->4
          }
        } else if (bytesmltemp != PKwhSequence[SequenceIndex]) {
          SequenceIndex = 0;
        }
        break;
      case 4: // Gesamtleistung Speichern
        SmlPKwh[SequenceIndex] = bytesmltemp;
        SequenceIndex ++;
        if (SequenceIndex == 4) {
          SequenceIndex = 0;
          SequenceStep ++; //Sequenzschritt 4-->5
        }
        break;
      case 5: // Suche PW-Sequence
        if (bytesmltemp == PWSequence[SequenceIndex]) {
          SequenceIndex ++;
          if (SequenceIndex == 15) {
            SequenceIndex = 0;
            SequenceStep ++; //Sequenzschritt 5-->6
          }
        } else if (bytesmltemp != PWSequence[SequenceIndex]) {
          SequenceIndex = 0;
        }
        break;
      case 6: // Aktuelle Wirkleistung speichern
        SmlPW[SequenceIndex] = bytesmltemp;
        SequenceIndex ++;
        if (SequenceIndex == 4) {
          SequenceIndex = 0;
          SequenceStep ++; //Sequenzschritt 6-->7
        }
        break;
      case 7: // Suche Stop-Sequence
        if (bytesmltemp == StopSequence[SequenceIndex]) {
          SequenceIndex ++;
          if (SequenceIndex == 5) {
            SequenceIndex = 0;
            SequenceStep ++; //Sequenzschritt 7-->8
          }
        } else if (bytesmltemp != StopSequence[SequenceIndex]) {
          SequenceIndex = 0;
        }
        break;
      case 8: // Letzte 3 Byte des Protokolls abwarten
        SequenceIndex ++;
        if (SequenceIndex == 3) {
          SequenceIndex = 0;
          SequenceStep ++; //Sequenzschritt 8-->9
        }
        break;
    }
  }
  //Serial.print(String(SequenceStep));

  //Daten Zusammenstellen
  if (SequenceStep == 9) {
    //Gesamtleistung Zähler
    currentPKwh = ((long)SmlPKwh[0] << 24 | (long)SmlPKwh[1] << 16 | (long)SmlPKwh[2] << 8 | (long)SmlPKwh[3]);
    dblcurrentPKwh = (double)currentPKwh / 10000;
    //Aktuelle leistung in Watt
    currentPW = ((long)SmlPW[0] << 24 | (long)SmlPW[1] << 16 | (long)SmlPW[2] << 8 | (long)SmlPW[3]);
    dblcurrentPW = dblcurrentPW + currentPW;
    /*
    CountLoop[0] ++;
    if (CountLoop[0] == CountLoop[1]) {
      dblcurrentPW = dblcurrentPW / CountLoop[1];
      SequenceStep ++; //Sequenzschritt 9-->10
    }else{ //Sequenzstep 1 bis Anzahl Loop erreicht
       SequenceStep = 1; //Sequenzschritt 10-->1
       //digitalWrite(LedOkPin, LOW); //LED Prozess SML Running OK
    }
    */
    SequenceStep ++; 
  }

  //Verbindung zur Datenbank prüfen und Paket absenden
  if (SequenceStep == 10) {
    //Serial.print(String(bytesmltemp));
      irDataBuffer = String(dblcurrentPW) + " " + String(dblcurrentPKwh);
        if (irDataBuffer != "") {
          //Serial.print(String(hexValue));
          hexValue ="";
          if (currentMillis - previousMillis >= interval) { //Das ist unnötig geworden.
            if (useWiFiMode == true) { //Könnte man eigentlich auch hier schreiben mit dem Verbinden, aber bringt so zeitlich einen Vorteil.
              WiFiClient client;
              //if (port == 443) {
              //    WiFiClientSecure client;
              //}
              if (client.connect(server, port)) {
                Serial.println("HTTP-Anfrage senden");
                String url = "/stromzeahler.php?password=" + secret + "&power_consumption=" + String(currentPW) + "&counter_reading=" + String(currentPKwh);
                client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                            "Host: " + server + "\r\n" +
                            "Connection: close\r\n\r\n");
                              // Antwort vom Server lesen
              while (client.available()) {
                String line = client.readStringUntil('\r');
                Serial.print(line);
              }

              Serial.println();
              Serial.println("Verbindung geschlossen");
              } else {
                Serial.println("Verbindung fehlgeschlagen");
              }
              WiFi.disconnect(true);
              WiFi.mode(WIFI_OFF);
              int scriptTimeSleep = (sleepTime - (millis() - scriptTimer));
              esp_sleep_enable_timer_wakeup(scriptTimeSleep * uS_TO_mS_FACTOR);
              scriptTimerAfterSleep = millis();
              Serial.println("Going to sleep now");
              Serial.flush();
              esp_deep_sleep_start();
            }
            if (useLoRaMode == true) {
              irDataBuffer = irDataBuffer.substring(0, 255);
              //transmissionState = radio.startTransmit(irDataBuffer);
              int sizeSmlPKwh = sizeof(SmlPKwh);
              int sizeSmlPW = sizeof(SmlPW);
              int combinedSize = sizeSmlPKwh + sizeSmlPW;
              byte sendArray[combinedSize];
              if (deviceidIsOn == true) {
                for (int i = 0; i < sizeof(deviceid); i++) {
                  sendArray[i] = deviceid[i];
                }
              }
              for (int i = 0; i < sizeSmlPKwh; i++) {
                sendArray[i] = SmlPKwh[i];
              }
              for (int i = 0; i < sizeSmlPW; i++) {
                sendArray[sizeSmlPKwh + i] = SmlPW[i];
              }
              transmissionState = radio.startTransmit(sendArray , combinedSize);
              Serial.print("[SX1262] Sending packet ... " + irDataBuffer);
              transmitFlag = true;
              irDataBuffer = "";
              previousMillis = currentMillis;
              // Geht auch auf einmal nicht mehr.
              while (transmitFlag) {
                if(operationDone) {
                  operationDone = false;
                  if(transmitFlag) {
                    if (transmissionState == RADIOLIB_ERR_NONE) { //Lora muss glaub nach dem runter und hoch takten neu gestartet werden.
                      Serial.println(F("transmission finished!"));
                    } else {
                      Serial.print(F("failed, code "));
                      Serial.println(transmissionState);
                    }
                    // listen for response
                    //radio.startReceive();
                    transmitFlag = false;
                  }
                }
                delay(1);
              }
              radio.sleep(false);

              int scriptTimeSleep = (sleepTime - (millis() - scriptTimer));
              if(scriptTimeSleep > 0) {
                  Serial.println(scriptTimeSleep);

                esp_sleep_enable_timer_wakeup(scriptTimeSleep * uS_TO_mS_FACTOR);
              } else {
                esp_sleep_enable_timer_wakeup(sleepTime-1000 * uS_TO_mS_FACTOR);
              }
              scriptTimerAfterSleep = millis();
              //radio.standby();
              Serial.println("Going to sleep now"); //Funktioniert ohne das nicht, wieso auch immmer.
              Serial.flush();
              esp_deep_sleep_start();
            }
          }
        } else {
          previousMillis = currentMillis;
        }
      CountLoop[0] = 0;
      dblcurrentPW = 0;
      SequenceStep = 1; //Sequenzschritt 10-->1
      //digitalWrite(LedOkPin, LOW); //LED Prozess SML Running OK
    }
}
