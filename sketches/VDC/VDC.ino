/*
  Vehicle Dynamics Controller (VDC)
  McGill ASABE Tractor Team
  Revision 2016
*/

/* --- Libraries --- */
#include "PID_v1.h"

/* --- Global Constants --- */
const char UID[] = "VDC";
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;
const int STEERING_POSITION_PIN = A0;
const int ACTUATOR_POSITION_PIN = A1;
const int SUSPENSION_POSITION_PIN = A2;
const int CART_POSITION_PIN = A3;

/* --- Global Variables --- */
int STR_POS = 0;
int ACT_POS = 0;
int SUSP_POS = 0;
int CART_POS = 0;
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];
double SSetpoint, SInput, SOutput;
double BSetpoint, BInput, BOutput;
PID steering(&SInput, &SOutput, &SSetpoint,2,5,1, DIRECT);
PID ballast(&BInput, &BOutput, &BSetpoint,2,5,1, DIRECT);

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(STEERING_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
  pinMode(SUSPENSION_POSITION_PIN, INPUT);
  pinMode(CART_POSITION_PIN, INPUT);
  BSetpoint = 100;
  SSetpoint = 100;
  steering.SetMode(AUTOMATIC);
  ballast.SetMode(AUTOMATIC);
}

/* --- Loop --- */
void loop() {
  
  // read sensors
  STR_POS = analogRead(STEERING_POSITION_PIN);
  ACT_POS = analogRead(ACTUATOR_POSITION_PIN);
  CART_POS = analogRead(CART_POSITION_PIN);
  SUSP_POS = analogRead(SUSPENSION_POSITION_PIN);
  
  // format data buffer
  sprintf(DATA_BUFFER, "[str:%d,act:%d,cart:%d,susp:%d]", STR_POS, ACT_POS, CART_POS, SUSP_POS);
  
  // format output to USB host {id, data, chksum}
  sprintf(OUTPUT_BUFFER, "{'id':%s,'data':%s,'chksum':%d}", UID, DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
  
  // handle PID
  steering.Compute();
  ballast.Compute();
}

/* --- Check Sum --- */
int checksum() {
  int sum = 0;
  for (int i = 0; i < DATA_SIZE; i++) {
    sum += DATA_BUFFER[i];
  }
  int val = sum % 256;
  return val;
}
