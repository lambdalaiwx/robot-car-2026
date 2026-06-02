/*
 * Motor-only test for the Basra / Arduino UNO car.
 *
 * Use this before wireless debugging. It checks the requested motor pins:
 *   Left motor  = D5, D6
 *   Right motor = D9, D10
 *
 * Do not connect the wireless module while running this test.
 * This sketch only drives D5/D6/D9/D10; it does not test servos.
 */

#define PIN_LEFT_A   5
#define PIN_LEFT_B   6
#define PIN_RIGHT_A  9
#define PIN_RIGHT_B  10

#define TEST_SPEED   150
#define STEP_MS      1200

static void setMotor(uint8_t pinA, uint8_t pinB, int16_t speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    analogWrite(pinA, speed);
    digitalWrite(pinB, LOW);
  } else if (speed < 0) {
    digitalWrite(pinA, LOW);
    analogWrite(pinB, -speed);
  } else {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, LOW);
  }
}

static void drive(int16_t left, int16_t right) {
  setMotor(PIN_LEFT_A, PIN_LEFT_B, left);
  setMotor(PIN_RIGHT_A, PIN_RIGHT_B, right);
}

void setup() {
  digitalWrite(PIN_LEFT_A, LOW);
  digitalWrite(PIN_LEFT_B, LOW);
  digitalWrite(PIN_RIGHT_A, LOW);
  digitalWrite(PIN_RIGHT_B, LOW);

  pinMode(PIN_LEFT_A, OUTPUT);
  pinMode(PIN_LEFT_B, OUTPUT);
  pinMode(PIN_RIGHT_A, OUTPUT);
  pinMode(PIN_RIGHT_B, OUTPUT);

  drive(0, 0);
  delay(1000);
}

void loop() {
  drive(TEST_SPEED, TEST_SPEED);
  delay(STEP_MS);

  drive(0, 0);
  delay(500);

  drive(-TEST_SPEED, -TEST_SPEED);
  delay(STEP_MS);

  drive(0, 0);
  delay(500);

  drive(-TEST_SPEED, TEST_SPEED);
  delay(STEP_MS);

  drive(0, 0);
  delay(500);

  drive(TEST_SPEED, -TEST_SPEED);
  delay(STEP_MS);

  drive(0, 0);
  delay(1500);
}
