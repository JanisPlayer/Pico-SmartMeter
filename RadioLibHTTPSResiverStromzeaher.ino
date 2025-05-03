#include <WiFi.h>
#include <WiFiMulti.h>
#include <RadioLib.h>
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

// Definiere SPI-Pins für ESP32
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5

#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET 22
#define LORA_DIO1 4
#define LORA_BUSY 21

// Definiere SPI-Pins für Heltec LoRa 32(V3)
// #define LORA_SCK 9
// #define LORA_MISO 11
// #define LORA_MOSI 10
// #define LORA_CS 8

// #define LORA_DIO0 RADIOLIB_NC
// #define LORA_RESET 12
// #define LORA_DIO1 14
// #define LORA_BUSY 13

// Definiere SPI-Pins für Pico
// #define LORA_SCK 10
// #define LORA_MISO 12
// #define LORA_MOSI 11
// #define LORA_CS 3

// #define LORA_DIO0 RADIOLIB_NC
// #define LORA_RESET 15
// #define LORA_DIO1 20
// #define LORA_BUSY 2

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

// Initialisiere das SX1262 Modul
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI, RADIOLIB_DEFAULT_SPI_SETTINGS);

// Zustandsvariable global deklarieren
volatile bool receivedFlag = false;

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif

// Diese Funktion wird aufgerufen, wenn ein Paket empfangen wurde
void setFlag(void) {
  // Ein Paket wurde empfangen, setze das Flag
  receivedFlag = true;
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

  // Initialisiere das LoRa-Modul
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS); // SPI Pins korrekt initialisieren
  Serial.print(F("[SX1262] Initialisiere ... "));
  int state = radio.begin(869.4, 250.0, 7, 5, 0x12, 1);  // Standardparameter für LoRa
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("Erfolgreich!"));
  } else {
    Serial.print(F("Fehlgeschlagen, Code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // Funktion für den Empfang festlegen
  radio.setPacketReceivedAction(setFlag);

  // Starten des Empfangsmodus
  Serial.print(F("[SX1262] Starte den Empfang ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("Erfolgreich!"));
  } else {
    Serial.print(F("Fehlgeschlagen, Code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  setClock();
}

void loop() {
  // Prüfe, ob das Flag gesetzt wurde
  if(receivedFlag) {
    // Flag zurücksetzen
    receivedFlag = false;

    // Lese die empfangenen Daten aus
    uint8_t buffer[64];
    int state = radio.readData(buffer, sizeof(buffer));
    size_t len = radio.getPacketLength();

    if (state == RADIOLIB_ERR_NONE) {
      // Paket wurde erfolgreich empfangen
      Serial.println(F("[SX1262] Received packet!"));

      if(len >= 8) {
        bool match = true;
        if (deviceidIsOn == true) {
          for (int i = 0; i <= 3; i++) {
              if (deviceid[i] != buffer[i]) {
                  match = false;
                  break; // Brechen Sie die Schleife ab, wenn keine Übereinstimmung gefunden wird
              }
          }
        }

        if (match) {
          if (deviceidIsOn == true) {
            currentPKwh = ((long)buffer[4] << 24 | (long)buffer[5] << 16 | (long)buffer[6] << 8 | (long)buffer[7]);
            //Aktuelle leistung in Watt
            currentPW = ((long)buffer[8] << 24 | (long)buffer[9] << 16 | (long)buffer[10] << 8 | (long)buffer[11]);
          } else {
            currentPKwh = ((long)buffer[0] << 24 | (long)buffer[1] << 16 | (long)buffer[2] << 8 | (long)buffer[3]);
            //Aktuelle leistung in Watt
            currentPW = ((long)buffer[4] << 24 | (long)buffer[5] << 16 | (long)buffer[6] << 8 | (long)buffer[7]);
          }
          //if (currentPKwh > 0) {
          //  dblcurrentPKwh = (double)currentPKwh / 10000.0;
          //}
          //String irDataBuffer = String(currentPW) + " " + String(dblcurrentPKwh);
          //Serial.print(irDataBuffer);

          Serial.print("Empfangen: Watt = ");
          Serial.print(currentPW);
          Serial.print(" | kWh = ");
          Serial.println(currentPKwh);

          Serial.println("HTTP-Anfrage senden");
          sendhttps(currentPW,currentPKwh);
        } else {
          Serial.println("Kennung stimmt nicht überein.");
        }
      } else {
        Serial.println("Fehler beim Empfang oder zu wenig Daten");
      }

      // RSSI (Empfangsstärke) ausgeben
      Serial.print(F("[SX1262] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // SNR (Signal-Rausch-Verhältnis) ausgeben
      Serial.print(F("[SX1262] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      // Frequenzfehler ausgeben
      Serial.print(F("[SX1262] Frequency error:\t"));
      Serial.print(radio.getFrequencyError());
      Serial.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // Paket wurde empfangen, ist aber fehlerhaft
      Serial.println(F("CRC error!"));

    } else {
      // Ein anderer Fehler ist aufgetreten
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    // Starten des Empfangsmodus nach dem Lesen der Daten
    Serial.print(F("[SX1262] Restarting listen mode ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }
}
