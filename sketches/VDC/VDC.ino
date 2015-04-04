/*
  Vehicle Dynamics Controller (VDC)
  McGill ASABE Tractor Team
  Revision 2016
*/

/* --- LIBRARIES --- */
#include "PID_v1.h"
#include <DualVNH5019MotorShield.h>

/* --- GLOBAL CONSTANTS --- */
const char UID[] = "VDC";
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;

// Analog Inputs
const int STEERING_POSITION_PIN = A0;
const int ACTUATOR_POSITION_PIN = A1;
const int SUSPENSION_POSITION_PIN = A2;
const int CART_POSITION_PIN = A3;

// Digital Inputs
const int CART_FORWARD_PIN = 2;
const int CART_BACKWARD_PIN = 3;

/* --- GLOBAL VARIABLES --- */
int LOAD_BALANCE_MODE = false;
int STR_POS = 0;
int ACT_POS = 0;
int SUSP_POS = 0;
int CART_POS = 0;

char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

double steering_set, steering_in, steering_out;
double ballast_set, ballast_in, ballast_out;

PID steering(&steering_in, &steering_out, &steering_set, 2, 5, 1, DIRECT);
PID ballast(&ballast_in, &ballast_out, &ballast_set, 2, 5, 1, DIRECT);

DualVNH5019MotorShield VDC; // M1 is Steering, M2 is Ballast

/* --- SETUP --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(STEERING_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
  pinMode(SUSPENSION_POSITION_PIN, INPUT);
  pinMode(CART_POSITION_PIN, INPUT);
  ballast_set= 100;
  steering_set = 100;
  steering.SetMode(AUTOMATIC);
  ballast.SetMode(AUTOMATIC);
}

/* --- LOOP --- */
void loop() {
    
  // Set Steering
  STR_POS = analogRead(STEERING_POSITION_PIN);
  ACT_POS = analogRead(ACTUATOR_POSITION_PIN);
  steering_in = double(ACT_POS);
  steering_set = double(STR_POS);
  steering.Compute();
  if (VDC.getM1CurrentMilliamps()) {
    VDC.setM1Speed(int(steering_out));
  }
  else {
    VDC.setM1Speed(0);
  }
  
  // Set Ballast
  CART_POS = analogRead(CART_POSITION_PIN);
  SUSP_POS = analogRead(SUSPENSION_POSITION_PIN);
  ballast.Compute();
  if (LOAD_BALANCE_MODE) {
    int val = int(ballast_out);
    VDC.setM2Speed(val);
  }
  else {
    if (digitalRead(CART_FORWARD_PIN) && !(digitalRead(CART_BACKWARD_PIN))) {
      VDC.setM2Speed(400);
    }
    else if (digitalRead(CART_BACKWARD_PIN) && !(digitalRead(CART_FORWARD_PIN))) {
      VDC.setM2Speed(-400);
    }
    else {
      VDC.setM2Speed(0);
    }
  }
  
  // Format data buffer
  sprintf(DATA_BUFFER, "[str:%d,act:%d,cart:%d,susp:%d]", STR_POS, ACT_POS, CART_POS, SUSP_POS);
  
  // format output to USB host {id, data, chksum}
  sprintf(OUTPUT_BUFFER, "{'id':%s,'data':%s,'chksum':%d}", UID, DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
  
  // handle PID
  steering.Compute();
  ballast.Compute();
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

// Check Throttle Up
void set_cart_backward(void) {
  VDC.setM1Speed(-400);
}

// Check Throttle Down
void set_cart_forward(void) {
  VDC.setM1Speed(400);
}
