/*
  Power Steering Subsystem (PSS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */

/* --- Global Constants --- */
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int STEERING_POSITION_PIN = A0;
const int ACTUATOR_POSITION_PIN = A1;

/* --- Global Variables --- */
int steering_position = 0;
int actuator_position = 0;
char output_buffer[BUFFER_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(STEERING_POSITION_PIN, INPUT);
  pinMode(ACTUATOR_POSITION_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  steering_position = analogRead(STEERING_POSITION_PIN);
  actuator_position = analogRead(ACTUATOR_POSITION_PIN);
  sprintf(output_buffer, "steering_position:%d, actuator_position:%d", steering_position, actuator_position);
  Serial.println(output_buffer);
}
