#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

/*
.:: Connect ::.
--------------------------------------
RFID-RC522 -> Arduino Pro Mini 3.3V
  3.3V     ->   3.3V
  RES      ->   D9
  GND      ->   GND
  MISO     ->   D12
  MOSI     ->   D11
  SCK      ->   D13
  SDA (SS) ->   D10
--------------------------------------
 LCD (I2C) -> Arduino Pro Mini 3.3V
  GND      ->   GND
  VCC      ->   5V (on Linkit Smart 7688)
  SDA      ->   A4
  SCL      ->   A5
--------------------------------------
 Linkit    -> Arduino Pro Mini 3.3V
  3.3V     ->   VCC
  GND      ->   GND
  P16      ->   Tx (0)
  P17      ->   Rx (1)
  P26      ->   RST
--------------------------------------
 Speaker   -> Arduino Pro Mini 3.3V
  SP+      ->   D8
  SP-      ->   GND
--------------------------------------
On GitHub : 
*/

#define SS_PIN 10
#define RST_PIN 9
#define SP_PIN 8
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[3];

void setup() { 
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  lcd.begin();

  lcd.setCursor(0, 0);
  lcd.print("     IOXhop     ");
  lcd.setCursor(0, 1);
  lcd.print(" www.ioxhop.com ");
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Clocking !   ");
  lcd.setCursor(0, 1);
  lcd.print("Linkit + ProMini");
  delay(2000);

  lcd.clear();
  lcd.print("    Tap Card    ");
}
 
void loop() {

  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  // Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  String strID = "";
  for (byte i = 0; i < 4; i++) {
    strID += 
      (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + 
      String(rfid.uid.uidByte[i], HEX) + 
      (i!=3 ? ":" : "");
  }
  strID.toUpperCase();
 
  tone(SP_PIN, 1000, 100);
  Serial.print("ID:");
  Serial.println(strID);

  lcd.setCursor(0, 0);
  lcd.print("Load data...");
  unsigned long startMillis = millis();
  bool done = false;
  String uName = "[Not Found]";
  String uTime = "00:00:00";
  while((millis()-startMillis<1000) && !done) {
    if (Serial.available()) {
      String line =  Serial.readStringUntil('\r');
      Serial.readStringUntil('\n');
      String Start = line.substring(0, 2);
      String data = line.substring(3);
      if (line.indexOf("NA")>=0)
        uName = data;
      else if (line.indexOf("TI")>=0) {
        uTime = data;
        done = true;
      }
    }
  }
  lcd.clear();
  if (done) {
    lcd.setCursor(0, 0);
    lcd.print("Name: ");
    lcd.print(uName);
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(uTime);
  } else {
   lcd.print(" Error time out ");
  }
  delay(2000);
  lcd.clear();
  lcd.print("    Tap Card    ");

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}
