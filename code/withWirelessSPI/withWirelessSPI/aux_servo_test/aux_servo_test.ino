/*
 * Aux servo test for the Basra / Arduino UNO car.
 *
 * Wiring:
 *   Aux servo signal = D12
 *   Servo VCC        = stable 5V
 *   Servo GND        = Basra GND common ground
 *
 * Behavior:
 *   100 degrees home
 *   110 degrees: one S1-style clockwise step
 *   90 degrees: one S2-style counterclockwise step
 */

#include <Servo.h>

#define PIN_SERVO_AUX  12
#define AUX_HOME       100
#define AUX_CW_STEP    110
#define AUX_CCW_STEP   90
#define HOLD_MS        1200

static Servo servoAux;

void setup() {
  servoAux.attach(PIN_SERVO_AUX);
  servoAux.write(AUX_HOME);
  delay(HOLD_MS);
}

void loop() {
  servoAux.write(AUX_CW_STEP);
  delay(HOLD_MS);

  servoAux.write(AUX_HOME);
  delay(HOLD_MS);

  servoAux.write(AUX_CCW_STEP);
  delay(HOLD_MS);

  servoAux.write(AUX_HOME);
  delay(HOLD_MS);
}
