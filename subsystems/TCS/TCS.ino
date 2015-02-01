/*
  Traction Control Subsystem (TCS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include "PID_v1.h"

/* --- Global Constants --- */
const int BAUD = 57600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;

/* --- Global Variables --- */
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
}

/* --- Loop --- */
void loop() {
  sprintf(DATA_BUFFER, "[]");
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
