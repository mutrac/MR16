/*
  Traction Control Subsystem (TCS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include "PID_v1.h"
#include "AFMotor.h"
#include <RunningMedian.h>

/* --- Global Constants --- */
// CAN
const char UID[] = "TCS";
const char PULL[] = "pull";
const char PUSH[] = "push";
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;
const int INTERVAL = 200; // millisecond wait

// Digital Inputs (some are Interrupts)
const int DRIVESHAFT_INT = 22;
const int WHEEL_INT = 24;
const int SPARKPLUG_INT = 26;
const int ENCODER_INT = 28;
const int ENCODER_A_PIN = 28;
const int ENCODER_B_PIN = 30;
const int CVT_MODE_PIN = 32; // the optional mode for pull vs. manual

// Analog Input
const int CVT_POSITION_PIN = A0;

// Stepper Motor Settings
const int STEPPER_SPEED = 100;
const int STEPPER_RESOLUTION = 48;
const int STEPPER_TYPE = 2; // biploar
const int STEPPER_DISENGAGE = 0; // the encoder reading when the stepper motor engages/disenchages the CVT
const int STEPPER_MAX = 1024; // the encoder reading when the stepper is fully extended

// CVT Settings
const float CVT_RATIO_MAX = 4.13;
const float CVT_RATIO_MIN = 0.77;

// Differential Settings
const float DIFF_RATIO_MAX = 3;
const float DIFF_RATIO_MIN = 1;

// Sample sets
const int SPARKPLUG_SAMPLESIZE = 10;
const int DRIVESHAFT_SAMPLESIZE = 10;
const int WHEEL_SAMPLESIZE = 10;

/* --- Global Variables --- */
// Mode variables
boolean PULL_MODE = 0;
int CVT_POSITION = 0;
float CVT_RATIO = CVT_RATIO_MIN;
float DIFF_RATIO = DIFF_RATIO_MIN;

// Manual mode PID
double MANUAL_P = 1;
double MANUAL_I = 0.05;
double MANUAL_D = 0.25;
double MANUAL_SET;
double MANUAL_IN;
double MANUAL_OUT;
PID MANUAL_PID(&MANUAL_IN, &MANUAL_OUT, &MANUAL_SET, MANUAL_P, MANUAL_I, MANUAL_D, DIRECT);

// Pull mode PID
double PULL_P = 1;
double PULL_I = 0.05;
double PULL_D = 0.25;
double PULL_SET;
double PULL_IN;
double PULL_OUT;
PID PULL_PID(&PULL_IN, &PULL_OUT, &PULL_SET, PULL_P, PULL_I, PULL_D, DIRECT);

// Initialize stepper motor
AF_Stepper STEPPER(STEPPER_RESOLUTION, STEPPER_TYPE); // TODO: find stepper resolution

// Counters
volatile int SPARKPLUG_PULSES = 0;
volatile int DRIVESHAFT_PULSES = 0;
volatile int WHEEL_PULSES = 0;
volatile int ENCODER_PULSES = 0;

int ENGINE_RPM = 0;
int DRIVESHAFT_RPM = 0;
int WHEEL_RPM = 0;

// Char Buffers
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

// Timers
long TIME = millis();

// Sample sets
RunningMedian ENGINE_HIST = RunningMedian(SPARKPLUG_SAMPLESIZE);
RunningMedian DRIVESHAFT_HIST = RunningMedian(DRIVESHAFT_SAMPLESIZE);
RunningMedian WHEEL_HIST = RunningMedian(WHEEL_SAMPLESIZE);

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
  STEPPER.setSpeed(STEPPER_SPEED); // the stepper speed in rpm
}

/* --- Loop --- */
void loop() {
  
  // Reset looping counters (doesn't include the encoder)
  SPARKPLUG_PULSES = 0;
  DRIVESHAFT_PULSES = 0;
  WHEEL_PULSES = 0;
  
  // Wait and get volatile values
  TIME = millis();
  delay(INTERVAL);
  DRIVESHAFT_HIST.add(DRIVESHAFT_PULSES / float(millis() - TIME));
  ENGINE_HIST.add(2 * SPARKPLUG_PULSES / float(millis() - TIME));
  WHEEL_HIST.add(WHEEL_PULSES / float(millis() - TIME));
  DRIVESHAFT_RPM = DRIVESHAFT_HIST.getAverage();
  ENGINE_RPM = ENGINE_HIST.getAverage();
  WHEEL_RPM = WHEEL_HIST.getAverage();
  CVT_RATIO = ENGINE_RPM / DRIVESHAFT_RPM;
  DIFF_RATIO = DRIVESHAFT_RPM / WHEEL_RPM;
  
  // Read the position of the joystick
  CVT_POSITION = analogRead(CVT_POSITION_PIN);
  
  // Calculate MANUAL controller output
  // In this mode, the encoder position to track the joystick position
  MANUAL_IN = double(ENCODER_PULSES);
  MANUAL_SET = double(CVT_POSITION);
  MANUAL_PID.Compute();
  
  // Calculate AUTOMATIC controller output
  // In this mode, the engine RPM is maximized by reducing the cvt_ratio until the disengagement threshold is reached
  PULL_IN = double(CVT_RATIO);
  PULL_SET = double(CVT_POSITION);
  PULL_PID.Compute();
   
  // Set CVT Mode
  if (digitalRead(CVT_MODE_PIN)) {
    if (PULL_MODE) {
      PULL_MODE = 0;
    }
    else {
      PULL_MODE = 1;
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
  sprintf(DATA_BUFFER, "{'driveshaft_rpm':%d,'wheel_rpm':%d,'engine_rpm':%d,'cvt_ratio':%d,'pull_mode':%d}", DRIVESHAFT_RPM, WHEEL_RPM,  ENGINE_RPM, CVT_RATIO, PULL_MODE);
  sprintf(OUTPUT_BUFFER, "{'uid':'%s',data':%s,'chksum':%d,'task':'%s'}", UID, DATA_BUFFER, checksum(), PUSH);
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
