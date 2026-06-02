// ============================================================
// 智能搬运机器人 - 底盘移动与四轴机械臂联合抓取任务
// ============================================================
#include <Servo.h>

// --- 1. 机械臂引脚与对象 ---
Servo myBase, myGripper, myShoulder, myElbow;
const int PIN_BASE = 3;
const int PIN_GRIPPER = 4;
const int PIN_SHOULDER = 8;
const int PIN_ELBOW = 12;

// --- 2. 机械臂安全角度 (极其重要：请填入你之前测出的完美角度！) ---
const int GRIPPER_OPEN = 90;    // 夹爪张开
const int GRIPPER_CLOSE = 60;   // 夹爪闭合 (绝对不能写太小导致吱吱声)

const int SHOULDER_UP = 90;     // 大臂抬起安全高度
const int SHOULDER_DOWN = 45;   // 大臂下探抓取高度

const int ELBOW_RETRACT = 90;   // 小臂收回
const int ELBOW_EXTEND = 120;   // 小臂伸出

const int BASE_CENTER = 90;     // 底座居中朝前

// --- 3. 底盘电机引脚定义 (已更新为你的最新接线) ---
const int MOTOR_LEFT_F = 9;     // 左轮前进
const int MOTOR_LEFT_B = 10;    // 左轮后退
const int MOTOR_RIGHT_F = 5;    // 右轮前进
const int MOTOR_RIGHT_B = 6;    // 右轮后退

// --- 4. 运动参数 (核心调参区) ---
// 小车走完 1 米大概需要的毫秒数 (需要你真实测试并修改)
const unsigned long TIME_1_METER = 3000; 

// 任务锁：保证整套动作只执行一次
bool taskCompleted = false; 

void setup() {
  Serial.begin(9600);
  
  // 1. 初始化电机引脚并确保刹车
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();
  
  // 2. 初始化机械臂引脚
  myBase.attach(PIN_BASE);
  myGripper.attach(PIN_GRIPPER);
  myShoulder.attach(PIN_SHOULDER);
  myElbow.attach(PIN_ELBOW);
  
  // 3. 开机先让机械臂复位到防御姿态，防撞击
  Serial.println("系统复位中...");
  myBase.write(BASE_CENTER);
  myShoulder.write(SHOULDER_UP);
  myElbow.write(ELBOW_RETRACT);
  myGripper.write(GRIPPER_OPEN);
  
  // 留出 5 秒钟，让你把小车放到起跑线上，并放好目标物块
  Serial.println("准备就绪！5 秒后开始冲刺抓取！");
  delay(5000); 
}

void loop() {
  // 如果任务还没做完，就开始流水线作业
  if (!taskCompleted) {
    Serial.println(">>> 联合抓取任务开始 <<<");
    
    // ==========================================
    // 阶段一：底盘寻的 (前进 1 米)
    // ==========================================
    Serial.println("1. 小车前进中...");
    moveForward();
    delay(TIME_1_METER); // 持续通电前进设定的时间
    
    stopCar();
    Serial.println("到达目标点，紧急刹车！");
    delay(1500); // 停稳，等车身不晃了再动手
    
    // ==========================================
    // 阶段二：探手抓取
    // ==========================================
    Serial.println("2. 机械臂下探...");
    myElbow.write(ELBOW_EXTEND);
    delay(800);
    myShoulder.write(SHOULDER_DOWN);
    delay(800);
    
    Serial.println("3. 夹爪闭合，死死咬住...");
    myGripper.write(GRIPPER_CLOSE);
    delay(1000);
    
    // ==========================================
    // 阶段三：安全收回
    // ==========================================
    Serial.println("4. 举起战利品，收回手臂...");
    myShoulder.write(SHOULDER_UP);
    delay(800);
    myElbow.write(ELBOW_RETRACT);
    delay(800);
    
    Serial.println(">>> 任务圆满成功！引擎熄火，原地待命 <<<");
    // 闭锁，防止 loop 循环导致车子一直往前开
    taskCompleted = true; 
  }
}

// --- 电机底层控制函数 ---
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
