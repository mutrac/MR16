/*
  Dynamic Ballast Subsystem (DBS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */
#include <PID_v1.h>
#include <DualVNH5019MotorShield.h>


/* --- Global Constants --- */
const char ID[] = "DBS";
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int LOAD_CELL_PIN = A0;
const int CART_POSITION_PIN = A1;
const double K_P=1, K_I=0.05, K_D=0.25;

/* --- Global Variables --- */
double setpoint, input, output;
PID ballast_pid(&input, &output, &setpoint, K_P, K_I, K_D, DIRECT);
int load_cell = 0;
int cart_position = 0;
char output_buffer[BUFFER_SIZE];

/* --- Functions --- */
int find_cart_position(void) {
  int val = 0;
  return val;
}

int find_front_load(void) {
  int val = 0;
  return val;
}

/* --- Setup --- */
void setup(void) {
  Serial.begin(BAUD);
  pinMode(LOAD_CELL_PIN, INPUT);
  pinMode(CART_POSITION_PIN, INPUT);
  ballast_pid.SetMode(AUTOMATIC);
  ballast_pid.SetTunings(K_P, K_I, K_D);
  setpoint = INIT_SETPOINT;
}

/* --- Loop --- */
void loop(void) {
  if (Serial.available()) {
    Serial.parseInt();
  }
  else {
    ballast_pid.Compute();
    cart_position = find_cart_position();
    load_cell = find_front_load();
    sprintf(output_buffer, "{'pid':%s,'cart_position':%d,'load_cell':%d}", ID, cart_position, load_cell);
    Serial.println(output_buffer);
  }
}

