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
const char PULL[] = "pull";
const char PUSH[] = "push";
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

// Stepper Motor Settings
const int STEPPER_SPEED = 100;
const int STEPPER_RESOLUTION = 48;
const int STEPPER_TYPE = 2; // biploar
const int STEPPER_DISENGAGE = 0; // the encoder reading when the stepper motor engages/disenchages the CVT
const int STEPPER_MAX = 1024; // the encoder reading when the stepper is fully extended

// CVT Settings
const float CVT_RATIO_MAX = 4.13;
const float CVT_RATIO_MIN = 0.77;

/* --- Global Variables --- */
boolean PULL_MODE = 0;
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
AF_Stepper STEPPER(STEPPER_RESOLUTION, STEPPER_TYPE); // TODO: find stepper resolution

// Counters
volatile int SPARKPLUG_PULSES = 0;
volatile int DRIVESHAFT_PULSES = 0;
volatile int WHEEL_PULSES = 0;
volatile int ENCODER_PULSES = 0;

// Char Buffers
char OUTPUT_BUFFER[OUTPUT_SIZE];
char DATA_BUFFER[DATA_SIZE];

// Timers
long TIME_A = millis();
long TIME_B = millis();

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
  TIME_A = millis();
  delay(INTERVAL);
  TIME_B = millis();
  int driveshaft_rpm =  DRIVESHAFT_PULSES / INTERVAL;
  int sparkplug_rpm =  SPARKPLUG_PULSES / INTERVAL;
  int engine_rpm =  2 * sparkplug_rpm;
  int wheel_rpm =  WHEEL_PULSES / INTERVAL;
  int cvt_ratio = sparkplug_rpm / driveshaft_rpm;
  int differential_ratio = driveshaft_rpm / wheel_rpm;
  
  // Read the position of the joystick
  CVT_POSITION = analogRead(CVT_POSITION_PIN);
  
  // Calculate MANUAL controller output
  // In this mode, the encoder position to track the joystick position
  MANUAL_IN = double(ENCODER_PULSES);
  MANUAL_SET = double(CVT_POSITION);
  MANUAL_PID.Compute();
  
  // Calculate AUTOMATIC controller output
  // In this mode, the engine RPM is maximized by reducing the cvt_ratio until the disengagement threshold is reached
  PULL_IN = double(cvt_ratio);
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
  sprintf(DATA_BUFFER, "{'driveshaft_rpm':%d,'wheel_rpm':%d,'engine_rpm':%d,'cvt_ratio':%d,'pull_mode':%d}", driveshaft_rpm, wheel_rpm,  engine_rpm, cvt_ratio, PULL_MODE);
  sprintf(OUTPUT_BUFFER, "{'uid':'%s',data':%s,'chksum':%d,'task':%s}", UID, DATA_BUFFER, checksum(), PUSH);
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
