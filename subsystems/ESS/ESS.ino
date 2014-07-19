/*
  Engine Safety Subsystem (ESS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */

/* --- Global Constants --- */
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int SEAT_KILLSWITCH_PIN = 2;
const int HITCH_KILLSWITCH_PIN = 3;
const int BUTTON_KILLSWITCH_PIN = 4;
const int IGNITION_PIN = 5;
const int CVT_INTERLOCK_PIN = 6;
const int BRAKE_INTERLOCK_PIN = 7; // could also get position from EBS
const int GROUND_RELAY_PIN = 8;
const int REGULATOR_RELAY_PIN = 9;
const int STARTER_RELAY_PIN = 10;

/* --- Global Variables --- */
int seat_killswitch = 0;
int hitch_killswitch = 0;
int button_killswitch = 0;
int ignition = 0;
int cvt_interlock = 0;
int brake_interlock = 0;
int ground_relay = 0;
int regulator_relay = 0;
int starter_relay = 0;
char output_buffer[BUFFER_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(SEAT_KILLSWITCH_PIN, INPUT);
  pinMode(HITCH_KILLSWITCH_PIN, INPUT);
  pinMode(BUTTON_KILLSWITCH_PIN, INPUT);
  pinMode(IGNITION_PIN, INPUT);
  pinMode(GROUND_RELAY_PIN, OUTPUT);
  pinMode(REGULATOR_RELAY_PIN, OUTPUT);
  pinMode(STARTER_RELAY_PIN, OUTPUT);
}

/* --- Loop --- */
void loop() {
  seat_killswitch = digitalRead(SEAT_KILLSWITCH_PIN);
  hitch_killswitch = digitalRead(HITCH_KILLSWITCH_PIN);
  button_killswitch = digitalRead(BUTTON_KILLSWITCH_PIN);
  ignition = digitalRead(IGNITION_PIN);
  cvt_interlock = analogRead(CVT_INTERLOCK_PIN);
  brake_interlock = digitalRead(BRAKE_INTERLOCK_PIN);
  sprintf(output_buffer, "ground_relay:%d, regulator_relay:%d, starter_relay:%d, brake_interlock:%d, cvt_interlock:%d, seat_killswitch:%d, hitch_killswitch:%d, button_killswitch:%d, ignition:%d", ground_relay, regulator_relay, starter_relay, brake_interlock, cvt_interlock, seat_killswitch, hitch_killswitch, button_killswitch, ignition);
  Serial.println(output_buffer);
}
