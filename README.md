# Pico-SmartMeter

SmartMeter over LoRa and WiFi with a graphical user interface.  
👉 A detailed **making-of article** can be found [here](https://heldendesbildschirms.de/artikel/stromzaehler-auslesen-mit-dem-pico-daten-ueber-lora-senden/).

This [project](https://www.ploesch.de/index.php?side=g-electricmeter) is a modified version tailored by me for the Pico with the LoRa module. I have created my own web interface.

Implementing DeepSleep under the Arduino IDE is somewhat complicated. The power consumption is approximately 18 mAh at a 50 MHz clock. The 600 mAh battery was depleted after 33 hours. I also attempted to implement my own DeepSleep and further lower the clock frequency, but encountered strange issues when the clock frequency was increased again. Since this problem only occurred when the Pico was actually connected to the electricity meter and the entire code was executed, I didn't want to test it for too long in the community basement. So, if anyone can further reduce the power consumption, that would be great. However, one should not expect too significant improvements. DeepSleep would be much more beneficial. Currently, a 10000 mAh battery is recommended at least to avoid frequent trips to the cellar each month.

The DeepSleep issue is explained quite well [here](https://github.com/earlephilhower/arduino-pico/issues/345) ([2](https://github.com/earlephilhower/arduino-pico/discussions/1544)), and implementing it under the Arduino IDE is not an easy task. For testing purposes, the receiving script still includes a Heltec library. So, the code still needs to be adjusted for the RadioLib, but this should be done quickly. For the reader head, the pins can be easily changed here: `SoftwareSerial SmlSerial(1, 0); // RX, TX`. The LoRa pins can also be defined and adjusted above. WiFi or LoRa can be activated and deactivated with the variables `useWiFiMode` and `useLoRaMode`. The device ID can be turned on or off. If there are hardly any other devices in the radio range, it can be turned off. The data is currently not encrypted, and the device ID is also static. SSID and password, server, port, and secret should be adjusted. For other microcontrollers, you can find the pinout for most LoRa-compatible devices not only in the manufacturer's documentation but also at [Meshtastic](https://github.com/meshtastic/firmware/blob/master/variants/). The pins are listed in `variant.h`. The web interface still lacks a few options, such as monthly consumption, better performance through the use of the Decimation-LTTB algorithm, and caching of database queries.

For receiving data, you can use the [`RadioLibHTTPSResiverStromzeaher.ino`](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/RadioLibHTTPSResiverStromzeaher.ino) file. The pin configuration may need to be adjusted.

The ESP32 in [senderESP32.ino](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/senderESP32.ino) uses DeepSleep. This version is based on [senderPico.ino](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/senderPico.ino) for the Pico. An RTC is required to measure the time after waking up, but this solution still needs to be implemented. Currently, the time is estimated. Here is a rough estimate: 51 seconds (85%) per minute DeepSleep, and 9 seconds (15%) active. (25 mA × 0,15 = 3,75 mA) + (0,01 mA × 0,85 = 0,0085 mA) = 3,7585 mA. This could be improved by later transmission and intermediate storage in RTC RAM, or by minute intervals, and by skipping boot verification. 10.000 mAh could last for about 110 days this way.

During testing, a runtime of approximately two months was observed. This can potentially be improved by completely powering down the LoRa module during ESP32 deep sleep using a MOSFET or transistor.
Note: The specified maximum capacity (mAh) has not been verified to actually reach 10 000 mAh, nor whether the cell quality matches the seller’s claims.
It is possible that these values are slightly exaggerated due to the physical size of the cells, and the real capacity may only be between 6000 and 8000 mAh.

I have created a test version, [sender_ESP32_NiMH_Battery.ino](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/sender_ESP32_NiMH_Battery.ino), optimized for the Heltec Wireless Stick (V3) and designed to run on three AAA NiMH cells.
This configuration aims to provide an absolutely safe alternative without LiPo, requiring no active monitoring.
Currently, it transmits only every 20 minutes, but it is also possible to use a LiFePO₄ battery, which would make the setup more compact and provide more stable voltage.

An additional idea for encryption: replay protection via a time-linked client ID. The Unix timestamp can be requested from the target client with internet access.

# Pico-SmartMeter

SmartMeter über LoRa und WiFi mit grafischer Benutzeroberfläche  
👉 Einen ausführlichen **Making-of-Artikel** findest du [hier](https://heldendesbildschirms.de/artikel/stromzaehler-auslesen-mit-dem-pico-daten-ueber-lora-senden/).

Dieses [Projekt](https://www.ploesch.de/index.php?side=g-electricmeter) ist eine modifizierte Version, die von mir für den Pico mit dem LoRa-Modul angepasst wurde. Ich habe eine eigene Web-Oberfläche erstellt.

Die Implementierung von DeepSleep unter der Arduino IDE gestaltet sich etwas kompliziert. Der Stromverbrauch beträgt bei einem 50 MHz Takt etwa 18 mAh. Der 600 mAh Akku war nach 33 Stunden leer. Ich habe auch versucht, meinen eigenen DeepSleep zu implementieren und den Takt weiter zu senken, was jedoch zu seltsamen Problemen führte, wenn der Takt wieder erhöht wurde. Da dieses Problem nur auftrat, wenn der Pico tatsächlich am Stromzähler angeschlossen war und der gesamte Code durchlaufen wurde, wollte ich nicht zu lange im Gemeinschaftskeller testen. Wenn also jemand den Verbrauch weiter senken kann, wäre das großartig. Allerdings sollte man keine zu großen Verbesserungen erwarten. DeepSleep würde deutlich mehr helfen. Aktuell wird ein 10000 mAh Akku mindestens empfohlen, damit man nicht zu oft im Monat in den Keller laufen muss.

Die DeepSleep-Problematik wird [hier](https://github.com/earlephilhower/arduino-pico/issues/345) ([2](https://github.com/earlephilhower/arduino-pico/discussions/1544)) recht gut erklärt, und die Implementierung unter der Arduino IDE ist keine einfache Aufgabe. Für Testzwecke hat das Empfangsskript noch eine Heltec-Bibliothek. Der Code muss also noch für die RadioLib angepasst werden, was jedoch schnell erledigt sein sollte. Für den Lesekopf können die Pins ganz einfach hier `SoftwareSerial SmlSerial(1, 0); // RX, TX` geändert werden. Die Pins für LoRa können auch oben definiert und angepasst werden. WiFi oder LoRa können mit den Variablen `useWiFiMode` und `useLoRaMode` aktiviert und deaktiviert werden. Die Device-ID kann ein- oder ausgeschaltet werden. Wenn kaum andere Geräte in der Funkreichweite sind, kann sie deaktiviert werden. Die Daten sind zum jetzigen Zeitpunkt nicht verschlüsselt, und die Device-ID ist auch statisch. SSID und Passwort, Server, Port und Secret sollten angepasst werden. Für andere Mikrocontroller könnt ihr das Pinout für die meisten LoRa-fähigen Geräte nicht nur in der Herstellerdokumentation finden, sondern auch bei [Meshtastic](https://github.com/meshtastic/firmware/blob/master/variants/). In der `variant.h` stehen die Pins. Der Weboberfläche fehlen noch ein paar Optionen, wie zum Beispiel einen Monatsverbrauch, eine bessere Leistung durch die Nutzung des Decimation-LTTB-Algorithmus und das Caching von Datenbank Anfragen.

Zum Empfangen kann die Datei [`RadioLibHTTPSResiverStromzeaher.ino`](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/RadioLibHTTPSResiverStromzeaher.ino) verwendet werden. Die Pins müssen ggf. angepasst werden.

Für den ESP32 in [senderESP32.ino](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/senderESP32.ino) wird DeepSleep genutzt. Diese Version basiert auf [senderPico.ino](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/senderPico.ino) für den Pico. Eine RTC ist nötig, um nach dem Aufwachen die Zeit zu messen, allerdings muss diese Lösung noch implementiert werden. Aktuell wird die Zeit geschätzt. Hier eine grobe Schätzung: 51 Sekunden (85 %) pro Minute DeepSleep und 9 Sekunden (15 %) aktiv. (25 mA × 0,15 = 3,75 mA) + (0,01 mA × 0,85 = 0,0085 mA) = 3,7585 mA. Das könnte durch späteres Senden und Zwischenspeichern im RTC-RAM oder durch minütliche Intervalle verbessert werden, sowie durch das Weglassen der Boot-Verifikation. 10.000 mAh können so also circa 110 Tage halten.

In Tests wurde eine Laufzeit von etwa zwei Monaten festgestellt. Diese könnte verbessert werden, wenn das LoRa-Modul beim Einsatz mit einem ESP32 mithilfe eines MOSFETs oder Transistors im Deep-Sleep-Modus vollständig abgeschaltet wird.
Hinweis: Es wurde nicht überprüft, ob die Angaben zur maximalen Kapazität (mAh) tatsächlich bei 10 000 mAh liegen und ob die Zellqualität den Angaben des Händlers entspricht.
Es ist möglich, dass diese Werte aufgrund der Baugröße etwas übertrieben angegeben wurden und die tatsächliche Kapazität nur zwischen 6000 und 8000 mAh liegt.

Ich habe eine Testversion [sender_ESP32_NiMH_Battery.ino](https://github.com/JanisPlayer/Pico-SmartMeter/blob/main/sender_ESP32_NiMH_Battery.ino) erstellt, die für den Heltec Wireless Stick (V3) optimiert ist und auf die Nutzung von 3 × AAA NiMH-Akkus ausgelegt wurde.
Diese Variante soll eine absolut sichere Lösung ohne LiPo darstellen, die keine Überwachung benötigt.
Momentan sendet sie allerdings nur alle 20 Minuten.
Alternativ kann auch ein LiFePO₄-Akku verwendet werden, was das System kompakter macht und die Spannung stabilisiert.

Idee zusätzlich zur Verschlüsselung: Replay-Schutz über zeitverknüpfte Client-ID. Der Unix-Timestamp kann vom Ziel-Client mit Internetzugang angefordert werden.

![SmartMeter WebPage](https://github.com/JanisPlayer/Pico-SmartMeter/assets/54918417/dcb32c20-5dc7-4233-99b6-c9854a775582)
