/*
 * Servo-only test for the Basra / Arduino UNO car arm.
 *
 * Servo pins from the vehicle design:
 *   Arm servo A = D3
 *   Arm servo B = D7
 *   Gripper     = D8
 *
 * The two arm servos move together from 45 to 135 degrees.
 * The gripper moves between 0 and 60 degrees.
 */

#include <Servo.h>

#define PIN_SERVO_ARM1     3
#define PIN_SERVO_ARM2     7
#define PIN_SERVO_GRIPPER  8

#define ARM_MIN_ANGLE      45
#define ARM_CENTER         90
#define ARM_MAX_ANGLE      135
#define GRIPPER_CLOSE      0
#define GRIPPER_OPEN       120

static Servo servoArm1;
static Servo servoArm2;
static Servo servoGripper;

static void setArm(uint8_t angle) {
  servoArm1.write(angle);
  servoArm2.write(angle);
}

void setup() {
  servoArm1.attach(PIN_SERVO_ARM1);
  servoArm2.attach(PIN_SERVO_ARM2);
  servoGripper.attach(PIN_SERVO_GRIPPER);

  setArm(ARM_CENTER);
  servoGripper.write(GRIPPER_CLOSE);
  delay(1000);
}

void loop() {
  setArm(ARM_MIN_ANGLE);
  delay(1000);

  setArm(ARM_CENTER);
  delay(1000);

  setArm(ARM_MAX_ANGLE);
  delay(1000);

  setArm(ARM_CENTER);
  delay(1000);

  servoGripper.write(GRIPPER_OPEN);
  delay(1000);

  servoGripper.write(GRIPPER_CLOSE);
  delay(1500);
}
