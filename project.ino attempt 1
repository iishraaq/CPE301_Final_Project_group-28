
// Members: Ishraqul Islam, Peter Pickard


#include <LiquidCrystal.h>  // Allowed for LCD
#include <dht.h>            // Allowed for DHT11
#include <DS1307RTC.h>      // Allowed for RTC
#include <Stepper.h>        // Allowed for stepper motor
#include <TimeLib.h>        // Required for RTC

      // Pin Definitions
#define GREEN_LED 13    // PB7
#define YELLOW_LED 12   // PB6
#define RED_LED 11      // PB5
#define BLUE_LED 10     // PB4
#define FAN_PIN 43      // PL6
#define RESET_PIN 2     // PD2
#define STOP_PIN 3      // PD3
#define START_PIN 18    // PE2
#define VENT_LEFT_BUTTON 42  // PK0
#define VENT_RIGHT_BUTTON 46 // PK4
#define DHT_PIN 22
#define WATER_LEVEL_PIN A0

// LCD Pins
#define RS_PIN 4
#define EN_PIN 5
#define D4_PIN 6
#define D5_PIN 7
#define D6_PIN 8
#define D7_PIN 9

// --- Configuration Constants ---
const float TEMP_HIGH_LIMIT = 25.0;
const unsigned long DISPLAY_REFRESH_INTERVAL = 60000;
const int WATER_SENSOR_THRESHOLD = 100;

// --- Global Variables ---
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
dht DHT;
volatile bool startSignal = false;
float tempValue = 0.0;
float humidityValue = 0.0;
int waterSensorReading = 0;
unsigned long previousDisplayUpdate = 0;

// Using enum for system state 
enum SystemState { DISABLED, IDLE, ERROR, RUNNING };
SystemState systemState = DISABLED;

void setup() {
  // Initialize all hardware
  setupHardware();
  
  // Initial sensor read
  readSensors();
  
  // Start in disabled state
  setDisabledState();
}

void loop() {
  // State machine
  switch(systemState) {
    case DISABLED: handleDisabledState(); break;
    case IDLE: handleIdleState(); break;
    case ERROR: handleErrorState(); break;
    case RUNNING: handleRunningState(); break;
  }
}

// ========================
// Hardware Setup Functions
// ========================

void setupHardware() {
  // LCD Setup
  lcd.begin(16, 2);
  lcd.print("System Booting");
  
  // LED outputs (PB4-PB7)
  DDRB |= (1 << DDB7) | (1 << DDB6) | (1 << DDB5) | (1 << DDB4);
  
  // Fan output (PL6)
  DDRL |= (1 << DDL6);
  
  // Button inputs with pull-ups
  // RESET (PD2), STOP (PD3)
  DDRD &= ~((1 << DDD2) | (1 << DDD3));
  PORTD |= (1 << PORTD2) | (1 << PORTD3);
  
  // START (PE2)
  DDRE &= ~(1 << DDE2);
  PORTE |= (1 << PORTE2);
  
  // Vent buttons (PK0, PK4)
  DDRK &= ~((1 << DDK0) | (1 << DDK4));
  PORTK |= (1 << PORTK0) | (1 << PORTK4);
  
  // Set up interrupt for start button (allowed)
  attachInterrupt(digitalPinToInterrupt(START_PIN), startButtonISR, FALLING);
  
  // Configure ADC
  ADMUX = (1 << REFS0);  // AVcc reference
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);  // Enable ADC, prescaler 64
}

// ========================
// Sensor Reading Functions
// ========================

void readSensors() {
  // Read DHT11 
  DHT.read11(DHT_PIN);
  tempValue = DHT.temperature;
  humidityValue = DHT.humidity;
  
  // Read water level
  waterSensorReading = readADC(WATER_LEVEL_PIN);
}

int readADC(uint8_t channel) {
  channel &= 0x07;  // Limit to 0-7
  ADMUX = (ADMUX & 0xF8) | channel;  // Set channel
  ADCSRA |= (1 << ADSC);              // Start conversion
  while (ADCSRA & (1 << ADSC));       // Wait for completion
  return ADC;
}

// ========================
// State Handling Functions
// ========================

void setDisabledState() {
  // Yellow LED on, others off
  PORTB = (PORTB & 0x0F) | (1 << PORTB6);
  // Fan off
  PORTL &= ~(1 << PORTL6);
  
  lcd.clear();
  lcd.print("System Disabled");
}

void handleDisabledState() {
  if (startSignal) {
    systemState = IDLE;
    startSignal = false;
    logEvent("System Enabled");
    setIdleState();
  }
}

void setIdleState() {
  // Green LED on, others off
  PORTB = (PORTB & 0x0F) | (1 << PORTB7);
  // Fan off
  PORTL &= ~(1 << PORTL6);
  
  updateLCD();
}

void handleIdleState() {
  readSensors();
  checkWaterLevel();
  
  if (tempValue >= TEMP_HIGH_LIMIT) {
    systemState = RUNNING;
    logEvent("Fan Started");
    setRunningState();
  }
  
  // Check stop button (PD3)
  if (!(PIND & (1 << PIND3))) {
    systemState = DISABLED;
    logEvent("System Stopped");
    setDisabledState();
  }
  
  updateLCD();
  controlVent();
}


void logEvent(const char* message) {
  tmElements_t tm;
  if (RTC.read(tm)) {
    lcd.clear();
    lcd.print(tm.Hour);
    lcd.print(":");
    lcd.print(tm.Minute);
    lcd.print(" ");
    lcd.print(message);
    delay(2000);  // Show message for 2 seconds
  }
}

void updateLCD() {
  if (millis() - previousDisplayUpdate >= DISPLAY_REFRESH_INTERVAL) {
    previousDisplayUpdate = millis();
    lcd.clear();
    lcd.print("T:");
    lcd.print(tempValue);
    lcd.print("C H:");
    lcd.print(humidityValue);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Water:");
    lcd.print(waterSensorReading);
  }
}

// Interrupt Service Routine
void startButtonISR() {
  startSignal = true;
}
