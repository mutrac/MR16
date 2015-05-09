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
const int OUTPUT_SIZE = 512;
const int DATA_SIZE = 256;
const int RFID_READ = 0x02;

// Failsafe Digital Input
const int BUTTON_KILL_PIN = 22;
const int HITCH_KILL_PIN = 24;
const int SEAT_KILL_PIN = 26;
const int SEAT_KILL_PWR = 28;
const int HITCH_KILL_PWR = 30;
const int BUTTON_KILL_PWR = 32;

// Joystick (Digital) 
// Block 1 (pins 23 - 29)
const int IGNITION_PIN = 23;
const int TRIGGER_KILL_PIN = 25;
const int PULL_PIN = 27;

// Block 2 (pins 31 - 37)
const int CART_FORWARD_PIN = 31;
const int CART_BACKWARD_PIN = 33;
const int CART_MODE_PIN = 35;

// Block 3 (pins 39 - 45)
const int THROTTLE_UP_PIN = 39; // These are set to an interrupt
const int THROTTLE_DOWN_PIN = 41; // These are set to an interrupt
const int RPM_UP_PIN = 43; // These are set to an interrupt

// Block 4 (pins 47 - 53)
const int RPM_DOWN_PIN = 47;
const int DISPLAY_MODE_PIN = 49;

// Relay Pins
const int STOP_RELAY_PIN = 38;
const int REGULATOR_RELAY_PIN = 39;
const int STARTER_RELAY_PIN = 40;
const int ATOM_RELAY_PIN = 41;

// Analog Input Pins
// A0 Reserved for DualVNH5019
// A1 Reserved for DualVNH5019
const int LEFT_BRAKE_PIN = A14;
const int RIGHT_BRAKE_PIN = A15;
const int CVT_GUARD_PIN = A4;
const int THROTTLE_POS_PIN = A8;
const int THROTTLE_MIN_PIN = A9;
const int THROTTLE_MAX_PIN = A10;

// Time Delays
const int KILL_WAIT = 10;
const int IGNITION_WAIT = 1000;
const int STANDBY_WAIT = 10;

// Brake variables
int BRAKES_THRESHOLD = 512;
int BRAKES_MILLIAMP_THRESH = 15000;
int BRAKES_MIN = 0;
int BRAKES_MAX = 1024;

// Throttle
int THROTTLE_MIN = 0;
int THROTTLE_MAX = 1024;
int THROTTLE_MILLIAMP_THRESH = 15000;
int THROTTLE_P = 5;
int THROTTLE_I = 1;
int THROTTLE_D = 2;
int THROTTLE_STEP = 64;

// RPM
int RPM_MIN = 1550;
int RPM_MAX = 3600;
int RPM_STEP = 100;

// CVT Guard
int CVT_GUARD_THRESH = 100;

/* --- GLOBAL VARIABLES --- */
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
int CART_FORWARD = 0;
int CART_BACKWARD = 0;
int CART_MODE = 0;
int THROTTLE_UP = 0;
int THROTTLE_DOWN = 0;
int RPM_UP = 0;
int RPM_DOWN = 0;
int RPM = RPM_MIN;
int THROTTLE = THROTTLE_MIN;
volatile int THROTTLE_SPEED = 0;

// String output
char DATA_BUFFER[DATA_SIZE];
char OUTPUT_BUFFER[OUTPUT_SIZE];

// Throttle PID Controller
double throttle_set, throttle_in, throttle_out;
PID throttle_pid(&throttle_in, &throttle_out, &throttle_set, 2, 5, 1, DIRECT);

// Brake Motor Controller
DualVNH5019MotorShield ESC;

/* --- SETUP --- */
void setup() {

  // Serial
  Serial.begin(BAUD);
  Serial3.begin(RFID_BAUD); // Pins 14 and 15
  Serial3.write(RFID_READ);

  // Failsafe Digital Input
  pinMode(SEAT_KILL_PIN, INPUT);
  digitalWrite(SEAT_KILL_PIN, HIGH);
  pinMode(SEAT_KILL_PWR, OUTPUT);
  digitalWrite(SEAT_KILL_PWR, LOW);
  
  pinMode(HITCH_KILL_PIN, INPUT);
  digitalWrite(HITCH_KILL_PIN, HIGH);
  pinMode(HITCH_KILL_PWR, OUTPUT);
  digitalWrite(HITCH_KILL_PWR, LOW);
  
  pinMode(BUTTON_KILL_PIN, INPUT);
  digitalWrite(BUTTON_KILL_PIN, HIGH);
  pinMode(BUTTON_KILL_PWR, OUTPUT);
  digitalWrite(BUTTON_KILL_PWR, LOW);

  // Joystick Digital Input
  pinMode(IGNITION_PIN, INPUT);
  pinMode(TRIGGER_KILL_PIN, INPUT);
  pinMode(PULL_PIN, INPUT);
  pinMode(CART_FORWARD_PIN, INPUT);
  pinMode(CART_BACKWARD_PIN, INPUT);
  pinMode(CART_MODE_PIN, INPUT);
  pinMode(THROTTLE_UP_PIN, INPUT);
  pinMode(THROTTLE_DOWN_PIN, INPUT);
  pinMode(RPM_UP_PIN, INPUT);
  pinMode(RPM_DOWN_PIN, INPUT);
  pinMode(DISPLAY_MODE_PIN, INPUT);
  
  // Throttle
  pinMode(THROTTLE_POS_PIN, INPUT);
  pinMode(THROTTLE_MIN_PIN, OUTPUT);
  digitalWrite(THROTTLE_MIN_PIN, LOW);
  pinMode(THROTTLE_MAX_PIN, OUTPUT);
  digitalWrite(THROTTLE_MAX_PIN, HIGH);

  // Relays
  pinMode(STOP_RELAY_PIN, OUTPUT);
  pinMode(REGULATOR_RELAY_PIN, OUTPUT);
  pinMode(STARTER_RELAY_PIN, OUTPUT);
  pinMode(ATOM_RELAY_PIN, OUTPUT);

  // Analog Input
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
  pinMode(CVT_GUARD_PIN, INPUT);

  // Brake Motors
  ESC.init();

  // PID
  // throttle_pid.SetMode(AUTOMATIC);
}

/* --- LOOP --- */
void loop() {

  // Check switches
  SEAT_KILL = check_switch(SEAT_KILL_PIN);
  HITCH_KILL = check_switch(HITCH_KILL_PIN);
  BUTTON_KILL = check_switch(BUTTON_KILL_PIN);
  TRIGGER_KILL = check_switch(TRIGGER_KILL_PIN);
  IGNITION = check_switch(IGNITION_PIN);
  DISPLAY_MODE = check_switch(DISPLAY_MODE_PIN);
  CART_FORWARD = check_switch(CART_FORWARD_PIN);
  CART_BACKWARD = check_switch(CART_BACKWARD_PIN);
  CART_MODE = check_switch(CART_MODE_PIN);
  RPM_UP = check_switch(RPM_UP_PIN);
  RPM_DOWN = check_switch(RPM_DOWN_PIN);
  THROTTLE_UP = check_switch(THROTTLE_UP_PIN);
  THROTTLE_DOWN = check_switch(THROTTLE_DOWN_PIN);

  // Check non-switches
  CVT_GUARD = check_guard();
  RFID_AUTH = check_rfid();
  LEFT_BRAKE = check_brake(LEFT_BRAKE_PIN);
  RIGHT_BRAKE = check_brake(RIGHT_BRAKE_PIN);

  // Set Brakes Always
  set_brakes(RIGHT_BRAKE, LEFT_BRAKE);

  // Adjust RPM limit
  if (RPM_UP && !RPM_DOWN) {
      RPM = RPM + RPM_STEP;
  }
  else if (RPM_DOWN && !RPM_UP) {
    RPM = RPM - RPM_STEP;
  }
  else {
    if (RPM >= RPM_MAX) {
      RPM = 3600;
    }
    else if (RPM <= RPM_MIN) {
      RPM = 1550;
    }
  }
  
  // Adjust throttle limit
  if (THROTTLE_UP && !THROTTLE_DOWN) {
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

  // If OFF
  if (RUN_MODE == 0) {
    if (RFID_AUTH) {
      standby();
    }
  }
  // If STANDBY
  else if (RUN_MODE == 1) {
    if (SEAT_KILL || HITCH_KILL || BUTTON_KILL) {
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
      if (TRIGGER_KILL) {
        set_throttle(0); // DISABLE THROTTLE IF OPERATOR RELEASES TRIGGER
      }
      else {
        set_throttle(THROTTLE);
      }
    }
  }
  // If in State ? (UNKNOWN)
  else {
    kill();
  }

  // USB
  sprintf(DATA_BUFFER, "{'run_mode':%d,'display_mode':%d,'right_brake':%d,'left_brake':%d,'cvt_guard':%d,'seat':%d,'hitch':%d,'ignition':%d,'rfid':'%d','cart_mode':%d,'cart_fwd':%d,'cart_bwd':%d','rpm'%d,'throttle':%d}", RUN_MODE, DISPLAY_MODE, RIGHT_BRAKE, LEFT_BRAKE, CVT_GUARD, SEAT_KILL, HITCH_KILL, IGNITION, RFID_AUTH, CART_MODE, CART_FORWARD, CART_BACKWARD, RPM, THROTTLE);
  sprintf(OUTPUT_BUFFER, "{'uid':'%s','data':%s,'chksum':%d,'task':'%s'}", UID, DATA_BUFFER, checksum(), PUSH);
  Serial.println(OUTPUT_BUFFER);
  Serial.flush();
}

/* --- SYNCHRONOUS TASKS --- */

// Set Throttle
int set_throttle(int val) {
  throttle_set = val;
  throttle_in = analogRead(THROTTLE_POS_PIN); // get the position feedback from the linear actuator
  throttle_pid.Compute(); // this ghost-overwrites the 'throttle_out' variable
  
  // Engage throttle actuator
  if (ESC.getM1CurrentMilliamps() >  THROTTLE_MILLIAMP_THRESH) {
    ESC.setM1Speed(0); // disable if over-amp
  }
  else {
    int output = map(throttle_out, 0, 255, -400, 400);
    ESC.setM1Speed(output);
  }
}

// Check Brake
int check_brake(int pin) {
  int val = analogRead(pin); // read left brake signal
  if (val > BRAKES_THRESHOLD) {
    return map(val, BRAKES_MIN, BRAKES_MAX, 0, 400);
  }
  else {
    return 0;
  }
}

// Set brakes
// Returns true if the brake interlock is engaged
int set_brakes(int left_brake, int right_brake) {
  
  int val = (left_brake + right_brake) / 2;
  int output = map(val, BRAKES_MIN, BRAKES_MAX, 0, 400); // linearly map the brakes power output to 0-400

  // Engage brake motor
  if (ESC.getM2CurrentMilliamps() > BRAKES_MILLIAMP_THRESH) {
    ESC.setM2Speed(0); // disable breaks if over-amp
  }
  else {
    ESC.setM2Speed(output);
  }

  // Check interlock
  if  (val >= BRAKES_THRESHOLD) {
    return 1;
  }
  else {
    return 0;
  }
}

// Check Display Mode
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

// Check RFID
int check_rfid(void) {
  Serial3.write(RFID_READ);
  delay(2);
  int a = Serial3.read();
  int b = Serial3.read();
  int c = Serial3.read();
  int d = Serial3.read();
  if ((a > 0) && (b > 0) && (c > 0) && (d > 0)) {
    return a + b + c + d;
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

// Kill
void kill(void) {
  Serial.println("kill");
  digitalWrite(STOP_RELAY_PIN, HIGH);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  digitalWrite(REGULATOR_RELAY_PIN, HIGH);
  delay(KILL_WAIT);
  RUN_MODE = 0;
}

// Standby
void standby(void) {
  Serial.println("standby");
  digitalWrite(STOP_RELAY_PIN, LOW);
  digitalWrite(REGULATOR_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  delay(STANDBY_WAIT);
  RUN_MODE = 1;
}

// Ignition
void ignition(void) {
  Serial.println("ignition");
  ESC.setM1Speed(0);
  ESC.setM2Speed(0);
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

/* --- ASYNCHRONOUS TASKS --- */

