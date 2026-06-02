/*
 * Gripper-only test for the Basra / Arduino UNO car.
 *
 * Use this sketch when the arm servos work but the gripper does not.
 *
 * Wiring:
 *   Gripper servo signal = D8
 *   Servo VCC            = stable 5V
 *   Servo GND            = Basra GND common ground
 *
 * Behavior:
 *   D8 moves between close/open automatically.
 *   The onboard LED blinks when the target angle changes.
 */

#include <Servo.h>

#define PIN_SERVO_GRIPPER  8
#define PIN_LED            13

// Tune these two values if the mechanical claw is installed in the opposite
// direction or needs a larger/smaller travel range.
#define GRIPPER_CLOSE      0
#define GRIPPER_OPEN       120

#define HOLD_MS            1500

static Servo servoGripper;

static void writeGripper(uint8_t angle) {
  servoGripper.write(angle);
  digitalWrite(PIN_LED, HIGH);
  delay(120);
  digitalWrite(PIN_LED, LOW);
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  servoGripper.attach(PIN_SERVO_GRIPPER);

  writeGripper(GRIPPER_CLOSE);
  delay(HOLD_MS);
}

void loop() {
  writeGripper(GRIPPER_OPEN);
  delay(HOLD_MS);

  writeGripper(GRIPPER_CLOSE);
  delay(HOLD_MS);
}
