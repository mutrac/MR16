/*
  Engine Safety Controller (ESC)
  McGill ASABE Tractor Team
*/

/* --- LIBRARIES --- */
#include "DualVNH5019MotorShield.h"
#include "DallasTemperature.h"
#include "OneWire.h"

/* --- GLOBAL CONSTANTS --- */
// CAN
const char ID[] = "ESC";
const int BAUD = 9600;
const int OUTPUT_SIZE = 256;
const int DATA_SIZE = 128;

// Failsafe Digital Input
const int SEAT_KILL_PIN = 21;
const int SEAT_KILL_PWR = 22;
const int HITCH_KILL_PIN = 23;
const int HITCH_KILL_PWR = 24;
const int BUTTON_KILL_PIN = 25;
const int BUTTON_KILL_PWR = 26;

// Joystick (Digital) 
// Note: in this revision, the joystick gives input to multiple controllers
const int TRIGGER_KILL_PIN = 27;
const int PULL_PIN = 31;
const int DBS_PIN = 32;
const int IGNITION_PIN = 33;
const int CYCLE_DISPLAY_PIN = 34;

// Interrupts
const int THROTTLE_UP_INT = 14; // These are set to an interrupt
const int THROTTLE_DOWN_INT = 15; // These are set to an interrupt

// Relays
const int GROUND_RELAY_PIN = 2;
const int REGULATOR_RELAY_PIN = 3;
const int STARTER_RELAY_PIN = 4;
const int ATOM_RELAY_PIN = 5;

// Analog Input
// A0 Reserved for DualVNH5019
// A1 Reserved for DualVNH5019
const int LEFT_BRAKE_PIN = A2;
const int RIGHT_BRAKE_PIN = A3;
const int CVT_GUARD_PIN = A4;

/* --- GLOBAL VARIABLES --- */
// Brakes
int BRAKES_THRESH = 512;
int BRAKES_P = 1;
int BRAKES_I = 0;
int BRAKES_D = 0;

// Throttle
int THROTTLE_MIN = 0;
int THROTTLE_MAX = 1024;
int THROTTLE_P = 1;
int THROTTLE_I = 0;
int THROTTLE_D = 0;

// CVT Guard
int CVT_GUARD_THRESH = 100;

// Peripheral values
int SEAT_KILL = 0;
int HITCH_KILL = 0;
int IGNITION = 0;
int CVT_GUARD = 0;
int GND_RELAY = 0;
int REG_RELAY = 0;
int START_RELAY = 0;
int LEFT_BRAKE = 0;
int RIGHT_BRAKE = 0;
int RFID_AUTH = 0;
volatile int THROTTLE_SPEED = 0;

// String output
char DATA_BUFFER[DATA_SIZE];
char OUTPUT_BUFFER[OUTPUT_SIZE];

/* --- SETUP --- */
void setup() {
  
  Serial.begin(BAUD);
  
  // Failsafe Digital Input
  pinMode(SEAT_KILL_PIN, INPUT);
  pinMode(SEAT_KILL_PWR, OUTPUT);
  pinMode(HITCH_KILL_PIN, INPUT);
  pinMode(HITCH_KILL_PWR, OUTPUT);
  pinMode(BUTTON_KILL_PIN, INPUT);
  pinMode(BUTTON_KILL_PWR, OUTPUT);
  
  // Joystick Digital Input
  pinMode(TRIGGER_KILL_PIN, INPUT);
  pinMode(IGNITION_PIN, INPUT);

  // Relays
  pinMode(GROUND_RELAY_PIN, OUTPUT);
  pinMode(REGULATOR_RELAY_PIN, OUTPUT);
  pinMode(STARTER_RELAY_PIN, OUTPUT);
  pinMode(ATOM_RELAY_PIN, OUTPUT);
  
  // Analog Input
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
  pinMode(CVT_GUARD_PIN, INPUT);
  
  // Interrupts
  attachInterrupt(THROTTLE_UP_INT, count_throttle_up, RISING);
  attachInterrupt(THROTTLE_DOWN_INT, count_throttle_down, RISING);
}

/* --- LOOP --- */
void loop() {
  
  // Digital Input
  SEAT_KILL = digitalRead(SEAT_KILL_PIN);
  HITCH_KILL = digitalRead(HITCH_KILL_PIN);
  IGNITION = check_ignition();
  
  // Analog Input
  CVT_GUARD = check_guard();
  RIGHT_BRAKE = check_left_brake();
  LEFT_BRAKE = check_right_brake();
  
  // USB
  sprintf(DATA_BUFFER, "{gnd_relay:%d,reg_relay:%d,starter_relay:%d,right_brake:%d,left_brake:%d,cvt_guard:%d,seat:%d,hitch:%d,ignition:%d}", GND_RELAY, REG_RELAY, START_RELAY, RIGHT_BRAKE, LEFT_BRAKE, CVT_GUARD, SEAT_KILL, HITCH_KILL, IGNITION);
  sprintf(OUTPUT_BUFFER, "{'id':%s,'data':%s,'chksum':%d}", ID, DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
}

/* --- SYNCHRONOUS TASKS --- */
// Checksum
int checksum() {
  int sum = 0;
  for (int i = 0; i < DATA_SIZE; i++) {
    sum += DATA_BUFFER[i];
  }
  int val = sum % 256;
  return val;
}

// Check Right Brake
int check_right_brake(void) {
  if  (analogRead(RIGHT_BRAKE_PIN) >= BRAKES_THRESH) {
    if (analogRead(RIGHT_BRAKE_PIN) >= BRAKES_THRESH) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

// Check Left Brake
boolean check_left_brake(void) {
  if  (analogRead(LEFT_BRAKE_PIN) >= BRAKES_THRESH) {
    if (analogRead(LEFT_BRAKE_PIN) >= BRAKES_THRESH) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

// Check Ignition
boolean check_ignition(void) {
  if  (digitalRead(IGNITION_PIN)) {
    if (digitalRead(IGNITION_PIN)) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

// Check Guard --> Returns true if guard open
boolean check_guard(void) {
  if (analogRead(CVT_GUARD_PIN) >= CVT_GUARD_THRESH) {
    if (analogRead(CVT_GUARD_PIN) >= CVT_GUARD_THRESH) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

// Check Seat() --> Returns true if seat kill engaged
boolean check_seat(void) {
  if (digitalRead(SEAT_KILL_PIN)) {
    if (digitalRead(SEAT_KILL_PIN)) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

// Check Hitch
boolean check_hitch(void) {
  if (digitalRead(HITCH_KILL_PIN)) {
    if (digitalRead(HITCH_KILL_PIN)) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

// Check Button
boolean check_button(void) {
  if (digitalRead(BUTTON_KILL_PIN)) {
    if (digitalRead(BUTTON_KILL_PIN)) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

/* --- ASYNCHRONOUS TASKS --- */
// Check Throttle Up
void count_throttle_up(void) {
  THROTTLE_SPEED++;
  if (THROTTLE_SPEED > THROTTLE_MIN) {
    THROTTLE_SPEED = THROTTLE_MIN;
  }
}

// Check Throttle Down
void count_throttle_down(void) {
  THROTTLE_SPEED--;
  if (THROTTLE_SPEED > THROTTLE_MAX) {
    THROTTLE_SPEED = THROTTLE_MAX;
  }
}
