/*
  Dynamic Ballast Subsystem (DBS)
  McGill ASABE Tractor Team
*/

/* --- Libraries --- */

/* --- Global Constants --- */
const int BAUD = 57600;
const int BUFFER_SIZE = 256;
const int LOAD_CELL_PIN = A0;
const int CART_POSITION_PIN = A1;

/* --- Global Variables --- */
int load_cell = 0;
int cart_position = 0;
char output_buffer[BUFFER_SIZE];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(LOAD_CELL_PIN, INPUT);
  pinMode(CART_POSITION_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  cart_position = analogRead(CART_POSITION_PIN);
  sprintf(output_buffer, "cart_position:%d, load_cell:%d", cart_position, load_cell);
  Serial.println(output_buffer);
}
