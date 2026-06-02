// ============================================================
// 智能搬运机器人 - OpenMV视觉 + 底盘移动 + 四轴机械臂联合抓取
// ============================================================
#include <SoftwareSerial.h>
#include <Servo.h>

// --- OpenMV通信 (软串口) ---
SoftwareSerial OpenMV_Serial(11, 13); // RX=11(接OpenMV TX), TX=13(接OpenMV RX)

// --- 机械臂引脚与对象 ---
Servo myBase, myGripper, myShoulder, myElbow;
const int PIN_BASE = 3;
const int PIN_GRIPPER = 4;
const int PIN_SHOULDER = 8;
const int PIN_ELBOW = 12;

// --- 机械臂安全角度 ---
const int GRIPPER_OPEN = 90;
const int GRIPPER_CLOSE = 60;
const int SHOULDER_UP = 90;
const int SHOULDER_DOWN = 45;
const int ELBOW_RETRACT = 90;
const int ELBOW_EXTEND = 120;
const int BASE_CENTER = 90;

// --- 底盘电机引脚 ---
const int MOTOR_LEFT_F = 9;
const int MOTOR_LEFT_B = 10;
const int MOTOR_RIGHT_F = 5;
const int MOTOR_RIGHT_B = 6;

// --- 运动参数 ---
const unsigned long TIME_1_METER = 3000;

// --- 视觉目标坐标 ---
int target_x = -1;
int target_y = -1;
bool hasTarget = false;

void setup() {
  Serial.begin(115200);
  OpenMV_Serial.begin(115200);

  // 初始化电机
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();

  // 初始化机械臂
  myBase.attach(PIN_BASE);
  myGripper.attach(PIN_GRIPPER);
  myShoulder.attach(PIN_SHOULDER);
  myElbow.attach(PIN_ELBOW);

  // 机械臂复位
  Serial.println("系统复位中...");
  myBase.write(BASE_CENTER);
  myShoulder.write(SHOULDER_UP);
  myElbow.write(ELBOW_RETRACT);
  myGripper.write(GRIPPER_OPEN);

  Serial.println("准备就绪！等待 OpenMV 视觉数据...");
  delay(2000);
}

void loop() {
  // 1. 检查OpenMV数据
  readOpenMV();

  // 2. 如果收到目标坐标，执行抓取任务
  if (hasTarget) {
    hasTarget = false;
    Serial.print("收到目标: X=");
    Serial.print(target_x);
    Serial.print(", Y=");
    Serial.println(target_y);

    // TODO: 根据坐标判断是否需要转向、前进距离等
    // 目前先按固定流程执行
    executeGrabTask();
  }
}

// --- 读取OpenMV数据 ---
void readOpenMV() {
  if (OpenMV_Serial.available() > 0) {
    String receivedData = OpenMV_Serial.readStringUntil('\n');
    receivedData.trim();

    if (receivedData.length() > 0) {
      Serial.print("收到 OpenMV: ");
      Serial.println(receivedData);

      // 解析 "X:150,Y:120" 格式
      int xIdx = receivedData.indexOf("X:");
      int yIdx = receivedData.indexOf(",Y:");

      if (xIdx >= 0 && yIdx >= 0) {
        target_x = receivedData.substring(xIdx + 2, yIdx).toInt();
        target_y = receivedData.substring(yIdx + 3).toInt();
        hasTarget = true;
      }
    }
  }
}

// --- 抓取任务流程 ---
void executeGrabTask() {
  Serial.println(">>> 联合抓取任务开始 <<<");

  // 阶段一：底盘寻的
  Serial.println("1. 小车前进中...");
  moveForward();
  delay(TIME_1_METER);

  stopCar();
  Serial.println("到达目标点，刹车！");
  delay(1500);

  // 阶段二：探手抓取
  Serial.println("2. 机械臂下探...");
  myElbow.write(ELBOW_EXTEND);
  delay(800);
  myShoulder.write(SHOULDER_DOWN);
  delay(800);

  Serial.println("3. 夹爪闭合...");
  myGripper.write(GRIPPER_CLOSE);
  delay(1000);

  // 阶段三：安全收回
  Serial.println("4. 举起战利品，收回手臂...");
  myShoulder.write(SHOULDER_UP);
  delay(800);
  myElbow.write(ELBOW_RETRACT);
  delay(800);

  Serial.println(">>> 任务完成，等待下一个目标 <<<");
}

// --- 电机控制 ---
void moveForward() {
  digitalWrite(MOTOR_LEFT_F, HIGH);
  digitalWrite(MOTOR_LEFT_B, LOW);
  digitalWrite(MOTOR_RIGHT_F, HIGH);
  digitalWrite(MOTOR_RIGHT_B, LOW);
}

void stopCar() {
  digitalWrite(MOTOR_LEFT_F, LOW);
  digitalWrite(MOTOR_LEFT_B, LOW);
  digitalWrite(MOTOR_RIGHT_F, LOW);
  digitalWrite(MOTOR_RIGHT_B, LOW);
}
