/media/nvme0n1p2/Users/janis/Documents/Arduino/ResiverStromzeaher/* Heltec Automation Ping Pong communication test example
 *
 * Function:
 * 1. Ping Pong communication in two esp32 device.
 * 
 * Description:
 * 1. Only hardware layer communicate, no LoRaWAN protocol support;
 * 2. Download the same code into two esp32 devices, then they will begin Ping Pong test each other;
 * 3. This example is for esp32 hardware basic test.
 *
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * */


#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
//#include <WiFiClientSecure.h>
#include <WiFiClient.h>


#define RF_FREQUENCY                                869400000 // Hz

#define TX_OUTPUT_POWER                             1        // dBm

#define LORA_BANDWIDTH                              1         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 1024 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

typedef enum
{
    LOWPOWER,
    STATE_RX,
    STATE_TX
}States_t;

int16_t txNumber;
States_t state;
bool sleepMode = false;
int16_t Rssi,rxSize;

bool lora_idle = true;

unsigned long currentPW = 0; //Aktuelle Leistung W Ausgabe DEC
unsigned long currentPKwh = 0; //Gesamtleistung Kwh Ausgabe DEC
double dblcurrentPW = 0; //currentPW/Count
double dblcurrentPKwh = 0; //currentPKwh/10000

// WiFi-Zugangsdaten
const char* ssid = "";
const char* password = "";

// Serverdaten
const char* server = "192.168.0.24";
const int port = 80;

// HTTP-Anfrage
String secret = "";

// ID für Identifikation
bool deviceidIsOn = false;
const byte deviceid[] = { 0x00, 0x00, 0x00, 0x00 };

void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.print("Connect to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi verbunden");
    Serial.println("IP-Adresse: ");
    Serial.println(WiFi.localIP());

    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
    txNumber=0;
    Rssi=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    state=STATE_TX;
}

unsigned long previousMillis = 0;
unsigned long interval = 30000;
void wifiReconnect() {
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}

void loop()
{
  wifiReconnect();
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    rssi=rssi;
    rxSize=size;
    if (size <= BUFFER_SIZE)
    {
      memcpy(rxpacket, payload, size );
      rxpacket[size]='\0';
      Radio.Sleep();
      //Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,rssi,rxSize);
        // Payload in Hexadezimal ausgeben
        Serial.printf("\r\nreceived packet in hex: ");
        for (uint16_t i = 0; i < size; i++) {
            Serial.printf("%02X", payload[i]);
        }
        Serial.printf(" with rssi %d, length %d\r\n", rssi, rxSize);
      bool match = true;
      if (deviceidIsOn == true) {
        for (int i = 0; i <= 3; i++) {
            if (deviceid[i] != payload[i]) {
                match = false;
                break; // Brechen Sie die Schleife ab, wenn keine Übereinstimmung gefunden wird
            }
        }
      }

      if (match) {
        if (deviceidIsOn == true) {
          currentPKwh = ((long)payload[4] << 24 | (long)payload[5] << 16 | (long)payload[6] << 8 | (long)payload[7]);
          //Aktuelle leistung in Watt
          currentPW = ((long)payload[8] << 24 | (long)payload[9] << 16 | (long)payload[10] << 8 | (long)payload[11]);
        } else {
          currentPKwh = ((long)payload[0] << 24 | (long)payload[1] << 16 | (long)payload[2] << 8 | (long)payload[3]);
          //Aktuelle leistung in Watt
          currentPW = ((long)payload[4] << 24 | (long)payload[5] << 16 | (long)payload[6] << 8 | (long)payload[7]);
        }
        if (currentPKwh > 0) {
          dblcurrentPKwh = (double)currentPKwh / 10000.0;
        }
        String irDataBuffer = String(currentPW) + " " + String(dblcurrentPKwh);
        Serial.print(irDataBuffer);

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
      } else {
        Serial.println("Kennung stimmt nicht überein.");
      }
    }
    lora_idle = true;
}

void OnTxDone( void )
{
  Serial.print("TX done......");
  state=STATE_RX;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.print("TX Timeout......");
    state=STATE_TX;
}

/*
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep();

    Serial.printf("\r\nreceived packet \"%s\" with Rssi %d , length %d\r\n",rxpacket,Rssi,rxSize);
    Serial.println("wait to send next packet");

    state=STATE_TX;
}
*/
