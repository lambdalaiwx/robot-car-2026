#include <SoftwareSerial.h>
#include <U8x8lib.h>
#include <Servo.h>

// ==========================================
// 硬件引脚与对象定义区 
// ==========================================

// 1. OpenMV 与 OLED 通信
SoftwareSerial OpenMVSerial(A0, A1); 
U8X8_SH1106_128X64_NONAME_HW_I2C display(/* reset=*/ U8X8_PIN_NONE);

// 2. 底盘电机物理连线
const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

// 3. 机械臂引脚与对象
Servo myBase;      // 第一关节
Servo myShoulder;  // 第二关节
Servo myElbow;     // 第三关节
Servo myGripper;   // 夹爪   
const int PIN_BASE = 3;      
const int PIN_SHOULDER = 4;
const int PIN_ELBOW = 8;
const int PIN_GRIPPER = 12;

// 4. 机械臂已测定角度常数 (需根据实际抓取位置微调)
const int INIT_BASE = 120;     
const int INIT_SHOULDER = 15;  
const int INIT_ELBOW = 80;     
const int INIT_GRIPPER = 90; 

// 抓取动作预设角度 (结合之前的量程数据)
const int GRIPPER_OPEN = 90;
const int GRIPPER_CLOSE = 30;
const int BASE_CENTER = 120;  // 假设正前方面对物块
const int BASE_BASKET = 180;  // 假设 180 度是放进箩筐
const int SHOULDER_DOWN = 100; // 下探时大臂角度 (需实测)
const int ELBOW_EXTEND = 20;   // 下探时小臂角度 (需实测)

// ==========================================
// 配置与状态区
// ==========================================

float left_ratio = 0.95;  
float right_ratio = 1.0;  
const int SPEED_FULL = 255;  
const int SPEED_SLOW = 80;   

// --- 扩充任务状态机 ---
enum TaskState {
  WAITING_START, 
  FAST_FORWARD,  
  SLOW_CRUISE,   
  QR_SCANNED,    // 扫码成功
  WAIT_3S,       // 停下3秒
  FORWARD_1S,    // 前进1秒
  SEEK_BLOCK,    // 寻找物块
  WAIT_ALIGN,    // 等待对准
  GRABBING,      // 夹取物块
  ALL_DONE       // 所有任务完成
};
TaskState currentState = WAITING_START;
unsigned long stateTimer = 0; // 用于非阻塞延时记录时间

String inputString = "";         
boolean stringComplete = false;  

// 扫码数据存储
String qrData = "";
int currentTaskIdx = 0; // 当前做到第几个数字了

// ==========================================
// 初始化 Setup
// ==========================================
void setup() {
  Serial.begin(9600);
  OpenMVSerial.begin(9600);
  
  display.begin();
  display.setPowerSave(0);
  display.setFont(u8x8_font_chroma48medium8_r);
  
  // 电机引脚初始化
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();
  
  // --- 🦾 阶段 1：机械臂占用 Timer1 执行安全复位 ---
  display.clear();
  display.drawString(0, 1, "Arm Resetting...");
  Serial.println("========================================");
  Serial.println("机械臂占用定时器，执行全关节安全复位...");
  
  myBase.attach(PIN_BASE);      myBase.write(INIT_BASE);
  myShoulder.attach(PIN_SHOULDER); myShoulder.write(INIT_SHOULDER);
  myElbow.attach(PIN_ELBOW);    myElbow.write(INIT_ELBOW);
  myGripper.attach(PIN_GRIPPER);  myGripper.write(INIT_GRIPPER);
  delay(1500); 
  
  // --- ⚠️ 阶段 2：强制释放与重置 Timer1 ---
  Serial.println("复位完成！释放控制权，拯救底盘 PWM...");
  detachAndRestorePWM(); // 封装成了函数，方便后面复用
  
  display.drawString(0, 3, "3s to RUN...");
  delay(3000);
  
  // --- 🏎️ 阶段 3：底盘起步走位硬编码 ---
  display.clear();
  display.drawString(0, 1, "Auto Routing...");
  
  moveForward(150); delay(600); stopCar(); delay(1000);
  turnRight(150); delay(650); stopCar(); delay(1000);
  moveForward(150); delay(500); stopCar(); delay(1000);  
  turnLeft(150); delay(450); stopCar(); delay(2000);
  
  // --- 🚀 正式启动全速视觉巡航 ---
  display.clear();
  display.drawString(0, 1, "FAST Mode >>>");
  moveForward(SPEED_FULL);
  currentState = FAST_FORWARD;
}

// ==========================================
// 主循环 Loop (状态机驱动)
// ==========================================
void loop() {
  // 1. 接收 OpenMV 串口数据
  while (OpenMVSerial.available()) {
    char inChar = (char)OpenMVSerial.read();
    if (inChar != '\n') {
      inputString += inChar;
    } else {
      inputString.trim();
      stringComplete = true;
    }
  }

  // 2. 根据当前状态执行逻辑
  switch (currentState) {

    case FAST_FORWARD:
      if (stringComplete && inputString == "SLOW") {
        Serial.println("⚠️ 发现二维码轮廓，减速...");
        display.clear();
        display.drawString(0, 1, "SLOW Mode >  ");
        moveForward(SPEED_SLOW); 
        currentState = SLOW_CRUISE;
      }
      break;

    case SLOW_CRUISE:
      if (stringComplete && inputString != "SLOW" && inputString.length() >= 3) {
        Serial.print("✅ 扫码完成，数据: ");
        Serial.println(inputString);
        stopCar();
        
        qrData = inputString; // 保存扫到的数字，如 "123"
        currentTaskIdx = 0;   // 进度清零
        
        // OLED 永久保留该信息
        display.clear();
        display.drawString(0, 1, "Data Locked:");
        display.draw2x2String(0, 4, qrData.c_str());
        
        stateTimer = millis();
        currentState = WAIT_3S;
      }
      break;

    case WAIT_3S:
      // 停下3秒
      if (millis() - stateTimer >= 3000) {
        Serial.println("3秒完毕，开始前进1秒...");
        moveForward(150); 
        stateTimer = millis();
        currentState = FORWARD_1S;
      }
      break;

    case FORWARD_1S:
      // 前进1秒
      if (millis() - stateTimer >= 1000) {
        stopCar();
        Serial.println("前进结束，进入寻物模式。");
        currentState = SEEK_BLOCK;
      }
      break;

    case SEEK_BLOCK:
      if (currentTaskIdx < qrData.length()) {
        char target = qrData.charAt(currentTaskIdx);
        
        // 1对应红, 2对应绿, 3对应蓝
        if (target == '1') OpenMVSerial.print("FIND_R\n");
        else if (target == '2') OpenMVSerial.print("FIND_G\n");
        else if (target == '3') OpenMVSerial.print("FIND_B\n");
        
        Serial.print("告诉OpenMV去寻找: "); Serial.println(target);
        
        moveForward(SPEED_SLOW); // 缓慢前进寻找
        currentState = WAIT_ALIGN;
      } else {
        // 所有数字都抓完了
        currentState = ALL_DONE;
      }
      break;

    case WAIT_ALIGN:
      if (stringComplete && inputString == "OK") {
        stopCar();
        Serial.println("✅ 物块已对准！开始抓取！");
        currentState = GRABBING;
      }
      break;

    case GRABBING:
      // 因为抓取过程不需要底盘移动，我们可以直接用 delay() 实现这套连招
      
      // 1. 重新挂载舵机
      myBase.attach(PIN_BASE);      myBase.write(BASE_CENTER);
      myShoulder.attach(PIN_SHOULDER); myShoulder.write(INIT_SHOULDER);
      myElbow.attach(PIN_ELBOW);    myElbow.write(INIT_ELBOW);
      myGripper.attach(PIN_GRIPPER);  myGripper.write(GRIPPER_OPEN);
      delay(800);
      
      // 2. 下探与夹紧 (这里的 SHOULDER_DOWN 和 ELBOW_EXTEND 需要你根据车身高度实测修改)
      myElbow.write(ELBOW_EXTEND);  delay(600);
      myShoulder.write(SHOULDER_DOWN); delay(600);
      myGripper.write(GRIPPER_CLOSE); delay(800);
      
      // 3. 抬起并转向箩筐
      myShoulder.write(INIT_SHOULDER); delay(600);
      myElbow.write(INIT_ELBOW);    delay(600);
      myBase.write(BASE_BASKET);    delay(1000);
      
      // 4. 松爪释放
      myGripper.write(GRIPPER_OPEN); delay(800);
      
      // 5. 回到初始安全位
      myBase.write(INIT_BASE); delay(800);
      myGripper.write(INIT_GRIPPER); delay(500);
      
      // 6. 脱离舵机，恢复底盘 PWM，准备下一次移动
      detachAndRestorePWM();
      
      // 7. 进入下一个物块的任务
      currentTaskIdx++;
      currentState = SEEK_BLOCK;
      break;

    case ALL_DONE:
      stopCar();
      display.clear();
      display.drawString(0, 1, "Mission");
      display.draw2x2String(0, 4, "COMPLETE");
      break;
  }

  // 3. 一轮循环结束，如果数据被处理完了，清空缓存
  if (stringComplete) {
    inputString = "";
    stringComplete = false;
  }
}

// ==========================================
// 辅助与核心控制函数
// ==========================================

void detachAndRestorePWM() {
  myBase.detach();
  myShoulder.detach();
  myElbow.detach();
  myGripper.detach();

  // 强制恢复 Arduino Timer1 的标准 8-bit PWM 模式
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(CS10);
}

void moveForward(int speed) {
  speed = constrain(speed, 0, 255); 
  int true_left_speed = speed * left_ratio;
  int true_right_speed = speed * right_ratio;
  
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, true_left_speed);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, true_right_speed);
}

void turnLeft(int speed) {
  speed = constrain(speed, 0, 255);
  analogWrite(MOTOR_RIGHT_F, speed);
  analogWrite(MOTOR_RIGHT_B, 0);
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, speed);
}

void turnRight(int speed) {
  speed = constrain(speed, 0, 255);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, speed);
  analogWrite(MOTOR_LEFT_F, speed);
  analogWrite(MOTOR_LEFT_B, 0);
}

void stopCar() {
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, 0);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, 0);
}