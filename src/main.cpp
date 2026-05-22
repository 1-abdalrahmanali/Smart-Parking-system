#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>


void startWebsite();   
void handleWebClient(); 
// pin setinngs 
const int LDR_PIN       = 34;   
const int LAMP_1_PIN    = 25;   
const int LAMP_2_PIN    = 26;   

const int ENTRY_IR_PIN  = 4;    
const int EXIT_IR_PIN   = 5;    
const int SERVO_PIN     = 18;   
const int BUZZER_PIN    = 19;    
const int NIGHT_LED_PIN = 23;   

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gateServo;

const int TOTAL_SLOTS = 4;
int availableSlots = TOTAL_SLOTS;

const int GATE_OPEN_ANGLE   = 0;
const int GATE_CLOSED_ANGLE = 100;

bool carProcessing = false;
unsigned long lastActionTime = 0;
const unsigned long DEBOUNCE_TIME = 2000;

const int DARK_THRESHOLD  = 1500; 
const int LIGHT_THRESHOLD = 2200; 
bool lampsActive = false;

enum BuzzerMode { BUZZER_IDLE, BUZZER_SHORT, BUZZER_ALARM };
BuzzerMode buzzerMode = BUZZER_IDLE;
unsigned long buzzerStartedAt = 0;
unsigned long buzzerLastToggleAt = 0;
unsigned long buzzerDurationMs = 0;
bool buzzerOutputState = false;

void buzzerOn() { digitalWrite(BUZZER_PIN, HIGH); buzzerOutputState = true; }
void buzzerOff() { digitalWrite(BUZZER_PIN, LOW); buzzerOutputState = false; }

void startShortBeep(unsigned long durationMs) {
  buzzerMode = BUZZER_SHORT;
  buzzerStartedAt = millis();
  buzzerDurationMs = durationMs;
  buzzerOn();
}

void startFullAlarm() {
  buzzerMode = BUZZER_ALARM;
  buzzerStartedAt = millis();
  buzzerLastToggleAt = millis();
  buzzerDurationMs = 2500;
  buzzerOff();
}

void updateBuzzer() {
  unsigned long now = millis();
  if (buzzerMode == BUZZER_IDLE) return;
  if (buzzerMode == BUZZER_SHORT) {
    if (now - buzzerStartedAt >= buzzerDurationMs) { buzzerOff(); buzzerMode = BUZZER_IDLE; }
    return;
  }
  if (buzzerMode == BUZZER_ALARM) {
    if (now - buzzerStartedAt >= buzzerDurationMs) { buzzerOff(); buzzerMode = BUZZER_IDLE; return; }
    if (now - buzzerLastToggleAt >= 220) {
      buzzerLastToggleAt = now;
      if (buzzerOutputState) buzzerOff(); else buzzerOn();
    }
  }
}

void updateLightingLogic() {
    int ldrValue = analogRead(LDR_PIN);

    if (!lampsActive && ldrValue < DARK_THRESHOLD) {
        digitalWrite(LAMP_1_PIN, HIGH);
        digitalWrite(LAMP_2_PIN, HIGH);
        digitalWrite(NIGHT_LED_PIN, HIGH);
        lampsActive = true;
        Serial.println("LDR: It's Dark - Lamps ON");
    } 
    else if (lampsActive && ldrValue > LIGHT_THRESHOLD) {
        digitalWrite(LAMP_1_PIN, LOW);
        digitalWrite(LAMP_2_PIN, LOW);
        digitalWrite(NIGHT_LED_PIN, LOW);
        lampsActive = false;
        Serial.println("LDR: Bright Enough - Lamps OFF");
    }
}

void openGate() { gateServo.write(GATE_OPEN_ANGLE); }
void closeGate() { gateServo.write(GATE_CLOSED_ANGLE); }

void updateParkingLogic() {
  bool entryDetected = (digitalRead(ENTRY_IR_PIN) == LOW);
  bool exitDetected  = (digitalRead(EXIT_IR_PIN) == LOW);

  if (carProcessing || (millis() - lastActionTime < DEBOUNCE_TIME)) {
    if (!entryDetected && !exitDetected) carProcessing = false; 
    return;
  }

  if (entryDetected && !exitDetected) {
    if (availableSlots > 0) {
      availableSlots--;
      openGate(); 
      startShortBeep(120);
      carProcessing = true; 
      lastActionTime = millis();
    } else {
      startFullAlarm();
      carProcessing = true;
    }
  }
  else if (exitDetected && !entryDetected) {
    if (availableSlots < TOTAL_SLOTS) {
      availableSlots++;
      openGate();
      startShortBeep(80);
      carProcessing = true;
      lastActionTime = millis();
    }
  }
}

void updateDisplay() {
  static unsigned long lastRefresh = 0;
  if (millis() - lastRefresh < 500) return;
  lastRefresh = millis();

  lcd.setCursor(0, 0);
  if (availableSlots == 0) {
    lcd.print("  PARKING FULL  ");
    lcd.setCursor(0, 1); 
    lcd.print("  SORRY NO SLOT ");
  } else {
    lcd.print(" SMART PARKING  ");
    lcd.setCursor(0, 1);
    lcd.print("Slots Left: ");
    lcd.print(availableSlots);
    lcd.print("   ");
  }
}

void setup() {
  startWebsite();
  Serial.begin(115200);
  Wire.begin(21, 22); 
  lcd.init();
  lcd.backlight();
  
  pinMode(LDR_PIN, INPUT); 
  analogSetPinAttenuation(LDR_PIN, ADC_11db); 

  pinMode(ENTRY_IR_PIN, INPUT_PULLUP);
  pinMode(EXIT_IR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(NIGHT_LED_PIN, OUTPUT);
  pinMode(LAMP_1_PIN, OUTPUT);
  pinMode(LAMP_2_PIN, OUTPUT);

  gateServo.attach(SERVO_PIN);
  closeGate(); 
  
  lcd.print(" SYSTEM READY ");
  delay(1000);
  lcd.clear();
}

void loop() {
  handleWebClient();
  updateParkingLogic();  
  updateBuzzer();        
  updateDisplay();       
  updateLightingLogic(); 
  if (millis() - lastActionTime > 3000) {
    closeGate();
  }
}