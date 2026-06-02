/*
 * Basra car slave — 自定义舵机版本 + OpenMV二维码识别 + OLED显示
 *
 * 舵机引脚与量程：
 *   D12  夹爪      30°(夹紧)  ~ 90°(放松)     初始 90°
 *   D8   关节3     10°(往下)  ~ 170°(往上)    初始 80°
 *   D4   关节2     15°(往里)  ~ 160°(往外)    初始 30°
 *   D3   关节1     75°(往里)  ~ 180°(往外)    初始 120°
 *   D7   越障舵机                                      初始 90°
 *
 * 电机引脚：IN1=D5  IN2=D6  IN3=D9  IN4=D10
 *
 * OLED: SH1106 128x64 I2C, SDA=A4  SCL=A5
 * OpenMV: SoftwareSerial RX=A0  TX=A1
 *
 * 通信协议（CSV）：
 *   "cmd,j1,j2,j3,gripper,obstacle"   6字段
 *   "cmd,j1,j2,j3,gripper"            5字段（兼容）
 *
 * 调试命令：
 *   "G90"    直接设置夹爪角度
 *   "O45"    直接设置越障舵机角度
 */

#include <Servo.h>
#include <SoftwareSerial.h>
#include <U8x8lib.h>
#include <stdlib.h>
#include <string.h>

// ─── 电机引脚 ───
#define PIN_IN1          5
#define PIN_IN2          6
#define PIN_IN3          9
#define PIN_IN4          10

// ─── 舵机引脚 ───
#define PIN_SERVO_GRIPPER    12   // 夹爪
#define PIN_SERVO_JOINT3     8    // 关节3（靠近夹爪，从根部往上第三个）
#define PIN_SERVO_JOINT2     4    // 关节2（从根部往上第二个）
#define PIN_SERVO_JOINT1     3    // 关节1（从根部往上第一个）
#define PIN_SERVO_OBSTACLE   7    // 越障舵机
#define PIN_SERVO_OBSTACLE_REAR 11  // 越障后轮

// ─── 通信 ───
#define SERIAL_BAUD      9600
#define RX_TIMEOUT_MS    500
#define SERVO_STEP_DELAY_MS  15
#define SERVO_STEP_SIZE      1

// ─── 行驶命令 ───
#define CMD_FORWARD      1
#define CMD_BACKWARD     2
#define CMD_STOP         3
#define CMD_LEFT         4
#define CMD_RIGHT        5

// ─── 夹爪 D12 ───
#define GRIPPER_CLOSE    0      // 夹紧
#define GRIPPER_OPEN     180    // 张开
#define GRIPPER_HOME     180    // 初始（张开）

// ─── 关节3 D8（靠近夹爪，第三个关节）───
#define JOINT3_MIN       10     // 往下
#define JOINT3_MAX       170    // 往上
#define JOINT3_HOME      80

// ─── 关节2 D4（第二个关节）───
#define JOINT2_MIN       15     // 往里
#define JOINT2_MAX       160    // 往外
#define JOINT2_HOME      30

// ─── 关节1 D3（第一个关节，根部）───
#define JOINT1_MIN       75     // 往里
#define JOINT1_MAX       180    // 往外
#define JOINT1_HOME      120

// ─── 越障 D7 ───
#define OBSTACLE_HOME    90

// ─── 越障后轮 D11 ───
#define OBSTACLE_REAR_HOME 90

// ─── OpenMV + OLED ───
SoftwareSerial OpenMVSerial(A0, A1);            // RX=A0, TX=A1
U8X8_SH1106_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE);

String inputString = "";
boolean stringComplete = false;

// ─── 状态 ───
static unsigned long lastRxMs = 0;
static unsigned long lastServoStepMs = 0;
static uint8_t currentCmd = CMD_STOP;

static uint8_t currentJoint1Angle = JOINT1_HOME;
static uint8_t targetJoint1Angle  = JOINT1_HOME;
static uint8_t currentJoint2Angle = JOINT2_HOME;
static uint8_t targetJoint2Angle  = JOINT2_HOME;
static uint8_t currentJoint3Angle = JOINT3_HOME;
static uint8_t targetJoint3Angle  = JOINT3_HOME;
static uint8_t currentGripperAngle = GRIPPER_HOME;
static uint8_t targetGripperAngle  = GRIPPER_HOME;
static uint8_t currentObstacleAngle = OBSTACLE_HOME;
static uint8_t targetObstacleAngle  = OBSTACLE_HOME;
static uint8_t currentObstacleRearAngle = OBSTACLE_REAR_HOME;
static uint8_t targetObstacleRearAngle  = OBSTACLE_REAR_HOME;

static char rxLine[32];
static uint8_t rxLen = 0;

static Servo servoJoint1;
static Servo servoJoint2;
static Servo servoJoint3;
static Servo servoGripper;
static Servo servoObstacle;
static Servo servoObstacleRear;

// ─── 电机 ───
#define MOTOR_SPEED    255     // PWM占空比 0~255，255=满速

static void writeMotors(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
  analogWrite(PIN_IN1, in1);
  analogWrite(PIN_IN2, in2);
  analogWrite(PIN_IN3, in3);
  analogWrite(PIN_IN4, in4);
}

static void stopCar()   { writeMotors(0, 0, 0, 0); }
static void forward()   { writeMotors(0, MOTOR_SPEED, 0, MOTOR_SPEED); }
static void backward()  { writeMotors(MOTOR_SPEED, 0, MOTOR_SPEED, 0); }
static void left()      { writeMotors(0, MOTOR_SPEED, MOTOR_SPEED, 0); }
static void right()     { writeMotors(MOTOR_SPEED, 0, 0, MOTOR_SPEED); }

static void runCommand(uint8_t cmd) {
  switch (cmd) {
    case CMD_FORWARD:  forward();  break;
    case CMD_BACKWARD: backward(); break;
    case CMD_LEFT:     left();     break;
    case CMD_RIGHT:    right();    break;
    case CMD_STOP:
    default:           stopCar();  break;
  }
}

// ─── 舵机控制 ───
static void setJoint1Angle(uint8_t angle) {
  targetJoint1Angle = constrain(angle, JOINT1_MIN, JOINT1_MAX);
}

static void setJoint2Angle(uint8_t angle) {
  targetJoint2Angle = constrain(angle, JOINT2_MIN, JOINT2_MAX);
}

static void setJoint3Angle(uint8_t angle) {
  targetJoint3Angle = constrain(angle, JOINT3_MIN, JOINT3_MAX);
}

static void setGripperAngle(uint8_t angle) {
  targetGripperAngle = constrain(angle, 0, 255);
}

static void setObstacleAngle(uint8_t angle) {
  targetObstacleAngle = constrain(angle, 0, 180);
}

static void setObstacleRearAngle(uint8_t angle) {
  targetObstacleRearAngle = constrain(angle, 0, 180);
}

static void writeJoint1Home() {
  currentJoint1Angle = JOINT1_HOME;
  targetJoint1Angle  = JOINT1_HOME;
  servoJoint1.write(JOINT1_HOME);
}

static void writeJoint2Home() {
  currentJoint2Angle = JOINT2_HOME;
  targetJoint2Angle  = JOINT2_HOME;
  servoJoint2.write(JOINT2_HOME);
}

static void writeJoint3Home() {
  currentJoint3Angle = JOINT3_HOME;
  targetJoint3Angle  = JOINT3_HOME;
  servoJoint3.write(JOINT3_HOME);
}

static void writeGripperHome() {
  currentGripperAngle = GRIPPER_HOME;
  targetGripperAngle  = GRIPPER_HOME;
  servoGripper.write(GRIPPER_HOME);
}

static void writeObstacleHome() {
  currentObstacleAngle = OBSTACLE_HOME;
  targetObstacleAngle  = OBSTACLE_HOME;
  servoObstacle.write(OBSTACLE_HOME);
}

static void writeObstacleRearHome() {
  currentObstacleRearAngle = OBSTACLE_REAR_HOME;
  targetObstacleRearAngle  = OBSTACLE_REAR_HOME;
  servoObstacleRear.write(OBSTACLE_REAR_HOME);
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

  if (currentJoint1Angle != targetJoint1Angle) {
    currentJoint1Angle = stepTowards(currentJoint1Angle, targetJoint1Angle);
    servoJoint1.write(currentJoint1Angle);
  }
  if (currentJoint2Angle != targetJoint2Angle) {
    currentJoint2Angle = stepTowards(currentJoint2Angle, targetJoint2Angle);
    servoJoint2.write(currentJoint2Angle);
  }
  if (currentJoint3Angle != targetJoint3Angle) {
    currentJoint3Angle = stepTowards(currentJoint3Angle, targetJoint3Angle);
    servoJoint3.write(currentJoint3Angle);
  }
  if (currentGripperAngle != targetGripperAngle) {
    currentGripperAngle = stepTowards(currentGripperAngle, targetGripperAngle);
    servoGripper.write(currentGripperAngle);
  }
  if (currentObstacleAngle != targetObstacleAngle) {
    currentObstacleAngle = stepTowards(currentObstacleAngle, targetObstacleAngle);
    servoObstacle.write(currentObstacleAngle);
  }
  if (currentObstacleRearAngle != targetObstacleRearAngle) {
    currentObstacleRearAngle = stepTowards(currentObstacleRearAngle, targetObstacleRearAngle);
    servoObstacleRear.write(currentObstacleRearAngle);
  }
}

static void initServos() {
  servoJoint1.attach(PIN_SERVO_JOINT1);
  servoJoint2.attach(PIN_SERVO_JOINT2);
  servoJoint3.attach(PIN_SERVO_JOINT3);
  servoGripper.attach(PIN_SERVO_GRIPPER);
  servoObstacle.attach(PIN_SERVO_OBSTACLE);
  servoObstacleRear.attach(PIN_SERVO_OBSTACLE_REAR);

  writeJoint1Home();
  writeJoint2Home();
  writeJoint3Home();
  writeGripperHome();
  writeObstacleHome();
  writeObstacleRearHome();
}

// ─── 命令解析 ───
static void applyParsedCommand(uint8_t driveCmd, int j1, int j2, int j3,
                                int gripper, int obstacle, int obstacleRear) {
  currentCmd = driveCmd;
  lastRxMs = millis();
  runCommand(currentCmd);

  if (j1 >= 0)          setJoint1Angle((uint8_t)j1);
  if (j2 >= 0)          setJoint2Angle((uint8_t)j2);
  if (j3 >= 0)          setJoint3Angle((uint8_t)j3);
  if (gripper >= 0)     setGripperAngle((uint8_t)gripper);
  if (obstacle >= 0)    setObstacleAngle((uint8_t)obstacle);
  if (obstacleRear >= 0) setObstacleRearAngle((uint8_t)obstacleRear);
}

static void parseLine(char *line) {
  // 调试快捷命令
  if (line[0] == 'G' || line[0] == 'g') {
    setGripperAngle((uint8_t)atoi(line + 1));
    return;
  }
  if (line[0] == 'O' || line[0] == 'o') {
    setObstacleAngle((uint8_t)atoi(line + 1));
    return;
  }
  if (line[0] == 'J' || line[0] == 'j') {
    setJoint1Angle((uint8_t)atoi(line + 1));
    return;
  }
  if (line[0] == 'K' || line[0] == 'k') {
    setJoint2Angle((uint8_t)atoi(line + 1));
    return;
  }
  if (line[0] == 'M' || line[0] == 'm') {
    setJoint3Angle((uint8_t)atoi(line + 1));
    return;
  }

  // CSV格式: cmd,j1,j2,j3,gripper,obstacle[,obstacle_rear]
  if (line[0] < '1' || line[0] > '5') {
    return;
  }

  uint8_t driveCmd = (uint8_t)(line[0] - '0');
  int j1 = -1, j2 = -1, j3 = -1, gripper = -1, obstacle = -1, obstacleRear = -1;

  char *c1 = strchr(line, ',');
  if (c1) {
    j1 = atoi(c1 + 1);
    char *c2 = strchr(c1 + 1, ',');
    if (c2) {
      j2 = atoi(c2 + 1);
      char *c3 = strchr(c2 + 1, ',');
      if (c3) {
        j3 = atoi(c3 + 1);
        char *c4 = strchr(c3 + 1, ',');
        if (c4) {
          gripper = atoi(c4 + 1);
          char *c5 = strchr(c4 + 1, ',');
          if (c5) {
            obstacle = atoi(c5 + 1);
            char *c6 = strchr(c5 + 1, ',');
            if (c6) {
              obstacleRear = atoi(c6 + 1);
            }
          }
        }
      }
    }
  }

  applyParsedCommand(driveCmd, j1, j2, j3, gripper, obstacle, obstacleRear);
}

static void readSerialCommands() {
  while (Serial.available() > 0) {
    int b = Serial.read();

    if (b >= 1 && b <= 5) {
      applyParsedCommand((uint8_t)b, -1, -1, -1, -1, -1, -1);
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

// ─── OpenMV 二维码接收 + OLED 显示 ───
static void readOpenMVAndDisplay() {
  while (OpenMVSerial.available()) {
    char inChar = (char)OpenMVSerial.read();
    if (inChar != '\n') {
      inputString += inChar;
    } else {
      inputString.trim();
      stringComplete = true;
    }
  }

  if (stringComplete) {
    if (inputString == "SLOW") {
      // 检测到二维码轮廓 → 减速提示
      display.drawString(0, 1, "SLOW Mode >  ");
    } else {
      // 扫码完成，停车显示数据
      stopCar();
      currentCmd = CMD_STOP;
      display.clear();
      display.drawString(0, 1, "Target Found:");
      display.draw2x2String(0, 4, inputString.c_str());
    }
    inputString = "";
    stringComplete = false;
  }
}

// ─── setup / loop ───
void setup() {
  analogWrite(PIN_IN1, 0);
  analogWrite(PIN_IN2, 0);
  analogWrite(PIN_IN3, 0);
  analogWrite(PIN_IN4, 0);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  stopCar();
  initServos();
  Serial.begin(SERIAL_BAUD);
  lastRxMs = millis();

  // OpenMV 串口
  OpenMVSerial.begin(9600);
  inputString.reserve(32);

  // OLED 初始化
  display.begin();
  display.setPowerSave(0);
  display.setFont(u8x8_font_chroma48medium8_r);
  display.drawString(0, 0, "Basra Ready");
}

void loop() {
  readSerialCommands();

  if (millis() - lastRxMs > RX_TIMEOUT_MS) {
    currentCmd = CMD_STOP;
    stopCar();
  }

  updateServos();
  readOpenMVAndDisplay();
}
