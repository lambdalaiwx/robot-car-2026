#include <SoftwareSerial.h>
#include <U8x8lib.h>

// --- 硬件引脚定义 ---
SoftwareSerial OpenMVSerial(A0, A1); 
U8X8_SH1106_128X64_NONAME_HW_I2C display(/* reset=*/ U8X8_PIN_NONE);

const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

// --- ⚙️ 底盘直线行驶校准区 ---
float left_ratio = 0.95;  // 修正偏右问题
float right_ratio = 1.0;  

// --- 速度配置 ---
const int SPEED_FULL = 255;  
const int SPEED_SLOW = 80;   

// --- 状态机定义 ---
enum TaskState {
  WAITING_START, 
  FAST_FORWARD,  
  SLOW_CRUISE,   
  TASK_DONE      
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
  
  // ==========================================
  // 🏎️ 发车前硬编码：S型向左平移变道
  // ==========================================
  display.clear();
  display.drawString(0, 1, "Shifting Left..");
  
  // 1. 原地微偏左转
  turnLeft(120); 
  delay(200);     // 延时决定了转角大小，请根据实际场地微调
  
  // 2. 向左前方直行一小步
  moveForward(120);
  delay(300);     // 延时决定了向左平移的距离
  
  // 3. 原地右转回正车头
  turnRight(120);
  delay(200);     // 理论上和左转的时间保持一致即可回正
  
  // 4. 停稳，准备进入视觉巡航
  stopCar();
  delay(500);     // 缓冲一下，消除车身晃动
  // ==========================================
  
  display.clear();
  display.drawString(0, 1, "FAST Mode >>>");
  moveForward(SPEED_FULL);
  currentState = FAST_FORWARD;
}

void loop() {
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
    if (currentState == FAST_FORWARD && inputString == "SLOW") {
      Serial.println("⚠️ 发现二维码轮廓，开始减速巡航...");
      display.drawString(0, 1, "SLOW Mode >  ");
      moveForward(SPEED_SLOW); 
      currentState = SLOW_CRUISE;
    } 
    else if (currentState == SLOW_CRUISE && inputString != "SLOW") {
      Serial.print("✅ 扫码完成，数据: ");
      Serial.println(inputString);
      
      stopCar();
      
      display.clear();
      display.drawString(0, 1, "Target Found:");
      display.draw2x2String(0, 4, inputString.c_str());
      
      currentState = TASK_DONE;
    }
    inputString = "";
    stringComplete = false;
  }
}

// ================= 核心控制函数 (已适配头尾互换) =================

void moveForward(int speed) {
  speed = constrain(speed, 0, 255); 
  int true_left_speed = speed * left_ratio;
  int true_right_speed = speed * right_ratio;
  
  // 小车实际前进（物理后退引脚生效）
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, true_left_speed);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, true_right_speed);
}

// ⚠️ 新增：左转函数
void turnLeft(int speed) {
  speed = constrain(speed, 0, 255);
  // 正常左转是左轮后退、右轮前进
  // 由于车头尾互换：物理左轮要给 F 信号，物理右轮要给 B 信号
  analogWrite(MOTOR_LEFT_F, speed);
  analogWrite(MOTOR_LEFT_B, 0);
  
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, speed);
}

// ⚠️ 新增：右转函数
void turnRight(int speed) {
  speed = constrain(speed, 0, 255);
  // 正常右转是左轮前进、右轮后退
  // 由于车头尾互换：物理左轮要给 B 信号，物理右轮要给 F 信号
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, speed);
  
  analogWrite(MOTOR_RIGHT_F, speed);
  analogWrite(MOTOR_RIGHT_B, 0);
}

void stopCar() {
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, 0);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, 0);
}