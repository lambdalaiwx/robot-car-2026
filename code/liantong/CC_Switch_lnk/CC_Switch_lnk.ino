// ============================================================
// 智能搬运机器人 - 串口通信双向握手测试 (Ping-Pong)
// ============================================================
#include <SoftwareSerial.h>

// --- 1. 串口通信定义 ---
// Bigfish A0 (RX) 连 OpenMV P4 (TX)
// Bigfish A1 (TX) 连 OpenMV P5 (RX)
SoftwareSerial OpenMVSerial(A0, A1); 
String inputString = "";         
boolean stringComplete = false;  

// --- 2. 底盘电机引脚定义 ---
const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

const unsigned long TIME_1_METER = 3000; // 前进1米的时间(毫秒)

// --- 3. 状态机流程控制 ---
enum TaskState {
  START_DELAY,    // 开机等待
  MOVE_FIRST_1M,  // 第一次前进 1 米
  SEND_PING,      // 发送握手信号 PING
  WAIT_PONG,      // 等待回复 PONG
  MOVE_SECOND_1M, // 通信成功，第二次前进 1 米
  COMM_FAILED,    // 通信失败，原地待命
  TASK_DONE       // 任务结束
};
TaskState currentState = START_DELAY; 

unsigned long stateStartTime = 0;

void setup() {
  Serial.begin(9600);          // 电脑调试串口
  OpenMVSerial.begin(9600);    // 初始化 A0/A1 软串口
  
  // 初始化电机
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();
  
  Serial.println(">>> 通信验证测试程序已启动 <<<");
  Serial.println("5秒后小车将进行第一次移动...");
  stateStartTime = millis();
}

void loop() {
  // --- 持续监听 OpenMV 传回的数据 ---
  while (OpenMVSerial.available()) {
    char inChar = (char)OpenMVSerial.read();
    if (inChar != '\n') {
      inputString += inChar;
    } else {
      stringComplete = true;
    }
  }

  // --- 测试流程状态机 ---
  switch (currentState) {
    
    case START_DELAY:
      // 开机延时 5 秒
      if (millis() - stateStartTime >= 5000) {
        Serial.println("1. 开始第一次前进 (1米)...");
        moveForward();
        stateStartTime = millis();
        currentState = MOVE_FIRST_1M;
      }
      break;

    case MOVE_FIRST_1M:
      // 走满 1 米的时间后刹车
      if (millis() - stateStartTime >= TIME_1_METER) {
        stopCar();
        Serial.println("第一次前进结束，准备测试通信链路...");
        delay(1500); // 停稳缓冲
        currentState = SEND_PING;
      }
      break;

    case SEND_PING:
      Serial.println("2. 正在呼叫 OpenMV (发送 PING)...");
      OpenMVSerial.print("PING\n"); // 发送握手信号
      inputString = "";             // 清空接收缓存
      stringComplete = false;
      stateStartTime = millis();    // 记录开始等待的时间
      currentState = WAIT_PONG;
      break;

    case WAIT_PONG:
      // 检查是否收到了完整的字符串
      if (stringComplete) {
        inputString.trim();
        if (inputString == "PONG") {
          Serial.println("✅ 通信成功！收到 OpenMV 的 PONG 回复！");
          Serial.println("3. 开始第二次前进 (1米)...");
          moveForward();
          stateStartTime = millis();
          currentState = MOVE_SECOND_1M;
        } else {
          Serial.print("收到未知回复: ");
          Serial.println(inputString);
          inputString = "";
          stringComplete = false;
        }
      }
      
      // 超时判断：如果等了 5 秒还没收到 PONG
      if (millis() - stateStartTime > 5000 && currentState == WAIT_PONG) {
        Serial.println("❌ 通信失败！5秒内未收到 OpenMV 回复，请检查接线和供电！");
        currentState = COMM_FAILED;
      }
      break;

    case MOVE_SECOND_1M:
      // 通信成功后的第二次前进
      if (millis() - stateStartTime >= TIME_1_METER) {
        stopCar();
        Serial.println("✅ 第二次前进结束！全流程验证完美通过！");
        currentState = TASK_DONE;
      }
      break;

    case COMM_FAILED:
      // 失败了就原地趴窝，不执行任何动作
      stopCar();
      break;

    case TASK_DONE:
      // 成功做完，原地熄火
      stopCar();
      break;
  }
}

// --- 底层电机驱动 ---
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
