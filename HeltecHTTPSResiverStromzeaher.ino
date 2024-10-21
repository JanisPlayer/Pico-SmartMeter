#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>

const char *rootCACertificate = "-----BEGIN CERTIFICATE-----\n"
                                  "MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n"
                                  "VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n"
                                  "A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n"
                                  "WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n"
                                  "IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n"
                                  "AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n"
                                  "QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n"
                                  "HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n"
                                  "BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n"
                                  "9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n"
                                  "p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n"
                                  "-----END CERTIFICATE-----\n";

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

const char* ssid = "HeldendesBildschirms";
const char* password = "";

// Serverdaten
const char* server = "heldendesbildschirms.de";

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
    setClock();
}

void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
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

void sendhttps(unsigned long currentPW, unsigned long currentPKwh) {
  NetworkClientSecure *client = new NetworkClientSecure;
  if (client) {
    client->setCACert(rootCACertificate);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before NetworkClientSecure *client is
      HTTPClient https;

      Serial.print("[HTTPS] begin...\n");
      String url = "https://" + String(server) + "/stromzeahler.php?password=" + secret + "&power_consumption=" + String(currentPW) + "&counter_reading=" + String(currentPKwh);
      if (https.begin(*client, url)) {  // HTTPS
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }

    delete client;
  } else {
    Serial.println("Unable to create client");
  }
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
          dblcurrentPKwh = (double)currentPKwh / 10000;
          //Aktuelle leistung in Watt
          currentPW = ((long)payload[8] << 24 | (long)payload[9] << 16 | (long)payload[10] << 8 | (long)payload[11]);
        } else {
          currentPKwh = ((long)payload[0] << 24 | (long)payload[1] << 16 | (long)payload[2] << 8 | (long)payload[3]);
          dblcurrentPKwh = (double)currentPKwh / 10000;
          //Aktuelle leistung in Watt
          currentPW = ((long)payload[4] << 24 | (long)payload[5] << 16 | (long)payload[6] << 8 | (long)payload[7]);
        }
        String irDataBuffer = String(currentPW) + " " + String(dblcurrentPKwh);
        Serial.print(irDataBuffer);

        Serial.println("HTTP-Anfrage senden");
        sendhttps(currentPW,currentPKwh);

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
    Radio.Sleep();
    Serial.print("TX Timeout......");
    state=STATE_TX;
}
