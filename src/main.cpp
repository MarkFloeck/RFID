#include <Arduino.h>
#include <SPI.h>     //Datenbus
#include <MFRC522.h> //Kartenleser
#include "rfid.h"
#include "main.h"
#include <WiFi.h>
#include <ezTime.h>
#include <ArduinoJson.h>

/* =====================================================================
   Objekt-Definitionen
   ===================================================================== */
MFRC522 mfrc522(PIN_SDA, RST_PIN);                                                  // Kartenleser-Objekt definieren
RFID rfid(PIN_SDA, RST_PIN);                                                        // RFID-Klasse
Timezone myTZ;                                                                      // NTP-Zeit-Objekt
StaticJsonDocument<4096> data;                                                      // JSON-Dokument zur Datenspeicherung
JsonObject root;                                                                    // JSON-Wurzelknoten

/* =====================================================================
   Variablen-Definitionen
   ===================================================================== */
String master = "0459B1E7";                                                         // Master-Transponder
const char ssid[] = "bbw-Gruppe";                                                   // WLAN-SSID
const char pass[] = "bbw-Gruppe";                                                   // WLAN-Passwort

/* =====================================================================
   Variablen-Definitionen
   ===================================================================== */
void tonex(int pin, int frequency, int duration);                                   // Überschreibung der Tone-Klasse für ESP32





/* =====================================================================
   Setup-Routine
   ===================================================================== */

void setup()
{
  root = data.to<JsonObject>();                                                     // JSON-Knoten aus Dokument erstellen
  pinMode(PIN_BEEP, OUTPUT);                                                        // Buzzer-Pin als Ausgang definieren
  ledcSetup(0, 2000, 8);                                                            // Buzzer-Pin konfigurieren (Kanal 0, 2000Hz PWM-Frequenz, 8bit Auflösung)
  ledcAttachPin(PIN_BEEP, 0);                                                       // PWM auf Buzzer-Pin aktivieren
  
  Serial.begin(115200);                                                             // Serielle Schnittstelle initialisieren
  Serial.println();                                                                 // leere Zeile ausgeben
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SDA);                                  // Datenbus starten
  WiFi.begin(ssid, pass);                                                           // WLAN verbinden
  while (WiFi.status() != WL_CONNECTED)                                             // Warten auf erfolgreiche Verbindung
  {                                                                                 //    
    delay(500);                                                                     // 500ms warten
    Serial.print(".");                                                              // Punkt als "Ladebalken" ausgeben
  }                                                                                 // Prüfung wiederholen 
  waitForSync();                                                                    // Zeit mit Zeitserver synchronisieren
  Serial.println();                                                                 // Leere Zeile ausgeben
  myTZ.setLocation(F("Europe/Berlin"));                                             // Zeitzone festlegen  
  rfid.init();                                                                      // Kartenleser initialisieren
  tonex(PIN_BEEP, BEEP_HIGH, 100);                                                  // Bestätigungston ausgeben
}


/* =====================================================================
   Haupt-Routine
   ===================================================================== */
void loop()
{
  if (rfid.handle())                                                                // Prüfen, ob Kartendaten vorhanden. Falls JA:
  {
    String uid = rfid.getTagUID();                                                  // Karten-UID laden
    if (uid == master)                                                              // falls Master-Transponder:
    {
      tonex(PIN_BEEP, BEEP_HIGH, 100);                                              //      Ton abspielen
      delay(100);                                                                   //      100ms Pause
      tonex(PIN_BEEP, BEEP_HIGH, 100);                                              //      Ton abspielen
      delay(100);                                                                   //      100ms Pause
      tonex(PIN_BEEP, BEEP_HIGH, 100);                                              //      Ton abspielen
      Serial.println();                                                             //      leere Zeile ausgeben  
      Serial.println();                                                             //      leere Zeile ausgeben  
      Serial.println();                                                             //      leere Zeile ausgeben
      serializeJsonPretty(root, Serial);                                            //      gespeicherte Daten als formatiertes JSON ausgeben
      Serial.println();                                                             //      leere Zeile ausgeben
      Serial.println();                                                             //      leere Zeile ausgeben
      Serial.println();                                                             //      leere Zeile ausgeben
      return;
    }

    /* ======================
       Speicher konfigurieren
       ======================*/
    if (!root.containsKey(uid))                                                     // Falls noch kein Eintrag im Speicher vorhanden:
    {                                                                               //
      root[uid] = data.createNestedObject();                                        //      Leeren Eintrag zum Transpondercode erstellen
    }                                                                               //

    String k = myTZ.dateTime("Y-m-d");                                              // Datums-Stempel im Format JJJJ-MM-TT erstellen
    if (!root[uid].containsKey(k))                                                  // Falls noch kein Eintrag zum Datum im Transponder-Key enthalten:
    {                                                                               //
      root[uid].createNestedArray(k);                                               //      Leeres Array für dieses datum anlegen
    }                                                                               //
    
    /* ======================
       Kommen/Gehen-Prüfung
       ======================*/
    size_t c = root[uid][k].size();                                                 // Größe des Datums-Arrays lesen
    bool addEntry = true;                                                           // Flag zur Datenspeicherung. Speichern ja/nein?
    if (c > 0)                                                                      // Falls Daten vorhanden:
    {                                                                               //
      long t = root[uid][k][c - 1];                                                 //      letzten Wert des Array auslesen
      if ((myTZ.now() - t) < RPT_TIMEOUT)                                           //      falls kleiner aus Wiederholungs-Timeout:
        addEntry = false;                                                           //          Speicher-Flag auf NEIN
    }                                                                               //
    if (addEntry == true)                                                           // Wenn Speichern=JA:
    {                                                                               //
      String mode = "\tkommen";                                                     // Text-Variable vorbelegen mit "KOMMEN"
      if ((c % 2) > 0)                                                              // Wenn UNGERADE Anzahl im Speicher:
        mode = "\tgehen";                                                           //    Text mit "GEHEN" überschreiben
      root[uid][k].add(myTZ.now());                                                 // neuen Zeitstempel ans Ende des Arrays speichern
      Serial.print(uid);                                                            // UID ausgeben
      Serial.print("\t");                                                           // Tabulator ausgeben
      Serial.print(myTZ.dateTime("d-m-Y H:i:s"));                                   // aktuellen Zeitstempel ausgeben
      Serial.println(mode);                                                         // Kommen / Gehen ausgeben
      tonex(PIN_BEEP, BEEP_HIGH, 100);                                              // Piepsen
    }
    else                                                                            // Wenn Speichern=NEIN
    {
      tonex(PIN_BEEP, BEEP_LOW, 100);                                               // Warnton ausgeben
    }
  }
}


/* =====================================================================
   Hilfs-Routine zur Tonausgabe
   ===================================================================== */
void tonex(int pin, int frequency, int duration)
{
  ledcWriteTone(0, frequency);                                                      // Ton mit <frequency>Hz ausgeben
  delay(duration);                                                                  // <duration> Millisekunden warten
  ledcWriteTone(0, 0);                                                              // Ton aus
}
