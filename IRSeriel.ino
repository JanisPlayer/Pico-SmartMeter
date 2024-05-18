
#define ARDUINO_ARCH_AVR

#ifndef HAS_WIFI
#define HAS_WIFI 1
#endif

#define USE_SH1106 1

// default I2C pins:
// SDA = 4
// SCL = 5

// Recommended pins for SerialModule:
// txd = 8
// rxd = 9

#define EXT_NOTIFY_OUT 22
#define BUTTON_PIN 17

#define BATTERY_PIN 26
// ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION

#define USE_SX1262

#undef LORA_SCK
#undef LORA_MISO
#undef LORA_MOSIdio0
#undef LORA_CS

#define LORA_SCK 10
#define LORA_MISO 12
#define LORA_MOSI 11
#define LORA_CS 3

#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET 15
#define LORA_DIO1 20
#define LORA_DIO2 2
#define LORA_DIO3 RADIOLIB_NC

#ifdef USE_SX1262
#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif

#include <WiFi.h>
//#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#include <WebServer.h>
#include <SoftwareSerial.h>
#include <pico/stdlib.h>
#include "hardware/vreg.h"

#include <RadioLib.h>
//#include "C:\Users\janis\Documents\Arduino\libraries\PicoExtras\src\rp2_common\pico_sleep\include\pico\sleep.h"

/*
// for lightsleep
#include "hardware/clocks.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/syscfg.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"
#include "hardware/sync.h"

void lightsleep( uint32_t delay_ms)
{
    const uint32_t xosc_hz = XOSC_MHZ * 1000000;
    //  bool use_timer_alarm =  delay_ms < (1ULL <<32) /1000;
    
    // wait for debug output
    uart_default_tx_wait_blocking();
    
    // Disable USB and ADC clocks.
    clock_stop(clk_usb);
    clock_stop(clk_adc);

    // CLK_REF = XOSC
    clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0, xosc_hz, xosc_hz);

    // CLK_SYS = CLK_REF
    clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, xosc_hz, xosc_hz);

    // CLK_RTC = XOSC / 256
    clock_configure(clk_rtc, 0, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, xosc_hz, xosc_hz / 256);

    // CLK_PERI = CLK_SYS
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, xosc_hz, xosc_hz);

    // Disable PLLs.
    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    // Disable ROSC.
    rosc_hw->ctrl = ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB;

    uint32_t sleep_en0 = clocks_hw->sleep_en0;
    uint32_t sleep_en1 = clocks_hw->sleep_en1;
    clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS;
 //   if (use_timer_alarm) {
        // Use timer alarm to wake.
        clocks_hw->sleep_en1 = CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS;
        timer_hw->alarm[3] = timer_hw->timerawl + delay_ms * 1000;
 //   } else {
 //      // TODO: Use RTC alarm to wake.
 //      clocks_hw->sleep_en1 = 0;
 //   }
    scb_hw->scr |= M0PLUS_SCR_SLEEPDEEP_BITS;
    __wfi();
    scb_hw->scr &= ~M0PLUS_SCR_SLEEPDEEP_BITS;
    clocks_hw->sleep_en0 = sleep_en0;
    clocks_hw->sleep_en1 = sleep_en1;

    // Enable ROSC.
    rosc_hw->ctrl = ROSC_CTRL_ENABLE_VALUE_ENABLE << ROSC_CTRL_ENABLE_LSB;

    // Bring back all clocks.
    clocks_init();
//  set_sys_clock_khz(130000, true);
}

// for sleep_hms
#include "hardware/structs/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/clocks.h"

void callback(){}	
void sleep_hms(uint8_t h, uint8_t m, uint8_t s)
{
    uint orginsave = scb_hw->scr;
    uint clock0 = clocks_hw->sleep_en0;
    uint clock1 = clocks_hw->sleep_en1;	
    static datetime_t base = {2020,1,1,0,0,0,0};	  		
    datetime_t alarm = base;
    
    uart_default_tx_wait_blocking();
    
    rtc_init();
    rtc_set_datetime(&base);
    alarm.hour = h;
    alarm.min = m;
    alarm.sec = s;
    
    scb_hw->scr = orginsave | M0PLUS_SCR_SLEEPDEEP_BITS;
    
    sleep_run_from_xosc();		
    sleep_goto_sleep_until(&alarm, &callback);
    
    //rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    scb_hw->scr = orginsave;	
    clocks_hw->sleep_en0 = clock0;
    clocks_hw->sleep_en1 = clock1;
    clocks_init();
    //stdio_init_all();
}

class Sleep {
public:
    enum  MODE { NORMAL = 1, SLEEP = 2, DORMANT = 4 }; 
    // class implemented using the
    // Singleton design pattern.
    static Sleep& instance() {
        static Sleep _instance;
        return _instance;
    }

    // No copy constructor or assignment operator = 
    Sleep(Sleep const&)           = delete;
    void operator=(Sleep const&)  = delete;

    // used to (re-)configure 
    void configureDormant(void (*setupfnc)(), void (*loopfnc)(), 
                    uint8_t WAKEUP_PIN, bool edge, bool active);                                           
                                            
    // get current mode
    MODE get_mode();

    // display current frequencies
    void measure_freqs();

    // saves clock registers
    // and initializes RTC for sleep mode
    void before_sleep();

    // function responsible for sleep
    // sleep ends with high edge (DORMANT) or when alarm time is reached (SLEEP)
    void start_sleep(); 

    // sleep recovery
    void after_sleep();

    // kind of run shell: calls _setup once, and 
    // implements infinite loop where sleep phases
    // are initiated and _loop is being called
    // in each iteration.
    // Actually, this function implements an
    // event loop:
    void run();

private:
    uint _scb_orig;     // clock registers saved before DORMANT or SLEEP mode
    uint _en0_orig;     // ""
    uint _en1_orig;     // ""
    MODE _mode;         // can be SLEEP, DORMANT or NORMAL

    uint _wakeup_pin; bool _edge; bool _active;             
    datetime_t _init_time;    // initial time set
    datetime_t _alarm_time;   // alarm time
    void (* _setup)();        // user-defined setup function - called once
    void (* _loop) ();        // user-defined loop function: called in each                                    // iteration
    Sleep(){};                // private constructor
};

void Sleep::run() {
    _setup(); // called once
    while(true) {
        if (_mode != MODE::NORMAL) {
            before_sleep();
            start_sleep(); 
            after_sleep(); // calls _loop in each iteration
        }
        else {
            _loop(); // NORMAL mode =>
            // must explicitly call _loop
        }
    }
}*/

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_DIO2, SPI1, RADIOLIB_DEFAULT_SPI_SETTINGS);

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

SoftwareSerial SmlSerial(1, 0); // RX, TX

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

void setup() {
  //pinMode(IR_RECEIVE_PIN, INPUT);
  //radio.standby();
  Serial.begin(9600);
  delay(2000);

  if (useLoRaMode == true) {
    // from GUVWAF's post:
    SPI1.setSCK(LORA_SCK);
    SPI1.setTX(LORA_MOSI);
    SPI1.setRX(LORA_MISO);
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    SPI1.begin(false);

    // initialize SX1262 with default settings
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
    #endif  //SmlSerial.begin(9600);

    SmlSerial.begin(9600, SERIAL_8N1); // Initialisierung der Hardware-Seriellen Schnittstelle 
  }
  /*
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  */

  /*
  server.on("/", handleRoot);
  server.on("/data", handleData); // Handler für AJAX-Anfragen hinzufügen
  server.begin();#
  */
}
 String hexValue = "";

bool WiFiiConnecting = false;
unsigned long scriptTimer = 0;
unsigned long scriptTimerAfterSleep = 0;
void loop() {
  //server.handleClient();
  unsigned long currentMillis = millis();
  if(scriptTimerAfterSleep == 0)  {
    scriptTimer = currentMillis;
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
    /*
    String hexValuetemp = String(bytesmltemp, HEX); // Konvertierung des Zeichens in eine Hexadezimalzeichenfolge
    if (hexValuetemp.length() == 1) { // Überprüfen, ob die Hexadezimalzeichenfolge nur eine Ziffer hat
      hexValuetemp = "0" + hexValuetemp; // Hinzufügen einer führenden Null, wenn erforderlich
    }
    hexValue = hexValue + " " + hexValuetemp;
    */


    //Serial.print(String(bytesmltemp));
    //Serial.print(String(SequenceStep));

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
              int scriptTimeSleep = (sleepTime - (millis() - scriptTimer)); //dont work
              sleep_ms(scriptTimeSleep);
              scriptTimerAfterSleep = millis();
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
                sleep_ms(1);
              }
              radio.sleep();
              
              
              //int scriptTimeSleep = (sleepTime - (millis() - scriptTimer));
              //int scriptTimeSleep = (sleepTime - ((millis() - scriptTimer)+(changeTime*4)));

              /*
              sleep_ms(changeTime);
              set_sys_clock_khz(10000, false);
              sleep_ms(changeTime);
              vreg_set_voltage(VREG_VOLTAGE_0_95);
              */
              
              //Serial.print(String(scriptTimeSleep));

              // int scriptTimeSleep = ((sleepTime - (millis() - scriptTimer))-(changeTime*3)); //dont work
              int scriptTimeSleep = (sleepTime - (millis() - scriptTimer)); //dont work
              if(scriptTimeSleep > 0) {
                Serial.print(String(scriptTimeSleep)); //Wieso geht es nicht wenn es hier ist.
                sleep_ms(scriptTimeSleep); //Ja die Funktion ist nur ein Platzhalter.
              } else {
                sleep_ms(sleepTime-1000);
              }
              scriptTimerAfterSleep = millis();
              
              //Lora crash hier, and the Loop dont work with if (currentMillis - previousMillis >= interval) {, but its works with SequenceStep = 10;
              //Es ist einfach zu instabil, bei 10 MHz sollte vielleicht alles zu sicherheit neugestartet werden.
              /*
              vreg_set_voltage(VREG_VOLTAGE_DEFAULT);
              sleep_ms(changeTime);
              set_sys_clock_48mhz();
              sleep_ms(changeTime);
              */

              radio.standby();

              //sleep_hms(0,0,19);
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

    /*
    int c = SmlSerial.read();
    String hexValue = String(c, HEX); // Konvertierung des Zeichens in eine Hexadezimalzeichenfolge
    if (hexValue.length() == 1) { // Überprüfen, ob die Hexadezimalzeichenfolge nur eine Ziffer hat
      hexValue = "0" + hexValue; // Hinzufügen einer führenden Null, wenn erforderlich
    }
    irDataBuffer += hexValue + " "; // Anhängen des empfangenen Zeichens an den Buffer
    Serial.print(hexValue + " "); // Ausgabe des empfangenen Zeichens über die serielle Schnittstelle
    
  }
  if (irDataBuffer != "") {
    if (currentMillis - previousMillis >= interval) {
      irDataBuffer = irDataBuffer.substring(0, 240);
      transmissionState = radio.startTransmit(irDataBuffer);
      Serial.print(F("[SX1262] Sending another packet ... "));
      transmitFlag = true;
      irDataBuffer = "";
      previousMillis = currentMillis;
    }
  } else {
    previousMillis = currentMillis;
  }*/
  
  /*
  // check if the previous operation finished
  if(operationDone) {
    // reset flag
    operationDone = false;

    if(transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
      }

      // listen for response
      //radio.startReceive();
      transmitFlag = false;

    } else {
      /*
      // the previous operation was reception
      // print data and send another packet
      //String str;
      //int state = radio.readData(str);

      // wait a second before transmitting again
      delay(1000);

      // send another one
      Serial.print(F("[SX1262] Sending another packet ... "));
      //transmissionState = radio.startTransmit(irDataBuffer);
      transmitFlag = true;
      irDataBuffer = "";
      //*//*
    }
  }*/
}

/*
void handleRoot() {
  digitalWrite(LED_BUILTIN, HIGH); // LED einschalten
  String html = "<!DOCTYPE html><html><head><title>IR Data Viewer</title></head><body>";
  html += "<h1>IR Data Viewer</h1>";
  html += "<div id='irData'></div>";
  html += "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script>";
  html += "<script>function updateIRData() {$.ajax({url: '/data', success: function(data) {$('#irData').append(data);}});}";
  html += "setInterval(updateIRData, 100);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  digitalWrite(LED_BUILTIN, LOW); // LED ausschalten
}

void handleData() {
  server.send(200, "text/plain", irDataBuffer);
  irDataBuffer = ""; // Leeren des Datenpuffers nach dem Senden
}
*/
