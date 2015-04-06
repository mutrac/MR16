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

// Digital Inputs
const int CVT_MODE_PIN = 12;

// Analog Input
const int CVT_POSITION_PIN = A0;

/* --- Global Variables --- */
boolean PULL_MODE = false;
int CVT_POSITION = 0;

// Manual PID
double MANUAL_P=1, MANUAL_I=0.05, MANUAL_D=0.25;
double MANUAL_SET, MANUAL_IN, MANUAL_OUT;
PID MANUAL_PID(&MANUAL_IN, &MANUAL_OUT, &MANUAL_SET, MANUAL_P, MANUAL_I, MANUAL_D, DIRECT);

// Pull PID
double PULL_P=1, PULL_I=0.05, PULL_D=0.25;
double PULL_SET, PULL_IN, PULL_OUT;
PID PULL_PID(&PULL_IN, &PULL_OUT, &PULL_SET, PULL_P, PULL_I, PULL_D, DIRECT);

// Stepper
AF_Stepper STEPPER(48, 2); // TODO: find stepper resolution

// Counters
volatile int SPARKPLUG_PULSES = 0;
volatile int DRIVESHAFT_PULSES = 0;
volatile int WHEEL_PULSES = 0;
volatile int ENCODER_PULSES = 0;

// Char Buffers
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

/* --- Setup --- */
void setup() {
  
  // Serial
  Serial.begin(BAUD);
  
  // Analog Inputs
  pinMode(CVT_POSITION_PIN, INPUT);
  pinMode(CVT_MODE_PIN, INPUT);

  // Initialize Manual PID
  MANUAL_PID.SetMode(AUTOMATIC);
  MANUAL_PID.SetTunings(MANUAL_P, MANUAL_I, MANUAL_D);
  
  // Initialid Pull PID
  PULL_PID.SetMode(AUTOMATIC);
  PULL_PID.SetTunings(PULL_P, PULL_I, PULL_D);
  
  // Initilize asynch interrupts
  attachInterrupt(SPARKPLUG_INT, sparkplug_counter, RISING);
  attachInterrupt(DRIVESHAFT_INT, driveshaft_counter, RISING);
  attachInterrupt(WHEEL_INT, wheel_counter, RISING);
  attachInterrupt(ENCODER_INT, encoder_counter, RISING);
  
  // Initialize Stepper
  STEPPER.setSpeed(100); // the stepper speed in rpm
}

/* --- Loop --- */
void loop() {
  
  // Reset looping counters (doesn't include the encoder)
  SPARKPLUG_PULSES = 0;
  DRIVESHAFT_PULSES = 0;
  WHEEL_PULSES = 0;
  
  // Wait and get values
  delay(INTERVAL);
  int driveshaft_rpm =  DRIVESHAFT_PULSES / INTERVAL;
  int sparkplug_rpm =  SPARKPLUG_PULSES / INTERVAL;
  int wheel_rpm =  WHEEL_PULSES / INTERVAL;
  int cvt_ratio = sparkplug_rpm / driveshaft_rpm;
  int differential_ratio = driveshaft_rpm / wheel_rpm;
  CVT_POSITION = analogRead(CVT_POSITION_PIN);
  
  // Calculate manual controller output
  MANUAL_IN = double(ENCODER_PULSES);
  MANUAL_SET = double(CVT_POSITION);
  MANUAL_PID.Compute();
  
  // Calculate automatic controller output
  PULL_IN = double(cvt_ratio);
  PULL_SET = double(CVT_POSITION);
  PULL_PID.Compute();
   
  // Set CVT Mode
  if (digitalRead(CVT_MODE_PIN)) {
    if (PULL_MODE) {
      PULL_MODE = false;
    }
    else {
      PULL_MODE = true;
    }
  }

  // Engage Stepper motor for either Manual or Pull Mode
  if (PULL_MODE) {
    if (PULL_OUT > 0) {
      STEPPER.step(PULL_OUT, FORWARD, SINGLE); 
    }
    else if (PULL_OUT < 0) {
      STEPPER.step(PULL_OUT, BACKWARD, SINGLE); 
    }
  }
  else {
    if (MANUAL_OUT > 0) {
      STEPPER.step(MANUAL_OUT, FORWARD, SINGLE); 
    }
    else if (MANUAL_OUT < 0) {
      STEPPER.step(MANUAL_OUT, BACKWARD, SINGLE); 
    }
  }
  
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
  SPARKPLUG_PULSES++;
}

void driveshaft_counter(void) {
  DRIVESHAFT_PULSES++;
}

void wheel_counter(void) {
  WHEEL_PULSES++;
}

void encoder_counter(void) {
  if (digitalRead(ENCODER_A_PIN) == digitalRead(ENCODER_B_PIN)) {
    ENCODER_PULSES++;
  }
  else {
    ENCODER_PULSES--;
  }
}
