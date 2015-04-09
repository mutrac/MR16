/*
  Vehicle Dynamics Controller (VDC)
  McGill ASABE Tractor Team
  Revision 2016
*/

/* --- LIBRARIES --- */
#include "PID_v1.h"
#include <DualVNH5019MotorShield.h>

/* --- GLOBAL CONSTANTS --- */
// USB/CanBUS Info
const char UID[] = "VDC";
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;

// Analog Inputs
const int STEERING_POSITION_PIN = A0;
const int ACTUATOR_POSITION_PIN = A1;
const int SUSPENSION_POSITION_PIN = A2;

// Digital Inputs
const int CART_FORWARD_PIN = 2;
const int CART_BACKWARD_PIN = 3;
const int CART_MODE_PIN = 4;

/* --- GLOBAL VARIABLES --- */
// Create sensor values
int LOAD_BALANCE_MODE = false;
int STR_POS = 0;
int ACT_POS = 0;
int SUSP_POS = 0;
int CART_POS = 0;
boolean CART_FORWARD = false;
boolean CART_BACKWARD = false;

// Create character buffers
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

// Create PID controller objects
double STEERING_SET, STEERING_IN, STEERING_OUT;
double BALLAST_SET, BALLAST_IN, BALLAST_OUT;
PID STEERING_PID(&STEERING_IN, &STEERING_OUT, &STEERING_SET, 2, 5, 1, DIRECT);
PID BALLAST_PID(&BALLAST_IN, &BALLAST_OUT, &BALLAST_SET, 2, 5, 1, DIRECT);

DualVNH5019MotorShield VDC; // M1 is Steering, M2 is Ballast

/* --- SETUP --- */
void setup() {
  Serial.begin(BAUD);
  
  // Initialize Inputs
  pinMode(CART_FORWARD_PIN, INPUT);
  pinMode(CART_BACKWARD_PIN, INPUT);
  pinMode(STEERING_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
  pinMode(SUSPENSION_POSITION_PIN, INPUT);
  
  // TODO: Need a calibrated initial ballast/steering setpoints
  BALLAST_SET = 0; 
  STEERING_SET = 512;
  STEERING_PID.SetMode(AUTOMATIC);
  BALLAST_PID.SetMode(AUTOMATIC);
}

/* --- LOOP --- */
void loop() {
    
  /* --- START Steering Subsystem --- */
  // Set Steering
  STR_POS = analogRead(STEERING_POSITION_PIN);
  ACT_POS = analogRead(ACTUATOR_POSITION_PIN);
  STEERING_IN = double(ACT_POS);
  STEERING_SET = double(STR_POS);
  STEERING_PID.Compute();
  
  // Set steering actuator power output
  if (VDC.getM1CurrentMilliamps()) {
    VDC.setM1Speed(int(STEERING_OUT));
  }
  else {
    VDC.setM1Speed(0);
  }
  /* --- END Steering Subsystem --- */
  
  /* --- START Ballast Subsystm --- */
  // Get Ballast system parameters and recompute PID
  SUSP_POS = analogRead(SUSPENSION_POSITION_PIN);
  CART_FORWARD = digitalRead(CART_FORWARD_PIN);
  CART_BACKWARD = digitalRead(CART_BACKWARD_PIN);
  BALLAST_IN = double(SUSP_POS);
  BALLAST_PID.Compute();
 
  // Set cart mode
  if (digitalRead(CART_MODE_PIN)) {
    if (LOAD_BALANCE_MODE) {
      LOAD_BALANCE_MODE = false;
    }
    else {
      LOAD_BALANCE_MODE = true;
    }
  }
  
  // Set ballast motor power
  if (LOAD_BALANCE_MODE) {
    int val = int(BALLAST_OUT);
    VDC.setM2Speed(val);
  }
  else {
    if (CART_FORWARD && !CART_BACKWARD) {
      VDC.setM1Speed(400);
    }
    else if (CART_BACKWARD && !CART_FORWARD) {
      VDC.setM1Speed(-400);
    }
    else {
      VDC.setM2Speed(0);
    }
  }
  /* --- END Ballast Subsystem--- */

  // Format data buffer
  sprintf(DATA_BUFFER, "{'str':%d,'act':%d,'cart':%d,'susp':%d}", STR_POS, ACT_POS, CART_POS, SUSP_POS);
  
  // Format output to USB host by the following structure: {uid, data, chksum}
  sprintf(OUTPUT_BUFFER, "{'uid':%s,'data':%s,'chksum':%d}", UID, DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
 
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
