# WarmLink

Wärme spüren über Distanz. Ein IoT-Prototyp, der zwei Personen an
unterschiedlichen Orten über die gemessene Raumtemperatur und einen
kurzen Lichtimpuls miteinander verbindet.

Dieses Repository enthält alles, was benötigt wird, um WarmLink
selbst nachzubauen: Bauteilliste, Verkabelung, Firmware, Gehäuse und
Web-Dashboard.

## Inhaltsverzeichnis

- [Was ist WarmLink](#was-ist-warmlink)
- [Benötigte Bauteile](#benötigte-bauteile)
- [Verkabelung](#verkabelung)
- [Firmware aufspielen](#firmware-aufspielen)
- [MQTT-Broker einrichten](#mqtt-broker-einrichten)
- [Web-Dashboard einrichten](#web-dashboard-einrichten)
- [Gehäuse drucken](#gehäuse-drucken)
- [Zusammenbau](#zusammenbau)
- [Bedienung](#bedienung)
- [Bekannte Probleme](#bekannte-probleme)

## Was ist WarmLink

WarmLink besteht aus zwei baugleichen Endpunkten. Jeder Endpunkt
misst die Umgebungstemperatur und Luftfeuchtigkeit und sendet die
Werte über MQTT an den jeweils anderen Endpunkt. Dort wird die
empfangene Temperatur über eine RGB-LED farblich dargestellt (blau
bei kalt bis rot bei warm). Über einen Taster kann zusätzlich ein
kurzer weißer Lichtimpuls an die Gegenseite gesendet werden, um
bewusst ein Zeichen der Verbundenheit zu setzen. Ein optionales
Web-Dashboard zeigt die Werte beider Endpunkte zusätzlich im Browser
an.

## Benötigte Bauteile

Für **einen** Endpunkt (für WarmLink werden zwei baugleiche
Endpunkte benötigt):

| Bauteil | Menge | Bemerkung |
|---|---|---|
| ESP8266 NodeMCU (Crowtail) | 1 | Mikrocontroller mit WLAN |
| Temperatur-/Feuchtigkeitssensor (DHT11, Crowtail) | 1 | physischer Input |
| RGB-LED-Modul (WS2812B, Crowtail) | 1 | physischer Output |
| Taster (Crowtail Button) | 1 | Verbundenheits-Signal |
| Micro-USB-Kabel + 5V-Netzteil | 1 | Stromversorgung |

Zusätzlich, einmalig für das gesamte System:

- Ein Computer im selben WLAN, auf dem MQTT-Broker und (optional)
  Node-RED laufen
- 3D-Drucker (für das Gehäuse, optional)

## Verkabelung

Beide Endpunkte werden identisch verkabelt:

| Bauteil | Pin am NodeMCU |
|---|---|
| Temperatursensor | D3 |
| RGB-LED | D1 |
| Taster | D2 |

Alle Module nutzen die 4-poligen Crowtail-Stecker und werden einfach
in die entsprechenden Ports auf dem Base-Shield gesteckt.

## Firmware aufspielen

1. [Arduino IDE](https://www.arduino.cc/en/software) installieren
2. In der Arduino IDE unter **Werkzeuge → Board → Boardverwalter**
   nach `esp8266` suchen und das Paket "esp8266 by ESP8266
   Community" installieren
3. Unter **Werkzeuge → Bibliotheken verwalten** folgende
   Bibliotheken installieren:
   - `PubSubClient` (Nick O'Leary)
   - `DHT sensor library` (Adafruit, inkl. Abhängigkeit "Adafruit
     Unified Sensor")
   - `Adafruit NeoPixel`
4. Die passende Firmware-Datei öffnen:
   - [`firmware/warmlink_endpoint1.ino`](firmware/warmlink_endpoint1.ino)
     für den ersten Endpunkt
   - [`firmware/warmlink_endpoint2.ino`](firmware/warmlink_endpoint2.ino)
     für den zweiten Endpunkt
5. Im Code folgende Zeilen an die eigene Umgebung anpassen:
   ```cpp
   const char* WIFI_SSID     = "DEIN_WLAN_NAME";
   const char* WIFI_PASSWORD = "DEIN_WLAN_PASSWORT";
   const char* MQTT_BROKER   = "IP_DEINES_BROKER_RECHNERS";
   ```
6. Board auswählen: **Werkzeuge → Board → NodeMCU 1.0 (ESP-12E
   Module)**
7. Passenden COM-Port auswählen und hochladen

## MQTT-Broker einrichten

WarmLink benötigt einen MQTT-Broker im lokalen Netzwerk. Empfohlen
wird [Mosquitto](https://mosquitto.org/download/):

1. Mosquitto installieren
2. In der Datei `mosquitto.conf` folgende Zeilen ergänzen, damit der
   Broker auch für andere Geräte im Netzwerk erreichbar ist:
   ```
   listener 1883
   allow_anonymous true
   ```
3. Broker starten:
   ```
   mosquitto -v -c mosquitto.conf
   ```
4. In der Windows-Firewall eingehende Verbindungen auf Port 1883
   erlauben

## Web-Dashboard einrichten

Optional, aber empfohlen für eine zusätzliche browserbasierte
Ansicht:

1. [Node.js](https://nodejs.org) installieren
2. Node-RED installieren:
   ```
   npm install -g --unsafe-perm node-red
   ```
3. Node-RED starten:
   ```
   node-red
   ```
4. Im Browser `http://localhost:1880` öffnen
5. Über **Menü → Palette verwalten → Installieren** das Paket
   `node-red-dashboard` installieren
6. Über **Menü → Import** die Datei
   [`dashboard/warmlink_flow.json`](dashboard/warmlink_flow.json)
   importieren und mit **Deploy** aktivieren
7. Dashboard ist danach unter `http://localhost:1880/ui` erreichbar

Für dauerhaft gespeicherte Minimal-/Maximalwerte sollte zusätzlich in
der Node-RED `settings.js` folgender Abschnitt aktiviert werden:

```js
contextStorage: {
    default: {
        module: "localfilesystem"
    }
},
```

## Gehäuse drucken

Das Gehäuse liegt als parametrisches OpenSCAD-Modell vor:
[`gehaeuse/warmlink_gehaeuse.scad`](gehaeuse/warmlink_gehaeuse.scad)

1. [OpenSCAD](https://openscad.org/downloads.html) installieren
2. Datei öffnen, bei Bedarf Maße im Parameter-Block oben anpassen
3. Boden und Deckel werden **getrennt** exportiert:
   - Für den Boden: die Zeile mit `lid();` am Dateiende auskommentieren,
     F6 drücken, dann **Datei → Exportieren → Als STL exportieren**
   - Für den Deckel: umgekehrt vorgehen
4. Beide STL-Dateien in einem Slicer (z. B. Cura, PrusaSlicer) laden
   und mit PLA drucken (empfohlen: 0,2 mm Schichthöhe, 15-20% Infill)
5. Der Diffusor (Milchglas-Einsatz) wird separat aus dem Modul
   `diffuser()` exportiert und idealerweise in weißem, transluzentem
   Filament gedruckt

## Zusammenbau

1. Sensor, LED und Taster wie oben beschrieben mit dem NodeMCU
   verbinden
2. Sensor stehend im Inneren an die vorgesehene Halterung kleben,
   mit der Sensorfläche in Richtung der Lüftungsschlitze
3. NodeMCU im Boden platzieren, USB-Anschluss zur seitlichen Öffnung
   ausrichten
4. Diffusor in die Führungsschiene im Deckel schieben
5. RGB-LED mit Heißkleber oder doppelseitigem Klebeband direkt unter
   dem Diffusor-Fenster befestigen (Kabel vorher anschließen!)
6. Taster durch die runde Öffnung in der Seitenwand stecken
7. Deckel auf den Boden setzen und mit vier Schrauben fixieren

## Bedienung

1. Beide Endpunkte mit Strom versorgen (USB-Netzteil)
2. Beide Endpunkte verbinden sich automatisch mit dem hinterlegten
   WLAN und dem MQTT-Broker
3. Die LED zeigt nach kurzer Zeit die Temperatur der Gegenseite an
4. Ein Druck auf den Taster löst einen kurzen weißen Lichtimpuls auf
   der LED des anderen Endpunkts aus

## Bekannte Probleme

- **ESP8266 verbindet sich nicht mit dem WLAN:** Der ESP8266
  unterstützt ausschließlich 2,4-GHz-WLAN mit WPA2-Verschlüsselung.
  Moderne Router mit WPA3 oder reinem 5-GHz-Band führen zu
  Verbindungsproblemen.
- **RGB-LED verursacht WLAN-Aussetzer:** Bei knapper
  Stromversorgung kann die LED beim Ansteuern kurzzeitig die
  WLAN-Verbindung stören. Abhilfe schafft eine Reduzierung der
  LED-Helligkeit im Code über `strip.setBrightness(80)` (Wertebereich
  0 bis 255) sowie ein stabiles Netzteil.
- **MQTT-Verbindung schlägt fehl, obwohl WLAN steht:** Meist liegt
  es daran, dass der Broker nicht für Netzwerkverbindungen
  konfiguriert ist (siehe Abschnitt MQTT-Broker einrichten) oder die
  Windows-Firewall den Port blockiert.
