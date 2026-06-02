/*
 * Basra car slave, rebuilt from the original slave.abp logic.
 *
 * Communication follows the source project:
 *   Serial.read() receives text lines from the wireless serial module.
 *
 * Supported formats:
 *   "1"          old source-compatible drive-only command
 *   "1,90,60"    drive command, arm angle, gripper angle
 *   "1,90,60,100" drive command, arm angle, gripper angle, aux angle
 *   "G120"       debug shortcut: gripper angle
 *   "A110"       debug shortcut: D12 aux servo angle
 *
 * Motor pins requested:
 *   IN1 = D5
 *   IN2 = D6
 *   IN3 = D9
 *   IN4 = D10
 *
 * D12 is used by the aux servo in this serial-wireless version.
 */

#include <Servo.h>
#include <stdlib.h>
#include <string.h>

#define PIN_IN1          5
#define PIN_IN2          6
#define PIN_IN3          9
#define PIN_IN4          10
#define PIN_STATUS_LED   A0

#define PIN_SERVO_ARM1       3
#define PIN_SERVO_ARM2       7
#define PIN_SERVO_GRIPPER    8
#define PIN_SERVO_AUX        12

#define SERIAL_BAUD      9600
#define RX_TIMEOUT_MS    500
#define SERVO_STEP_DELAY_MS  15   // 每步间隔(ms)，越大越慢
#define SERVO_STEP_SIZE      1    // 每步角度，越小越平滑

#define CMD_FORWARD      1
#define CMD_BACKWARD     2
#define CMD_STOP         3
#define CMD_LEFT         4
#define CMD_RIGHT        5

#define ARM_CENTER       90
#define ARM_MIN_ANGLE    45
#define ARM_MAX_ANGLE    135
#define GRIPPER_CLOSE    0
#define GRIPPER_OPEN     120

#define AUX_HOME         100
#define AUX_MIN_ANGLE    0
#define AUX_MAX_ANGLE    180

static unsigned long lastRxMs = 0;
static unsigned long lastServoStepMs = 0;
static uint8_t currentCmd = CMD_STOP;
static uint8_t currentArmAngle = ARM_CENTER;
static uint8_t targetArmAngle = ARM_CENTER;
static uint8_t currentGripperAngle = GRIPPER_CLOSE;
static uint8_t targetGripperAngle = GRIPPER_CLOSE;
static uint8_t currentAuxAngle = AUX_HOME;
static uint8_t targetAuxAngle = AUX_HOME;
static char rxLine[24];
static uint8_t rxLen = 0;

static Servo servoArm1;
static Servo servoArm2;
static Servo servoGripper;
static Servo servoAux;

static void writeMotors(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
  digitalWrite(PIN_IN1, in1);
  digitalWrite(PIN_IN2, in2);
  digitalWrite(PIN_IN3, in3);
  digitalWrite(PIN_IN4, in4);
}

static void stopCar() {
  writeMotors(LOW, LOW, LOW, LOW);
}

static void forward() {
  writeMotors(LOW, HIGH, LOW, HIGH);
}

static void backward() {
  writeMotors(HIGH, LOW, HIGH, LOW);
}

static void left() {
  writeMotors(LOW, HIGH, HIGH, LOW);
}

static void right() {
  writeMotors(HIGH, LOW, LOW, HIGH);
}

static void runCommand(uint8_t cmd) {
  switch (cmd) {
    case CMD_FORWARD:
      forward();
      break;
    case CMD_BACKWARD:
      backward();
      break;
    case CMD_LEFT:
      left();
      break;
    case CMD_RIGHT:
      right();
      break;
    case CMD_STOP:
    default:
      stopCar();
      break;
  }
}

static void setArmAngle(uint8_t angle) {
  targetArmAngle = constrain(angle, ARM_MIN_ANGLE, ARM_MAX_ANGLE);
}

static void writeArmHome() {
  currentArmAngle = ARM_CENTER;
  targetArmAngle = ARM_CENTER;
  servoArm1.write(ARM_CENTER);
  servoArm2.write(ARM_CENTER);
}

static void setGripperAngle(uint8_t angle) {
  targetGripperAngle = constrain(angle, GRIPPER_CLOSE, GRIPPER_OPEN);
}

static void writeGripperHome() {
  currentGripperAngle = GRIPPER_CLOSE;
  targetGripperAngle = GRIPPER_CLOSE;
  servoGripper.write(GRIPPER_CLOSE);
}

static void setAuxAngle(uint8_t angle) {
  targetAuxAngle = constrain(angle, AUX_MIN_ANGLE, AUX_MAX_ANGLE);
}

static void writeAuxHome() {
  currentAuxAngle = AUX_HOME;
  targetAuxAngle = AUX_HOME;
  servoAux.write(AUX_HOME);
}

static uint8_t stepTowards(uint8_t current, uint8_t target) {
  int diff = (int)target - (int)current;
  if (abs(diff) <= (int)SERVO_STEP_SIZE) {
    return target;
  }
  return (uint8_t)((int)current + (diff > 0 ? (int)SERVO_STEP_SIZE : -(int)SERVO_STEP_SIZE));
}

static void updateServos() {
  unsigned long now = millis();
  if (now - lastServoStepMs < SERVO_STEP_DELAY_MS) {
    return;
  }
  lastServoStepMs = now;

  if (currentArmAngle != targetArmAngle) {
    currentArmAngle = stepTowards(currentArmAngle, targetArmAngle);
    servoArm1.write(currentArmAngle);
    servoArm2.write(currentArmAngle);
  }

  if (currentGripperAngle != targetGripperAngle) {
    currentGripperAngle = stepTowards(currentGripperAngle, targetGripperAngle);
    servoGripper.write(currentGripperAngle);
  }

  if (currentAuxAngle != targetAuxAngle) {
    currentAuxAngle = stepTowards(currentAuxAngle, targetAuxAngle);
    servoAux.write(currentAuxAngle);
  }
}

static void initArm() {
  servoArm1.attach(PIN_SERVO_ARM1);
  servoArm2.attach(PIN_SERVO_ARM2);
  servoGripper.attach(PIN_SERVO_GRIPPER);
  servoAux.attach(PIN_SERVO_AUX);

  writeArmHome();
  writeGripperHome();
  writeAuxHome();
}

static void applyParsedCommand(uint8_t driveCmd, int armAngle, int gripperAngle, int auxAngle) {
  currentCmd = driveCmd;
  lastRxMs = millis();
  runCommand(currentCmd);

  if (armAngle >= 0) {
    setArmAngle((uint8_t)armAngle);
  }

  if (gripperAngle >= 0) {
    setGripperAngle((uint8_t)gripperAngle);
  }

  if (auxAngle >= 0) {
    setAuxAngle((uint8_t)auxAngle);
  }
}

static void parseLine(char *line) {
  if (line[0] == 'G' || line[0] == 'g') {
    setGripperAngle((uint8_t)atoi(line + 1));
    return;
  }

  if (line[0] == 'A' || line[0] == 'a') {
    setAuxAngle((uint8_t)atoi(line + 1));
    return;
  }

  if (line[0] < '1' || line[0] > '5') {
    return;
  }

  uint8_t driveCmd = (uint8_t)(line[0] - '0');
  int armAngle = -1;
  int gripperAngle = -1;
  int auxAngle = -1;

  char *firstComma = strchr(line, ',');
  if (firstComma != NULL) {
    armAngle = atoi(firstComma + 1);

    char *secondComma = strchr(firstComma + 1, ',');
    if (secondComma != NULL) {
      gripperAngle = atoi(secondComma + 1);

      char *thirdComma = strchr(secondComma + 1, ',');
      if (thirdComma != NULL) {
        auxAngle = atoi(thirdComma + 1);
      }
    }
  }

  applyParsedCommand(driveCmd, armAngle, gripperAngle, auxAngle);
}

static void readSerialCommands() {
  while (Serial.available() > 0) {
    int b = Serial.read();

    if (b >= 1 && b <= 5) {
      applyParsedCommand((uint8_t)b, -1, -1, -1);
      rxLen = 0;
      continue;
    }

    if (b == '\n' || b == '\r') {
      if (rxLen > 0) {
        rxLine[rxLen] = '\0';
        parseLine(rxLine);
        rxLen = 0;
      }
      continue;
    }

    if (rxLen < sizeof(rxLine) - 1) {
      rxLine[rxLen++] = (char)b;
    } else {
      rxLen = 0;
    }
  }
}

void setup() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, LOW);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);

  stopCar();
  initArm();
  Serial.begin(SERIAL_BAUD);
  lastRxMs = millis();
}

void loop() {
  readSerialCommands();

  if (millis() - lastRxMs > RX_TIMEOUT_MS) {
    currentCmd = CMD_STOP;
    stopCar();
  }

  updateServos();

  digitalWrite(PIN_STATUS_LED, currentCmd == CMD_STOP ? LOW : HIGH);
}
