#include <SPI.h> 
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define SS_PIN 10 
#define RST_PIN 9 
#define LED_DENIED_PIN 7
#define LED_ACCESS_PIN 6

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(3, 2); //SIM800L Tx & Rx is connected to Arduino #3 & #2

LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522 mfrc522(SS_PIN, RST_PIN); // Instance of the class

#define numberOfPass 10
String authorisedCode[numberOfPass] = { "3574122247", "1952110254" };
bool userStatus[numberOfPass] = { false, false };

int codeRead = 0;
int userStatusCount = 0;

String uidString;

void setup() {
  Serial.begin(9600);
  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);

  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  updateSerial();
  mySerial.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  updateSerial();
  mySerial.println("AT+CREG?"); //Check whether it has registered in the network
  updateSerial();

  SPI.begin();       // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 

  Serial.println("Arduino RFID reading UID");
  pinMode( LED_DENIED_PIN , OUTPUT); 
  pinMode( LED_ACCESS_PIN , OUTPUT);

  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Show your card:)");
}

void loop() {
  updateSerial();

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

  if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  lcd.clear();
  Serial.print("Tag UID:");
  lcd.setCursor(0,0);
  lcd.print("Tag UID:");
  lcd.setCursor(0,1);

  String readCardId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    readCardId.concat(mfrc522.uid.uidByte[i]);
  }
  Serial.print(readCardId);
  lcd.print(readCardId);
  Serial.println();

  boolean match =  false;
  boolean wasTrue = false;
  for (int i = 0; i < sizeof(authorisedCode); i++) {
    if(authorisedCode[i] == readCardId) {
      if (userStatus[i] == true) {
        wasTrue = true;
      }
      userStatus[i] = true;
      match = true;
    }
  }

  if (!wasTrue) {
    userStatusCount++;
    sendMessage("User " + readCardId + " aboard.");
  }

  delay(3000);

  lcd.clear();
  lcd.setCursor(0,0);
  
  if (match) {
    digitalWrite( LED_ACCESS_PIN , HIGH);
    lcd.print("Authorized access");
  } else {
    digitalWrite( LED_DENIED_PIN , HIGH);
    lcd.print(" Access denied  "); 
    Serial.println("\nUnknown Card");
  }
  Serial.println("============================");
  mfrc522.PICC_HaltA();
  delay(3000); 
  reset_state();
}

void reset_state() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Show your card:)");
  lcd.setCursor(0,1);
  lcd.print("Total: ");
  lcd.print(userStatusCount);
  digitalWrite( LED_ACCESS_PIN , LOW);
  digitalWrite( LED_DENIED_PIN , LOW);
}

void updateSerial() {
  delay(500);
  while (Serial.available()) {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial.available()) {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}

void sendMessage(String message) {
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  mySerial.println("AT+CMGS=\"+254704351180\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  mySerial.print(message); //text content
  updateSerial();
  mySerial.write(26);
}