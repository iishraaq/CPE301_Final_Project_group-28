// Group#28
// Members: Ishraqul Islam, Peter Pickard

#include <LiquidCrystal.h>
#include <dht.h>
#include <Servo.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Stepper.h>
#include <TimeLib.h>

// --- Pin Definitions ---
#define GREEN_LED 13
#define YELLOW_LED 12
#define RED_LED 11
#define BLUE_LED 10
#define FAN_PIN 43
#define RESET_PIN 2
#define STOP_PIN 3
#define START_PIN 18
#define VENT_LEFT_BUTTON 42
#define VENT_RIGHT_BUTTON 46
#define SERVO_PIN 45
#define DHT_PIN 22
#define RS_PIN 4
#define EN_PIN 5
#define D4_PIN 6
#define D5_PIN 7
#define D6_PIN 8
#define D7_PIN 9
#define WATER_LEVEL_PIN A0

// --- Configuration Constants ---
const float TEMP_HIGH_LIMIT = 25.0;
const unsigned long DISPLAY_REFRESH_INTERVAL = 60000;
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;
const int SERVO_MOVE_STEP = 15;
const int WATER_SENSOR_THRESHOLD = 100;

// --- Global Variables ---
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
dht DHT;
Servo ventServo;
String systemStatus = "DISABLED";
volatile bool startSignal = false;
float tempValue = 0.0;
float humidityValue = 0.0;
int waterSensorReading = 0;
unsigned long previousDisplayUpdate = 0;
int ventAngle = 90;

// --- Function Declarations ---
void setupSystem();
void manageSystemState();
void readSensors();
void updateLCD();
void logSystemEvent(String text);
void recordStartupTime();
void controlVent();
void evaluateWaterLevel();
void stateDisabled();
void stateIdle();
void stateError();
void stateRunning();
void onStartButtonPress();
void configureADC();
int fetchADC();

void setup() {
  setupSystem();
}

void loop() {
  manageSystemState();
}

void setupSystem() {
  Serial.begin(9600);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(STOP_PIN, INPUT_PULLUP);
  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(VENT_LEFT_BUTTON, INPUT_PULLUP);
  pinMode(VENT_RIGHT_BUTTON, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(START_PIN), onStartButtonPress, RISING);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("System Booting...");

  DHT.read11(DHT_PIN);
  ventServo.attach(SERVO_PIN);
  ventServo.write(ventAngle);

  configureADC();
  readSensors();
  stateDisabled();
}

void manageSystemState() {
  if (systemStatus == "DISABLED") stateDisabled();
  else if (systemStatus == "IDLE") stateIdle();
  else if (systemStatus == "ERROR") stateError();
  else if (systemStatus == "RUNNING") stateRunning();
}

void stateDisabled() {
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(FAN_PIN, LOW);
  lcd.clear();
  lcd.print("System Disabled");
  if (startSignal) {
    systemStatus = "IDLE";
    lcd.clear();
    lcd.print("Enabling System");
    logSystemEvent("System Enabled");
    recordStartupTime();
    startSignal = false;
  }
}

void stateIdle() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(FAN_PIN, LOW);

  readSensors();
  evaluateWaterLevel();
  updateLCD();
  controlVent();

  if (tempValue >= TEMP_HIGH_LIMIT) {
    systemStatus = "RUNNING";
    logSystemEvent("Entering RUNNING state");
  }
  if (digitalRead(STOP_PIN) == LOW) {
    systemStatus = "DISABLED";
    logSystemEvent("System Stopped");
  }
}

void stateError() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(FAN_PIN, LOW);

  lcd.clear();
  lcd.print("Check Water Level!");
  if (digitalRead(RESET_PIN) == LOW && waterSensorReading > WATER_SENSOR_THRESHOLD) {
    systemStatus = "IDLE";
    logSystemEvent("Error Cleared");
  }
}

void stateRunning() {
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(FAN_PIN, HIGH);

  readSensors();
  updateLCD();
  controlVent();
  evaluateWaterLevel();

  if (tempValue < TEMP_HIGH_LIMIT) {
    systemStatus = "IDLE";
    logSystemEvent("Temperature Normalized");
  }
  if (digitalRead(STOP_PIN) == LOW) {
    systemStatus = "DISABLED";
    logSystemEvent("System Stopped");
  }
}

void readSensors() {
  DHT.read11(DHT_PIN);
  tempValue = DHT.temperature;
  humidityValue = DHT.humidity;
  waterSensorReading = fetchADC();

  Serial.print("Temp: "); Serial.print(tempValue);
  Serial.print(" C, Humidity: "); Serial.print(humidityValue);
  Serial.print(" %, Water: "); Serial.println(waterSensorReading);
}

void updateLCD() {
  if (millis() - previousDisplayUpdate >= DISPLAY_REFRESH_INTERVAL) {
    previousDisplayUpdate = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tempValue);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Hum:");
    lcd.print(humidityValue);
    lcd.print("% W:");
    lcd.print(waterSensorReading);
  }
}

void controlVent() {
  if (digitalRead(VENT_LEFT_BUTTON) == LOW && ventAngle > SERVO_MIN) {
    ventAngle -= SERVO_MOVE_STEP;
    ventServo.write(ventAngle);
    logSystemEvent("Vent Left: " + String(ventAngle));
  } else if (digitalRead(VENT_RIGHT_BUTTON) == LOW && ventAngle < SERVO_MAX) {
    ventAngle += SERVO_MOVE_STEP;
    ventServo.write(ventAngle);
    logSystemEvent("Vent Right: " + String(ventAngle));
  }
}

void evaluateWaterLevel() {
  if (waterSensorReading < WATER_SENSOR_THRESHOLD) {
    systemStatus = "ERROR";
    logSystemEvent("Water Level Critical");
  }
}

void logSystemEvent(String text) {
  tmElements_t tm;
  if (RTC.read(tm)) {
    Serial.print(tm.Hour); Serial.print(":");
    Serial.print(tm.Minute); Serial.print(":");
    Serial.print(tm.Second); Serial.print(" ");
    Serial.print(tm.Month); Serial.print("/");
    Serial.print(tm.Day); Serial.print("/");
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(" - ");
    Serial.println(text);
  } else {
    Serial.println("RTC Failed to Read");
  }
}

void recordStartupTime() {
  logSystemEvent("System Startup Time");
}

void onStartButtonPress() {
  startSignal = true;
}

void configureADC() {
  ADMUX = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

int fetchADC() {
  ADMUX &= 0xF0;
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}
