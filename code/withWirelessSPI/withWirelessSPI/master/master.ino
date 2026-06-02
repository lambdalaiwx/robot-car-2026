/*
 * Birdmen remote master, rebuilt from the original master.abp logic.
 *
 * Communication follows the source project:
 *   Serial.println("drive,arm,gripper,aux")
 *
 * The first field is still the original 1~5 drive command, so the receiver can
 * stay backward-compatible with old single-number commands.
 *
 * The wireless module is treated as a serial transparent-transmission module:
 *   Basra TX(D1) -> wireless RX
 *   Basra RX(D0) -> wireless TX
 *   GND          -> GND
 *   VCC          -> module VCC
 *
 * Because D0/D1 are used by Serial, unplug the wireless module when uploading
 * if the IDE reports "programmer is not responding" or "not in sync".
 */

#define PIN_JOY_X        A0
#define PIN_JOY_Y        A1
#define PIN_ARM_Y        A3    // 右摇杆 Y → 机械臂
#define PIN_GRIPPER_X    A2    // 右摇杆 X → 夹爪
#define PIN_BTN_S1       2
#define PIN_BTN_S2       3
#define PIN_LED_ACTIVE   7
#define PIN_LED_STOP     8

#define SERIAL_BAUD      9600
#define SEND_INTERVAL_MS 50

// Keep the thresholds close to the original ArduBlock map(0~1024 -> 0~3)
// behavior, but add a center stop zone so the car does not drift.
#define LOW_ZONE_MAX     340
#define HIGH_ZONE_MIN    680

#define CMD_FORWARD      1
#define CMD_BACKWARD     2
#define CMD_STOP         3
#define CMD_LEFT         4
#define CMD_RIGHT        5

#define ARM_CENTER       90
#define ARM_MIN_ANGLE    45
#define ARM_MAX_ANGLE    135
#define ARM_DEADZONE     80
#define ARM_STEP         2              // 每次增量角度
#define ARM_STEP_INTERVAL_MS 30        // 增量间隔(ms)，越小越快

#define GRIPPER_CLOSE    0
#define GRIPPER_OPEN     120
#define GRIPPER_HOME     60             // 初始中间位置
#define GRIPPER_DEADZONE 60
#define GRIPPER_STEP     2              // 每次增量角度
#define GRIPPER_STEP_INTERVAL_MS 30    // 增量间隔(ms)

#define AUX_HOME         100
#define AUX_MIN_ANGLE    0
#define AUX_MAX_ANGLE    180
#define AUX_STEP         10
#define AUX_STEP_INTERVAL_MS 150

static unsigned long lastSendMs = 0;
static unsigned long lastAuxStepMs = 0;
static unsigned long lastArmStepMs = 0;
static unsigned long lastGripperStepMs = 0;
static uint8_t heldArmAngle = ARM_CENTER;
static uint8_t heldGripperAngle = GRIPPER_HOME;
static uint8_t heldAuxAngle = AUX_HOME;
static bool lastS1Pressed = false;
static bool lastS2Pressed = false;

static uint8_t readCommandFromJoystick() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);

  if (x > HIGH_ZONE_MIN) {
    return CMD_FORWARD;
  }
  if (x < LOW_ZONE_MAX) {
    return CMD_BACKWARD;
  }
  if (y < LOW_ZONE_MAX) {
    return CMD_LEFT;
  }
  if (y > HIGH_ZONE_MIN) {
    return CMD_RIGHT;
  }
  return CMD_STOP;
}

static uint8_t readArmAngle() {
  int raw = analogRead(PIN_ARM_Y);
  int centered = raw - 512;

  // 处于死区，不动
  if (abs(centered) < ARM_DEADZONE) {
    return heldArmAngle;
  }

  unsigned long now = millis();
  if (now - lastArmStepMs < ARM_STEP_INTERVAL_MS) {
    return heldArmAngle;
  }
  lastArmStepMs = now;

  // 摇杆下压(值大) → 降臂(角度减小)
  // 摇杆上推(值小) → 抬臂(角度增大)
  if (centered > 0) {
    // 下推，降臂
    heldArmAngle = (uint8_t)constrain((int)heldArmAngle - ARM_STEP,
                                       ARM_MIN_ANGLE,
                                       ARM_MAX_ANGLE);
  } else {
    // 上推，抬臂
    heldArmAngle = (uint8_t)constrain((int)heldArmAngle + ARM_STEP,
                                       ARM_MIN_ANGLE,
                                       ARM_MAX_ANGLE);
  }
  return heldArmAngle;
}

static uint8_t readGripperAngle() {
  int raw = analogRead(PIN_GRIPPER_X);
  int centered = raw - 512;

  // 处于死区，不动
  if (abs(centered) < GRIPPER_DEADZONE) {
    return heldGripperAngle;
  }

  unsigned long now = millis();
  if (now - lastGripperStepMs < GRIPPER_STEP_INTERVAL_MS) {
    return heldGripperAngle;
  }
  lastGripperStepMs = now;

  // 摇杆右推(值大) → 闭合(角度减小)
  // 摇杆左推(值小) → 张开(角度增大)
  if (centered > 0) {
    // 右推，闭合
    heldGripperAngle = (uint8_t)constrain((int)heldGripperAngle - GRIPPER_STEP,
                                           GRIPPER_CLOSE,
                                           GRIPPER_OPEN);
  } else {
    // 左推，张开
    heldGripperAngle = (uint8_t)constrain((int)heldGripperAngle + GRIPPER_STEP,
                                           GRIPPER_CLOSE,
                                           GRIPPER_OPEN);
  }
  return heldGripperAngle;
}

static uint8_t readAuxAngle() {
  bool s1Pressed = digitalRead(PIN_BTN_S1) == LOW;
  bool s2Pressed = digitalRead(PIN_BTN_S2) == LOW;
  unsigned long now = millis();

  if (s1Pressed && !lastS1Pressed && !s2Pressed &&
      now - lastAuxStepMs >= AUX_STEP_INTERVAL_MS) {
    heldAuxAngle = (uint8_t)constrain((int)heldAuxAngle + AUX_STEP,
                                      AUX_MIN_ANGLE,
                                      AUX_MAX_ANGLE);
    lastAuxStepMs = now;
  }

  if (s2Pressed && !lastS2Pressed && !s1Pressed &&
      now - lastAuxStepMs >= AUX_STEP_INTERVAL_MS) {
    heldAuxAngle = (uint8_t)constrain((int)heldAuxAngle - AUX_STEP,
                                      AUX_MIN_ANGLE,
                                      AUX_MAX_ANGLE);
    lastAuxStepMs = now;
  }

  lastS1Pressed = s1Pressed;
  lastS2Pressed = s2Pressed;

  return heldAuxAngle;
}

static void sendControlFrame() {
  uint8_t cmd = readCommandFromJoystick();
  uint8_t armAngle = readArmAngle();
  uint8_t gripperAngle = readGripperAngle();
  uint8_t auxAngle = readAuxAngle();

  Serial.print(cmd);
  Serial.print(',');
  Serial.print(armAngle);
  Serial.print(',');
  Serial.print(gripperAngle);
  Serial.print(',');
  Serial.println(auxAngle);

  digitalWrite(PIN_LED_ACTIVE, auxAngle != AUX_HOME ? HIGH : (cmd != CMD_STOP ? HIGH : LOW));
  digitalWrite(PIN_LED_STOP, cmd == CMD_STOP ? HIGH : LOW);
}

void setup() {
  pinMode(PIN_BTN_S1, INPUT_PULLUP);
  pinMode(PIN_BTN_S2, INPUT_PULLUP);
  pinMode(PIN_LED_ACTIVE, OUTPUT);
  pinMode(PIN_LED_STOP, OUTPUT);

  Serial.begin(SERIAL_BAUD);

  digitalWrite(PIN_LED_ACTIVE, HIGH);
  digitalWrite(PIN_LED_STOP, HIGH);
  delay(200);
  digitalWrite(PIN_LED_ACTIVE, LOW);
  digitalWrite(PIN_LED_STOP, LOW);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendMs < SEND_INTERVAL_MS) {
    return;
  }
  lastSendMs = now;

  sendControlFrame();
}
