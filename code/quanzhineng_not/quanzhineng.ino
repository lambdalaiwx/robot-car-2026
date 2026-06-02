#include <SoftwareSerial.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

SoftwareSerial OpenMVSerial(A0, A1); 
String inputString = "";         
boolean stringComplete = false;  

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo myBase, myGripper, myShoulder, myElbow;
const int PIN_BASE = 3;
const int PIN_GRIPPER = 4;
const int PIN_SHOULDER = 8;
const int PIN_ELBOW = 12;

// --- 抓取与箩筐位姿定义 (需根据你的车模机械结构微调) ---
const int GRIPPER_OPEN = 90;    
const int GRIPPER_CLOSE = 60;   
const int SHOULDER_UP = 90;     
const int SHOULDER_DOWN = 45;   
const int ELBOW_RETRACT = 90;   
const int ELBOW_EXTEND = 120;   

const int BASE_CENTER = 90;   // 机械臂朝前 (抓物料)
const int BASE_BASKET = 180;  // 机械臂朝后或侧面 (放进车内箩筐)

const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

// --- 任务队列变量 ---
char taskList[3];       // 用来存扫码出来的3个数字，比如 '1', '2', '3'
int currentTaskIdx = 0; // 当前做到第几个任务了 (0, 1, 2)

enum TaskState {
  START_DELAY,
  TRIGGER_SCAN,
  WAIT_SCAN,
  SEEK_MATERIAL,   // 寻找物料
  WAIT_ALIGN,      // 等待OpenMV说"OK"(对准了)
  ARM_GRAB_DROP,   // 抓取并放入箩筐
  TASK_DONE
};
TaskState currentState = START_DELAY; 
unsigned long stateStartTime = 0;

void setup() {
  Serial.begin(9600);          
  OpenMVSerial.begin(9600);    

  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    updateOLED("System Ready", "Wait 5s...");
  }
  
  pinMode(MOTOR_LEFT_F, OUTPUT); pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT); pinMode(MOTOR_RIGHT_B, OUTPUT);
  stopCar();
  
  myBase.attach(PIN_BASE); myGripper.attach(PIN_GRIPPER);
  myShoulder.attach(PIN_SHOULDER); myElbow.attach(PIN_ELBOW);
  
  armReset(); // 机械臂归位
  stateStartTime = millis();
}

void loop() {
  while (OpenMVSerial.available()) {
    char inChar = (char)OpenMVSerial.read();
    if (inChar != '\n') {
      inputString += inChar;
    } else {
      stringComplete = true;
    }
  }

  switch (currentState) {
    
    case START_DELAY:
      if (millis() - stateStartTime >= 5000) {
        currentState = TRIGGER_SCAN;
      }
      break;

    case TRIGGER_SCAN:
      OpenMVSerial.print("SCAN\n"); 
      inputString = ""; stringComplete = false;
      updateOLED("Phase 1", "Scan QR...");
      currentState = WAIT_SCAN;
      break;

    case WAIT_SCAN:
      if (stringComplete) {
        inputString.trim();
        // 假设收到了 "123"
        if(inputString.length() >= 3) {
          taskList[0] = inputString.charAt(0);
          taskList[1] = inputString.charAt(1);
          taskList[2] = inputString.charAt(2);
          
          updateOLED("QR Found:", inputString);
          delay(2000);
          currentState = SEEK_MATERIAL;
        } else {
          // 扫码有误，重试
          inputString = ""; stringComplete = false;
          OpenMVSerial.print("SCAN\n");
        }
      }
      break;

    case SEEK_MATERIAL:
      if (currentTaskIdx < 3) {
        // 向 OpenMV 发送当前要找的颜色指令
        String cmd = "FIND_";
        cmd += taskList[currentTaskIdx]; 
        cmd += "\n";
        OpenMVSerial.print(cmd);
        
        inputString = ""; stringComplete = false;
        
        String oledMsg = "Find Color "; oledMsg += taskList[currentTaskIdx];
        updateOLED("Phase 2", oledMsg);
        
        // 启动底盘慢速前进，寻找物料
        moveForwardSlowly();
        currentState = WAIT_ALIGN;
      } else {
        // 3个全抓完了
        updateOLED("Mission", "ALL DONE!");
        currentState = TASK_DONE;
      }
      break;

    case WAIT_ALIGN:
      // 小车在前进，OpenMV 在判断。如果收到 "OK"，说明对准了
      if (stringComplete) {
        inputString.trim();
        if (inputString == "OK") {
          stopCar(); // 刹车！
          updateOLED("Aligned!", "Grabbing...");
          currentState = ARM_GRAB_DROP;
        }
        inputString = ""; stringComplete = false;
      }
      break;

    case ARM_GRAB_DROP:
      // 1. 下探抓取
      myElbow.write(ELBOW_EXTEND); delay(800);
      myShoulder.write(SHOULDER_DOWN); delay(800);
      myGripper.write(GRIPPER_CLOSE); delay(1000);
      myShoulder.write(SHOULDER_UP); delay(800);
      myElbow.write(ELBOW_RETRACT); delay(800);
      
      // 2. 转身放入车内箩筐
      updateOLED("Putting", "into Basket");
      myBase.write(BASE_BASKET); // 转向箩筐
      delay(1500); // 给舵机一点时间转过去
      
      myShoulder.write(SHOULDER_DOWN); delay(800); // 可以视情况决定要不要下放
      myGripper.write(GRIPPER_OPEN); delay(1000);  // 松爪，掉入箩筐
      
      // 3. 复位，准备抓下一个
      armReset();
      delay(1000);
      
      // 任务索引 +1，进入下一轮循环
      currentTaskIdx++;
      currentState = SEEK_MATERIAL; 
      break;

    case TASK_DONE:
      stopCar();
      break;
  }
}

// --- 动作与显示辅助函数 ---
void armReset() {
  myShoulder.write(SHOULDER_UP);
  myElbow.write(ELBOW_RETRACT);
  myGripper.write(GRIPPER_OPEN);
  delay(500); // 先把手收回来，再转底座防止打到东西
  myBase.write(BASE_CENTER);
}

void moveForwardSlowly() {
  // 可以适当用 PWM (analogWrite) 降低速度，防止开太快冲过头
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

void updateOLED(String line1, String line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5);
  display.println(line1);
  
  display.setTextSize(2);
  display.setCursor(0, 30);
  display.println(line2);
  display.display();
}