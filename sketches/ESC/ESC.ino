/*
  Engine Safety Controller (ESC)
  McGill ASABE Tractor Team
  
  Requirements: Arduino Mega
*/

/* --- LIBRARIES --- */
#include "DualVNH5019MotorShield.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include <PID_v1.h>

/* --- GLOBAL CONSTANTS --- */
// CAN
const char UID[] = "ESC";
const char PUSH[] = "push";
const char PULL[] = "pull";
const int BAUD = 9600;
const int RFID_BAUD = 9600;
const int OUTPUT_SIZE = 256;
const int DATA_SIZE = 128;
const int RFID_READ = 0x02;

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
const int DISPLAY_MODE_PIN = 34;

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
int TRIGGER_KILL = 0;
int BUTTON_KILL = 0;
int IGNITION = 0;
int CVT_GUARD = 0;
int RUN_MODE = 0;
int LEFT_BRAKE = 0;
int RIGHT_BRAKE = 0;
int RFID_AUTH = 0;
int DISPLAY_MODE = 0; // the desired display mode on the HUD
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
  
  // Serial
  Serial.begin(BAUD);
  Serial3.begin(RFID_BAUD); // Pins 14 and 15
  Serial3.write(RFID_READ);
  
  
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
  
  // Get Inputs Always
  SEAT_KILL = check_seat();
  HITCH_KILL = check_hitch();
  BUTTON_KILL = check_button();
  TRIGGER_KILL = check_trigger();
  IGNITION = check_ignition();
  CVT_GUARD = check_guard();
  RFID_AUTH = check_rfid();
  DISPLAY_MODE = check_display();
  
  // Set Brakes Always
  LEFT_BRAKE = set_left_brake();
  RIGHT_BRAKE = set_right_brake();
  
  // If OFF
  if (RUN_MODE == 0) {
    if (RFID_AUTH) {
      standby();
    }
  }
  // If STANDBY
  else if (RUN_MODE == 1) {
    if (SEAT_KILL || HITCH_KILL || BUTTON_KILL || TRIGGER_KILL) {
      kill(); // kill engine
    }
    else if (IGNITION && !LEFT_BRAKE && !RIGHT_BRAKE && !CVT_GUARD) {
      ignition(); // execute ignition sequence
    }
    else {
      standby(); // remain in standby (RUN_MODE 1)
    }
  }
  // If DRIVE
  else if (RUN_MODE == 2) {
    if (SEAT_KILL || HITCH_KILL || BUTTON_KILL || TRIGGER_KILL) {
      kill(); // kill engine
      standby();
    }
    else {
      set_throttle();
    }
  }
  // If in State ? (UNKNOWN)
  else {
    kill();
  }
  
  // USB
  sprintf(DATA_BUFFER, "{'run_mode':%d,'display_mode':%d,'right_brake':%d,'left_brake':%d,'cvt_guard':%d,'seat':%d,'hitch':%d,'ignition':%d,'rfid':'%d'}", RUN_MODE, DISPLAY_MODE, RIGHT_BRAKE, LEFT_BRAKE, CVT_GUARD, SEAT_KILL, HITCH_KILL, IGNITION, RFID_AUTH);
  sprintf(OUTPUT_BUFFER, "{'uid':'%s','data':%s,'chksum':%d,'task':'%s'}", UID, DATA_BUFFER, checksum(),PUSH);
  Serial.println(OUTPUT_BUFFER);
}

/* --- SYNCHRONOUS TASKS --- */
// Set Throttle
void set_throttle(void) {
  analogWrite(THROTTLE_PWM_PIN, THROTTLE_SPEED);
}

// Right Brake
// Returns true if the brake interlock is engaged
int set_right_brake(void) {
  int val = analogRead(RIGHT_BRAKE_PIN); // read right brake signal
  int output = map(BRAKES_VMIN, BRAKES_VMAX, 0, 400, val);  // map the brakes power output to 0-400
   
  // Engage brake motor
  if (brakes.getM2CurrentMilliamps() >  BRAKES_MILLIAMP_THRESH) {
    brakes.setM2Speed(0); // disable breaks if over-amp
  }
  else {
    brakes.setM2Speed(output);
  }
  
  // Check interlock
  if  (val >= BRAKES_VTHRESH) {
    return 1;
  }
  else {
    return 0;
  }
}

// Left brake
// Returns true if the brake interlock is engaged
int set_left_brake(void) {
  int val = analogRead(LEFT_BRAKE_PIN); // read left brake signal
  int output = map(BRAKES_VMIN, BRAKES_VMAX, 0, 400, val); // linearly map the brakes power output to 0-400
  
  // Engage brake motor
  if (brakes.getM2CurrentMilliamps() > BRAKES_MILLIAMP_THRESH) {
    brakes.setM2Speed(0); // disable breaks if over-amp
  }
  else {
    brakes.setM2Speed(output);
  }
  
  // Check interlock
  if  (val >= BRAKES_VTHRESH) {
    return 1;
  }
  else {
    return 0;
  }
}

// Check Display
int check_display(void) {
  if  (digitalRead(DISPLAY_MODE_PIN)) {
    if (digitalRead(DISPLAY_MODE_PIN)) {
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

// Check RFID
int check_rfid(void) {
  Serial3.write(RFID_READ);
  if (Serial3.read() >= 0) {
    return 1;
  }
  else {
    return 0;
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

// Check Ignition
int check_ignition(void) {
  if  (digitalRead(IGNITION_PIN)) {
    if (digitalRead(IGNITION_PIN)) {
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

// Check Guard --> Returns true if guard open
int check_guard(void) {
  if (analogRead(CVT_GUARD_PIN) >= CVT_GUARD_THRESH) {
    if (analogRead(CVT_GUARD_PIN) >= CVT_GUARD_THRESH) {
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

// Check Seat() --> Returns true if seat kill engaged
int check_seat(void) {
  if (digitalRead(SEAT_KILL_PIN)) {
    if (digitalRead(SEAT_KILL_PIN)) {
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

// Check Trigger
int check_trigger(void) {
  if (digitalRead(TRIGGER_KILL_PIN)) {
    if (digitalRead(TRIGGER_KILL_PIN)) {
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

// Check Hitch
int check_hitch(void) {
  if (digitalRead(HITCH_KILL_PIN)) {
    if (digitalRead(HITCH_KILL_PIN)) {
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

// Check Button
int check_button(void) {
  if (digitalRead(BUTTON_KILL_PIN)) {
    if (digitalRead(BUTTON_KILL_PIN)) {
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

// Kill
void kill(void) {
  brakes.setM1Speed(0);
  brakes.setM2Speed(0);
  digitalWrite(STOP_RELAY_PIN, HIGH);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  digitalWrite(REGULATOR_RELAY_PIN, HIGH);
  delay(KILL_WAIT);
  RUN_MODE = 0;
}

// Standby
void standby(void) {
  brakes.setM1Speed(0);
  brakes.setM2Speed(0);
  digitalWrite(STOP_RELAY_PIN, LOW);
  digitalWrite(REGULATOR_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(STANDBY_WAIT);
  RUN_MODE = 1;
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
  RUN_MODE = 2;
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
