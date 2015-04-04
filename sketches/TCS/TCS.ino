/*
  Traction Control Subsystem (TCS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include "PID_v1.h"
#include "AFMotor.h"

/* --- Global Constants --- */
// CAN
const char UID[] = "TCS";
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;
const int INTERVAL = 200; // millisecond wait

// Interrupts
const int SPARKPLUG_INT = 0;
const int DRIVESHAFT_INT = 1;
const int WHEEL_INT = 2;
const int ENCODER_INT = 3;
const int ENCODER_A_PIN = 10;
const int ENCODER_B_PIN = 11;

// Analog Input
const int CVT_PIN = A0;

/* --- Global Variables --- */
// PID
double K_P=1, K_I=0.05, K_D=0.25;
double setpoint, input, output;
PID cvt_pid(&input, &output, &setpoint, K_P, K_I, K_D, DIRECT);

// Counters
volatile int sparkplug_pulses = 0;
volatile int driveshaft_pulses = 0;
volatile int wheel_pulses = 0;
volatile int encoder_pulses = 0;

// Char Buffers
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

/* --- Setup --- */
void setup() {
  // Serial
  Serial.begin(BAUD);
  
  // Analog Inputs
  pinMode(CVT_PIN, INPUT);
  
  // Initialize PID
  cvt_pid.SetMode(AUTOMATIC);
  cvt_pid.SetTunings(K_P, K_I, K_D);
  
  // Initilize asynch interrupts
  attachInterrupt(SPARKPLUG_INT, sparkplug_counter, RISING);
  attachInterrupt(DRIVESHAFT_INT, driveshaft_counter, RISING);
  attachInterrupt(WHEEL_INT, wheel_counter, RISING);
  attachInterrupt(ENCODER_INT, encoder_counter, RISING);
}

/* --- Loop --- */
void loop() {
  // Reset looping counters (doesn't include the encoder)
  sparkplug_pulses = 0;
  driveshaft_pulses = 0;
  wheel_pulses = 0;
  
  // Wait and calculate
  delay(INTERVAL);
  int driveshaft_rpm =  driveshaft_pulses / INTERVAL;
  int sparkplug_rpm =  sparkplug_pulses / INTERVAL;
  int wheel_rpm =  wheel_pulses / INTERVAL;
  input = encoder_pulses;
  setpoint = analogRead(CVT_PIN);
  cvt_pid.Compute();
  
  // Output string buffer
  sprintf(DATA_BUFFER, "{'driveshaft':%d,'wheel':%d,'sparkplug':%d}", driveshaft_rpm, wheel_rpm, sparkplug_rpm);
  sprintf(OUTPUT_BUFFER, "{'uid':%s,data':%s,'chksum':%d}", UID, DATA_BUFFER, checksum());
  Serial.println(OUTPUT_BUFFER);
}

/* --- SYNCHRONOUS TASKS --- */
// Check Sum
int checksum() {
  int sum = 0;
  for (int i = 0; i < DATA_SIZE; i++) {
    sum += DATA_BUFFER[i];
  }
  int val = sum % 256;
  return val;
}

/* --- ASYNCHRONOUS TASKS --- */
void sparkplug_counter(void) {
  sparkplug_pulses++;
}

void driveshaft_counter(void) {
  driveshaft_pulses++;
}

void wheel_counter(void) {
  wheel_pulses++;
}

void encoder_counter(void) {
  if (digitalRead(ENCODER_A_PIN) == digitalRead(ENCODER_B_PIN)) {
    encoder_pulses++;
  }
  else {
    encoder_pulses--;
  }
}
