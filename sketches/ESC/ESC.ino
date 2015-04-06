/*
  Engine Safety Controller (ESC)
  McGill ASABE Tractor Team
*/

/* --- LIBRARIES --- */
#include "DualVNH5019MotorShield.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include <PID_v1.h>
#define RFID_READ 0x02

/* --- GLOBAL CONSTANTS --- */
// CAN
const char UID[] = "ESC";
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
// Note 1: in this revision, the joystick gives input to multiple controllers
const int TRIGGER_KILL_PIN = 30;
const int PULL_PIN = 31;
const int DBS_PIN = 32;
const int IGNITION_PIN = 33;
const int CYCLE_DISPLAY_PIN = 34;

// Interrupts
const int THROTTLE_UP_INT = 14; // These are set to an interrupt
const int THROTTLE_DOWN_INT = 15; // These are set to an interrupt

// Relays
const int STOP_RELAY_PIN = 35;
const int REGULATOR_RELAY_PIN = 36;
const int STARTER_RELAY_PIN = 37;
const int ATOM_RELAY_PIN = 38;

// Analog Input
// A0 Reserved for DualVNH5019
// A1 Reserved for DualVNH5019
const int LEFT_BRAKE_PIN = A2;
const int RIGHT_BRAKE_PIN = A3;
const int CVT_GUARD_PIN = A4;

// Analog Output (PWM)
const int THROTTLE_PWM_PIN = 2;

// Time Delays
const int KILL_WAIT = 1000;
const int IGNITION_WAIT = 1000;
const int STANDBY_WAIT = 1000;

/* --- GLOBAL VARIABLES --- */
// Brakes
int BRAKES_VTHRESH = 512;
int BRAKES_MILLIAMP_THRESH = 15000;
int BRAKES_VMIN = 0;
int BRAKES_VMAX = 1024;

// Throttle
int THROTTLE_MIN = 0;
int THROTTLE_MAX = 1024;
int THROTTLE_P = 5;
int THROTTLE_I = 1;
int THROTTLE_D = 2;

// CVT Guard
int CVT_GUARD_THRESH = 100;

// Peripheral values
int SEAT_KILL = 0;
int HITCH_KILL = 0;
int IGNITION = 0;
int CVT_GUARD = 0;
int STOP_RELAY = 0;
int REG_RELAY = 0;
int START_RELAY = 0;
int LEFT_BRAKE = 0;
int RIGHT_BRAKE = 0;
int RFID_AUTH = 0;
volatile int THROTTLE_SPEED = 0;

// String output
char DATA_BUFFER[DATA_SIZE];
char OUTPUT_BUFFER[OUTPUT_SIZE];
 
// Throttle PID Controller
double throttle_set, throttle_in, throttle_out;
PID throttle_pid(&throttle_in, &throttle_out, &throttle_set, 2, 5, 1, DIRECT);

// Brake Motor Controller
DualVNH5019MotorShield brakes;

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
  pinMode(STOP_RELAY_PIN, OUTPUT);
  pinMode(REGULATOR_RELAY_PIN, OUTPUT);
  pinMode(STARTER_RELAY_PIN, OUTPUT);
  pinMode(ATOM_RELAY_PIN, OUTPUT);
  
  // Analog Input
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
  pinMode(CVT_GUARD_PIN, INPUT);
  
  // Analog Output
  pinMode(THROTTLE_PWM_PIN, OUTPUT);
  
  // Interrupts
  attachInterrupt(THROTTLE_UP_INT, count_throttle_up, RISING);
  attachInterrupt(THROTTLE_DOWN_INT, count_throttle_down, RISING);
  
  // Brake Motors
  brakes.init();
}

/* --- LOOP --- */
void loop() {
  
  // Digital Input
  SEAT_KILL = check_seat();
  HITCH_KILL = check_hitch();
  IGNITION = check_ignition();
  
  // Analog Input
  CVT_GUARD = check_guard();
  RIGHT_BRAKE = check_left_brake();
  LEFT_BRAKE = check_right_brake();
  
  // Set Throttle
  set_throttle();
  set_left_brake();
  set_right_brake();
  
  // USB
  sprintf(DATA_BUFFER, "{stop_relay:%d,reg_relay:%d,starter_relay:%d,right_brake:%d,left_brake:%d,cvt_guard:%d,seat:%d,hitch:%d,ignition:%d}", STOP_RELAY, REG_RELAY, START_RELAY, RIGHT_BRAKE, LEFT_BRAKE, CVT_GUARD, SEAT_KILL, HITCH_KILL, IGNITION);
  sprintf(OUTPUT_BUFFER, "{'uid':%s,'data':%s,'chksum':%d}", UID, DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
}

/* --- SYNCHRONOUS TASKS --- */
// Set Throttle
void set_throttle(void) {
  analogWrite(THROTTLE_PWM_PIN, THROTTLE_SPEED);
}

// Right Brake
void set_right_brake(void) {
  int val = analogRead(RIGHT_BRAKE_PIN); // read right brake signal
  if (brakes.getM2CurrentMilliamps() >  BRAKES_MILLIAMP_THRESH) {
    brakes.setM2Speed(0); // disable breaks if over-amp
  }
  else {
    int output = map(BRAKES_VMIN, BRAKES_VMAX, 0, 400, val); // map the brakes power output to 0-400
    brakes.setM2Speed(output);
  }
}

// Left brake
void set_left_brake(void) {
  int val = analogRead(LEFT_BRAKE_PIN); // read left brake signal
  if (brakes.getM2CurrentMilliamps() > BRAKES_MILLIAMP_THRESH) {
    brakes.setM2Speed(0); // disable breaks if over-amp
  }
  else {
    int output = map(BRAKES_VMIN, BRAKES_VMAX, 0, 400, val); // map the brakes power output to 0-400
    brakes.setM2Speed(output);
  }
}

// Check RFID
boolean check_rfid(void) {
  Serial3.write(RFID_READ);
  if (Serial3.read() >= 0) {
    return true;
  }
  else {
    return false;
  }
}

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
  if  (analogRead(RIGHT_BRAKE_PIN) >= BRAKES_VTHRESH) {
    if (analogRead(RIGHT_BRAKE_PIN) >= BRAKES_VTHRESH) {
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
  if  (analogRead(LEFT_BRAKE_PIN) >= BRAKES_VTHRESH) {
    if (analogRead(LEFT_BRAKE_PIN) >= BRAKES_VTHRESH) {
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

// Kill
void kill(void) {
  brakes.setM1Speed(0);
  brakes.setM2Speed(0);
  digitalWrite(STOP_RELAY_PIN, HIGH);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  digitalWrite(REGULATOR_RELAY_PIN, HIGH);
  delay(KILL_WAIT);
}

// Standby
void standby(void) {
  brakes.setM1Speed(0);
  brakes.setM2Speed(0);
  digitalWrite(STOP_RELAY_PIN, LOW);
  digitalWrite(REGULATOR_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(STANDBY_WAIT);
}

// Ignition
void ignition(void) {
  Serial.println('ignition');
  brakes.setM1Speed(0);
  brakes.setM2Speed(0);
  while (check_ignition()) {
    digitalWrite(STOP_RELAY_PIN, LOW);
    digitalWrite(REGULATOR_RELAY_PIN, LOW);
    digitalWrite(STARTER_RELAY_PIN, LOW);
    delay(IGNITION_WAIT);
  }
  digitalWrite(STOP_RELAY_PIN, LOW);
  digitalWrite(REGULATOR_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(STANDBY_WAIT);
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
