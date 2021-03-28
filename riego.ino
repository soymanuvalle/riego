#include <arduino-timer.h>
#include <Button.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

Timer<2> timer;
RTC_DS1307 rtc;

// Sprinklers
const int sprinklerPin1 = 9;
const int sprinklerPin2 = 10;
const int sprinklerPin3 = 8;
int activeSprinkler = 0;

// Modes
const int modeLedPin = 22;
Button modeButton(24);
int activeMode = 1;
int modeLedState = LOW;
int modeLedIndex = 0;

// Setup
void setup() {
  Serial.begin(9600); // Prepare serial interface
  
  #ifndef ESP8266
    while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif

  Serial.println("");
  Serial.println(F("----------------------------------------------------"));
  Serial.println(F("            SISTEMA DE RIEGO AUTOMATICO             "));
  Serial.println(F("                       v1.0                         "));
  Serial.println(F("----------------------------------------------------"));

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC Module not found.");
    Serial.flush();
    abort();
  }

  // Set the RTC datetime to the last time this sketch was compiled
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Set active mode from EEPROM storage
  int storedActiveMode = EEPROM.read(0);
  Serial.print("Stored active mode: ");
  Serial.println(storedActiveMode);

  if (storedActiveMode == 255) {
    activeMode = 1;
    EEPROM.write(0, activeMode);
  } else {
    activeMode = storedActiveMode;
  }

  // Setup sprinklers
  pinMode(sprinklerPin1, OUTPUT);
  pinMode(sprinklerPin2, OUTPUT);
  pinMode(sprinklerPin3, OUTPUT);
  digitalWrite(sprinklerPin1, HIGH);
  digitalWrite(sprinklerPin2, HIGH);
  digitalWrite(sprinklerPin3, HIGH);

  // Setup mode
  pinMode(modeLedPin, OUTPUT);
  modeButton.begin();

  // Timers
  timer.every(200, updateModeLed);
  timer.every(1000, updateSprinklers);
}

// Loop
void loop() {
  timer.tick();
  
  // Handle mode change
  if (modeButton.released()) {
    if (activeMode == 3) activeMode = 1;
    else activeMode++;
    //EEPROM.write(0, activeMode);
    Serial.println("Active mode: " + (String)activeMode);

    // Reset mode led
    modeLedState = LOW;
    digitalWrite(modeLedPin, LOW);
    modeLedIndex = 0;
  }
}

bool updateModeLed(void *) {
  if (modeLedIndex < activeMode * 2) {
    if (modeLedState == LOW) {
      modeLedState = HIGH;
      digitalWrite(modeLedPin, HIGH);
    } else {
      modeLedState = LOW;
      digitalWrite(modeLedPin, LOW);
    }

    modeLedIndex++;
  }

  return true;
}

void setActiveSprinklerOnce(int index) {
  if (activeSprinkler != index) {
    activeSprinkler = index;
    Serial.print("Active sprinkler: ");
    Serial.println(activeSprinkler);
    
    if (activeSprinkler == 1) {
      digitalWrite(sprinklerPin1, LOW);
      digitalWrite(sprinklerPin2, HIGH);
      digitalWrite(sprinklerPin3, HIGH);
    } else if (activeSprinkler == 2) {
      digitalWrite(sprinklerPin1, HIGH);
      digitalWrite(sprinklerPin2, LOW);
      digitalWrite(sprinklerPin3, HIGH);
    } else if (activeSprinkler == 3) {
      digitalWrite(sprinklerPin1, HIGH);
      digitalWrite(sprinklerPin2, HIGH);
      digitalWrite(sprinklerPin3, LOW);
    } else {
      digitalWrite(sprinklerPin1, HIGH);
      digitalWrite(sprinklerPin2, HIGH);
      digitalWrite(sprinklerPin3, HIGH);
    }
  }
}

bool updateSprinklers(void *) {
  DateTime date = rtc.now();
  const int hour = date.hour();
  const int minute = date.minute();

  Serial.print("Hour: ");
  Serial.print(hour);
  Serial.print(" Minute: ");
  Serial.println(minute);
  
  if (
    (activeMode == 1 && (hour == 10 || hour == 20)) || // Mode 1
    (activeMode == 2 && (hour == 10 || hour == 12 || hour == 20)) || // Mode 2
    (activeMode == 3 && (hour == 10 || hour == 12 || hour == 15 || hour == 17 || hour == 20)) // Mode 3
  ) {
    if (minute >= 0 && minute <= 19) {
      setActiveSprinklerOnce(1);
    } else if (minute >= 20 && minute <= 39) {
      setActiveSprinklerOnce(2);
    } else if (minute >= 40 && minute <= 59) {
      setActiveSprinklerOnce(3);
    } else {
      setActiveSprinklerOnce(0);
    }
  // Other
  } else {
    setActiveSprinklerOnce(0);
  }

  return true;
}
