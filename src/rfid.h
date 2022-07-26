#ifndef RFID_H
#define RFID_H
#include <Arduino.h>
#include <SPI.h>     //Datenbus
#include <MFRC522.h> //Kartenleser


class RFID
{
private:
  MFRC522 rfid;
  String tmpUID;
  String lastUID;
  String tagUID;
  long tmr;
  long cnt = 0;

  String cardUIDHex(byte *buffer, byte bufferSize)
  {
    String o = "";
    for (byte i = 0; i < bufferSize; i++)
    {
      o = o + (buffer[i] < 0x10 ? "0" : "");
      o = o + String((char)buffer[i], HEX);
    }
    o.toUpperCase();
    return o;
  }

public:
  RFID(int sda, int rst)
  {
    rfid = MFRC522(sda, rst);
    tmpUID = "";
    this->lastUID = "";
    this->tagUID = "";
  }

  void init()
  {
    rfid.PCD_Init();
    rfid.PCD_DumpVersionToSerial();
    tmr = millis();
  }

  bool handle()
  {
      if (cnt > 4) this->tmpUID = "";
      if ((millis() - this->tmr) > 5000) this->lastUID = "";
      if (!rfid.PICC_IsNewCardPresent()) {
        cnt = cnt + 1;
        return false;
    } else {
      cnt = 0;
    }
      
    if (!rfid.PICC_ReadCardSerial()) {
      return false; // Daten vorhanden?
    }
      
    String uid = cardUIDHex(rfid.uid.uidByte, rfid.uid.size); // UUID ausgeben
    this->tmr = millis();
    if (this->tmpUID == "") {
      this->tmpUID = uid;
      if (this->lastUID != uid) {
        //Serial.println(uid);
        this->tagUID = uid;
        return true;
      }
      this->lastUID = uid;
      
    }

    return false;
  }

  String getTagUID(){
    String o = this->tagUID;
    this->tagUID = "";
    return o;
  }
};

#endif