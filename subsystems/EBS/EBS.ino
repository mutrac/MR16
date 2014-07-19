/*
  Electronic Braking Subsystem (EBS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */

/* --- Global Constants --- */
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int LEFT_BRAKE_PIN = A0;
const int RIGHT_BRAKE_PIN = A1;

/* --- Global Variables --- */
int right_brake = 0;
int left_brake = 0;
char output_buffer[BUFFER_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  right_brake = analogRead(RIGHT_BRAKE_PIN);
  left_brake = analogRead(LEFT_BRAKE_PIN);
  sprintf(output_buffer, "left_brake:%d, right_brake:%d", left_brake, right_brake);
  Serial.println(output_buffer);
}
