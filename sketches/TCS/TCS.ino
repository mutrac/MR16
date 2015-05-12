/*
  Traction Control Subsystem (TCS)
  McGill ASABE Engineering Team
  
  Board: Arduino DUE ATmega368
*/

/* --- Libraries --- */
#include "PID_v1.h"
#include <Wire.h>
#include "Adafruit_MotorShield.h"
#include "utility/Adafruit_PWMServoDriver.h"
#include <RunningMedian.h>
#include "stdio.h"

/* --- Global Constants --- */
// CMQ
const char UID[] = "TCS";
const char PULL[] = "pull";
const char PUSH[] = "push";
const char PULL_CMD = 'P';
const char AUTO_CMD = 'A';
const char MANUAL_CMD = 'M';
const int BAUD = 9600;
const int DATA_SIZE = 128;
const int OUTPUT_SIZE = 256;
const int INTERVAL = 500; // millisecond wait

// Digital Inputs (some are Interrupts)
const int DRIVESHAFT_PIN = 22;
const int WHEEL_PIN = 24;
const int SPARKPLUG_PIN = 26;
const int ENCODER_A_PIN = 28;
const int ENCODER_B_PIN = 30;

// Analog Input
const int CVT_POSITION_PIN = A0;
const int CVT_POSITION_MIN = 512; 
const int CVT_POSITION_MAX = 1024; 

// Stepper Motor Settings
const int STEPPER_SPEED = 600;
const int STEPPER_RESOLUTION = 48;
const int STEPPER_TYPE = 2; // biploar
const int STEPPER_DISENGAGE = 0; // the encoder reading when the stepper motor engages/disenchages the CVT
const int STEPPER_MAX = 1024; // the encoder reading when the stepper is fully extended
const int STEPPER_RESET_TIME = 1000; // time it takes to fully retract the STEPPER
const int STEPPER_OUT_MIN = -100;
const int STEPPER_OUT_MAX = 100;

// Interrupt pulses/rev
const int DRIVESHAFT_PULSES_PER_REV = 12;
const int WHEEL_PULSES_PER_REV = 10;
const int ENGINE_PULSES_PER_REV = 2;

// Engine RPM
const int ENGINE_RPM_MIN = 1550;
const int ENGINE_RPM_MAX = 3600;

// CVT Settings
const float CVT_RATIO_MAX = 4.13;
const float CVT_RATIO_MIN = 0.77;

// Differential Settings
const float DIFF_RATIO_MAX = 3.0;
const float DIFF_RATIO_MIN = 1.0;

// Sample sets
const int SPARKPLUG_SAMPLESIZE = 5;
const int DRIVESHAFT_SAMPLESIZE = 5;
const int WHEEL_SAMPLESIZE = 5;

// Float to char
const int PRECISION = 2; // number of floating point decimal places
const int DIGITS = 6; // number of floating point digits 
const int CHARS = 8;

/* --- Global Variables --- */
// Float strings
char CVT_RATIO_S[CHARS];
char DIFF_RATIO_S[CHARS];

// Mode variables
boolean CVT_MODE = 0;
int CVT_POSITION = 0;
float CVT_RATIO = CVT_RATIO_MIN;
float DIFF_RATIO = DIFF_RATIO_MIN;

// Manual mode PID
double MANUAL_P = 1;
double MANUAL_I = 0;
double MANUAL_D = 0;
double MANUAL_SET;
double MANUAL_IN;
double MANUAL_OUT;
double MANUAL_OUT_MIN = -100;
double MANUAL_OUT_MAX = 100;
PID MANUAL_PID(&MANUAL_IN, &MANUAL_OUT, &MANUAL_SET, MANUAL_P, MANUAL_I, MANUAL_D, DIRECT);

// Pull mode PID
double PULL_P = 1;
double PULL_I = 0;
double PULL_D = 0;
double PULL_SET;
double PULL_IN;
double PULL_OUT;
double PULL_OUT_MIN = -100;
double PULL_OUT_MAX = 100;
PID PULL_PID(&PULL_IN, &PULL_OUT, &PULL_SET, PULL_P, PULL_I, PULL_D, DIRECT);

// Initialize stepper motor
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_StepperMotor *STEPPER = AFMS.getStepper(STEPPER_RESOLUTION, STEPPER_TYPE);

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
 
   // Initialid MANUAL MODE PID
  MANUAL_PID.SetMode(AUTOMATIC);
  MANUAL_PID.SetTunings(MANUAL_P, MANUAL_I, MANUAL_D);
  MANUAL_PID.SetOutputLimits(MANUAL_OUT_MIN, MANUAL_OUT_MAX);
  
  // Initialid PULL MODE PID
  PULL_PID.SetMode(AUTOMATIC);
  PULL_PID.SetTunings(PULL_P, PULL_I, PULL_D);
  PULL_PID.SetOutputLimits(PULL_OUT_MIN, PULL_OUT_MAX);
  
  // Initilize asynch interrupts
  attachInterrupt(SPARKPLUG_PIN, sparkplug_counter, RISING);
  attachInterrupt(DRIVESHAFT_PIN, driveshaft_counter, LOW);
  attachInterrupt(WHEEL_PIN, wheel_counter, LOW);
  attachInterrupt(ENCODER_A_PIN, encoder_counter, CHANGE);
  
  // Initialize Stepper
  AFMS.begin();  // create with the default frequency 1.6KHz
  STEPPER->setSpeed(STEPPER_SPEED); // the stepper speed in rpm
  STEPPER->step(STEPPER_MAX, BACKWARD, DOUBLE); // fully retract the STEPPER
  delay(STEPPER_RESET_TIME);
  ENCODER_PULSES = 0; // reset the ENCODER to zero
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
  
  // Drive Shaft RPM
  DRIVESHAFT_HIST.add(DRIVESHAFT_PULSES / float(millis() - TIME));
  DRIVESHAFT_RPM = 60 * DRIVESHAFT_HIST.getAverage() / DRIVESHAFT_PULSES_PER_REV;
  
  // Engine RPM
  ENGINE_HIST.add(SPARKPLUG_PULSES / float(millis() - TIME));
  ENGINE_RPM = 60 * ENGINE_HIST.getAverage() / ENGINE_PULSES_PER_REV;
  
  // Wheel RPM
  WHEEL_HIST.add(WHEEL_PULSES / float(millis() - TIME));
  WHEEL_RPM = 60 * WHEEL_HIST.getAverage() / WHEEL_PULSES_PER_REV;
  
  // Determine the CVT Ratio
  // TODO: Should it handle disengaged CVT?
  CVT_RATIO = ENGINE_RPM / DRIVESHAFT_RPM;
  dtostrf(CVT_RATIO, DIGITS, PRECISION, CVT_RATIO_S);

  // Determine the Differential Ratio
  DIFF_RATIO = DRIVESHAFT_RPM / WHEEL_RPM;
  dtostrf(DIFF_RATIO, DIGITS, PRECISION, DIFF_RATIO_S);
    
  // Read the position of the joystick
  CVT_POSITION = analogRead(CVT_POSITION_PIN);
  
  // Calculate MANUAL controller output
  // In this mode, the encoder to tracks the joystick position
  // the reading of CVT_POSITION is mapped to the range of the encoder
  // The CVT_POSITION (in ENCODER_PULSE units) is used as the SETPOINT to the PID object
  // The ENCODER_PULSES is used as the INPUT to the PID object
  MANUAL_IN = double(ENCODER_PULSES);
  MANUAL_SET = double (map(CVT_POSITION, CVT_POSITION_MIN, CVT_POSITION_MAX, 0, STEPPER_MAX));
  MANUAL_PID.Compute();
  
  // Calculate PULL (CVT Ratio Tracking) controller output
  // In this mode, the ENGINE_RPM is maximized by reducing the CVT_RATIO until the disengagement threshold is reached
  // CVT_RATIO is used as the INPUT to the PID object
  PULL_IN = double(ENGINE_RPM);
  PULL_SET = double(ENGINE_RPM_MAX);
  PULL_PID.Compute(); // ghost-writes to PULL_OUT
   
  // Set CVT Mode
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case PULL_CMD:
        CVT_MODE = 1;
        break;
      case MANUAL_CMD:
        CVT_MODE = 0;
        break;
    }
  }
  
  // Engage Stepper motor for either Manual or Pull Mode
  if (CVT_MODE) {
    if (PULL_IN > 0) {
      STEPPER->step(abs(int(PULL_OUT)), FORWARD, DOUBLE); 
    }
    else if (PULL_IN < 0) {
      STEPPER->step(abs(int(PULL_OUT)), BACKWARD, DOUBLE);
    }
  }
  else {
    if (MANUAL_OUT > 0) {
      STEPPER->step(abs(int(MANUAL_OUT)), FORWARD, DOUBLE); 
    }
    else if (MANUAL_IN < 0) {
      STEPPER->step(abs(int(MANUAL_OUT)), BACKWARD, DOUBLE);
    }
  }
  
  // Output string buffer
  sprintf(DATA_BUFFER, "{'driveshaft_rpm':%d,'wheel_rpm':%d,'engine_rpm':%d,'cvt_ratio':%s,'diff_ratio:%s,'cvt_mode':%d,'cvt_enc':%d}", DRIVESHAFT_RPM, WHEEL_RPM,  ENGINE_RPM, CVT_RATIO_S, DIFF_RATIO_S, CVT_MODE, ENCODER_PULSES);
  sprintf(OUTPUT_BUFFER, "{'uid':'%s',data':%s,'chksum':%d,'task':'%s'}", UID, DATA_BUFFER, checksum(), PUSH);
  Serial.println(OUTPUT_BUFFER);
}

/* --- SYNCHRONOUS TASKS --- */
// Float to Str
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}

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
