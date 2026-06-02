#include <SoftwareSerial.h>
#include <U8x8lib.h>

// --- 硬件引脚定义 ---
SoftwareSerial OpenMVSerial(A0, A1); 
U8X8_SH1106_128X64_NONAME_HW_I2C display(/* reset=*/ U8X8_PIN_NONE);

const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

// --- 速度配置 (根据你的电池微调) ---
const int SPEED_FULL = 255;  // 全速狂奔
const int SPEED_SLOW = 80;   // 减速巡航 (注意不要低于电机的启动死区)

// --- 状态机定义 ---
enum TaskState {
  WAITING_START, // 等待开机
  FAST_FORWARD,  // 全速前进
  SLOW_CRUISE,   // 减速巡航扫码
  TASK_DONE      // 扫码完成，停车显示
};
TaskState currentState = WAITING_START;

String inputString = "";         
boolean stringComplete = false;  

void setup() {
  Serial.begin(9600);
  OpenMVSerial.begin(9600);
  
  display.begin();
  display.setPowerSave(0);
  display.setFont(u8x8_font_chroma48medium8_r);
  
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();
  
  display.drawString(0, 1, "System Ready");
  display.drawString(0, 3, "3s to RUN...");
  delay(3000);
  
  // 启动！进入全速模式
  display.clear();
  display.drawString(0, 1, "FAST Mode >>>");
  moveForward(SPEED_FULL);
  currentState = FAST_FORWARD;
}

void loop() {
  // 1. 监听 OpenMV 数据
  while (OpenMVSerial.available()) {
    char inChar = (char)OpenMVSerial.read();
    if (inChar != '\n') {
      inputString += inChar;
    } else {
      inputString.trim();
      stringComplete = true;
    }
  }

  // 2. 状态机响应逻辑
  if (stringComplete) {
    if (currentState == FAST_FORWARD && inputString == "SLOW") {
      // --- 收到预警，立刻减速 ---
      Serial.println("⚠️ 发现二维码轮廓，开始减速巡航...");
      display.drawString(0, 1, "SLOW Mode >  ");
      moveForward(SPEED_SLOW); 
      currentState = SLOW_CRUISE;
      
    } 
    else if (currentState == SLOW_CRUISE && inputString != "SLOW") {
      // --- 在减速状态下收到了并非 SLOW 的数据，说明扫码成功 ---
      Serial.print("✅ 扫码完成，数据: ");
      Serial.println(inputString);
      
      stopCar();
      
      // OLED 清屏并大字显示结果
      display.clear();
      display.drawString(0, 1, "Target Found:");
      display.draw2x2String(0, 4, inputString.c_str());
      
      currentState = TASK_DONE;
    }
    
    // 清空缓存，准备接下一次数据
    inputString = "";
    stringComplete = false;
  }
}

// --- 核心调速运动函数 ---
void moveForward(int speed) {
  speed = constrain(speed, 0, 255); 
  analogWrite(MOTOR_LEFT_F, speed);
  analogWrite(MOTOR_LEFT_B, 0);
  analogWrite(MOTOR_RIGHT_F, speed);
  analogWrite(MOTOR_RIGHT_B, 0);
}

void stopCar() {
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, 0);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, 0);
}