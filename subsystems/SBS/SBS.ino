/*
  Steering and Ballast Subsystem (SBS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include "PID_v1.h"

/* --- Global Constants --- */
const int BAUD = 57600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;
const int STEERING_POSITION_PIN = A0;
const int ACTUATOR_POSITION_PIN = A1;
const int SUSPENSION_POSITION_PIN = A2;
const int CART_POSITION_PIN = A3;

/* --- Global Variables --- */
int STR_POS = 0;
int ACT_POS = 0;
char OUTPUT_BUFFER[OUTPUT_SIZE];
int SUSP_POS = 0;
int CART_POS = 0;
char DATA_BUFFER[DATA_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(STEERING_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
  pinMode(SUSPENSION_POSITION_PIN, INPUT);
  pinMode(CART_POSITION_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  STR_POS = analogRead(STEERING_POSITION_PIN);
  ACT_POS = analogRead(ACTUATOR_POSITION_PIN);
  CART_POS = analogRead(CART_POSITION_PIN);
  SUSP_POS = analogRead(SUSPENSION_POSITION_PIN);
  sprintf(DATA_BUFFER, "[str_pos:%d,act_posi:%d,cart_pos:%d,susp_pos:%d]", STR_POS, ACT_POS, CART_POS, SUSP_POS);
  sprintf(OUTPUT_BUFFER, "'{data':%s,'chksum':%d}", DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
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
