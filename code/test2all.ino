// =================================================================
// 2026 CRAIC 全地形分拣赛道 - 四轮底盘【大轮永远在前·精准方向校正版】
// =================================================================
#include <EEPROM.h>

// =================== 【参数调校区（现场根据跑车角度微调）】 ===================

// 1. 速度控制 (0 - 255)
const int SPEED_NORMAL      = 120;   // 大轮在前，直线平稳行驶的速度
const int SPEED_SCAN        = 80;    // 过弯后，识别收纳筐时的“缓慢行走”速度（方便传感器捕捉）
const int SPEED_GRID        = 255;   // 冲 3cm 栅格区，全速硬冲
const int SPEED_TURN_STRONG = 160;   // 弯道原地大转弯速度（高爆发力，防打滑转不够）
const int SPEED_RESET_TURN  = 130;   // 终点回正时的转弯速度

// 2. 前半程盲走时间调节 (大轮在前行驶，单位：毫秒 ms)
const unsigned long TIME_START_TO_GRID   = 1200; // 1. 从等待区启动到刚触碰栅格的时间
const unsigned long TIME_PASS_GRID       = 2000; // 2. 冲过整个栅格区域的时间
const unsigned long TIME_GRID_TO_EDGE    = 1000; // 3. 冲出栅格后，继续往前开到半圆切线边缘的时间

// 【核心调校：折线代替半圆弯道】
// 如果弯道过完后，小车还没有和直跑道平行（转不够）：请逐步【增大】TIME_TURN_90_A 和 TIME_TURN_90_B
const unsigned long TIME_TURN_90_A       = 850;  // 4. 第一次原地右转（加大时间，确保车头彻底转到朝下）
const unsigned long TIME_CROSS_440MM     = 1350; // 5. 笔直向下跨越两跑道间距（440mm）所需的时间
const unsigned long TIME_TURN_90_B       = 850; // 6. 第二次原地右转（加大时间，确保车头彻底转到朝右，与跑道平行）

const unsigned long TIME_TURN_TO_BOX1    = 600;  // 7. 切入平直道后，开到接近一号收纳箱前的时间

// 3. 盒子固定间距缓慢行走时间 (中心间距 390mm)
const unsigned long TIME_BETWEEN_BOXES   = 1800; // 在 SPEED_SCAN 慢速下，挪动到下一个箱子的时间

// 4. 后半程回程与回正盲走时间
const unsigned long TIME_BOX_TO_CORNER   = 900;  // 8. 从目标收纳箱往前开到右下角直角弯的时间
const unsigned long TIME_RIGHT_ANGLE_TURN= 850;  // 9. 右下角直角拐弯（原地右转 90 度面向等待区）的时间
const unsigned long TIME_CORNER_TO_START = 1600; // 10. 笔直冲回等待区精准停下的时间

// 【核心修正：终点顺时针右转 90° 回正】
// 如果终点回正的角度大或者小了，微调这个时间，直到小车恢复到最初出发时的姿态
const unsigned long TIME_RESET_TURN_R90  = 850;  // 11. 在等待区原地【顺时针右转 90度】回到初始状态的时间

// =================== 【硬件引脚严格定义】 ===================
const int MOTOR_L1 = 5;  // 左轮红线
const int MOTOR_L2 = 6;  // 左轮黑线
const int MOTOR_R1 = 9;  // 右轮红线
const int MOTOR_R2 = 10; // 右轮黑线

// 颜色传感器引脚
const int TCS_S0  = A0; const int TCS_S1  = A1;
const int TCS_S2  = A4; const int TCS_S3  = A5;
const int TCS_OUT = 2;  const int TCS_LED = A3;

const int LED_INDICATOR = 13; 

// 循环系统变量
int current_step = 0;        
int box_index = 1;          
int green_base_box = 0;     
int total_cycles = 0;       

void setup() {
  Serial.begin(9600);
  pinMode(LED_INDICATOR, OUTPUT); digitalWrite(LED_INDICATOR, LOW);
  
  pinMode(MOTOR_L1, OUTPUT); pinMode(MOTOR_L2, OUTPUT);
  pinMode(MOTOR_R1, OUTPUT); pinMode(MOTOR_R2, OUTPUT);
  stopCar();
  
  pinMode(TCS_S0, OUTPUT); pinMode(TCS_S1, OUTPUT);
  pinMode(TCS_S2, OUTPUT); pinMode(TCS_S3, OUTPUT);
  pinMode(TCS_LED, OUTPUT); pinMode(TCS_OUT, INPUT);
  
  digitalWrite(TCS_S0, HIGH); digitalWrite(TCS_S1, LOW); // 20% 频率缩放
  digitalWrite(TCS_LED, HIGH); // 打开颜色传感器补光灯
  
  green_base_box = 0; 
  Serial.println("--- 大轮在前·终点顺时针右转版闭环系统就绪 ---");
  current_step = 0;   
}

void loop() {
  switch (current_step) {
    
    // -----------------------------------------------------------
    // 步骤 0: 初始状态（大轮已朝前，放置台在正后方），直接抓取，不进行任何旋转
    // -----------------------------------------------------------
    case 0:
      stopCar();
      Serial.println("[Step 0] Position ready, big wheels forward. Arm grabbing...");
      
      // 【机械臂动作插槽】：从正后方的放置区抓取物料
      executeArmGrab(); 
      
      delay(1000);       
      current_step = 1;  // 抓取完，无需旋转，直接大轮在前点火平稳出发
      break;

    // -----------------------------------------------------------
    // 步骤 1: 闭环前半程（平稳向前冲栅格 -> 强力连续两次右转过弯 -> 驶入平直道）
    // -----------------------------------------------------------
    case 1:
      Serial.println("[Step 1] Moving smoothly forward to Grid...");
      moveForward(SPEED_NORMAL); // 平稳向前走
      delay(TIME_START_TO_GRID);      
      
      Serial.println("[Step 1] Smashing the Grid...");
      moveForward(SPEED_GRID);
      delay(TIME_PASS_GRID);          // 全速跨越栅格
      
      moveForward(SPEED_NORMAL);
      delay(TIME_GRID_TO_EDGE);       
      stopCar(); delay(300);          // 刹车稳定姿态
      
      // 执行第一次右转：原地强力右转，让车头朝下
      Serial.println("[Step 1] Over-curve Turn 1 (Right 90)...");
      turnRight(SPEED_TURN_STRONG);   
      delay(TIME_TURN_90_A);          
      stopCar(); delay(300);
      
      // 笔直向下跨越平行间距
      Serial.println("[Step 1] Driving straight cross 440mm...");
      moveForward(SPEED_NORMAL);
      delay(TIME_CROSS_440MM);        
      stopCar(); delay(300);
      
      // 执行第二次右转：【核心增强点】强力右转，使其强行转够角度，与平直道完全平行
      Serial.println("[Step 1] Over-curve Turn 2 (Right 90) -> Parallel alignment...");
      turnRight(SPEED_TURN_STRONG);   
      delay(TIME_TURN_90_B);          // 转完后车头完美朝右，与直跑道平行
      stopCar(); delay(300);
      
      // 向前靠拢到接近 1 号收纳箱位置
      moveForward(SPEED_NORMAL);
      delay(TIME_TURN_TO_BOX1);       
      stopCar(); delay(400);          
      
      box_index = 1;
      current_step = 2;               // 切换到步骤2：缓慢行走并识别
      break;

    // -----------------------------------------------------------
    // 步骤 2: 缓慢行走识别 + 精准投放（大轮在前）
    // -----------------------------------------------------------
    case 2:
      if (green_base_box == 0) {
        // 【第一圈】：大轮在前缓慢行走，同时颜色模块实时识别绿色收纳筐
        Serial.print("First Lap - Slow scanning Box "); Serial.println(box_index);
        
        moveForward(SPEED_SCAN); // 保持缓慢行走
        
        unsigned long scan_start_time = millis();
        bool found_green_in_this_box = false;
        
        while (millis() - scan_start_time < TIME_BETWEEN_BOXES) {
          if (checkIfGreenSign()) {
            found_green_in_this_box = true;
            break; // 识别到了绿色，立刻跳出滑行
          }
          delay(20); 
        }
        
        stopCar(); // 识别到后停止
        delay(400);
        
        if (found_green_in_this_box) {
          Serial.println("--> [GREEN FOUND!] Locker engaged.");
          green_base_box = box_index; // 锁定基准
          
          digitalWrite(LED_INDICATOR, HIGH);
          executeArmRelease();        // 放置物料到筐里
          digitalWrite(LED_INDICATOR, LOW);
          
          current_step = 3;           // 前往步骤3回程
        } else {
          if (box_index < 3) {
            box_index++;              // 去下一个箱子扫描
          } else {
            green_base_box = 3;
            executeArmRelease();
            current_step = 3;
          }
        }
      } 
      else {
        // 【第 2 圈及以后】：记忆直接对位投放
        Serial.print("Base Lap - Directly moving to Box "); Serial.println(green_base_box);
        
        if (green_base_box == 1) {
          // 已经在 1 号箱区域
        } else if (green_base_box == 2) {
          moveForward(SPEED_NORMAL); delay(TIME_BETWEEN_BOXES); stopCar();
        } else if (green_base_box == 3) {
          moveForward(SPEED_NORMAL); delay(TIME_BETWEEN_BOXES * 2); stopCar();
        }
        delay(400);
        executeArmRelease(); // 停止并放置物料
        current_step = 3;
      }
      break;

    // -----------------------------------------------------------
    // 步骤 3: 闭环后半程（继续直行 -> 右转直角弯 -> 冲回等待区 -> 顺时针旋转90°回正）
    // -----------------------------------------------------------
    case 3:
      Serial.println("[Step 3] Continuing straight after deposit...");
      int current_stop_box = (total_cycles == 0) ? box_index : green_base_box;
      
      // 投完物料后，继续大轮在前直行行走
      moveForward(SPEED_NORMAL);
      if (current_stop_box == 1) {
        delay(TIME_BETWEEN_BOXES * 2 + TIME_BOX_TO_CORNER);
      } else if (current_stop_box == 2) {
        delay(TIME_BETWEEN_BOXES + TIME_BOX_TO_CORNER);
      } else {
        delay(TIME_BOX_TO_CORNER);
      }
      stopCar(); delay(400);
      
      // 右下角执行直角拐弯（原地右转 90 度）
      Serial.println("[Step 3] Right-Angle turning (Right 90)...");
      turnRight(SPEED_TURN_STRONG); 
      delay(TIME_RIGHT_ANGLE_TURN);
      stopCar(); delay(400);
      
      // 直行到等待区
      Serial.println("[Step 3] Moving straight into Loading Zone...");
      moveForward(SPEED_NORMAL);
      delay(TIME_CORNER_TO_START);
      stopCar(); delay(600); // 彻底停在等待区
      
      // 【核心方向修正】：原地【顺时针（右转）旋转 90 度】得到初始状态
      Serial.println("[Step 3] Clockwise Turning 90 deg to Reset...");
      turnRight(SPEED_RESET_TURN);  // 右转即为顺时针旋转，使放置台重新回到正后方
      delay(TIME_RESET_TURN_R90);
      stopCar();
      
      total_cycles++;
      Serial.print(">>> TOTAL CYCLES: "); Serial.println(total_cycles);
      delay(1000); 
      
      current_step = 0; // 状态彻底归零，大轮朝前完好如初，开启下一轮物料循环
      break;
  }
}

// =================== 【底盘底层驱动封装（红黑线严格匹配版）】 ===================

void moveForward(int speed) {
  // 左轮红高黑低，右轮红高黑低 -> 大轮在前方笔直推进
  analogWrite(MOTOR_L1, speed); analogWrite(MOTOR_L2, 0);
  analogWrite(MOTOR_R1, speed); analogWrite(MOTOR_R2, 0);
}

void turnLeft(int speed) {
  // 左后退，右前进 -> 原地逆时针（左）旋转
  analogWrite(MOTOR_L1, 0);     analogWrite(MOTOR_L2, speed);
  analogWrite(MOTOR_R1, speed); analogWrite(MOTOR_R2, 0);
}

void turnRight(int speed) {
  // 左前进，右后退 -> 原地顺时针（右）旋转
  analogWrite(MOTOR_L1, speed); analogWrite(MOTOR_L2, 0);
  analogWrite(MOTOR_R1, 0);     analogWrite(MOTOR_R2, speed);
}

void stopCar() {
  analogWrite(MOTOR_L1, 0); analogWrite(MOTOR_L2, 0);
  analogWrite(MOTOR_R1, 0); analogWrite(MOTOR_R2, 0);
}

// =================== 【动作功能占位（代码已省略）】 ===================
void executeArmGrab() {
  Serial.println(">>> 机械臂正在从正后方放置区抓取物料...");
  delay(2000); 
}

void executeArmRelease() {
  Serial.println(">>> 停止，机械臂放置物料到收纳筐中...");
  delay(2000); 
}

// =================== 【颜色传感器检测通道】 ===================
bool checkIfGreenSign() {
  digitalWrite(TCS_S2, HIGH); digitalWrite(TCS_S3, HIGH); // 锁定绿色通道
  long duration = pulseIn(TCS_OUT, LOW, 20000); 
  
  if (duration > 15 && duration < 135) { 
    return true; 
  }
  return false; 
}