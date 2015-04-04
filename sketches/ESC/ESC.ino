/*
  Engine Safety Controller (ESC)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include "DualVNH5019MotorShield.h"
#include "DallasTemperature.h"
#include "OneWire.h"

/* --- Global Constants --- */
const char ID[] = "ESC";
const int BAUD = 9600;
const int OUTPUT_SIZE = 256;
const int DATA_SIZE = 128;

// Digital Input
const int SEAT_KILLSWITCH_PIN = 2;
const int HITCH_KILLSWITCH_PIN = 3;
const int BUTTON_KILLSWITCH_PIN = 4;
const int IGNITION_PIN = 5;

// Relays
const int GROUND_RELAY_PIN = 8;
const int REGULATOR_RELAY_PIN = 9;
const int STARTER_RELAY_PIN = 10;
const int ATOM_RELAY_PIN = 11;

// Analog Input
const int LEFT_BRAKE_PIN = A0;
const int RIGHT_BRAKE_PIN = A1;
const int CVT_INTERLOCK_PIN = A2;

/* --- Global Variables --- */
int SEAT_KILL = 0;
int HITCH_KILL = 0;
int IGNITION = 0;
int CVT_LOCK = 0;
int BRAKE_LOCK = 0;
int GND_RELAY = 0;
int REG_RELAY = 0;
int START_RELAY = 0;
int LEFT_BRAKE = 0;
int RIGHT_BRAKE = 0;
int RFID_AUTH = 0;
char DATA_BUFFER[DATA_SIZE];
char OUTPUT_BUFFER[OUTPUT_SIZE];

/* --- Setup --- */
void setup() {
  
  Serial.begin(BAUD);
  
  // Digital Input
  pinMode(SEAT_KILLSWITCH_PIN, INPUT);
  pinMode(HITCH_KILLSWITCH_PIN, INPUT);
  pinMode(BUTTON_KILLSWITCH_PIN, INPUT);
  pinMode(IGNITION_PIN, INPUT);
  
  // Relays
  pinMode(GROUND_RELAY_PIN, OUTPUT);
  pinMode(REGULATOR_RELAY_PIN, OUTPUT);
  pinMode(STARTER_RELAY_PIN, OUTPUT);
  
  // Analog Input
  pinMode(RIGHT_BRAKE_PIN, INPUT);
  pinMode(LEFT_BRAKE_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  
  // Digital Input
  SEAT_KILL = digitalRead(SEAT_KILLSWITCH_PIN);
  HITCH_KILL = digitalRead(HITCH_KILLSWITCH_PIN);
  IGNITION = digitalRead(IGNITION_PIN);
  
  // Analog Input
  CVT_LOCK = analogRead(CVT_INTERLOCK_PIN);
  RIGHT_BRAKE = analogRead(RIGHT_BRAKE_PIN);
  LEFT_BRAKE = analogRead(LEFT_BRAKE_PIN);
  
  // USB
  sprintf(DATA_BUFFER, "{gnd_relay:%d,reg_relay:%d,starter_relay:%d,right_brake:%d,left_brake:%d,cvt_lock:%d,seat:%d,hitch:%d,ignition:%d}", GND_RELAY, REG_RELAY, START_RELAY, RIGHT_BRAKE, LEFT_BRAKE, CVT_LOCK, SEAT_KILL, HITCH_KILL, IGNITION);
  sprintf(OUTPUT_BUFFER, "{'id':%s,'data':%s,'chksum':%d}", ID, DATA_BUFFER, checksum());
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

