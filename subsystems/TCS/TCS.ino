/*
  Traction Control Subsystem (TCS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include "PID_v1.h"
#include "AFMotor.h"

/* --- Global Constants --- */
const char ID[] = "DTS";
const int BAUD = 57600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;
const int INTERVAL = 200; // millisecond wait
const int SPARKPLUG_INT = 0;
const int DRIVESHAFT_INT = 1;
const int ENCODER_A_PIN = 10;
const int ENCODER_B_PIN = 11;
const double K_P=1, K_I=0.05, K_D=0.25;
const int INIT_SETPOINT = 100;

/* --- Global Variables --- */
double setpoint, input, output;
PID cvt_pid(&input, &output, &setpoint, K_P, K_I, K_D, DIRECT);
volatile int sparkplug_pulses = 0;
volatile int driveshaft_pulses = 0;
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  cvt_pid.SetMode(AUTOMATIC);
  cvt_pid.SetTunings(K_P, K_I, K_D);
  attachInterrupt(SPARKPLUG_INT, sparkplug_counter, RISING);
  attachInterrupt(DRIVESHAFT_INT, driveshaft_counter, RISING);
}

/* --- Loop --- */
void loop() {
  sparkplug_pulses = 0;
  driveshaft_pulses = 0;
  delay(INTERVAL);
  setpoint = driveshaft_pulses / sparkplug_pulses;
  cvt_pid.Compute();
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

/* --- Functions --- */
void sparkplug_counter(void) {
  sparkplug_pulses++;
}

void driveshaft_counter(void) {
  driveshaft_pulses++;
}
