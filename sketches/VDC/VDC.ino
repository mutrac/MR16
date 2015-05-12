/*
  Vehicle Dynamics Controller (VDC)
  McGill ASABE Tractor Team
  Revision 2016
  
  Network Info:
  The VDC reads sensors for the steering wheel and actuator position via potentiometers,
  and tracks the suspension position via the hall effect sensor mounted on the front axle. 
  
  However, information for DBS control is best handled via the CMQ (see CMQ Commands).
    
  Requires: Arduino Mega with Pololu DualVNH5019 Motor Shield
*/

/* --- LIBRARIES --- */
#include "PID_v1.h"
#include <DualVNH5019MotorShield.h>

/* --- GLOBAL CONSTANTS --- */
// CMQ Commands (single letter requires single quotes)
const char FWD_CMD = 'F'; // move cart forward
const char BWD_CMD = 'B'; // move cart backward
const char MOD_CMD = 'M'; // change cart mode

// USB/CanBUS Info
const char UID[] = "VDC";
const char PULL[] = "pull";
const char PUSH[] = "push";
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;

// Analog Inputs
const int STEERING_POSITION_PIN = A2;
const int ACTUATOR_POSITION_PIN = A3;
const int SUSPENSION_POSITION_PIN = A5;

// Digital Inputs

// Safety Limits
const int STEERING_MILLIAMP_LIMIT = 15000;
const int BALLAST_MILLIAMP_LIMIT = 15000;

// Calibration Parameters
const int SUSP_POS_MIN = 200;
const int SUSP_POS_MAX = 450;
const int ACT_POS_MIN = 0;
const int ACT_POS_MAX = 1000;
const int STR_POS_MIN = 0;
const int STR_POS_MAX = 1023;
const int STEERING_GAIN = 5;
const int STEERING_THRESHOLD = 75;

/* --- GLOBAL VARIABLES --- */
// Create sensor values
int CART_MODE = 0;
int STR_POS = 0;
int ACT_POS = 0;
int SUSP_POS = 0;
int CART_FORWARD = 0;
int CART_BACKWARD = 0;

// Create character buffers
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

// Create PID controller objects
double STEERING_SET, STEERING_IN, STEERING_OUT;
double STEERING_OUT_MIN = -400;
double STEERING_OUT_MAX = 400;
PID STEERING_PID(&STEERING_IN, &STEERING_OUT, &STEERING_SET, 5, 0.5, 0, DIRECT);

double BALLAST_SET, BALLAST_IN, BALLAST_OUT;
double BALLAST_OUT_MIN = -400;
double BALLAST_OUT_MAX = 400;
PID BALLAST_PID(&BALLAST_IN, &BALLAST_OUT, &BALLAST_SET, 2, 5, 1, DIRECT);

DualVNH5019MotorShield VDC; // M1 is Steering, M2 is Ballast

/* --- SETUP --- */
void setup() {
  Serial.begin(BAUD);
  
  // Initialize Inputs
  pinMode(STEERING_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
  pinMode(SUSPENSION_POSITION_PIN, INPUT);
  
  // TODO: Need a calibrated initial ballast/steering setpoints
  STEERING_SET = 512;
  STEERING_PID.SetMode(AUTOMATIC);
  STEERING_PID.SetOutputLimits(STEERING_OUT_MIN, STEERING_OUT_MAX);

  BALLAST_SET = 0; 
  BALLAST_PID.SetMode(AUTOMATIC);
  BALLAST_PID.SetOutputLimits(BALLAST_OUT_MIN, BALLAST_OUT_MAX);

}

/* --- LOOP --- */
void loop() {
    
  /* --- START Steering Subsystem --- */
  // Set Steering
  STR_POS = analogRead(STEERING_POSITION_PIN);
  ACT_POS = analogRead(ACTUATOR_POSITION_PIN);
  
  // M1 - Set steering actuator power output
  if (VDC.getM1CurrentMilliamps() < STEERING_MILLIAMP_LIMIT) {
    int act = map(ACT_POS, ACT_POS_MIN, ACT_POS_MAX, 0, 1023);
    int str = map(STR_POS, STR_POS_MIN, STR_POS_MAX, 1023, 0);
    int val = STEERING_GAIN * (act - str);
    if (abs(val) < STEERING_THRESHOLD) {
      VDC.setM1Speed(0);
    }
    else {
      VDC.setM1Speed(val);
    }
  }
  else {
    VDC.setM1Speed(0);
  }
  /* --- END Steering Subsystem --- */
  
  /* --- START Ballast Subsystm --- */
  // Get next serial cart control commands
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case FWD_CMD:
        CART_FORWARD = 1;
        break;
      case BWD_CMD:
        CART_BACKWARD = 1;
        break;
      case MOD_CMD:
        if (CART_MODE) {
          CART_MODE = 0;
        }
        else {
          CART_MODE = 1;
        }
        break;
      default:
        break;
    }
  }
  
  // M2 - Set ballast motor power
  SUSP_POS = analogRead(SUSPENSION_POSITION_PIN);
  if (!CART_MODE) {
    int val;
    if (SUSP_POS > SUSP_POS_MAX) {val=400;}
    else if (SUSP_POS < SUSP_POS_MIN) {val=-400;}
    else {val=0;}
    VDC.setM2Speed(val);
  }
  else {
    if (CART_FORWARD && !CART_BACKWARD) {
      VDC.setM2Speed(400);
    }
    else if (CART_BACKWARD && !CART_FORWARD) {
      VDC.setM2Speed(-400);
    }
    else {
      VDC.setM2Speed(0);
    }
  }
  /* --- END Ballast Subsystem--- */

  // Format data buffer
  sprintf(DATA_BUFFER, "{'str':%d,'act':%d,'cart_mode':%d,'cart_fwd':%d,'cart_bwd':%d,'susp':%d}", STR_POS, ACT_POS, CART_MODE, CART_FORWARD, CART_BACKWARD, SUSP_POS);
  
  // Format output to USB host by the following structure: {uid, data, chksum}
  sprintf(OUTPUT_BUFFER, "{'uid':'%s','data':%s,'chksum':%d,'task':'%s'}", UID, DATA_BUFFER, checksum(), PUSH);
  Serial.println(OUTPUT_BUFFER);
 
  // Reset values (VERY IMPORTANT or else cart would get stuck on/off)
  CART_FORWARD = 0;
  CART_BACKWARD = 0;
}

/* --- SYNCHRONOUS TASKS --- */
// Check sum
int checksum() {
  int sum = 0;
  for (int i = 0; i < DATA_SIZE; i++) {
    sum += DATA_BUFFER[i];
  }
  int val = sum % 256;
  return val;
}

