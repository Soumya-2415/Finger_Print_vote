#include <Adafruit_Fingerprint.h>
#include <avr/pgmspace.h>
#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3);
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id = 4; // Start ID from 4 (Default ID)
const int ledPinActive = 9; // Define the LED pin
const int ledPinSystem = 13; // Define the system status LED pin

const char msgFingerprintSensor[] PROGMEM = "Adafruit Fingerprint sensor enrollment";
const char msgFoundSensor[] PROGMEM = "Found fingerprint sensor!";
const char msgNotFoundSensor[] PROGMEM = "Did not find fingerprint sensor :(";
const char msgReadingParams[] PROGMEM = "Reading sensor parameters";
const char msgStatus[] PROGMEM = "Status: 0x";
const char msgSysID[] PROGMEM = "Sys ID: 0x";
const char msgCapacity[] PROGMEM = "Capacity: ";
const char msgSecurityLevel[] PROGMEM = "Security level: ";
const char msgDeviceAddr[] PROGMEM = "Device address: ";
const char msgPacketLen[] PROGMEM = "Packet len: ";
const char msgBaudRate[] PROGMEM = "Baud rate: ";

void setup() {
  pinMode(ledPinActive, OUTPUT); // Initialize the active LED pin as an output
  pinMode(ledPinSystem, OUTPUT); // Initialize the system status LED pin as an output
  digitalWrite(ledPinActive, LOW); // Ensure the active LED is off initially
  digitalWrite(ledPinSystem, HIGH); // Turn the system status LED on initially
  
  Serial.begin(9600);
  while (!Serial);
  delay(100);
  Serial.println(F("\n\nAdafruit Fingerprint sensor enrollment"));

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println(F("Found fingerprint sensor!"));
  } else {
    Serial.println(F("Did not find fingerprint sensor :("));
    while (1) { delay(1); }
  }
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'E') {
      Serial.println(F("Ready to enroll a fingerprint!"));
      Serial.print(F("Enrolling ID #"));
      Serial.println(id);

      int enrollmentResult = getFingerprintEnroll();

      if (enrollmentResult == FINGERPRINT_OK) {
        Serial.println(F("Stored!"));
        id++; // Increment ID only if enrollment was successful
      } else {
        Serial.print(F("Enrollment failed with error code "));
        Serial.println(enrollmentResult);
      }
    } else if (command == 'S') {
      // Set starting ID
      id = Serial.parseInt();
      Serial.print(F("Starting ID set to "));
      Serial.println(id);
    }
  }
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  digitalWrite(ledPinSystem, LOW); // Turn off the system status LED
  Serial.print(F("Waiting for valid finger to enroll as #")); Serial.println(id);
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    blinkLED();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println(F("Image taken"));
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(F("."));
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println(F("Communication error"));
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println(F("Imaging error"));
        break;
      default:
        Serial.println(F("Unknown error"));
        break;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
    return p;
  }

  Serial.println(F("Remove finger"));
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  Serial.println(F("Place same finger again"));
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    blinkLED();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println(F("Image taken"));
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(F("."));
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println(F("Communication error"));
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println(F("Imaging error"));
        break;
      default:
        Serial.println(F("Unknown error"));
        break;
    }
  }
  
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println(F("Image too messy"));
      digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println(F("Could not find fingerprint features"));
      digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println(F("Could not find fingerprint features"));
      digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
      return p;
    default:
      Serial.println(F("Unknown error"));
      digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
      return p;
  }

  // OK converted!
  Serial.print(F("Creating model for #"));  Serial.println(id);

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("Failed to create model"));
    digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED if failed
    return p;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println(F("Success!"));
  } else {
    Serial.print(F("Store failed with error code "));
    Serial.println(p);
  }

  digitalWrite(ledPinSystem, HIGH); // Turn on the system status LED after successful enrollment
  return p;
}

void blinkLED() {
  digitalWrite(ledPinActive, HIGH);
  delay(100);
  digitalWrite(ledPinActive, LOW);
  delay(100);
}