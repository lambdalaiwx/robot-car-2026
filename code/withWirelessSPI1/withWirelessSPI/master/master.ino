/*
 * Birdmen remote master — 模式切换版
 *
 * 左摇杆 A0/A1  → 行驶（前进/后退/左转/右转）
 * 右摇杆 Y(A3) → 根据模式控制不同舵机
 *
 *   模式0：Y → 越障(D7)
 *   模式1：Y → 越障后轮(D11)
 *   模式2：Y → 关节1(D3)
 *   模式3：Y → 关节2(D4)
 *   模式4：Y → 关节3(D8)
 *   模式5：Y → 夹爪(D12)
 *
 * Button0(D2) 正序切换：0→1→2→3→4→5→0
 * Button1(D3) 逆序切换：5→4→3→2→1→0→5
 *
 * 协议：Serial.println("cmd,j1,j2,j3,gripper,obstacle,obstacle_rear")
 */

#define PIN_JOY_X        A0
#define PIN_JOY_Y        A1
#define PIN_STICK_Y      A3
#define PIN_STICK_X      A2
#define PIN_BTN_0        2
#define PIN_BTN_1        3

#define SERIAL_BAUD      9600
#define SEND_INTERVAL_MS 50

#define LOW_ZONE_MAX     340
#define HIGH_ZONE_MIN    680

#define CMD_FORWARD      1
#define CMD_BACKWARD     2
#define CMD_STOP         3
#define CMD_LEFT         4
#define CMD_RIGHT        5

// ─── 舵机参数 ───
#define STICK_DEADZONE     80
#define STICK_STEP         2
#define STICK_STEP_INTERVAL_MS 15

#define JOINT1_HOME  120
#define JOINT1_MIN   75
#define JOINT1_MAX   180

#define JOINT2_HOME  30
#define JOINT2_MIN   15
#define JOINT2_MAX   160

#define JOINT3_HOME  80
#define JOINT3_MIN   10
#define JOINT3_MAX   170

#define GRIPPER_HOME 180
#define GRIPPER_MIN  0
#define GRIPPER_MAX  180

#define OBSTACLE_HOME 90
#define OBSTACLE_MIN  0
#define OBSTACLE_MAX  180

#define OBSTACLE_REAR_HOME 90
#define OBSTACLE_REAR_MIN  0
#define OBSTACLE_REAR_MAX  180

// ─── 模式 ───
#define MODE_0       0
#define MODE_1       1
#define MODE_2       2
#define MODE_3       3
#define MODE_4       4
#define MODE_5       5
#define MODE_COUNT   6

// 索引: 0=joint1, 1=joint2, 2=joint3, 3=gripper, 4=obstacle, 5=obstacle_rear
static const uint8_t HOME_VALS[] = { JOINT1_HOME, JOINT2_HOME, JOINT3_HOME, GRIPPER_HOME, OBSTACLE_HOME, OBSTACLE_REAR_HOME };
static const uint8_t MIN_VALS[]  = { JOINT1_MIN,  JOINT2_MIN,  JOINT3_MIN,  GRIPPER_MIN,  OBSTACLE_MIN,  OBSTACLE_REAR_MIN };
static const uint8_t MAX_VALS[]  = { JOINT1_MAX,  JOINT2_MAX,  JOINT3_MAX,  GRIPPER_MAX,  OBSTACLE_MAX,  OBSTACLE_REAR_MAX };

// ─── 状态 ───
static unsigned long lastSendMs = 0;
static unsigned long lastStepMs = 0;
static uint8_t angles[6];                        // 当前各舵机角度
static uint8_t mode = MODE_0;
static bool lastBtn0 = false;
static bool lastBtn1 = false;

// ─── 行驶 ───
static uint8_t readDriveCmd() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  if (x > HIGH_ZONE_MIN) return CMD_FORWARD;
  if (x < LOW_ZONE_MAX)  return CMD_BACKWARD;
  if (y < LOW_ZONE_MAX)  return CMD_LEFT;
  if (y > HIGH_ZONE_MIN) return CMD_RIGHT;
  return CMD_STOP;
}

// ─── 右摇杆Y → 舵机 ───
// servoIdx: 目标舵机在 angles[] 中的索引
static void readStick(uint8_t servoIdx) {
  int raw = analogRead(PIN_STICK_Y);
  int centered = raw - 512;

  if (abs(centered) < STICK_DEADZONE) return;

  unsigned long now = millis();
  if (now - lastStepMs < STICK_STEP_INTERVAL_MS) return;
  lastStepMs = now;

  int delta = (centered > 0) ? -STICK_STEP : STICK_STEP;
  angles[servoIdx] = (uint8_t)constrain((int)angles[servoIdx] + delta,
                                         MIN_VALS[servoIdx], MAX_VALS[servoIdx]);
}

// ─── 按钮切模式 ───
static void readButtons() {
  bool b0 = digitalRead(PIN_BTN_0) == LOW;
  bool b1 = digitalRead(PIN_BTN_1) == LOW;

  if (b0 && !lastBtn0) {
    mode = (mode + 1) % MODE_COUNT;
    Serial.print("Mode: "); Serial.println(mode);
  }
  if (b1 && !lastBtn1) {
    mode = (mode + MODE_COUNT - 1) % MODE_COUNT;
    Serial.print("Mode: "); Serial.println(mode);
  }

  lastBtn0 = b0;
  lastBtn1 = b1;
}

// ─── 模式 → 舵机索引 ───
// 索引: 0=joint1, 1=joint2, 2=joint3, 3=gripper, 4=obstacle, 5=obstacle_rear
// 模式0=obstacle, 模式1=obstacle_rear, 模式2=joint1, 模式3=joint2, 模式4=joint3, 模式5=gripper
static const uint8_t MODE_TO_SERVO[] = { 4, 5, 0, 1, 2, 3 };

// ─── 发送 ───
static void sendControlFrame() {
  uint8_t cmd = readDriveCmd();
  readButtons();
  readStick(MODE_TO_SERVO[mode]);

  // cmd, joint1, joint2, joint3, gripper, obstacle, obstacle_rear
  Serial.print(cmd);        Serial.print(',');
  Serial.print(angles[0]);  Serial.print(',');
  Serial.print(angles[1]);  Serial.print(',');
  Serial.print(angles[2]);  Serial.print(',');
  Serial.print(angles[3]);  Serial.print(',');
  Serial.print(angles[4]);  Serial.print(',');
  Serial.println(angles[5]);
}

void setup() {
  pinMode(PIN_BTN_0, INPUT_PULLUP);
  pinMode(PIN_BTN_1, INPUT_PULLUP);
  Serial.begin(SERIAL_BAUD);

  // 开机复位
  for (int i = 0; i < 6; i++) {
    angles[i] = HOME_VALS[i];
  }
  mode = MODE_0;
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendMs < SEND_INTERVAL_MS) return;
  lastSendMs = now;

  sendControlFrame();
}
