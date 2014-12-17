/*
  Power Steering Subsystem (PSS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include <PID_v1.h>

/* --- Global Constants --- */
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int WHEEL_POSITION_PIN = A0;
const int ACTUATOR_POSITION_PIN = A1;

/* --- Global Variables --- */
int Wheel_position = 0;
int Actuator_position = 0;
char Output_buffer[BUFFER_SIZE];

/* --- Functions --- */
int check actuator_position(void) {
  int val = analogRead(ACTUATOR_POSITION_PIN);
  return val;
}

int check wheel_position(void) {
  int val = analogRead(WHEEL_POSITION_PIN);
  return val;
}

/* --- Setup --- */
void setup(void) {
  Serial.begin(BAUD);
  pinMode(WHEEL_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
}

/* --- Loop --- */
void loop(void) {
  Wheel_position = analogRead(WHEEL_POSITION_PIN);
  Actuator_position = analogRead(ACTUATOR_POSITION_PIN);
  sprintf(Output_buffer, "{'wheel_pos':%d,'actuator_pos':%d}", Wheel_position, Actuator_position);
  Serial.println(Output_buffer);
}
