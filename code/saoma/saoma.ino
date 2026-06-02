// ============================================================
// 智能搬运机器人 - 全自动连贯任务（行驶->视觉扫码->联合抓取）
// ============================================================
#include <SoftwareSerial.h>
#include <Servo.h>

// --- 1. 串口通信定义 ---
// Nano A0 (RX) 连 OpenMV P4 (TX)
// Nano A1 (TX) 连 OpenMV P5 (RX)
SoftwareSerial OpenMVSerial(A0, A1); 
String inputString = "";         
boolean stringComplete = false;  

// --- 2. 机械臂引脚与安全角度 ---
Servo myBase, myGripper, myShoulder, myElbow;
const int PIN_BASE = 3;
const int PIN_GRIPPER = 4;
const int PIN_SHOULDER = 8;
const int PIN_ELBOW = 12;

const int GRIPPER_OPEN = 90;    
const int GRIPPER_CLOSE = 60;   
const int SHOULDER_UP = 90;     
const int SHOULDER_DOWN = 45;   
const int ELBOW_RETRACT = 90;   
const int ELBOW_EXTEND = 120;   
const int BASE_CENTER = 90;     

// --- 3. 底盘电机引脚定义 ---
const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

const unsigned long TIME_1_METER = 3000; // 跑1米大概的时间(自行微调)

// --- 4. 状态机流程控制 ---
enum TaskState {
  START_DELAY,    // 开机等待
  MOVE_FORWARD,   // 底盘前进
  TRIGGER_SCAN,   // 触发OpenMV扫码
  WAIT_SCAN,      // 等待扫码结果
  ARM_GRAB,       // 机械臂抓取
  TASK_DONE       // 任务结束
};
TaskState currentState = START_DELAY; 
unsigned long stateStartTime = 0;

void setup() {
  Serial.begin(9600);          
  OpenMVSerial.begin(9600);    
  
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();
  
  myBase.attach(PIN_BASE);
  myGripper.attach(PIN_GRIPPER);
  myShoulder.attach(PIN_SHOULDER);
  myElbow.attach(PIN_ELBOW);
  
  // 机械臂复位防御姿态
  myBase.write(BASE_CENTER);
  myShoulder.write(SHOULDER_UP);
  myElbow.write(ELBOW_RETRACT);
  myGripper.write(GRIPPER_OPEN);
  
  Serial.println(">>> 联合抓取系统启动，5秒后小车冲刺 <<<");
  stateStartTime = millis();
}

void loop() {
  // 核心监听：实时接收 OpenMV 传回的数据
  while (OpenMVSerial.available()) {
    char inChar = (char)OpenMVSerial.read();
    if (inChar != '\n') {
      inputString += inChar;
    } else {
      stringComplete = true;
    }
  }

  // 状态机流转
  switch (currentState) {
    
    case START_DELAY:
      if (millis() - stateStartTime >= 5000) {
        Serial.println("1. 底盘前进 1 米...");
        moveForward();
        stateStartTime = millis();
        currentState = MOVE_FORWARD;
      }
      break;

    case MOVE_FORWARD:
      if (millis() - stateStartTime >= TIME_1_METER) {
        stopCar();
        Serial.println("到达位置，刹车。准备扫码...");
        delay(1500); // 等车身静止，防止画面糊掉
        currentState = TRIGGER_SCAN;
      }
      break;

    case TRIGGER_SCAN:
      Serial.println("2. 向 OpenMV 发送 SCAN 指令...");
      OpenMVSerial.print("SCAN\n"); 
      inputString = "";             
      stringComplete = false;
      currentState = WAIT_SCAN;
      break;

    case WAIT_SCAN:
      if (stringComplete) {
        inputString.trim();
        Serial.print("✅ 扫码成功！收到物块标识: ");
        Serial.println(inputString);
        
        Serial.println("3. 开始联合抓取动作...");
        currentState = ARM_GRAB;
      }
      break;

    case ARM_GRAB:
      Serial.println("-> 伸出小臂");
      myElbow.write(ELBOW_EXTEND);
      delay(800);
      
      Serial.println("-> 下探大臂");
      myShoulder.write(SHOULDER_DOWN);
      delay(800);
      
      Serial.println("-> 夹爪闭合抓取");
      myGripper.write(GRIPPER_CLOSE);
      delay(1000);
      
      Serial.println("-> 举起战利品");
      myShoulder.write(SHOULDER_UP);
      delay(800);
      
      Serial.println("-> 收回小臂");
      myElbow.write(ELBOW_RETRACT);
      delay(800);
      
      Serial.println(">>> 全流程任务圆满完成！ <<<");
      currentState = TASK_DONE;
      break;

    case TASK_DONE:
      stopCar();
      break;
  }
}

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