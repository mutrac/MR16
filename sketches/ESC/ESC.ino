/*
  Engine Safety Controller (ESC)
  McGill ASABE Engineering Team
 
 Board: Arduino Mega 2560
 Shields: Dual VNH5019 Motor Shield
 */

/* --- LIBRARIES --- */
#include "DualVNH5019MotorShield.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include <RunningMedian.h>

/* --- Global Constants --- */
/// Digital Pins
// D14 Reserved for Serial2 RFID
// D15 Reserved for Serial2 RFID
const int LPH_SENSOR_PIN = 18;  // interrupt (#5) required

// Failsafe Digital Input
const int BUTTON_KILL_POUT = 28;
const int HITCH_KILL_POUT = 30;
const int SEAT_KILL_POUT = 32;
const int SEAT_KILL_PIN = 26;
const int HITCH_KILL_PIN = 24;
const int BUTTON_KILL_PIN = 22;

// Joystick (Digital) 
const int IGNITION_PIN = 23;
const int TRIGGER_KILL_PIN = 25;
const int PULL_MODE_PIN = 27;
// D29 is a Blocked pin
const int CART_FORWARD_PIN = 31;
const int CART_BACKWARD_PIN = 33;
const int CART_MODE_PIN = 35;
// D37 is a Blocked pin
const int THROTTLE_HIGH_PIN = 39; // These are set to an interrupt
const int THROTTLE_LOW_PIN = 41; // These are set to an interrupt
const int THROTTLE_UP_PIN = 43; // These are set to an interrupt
// D45 is a Blocked pin
const int THROTTLE_DOWN_PIN = 47;
const int DISPLAY_MODE_PIN = 49;

// Relay Pins 
const int STOP_RELAY_PIN = 38; // TODO: check pin number
const int REGULATOR_RELAY_PIN = 40;
const int STARTER_RELAY_PIN = 42;
const int REBOOT_RELAY_PIN = 44;

// Additional Sensors
const int TEMP_SENSOR_PIN = 46; // TODO: find actual pin number

/// Analog Input Pins
// A0 Reserved for DualVNH5019
// A1 Reserved for DualVNH5019
const int THROTTLE_POS_PIN = A8;
const int THROTTLE_POS_MIN_PIN = A9;
const int THROTTLE_POS_MAX_PIN = A10;
const int PSI_SENSOR_PIN = A11;
const int CVT_GUARD_POUT = A12;
const int CVT_GUARD_PIN = A13;
const int LEFT_BRAKE_PIN = A14;
const int RIGHT_BRAKE_PIN = A15;

/* --- Global Settings --- */ 
/// CMQ
const char UID[] = "ESC";
const char PUSH[] = "push";
const char PULL[] = "pull";
const int BAUD = 19200;
const int OUTPUT_SIZE = 512;
const int DATA_SIZE = 256;

/// RFID
const int RFID_BAUD = 9600;
const int RFID_READ = 0x02;

/// Time Delays
const int KILL_WAIT = 1000;
const int IGNITION_WAIT = 500;
const int MOTORS_WAIT = 100;
const int STANDBY_WAIT = 10;
const int REBOOT_WAIT = 1000;

/// Brake variables
const int BRAKES_THRESHOLD = 150;
const int BRAKES_MILLIAMP_THRESH = 15000;
const int BRAKES_MIN = 0;
const int BRAKES_MAX = 512;

/// Throttle
const int THROTTLE_POS_MIN = 274;
const int THROTTLE_POS_MAX = 980;
const int THROTTLE_MIN = 0;
const int THROTTLE_MAX = 1024;
const int THROTTLE_MILLIAMP_THRESHOLD = 15000;
const int THROTTLE_P =10 ;
const int THROTTLE_I = 1;
const int THROTTLE_D = 2;
const int THROTTLE_STEP = 64;

/// CVT Guard
const int CVT_GUARD_THRESHOLD = 600;

// Seat
const int SEAT_LIMIT = 10;

// Engine sensors
const int LPH_SAMPLESIZE = 20;
const int PSI_SAMPLESIZE = 5;
const int TEMP_SAMPLESIZE = 3;
const int DIGITS = 4;
const int PRECISION = 2;

/* --- GLOBAL VARIABLES --- */
/// Safety switches, rfid, and Joystick button variables
int SEAT_COUNTER = 0;
int SEAT_KILL = 0;
int HITCH_KILL = 0;
int TRIGGER_KILL = 0;
int BUTTON_KILL = 0;
int IGNITION = 0;
int PULL_MODE = 0;
int CVT_GUARD = 0;
int RUN_MODE = 0;
int LEFT_BRAKE = 0;
int RIGHT_BRAKE = 0;
int RFID_AUTH = 0;
int DISPLAY_MODE = 0; // the desired display mode on the HUD
int CART_FORWARD = 0;
int CART_BACKWARD = 0;
int CART_MODE = 0;
int THROTTLE_HIGH = 0;
int THROTTLE_LOW = 0;
int THROTTLE_UP = 0;
int THROTTLE_DOWN = 0;
int THROTTLE = THROTTLE_MIN;
float LPH = 0;
float PSI = 0;
float TEMP = 0;

// Volatile values (Asynchronous variables)
volatile int LPH_COUNTER = 0;
volatile long LPH_TIME_A = millis();
volatile long LPH_TIME_B = millis();

/// String output
char DATA_BUFFER[DATA_SIZE];
char OUTPUT_BUFFER[OUTPUT_SIZE];
char TEMP_BUF[DIGITS + PRECISION];
char LPH_BUF[DIGITS + PRECISION];
char PSI_BUF[DIGITS + PRECISION];

/// Throttle PID values
int THROTTLE_SET;
int THROTTLE_IN;
int THROTTLE_OUT;

/* --- Global Objects --- */

/// Dual Motor Controller (M1 vs. M2)
DualVNH5019MotorShield ESC;

/// Temperature probe
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature TEMP_SENSOR(&oneWire);

// Fuel flow and Oil pressure
RunningMedian LPH_HIST = RunningMedian(LPH_SAMPLESIZE);
RunningMedian PSI_HIST = RunningMedian(PSI_SAMPLESIZE);
RunningMedian TEMP_HIST = RunningMedian(TEMP_SAMPLESIZE);

/* --- SETUP --- */
void setup() {
 
  // Initialize RFID authentication device
  Serial3.begin(RFID_BAUD); // Pins 14 and 15
  Serial3.write(RFID_READ);

  // Failsafe Seat
  pinMode(SEAT_KILL_PIN, INPUT);
  digitalWrite(SEAT_KILL_PIN, HIGH);
  pinMode(SEAT_KILL_POUT, OUTPUT);
  digitalWrite(SEAT_KILL_POUT, LOW);
  
  // Failsafe Hitch
  pinMode(HITCH_KILL_PIN, INPUT);
  digitalWrite(HITCH_KILL_PIN, HIGH);
  pinMode(HITCH_KILL_POUT, OUTPUT);
  digitalWrite(HITCH_KILL_POUT, LOW);
  
  // Failsafe Button
  pinMode(BUTTON_KILL_PIN, INPUT);
  digitalWrite(BUTTON_KILL_PIN, HIGH);
  pinMode(BUTTON_KILL_POUT, OUTPUT);
  digitalWrite(BUTTON_KILL_POUT, LOW);

  // Joystick Digital Input
  pinMode(IGNITION_PIN, INPUT);
  pinMode(TRIGGER_KILL_PIN, INPUT);
  pinMode(PULL_MODE_PIN, INPUT);
  pinMode(CART_FORWARD_PIN, INPUT);
  pinMode(CART_BACKWARD_PIN, INPUT);
  pinMode(CART_MODE_PIN, INPUT);
  pinMode(THROTTLE_HIGH_PIN, INPUT);
  pinMode(THROTTLE_LOW_PIN, INPUT);
  pinMode(THROTTLE_UP_PIN, INPUT);
  pinMode(THROTTLE_DOWN_PIN, INPUT);
  pinMode(DISPLAY_MODE_PIN, INPUT);
  
  // Throttle
  pinMode(THROTTLE_POS_PIN, INPUT);
  pinMode(THROTTLE_POS_MIN_PIN, OUTPUT);
  digitalWrite(THROTTLE_POS_MIN_PIN, LOW);
  pinMode(THROTTLE_POS_MAX_PIN, OUTPUT);
  digitalWrite(THROTTLE_POS_MAX_PIN, HIGH);

  // Relays
  pinMode(STOP_RELAY_PIN, OUTPUT); digitalWrite(STOP_RELAY_PIN, HIGH);
  pinMode(REGULATOR_RELAY_PIN, OUTPUT); digitalWrite(REGULATOR_RELAY_PIN, HIGH);
  pinMode(STARTER_RELAY_PIN, OUTPUT); digitalWrite(STARTER_RELAY_PIN, HIGH);
  pinMode(REBOOT_RELAY_PIN, OUTPUT); digitalWrite(REBOOT_RELAY_PIN, HIGH);

  // Reboot host
  reboot();
  
  // Brakes
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
  
  // CVT Guard
  pinMode(CVT_GUARD_POUT, OUTPUT);
  digitalWrite(CVT_GUARD_POUT, LOW);
  pinMode(CVT_GUARD_PIN, INPUT);
  digitalWrite(CVT_GUARD_PIN, HIGH);

  // DualVNH5019 Motor Controller
  ESC.init(); 

  // Engine temperature sensor DS18B20
  TEMP_SENSOR.begin();
  
  // Fuel Sensor
  attachInterrupt(LPH_SENSOR_PIN, lph_counter, RISING);
  
  // Begin serial communication
  Serial.begin(BAUD);
}

/* --- LOOP --- */
void loop() {

  // Check Failsafe Switches (Default to 1 if disconnected)
  HITCH_KILL = check_failsafe_switch(HITCH_KILL_PIN);
  BUTTON_KILL = check_failsafe_switch(BUTTON_KILL_PIN);
  
  // Check Seat
  SEAT_KILL = check_seat();
  
  // Check Regular Switches (Default to 0 if disconnected)
  TRIGGER_KILL = check_switch(TRIGGER_KILL_PIN);
  IGNITION = check_switch(IGNITION_PIN);
  DISPLAY_MODE = check_switch(DISPLAY_MODE_PIN);
  CART_FORWARD = check_switch(CART_FORWARD_PIN);
  CART_BACKWARD = check_switch(CART_BACKWARD_PIN);
  THROTTLE_UP = check_switch(THROTTLE_UP_PIN);
  THROTTLE_DOWN = check_switch(THROTTLE_DOWN_PIN);
  THROTTLE_HIGH = check_switch(THROTTLE_HIGH_PIN);
  THROTTLE_LOW = check_switch(THROTTLE_LOW_PIN);
  
  // Change VDC and TCS modes
  if (check_switch(CART_MODE_PIN)) {
    if (CART_MODE == 1) { CART_MODE = 0; }
    else { CART_MODE = 1; }
  }
  if (check_switch(PULL_MODE_PIN)) {
     if (PULL_MODE == 1) { PULL_MODE = 0; }
    else { PULL_MODE = 1; }
  }
  
  // Check non-switches
  CVT_GUARD = check_guard();
  if (RFID_AUTH == 0) {
    RFID_AUTH = check_rfid();
  }
  LEFT_BRAKE = check_brake(LEFT_BRAKE_PIN);
  RIGHT_BRAKE = check_brake(RIGHT_BRAKE_PIN);
  
  // Check engine condition
  TEMP = get_engine_temp();
  LPH = get_fuel_lph();
  PSI = get_oil_psi();
  
  // Set brakes ALWAYS
  set_brakes(RIGHT_BRAKE, LEFT_BRAKE);
    
  //  // Adjust throttle limit, either HIGH/LOW, or increment
  if (THROTTLE_HIGH  && !THROTTLE_LOW) {
    THROTTLE = THROTTLE_MAX;
  }
  else if (THROTTLE_LOW && !THROTTLE_HIGH) {
    THROTTLE = THROTTLE_MIN;
  }
  else if (THROTTLE_UP && !THROTTLE_DOWN) {
    THROTTLE = THROTTLE + THROTTLE_STEP;
  }
  else if (THROTTLE_DOWN && !THROTTLE_UP) {
    THROTTLE = THROTTLE - THROTTLE_STEP;
  }
  else {
    if (THROTTLE > THROTTLE_MAX) {
      THROTTLE = THROTTLE_MAX;
    }
    else if (THROTTLE < THROTTLE_MIN) {
      THROTTLE = THROTTLE_MIN;
    }
  }
  if (TRIGGER_KILL) {
    set_throttle(THROTTLE);
  }
  else {
    set_throttle(0); // DISABLE THROTTLE IF OPERATOR RELEASES TRIGGER
  }
  
  // (0) If OFF
  if (RUN_MODE == 0) {
    if (RFID_AUTH != 0 && !SEAT_KILL) {
      standby();
    }
  }
  // (1) If STANDBY
  else if (RUN_MODE == 1) {
    if (SEAT_KILL || HITCH_KILL || BUTTON_KILL) {
      kill(); // kill engine
    }
    else if (IGNITION && LEFT_BRAKE && RIGHT_BRAKE && !CVT_GUARD) {
      ignition(); // execute ignition sequence
    }
    else {
      standby(); // remain in standby (RUN_MODE 1)
    }
  }
  // (2) If DRIVE
  else if (RUN_MODE == 2) {
    if (SEAT_KILL || HITCH_KILL || BUTTON_KILL || TRIGGER_KILL) {
      kill(); // kill engine
      standby();
    }
    else {
      /* STUFF PERTAINING TO RUN_MODE 2 */
    }
  }
  // If ??? (UNKNOWN)
  else {
    kill();
  }

  // Format float to string
  dtostrf(LPH, DIGITS, PRECISION, LPH_BUF);
  dtostrf(TEMP, DIGITS, PRECISION, TEMP_BUF);
  dtostrf(PSI, DIGITS, PRECISION, PSI_BUF);
  
  // Output to USB Serial
  sprintf(DATA_BUFFER, "{'run_mode':%d,'display_mode':%d,'right_brake':%d,'left_brake':%d,'cvt_guard':%d,'button':%d,'seat':%d,'hitch':%d,'ignition':%d,'rfid':%d,'cart_mode':%d,'cart_fwd':%d,'cart_bwd':%d,'throttle':%d,'trigger':%d,'pull_mode':%d}", RUN_MODE, DISPLAY_MODE, RIGHT_BRAKE, LEFT_BRAKE, CVT_GUARD, BUTTON_KILL, SEAT_KILL, HITCH_KILL, IGNITION, RFID_AUTH, CART_MODE, CART_FORWARD, CART_BACKWARD, THROTTLE, TRIGGER_KILL, PULL_MODE);
  sprintf(OUTPUT_BUFFER, "{'uid':'%s','data':%s,'chksum':%d,'task':'%s'}", UID, DATA_BUFFER, checksum(), PUSH);
  Serial.println(OUTPUT_BUFFER);
  Serial.flush();
}

/* --- SYNCHRONOUS TASKS --- */
/// Set Throttle
int set_throttle(int val) {
  THROTTLE_SET = val;
  THROTTLE_IN = map(analogRead(THROTTLE_POS_PIN), THROTTLE_POS_MIN, THROTTLE_POS_MAX, 1024, 0); // get the position feedback from the linear actuator
  int error = THROTTLE_IN - THROTTLE_SET;
  THROTTLE_OUT = -1 * THROTTLE_P * map(error, -1024, 1024, -400, 400);
  
  // Engage throttle actuator
  if (ESC.getM1CurrentMilliamps() >  THROTTLE_MILLIAMP_THRESHOLD) {
    ESC.setM2Speed(0); // disable if over-amp
  }
  else {
    ESC.setM2Speed(THROTTLE_OUT);
  }
}

/// Check Brake
// Engage the brakes linearly
int check_brake(int pin) {
  int val = analogRead(pin); // read left brake signal
  if (val > BRAKES_THRESHOLD) {
    return map(val, BRAKES_MIN, BRAKES_MAX, 0, 400);
  }
  else {
    return 0;
  }
}

/// Set brakes
// Returns true if the brake interlock is engaged
int set_brakes(int left_brake, int right_brake) {
  
  // Map the brake values for DualVNH5019 output
  int val = (left_brake + right_brake) / 2;
  int output = map(val, BRAKES_MIN, BRAKES_MAX, 0, 400); // linearly map the brakes power output to 0-400

  // Engage brakes
  if (ESC.getM2CurrentMilliamps() > BRAKES_MILLIAMP_THRESH) {
    ESC.setM1Speed(0); // disable breaks if over-amp
  }
  else {
    ESC.setM1Speed(output);
  }
}

/// Check Switch
int check_switch(int pin_num) {
  if  (digitalRead(pin_num)) {
    if (digitalRead(pin_num)) {
      return 1;
    }
    else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

/// Check Switch
int check_failsafe_switch(int pin_num) {
  if  (digitalRead(pin_num)) {
    if (digitalRead(pin_num)) {
      return 0;
    }
    else {
      return 1;
    }
  }
  else {
    return 1;
  }
}

/// Check RFID
int check_rfid(void) {
  Serial3.write(RFID_READ);
  delay(2);
  int a = Serial3.read();
  int b = Serial3.read();
  int c = Serial3.read();
  int d = Serial3.read();
  if ((a > 0) && (b > 0) && (c > 0) && (d > 0)) {
    return a + b + c + d; // the user's key number
  }
  else {
    return 0;
  }
}

/// Checksum
int checksum() {
  int sum = 0;
  for (int i = 0; i < DATA_SIZE; i++) {
    sum += DATA_BUFFER[i];
  }
  int val = sum % 256;
  return val;
}

/// Check Guard
// Returns true if guard is open
int check_guard(void) {
  int val = analogRead(CVT_GUARD_PIN);
  if (val <= CVT_GUARD_THRESHOLD) {
      return 1;
    }
    else {
      return 0;
    }
}

/// Check Seat
// Returns 1 if seat is empty, checks for SEAT_LIMIT iterations before activating
int check_seat(void) {
  if (digitalRead(SEAT_KILL_PIN)) {
    SEAT_COUNTER++;
  } 
  else {
    SEAT_COUNTER = 0;
  }
  if (SEAT_COUNTER >= SEAT_LIMIT) {
    return 1;
  }
  else {
    return 0;
  }
}

/// Reboot the Atom
void reboot(void) {
  digitalWrite(REBOOT_RELAY_PIN, LOW);
  delay(REBOOT_WAIT);
  digitalWrite(REBOOT_RELAY_PIN, HIGH);
}

/// Kill
void kill(void) {
  delay(KILL_WAIT);
  digitalWrite(STOP_RELAY_PIN, HIGH);
  //digitalWrite(REGULATOR_RELAY_PIN, HIGH);
  //digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(KILL_WAIT);
  RUN_MODE = 0;
}

/// Standby
void standby(void) {
  digitalWrite(STOP_RELAY_PIN, LOW);
  digitalWrite(REGULATOR_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(STANDBY_WAIT);
  RUN_MODE = 1;
}

/// Ignition
void ignition(void) {
  ESC.setM1Speed(0);
  ESC.setM2Speed(0);
  delay(MOTORS_WAIT);
  while (check_switch(IGNITION_PIN)) {
    digitalWrite(STOP_RELAY_PIN, LOW);
    digitalWrite(REGULATOR_RELAY_PIN, LOW);
    digitalWrite(STARTER_RELAY_PIN, LOW);
    delay(IGNITION_WAIT);
  }
  digitalWrite(STOP_RELAY_PIN, LOW);
  digitalWrite(REGULATOR_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(STANDBY_WAIT);
  RUN_MODE = 2;
}

/// Get Fuel Rate
float get_fuel_lph(void) {
  LPH_TIME_A = millis();
  float lph = (float(LPH_COUNTER) * 0.00038 * 3600.0) / (float(LPH_TIME_B - LPH_TIME_A) / 1000.0);
  LPH_HIST.add(lph);
  LPH_COUNTER = 0;
  LPH_TIME_B = millis();
  return LPH_HIST.getAverage();
}

/// Get Engine Temperature
float get_engine_temp(void) {
  float tmp = TEMP_SENSOR.getTempCByIndex(0);
  TEMP_SENSOR.requestTemperatures();
  if (isnan(tmp)) {
    return TEMP_HIST.getAverage();
  }
  else {
    TEMP_HIST.add(tmp);
  }
  return TEMP_HIST.getAverage();
}

/// Get the Engine Oil Pressure
float get_oil_psi(void) {
  int ohms = analogRead(PSI_SENSOR_PIN);
  float psi = 0.0226 * ohms * ohms - 4.3316 * ohms + 240;
  PSI_HIST.add(psi);
  return PSI_HIST.getAverage();
}

/* --- ASYNCHRONOUS TASKS --- */
/// Increment the LPH counter
void lph_counter(void) {
  LPH_COUNTER++;
}
