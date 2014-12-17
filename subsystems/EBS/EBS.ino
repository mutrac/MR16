/*
  Electronic Braking Subsystem (EBS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include <PID_v1.h>

/* --- Global Constants --- */
const char ID[] = "EBS";
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int INTERVAL = 200; // millisecond wait
const int LEFT_BRAKE_PIN = A0;
const int RIGHT_BRAKE_PIN = A1;
const double K_P=1, K_I=0.05, K_D=0.25;
const int INIT_SETPOINT = 100;

/* --- Global Variables --- */
double setpoint, input, output;
PID brakes_pid(&input, &output, &setpoint, K_P, K_I, K_D, DIRECT);
int right_brake_pos = 0;
int left_brake_pos = 0;
char output_buffer[BUFFER_SIZE];

/* --- Functions --- */
int check_right_brake(void) {
  int val = analogRead(RIGHT_BRAKE_PIN);
  return val;
}

int check_left_brake(void) {
  int val = analogRead(LEFT_BRAKE_PIN);
  return val;
}

/* --- Setup --- */
void setup(void) {
  input = analogRead(0);
  brakes_pid.SetMode(AUTOMATIC);
  brakes_pid.SetTunings(K_P, K_I, K_D);
  setpoint = INIT_SETPOINT;
  Serial.begin(BAUD);
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
}

/* --- Loop --- */
void loop(void) {
  if (Serial.available()) {
    Serial.parseInt();
  }
  else {
    brakes_pid.Compute();
    left_brake_pos = check_left_brake();
    right_brake_pos = check_right_brake();
    sprintf(output_buffer, "left_brake:%d, right_brake:%d", left_brake_pos, right_brake_pos);
    Serial.println(output_buffer);
  }
}
