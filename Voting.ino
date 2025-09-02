#include <Adafruit_Fingerprint.h>
#include <avr/pgmspace.h>
#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(2, 3);
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const int buttonPin = A0;
const int ledPinSystem = 13;
const int ledPinCandidate1 = 10;
const int ledPinCandidate2 = 11;
const int ledPinCandidate3 = 12;
const int ledPinActive = 9;
int buttonState = 0;

const int candidate1Button = A1;
const int candidate2Button = A2;
const int candidate3Button = A3;
const int resultButton = A4;
const int resetButton = A5;

uint16_t candidate1Votes = 0;
uint16_t candidate2Votes = 0;
uint16_t candidate3Votes = 0;
uint8_t totalVoters = 0;
bool hasVoted[128] = {false};

const uint8_t authorizedIDs[] PROGMEM = {1, 2, 3};
const uint8_t numAuthorizedIDs = sizeof(authorizedIDs) / sizeof(authorizedIDs[0]);

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(candidate1Button, INPUT_PULLUP);
  pinMode(candidate2Button, INPUT_PULLUP);
  pinMode(candidate3Button, INPUT_PULLUP);
  pinMode(resultButton, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);
  pinMode(ledPinSystem, OUTPUT);
  pinMode(ledPinCandidate1, OUTPUT);
  pinMode(ledPinCandidate2, OUTPUT);
  pinMode(ledPinCandidate3, OUTPUT);
  pinMode(ledPinActive, OUTPUT);
  digitalWrite(ledPinSystem, HIGH);
  digitalWrite(ledPinCandidate1, LOW);
  digitalWrite(ledPinCandidate2, LOW);
  digitalWrite(ledPinCandidate3, LOW);
  digitalWrite(ledPinActive, LOW);

  Serial.begin(9600);
  while (!Serial);
  delay(100);
  Serial.println(F("\n\nAdafruit finger detect test"));

  finger.begin(57600);
  delay(5);
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

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.print(F("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example."));
  } else {
    Serial.println(F("Waiting for valid finger..."));
    Serial.print(F("Sensor contains ")); Serial.print(finger.templateCount); Serial.println(F(" templates"));
  }
}

void loop() {
  buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {
    digitalWrite(ledPinSystem, LOW);
    digitalWrite(ledPinCandidate1, HIGH);
    digitalWrite(ledPinCandidate2, HIGH);
    digitalWrite(ledPinCandidate3, HIGH);
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      if (getFingerprintID() == FINGERPRINT_OK) {
        if (!hasVoted[finger.fingerID]) {
          startVoting(finger.fingerID);
          digitalWrite(ledPinSystem, HIGH);
          digitalWrite(ledPinCandidate1, LOW);
          digitalWrite(ledPinCandidate2, LOW);
          digitalWrite(ledPinCandidate3, LOW);
        } else {
          Serial.println(F("You have already voted!"));
          digitalWrite(ledPinSystem, HIGH);
          digitalWrite(ledPinCandidate1, LOW);
          digitalWrite(ledPinCandidate2, LOW);
          digitalWrite(ledPinCandidate3, LOW);
        }
        break;
      }
      delay(1000);
    }
    if (millis() - startTime >= 10000) {
      digitalWrite(ledPinCandidate1, LOW);
      digitalWrite(ledPinCandidate2, LOW);
      digitalWrite(ledPinCandidate3, LOW);
      digitalWrite(ledPinSystem, HIGH);
    }
  }

  if (digitalRead(resultButton) == LOW) {
    verifyResult();
    delay(1000);
  }

  if (digitalRead(resetButton) == LOW) {
    verifyReset();
    delay(1000);
  }
}

uint8_t getFingerprintID() {
  digitalWrite(ledPinActive, HIGH);

  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image taken"));
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(F("No finger detected"));
      digitalWrite(ledPinActive, LOW);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      digitalWrite(ledPinActive, LOW);
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println(F("Imaging error"));
      digitalWrite(ledPinActive, LOW);
      return p;
    default:
      Serial.println(F("Unknown error"));
      digitalWrite(ledPinActive, LOW);
      return p;
  }

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println(F("Image too messy"));
      digitalWrite(ledPinActive, LOW);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      digitalWrite(ledPinActive, LOW);
      return p;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println(F("Could not find fingerprint features"));
      digitalWrite(ledPinActive, LOW);
      return p;
    default:
      Serial.println(F("Unknown error"));
      digitalWrite(ledPinActive, LOW);
      return p;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println(F("Found a print match!"));
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println(F("Communication error"));
    digitalWrite(ledPinActive, LOW);
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println(F("Did not find a match"));
    digitalWrite(ledPinActive, LOW);
    return p;
  } else {
    Serial.println(F("Unknown error"));
    digitalWrite(ledPinActive, LOW);
    return p;
  }

  Serial.print(F("Found ID #")); Serial.print(finger.fingerID);
  Serial.print(F(" with confidence of ")); Serial.println(finger.confidence);

  digitalWrite(ledPinActive, LOW);

  showVoterDetails(finger.fingerID);

  return FINGERPRINT_OK;
}

void showVoterDetails(int id) {
  Serial.print(F("DETAILS_CORRESPONDING_TO_ID "));
  Serial.println(id);
  delay(1000);
  while (Serial.available() > 0) {
    String response = Serial.readString();
    Serial.println(response);
  }
}

void startVoting(int voterID) {
  Serial.println(F("You can now vote!"));
  Serial.println(F("Vote for your candidate:"));
  Serial.println(F("Press the button for Candidate 1, Candidate 2, or Candidate 3"));
  
  bool voteCast = false;

  unsigned long startTime = millis();

  while (!voteCast) {
    if (digitalRead(candidate1Button) == LOW) {
      candidate1Votes++;
      Serial.println(F("You voted for Candidate 1"));
      blinkLED(ledPinCandidate1);
      voteCast = true;
    } else if (digitalRead(candidate2Button) == LOW) {
      candidate2Votes++;
      Serial.println(F("You voted for Candidate 2"));
      blinkLED(ledPinCandidate2);
      voteCast = true;
    } else if (digitalRead(candidate3Button) == LOW) {
      candidate3Votes++;
      Serial.println(F("You voted for Candidate 3"));
      blinkLED(ledPinCandidate3);
      voteCast = true;
    }
    if (millis() - startTime > 15000) {
      Serial.println(F("No vote casted. You can try again."));
      return;
    }
    delay(100);
  }
  totalVoters++;
  hasVoted[voterID] = true;
}

void blinkLED(int ledPin) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }
}

void showResults() {
  if (totalVoters == 0) {
    Serial.println(F("No votes have been cast yet."));
    digitalWrite(ledPinSystem, HIGH);
    return;
  }

  Serial.println(F("Voting Results:"));
  Serial.print(F("Total voters: ")); Serial.println(totalVoters);
  Serial.print(F("Candidate 1 Votes: ")); Serial.println(candidate1Votes);
  Serial.print(F("Candidate 2 Votes: ")); Serial.println(candidate2Votes);
  Serial.print(F("Candidate 3 Votes: ")); Serial.println(candidate3Votes);

  if (candidate1Votes > candidate2Votes && candidate1Votes > candidate3Votes) {
    Serial.println(F("Candidate 1 is the winner!"));
    glowWinningLED(ledPinCandidate1);
  } else if (candidate2Votes > candidate1Votes && candidate2Votes > candidate3Votes) {
    Serial.println(F("Candidate 2 is the winner!"));
    glowWinningLED(ledPinCandidate2);
  } else if (candidate3Votes > candidate1Votes && candidate3Votes > candidate2Votes) {
    Serial.println(F("Candidate 3 is the winner!"));
    glowWinningLED(ledPinCandidate3);
  } else {
    Serial.println(F("It's a draw!"));
    if (candidate1Votes == candidate2Votes && candidate1Votes > candidate3Votes) {
      Serial.println(F("There is a tie between Candidate 1 & Candidate 2"));
      glowDrawLEDs(ledPinCandidate1, ledPinCandidate2);
    } else if (candidate1Votes == candidate3Votes && candidate1Votes > candidate2Votes) {
      Serial.println(F("There is a tie between Candidate 1 & Candidate 3"));
      glowDrawLEDs(ledPinCandidate1, ledPinCandidate3);
    } else if (candidate2Votes == candidate3Votes && candidate2Votes > candidate1Votes) {
      Serial.println(F("There is a tie between Candidate 2 & Candidate 3"));
      glowDrawLEDs(ledPinCandidate2, ledPinCandidate3);
    } else {
      Serial.println(F("There is a tie between Candidate 1, Candidate 2, and Candidate 3."));
      glowDrawLEDs(ledPinCandidate1, ledPinCandidate2, ledPinCandidate3);
    }
  }
  digitalWrite(ledPinSystem, HIGH);
}

void glowWinningLED(int ledPin) {
  digitalWrite(ledPin, HIGH);
  delay(3000);
  digitalWrite(ledPin, LOW);
  digitalWrite(ledPinSystem, HIGH);
}

void glowDrawLEDs(int ledPin1, int ledPin2) {
  digitalWrite(ledPin1, HIGH);
  digitalWrite(ledPin2, HIGH);
  delay(3000);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPinSystem, HIGH);
}

void glowDrawLEDs(int ledPin1, int ledPin2, int ledPin3) {
  digitalWrite(ledPin1, HIGH);
  digitalWrite(ledPin2, HIGH);
  digitalWrite(ledPin3, HIGH);
  delay(3000);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin3, LOW);
  digitalWrite(ledPinSystem, HIGH);
}

void resetVoting() {
  candidate1Votes = 0;
  candidate2Votes = 0;
  candidate3Votes = 0;
  totalVoters = 0;
  memset(hasVoted, 0, sizeof(hasVoted)); 
  digitalWrite(ledPinSystem, HIGH);
  digitalWrite(ledPinCandidate1, HIGH);
  digitalWrite(ledPinCandidate2, HIGH);
  digitalWrite(ledPinCandidate3, HIGH);
  digitalWrite(ledPinActive, HIGH);
  delay(1000); 
  digitalWrite(ledPinSystem, LOW);
  digitalWrite(ledPinCandidate1, LOW);
  digitalWrite(ledPinCandidate2, LOW);
  digitalWrite(ledPinCandidate3, LOW);
  digitalWrite(ledPinActive, LOW);
  delay(1000);
  digitalWrite(ledPinSystem, HIGH);
  digitalWrite(ledPinCandidate1, HIGH);
  digitalWrite(ledPinCandidate2, HIGH);
  digitalWrite(ledPinCandidate3, HIGH);
  digitalWrite(ledPinActive, HIGH);
  delay(1000); 
  digitalWrite(ledPinSystem, LOW);
  digitalWrite(ledPinCandidate1, LOW);
  digitalWrite(ledPinCandidate2, LOW);
  digitalWrite(ledPinCandidate3, LOW);
  digitalWrite(ledPinActive, LOW);
  delay(1000);
  Serial.println(F("The voting process has been reset."));
  digitalWrite(ledPinSystem, HIGH);
}

void verifyReset() {
  digitalWrite(ledPinSystem, LOW);
  Serial.println(F("Please authenticate to reset the voting process."));

  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { 
    if (getFingerprintID() == FINGERPRINT_OK) {
      for (uint8_t i = 0; i < numAuthorizedIDs; i++) {
        if (finger.fingerID == pgm_read_byte(&authorizedIDs[i])) {
          resetVoting();
          return;
        }
      }
    }
    delay(1000); 
  }

  Serial.println(F("Unauthorized attempt to reset the voting process."));
  digitalWrite(ledPinSystem, HIGH);
}

void verifyResult() {
  digitalWrite(ledPinSystem, LOW);
  Serial.println(F("Place your finger to verify result display..."));

  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { 
    if (getFingerprintID() == FINGERPRINT_OK) {
      for (uint8_t i = 0; i < numAuthorizedIDs; i++) {
        if (finger.fingerID == pgm_read_byte(&authorizedIDs[i])) {
          showResults();
          return;
        }
      }
    }
    delay(1000); 
  }

  Serial.println(F("Unauthorized attempt to display the results of voting."));
  digitalWrite(ledPinSystem,HIGH);
}
